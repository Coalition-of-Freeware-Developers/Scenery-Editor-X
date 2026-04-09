/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray 
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * renderer.cpp
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#include "renderer.h"
#include "renderer_buffers.h"
#include "SceneryEditorX/scene/material.h"
#include "vulkan/bindless_manager.h"
#include "vulkan/descriptor_pool_manager.h"
#include "vulkan/swapchain.h"
#include "vulkan/uniform_buffer_set.h"
#include "vulkan/debug/graphics_debug.h"
#include "vulkan/debug/render_doc.h"
#include "vulkan/pipeline/pipeline.h"
#include "vulkan/shader/shader_manager.h"
#include <array>
#include <cstddef>
#include <mutex>
#include <sstream>
#include <thread>
#include <Editor/modules/scene_render.h>
#include <Editor/ui/ui_impl.h>
#include <SDL3/SDL.h>
#include <SceneryEditorX/asset/model.h>
#include <SceneryEditorX/asset/manager/asset_manager.h>
#include <SceneryEditorX/core/application/application.h>
#include <SceneryEditorX/core/window/monitor_data.h>
#include <SceneryEditorX/renderer/gbuffer.h>
#include <SceneryEditorX/scene/camera.h>
#include <SceneryEditorX/scene/entity.h>
#include <SceneryEditorX/scene/scene.h>
#include <slang/slang-com-ptr.h>
#include <slang/slang.h>
#include <volk/volk.h>

// --------------------------------------------------------------

namespace SceneryEditorX
{

#pragma region Static Renderer Properties

	/**
	 * @struct RendererProperties
	 * @brief Holds various properties and resources used by the Renderer.
	 */
	struct RendererProperties
	{
		VkDescriptorSet activeRendererDescriptorSet = nullptr;
		std::vector<VkDescriptorPool> descriptorPools;
		VkDescriptorPool materialDescriptorPool = VK_NULL_HANDLE;
		std::vector<uint32_t> descriptorPoolAllocationCount;
	
		/** Default samplers */
		VkSampler samplerClamp = nullptr;
		VkSampler samplerPoint = nullptr;
	
		int32_t selectedDrawCall = -1;
		int32_t drawCallCount = 0;
	};
	
	// --------------------------------------------------------------

	/*
	void dynamic_resolution()
	{
		if (cvar_dynamic_resolution.GetValue() != 0.0f)
		{
			float gpu_time_target   = 16.67f;                                               // target for 60 FPS
			float adjustment_factor = static_cast<float>(0.05f * DeltaTime::GetDeltaTimeSec()); // how aggressively to adjust screen percentage
			float screen_percentage = cvar_resolution_scale.GetValue();
			float gpu_time          = Profiler::GetTimeGpuLast();

			if (gpu_time < gpu_time_target) // gpu is under target, increase resolution
			{
				screen_percentage += adjustment_factor * (gpu_time_target - gpu_time);
			}
			else // gpu is over target, decrease resolution
			{
				screen_percentage -= adjustment_factor * (gpu_time - gpu_time_target);
			}

			// clamp screen_percentage to a reasonable range
			screen_percentage = xMath::Clamp(screen_percentage, 0.5f, 1.0f);

			ConsoleRegistry::Get().SetValueFromString("r.resolution_scale", std::to_string(screen_percentage));
		}
	}
	*/

	RendererProperties *Renderer::m_Data = nullptr;
	static Ref<Swapchain> s_Swapchain = nullptr;
	static uint64_t s_FrameNumber = 0;

	std::atomic<bool> Renderer::m_ResourcesInitialized = false;
	Scope<Model> Renderer::m_TestModel = nullptr;

	// Bindless draw data
	std::array<ShaderBuffer_DrawData, RENDERER_MAX_DRAW_CALLS> Renderer::m_DrawData_CPU;
	uint32_t Renderer::m_DrawData_Count = 0;
	Renderer::PassState Renderer::m_PassState = {};

	CommandList *Renderer::m_CmdList_Compute = nullptr;
	CommandList *Renderer::m_CmdList_Present = nullptr;
	CommandList *Renderer::m_CmdList_Frame   = nullptr; // per-frame UI command list, submitted alongside swapchain CB
	Scope<AssetManager> Renderer::s_AssetManager = nullptr;
	uint32_t Renderer::m_ResourceIndex = 0;

	// per-frame rotated buffers
	std::array<Renderer::IndirectFrameResource, DRAW_DATA_BUFFER_COUNT> Renderer::m_FrameResources;
	uint32_t Renderer::m_FrameResource_Index = 0;

	Scope<FrameSync> Renderer::m_FrameSync = nullptr;
	Scope<CommandPool> Renderer::m_CommandPool = nullptr;

	std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> Renderer::m_CommandBuffers = {};
	uint32_t Renderer::m_CurrentFrameIndex = 0;
	uint64_t Renderer::m_FrameNumber = 0;
	uint32_t Renderer::m_SwapchainImageIndex = 0;
	bool Renderer::m_FrameInProgress = false;

	// Basic forward pipeline
	VkPipeline Renderer::m_BasicPipeline = VK_NULL_HANDLE;
	VkPipelineLayout Renderer::m_BasicPipelineLayout = VK_NULL_HANDLE;
	Scope<ShaderManager> Renderer::m_BasicShaderManager = nullptr;
	std::array<VkBuffer, MAX_FRAMES_IN_FLIGHT> Renderer::m_BasicShaderDataBuffers = {};
	std::array<VmaAllocation, MAX_FRAMES_IN_FLIGHT> Renderer::m_BasicShaderDataAllocations = {};
	std::array<void *, MAX_FRAMES_IN_FLIGHT> Renderer::m_BasicShaderDataMapped = {};
	std::array<VkDeviceAddress, MAX_FRAMES_IN_FLIGHT> Renderer::m_BasicShaderDataAddresses = {};
	VkPipeline Renderer::m_GridPipeline = VK_NULL_HANDLE;
	VkPipelineLayout Renderer::m_GridPipelineLayout = VK_NULL_HANDLE;
	Scope<ShaderManager> Renderer::m_GridShaderManager = nullptr;
	VkBuffer Renderer::m_GridVertexBuffer = VK_NULL_HANDLE;
	VmaAllocation Renderer::m_GridVertexAllocation = VK_NULL_HANDLE;
	VkBuffer Renderer::m_GridIndexBuffer = VK_NULL_HANDLE;
	VmaAllocation Renderer::m_GridIndexAllocation = VK_NULL_HANDLE;
	uint32_t Renderer::m_GridIndexCount = 0;

#pragma region Camera Properties

	Camera *Renderer::m_Camera = nullptr;
	VkDescriptorSetLayout Renderer::m_CameraDescriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool Renderer::m_CameraDescriptorPool = VK_NULL_HANDLE;
	std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> Renderer::m_CameraDescriptorSets = {};
	std::array<VkBuffer, MAX_FRAMES_IN_FLIGHT> Renderer::m_CameraUboBuffers = {};
	std::array<VmaAllocation, MAX_FRAMES_IN_FLIGHT> Renderer::m_CameraUboAllocations = {};
	std::array<void *, MAX_FRAMES_IN_FLIGHT> Renderer::m_CameraUboMapped = {};

#pragma endregion

#pragma region Resolution & viewport (internal state)

	static xMath::Vec2 s_RendererResolution(0.0f, 0.0f);
	static xMath::Vec2 s_OutputResolution(0.0f, 0.0f);
	static Viewport s_Viewport = Viewport(0, 0, 0, 0);
	static Flag s_OrthoProjection_Dirty;
	static Scope<UniformBufferSet> s_UniformBuffers = nullptr;
	static VkSurfaceCapabilitiesKHR s_SurfaceCaps = {};
	static xMath::Vec2 s_JitterOffset(0.0f, 0.0f);
	static float s_NearPlane = 0.0f;
	static float s_FarPlane  = 1.0f;
	const uint8_t SWAPCHAIN_BUFFER_COUNT = 2;
	const uint32_t RESOLUTION_SHADOW_MIN = 128;
	constexpr uint32_t RENDERER_RESOURCE_FRAME_LIFETIME = MAX_FRAMES_IN_FLIGHT;

#pragma endregion

#pragma region Light Resources

	uint32_t Renderer::m_Count_ActiveLights = 0;
	std::vector<ShadowSlice> Renderer::m_ShadowSlices;

#pragma endregion

#pragma region Frame Synchronization Resources

	static std::vector<Ref<Fence>> s_FenceRefs;
	static std::vector<VkFence> s_FenceHandles;

	static std::vector<Ref<Semaphore>> s_RenderSemaphoreRefs;
	static std::vector<VkSemaphore> s_RenderSemaphoreHandles;
	// Mutex to protect frame-sync container mutations during debugging instrumentation
	static std::mutex s_FrameSyncMutex;

#pragma endregion

#pragma endregion

	static std::filesystem::path ResolveResourcePath(const std::filesystem::path& relativePath)
	{
		const std::filesystem::path cwd = std::filesystem::current_path();
		const std::array<std::filesystem::path, 4> candidates = {
			cwd / relativePath,
			cwd / ".." / relativePath,
			cwd / ".." / ".." / relativePath,
			cwd / ".." / ".." / ".." / relativePath,
		};

		for (const auto& candidate : candidates)
		{
			if (std::filesystem::exists(candidate))
			{
				return std::filesystem::weakly_canonical(candidate);
			}
		}

		return candidates[0];
	}

	/**
	 * @brief Refreshes the raw Vulkan handles from the Ref<> containers.
	 * This function should be called while holding the s_FrameSyncMutex to ensure thread safety.
	 * @note Call while holding s_FrameSyncMutex when possible.
	 */
	static void RefreshSyncHandles()
	{
		std::scoped_lock lock(s_FrameSyncMutex);

		s_FenceHandles.clear();
		s_FenceHandles.reserve(s_FenceRefs.size());
		for (const auto &f : s_FenceRefs)
		{
			s_FenceHandles.push_back(f ? f->GetFence() : VK_NULL_HANDLE);
		}

		s_RenderSemaphoreHandles.clear();
		s_RenderSemaphoreHandles.reserve(s_RenderSemaphoreRefs.size());
		for (const auto &s : s_RenderSemaphoreRefs)
		{
			s_RenderSemaphoreHandles.push_back(s ? s->GetSemaphore() : VK_NULL_HANDLE);
		}
	}

#pragma region Lifecycle Methods

	void Renderer::Init()
	{
		//SEDX_TRACK_CALL("Renderer::Init");
		
		// Prevent double-initialization
		/*
		if (s_Data)
		{
			SEDX_CORE_INFO_TAG("Renderer", "Init called but renderer is already initialized, skipping");
			return;
		}
		*/
		SEDX_CORE_TRACE_TAG("Renderer", "=== Initializing Renderer ===");

		// Initialize volk loader
		volkInitialize();

		if (!RenderContext::IsInitialized())
		{
			SEDX_CORE_FATAL_TAG("Renderer", "RenderContext failed to initialize, cannot proceed with renderer setup");
			return;
		}

		m_Data = new RendererProperties;

		if (Debugging::IsRenderdocEnabled())
		{
			RenderDoc::OnPreDeviceCreation();
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// SwapChain                                                                                                     ///
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		if (!Window::IsVisible())
		{
			// On startup the window can be temporarily hidden/minimized while platform
			// initialization completes. Don't abort renderer initialization permanently.
			SEDX_CORE_WARN_TAG("Swapchain", "Window is not visible yet; attempting swapchain setup anyway");
		}

		s_Swapchain = CreateRef<Swapchain>();
		// Temporary debug logging: record when the global swapchain Ref is assigned
		// This helps trace who/when mutates the global swapchain handle during init.
		if (s_Swapchain)
		{
			SEDX_CORE_TRACE_TAG("Swapchain", "Assigned s_Swapchain: Ref ptr = {}, swapchain handle = 0x{:x}",
								static_cast<void*>(s_Swapchain.Get()), reinterpret_cast<uint64_t>(s_Swapchain->Get()));
		}
		if (Window::GetWindow())
		{
			// Verify surface was created
			if (s_Swapchain->GetSurface() == VK_NULL_HANDLE)
			{
				SEDX_CORE_ERROR_TAG("Renderer", "Failed to create Vulkan surface, surface is still VK_NULL_HANDLE");
				return;
			}

		}
		else
		{
			SEDX_CORE_ERROR_TAG("Renderer", "Failed to get SDL window for surface creation");
			return;
		}

		// Get window dimensions
		uint32_t width = Window::GetWidth();
		uint32_t height = Window::GetHeight();

		SetOutputResolution(width, height, false);
		SetRendererResolution(1920, 1080, false);
		SetViewport(static_cast<float>(width), static_cast<float>(height));

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Frame Resources                                                                                               ///
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		s_AssetManager = CreateScope<AssetManager>();
		SEDX_CORE_TRACE_TAG("Renderer", "Created AssetManager");

		CreateFrameResources();
		// Temporary debug trace: log sync vectors sizes after frame resources creation
		SEDX_CORE_TRACE_TAG("Renderer", "Trace: After CreateFrameResources sizes - fences={}, render={}",
			static_cast<uint32_t>(s_FenceHandles.size()), static_cast<uint32_t>(s_RenderSemaphoreHandles.size()));

		// Structured draw-data buffer used by Renderer::WriteDrawData (including UI path).
		// Allocate one large mapped buffer partitioned by frame resource index.
		// ALL slots in GetStructuredBuffers() that are read each frame must be allocated here
		// to avoid nullptr dereferences in UpdateMaterials, UpdateLights, UpdateBoundingBoxes, etc.
		{
			auto& structuredBuffers = GetStructuredBuffers();

			// --- DrawData (large mapped ring buffer, one region per frame-in-flight) ---
			const uint32_t drawDataBufferIndex  = static_cast<uint32_t>(Renderer_Buffer::DrawData);
			const uint32_t drawDataElementCount = RENDERER_MAX_DRAW_CALLS * RENDERER_RESOURCE_FRAME_LIFETIME;
			if (!structuredBuffers[drawDataBufferIndex])
			{
				structuredBuffers[drawDataBufferIndex] = CreateRef<Buffer>(
					sizeof(ShaderBuffer_DrawData), drawDataElementCount, nullptr, true, "draw_data_buffer");
				SEDX_CORE_ASSERT(structuredBuffers[drawDataBufferIndex], "Failed to create draw_data_buffer");
			}

			if (Buffer* drawDataBuffer = structuredBuffers[drawDataBufferIndex].Get())
			{
				drawDataBuffer->Map();
				SEDX_CORE_ASSERT(drawDataBuffer->GetMappedData() != nullptr, "Failed to map draw_data_buffer");
			}
			else
			{
				SEDX_CORE_ERROR_TAG("Renderer", "Failed to create draw_data_buffer");
			}

			// --- MaterialParameters ---
			const uint32_t materialIndex = static_cast<uint32_t>(Renderer_Buffer::MaterialParameters);
			if (!structuredBuffers[materialIndex])
			{
				structuredBuffers[materialIndex] = CreateRef<Buffer>(
					sizeof(ShaderBuffer_Material), MAX_ARRAY_SIZE, nullptr, true, "material_parameters");
				SEDX_CORE_ASSERT(structuredBuffers[materialIndex], "Failed to create material_parameters buffer");
				SEDX_CORE_TRACE_TAG("Renderer", "Created material_parameters buffer");
			}

			// --- LightParameters ---
			const uint32_t lightIndex = static_cast<uint32_t>(Renderer_Buffer::LightParameters);
			if (!structuredBuffers[lightIndex])
			{
				structuredBuffers[lightIndex] = CreateRef<Buffer>(
					sizeof(ShaderBuffer_Light), MAX_ARRAY_SIZE, nullptr, true, "light_parameters");
				SEDX_CORE_ASSERT(structuredBuffers[lightIndex], "Failed to create light_parameters buffer");
				SEDX_CORE_TRACE_TAG("Renderer", "Created light_parameters buffer");
			}

			// --- AABBs (per-frame regions, prepass + indirect draw slots) ---
			const uint32_t aabbIndex = static_cast<uint32_t>(Renderer_Buffer::AABBs);
			if (!structuredBuffers[aabbIndex])
			{
				// Double the size to hold per-frame regions (m_FrameResource_Index * MAX_ARRAY_SIZE offset)
				const uint32_t aabbElementCount = MAX_ARRAY_SIZE * DRAW_DATA_BUFFER_COUNT;
				structuredBuffers[aabbIndex] = CreateRef<Buffer>(
					sizeof(ShaderBuffer_Aabb), aabbElementCount, nullptr, true, "aabb_buffer");
				SEDX_CORE_ASSERT(structuredBuffers[aabbIndex], "Failed to create aabb_buffer");
				SEDX_CORE_TRACE_TAG("Renderer", "Created aabb_buffer");
			}

			// --- ConstantFrame (per-frame constants written by UpdateFrameConstantBuffer) ---
			const uint32_t constantFrameIndex = static_cast<uint32_t>(Renderer_Buffer::ConstantFrame);
			if (!structuredBuffers[constantFrameIndex])
			{
				// Use RENDERER_RESOURCE_FRAME_LIFETIME slots so offset rotation works correctly
				structuredBuffers[constantFrameIndex] = CreateRef<Buffer>(
					sizeof(ConstantBuffer_Frame), RENDERER_RESOURCE_FRAME_LIFETIME, nullptr, true, "constant_frame_buffer");
				SEDX_CORE_ASSERT(structuredBuffers[constantFrameIndex], "Failed to create constant_frame_buffer");
				SEDX_CORE_TRACE_TAG("Renderer", "Created constant_frame_buffer");
			}

			// --- IndirectDrawArgs ---
			const uint32_t indirectArgsIndex = static_cast<uint32_t>(Renderer_Buffer::IndirectDrawArgs);
			if (!structuredBuffers[indirectArgsIndex])
			{
				structuredBuffers[indirectArgsIndex] = CreateRef<Buffer>(
					sizeof(ShaderBuffer_IndirectDrawArgs), MAX_ARRAY_SIZE, nullptr, true, "indirect_draw_args");
				SEDX_CORE_ASSERT(structuredBuffers[indirectArgsIndex], "Failed to create indirect_draw_args buffer");
				SEDX_CORE_TRACE_TAG("Renderer", "Created indirect_draw_args buffer");
			}

			// --- IndirectDrawData ---
			const uint32_t indirectDataIndex = static_cast<uint32_t>(Renderer_Buffer::IndirectDrawData);
			if (!structuredBuffers[indirectDataIndex])
			{
				structuredBuffers[indirectDataIndex] = CreateRef<Buffer>(
					sizeof(ShaderBuffer_DrawData), MAX_ARRAY_SIZE, nullptr, true, "indirect_draw_data");
				SEDX_CORE_ASSERT(structuredBuffers[indirectDataIndex], "Failed to create indirect_draw_data buffer");
				SEDX_CORE_TRACE_TAG("Renderer", "Created indirect_draw_data buffer");
			}

			// --- IndirectDrawCount (single uint32_t, reset to zero each frame by cull shader) ---
			const uint32_t indirectCountIndex = static_cast<uint32_t>(Renderer_Buffer::IndirectDrawCount);
			if (!structuredBuffers[indirectCountIndex])
			{
				structuredBuffers[indirectCountIndex] = CreateRef<Buffer>(
					sizeof(uint32_t), 1, nullptr, true, "indirect_draw_count");
				SEDX_CORE_ASSERT(structuredBuffers[indirectCountIndex], "Failed to create indirect_draw_count buffer");
				SEDX_CORE_TRACE_TAG("Renderer", "Created indirect_draw_count buffer");
			}

			// --- DummyInstance (identity instances for vertex-pulled draws with no instancing) ---
			const uint32_t dummyInstanceIndex = static_cast<uint32_t>(Renderer_Buffer::DummyInstance);
			if (!structuredBuffers[dummyInstanceIndex])
			{
				// Minimal buffer: one identity instance entry; real content written by passes
				structuredBuffers[dummyInstanceIndex] = CreateRef<Buffer>(
					sizeof(ShaderBuffer_DrawData), 1, nullptr, true, "dummy_instance_buffer");
				SEDX_CORE_ASSERT(structuredBuffers[dummyInstanceIndex], "Failed to create dummy_instance_buffer");
				SEDX_CORE_TRACE_TAG("Renderer", "Created dummy_instance_buffer");
			}

			SEDX_CORE_TRACE_TAG("Renderer", "All structured buffers initialized");
		}

		CreateRenderTargets(true, true, true);
		CreateModels();
		GeometryBuffer::Initialize();
		CreateShaders();

		// Query surface capabilities
		VkPhysicalDevice physicalDevice = RenderContext::Get()->GetDevice()->GetPhysicalDevice();
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, s_Swapchain->GetSurface(), &s_SurfaceCaps);

		// Draw calls are handled directly in RecordRenderCommands via m_TestModel
		RenderContext::Get()->renderables = nullptr;
		RenderContext::Get()->s_ShaderDataBuffers = s_UniformBuffers ? &s_UniformBuffers->Buffers() : nullptr;
		RenderContext::Get()->s_CommandBuffers = &m_CommandBuffers;

		// Provide pointers to the internal handle vectors so other subsystems can read them (null-safe)
		RenderContext::Get()->s_Fences = s_FenceHandles.empty() ? nullptr : &s_FenceHandles;
		RenderContext::Get()->s_PresentSemaphores = nullptr;
		RenderContext::Get()->s_RenderSemaphores = s_RenderSemaphoreHandles.empty() ? nullptr : &s_RenderSemaphoreHandles;

		m_ResourcesInitialized = true;
		SEDX_CORE_INFO_TAG("Renderer", "=== Renderer Initialization Complete ===");
	}

	void Renderer::Shutdown()
	{
		SEDX_CORE_INFO_TAG("Renderer", "=== Shutting Down Renderer ===");

		Ref<Device> device = RenderContext::Get()->GetDevice();

		// Wait for all GPU work to complete
		device->GetQueueManager()->WaitIdleAll();

		// Destroy shared descriptor pool before other device resources are torn down.
		// This ensures vkDestroyDescriptorPool is called while the logical device is still valid.
		DescriptorPoolManager::Shutdown();

		// Destroy basic forward pipeline resources
		{
			VkDevice dev = device->GetLogicalDevice();
			if (m_BasicPipeline != VK_NULL_HANDLE)
			{
				vkDestroyPipeline(dev, m_BasicPipeline, nullptr);
				m_BasicPipeline = VK_NULL_HANDLE;
			}
			if (m_BasicPipelineLayout != VK_NULL_HANDLE)
			{
				vkDestroyPipelineLayout(dev, m_BasicPipelineLayout, nullptr);
				m_BasicPipelineLayout = VK_NULL_HANDLE;
			}
			m_BasicShaderManager.reset();

			if (m_GridPipeline != VK_NULL_HANDLE)
			{
				vkDestroyPipeline(dev, m_GridPipeline, nullptr);
				m_GridPipeline = VK_NULL_HANDLE;
			}
			if (m_GridPipelineLayout != VK_NULL_HANDLE)
			{
				vkDestroyPipelineLayout(dev, m_GridPipelineLayout, nullptr);
				m_GridPipelineLayout = VK_NULL_HANDLE;
			}
			m_GridShaderManager.reset();

			VmaAllocator vma = device->GetMemoryAllocator().GetAllocator();
			if (m_GridVertexAllocation != VK_NULL_HANDLE)
			{
				vmaDestroyBuffer(vma, m_GridVertexBuffer, m_GridVertexAllocation);
				m_GridVertexBuffer = VK_NULL_HANDLE;
				m_GridVertexAllocation = VK_NULL_HANDLE;
			}
			if (m_GridIndexAllocation != VK_NULL_HANDLE)
			{
				vmaDestroyBuffer(vma, m_GridIndexBuffer, m_GridIndexAllocation);
				m_GridIndexBuffer = VK_NULL_HANDLE;
				m_GridIndexAllocation = VK_NULL_HANDLE;
			}
			m_GridIndexCount = 0;

			for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
			{
				if (m_BasicShaderDataAllocations[i] != VK_NULL_HANDLE)
				{
					vmaDestroyBuffer(vma, m_BasicShaderDataBuffers[i], m_BasicShaderDataAllocations[i]);
					m_BasicShaderDataBuffers[i]     = VK_NULL_HANDLE;
					m_BasicShaderDataAllocations[i] = VK_NULL_HANDLE;
					m_BasicShaderDataMapped[i]      = nullptr;
					m_BasicShaderDataAddresses[i]   = 0;
				}

				if (m_CameraUboAllocations[i] != VK_NULL_HANDLE)
				{
					vmaDestroyBuffer(vma, m_CameraUboBuffers[i], m_CameraUboAllocations[i]);
					m_CameraUboBuffers[i]     = VK_NULL_HANDLE;
					m_CameraUboAllocations[i] = VK_NULL_HANDLE;
					m_CameraUboMapped[i]      = nullptr;
				}
			}

			if (m_CameraDescriptorPool != VK_NULL_HANDLE)
			{
				vkDestroyDescriptorPool(dev, m_CameraDescriptorPool, nullptr);
				m_CameraDescriptorPool = VK_NULL_HANDLE;
				m_CameraDescriptorSets.fill(VK_NULL_HANDLE);
			}
			if (m_CameraDescriptorSetLayout != VK_NULL_HANDLE)
			{
				vkDestroyDescriptorSetLayout(dev, m_CameraDescriptorSetLayout, nullptr);
				m_CameraDescriptorSetLayout = VK_NULL_HANDLE;
			}
		}

		// Destroy test model GPU resources before frame resources are torn down
		if (m_TestModel)
		{
			VmaAllocator vma = device->GetMemoryAllocator().GetAllocator();
			m_TestModel->Destroy(vma);
			m_TestModel.reset();
			SEDX_CORE_TRACE_TAG("Renderer", "Test model destroyed");
		}

		// Destroy structured buffers and clear references so re-init starts from a clean state.
		// This prevents stale Ref<Buffer> entries across Init()/Shutdown() cycles.
		{
			auto& structuredBuffers = GetStructuredBuffers();
			for (Ref<Buffer>& bufferRef : structuredBuffers)
			{
				if (bufferRef)
				{
					bufferRef->Destroy();
					bufferRef.Reset();
				}
			}
			SEDX_CORE_TRACE_TAG("Renderer", "Structured buffers destroyed");
		}

		// Destroy frame resources
		GeometryBuffer::Shutdown();
		DestroyFrameResources();
		if (s_AssetManager)
		{
			s_AssetManager->DestroyAll();
			s_AssetManager.reset();
		}

		// Destroy swapchain
		if (s_Swapchain)
		{
			s_Swapchain.Reset();
		}

		if (Debugging::IsRenderdocEnabled())
		{
			RenderDoc::Shutdown();
		}

		// Cleanup renderer data
		delete m_Data;
		m_Data = nullptr;

		m_ResourcesInitialized = false;
		SEDX_CORE_TRACE_TAG("Renderer", "=== Renderer Shutdown Complete ===");

		// Signal the deletion queue that no further resources will be legitimately
		// enqueued.  Any AddDeletionQueue calls arriving after this point come from
		// Ref<> destructors running in the CRT static-dtor phase and must be dropped
		// to avoid a use-after-free on the already-destroyed s_DeletionQueue map.
		QueueManager::NotifyShutdown();
	}

	void Renderer::Tick()
	{
		// Let Scene drive ECS camera entities before renderer reads camera data.
		Scene::Tick();

		// Prefer the scene-owned camera when available, otherwise keep any camera that
		// may have been provided directly via Renderer::SetCamera().
		if (Camera* sceneCamera = Scene::GetCamera())
		{
			m_Camera = sceneCamera;
		}

		// Tick the active camera so its matrices are always up-to-date before draw calls.
		// If Scene has an ECS camera entity, Scene::Tick already updates the camera controller.
		if (m_Camera)
		{
			if (!Scene::HasCameraEntity())
			{
				m_Camera->Tick();
			}
			UpdateCameraUBO(m_CurrentFrameIndex); // Ensure UBO is ready before command recording
			SEDX_CORE_TRACE_TAG("Renderer", "Camera pos = (X: {:.3f}, Y: {:.3f}, Z: {:.3f})",
				m_Camera->GetEyePosition().x, m_Camera->GetEyePosition().y, m_Camera->GetEyePosition().z);
		}
		else
		{
			static bool s_WarnedNoCamera = false;
			if (!s_WarnedNoCamera)
			{
				SEDX_CORE_WARN_TAG("Renderer", "No active Scene camera is set; renderer will use default UBO data until Scene::Init provides one");
				s_WarnedNoCamera = true;
			}
		}

		Ref<Device> device = RenderContext::Get()->GetDevice();
		SEDX_CORE_ASSERT(device.IsValid(), "Device is not valid in Renderer::Tick");

		Ref<QueueManager> queueManager = device->GetQueueManager();
		SEDX_CORE_ASSERT(queueManager.IsValid(), "QueueManager is not valid in Renderer::Tick");

		device->GetMemoryAllocator().Tick(m_FrameNumber);
		bool canRender = !Window::IsMinimized() && m_ResourcesInitialized;

		// prevent write-after-present hazards when idle (skip first frame, nothing to wait for)
		if (!canRender && m_FrameNumber > 0)
		{
			if (Ref<Queue> *queue = queueManager->GetQueue(QueueType::Graphics); queue && *queue)
			{
				Queue::WaitIdle(*queue->Get());
			}
		}

		m_CmdList_Present = queueManager->NextCommandList();
		SEDX_CORE_ASSERT(m_CmdList_Present != nullptr, "Failed to acquire present command list");
		m_CmdList_Present->Begin();

		// m_CmdList_Frame is no longer used — UI rendering happens inside RecordRenderCommands
		// via SetExternalRecordingBuffer, so the swapchain dynamic render pass is already active.
		m_CmdList_Frame = nullptr;

#pragma region Compute Command List Setup

		/*
		// Bootstrap mode currently runs the graphics-only path in ProduceFrame().
		// Don't acquire/record compute command lists until the deferred compute passes
		// (and their synchronization) are fully wired.
		const bool needsComputeCommandList = true;
		*/
		m_CmdList_Compute = nullptr;
		if (canRender /*&& needsComputeCommandList*/)
		{	
			if (Ref<Queue> *queue = queueManager->GetQueue(QueueType::Compute); queue && *queue)
			{
				Queue::WaitIdle(*queue->Get());
			}

			m_CmdList_Compute = queueManager->NextCommandList();
			SEDX_CORE_ASSERT(m_CmdList_Compute != nullptr, "Failed to acquire compute command list");

			m_CmdList_Compute->Begin();
		}

#pragma endregion

		m_DrawData_Count = 0; // Reset draw call count each frame; it will be incremented by Renderer::WriteDrawData as draw calls are recorded.

		if (canRender)
		{
			bool isLoading = false;

			// rebuild geometry buffer if new meshes arrived
			if (!isLoading)
			{
				GeometryBuffer::BuildIfDirty();
			}

			// rotate per-frame buffers to avoid cpu-gpu race conditions without stalling; 
			// this allows the CPU to write to one buffer while the GPU reads from another, with a safe number of buffers in flight as a cushion
			RotateFrameBuffers();

			UpdateDrawCalls(m_CmdList_Present);

			// resource cleanup - frame-based retirement, no gpu stall required
			if (QueueManager::NeedToParseDeletionQueue())
			{
				QueueManager::ParseDeletionQueue();
			}

			// reset constant buffer offset periodically
			{
				m_ResourceIndex++;
				if (m_ResourceIndex == RENDERER_RESOURCE_FRAME_LIFETIME)
				{
					m_ResourceIndex = 0;
					Buffer* constantFrameBuffer = GetBuffer(Renderer_Buffer::ConstantFrame);
					if (constantFrameBuffer)
					{
						constantFrameBuffer->ResetOffset();
					}
					else
					{
						SEDX_CORE_ERROR_TAG("Renderer", "ConstantFrame buffer is null — cannot reset offset");
					}
				}
			}

#pragma region Bindless Resource Updates

			if (!isLoading)
			{
				bool initialize = GetFrameNumber() == 0;

#pragma region Lights

				if (initialize || Scene::UpdateSceneChanges(ComponentType::Light).IsDirty())
				{
					UpdateShadowAtlas();
					UpdateLights(m_CmdList_Present);
					BindlessManager::UpdateBuffer(BindlessResource::LightParameters, GetBuffer(Renderer_Buffer::LightParameters));
				}

#pragma endregion
#pragma region Materials

				if (initialize || Scene::UpdateSceneChanges(ComponentType::Material).IsDirty())
				{
					UpdateMaterials(m_CmdList_Present);
					BindlessManager::UpdateBuffer(BindlessResource::MaterialTextures, GetBuffer(Renderer_Buffer::MaterialParameters));
				}

#pragma endregion
#pragma region Samplers

				if (m_BindlessSamplers_Dirty.IsDirty())
				{
					// Update bindless samplers: pass pointer to the sampler array and its count
					auto &samplers = Renderer::GetSamplers();
					BindlessManager::UpdateSamplers(BindlessResource::SamplersRegular, samplers.data(), static_cast<uint32_t>(samplers.size()));
					m_BindlessSamplers_Dirty.Check();
				}

#pragma endregion
#pragma region AABBs (always, they change with entity transforms)

				{
					UpdateBoundingBoxes(m_CmdList_Present);

					static bool aabbs_descriptor_set = false;
					if (!aabbs_descriptor_set)
					{
						BindlessManager::UpdateBuffer(BindlessResource::Aabbs, GetBuffer(Renderer_Buffer::AABBs));
						aabbs_descriptor_set = true;
					}
				}

#pragma endregion
#pragma region Draw Data

				{
					if (m_DrawData_Count > 0)
					{
						Buffer *buffer = GetBuffer(Renderer_Buffer::DrawData);
						uint32_t frame_byte_offset = m_FrameResource_Index * MAX_DRAW_CALLS *
													 static_cast<uint32_t>(sizeof(ShaderBuffer_DrawData));
						uint32_t upload_size = static_cast<uint32_t>(sizeof(ShaderBuffer_DrawData)) * m_DrawData_Count;
						CommandList::UpdateBuffer(buffer, frame_byte_offset, upload_size, &m_DrawData_CPU[0]);
					}

					// the descriptor points to a single large buffer that holds all frames' draw data
					// at different offsets, so it only needs to be set once; this eliminates the race
					// where vkUpdateDescriptorSets (host-side, instantly visible under UPDATE_AFTER_BIND)
					// would change the buffer pointer while the previous frame's phase 3 transparent pass
					// was still reading from it on the gpu
					static bool draw_data_descriptor_set = false;
					if (!draw_data_descriptor_set)
					{
						BindlessManager::UpdateBuffer(BindlessResource::DrawData, GetBuffer(Renderer_Buffer::DrawData));
						draw_data_descriptor_set = true;
					}
				}

#pragma endregion
#pragma region Geometry Buffers (vertex pulling via bindless structured buffers)

				{
					static Buffer *lastVertexBuffer = nullptr;
					Buffer *currentVertex = GeometryBuffer::GetVertexBuffer();
					if (currentVertex && currentVertex != lastVertexBuffer)
					{
						BindlessManager::UpdateBuffer(BindlessResource::GeometryVertices, currentVertex);
						lastVertexBuffer = currentVertex;
					}

					static Buffer *lastIndexBuffer = nullptr;
					Buffer *currentIndex = GeometryBuffer::GetIndexBuffer();
					if (currentIndex && currentIndex != lastIndexBuffer)
					{
						BindlessManager::UpdateBuffer(BindlessResource::GeometryIndices, currentIndex);
						lastIndexBuffer = currentIndex;
					}
				}

#pragma endregion
#pragma region Dummy Instance Buffer (vertex pulling identity instances)

				{
					static bool instances_descriptor_set = false;
					if (!instances_descriptor_set)
					{
						BindlessManager::UpdateBuffer(BindlessResource::Instances, GetBuffer(Renderer_Buffer::DummyInstance));
						instances_descriptor_set = true;
					}
				}

#pragma endregion
#pragma region Indirect Draw Buffers

				if (m_Indirect_DrawCount > 0)
				{
					Buffer* args_buffer = GetBuffer(Renderer_Buffer::IndirectDrawArgs);
					if (!args_buffer)
					{
						SEDX_CORE_ERROR_TAG("Renderer", "IndirectDrawArgs buffer is null — skipping indirect buffer upload");
					}
					else
					{
						args_buffer->ResetOffset();
						args_buffer->Update(m_CmdList_Present, &m_Indirect_DrawArgs[0], args_buffer->GetStride() * m_Indirect_DrawCount);
					}

					Buffer* data_buffer = GetBuffer(Renderer_Buffer::IndirectDrawData);
					if (!data_buffer)
					{
						SEDX_CORE_ERROR_TAG("Renderer", "IndirectDrawData buffer is null — skipping indirect buffer upload");
					}
					else
					{
						data_buffer->ResetOffset();
						data_buffer->Update(m_CmdList_Present, &m_Indirect_DrawData[0], data_buffer->GetStride() * m_Indirect_DrawCount);
					}

					// reset count, the cull shader atomically increments it
					uint32_t zero = 0;
					Buffer* count_buffer = GetBuffer(Renderer_Buffer::IndirectDrawCount);
					if (!count_buffer)
					{
						SEDX_CORE_ERROR_TAG("Renderer", "IndirectDrawCount buffer is null — skipping count reset");
					}
					else
					{
						count_buffer->ResetOffset();
						count_buffer->Update(m_CmdList_Present, &zero, sizeof(uint32_t));
					}
				}
#pragma endregion

			}

#pragma endregion
				
			UpdateFrameConstantBuffer(m_CmdList_Present);
			UpdatePersistentLines();
			AddLinesToBeRendered();

			/*
			if (canRender)
			{
				BlitToBackBuffer(m_CmdList_Present, GetRenderTarget(Renderer_RenderTarget::frame_output));
			}

			SubmitAndPresent();
			*/

			// NOTE: BlitToBackBuffer via m_CmdList_Present is intentionally removed here.
			// m_CmdList_Present is used for resource updates (materials, lights, etc.) only;
			// it is never submitted with a swapchain acquire semaphore, so any render commands
			// recorded into it would be silently discarded. The blit-to-swapchain is handled
			// inside RecordRenderCommands() which runs within the properly-submitted
			// m_CommandBuffers[m_CurrentFrameIndex] command buffer.

			{
				m_Lines_Vertices.clear();
				m_Icons.clear();
			}
	
			// only count frames that actually rendered
			if (canRender)
			{
				s_FrameNumber++;
				if (s_FrameNumber == 1)
					FirstFrameRenderedEvent event(true);
			}

			/*
				bool initialize = GetFrameNumber() == 0;

				// lights
				if (initialize || Scene::HaveLightsChangedThisFrame())
				{
					UpdateShadowAtlas();
					UpdateLights(m_CmdList_Present);
					RHI_Device::UpdateBindlessLights(GetBuffer(Renderer_Buffer::LightParameters));
				}

				// materials
				if (initialize || Scene::HaveMaterialsChangedThisFrame())
				{
					UpdateMaterials(m_CmdList_Present);
					RHI_Device::UpdateBindlessMaterials(&m_Bindless_Textures, GetBuffer(Renderer_Buffer::MaterialParameters));
				}

				// samplers
				if (m_BindlessSamplers_Dirty)
				{
					RHI_Device::UpdateBindlessSamplers(&Renderer::GetSamplers());
					m_BindlessSamplers_Dirty = false;
				}

				// aabbs (always, they change with entity transforms)
				{}
				*/

			/*
			// Wire the modern pass-based renderer into the active frame loop.
			// This keeps the existing legacy path intact while enabling
			// `Pass_Depth_Prepass()` and `Pass_Grid()` execution.
			if (m_CmdList_Present && m_CmdList_Compute)
			{
				ImageResource* rtRender = GetRenderTarget(Renderer_RenderTarget::frame_render);
				ImageResource* rtOutput = GetRenderTarget(Renderer_RenderTarget::frame_output);

				const bool hasRtRender = rtRender && rtRender->Get() && *rtRender->Get() != VK_NULL_HANDLE;
				const bool hasRtOutput = rtOutput && rtOutput->Get() && *rtOutput->Get() != VK_NULL_HANDLE;

				if (hasRtRender && hasRtOutput)
				{
					ProduceFrame(m_CmdList_Present, m_CmdList_Compute);
				}
				else
				{
					static bool s_LoggedMissingPassTargets = false;
					if (!s_LoggedMissingPassTargets)
					{
						SEDX_CORE_WARN_TAG("Renderer", "Pass-based renderer disabled: frame render targets are not GPU-initialized yet");
						s_LoggedMissingPassTargets = true;
					}
				}
			}
			*/

			// UI rendering is done inside RecordRenderCommands (inside the dynamic render pass),
			// so no separate UI command list is needed here.

		}
	}   

#pragma region Frame Rendering Methods

	bool Renderer::BeginFrame()
	{
		SEDX_CORE_TRACE_TAG("Renderer", "Beginning frame {}", m_FrameNumber);

		// Reset per-frame draw data counter. Must be reset unconditionally (before
		// any early-return) so WriteDrawData never overflows m_DrawData_CPU[].
		m_DrawData_Count = 0;

		// Check if we can render
		if (!m_ResourcesInitialized)
		{
			SEDX_CORE_WARN_TAG("Renderer", "BeginFrame called but renderer not initialized");
			return false;
		}

		// Skip if window is minimized
		constexpr uint32_t minRenderDimension = 64;
		bool isValidResolution = s_RendererResolution.x >= minRenderDimension && s_RendererResolution.y >= minRenderDimension;

		if (Window::IsMinimized() || !isValidResolution)
		{
			SEDX_CORE_TRACE_TAG("Renderer", "Window is minimized or resolution is invalid ({}x{})", s_RendererResolution.x, s_RendererResolution.y);
			return false;
		}

		// Check swapchain validity with detailed diagnostics
		if (!s_Swapchain)
		{
			SEDX_CORE_ERROR_TAG("Renderer", "Swapchain is null, was Init() called successfully?");
			return false;
		}

		if (s_Swapchain->GetImages().empty())
		{
			SEDX_CORE_ERROR_TAG("Renderer", "Swapchain has no images, VkSwapchainKHR handle: {}, surface valid: {}",
								static_cast<void *>(s_Swapchain->Get()), s_Swapchain->GetSurface() != VK_NULL_HANDLE);
			
			// Attempt to recreate swapchain if surface is available and window is visible
			if (s_Swapchain->GetSurface() != VK_NULL_HANDLE)
			{
				SEDX_CORE_WARN_TAG("Renderer", "Attempting to recreate swapchain...");
				// Ensure GPU is idle before tearing down per-frame sync objects
				if (RenderContext::IsInitialized())
				{
					Ref<Device> device = RenderContext::Get()->GetDevice();
					if (device.IsValid() && device->GetQueueManager())
					{
						device->GetQueueManager()->WaitIdleAll();
					}
				}

				// Destroy frame-sync resources before swapchain recreation and recreate afterwards
				DestroyFrameResources();
				s_Swapchain->Recreate();
				CreateFrameResources();

				if (s_Swapchain != nullptr && !s_Swapchain->GetImages().empty())
				{
					SEDX_CORE_TRACE_TAG("Renderer", "Swapchain recreated successfully with {} images", s_Swapchain->GetImages().size());
				}
				else
				{
					SEDX_CORE_TRACE_TAG("Renderer", "Swapchain recreation failed or returned no images after recreation attempt");
					return false;
				}
			}
			else
			{
				SEDX_CORE_TRACE_TAG("Renderer", "Swapchain recreation failed or returned no images");
				return false;
			}
		}

		// Wait for the fence of the current frame-in-flight BEFORE acquiring the image
		if (!s_FenceHandles.empty())
		{
			SEDX_CORE_TRACE_TAG("Renderer", "Waiting for fence of frame {} (fence handle: {})", m_CurrentFrameIndex,
				static_cast<void *>(s_FenceHandles[m_CurrentFrameIndex]));
			if (m_CurrentFrameIndex < s_FenceHandles.size() && s_FenceHandles[m_CurrentFrameIndex] != VK_NULL_HANDLE)
			{
				VkDevice device = RenderContext::Get()->GetDevice()->GetLogicalDevice();
				vkWaitForFences(device, 1, &s_FenceHandles[m_CurrentFrameIndex], VK_TRUE, UINT64_MAX);
				vkResetFences(device, 1, &s_FenceHandles[m_CurrentFrameIndex]);
				SEDX_CORE_TRACE_TAG("Renderer", "Fence wait and reset complete for frame {}", m_CurrentFrameIndex);
			}
		}

		// Acquire image through the swapchain flow.
		s_Swapchain->AcquireNextImage();
		if (!s_Swapchain->IsImageAcquired())
			return false;

		m_SwapchainImageIndex = s_Swapchain->GetImageIndex();
		if (m_SwapchainImageIndex >= s_Swapchain->GetImages().size())
		{
			SEDX_CORE_WARN_TAG("Renderer", "Swapchain AcquireNextImage returned invalid index: {}", m_SwapchainImageIndex);
			return false;
		}

		SEDX_CORE_TRACE_TAG("Renderer", "Acquired swapchain image index: {}", m_SwapchainImageIndex);

		// Begin command buffer recording
		if (VkCommandBuffer cb = m_CommandBuffers[m_CurrentFrameIndex]; cb != VK_NULL_HANDLE)
		{
			vkResetCommandBuffer(cb, 0);
			SEDX_CORE_TRACE_TAG("Renderer", "Command buffer reset for frame {}", m_CurrentFrameIndex);

			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			VkResult result = vkBeginCommandBuffer(cb, &beginInfo);
			SEDX_VK_RESULT_ASSERT(result, "vkBeginCommandBuffer failed");
		}

		SEDX_CORE_TRACE_TAG("Renderer", "Command buffer recording begun for frame {}", m_CurrentFrameIndex);
		m_FrameInProgress = true;
		return true;
	}

	void Renderer::EndFrame()
	{
		SEDX_CORE_TRACE_TAG("Renderer", "Ending frame {}", m_FrameNumber);

		if (!m_FrameInProgress)
		{
			SEDX_CORE_WARN_TAG("Renderer", "EndFrame called but no frame in progress");
			return;
		}

		VkCommandBuffer cb = m_CommandBuffers[m_CurrentFrameIndex];
		if (cb != VK_NULL_HANDLE)
		{
			// Record the actual render commands for this frame
			RecordRenderCommands(cb, m_SwapchainImageIndex);

			// End command buffer recording
			VkResult result = vkEndCommandBuffer(cb);
			SEDX_VK_RESULT_ASSERT(result, "vkEndCommandBuffer failed");
		}

		m_FrameInProgress = false;
	}

	void Renderer::SubmitAndPresent()
	{
		SEDX_CORE_TRACE_TAG("Renderer", "Submitting command buffer and presenting frame {}", m_FrameNumber);

		if (m_FrameInProgress)
		{
			SEDX_CORE_ERROR_TAG("Renderer", "SubmitAndPresent called while a frame is still in progress (EndFrame must be called before submit)");
			return;
		}

		VkCommandBuffer cb = m_CommandBuffers[m_CurrentFrameIndex];
		if (cb == VK_NULL_HANDLE)
		{
			SEDX_CORE_TRACE_TAG("Renderer", "No command buffer available for frame {}", m_FrameNumber);
			return;
		}

		if (s_RenderSemaphoreHandles.empty() || s_FenceHandles.empty())
		{
			SEDX_CORE_ERROR_TAG("Renderer", "Cannot submit, synchronization primitives not created or empty.");
			return;
		}

		// Get graphics queue from context
		VkQueue graphicsQueue = VK_NULL_HANDLE;

		Ref<Device> device = RenderContext::Get()->GetDevice();
		SEDX_CORE_VERIFY(device.IsValid(), "Device is not valid during submit");

		if (device.IsValid())
		{
			Ref<QueueManager> queueManager = device->GetQueueManager();
			SEDX_CORE_VERIFY(queueManager.IsValid(), "QueueManager is not valid during submit");

			if (queueManager.IsValid())
			{
				if (Ref<Queue> *queueRef = queueManager->GetQueue(Graphics); queueRef && *queueRef)
				{
					graphicsQueue = (*queueRef)->GetQueue();
				}
			}
		}

		SEDX_CORE_VERIFY(graphicsQueue != VK_NULL_HANDLE, "No graphics queue available for submission");
		if (graphicsQueue == VK_NULL_HANDLE)
		{
			return;
		}

		// Submit command buffer
		VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

		// Extra lifetime / uninitialized pattern checks
		auto IsLikelyUninitialized = [](uint64_t val) 
		{
			// Common MSVC uninitialized patterns: 0xCCCCCCCCCCCCCCCC, 0xCDCDCDCDCDCDCDCD
			return val == 0xCCCCCCCCCCCCCCCCULL || val == 0xCDCDCDCDCDCDCDCDULL;
		};

		// Validate device and logical device
		VkDevice vkDevice = VK_NULL_HANDLE;
		if (device.IsValid())
		{
			vkDevice = device->GetDevice();
		}
		SEDX_CORE_VERIFY(vkDevice != VK_NULL_HANDLE, "Logical VkDevice is invalid before submit");

		// Ensure indices and swapchain are valid before dereferencing handle arrays
		if (m_SwapchainImageIndex >= s_RenderSemaphoreHandles.size())
		{
			SEDX_CORE_ERROR_TAG("Renderer", "Swapchain image index {} out of bounds for render semaphores (size {})",
								m_SwapchainImageIndex, s_RenderSemaphoreHandles.size());
			return;
		}

		SEDX_CORE_VERIFY(s_Swapchain && s_Swapchain->Get() != VK_NULL_HANDLE, "Swapchain invalid before present");
		VkSemaphore acquireSemaphore = s_Swapchain->GetAcquiredVkSemaphore();
		SEDX_CORE_VERIFY(acquireSemaphore != VK_NULL_HANDLE, "Acquire semaphore invalid before submit");

		// Detect obviously-uninitialized handles (do this after bounds checks)
	   uint64_t presentSem = reinterpret_cast<uint64_t>(acquireSemaphore);
		uint64_t renderSem = reinterpret_cast<uint64_t>(s_RenderSemaphoreHandles[m_SwapchainImageIndex]);
		uint64_t fenceHandle = reinterpret_cast<uint64_t>(s_FenceHandles[m_CurrentFrameIndex]);
		uint64_t cbHandle = reinterpret_cast<uint64_t>(cb);

		SEDX_CORE_VERIFY(!IsLikelyUninitialized(presentSem), "Present semaphore appears uninitialized: 0x{:x}", presentSem);
		SEDX_CORE_VERIFY(!IsLikelyUninitialized(renderSem), "Render semaphore appears uninitialized: 0x{:x}", renderSem);
		SEDX_CORE_VERIFY(!IsLikelyUninitialized(fenceHandle), "Fence handle appears uninitialized: 0x{:x}", fenceHandle);
		SEDX_CORE_VERIFY(!IsLikelyUninitialized(cbHandle), "Command buffer appears uninitialized: 0x{:x}", cbHandle);

		// UI is now rendered inside RecordRenderCommands (in the same CB as scene geometry),
		// so only the single swapchain CB needs to be submitted.
		VkCommandBuffer frameCBs[1] = { cb };
		uint32_t frameCBCount = 1;

		SEDX_CORE_TRACE_TAG("Renderer", "Submitting command buffer for frame {}, waiting on present semaphore {}, signaling render semaphore {}",
			m_FrameNumber,
			static_cast<void *>(acquireSemaphore),
			static_cast<void *>(s_RenderSemaphoreHandles[m_SwapchainImageIndex]));

		// Defensive validation of synchronization primitives and handles before submit
	  SEDX_CORE_VERIFY(acquireSemaphore != VK_NULL_HANDLE,
						 "Acquire semaphore handle invalid for image {}", m_SwapchainImageIndex);
		SEDX_CORE_VERIFY(s_RenderSemaphoreHandles[m_SwapchainImageIndex] != VK_NULL_HANDLE,
						 "Render semaphore handle invalid for swapchain image {}", m_SwapchainImageIndex);
		SEDX_CORE_VERIFY(s_FenceHandles[m_CurrentFrameIndex] != VK_NULL_HANDLE,
						 "Fence handle invalid for frame {}", m_CurrentFrameIndex);
		SEDX_CORE_VERIFY(cb != VK_NULL_HANDLE, "Command buffer invalid for frame {}", m_FrameNumber);
		SEDX_CORE_VERIFY(s_Swapchain && s_Swapchain->Get() != VK_NULL_HANDLE, "Swapchain invalid before present");

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
	   submitInfo.pWaitSemaphores = &acquireSemaphore;
		submitInfo.pWaitDstStageMask = &waitStages;
		submitInfo.commandBufferCount = frameCBCount;
		submitInfo.pCommandBuffers = frameCBs;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &s_RenderSemaphoreHandles[m_SwapchainImageIndex];

		VkResult submitResult = vkQueueSubmit(graphicsQueue, 1, &submitInfo, s_FenceHandles[m_CurrentFrameIndex]);
		SEDX_VK_RESULT_ASSERT(submitResult, "vkQueueSubmit failed");

		// Present the rendered image
		VkSwapchainKHR swapchainHandle = s_Swapchain->Get();
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &s_RenderSemaphoreHandles[m_SwapchainImageIndex];
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapchainHandle;
		presentInfo.pImageIndices = &m_SwapchainImageIndex;
		SEDX_CORE_TRACE_TAG("Renderer", "Presenting swapchain image index {} for frame {}", m_SwapchainImageIndex, m_FrameNumber);

		// Defensive validation before present
		SEDX_CORE_VERIFY(presentInfo.pSwapchains != nullptr && presentInfo.pImageIndices != nullptr, "VkPresentInfoKHR not correctly configured");
		SEDX_CORE_VERIFY(s_RenderSemaphoreHandles[m_SwapchainImageIndex] != VK_NULL_HANDLE, "Render semaphore invalid before present");
		SEDX_CORE_VERIFY(s_Swapchain && s_Swapchain->Get() != VK_NULL_HANDLE, "Swapchain handle invalid before present");

		VkResult presentResult = vkQueuePresentKHR(graphicsQueue, &presentInfo);
		if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
		{
			// Swapchain needs full teardown and rebuild on resize / out-of-date.
			// We must: (1) drain all in-flight GPU work so no command buffers or
			// semaphores are in use, (2) destroy frame resources that reference the
			// old swapchain images, (3) recreate the swapchain, (4) rebuild frame
			// resources sized to the new swapchain image count.
			// Skipping any of these steps causes VK_ERROR_DEVICE_LOST.
			SEDX_CORE_INFO_TAG("Renderer", "Swapchain out-of-date — recreating (present result: {})", static_cast<int>(presentResult));

			if (device.IsValid() && device->GetQueueManager())
			{
				device->GetQueueManager()->WaitIdleAll();
			}

			DestroyFrameResources();
			s_Swapchain->Recreate();
			CreateFrameResources();

			// Reset the current frame index so the new fences/semaphores are used
			// from index 0 on the next frame.
			m_CurrentFrameIndex = 0;

			SEDX_CORE_INFO_TAG("Renderer", "Swapchain recreation complete");
		}
		else if (presentResult != VK_SUCCESS)
		{
			SEDX_CORE_ERROR_TAG("Renderer", "vkQueuePresentKHR failed: {}", static_cast<int>(presentResult));
		}

		// Advance to next frame-in-flight
		m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
		m_FrameNumber++;

		SEDX_CORE_TRACE_TAG("Renderer", "Frame {} submitted and presented, advancing to frame index {}",
							m_FrameNumber, m_CurrentFrameIndex);
	}

	void Renderer::DrawFrame(CommandList *cmdList, CommandList *computeCmdList)
	{
		/* 
		 * TODO: This method is currently not used, but will be the main entry point for recording 
		 * draw calls once the renderer is fully modularized and other systems are integrated to call it.
		 */
		SEDX_CORE_TRACE_TAG("Renderer", "DrawFrame called for frame {}", m_FrameNumber);
		// This method will be called by modules to record their draw commands
		// For now, placeholder implementation

		VkCommandBuffer cb = m_CommandBuffers[m_CurrentFrameIndex];
		if (cb == VK_NULL_HANDLE || !s_Swapchain)
			return;

		SEDX_CORE_TRACE_TAG("Renderer", "Recording draw commands using command buffer for frame {}", m_FrameNumber);

		// Record render commands
		RecordRenderCommands(cb, m_SwapchainImageIndex);
		SEDX_CORE_TRACE_TAG("Renderer", "Draw commands recorded for frame {}", m_FrameNumber);
	}

	void Renderer::BlitToBackBuffer(CommandList *cmdList, ImageResource *img)
	{
		// compute blit: vulkan can't blit depth to float, amd uav requires float
		cmdList->Blit(img, s_Swapchain.Get());
	}
	
#pragma endregion

#pragma region Frame Resource Creation and Destruction

	// This was the prior function for creating frame resources before Blit implementation.
	void Renderer::CreateFrameResources()
	{
		SEDX_CORE_TRACE_TAG("Renderer", "Creating frame resources for {} frames in flight", MAX_FRAMES_IN_FLIGHT);

		// Get queue family index from queue manager
		uint32_t queueFamily = RenderContext::Get()->GetDevice()->GetQueueManager()->GetFamilyIndexByType(Graphics);

		// Create command pool
		m_CommandPool = CreateScope<CommandPool>(queueFamily, CommandPoolType::Resettable);
		SEDX_CORE_TRACE_TAG("Renderer", "Created command pool");
	
		// Allocate command buffers
		auto allocatedBuffers = m_CommandPool->Allocate(MAX_FRAMES_IN_FLIGHT);
		for (size_t i = 0; i < allocatedBuffers.size() && i < m_CommandBuffers.size(); ++i)
		{
			m_CommandBuffers[i] = allocatedBuffers[i];
		}
		SEDX_CORE_TRACE_TAG("Renderer", "Allocated {} command buffers", MAX_FRAMES_IN_FLIGHT);

		// TODO: Make a FrameSync class that encapsulates the fences and semaphores for each frame easier to work with and reduce this duplicate static globals usage. 
		// Create per-frame fences and semaphores and keep wrapper refs alive.
		s_FenceRefs.clear();
		s_FenceHandles.clear();
		s_RenderSemaphoreRefs.clear();
		s_RenderSemaphoreHandles.clear();

		// Create fences (one per frame in flight)
		{
			std::scoped_lock lock(s_FrameSyncMutex);
			std::ostringstream ssteam_fence;
			ssteam_fence << std::this_thread::get_id();
			SEDX_CORE_TRACE_TAG("Renderer", "Creating fences ({}) thread id: {}", MAX_FRAMES_IN_FLIGHT, ssteam_fence.str());
			s_FenceRefs.reserve(MAX_FRAMES_IN_FLIGHT);
			s_FenceHandles.reserve(MAX_FRAMES_IN_FLIGHT);
			for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
			{
				Ref<Fence> fence = CreateRef<Fence>();
				fence->CreateSyncObject();
				s_FenceRefs.push_back(fence);
				s_FenceHandles.push_back(fence->GetFence());
				Debugging::SetResourceName(fence.Get()->GetFence(), ResourceType::Fence, "FrameFence");
			}
		}

		const uint32_t swapchainImageCount = static_cast<uint32_t>(s_Swapchain->GetImages().size());

		// Create render semaphores (one per swapchain image)
		{
			std::scoped_lock lock(s_FrameSyncMutex);
			std::ostringstream ssteam_render;
			ssteam_render << std::this_thread::get_id();
			SEDX_CORE_TRACE_TAG("Renderer", "Creating render semaphores ({}) thread id: {}", swapchainImageCount, ssteam_render.str());
			s_RenderSemaphoreRefs.reserve(swapchainImageCount);
			s_RenderSemaphoreHandles.reserve(swapchainImageCount);
			for (uint32_t i = 0; i < swapchainImageCount; ++i)
			{
				Ref<Semaphore> sem = CreateRef<Semaphore>();
				sem->CreateSyncObject();
				s_RenderSemaphoreRefs.push_back(sem);
				s_RenderSemaphoreHandles.push_back(sem->GetSemaphore());
				Debugging::SetResourceName(sem.Get()->GetSemaphore(), ResourceType::Semaphore, "RenderSemaphore");
			}
		   // Temporary trace: log first render semaphore handle if available
			if (!s_RenderSemaphoreHandles.empty())
			{
				SEDX_CORE_TRACE_TAG("Renderer", "Trace: Render semaphores created: count = {}, first = 0x{:x}",
					static_cast<uint32_t>(s_RenderSemaphoreHandles.size()), reinterpret_cast<uint64_t>(s_RenderSemaphoreHandles[0]));
			}
		}
	
	
		m_FrameSync = CreateScope<FrameSync>(SyncType::Fence); // keep a simple FrameSync in case other systems expect it
	   SEDX_CORE_TRACE_TAG("Renderer", "Created frame sync objects (fences: {}, render semaphores: {})",
							static_cast<uint32_t>(s_FenceHandles.size()),
							static_cast<uint32_t>(s_RenderSemaphoreHandles.size()));

		m_FrameSync->SetUserCmdList(nullptr);
		SEDX_CORE_TRACE_TAG("Renderer", " Frame resources created");
	}

	// Create synchronization primitives (fences & semaphores) used per-frame and per-swapchain-image.
	/*
	void Renderer::CreateFrameResources()
	{
		std::scoped_lock lock(s_FrameSyncMutex);

		SEDX_CORE_TRACE_TAG("Renderer", "CreateFrameResources() - entering");

		// Clear any existing resources
		s_FenceRefs.clear();
		s_FenceHandles.clear();

		s_PresentSemaphoreRefs.clear();
		s_PresentSemaphoreHandles.clear();

		s_RenderSemaphoreRefs.clear();
		s_RenderSemaphoreHandles.clear();

		// Acquire device
		Ref<Device> device = RenderContext::Get()->GetDevice();
		VkDevice vkDevice = VK_NULL_HANDLE;
		if (device.IsValid())
		{
			vkDevice = device->GetDevice();
		}

		// Reserve vector capacity
		s_FenceRefs.reserve(MAX_FRAMES_IN_FLIGHT);
		s_FenceHandles.reserve(MAX_FRAMES_IN_FLIGHT);

		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			Ref<Fence> fence = CreateRef<Fence>();
			if (vkDevice != VK_NULL_HANDLE)
			{
				fence->CreateSyncObject(vkDevice);
			}
			else
			{
				fence->CreateSyncObject();
			}
			s_FenceRefs.push_back(fence);
			s_FenceHandles.push_back(fence->GetFence());
		}

		SEDX_CORE_TRACE_TAG("Renderer", "CreateFrameResources() - created {} fences", s_FenceRefs.size());

		// Present semaphores: one per frame-in-flight
		s_PresentSemaphoreRefs.reserve(MAX_FRAMES_IN_FLIGHT);
		s_PresentSemaphoreHandles.reserve(MAX_FRAMES_IN_FLIGHT);
		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			Ref<Semaphore> sem = CreateRef<Semaphore>(SyncType::Semaphore);
			if (vkDevice != VK_NULL_HANDLE)
			{
				sem->CreateSyncObject(vkDevice);
			}
			else
			{
				sem->CreateSyncObject();
			}
			s_PresentSemaphoreRefs.push_back(sem);
			s_PresentSemaphoreHandles.push_back(sem->GetSemaphore());
		}

		SEDX_CORE_TRACE_TAG("Renderer", "CreateFrameResources() - created {} present semaphores", s_PresentSemaphoreRefs.size());

		// Render semaphores: one per swapchain image
		uint32_t imageCount = 0;
		if (s_Swapchain)
		{
			imageCount = static_cast<uint32_t>(s_Swapchain->GetImages().size());
		}
		if (imageCount == 0)
			imageCount = SWAPCHAIN_BUFFER_COUNT;

		s_RenderSemaphoreRefs.reserve(imageCount);
		s_RenderSemaphoreHandles.reserve(imageCount);
		for (uint32_t i = 0; i < imageCount; ++i)
		{
			Ref<Semaphore> sem = CreateRef<Semaphore>(SyncType::Semaphore);
			if (vkDevice != VK_NULL_HANDLE)
			{
				sem->CreateSyncObject(vkDevice);
			}
			else
			{
				sem->CreateSyncObject();
			}
			s_RenderSemaphoreRefs.push_back(sem);
			s_RenderSemaphoreHandles.push_back(sem->GetSemaphore());
		}

		SEDX_CORE_TRACE_TAG("Renderer", "CreateFrameResources() - created {} render semaphores for swapchain images", s_RenderSemaphoreRefs.size());
	}
	*/

	/*
	void Renderer::DestroyFrameResources()
	{
		SEDX_CORE_TRACE_TAG("Renderer", "Destroying frame resources");
		std::scoped_lock lock(s_FrameSyncMutex);

		SEDX_CORE_TRACE_TAG("Renderer", "DestroyFrameResources() - entering: fences={}, presentSem={}, renderSem={}",
			s_FenceRefs.size(), s_PresentSemaphoreRefs.size(), s_RenderSemaphoreRefs.size());

		// Release strong references so underlying objects can destroy their Vulkan handles
		for (auto &f : s_FenceRefs)
		{
			if (f)
			{
				f.Reset();
			}
		}
		s_FenceRefs.clear();
		s_FenceHandles.clear();

		for (auto &s : s_PresentSemaphoreRefs)
		{
			if (s)
			{
			 s.Reset();
			}
		}
		s_PresentSemaphoreRefs.clear();
		s_PresentSemaphoreHandles.clear();

		for (auto &s : s_RenderSemaphoreRefs)
		{
			if (s)
			{
			 s.Reset();
			}
		}
		s_RenderSemaphoreRefs.clear();
		s_RenderSemaphoreHandles.clear();

		// Clear any pointers exposed to RenderContext to avoid dangling references
		if (RenderContext::IsInitialized())
		{
			RenderContext::Get()->s_Fences = s_FenceHandles.empty() ? nullptr : &s_FenceHandles;
			RenderContext::Get()->s_PresentSemaphores = s_PresentSemaphoreHandles.empty() ? nullptr : &s_PresentSemaphoreHandles;
			RenderContext::Get()->s_RenderSemaphores = s_RenderSemaphoreHandles.empty() ? nullptr : &s_RenderSemaphoreHandles;
		}

		SEDX_CORE_TRACE_TAG("Renderer", "DestroyFrameResources() - complete");
	}
	*/

	/*
	void Renderer::DestroyFrameResources()
	{
		SEDX_CORE_TRACE_TAG("Renderer", "Destroying frame resources");
		std::scoped_lock lock(s_FrameSyncMutex);

		// Release strong references so underlying objects can enqueue their
		// Vulkan handle destruction via their Destroy() implementations.
		for (auto &f : s_FenceRefs)
		{
			if (f)
			{
			 f.Reset();
			}
		}
		s_FenceRefs.clear();
		s_FenceHandles.clear();

		for (auto &s : s_PresentSemaphoreRefs)
		{
			if (s)
			{
			 s.Reset();
			}
		}
		s_PresentSemaphoreRefs.clear();
		s_PresentSemaphoreHandles.clear();

		for (auto &s : s_RenderSemaphoreRefs)
		{
			if (s)
			{
			 s.Reset();
			}
		}
		s_RenderSemaphoreRefs.clear();
		s_RenderSemaphoreHandles.clear();

		// Clear any pointers exposed to RenderContext to avoid dangling references
		if (RenderContext::IsInitialized())
		{
			RenderContext::Get()->s_Fences = s_FenceHandles.empty() ? nullptr : &s_FenceHandles;
			RenderContext::Get()->s_PresentSemaphores = s_PresentSemaphoreHandles.empty() ? nullptr : &s_PresentSemaphoreHandles;
			RenderContext::Get()->s_RenderSemaphores = s_RenderSemaphoreHandles.empty() ? nullptr : &s_RenderSemaphoreHandles;
		}
	}
	*/

	void Renderer::DestroyFrameResources()
	{
		SEDX_CORE_TRACE_TAG("Renderer", "Destroying frame resources");

	 // Destroy semaphores and fences through wrapper Destroy() so they get scheduled for deletion properly.
		{
			std::scoped_lock lock(s_FrameSyncMutex);
			for (auto &semRef : s_RenderSemaphoreRefs)
			{
				if (semRef)
				{
					semRef->Destroy();
					semRef.Reset();
				}
			}
			s_RenderSemaphoreHandles.clear();
			s_RenderSemaphoreRefs.clear();
		}
		SEDX_CORE_TRACE_TAG("Renderer", " Destroyed render semaphores");

		{
			std::scoped_lock lock(s_FrameSyncMutex);
			for (auto &fRef : s_FenceRefs)
			{
				if (fRef)
				{
					fRef->Destroy();
					fRef.Reset();
				}
			}
			s_FenceHandles.clear();
			s_FenceRefs.clear();
		}
		SEDX_CORE_TRACE_TAG("Renderer", " Destroyed fences");

		// Command buffers are freed when command pool is destroyed
		m_CommandBuffers.fill(VK_NULL_HANDLE);
		SEDX_CORE_TRACE_TAG("Renderer", " Freed command buffers ({} buffers)", MAX_FRAMES_IN_FLIGHT);

		// Destroy command pool
		if (m_CommandPool)
		{
			m_CommandPool->Destroy();
			m_CommandPool.reset();
			SEDX_CORE_TRACE_TAG("Renderer", " Destroyed command pool");
		}

		// Reset simple FrameSync wrapper
		m_FrameSync.reset();
		SEDX_CORE_TRACE_TAG("Renderer", " Destroyed frame sync objects");

		if (s_UniformBuffers)
		{
			s_UniformBuffers->Destroy();
			s_UniformBuffers.reset();
			SEDX_CORE_TRACE_TAG("Renderer", " Destroyed uniform buffer set");
		}

		if (Ref<RenderContext> context = RenderContext::Get(); context)
		{
			context->s_ShaderDataBuffers = nullptr;
			context->s_CommandBuffers = nullptr;
			context->s_Fences = nullptr;
			context->s_PresentSemaphores = nullptr;
			context->s_RenderSemaphores = nullptr;
			context->renderables = nullptr;
		}

		SEDX_CORE_TRACE_TAG("Renderer", " Frame resources destroyed");
	}

#pragma endregion

#pragma region Render Context Management

	Ref<RenderContext> Renderer::GetRenderContext()
	{
		return RenderContext::Get();
	}

	uint32_t Renderer::GetCurrentFrameIndex()
	{
		return m_CurrentFrameIndex;
	}

	uint64_t Renderer::GetFrameNumber()
	{
		return m_FrameNumber;
	}

	void Renderer::RenderThreadFunc(RenderThread *renderThread)
	{
		while (renderThread->IsRunning())
		{
			WaitAndRender(renderThread);
		}
	}
	
	void Renderer::WaitAndRender(RenderThread* renderThread)
	{
		auto& performanceTimers = Application::Get().m_PerformanceTimers;

		// Wait for kick, then set render thread to busy
		{
			Timer waitTimer;
			renderThread->WaitAndSet(RenderThread::State::Kick, RenderThread::State::Busy);
			performanceTimers.RenderThreadWaitTime = waitTimer.ElapsedMillis();
		}

		Timer workTimer; // Measure the time spent doing rendering work this frame (between kick and completion)

		if (BeginFrame())
		{
			Tick();
			EndFrame();
			SubmitAndPresent();
		}
		
		// Rendering has completed, set state to idle
		renderThread->Set(RenderThread::State::Idle);

		performanceTimers.RenderThreadWorkTime = workTimer.ElapsedMillis();
	}

#pragma endregion

#pragma region Swapchain Management 

	Swapchain *Renderer::GetSwapChain()
	{
		return s_Swapchain.Get();
	}

	uint32_t Renderer::GetSwapchainImageIndex()
	{
		return m_SwapchainImageIndex;
	}

#pragma endregion

#pragma region Viewport & Image Management

	const Viewport &Renderer::GetViewport()
	{
		return s_Viewport;
	}

	void Renderer::SetViewport(float width, float height)
	{
		constexpr float epsilon = 1e-5f;

		// Check if absolute value is greater than epsilon (instead of != 0)
		SEDX_CORE_ASSERT(std::abs(width) > epsilon, "Width can't be zero");
		SEDX_CORE_ASSERT(std::abs(height) > epsilon, "Height can't be zero");

		// Check if the difference is greater than epsilon (instead of !=)
		if (std::abs(s_Viewport.width - width) > epsilon || std::abs(s_Viewport.height - height) > epsilon)
		{
			s_Viewport.width = width;
			s_Viewport.height = height;
			s_OrthoProjection_Dirty.SetDirty();
		}

		SEDX_CORE_TRACE_TAG("Renderer", "Viewport set to {}x{}", width, height);
	}

	const Vec2 &Renderer::GetRendererResolution()
	{
		return s_RendererResolution;
	}

	void Renderer::SetRendererResolution(uint32_t width, uint32_t height, const bool recreateResources)
	{

		// Check if the difference is smaller than epsilon (safe ==)
		if (constexpr float epsilon = 1e-5f;
			std::abs(s_RendererResolution.x - static_cast<float>(width)) < epsilon &&
			std::abs(s_RendererResolution.y - static_cast<float>(height)) < epsilon)
		{
			return;
		}

		s_RendererResolution.x = static_cast<float>(width);
		s_RendererResolution.y = static_cast<float>(height);

		if (recreateResources && m_ResourcesInitialized)
		{
			// Wait for GPU to finish before recreating resources
			if (RenderContext::Get()->GetDevice())
			{
				RenderContext::Get()->GetDevice()->GetQueueManager()->WaitIdleAll();
			}

			CreateRenderTargets(true, false, true);
		}

		SEDX_CORE_TRACE_TAG("Renderer", "Render resolution set to {}x{}", width, height);
	}

	const Vec2 &Renderer::GetOutputResolution()
	{
		return s_OutputResolution;
	}

	void Renderer::SetOutputResolution(uint32_t width, uint32_t height, bool recreateResources)
	{

		// Check if the difference is smaller than epsilon (safe ==)
		if (constexpr float epsilon = 1e-5f; std::abs(s_OutputResolution.x - static_cast<float>(width)) < epsilon &&
											 std::abs(s_OutputResolution.y - static_cast<float>(height)) < epsilon)
		{
			return;
		}

		s_OutputResolution.x = static_cast<float>(width);
		s_OutputResolution.y = static_cast<float>(height);

		if (recreateResources && m_ResourcesInitialized)
		{
			CreateRenderTargets(false, true, false);
		}

		SEDX_CORE_TRACE_TAG("Renderer", "Output resolution set to {}x{}", width, height);
	}

#pragma endregion

#pragma region Command Buffer Access 

	void Renderer::SetCamera(Camera* camera)
	{
		m_Camera = camera;
		SEDX_CORE_INFO_TAG("Renderer", "Active camera {}", camera ? "set" : "cleared");
	}

	Camera* Renderer::GetCamera()
	{
		return m_Camera;
	}

	void Renderer::CreateCameraResources()
	{
		VkDevice device = RenderContext::Get()->GetDevice()->GetLogicalDevice();
		VmaAllocator vma = RenderContext::Get()->GetDevice()->GetMemoryAllocator().GetAllocator();

		SEDX_CORE_TRACE_TAG("Renderer", "Creating camera resources");

		// --- Descriptor set layout: one UBO binding at set 1, binding 0 ---
		VkDescriptorSetLayoutBinding uboBinding{};
		uboBinding.binding         = 0;
		uboBinding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboBinding.descriptorCount = 1;
		uboBinding.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo dslCI{};
		dslCI.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		dslCI.bindingCount = 1;
		dslCI.pBindings    = &uboBinding;

		SEDX_VK_RESULT_ASSERT(vkCreateDescriptorSetLayout(device, &dslCI, nullptr, &m_CameraDescriptorSetLayout),
							  "Failed to create camera descriptor set layout");

		// --- Per-frame UBO buffers ---
		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			VkBufferCreateInfo bufCI{};
			bufCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufCI.size  = sizeof(CameraShaderData);
			bufCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

			VmaAllocationCreateInfo allocCI{};
			allocCI.usage = VMA_MEMORY_USAGE_AUTO;
			allocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
							VMA_ALLOCATION_CREATE_MAPPED_BIT;

			VmaAllocationInfo allocInfo{};
			if (vmaCreateBuffer(vma, &bufCI, &allocCI,
								&m_CameraUboBuffers[i],
								&m_CameraUboAllocations[i],
								&allocInfo) != VK_SUCCESS)
			{
				SEDX_CORE_ERROR_TAG("Renderer", "Failed to create camera UBO buffer {}", i);
				continue;
			}
			m_CameraUboMapped[i] = allocInfo.pMappedData;

			// Write identity matrices as initial data so the first frame is stable
			CameraShaderData defaultData{};
			defaultData.view                  = Mat4(1.0f);
			defaultData.projection            = Mat4(1.0f);
			defaultData.viewProjection        = Mat4(1.0f);
			defaultData.inverseViewProjection = Mat4(1.0f);
			// Start slightly away from origin to avoid near-plane issues.
			defaultData.positionWorld = Vec3(
				0.0f,		// X left/right
				3.0f,		// Y up/down
				-5.0f);	// Z forward/backward

			if (m_CameraUboMapped[i])
			{
				std::memcpy(m_CameraUboMapped[i], &defaultData, sizeof(CameraShaderData));
			}
		}

		SEDX_CORE_TRACE_TAG("Renderer", "Created camera UBO buffers ({} frames)", MAX_FRAMES_IN_FLIGHT);

		// --- Descriptor pool and sets ---
		VkDescriptorPoolSize poolSize{};
		poolSize.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;

		VkDescriptorPoolCreateInfo poolCI{};
		poolCI.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCI.maxSets       = MAX_FRAMES_IN_FLIGHT;
		poolCI.poolSizeCount = 1;
		poolCI.pPoolSizes    = &poolSize;

		SEDX_VK_RESULT_ASSERT(vkCreateDescriptorPool(device, &poolCI, nullptr, &m_CameraDescriptorPool),
							  "Failed to create camera descriptor pool");

		SEDX_CORE_TRACE_TAG("Renderer", "Created camera descriptor pool");

		std::array<VkDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts;
		layouts.fill(m_CameraDescriptorSetLayout);

		VkDescriptorSetAllocateInfo dsAllocInfo{};
		dsAllocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		dsAllocInfo.descriptorPool     = m_CameraDescriptorPool;
		dsAllocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
		dsAllocInfo.pSetLayouts        = layouts.data();

		SEDX_VK_RESULT_ASSERT(vkAllocateDescriptorSets(device, &dsAllocInfo, m_CameraDescriptorSets.data()),
							  "Failed to allocate camera descriptor sets");
		
		SEDX_CORE_TRACE_TAG("Renderer", "Allocated camera descriptor sets");

		// Point each descriptor set at its per-frame UBO
		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			VkDescriptorBufferInfo bufInfo{};
			bufInfo.buffer = m_CameraUboBuffers[i];
			bufInfo.offset = 0;
			bufInfo.range  = sizeof(CameraShaderData);

			VkWriteDescriptorSet write{};
			write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.dstSet          = m_CameraDescriptorSets[i];
			write.dstBinding      = 0;
			write.dstArrayElement = 0;
			write.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			write.descriptorCount = 1;
			write.pBufferInfo     = &bufInfo;
			vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
		}

		SEDX_CORE_TRACE_TAG("Renderer", "Updated camera descriptor sets");
		SEDX_CORE_INFO_TAG("Renderer", "Camera UBO resources created ({} frames)", MAX_FRAMES_IN_FLIGHT);
	}

	VkCommandBuffer Renderer::GetCurrentCommandBuffer()
	{
		return m_CommandBuffers[m_CurrentFrameIndex];
	}

	CommandList* Renderer::GetCommandListPresent()
	{
		return m_CmdList_Present;
	}

	CommandList* Renderer::GetCommandListFrame()
	{
		return m_CmdList_Frame;
	}

	void Renderer::CreateModels()
	{
		SEDX_CORE_TRACE_TAG("Renderer", "Creating models and loading assets");
		VmaAllocationCreateInfo bufferAllocCI{};
		bufferAllocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
							  VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT;
		bufferAllocCI.usage = VMA_MEMORY_USAGE_AUTO;
		//bufferAllocCI.pool = VK_NULL_HANDLE; // Only set if using a custom pool

		VmaAllocator allocator = RenderContext::Get()->GetDevice()->GetMemoryAllocator().GetAllocator();

		// GetQueue() returns Ref<Queue>*, so we need to get the pointer first
		Ref<Queue> *queuePtr = RenderContext::Get()->GetDevice()->GetQueueManager()->GetQueue(Graphics);
		if (!queuePtr || !*queuePtr)
		{
			SEDX_CORE_ERROR_TAG("Renderer", "Graphics queue not available, skipping test model load");
			return;
		}

		if (!m_CommandPool)
		{
			SEDX_CORE_ERROR_TAG("Renderer", "Command pool not initialized, skipping test model load");
			return;
		}
		// DIAGNOSTIC: Log current working directory
		std::filesystem::path cwd = std::filesystem::current_path();
		SEDX_CORE_INFO_TAG("Renderer", "Current working directory: {}", cwd.string());

		// DIAGNOSTIC: Check if model file exists
		std::filesystem::path modelPath = ResolveResourcePath("resources/models/suzanne.obj");
		SEDX_CORE_TRACE_TAG("Renderer", "Looking for model at: {}", std::filesystem::absolute(modelPath).string());
		SEDX_CORE_TRACE_TAG("Renderer", "Model file exists: {}", std::filesystem::exists(modelPath));

		std::vector<std::string> texFiles = {
			ResolveResourcePath("resources/textures/suzanne0.ktx").string(),
			ResolveResourcePath("resources/textures/suzanne1.ktx").string(),
			ResolveResourcePath("resources/textures/suzanne2.ktx").string()
		};

		// Only proceed if file exists
		if (!std::filesystem::exists(modelPath))
		{
			SEDX_CORE_ERROR_TAG("Renderer", "Model file not found at expected path: {}", std::filesystem::absolute(modelPath).string());
			return; // Skip model loading instead of asserting
		}

		// Directly load test model, bypassing AssetManager for bootstrap testing
		m_TestModel = CreateScope<Model>();
		bool modelLoaded = m_TestModel->Load(allocator, m_CommandPool->GetPool(), (*queuePtr)->GetQueue(),
									   modelPath.string(), texFiles, bufferAllocCI);
		if (!modelLoaded)
		{
			SEDX_CORE_ERROR_TAG("Renderer", "Failed to load test model: {}", modelPath.string());
			m_TestModel.reset();
			return;
		}

		const Model& asset  = *m_TestModel;
		VkBuffer vBuffer    = asset.GetBuffer();
		VkDeviceSize vBufSize  = asset.GetVertexBufferSize();
		VkDeviceSize iBufSize  = asset.GetIndexBufferSize();
		uint32_t indexCount    = asset.GetIndexCount();
		SEDX_CORE_INFO_TAG("Renderer", "Loaded test model: vertex buffer {}, vertex size {}, index size {}, index count {}",
									static_cast<void *>(vBuffer), vBufSize, iBufSize, indexCount);

		// Create uniform buffers as static resource (per-frame) managed by UniformBufferSet RAII helper
		s_UniformBuffers = CreateScope<UniformBufferSet>(allocator);
		if (s_UniformBuffers)
		{
			s_UniformBuffers->Create();
		}
	}

	/**
	 * TODO: Move the shader loading and Slang initialization to a core ShaderManager to 
	 * avoid having it directly in the Renderer and allow other systems to access 
	 * compiled shader blobs as needed without depending on the Renderer.
	 */
	void Renderer::CreateShaders()
	{
		SEDX_CORE_TRACE_TAG("Renderer", "Creating shaders and initializing Slang shader compiler");

#pragma region Slang Shader Compilation
		// Initialize Slang shader compiler
		Slang::ComPtr<slang::IGlobalSession> slangGlobalSession;
		slang::createGlobalSession(slangGlobalSession.writeRef());
		auto slangTargets{std::to_array<slang::TargetDesc>(
			{{
				.format = SLANG_SPIRV, 
				.profile = slangGlobalSession->findProfile("spirv_1_4")
				}}
		)};
		auto slangOptions{std::to_array<slang::CompilerOptionEntry>(
			{{
				.name = slang::CompilerOptionName::EmitSpirvDirectly,
				.value = {.kind = slang::CompilerOptionValueKind::Int, .intValue0 = 1}
			}}
		)};

		slang::SessionDesc slangSessionDesc = {};
		slangSessionDesc.targets = slangTargets.data();
		slangSessionDesc.targetCount = static_cast<SlangInt>(slangTargets.size());

		// Keep row-major matrix layout to match existing CPU-side xMath uploads.
		slangSessionDesc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_ROW_MAJOR;
		slangSessionDesc.compilerOptionEntries = slangOptions.data();
		slangSessionDesc.compilerOptionEntryCount = static_cast<uint32_t>(slangOptions.size());

#pragma endregion

		// Load shader
		Slang::ComPtr<slang::ISession> slangSession;
		Slang::ComPtr<slang::IBlob> diagnosticsBlob; // Blob to capture any diagnostics from shader compilation
		slangGlobalSession->createSession(slangSessionDesc, slangSession.writeRef());

#pragma region Shader Module Loading
		const std::filesystem::path shaderPath = ResolveResourcePath("resources/shaders/shader.slang");
		const std::string shaderPathString = shaderPath.string();

		Slang::ComPtr<slang::IModule> slangModule{slangSession->loadModuleFromSource("triangle", shaderPathString.c_str(),
																					 diagnosticsBlob, diagnosticsBlob.writeRef())};
		if (!slangModule)
		{
			SEDX_CORE_ERROR_TAG("Renderer", "Failed to load shader module from source: {}", shaderPathString);
			if (diagnosticsBlob)
			{
				const char *errorMessage = static_cast<const char *>(diagnosticsBlob->getBufferPointer());
				SEDX_CORE_ERROR_TAG("Renderer", "Slang diagnostics: {}", errorMessage);
			}
			else
			{
				SEDX_CORE_ERROR_TAG("Renderer", "No diagnostics available from Slang.");
			}
			return;
		}
#pragma region Grid Shader
		Slang::ComPtr<ISlangBlob> gridSpirv;
		{
			const std::filesystem::path gridShaderPath = ResolveResourcePath("resources/shaders/grid.slang");
			const std::string gridShaderPathString = gridShaderPath.string();

			Slang::ComPtr<slang::IBlob> gridDiagnosticsBlob;
			Slang::ComPtr<slang::IModule> gridModule{
				slangSession->loadModuleFromSource("grid", gridShaderPathString.c_str(),
												   gridDiagnosticsBlob, gridDiagnosticsBlob.writeRef())
			};

			if (!gridModule)
			{
				SEDX_CORE_ERROR_TAG("Renderer", "Failed to load grid shader module from source: {}", gridShaderPathString);
				if (gridDiagnosticsBlob)
				{
					const char* errorMessage = static_cast<const char*>(gridDiagnosticsBlob->getBufferPointer());
					SEDX_CORE_ERROR_TAG("Renderer", "Grid shader diagnostics: {}", errorMessage);
				}
			}
			else
			{
				gridModule->getTargetCode(0, gridSpirv.writeRef());
				if (gridSpirv && gridSpirv->getBufferSize() > 0)
				{
					SEDX_CORE_INFO_TAG("Renderer", "Grid shader module compiled: {}", gridShaderPathString);
					SetShaderAvailable(Renderer_Shader::grid_vertex);
					SetShaderAvailable(Renderer_Shader::grid_frag);
				}
			}
		}
#pragma endregion
#pragma region Common Shader Module
		Slang::ComPtr<ISlangBlob> commonSpirv;
		{
			const std::filesystem::path commonShaderPath = ResolveResourcePath("resources/shaders/common.slang");
			const std::string commonShaderPathString = commonShaderPath.string();
			Slang::ComPtr<slang::IBlob> commonDiagnosticsBlob;
			Slang::ComPtr<slang::IModule> commonModule{slangSession->loadModuleFromSource("common", commonShaderPathString.c_str(),
												   commonDiagnosticsBlob, commonDiagnosticsBlob.writeRef())
			};
			if (!commonModule)
			{
				SEDX_CORE_ERROR_TAG("Renderer", "Failed to load common shader module from source: {}", commonShaderPathString);
				if (commonDiagnosticsBlob)
				{
					const char* errorMessage = static_cast<const char*>(commonDiagnosticsBlob->getBufferPointer());
					SEDX_CORE_ERROR_TAG("Renderer", "Common shader diagnostics: {}", errorMessage);
				}
			}
		}
#pragma endregion
#pragma region Constant Shader Module
		Slang::ComPtr<ISlangBlob> constantSpirv;
		{
			const std::filesystem::path constantsShaderPath = ResolveResourcePath("resources/shaders/constants.slang");
			const std::string constantsShaderPathString = constantsShaderPath.string();
			Slang::ComPtr<slang::IBlob> constantsDiagnosticsBlob;
			Slang::ComPtr<slang::IModule> constantsModule{slangSession->loadModuleFromSource("constants", constantsShaderPathString.c_str(),
												   constantsDiagnosticsBlob, constantsDiagnosticsBlob.writeRef())
			};
			if (!constantsModule)
			{
				SEDX_CORE_ERROR_TAG("Renderer", "Failed to load constants shader module from source: {}", constantsShaderPathString);
				if (constantsDiagnosticsBlob)
				{
					const char* errorMessage = static_cast<const char*>(constantsDiagnosticsBlob->getBufferPointer());
					SEDX_CORE_ERROR_TAG("Renderer", "Constants shader diagnostics: {}", errorMessage);
				}
			}
		}
#pragma endregion
#pragma region Resources Shader Module
		Slang::ComPtr<ISlangBlob> resourcesSpirv;
		{
			const std::filesystem::path resourcesShaderPath = ResolveResourcePath("resources/shaders/resources.slang");
			const std::string resourcesShaderPathString = resourcesShaderPath.string();
			Slang::ComPtr<slang::IBlob> resourcesDiagnosticsBlob;
			Slang::ComPtr<slang::IModule> resourcesModule{slangSession->loadModuleFromSource("resources", resourcesShaderPathString.c_str(),
												   resourcesDiagnosticsBlob, resourcesDiagnosticsBlob.writeRef())
			};
			if (!resourcesModule)
			{
				SEDX_CORE_ERROR_TAG("Renderer", "Failed to load resources shader module from source: {}", resourcesShaderPathString);
				if (resourcesDiagnosticsBlob)
				{
					const char* errorMessage = static_cast<const char*>(resourcesDiagnosticsBlob->getBufferPointer());
					SEDX_CORE_ERROR_TAG("Renderer", "Resources shader diagnostics: {}", errorMessage);
				}
			}
		}
#pragma endregion
#pragma region Math Shader Module
		Slang::ComPtr<ISlangBlob> mathSpirv;
		{
			const std::filesystem::path mathShaderPath = ResolveResourcePath("resources/shaders/math.slang");
			const std::string mathShaderPathString = mathShaderPath.string();
			Slang::ComPtr<slang::IBlob> mathDiagnosticsBlob;
			Slang::ComPtr<slang::IModule> mathModule{slangSession->loadModuleFromSource("math", mathShaderPathString.c_str(),
												   mathDiagnosticsBlob, mathDiagnosticsBlob.writeRef())
			};
			if (!mathModule)
			{
				SEDX_CORE_ERROR_TAG("Renderer", "Failed to load math shader module from source: {}", mathShaderPathString);
				if (mathDiagnosticsBlob)
				{
					const char* errorMessage = static_cast<const char*>(mathDiagnosticsBlob->getBufferPointer());
					SEDX_CORE_ERROR_TAG("Renderer", "Math shader diagnostics: {}", errorMessage);
				}
			}
		}
#pragma endregion
#pragma region Color Shader Module
		Slang::ComPtr<ISlangBlob> colorSpirv;
		{
			const std::filesystem::path colorShaderPath = ResolveResourcePath("resources/shaders/color.slang");
			const std::string colorShaderPathString = colorShaderPath.string();
			Slang::ComPtr<slang::IBlob> colorDiagnosticsBlob;
			Slang::ComPtr<slang::IModule> colorModule{slangSession->loadModuleFromSource("color", colorShaderPathString.c_str(),
												   colorDiagnosticsBlob, colorDiagnosticsBlob.writeRef())
			};
			if (!colorModule)
			{
				SEDX_CORE_ERROR_TAG("Renderer", "Failed to load color shader module from source: {}", colorShaderPathString);
				if (colorDiagnosticsBlob)
				{
					const char* errorMessage = static_cast<const char*>(colorDiagnosticsBlob->getBufferPointer());
					SEDX_CORE_ERROR_TAG("Renderer", "Color shader diagnostics: {}", errorMessage);
				}
			}
		}
#pragma region Output Shader Module
		Slang::ComPtr<ISlangBlob> outputSpirv;
		{
			const std::filesystem::path outputShaderPath = ResolveResourcePath("resources/shaders/output.slang");
			const std::string outputShaderPathString = outputShaderPath.string();
			Slang::ComPtr<slang::IBlob> outputDiagnosticsBlob;
			Slang::ComPtr<slang::IModule> outputModule{slangSession->loadModuleFromSource("output", outputShaderPathString.c_str(),
												   outputDiagnosticsBlob, outputDiagnosticsBlob.writeRef())
			};
			if (!outputModule)
			{
				SEDX_CORE_ERROR_TAG("Renderer", "Failed to load output shader module from source: {}", outputShaderPathString);
				if (outputDiagnosticsBlob)
				{
					const char* errorMessage = static_cast<const char*>(outputDiagnosticsBlob->getBufferPointer());
					SEDX_CORE_ERROR_TAG("Renderer", "Output shader diagnostics: {}", errorMessage);
				}
			}
			else
			{
				outputModule->getTargetCode(0, outputSpirv.writeRef());
				if (outputSpirv && outputSpirv->getBufferSize() > 0)
				{
					SEDX_CORE_INFO_TAG("Renderer", "Output shader module compiled: {}", outputShaderPathString);
					SetShaderAvailable(Renderer_Shader::output_c);
				}
			}
		}
#pragma endregion
#pragma region Blit Shader

		// Compile blit module and mark availability (actual compute path is still gated
		// until CommandList pipeline/descriptor wiring is completed).
		{
			const std::filesystem::path blitShaderPath = ResolveResourcePath("resources/shaders/blit.slang");
			const std::string blitShaderPathString = blitShaderPath.string();

			Slang::ComPtr<slang::IBlob> blitDiagnosticsBlob;
			Slang::ComPtr<slang::IModule> blitModule{slangSession->loadModuleFromSource("blit", blitShaderPathString.c_str(),
												   blitDiagnosticsBlob, blitDiagnosticsBlob.writeRef())
			};

			if (!blitModule)
			{
				SEDX_CORE_ERROR_TAG("Renderer", "Failed to load blit shader module from source: {}", blitShaderPathString);
				if (blitDiagnosticsBlob)
				{
					const char* errorMessage = static_cast<const char*>(blitDiagnosticsBlob->getBufferPointer());
					SEDX_CORE_ERROR_TAG("Renderer", "Blit shader diagnostics: {}", errorMessage);
				}
			}
			else
			{
				SEDX_CORE_INFO_TAG("Renderer", "Blit shader module compiled: {}", blitShaderPathString);
				SetShaderAvailable(Renderer_Shader::blit_c);
			}
		}
#pragma endregion
#pragma endregion

		Slang::ComPtr<ISlangBlob> spirv;
		slangModule->getTargetCode(0, spirv.writeRef());

		// Persist shader modules as a static member so they outlive this function.
		m_BasicShaderManager = CreateScope<ShaderManager>(spirv->getBufferPointer(), spirv->getBufferSize());
		if (!m_BasicShaderManager || !m_BasicShaderManager->IsCompiled())
		{
			SEDX_CORE_ERROR_TAG("Renderer", "Shader compilation produced no modules");
			return;
		}

		VkDevice device = RenderContext::Get()->GetDevice()->GetLogicalDevice();

		// The Slang shader uses `uniform ShaderData *shaderData` (pointer), which Slang
		// compiles to a push-constant block holding an 8-byte buffer device address.
		VkPushConstantRange pushConst{};
		pushConst.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		pushConst.offset     = 0;
		pushConst.size       = sizeof(VkDeviceAddress);

		// Set 0 = texture sampler array (populated by Asset)
		// Set 1 = camera UBO  (CameraShaderData, matches CameraBufferData in camera.slang)
		SEDX_CORE_TRACE_TAG("Renderer", "Creating camera resources");
		CreateCameraResources();

		SEDX_CORE_TRACE_TAG("Renderer", "Creating grid shader resources");
		if (gridSpirv && gridSpirv->getBufferSize() > 0)
		{
			m_GridShaderManager = CreateScope<ShaderManager>(gridSpirv->getBufferPointer(), gridSpirv->getBufferSize());

			if (m_GridShaderManager && m_GridShaderManager->IsCompiled())
			{

				VkPipelineLayoutCreateInfo gridLayoutCI{};
				gridLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
				gridLayoutCI.setLayoutCount = 1;
				gridLayoutCI.pSetLayouts = &m_CameraDescriptorSetLayout;

				// Must match PassBufferData in resources.slang:
				// uint4 + float4[3] = 64 bytes.
				VkPushConstantRange gridPushConstantRange{};
				gridPushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
				gridPushConstantRange.offset = 0;
				gridPushConstantRange.size = 64;
				gridLayoutCI.pushConstantRangeCount = 1;
				gridLayoutCI.pPushConstantRanges = &gridPushConstantRange;

				SEDX_VK_RESULT_ASSERT(vkCreatePipelineLayout(device, &gridLayoutCI, nullptr, &m_GridPipelineLayout),
									  "Failed to create grid pipeline layout");

				// Matches grid.slang GridVertex: float4 position + float2 uv (stride 24 bytes)
				struct GridVertex
				{
					float position[4]; // float4 - must match grid.slang
					float uv[2];
				};

				VkVertexInputBindingDescription gridBinding{};
				gridBinding.binding = 0;
				gridBinding.stride = sizeof(GridVertex);
				gridBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			   std::vector<VkVertexInputAttributeDescription> gridAttributes(1);
				gridAttributes[0] = {
					.location = 0,
					.binding  = 0,
					.format   = VK_FORMAT_R32G32B32A32_SFLOAT,
					.offset   = static_cast<uint32_t>(offsetof(GridVertex, position))
				};

				Pipeline::GraphicsCreateInfo gridPipeCI{};
				gridPipeCI.device = device;
				gridPipeCI.layout = m_GridPipelineLayout;
				gridPipeCI.shaderManager = m_GridShaderManager.get();
				gridPipeCI.vertexEntryPoint = "main_vs";
				gridPipeCI.fragmentEntryPoint = "main_frag";
				gridPipeCI.vertexBinding = gridBinding;
				gridPipeCI.vertexAttributes = gridAttributes;
				gridPipeCI.colorFormat = s_Swapchain->GetImageFormat();
				gridPipeCI.depthFormat = s_Swapchain->GetDepthFormat();

				m_GridPipeline = Pipeline::CreateGraphics(gridPipeCI);

				const std::array<GridVertex, 4> gridVertices = {
					GridVertex{{-1.0f, -1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
					GridVertex{{ 1.0f, -1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
					GridVertex{{ 1.0f,  1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
					GridVertex{{-1.0f,  1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}
				};
				const std::array<uint32_t, 6> gridIndices = {0, 1, 2, 2, 3, 0};

				VmaAllocator vma = RenderContext::Get()->GetDevice()->GetMemoryAllocator().GetAllocator();

				VkBufferCreateInfo vbCI{};
				vbCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				vbCI.size = sizeof(gridVertices);
				vbCI.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

				VmaAllocationCreateInfo allocCI{};
				allocCI.usage = VMA_MEMORY_USAGE_AUTO;
				allocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

				VmaAllocationInfo vbAllocInfo{};
				if (vmaCreateBuffer(vma, &vbCI, &allocCI, &m_GridVertexBuffer, &m_GridVertexAllocation, &vbAllocInfo) == VK_SUCCESS)
				{
					if (vbAllocInfo.pMappedData)
					{
						std::memcpy(vbAllocInfo.pMappedData, gridVertices.data(), sizeof(gridVertices));
					}
					else
					{
						SEDX_CORE_WARN_TAG("Renderer", "Grid vertex buffer allocation returned null mapped pointer");
					}
				}

				VkBufferCreateInfo ibCI{};
				ibCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				ibCI.size = sizeof(gridIndices);
				ibCI.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

				VmaAllocationInfo ibAllocInfo{};
				if (vmaCreateBuffer(vma, &ibCI, &allocCI, &m_GridIndexBuffer, &m_GridIndexAllocation, &ibAllocInfo) == VK_SUCCESS)
				{
				  if (ibAllocInfo.pMappedData)
					{
						std::memcpy(ibAllocInfo.pMappedData, gridIndices.data(), sizeof(gridIndices));
						m_GridIndexCount = static_cast<uint32_t>(gridIndices.size());
					}
					else
					{
						SEDX_CORE_WARN_TAG("Renderer", "Grid index buffer allocation returned null mapped pointer");
					}
				}
			}
		}

		// Bypassing asset manager for now and providing a single mesh item for testing.
		if (!m_TestModel)
		{
			SEDX_CORE_WARN_TAG("Renderer", "Test model not loaded; pipeline creation deferred");
			return;
		}

		Model& asset = *m_TestModel;

		VkDescriptorSetLayout textureLayout = asset.GetDescriptorLayout();
		const std::array<VkDescriptorSetLayout, 2> setLayouts = {textureLayout, m_CameraDescriptorSetLayout};
		VkPipelineLayoutCreateInfo layoutCI{};
		layoutCI.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutCI.setLayoutCount         = static_cast<uint32_t>(setLayouts.size());
		layoutCI.pSetLayouts            = setLayouts.data();
		layoutCI.pushConstantRangeCount = 1;
		layoutCI.pPushConstantRanges    = &pushConst;

		SEDX_VK_RESULT_ASSERT(vkCreatePipelineLayout(device, &layoutCI, nullptr, &m_BasicPipelineLayout), "Failed to create basic pipeline layout");

		Pipeline::GraphicsCreateInfo pipeCI{};
		pipeCI.device           = device;
		pipeCI.layout           = m_BasicPipelineLayout;
		pipeCI.shaderManager    = m_BasicShaderManager.get();
		pipeCI.vertexBinding    = Model::GetVertexBindingDescription();
		pipeCI.vertexAttributes = Model::GetVertexAttributeDescriptions();
		pipeCI.colorFormat      = s_Swapchain->GetImageFormat();
		pipeCI.depthFormat      = s_Swapchain->GetDepthFormat();

		m_BasicPipeline = Pipeline::CreateGraphics(pipeCI);
		if (m_BasicPipeline == VK_NULL_HANDLE)
		{
			SEDX_CORE_ERROR_TAG("Renderer", "Pipeline::CreateGraphics failed");
			return;
		}

		if (Ref<RenderContext> context = RenderContext::Get(); context)
		{
			context->pipeline = m_BasicPipeline;
			context->pipelineLayout = m_BasicPipelineLayout;
		}

		SEDX_CORE_INFO_TAG("Renderer", "Basic graphics pipeline created successfully");

		// Per-frame shader-data buffers (device-addressable, host-visible, mapped).
		// Layout must match the Slang shader's ShaderData struct:
		//   model[3], lightPos, selected  — projection/view are now in the camera UBO.
		struct BasicShaderData
		{
			Mat4     model[3];
			Vec4     lightPos;
			uint32_t selected;
			uint32_t _pad[3];
		};

		VmaAllocator vma = RenderContext::Get()->GetDevice()->GetMemoryAllocator().GetAllocator();

		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			VkBufferCreateInfo uboCI{};
			uboCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			uboCI.size  = sizeof(BasicShaderData);
			uboCI.usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

			VmaAllocationCreateInfo allocCI{};
			allocCI.usage = VMA_MEMORY_USAGE_AUTO;
			allocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
							VMA_ALLOCATION_CREATE_MAPPED_BIT;

			VmaAllocationInfo allocInfo{};
			if (vmaCreateBuffer(vma, &uboCI, &allocCI,
								&m_BasicShaderDataBuffers[i],
								&m_BasicShaderDataAllocations[i],
								&allocInfo) != VK_SUCCESS)
			{
				SEDX_CORE_ERROR_TAG("Renderer", "Failed to create shader-data buffer {}", i);
				continue;
			}
			m_BasicShaderDataMapped[i] = allocInfo.pMappedData;

			VkBufferDeviceAddressInfo bdaInfo{};
			bdaInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
			bdaInfo.buffer = m_BasicShaderDataBuffers[i];
			m_BasicShaderDataAddresses[i] = vkGetBufferDeviceAddress(device, &bdaInfo);

			// Initial transforms: three Suzanne instances spread on the X axis.
			BasicShaderData sd{};

			// Identity for model[0], translations for model[1] and model[2]
			sd.model[0]      = Mat4(1.0f);
			sd.model[1]      = Mat4::Translate(Vec3(-3.0f, 0.0f, 0.0f));
			sd.model[2]      = Mat4::Translate(Vec3( 3.0f, 0.0f, 0.0f));
			sd.lightPos      = Vec4(0.0f, 5.0f, 5.0f, 1.0f);
			sd.selected      = 0;

			if (m_BasicShaderDataMapped[i])
			{
				std::memcpy(m_BasicShaderDataMapped[i], &sd, sizeof(sd));
			}
		}
	}

	void Renderer::Screenshot()
	{
		/*
		static uint32_t screenshot_index = 0;

		ImageResource* frame_output = GetRenderTarget(Renderer_RenderTarget::frame_output);
		uint32_t width            = frame_output->GetWidth();
		uint32_t height           = frame_output->GetHeight();
		uint32_t bits_per_channel = frame_output->GetBitsPerChannel();
		uint32_t channel_count    = frame_output->GetChannelCount();
		size_t data_size          = static_cast<size_t>(width) * height * (bits_per_channel / 8) * channel_count;

		bool is_hdr = cvar_hdr.GetValueAs<bool>();

		auto staging = CreateRef<Buffer>(RHI_Buffer_Type::Constant, data_size, 1, nullptr, true, "screenshot_staging");

		if (CommandList* cmd_list = CommandList::BeginImmediateExecution(QueueType::Graphics))
		{
			cmd_list->CopyTextureToBuffer(frame_output, staging.get());
			CommandList::EndImmediateExecution(cmd_list);
		}

		void* mapped_data = staging->GetMappedData();
		SEDX_CORE_ASSERT(mapped_data, "Staging buffer not mappable");

		uint32_t index = screenshot_index++;
		std::string exr_path = "screenshot_" + std::to_string(index) + ".exr";
		std::string png_path = "screenshot_" + std::to_string(index) + ".png";

		ThreadPool::Submit([=]()
		{
			SEDX_CORE_INFO_TAG("Renderer", "Saving screenshots...");

			ImageImporter::Save(exr_path, width, height, channel_count, bits_per_channel, mapped_data);

			ImageImporter::SaveSdr(png_path, width, height, channel_count, bits_per_channel, mapped_data, is_hdr);

			SEDX_CORE_INFO_TAG("Renderer", "Screenshots saved as '{}' and '{}'", exr_path.c_str(), png_path.c_str());
		});
		*/

	}

#pragma endregion

#pragma region Private Rendering Methods

	void Renderer::RecordRenderCommands(VkCommandBuffer cb, uint32_t imageIndex)
	{
		SEDX_CORE_TRACE_TAG("Renderer", "Recording render commands for image index {}", imageIndex);
		if (!s_Swapchain)
		{
			SEDX_CORE_TRACE_TAG("Renderer", "Swapchain is null, cannot record render commands");
			return;
		}

		auto &swapchainImages = s_Swapchain->GetImages();
		auto &swapchainImageViews = s_Swapchain->GetImageViews();
		VkImageView depthImageView = s_Swapchain->GetDepthView();
		VkImage depthImage = s_Swapchain->GetDepthImage();

		if (imageIndex >= swapchainImages.size() || imageIndex >= swapchainImageViews.size())
		{
			SEDX_CORE_TRACE_TAG("Renderer", "Invalid image index: {}", imageIndex);
			return;
		}

		// If pass-render output exists, present that texture to the swapchain first.
		// This allows the pass-based pipeline (including editor grid) to appear in-window.
		// NOTE: Disabled — frame_output is never rendered into during bootstrap, so it is
		// always in VK_IMAGE_LAYOUT_UNDEFINED. The barrier below that expects
		// SHADER_READ_ONLY_OPTIMAL would trigger a validation error and produce a black frame.
		// Re-enable this path once the pass-based renderer writes into frame_output each frame.
		const bool useFrameOutputBlitPath = false;
		if (ImageResource* frameOutput = GetRenderTarget(Renderer_RenderTarget::frame_output);
			useFrameOutputBlitPath && frameOutput && frameOutput->Get() && *frameOutput->Get() != VK_NULL_HANDLE)
		{
			VkImageMemoryBarrier2 barriers[2]{};

			barriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			barriers[0].srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			barriers[0].srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
			barriers[0].dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
			barriers[0].dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
			barriers[0].oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barriers[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barriers[0].image = *frameOutput->Get();
			barriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barriers[0].subresourceRange.levelCount = 1;
			barriers[0].subresourceRange.layerCount = 1;

			barriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			barriers[1].srcStageMask = VK_PIPELINE_STAGE_2_NONE;
			barriers[1].srcAccessMask = 0;
			barriers[1].dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
			barriers[1].dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			barriers[1].oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			barriers[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barriers[1].image = swapchainImages[imageIndex];
			barriers[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barriers[1].subresourceRange.levelCount = 1;
			barriers[1].subresourceRange.layerCount = 1;

			VkDependencyInfo depInfo{};
			depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			depInfo.imageMemoryBarrierCount = 2;
			depInfo.pImageMemoryBarriers = barriers;
			vkCmdPipelineBarrier2(cb, &depInfo);

			VkImageBlit blit{};
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = 0;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;
			blit.srcOffsets[0] = {
				.x = 0, 
				.y = 0, 
				.z = 0
			};
			blit.srcOffsets[1] = {
				.x = static_cast<int32_t>(frameOutput->GetWidth()),
				.y = static_cast<int32_t>(frameOutput->GetHeight()),
				.z = 1
			};

			VkExtent2D extent = s_Swapchain->GetExtent();
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = 0;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;
			blit.dstOffsets[0] = {
				.x = 0, .y = 0, .z = 0
			};
			blit.dstOffsets[1] = {
				.x = static_cast<int32_t>(extent.width),
				.y = static_cast<int32_t>(extent.height),
				.z = 1 
			};

			vkCmdBlitImage(cb,
				*frameOutput->Get(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				swapchainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit, VK_FILTER_LINEAR);

			VkImageMemoryBarrier2 postBlit[2]{};

			// Restore pass output image layout for subsequent pass sampling/writes.
			postBlit[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			postBlit[0].srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
			postBlit[0].srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
			postBlit[0].dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			postBlit[0].dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
			postBlit[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			postBlit[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			postBlit[0].image = *frameOutput->Get();
			postBlit[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			postBlit[0].subresourceRange.levelCount = 1;
			postBlit[0].subresourceRange.layerCount = 1;

			// Transition swapchain image back to present for vkQueuePresentKHR.
			postBlit[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			postBlit[1].srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
			postBlit[1].srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			postBlit[1].dstStageMask = VK_PIPELINE_STAGE_2_NONE;
			postBlit[1].dstAccessMask = 0;
			postBlit[1].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			postBlit[1].newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			postBlit[1].image = swapchainImages[imageIndex];
			postBlit[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			postBlit[1].subresourceRange.levelCount = 1;
			postBlit[1].subresourceRange.layerCount = 1;

			VkDependencyInfo depPostBlit{};
			depPostBlit.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			depPostBlit.imageMemoryBarrierCount = 2;
			depPostBlit.pImageMemoryBarriers = postBlit;
			vkCmdPipelineBarrier2(cb, &depPostBlit);

			return;
		}

		if (depthImage == VK_NULL_HANDLE || depthImageView == VK_NULL_HANDLE)
		{
			SEDX_CORE_ERROR_TAG("Renderer", "Depth attachment is invalid (image: {}, view: {}), skipping render command recording", static_cast<void*>(depthImage), static_cast<void*>(depthImageView));
			return;
		}

		// Transition images to attachment optimal.
		// Use UNDEFINED as old layout to make the frame start robust across swapchain recreation.
		// We clear color/depth every frame, so previous contents are not needed.
		const VkImageLayout colorOldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		const VkPipelineStageFlags2 colorSrcStage = VK_PIPELINE_STAGE_2_NONE;
		const VkAccessFlags2 colorSrcAccess = 0;

		std::array<VkImageMemoryBarrier2, 2> outputBarriers{
			VkImageMemoryBarrier2{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
				.srcStageMask = colorSrcStage,
				.srcAccessMask = colorSrcAccess,
				.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				.oldLayout = colorOldLayout,
				.newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
				.image = swapchainImages[imageIndex],
				.subresourceRange{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, 
					.levelCount = 1, 
					.layerCount = 1}
			},
			VkImageMemoryBarrier2{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
				.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
				.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
				.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
				.image = depthImage,
				.subresourceRange{.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
								  .levelCount = 1,
								  .layerCount = 1}
			}
		};

		SEDX_CORE_TRACE_TAG("Renderer", "Transitioning swapchain image {} and depth image to attachment optimal layout", imageIndex);

		VkDependencyInfo barrierDependencyInfo{};
		barrierDependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		barrierDependencyInfo.imageMemoryBarrierCount = 2;
		barrierDependencyInfo.pImageMemoryBarriers = outputBarriers.data();
		SEDX_CORE_TRACE_TAG("Renderer", "Issuing pipeline barrier for image layout transitions to attachment optimal");

		vkCmdPipelineBarrier2(cb, &barrierDependencyInfo);

		// -----------------------------------------------------------------

		// Begin dynamic rendering
		VkRenderingAttachmentInfo colorAttachmentInfo{.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
													  .imageView = swapchainImageViews[imageIndex],
													  .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
													  .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
													  .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
													  .clearValue{.color{{0.0f, 0.0f, 0.0f, 1.0f}}}
		};
		SEDX_CORE_TRACE_TAG("Renderer", "Configured color attachment for dynamic rendering");

		VkRenderingAttachmentInfo depthAttachmentInfo{.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
													  .imageView = depthImageView,
													  .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
													  .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
													  .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
													  .clearValue = {.depthStencil = {
														  .depth = 1.0f, 
														  .stencil = 0}
													  }
		};
		SEDX_CORE_TRACE_TAG("Renderer", "Configured depth attachment for dynamic rendering");

		VkExtent2D extent = s_Swapchain->GetExtent();
		VkRenderingInfo renderingInfo{
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
			.renderArea{.extent{.width = extent.width, .height = extent.height}},
			.layerCount = 1,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colorAttachmentInfo,
			.pDepthAttachment = &depthAttachmentInfo
		};

		SEDX_CORE_TRACE_TAG("Renderer", "Beginning dynamic rendering with extent {}x{}", extent.width, extent.height);

		vkCmdBeginRendering(cb, &renderingInfo);

		SEDX_CORE_TRACE_TAG("Renderer", "Dynamic rendering begun, recording draw commands");

		// Set viewport and scissor
		VkViewport vp{};
		vp.width = static_cast<float>(extent.width);
		vp.height = static_cast<float>(extent.height);
		vp.minDepth = 0.0f;
		vp.maxDepth = 1.0f;

		vkCmdSetViewport(cb, 0, 1, &vp);
		SEDX_CORE_TRACE_TAG("Renderer", "Viewport set to {}x{}", vp.width, vp.height);

		VkRect2D scissor{.extent{.width = extent.width, .height = extent.height}};
		vkCmdSetScissor(cb, 0, 1, &scissor);
		SEDX_CORE_TRACE_TAG("Renderer", "Scissor set to {}x{}", scissor.extent.width, scissor.extent.height);

		// Bind the basic pipeline and draw every loaded asset
		// Upload camera UBO for this frame (view/projection offloaded to camera.slang UBO at set 1)
		{
			CameraShaderData cameraData{};
			Camera* activeCamera = Scene::GetCamera();
			if (!activeCamera)
			{
				activeCamera = m_Camera;
			}

			if (activeCamera)
			{
			 cameraData = activeCamera->GetShaderData();
				m_Camera = activeCamera;
			}
			else
			{
			  // No synthetic camera fallback here. Keep identity/default UBO data when
				// no scene-owned camera is available.
			}

			if (m_CameraUboMapped[m_CurrentFrameIndex])
			{
				std::memcpy(m_CameraUboMapped[m_CurrentFrameIndex], &cameraData, sizeof(CameraShaderData));
			}
		}

		// Per-frame shader-data update: rotate model matrices driven by the frame counter.
		// Projection/view are now owned by the camera UBO; only model/light/selected live here.
		if (m_BasicShaderDataMapped[m_CurrentFrameIndex])
		{
			struct BasicShaderData
			{
				Mat4     model[3];
				Vec4     lightPos;
				uint32_t selected;
				uint32_t _pad[3];
			};

			auto* pSd = static_cast<BasicShaderData*>(m_BasicShaderDataMapped[m_CurrentFrameIndex]);

			// Static model placements — no rotation.
			// Model transforms will be driven by the scene/ECS once wired up.
			pSd->model[0]      = Mat4(1.0f);
			pSd->model[1]      = Mat4::Translate(Vec3(-3.0f, 0.0f, 0.0f));
			pSd->model[2]      = Mat4::Translate(Vec3( 3.0f, 0.0f, 0.0f));
		}

		if (m_BasicPipeline != VK_NULL_HANDLE && m_TestModel && m_BasicShaderDataAddresses[m_CurrentFrameIndex] != 0)
		{
			Model& asset = *m_TestModel;

			vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, m_BasicPipeline);
			vkCmdSetCullMode(cb, VK_CULL_MODE_BACK_BIT);

			// Set 0: texture array
			VkDescriptorSet descSet = asset.GetDescriptorSet();
			vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS,
									m_BasicPipelineLayout,
									0, 1, &descSet,
									0, nullptr);

			// Set 1: camera UBO (view / projection)
			if (m_CameraDescriptorSets[m_CurrentFrameIndex] != VK_NULL_HANDLE)
			{
				vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS,
										m_BasicPipelineLayout,
										1, 1, &m_CameraDescriptorSets[m_CurrentFrameIndex],
										0, nullptr);
			}

			// Push the device address of this frame's shader-data buffer.
			VkDeviceAddress addr = m_BasicShaderDataAddresses[m_CurrentFrameIndex];
			vkCmdPushConstants(cb, m_BasicPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VkDeviceAddress), &addr);

			// The Model packs vertices then indices into a single VkBuffer.
			VkBuffer     vertBuf    = asset.GetBuffer();
			VkDeviceSize vertOffset = 0;
			vkCmdBindVertexBuffers(cb, 0, 1, &vertBuf, &vertOffset);
			vkCmdBindIndexBuffer(cb, vertBuf, asset.GetVertexBufferSize(), VK_INDEX_TYPE_UINT16);

			// Draw 3 instances (one per model[0..2] in the shader's ShaderData).
			vkCmdDrawIndexed(cb, asset.GetIndexCount(), 3, 0, 0, 0);
		}

		if (m_GridPipeline != VK_NULL_HANDLE && m_GridPipelineLayout != VK_NULL_HANDLE &&
			m_GridVertexBuffer != VK_NULL_HANDLE && m_GridIndexBuffer != VK_NULL_HANDLE &&
			m_GridIndexCount > 0 && m_CameraDescriptorSets[m_CurrentFrameIndex] != VK_NULL_HANDLE)
		{
				SEDX_CORE_TRACE_TAG("Renderer", "[Grid] Issuing grid draw for frame={} frameIndex={} imageIndex={} pipeline={} layout={} vb={} ib={} indexCount={}",
				m_FrameNumber, m_CurrentFrameIndex, imageIndex,
				static_cast<void*>(m_GridPipeline),
				static_cast<void*>(m_GridPipelineLayout),
				static_cast<void*>(m_GridVertexBuffer),
				static_cast<void*>(m_GridIndexBuffer), m_GridIndexCount);

				vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GridPipeline);
				SEDX_CORE_TRACE_TAG("Renderer", "[Grid] vkCmdBindPipeline issued");
				vkCmdSetCullMode(cb, VK_CULL_MODE_NONE);

				vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GridPipelineLayout,
									0, 1, &m_CameraDescriptorSets[m_CurrentFrameIndex],
									0, nullptr);

			struct GridPushConstants
			{
				uint32_t drawIndex;
				uint32_t materialIndex;
				uint32_t isTransparent;
				uint32_t padding;
				float values[3][4];
			};

			GridPushConstants gridPush{};
			gridPush.drawIndex = 0;
			gridPush.materialIndex = 0;
			gridPush.isTransparent = 0;
			gridPush.padding = 0;
			gridPush.values[0][0] = 5000.0f; // grid half-extent

			vkCmdPushConstants(cb, m_GridPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0, sizeof(GridPushConstants), &gridPush);

			VkDeviceSize gridOffset = 0;
			vkCmdBindVertexBuffers(cb, 0, 1, &m_GridVertexBuffer, &gridOffset);
			vkCmdBindIndexBuffer(cb, m_GridIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
			SEDX_CORE_TRACE_TAG("Renderer", "[Grid] vkCmdBindVertexBuffers + vkCmdBindIndexBuffer issued");
			vkCmdDrawIndexed(cb, m_GridIndexCount, 1, 0, 0, 0);
				SEDX_CORE_TRACE_TAG("Renderer", "[Grid] vkCmdDrawIndexed issued");
			}

			// Render ImGui UI inside the active dynamic render pass so all draw calls
			// execute on the swapchain image with a valid pipeline and attachment binding.
			// We redirect m_CmdList_Present's internal VkCommandBuffer to 'cb' (which has
			// the render pass open) for the duration of the UI render, then restore it.
			if (m_CmdList_Present && m_CmdList_Present->GetState() == CommandState::Recording)
			{
				if (ImGui::GetCurrentContext() && ImGui::GetDrawData())
				{
					m_CmdList_Present->SetExternalRecordingBuffer(cb, /*renderPassActive=*/true);
					UI::Render(ImGui::GetDrawData(), nullptr, false);
					m_CmdList_Present->RestoreCommandBuffer();
				}
			}

			vkCmdEndRendering(cb);
		SEDX_CORE_TRACE_TAG("Renderer", "Dynamic rendering ended");

		// Ensure the swapchain image is transitioned to the present layout so
		// vkQueuePresentKHR sees the image in VK_IMAGE_LAYOUT_PRESENT_SRC_KHR.
		// When using dynamic rendering or manual transitions earlier in the
		// frame some implementations require an explicit transition here.
		{
			VkImage swapchainImage = swapchainImages[imageIndex];
			VkImageMemoryBarrier presentBarrier{};
			presentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			presentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			presentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			presentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			presentBarrier.dstAccessMask = 0;
			presentBarrier.oldLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
			presentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			presentBarrier.image = swapchainImage;
			presentBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			presentBarrier.subresourceRange.baseMipLevel = 0;
			presentBarrier.subresourceRange.levelCount = 1;
			presentBarrier.subresourceRange.baseArrayLayer = 0;
			presentBarrier.subresourceRange.layerCount = 1;

			// Ensure color attachment writes are finished before present
			vkCmdPipelineBarrier(cb,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &presentBarrier);
		}
	}

	void Renderer::SetCommonTextures(CommandList* cmdList)
	{
		// gbuffer
		cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_albedo,   GetRenderTarget(Renderer_RenderTarget::gbuffer_color));
		cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_normal,   GetRenderTarget(Renderer_RenderTarget::gbuffer_normal));
		cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_material, GetRenderTarget(Renderer_RenderTarget::gbuffer_material));
		cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_velocity, GetRenderTarget(Renderer_RenderTarget::gbuffer_velocity));
		cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_depth,    GetRenderTarget(Renderer_RenderTarget::gbuffer_depth));

		// ssao (white = no occlusion when disabled)
		ImageResource* texSsao = GetRenderTarget(Renderer_RenderTarget::ssao);
		cmdList->SetTexture(Renderer_BindingsSrv::ssao, texSsao);
	}

	void Renderer::UpdateShadowAtlas()
	{
		const uint32_t resolutionAtlas = GetRenderTarget(Renderer_RenderTarget::shadow_atlas)->GetWidth();
		constexpr uint32_t minSliceRes = 256;
		constexpr uint32_t border = 8;

		// collect slices
		m_ShadowSlices.clear();
		for (Entity* entity : Scene::GetEntities())
		{
			if (!entity)
				continue;

			Light* light = entity->GetComponent<Light>();
			if (!light)
				continue;

			light->ClearAtlasRectangles();
			if (light->GetIndex() == std::numeric_limits<uint32_t>::max())
				continue;

			for (uint32_t i = 0; i < light->GetSliceCount(); ++i)
			{
				m_ShadowSlices.emplace_back(light, i, 0, xMath::Rectangle::ZERO);
			}
		}

		if (m_ShadowSlices.empty())
			return;

		// row-based packing: lays out uniform-sized slices left-to-right, wrapping to the next row.
		// when rects is null it only tests whether the layout fits; when non-null it writes the rectangles.
		auto pack_row = [&](uint32_t sliceRes, uint32_t numSlices, std::vector<ShadowSlice>* rects) -> bool
		{
			if (sliceRes > resolutionAtlas)
				return false;

			uint32_t x = 0, y = 0, rowH = 0;
			for (uint32_t i = 0; i < numSlices; ++i)
			{
				uint32_t leftPad = (x == 0) ? 0 : border;
				uint32_t placedX = x + leftPad;

				if (placedX + sliceRes > resolutionAtlas)
				{
					y        += rowH + border;
					x        = 0;
					rowH     = 0;
					placedX  = 0;
				}

				if (placedX + sliceRes > resolutionAtlas || y + sliceRes > resolutionAtlas)
					return false;

				if (rects)
				{
					(*rects)[i].res  = sliceRes;
					(*rects)[i].rect = xMath::Rectangle(static_cast<float>(placedX), static_cast<float>(y), static_cast<float>(sliceRes), static_cast<float>(sliceRes));
				}

				x = placedX + sliceRes;
				rowH = xMath::Max(rowH, sliceRes);
			}
			return true;
		};

		// binary search for max uniform slice resolution
		uint32_t maxSliceRes = resolutionAtlas;
		uint32_t numSlices    = static_cast<uint32_t>(m_ShadowSlices.size());
		if (numSlices > 1)
		{
			uint32_t low  = minSliceRes;
			uint32_t high = resolutionAtlas;
			while (low < high)
			{
				uint32_t mid = (low + high + 1) / 2;
				if (pack_row(mid, numSlices, nullptr))
				{
					low = mid;
				}
				else
				{
					high = mid - 1;
				}
			}
			maxSliceRes = low;
		}
		maxSliceRes = xMath::Max(maxSliceRes, minSliceRes);

		// assign rectangles
		pack_row(maxSliceRes, numSlices, &m_ShadowSlices);

		for (const auto& slice : m_ShadowSlices)
		{
			slice.light->SetAtlasRectangle(slice.slice_Index, slice.rect);
		}
	}

	void Renderer::UpdateDrawCalls(CommandList *cmdList)
	{
		// TODO: Implement draw call collection and sorting when the scene and material systems are integrated
		(void)cmdList;

		/*
		m_DrawCall_Count          = 0;
		m_DrawCalls_Prepass_Count = 0;
		m_DrawData_Count           = 0;
		m_Transparents_Present     = false;


		if (ProgressTracker::IsLoading())
			return;


		// collect draw calls
		{
			for (Entity* entity : Scene::GetEntities())
			{
				if (!entity->GetActive())
					continue;

				if (Renderable* renderable = entity->GetComponent<Renderable>())
				{
					Material* material = renderable->GetMaterial();
					if (!material)
						continue;

					if (material->IsTransparent())
					{
						m_Transparents_Present = true;
					}

					uint32_t draw_data_index = WriteDrawData(
						entity->GetMatrix(),
						entity->GetMatrixPrevious(),
						material->GetIndex(),
						material->IsTransparent() ? 1 : 0
					);

					Renderer_DrawCall& draw_call = m_DrawCalls[m_DrawCall_Count++];
					draw_call.renderable         = renderable;
					draw_call.distance_squared   = renderable->GetDistanceSquared();
					draw_call.lodIndex          = renderable->GetLodIndex();
					draw_call.isOccluder        = false;
					draw_call.cameraVisible     = renderable->IsVisible();
					draw_call.instanceIndex     = 0;
					draw_call.instanceCount     = renderable->GetInstanceCount();
					draw_call.drawData_Index    = draw_data_index;
				}
			}

			// sort: opaque before transparent, then material, then distance
			std::sort(m_DrawCalls.begin(), m_DrawCalls.begin() + m_DrawCall_Count, [](const Renderer_DrawCall& a, const Renderer_DrawCall& b)
			{
				bool a_transparent = a.renderable->GetMaterial()->IsTransparent();
				bool b_transparent = b.renderable->GetMaterial()->IsTransparent();
				if (a_transparent != b_transparent)
				{
					return !a_transparent;
				}

				uint64_t a_material_id = a.renderable->GetMaterial()->GetObjectId();
				uint64_t b_material_id = b.renderable->GetMaterial()->GetObjectId();
				if (a_material_id != b_material_id)
				{
					return a_material_id < b_material_id;
				}

				if (!a_transparent)
				{
					return a.distance_squared < b.distance_squared;
				}
				else
				{
					return a.distance_squared > b.distance_squared;
				}
			});
		}

		// prepass: visible opaques, sorted by alpha test then distance
		{
			for (uint32_t i = 0; i < m_DrawCall_Count; ++i)
			{
				const Renderer_DrawCall& dc = m_DrawCalls[i];
				if (!dc.renderable->GetMaterial()->IsTransparent() && dc.cameraVisible)
				{
					m_DrawCalls_Prepass[m_DrawCalls_Prepass_Count++] = dc;
				}
			}

			std::sort(m_DrawCalls_Prepass.begin(), m_DrawCalls_Prepass.begin() + m_DrawCalls_Prepass_Count, [](const Renderer_DrawCall& a, const Renderer_DrawCall& b)
			{
				bool a_alpha = a.renderable->GetMaterial()->IsAlphaTested();
				bool b_alpha = b.renderable->GetMaterial()->IsAlphaTested();
				if (a_alpha != b_alpha)
				{
					return !a_alpha;
				}
				return a.distance_squared < b.distance_squared;
			});
		}

		// indirect draw buffers (gpu-driven path)
		{
			m_Indirect_DrawCount = 0;
			for (uint32_t i = 0; i < m_DrawCall_Count; i++)
			{
				const Renderer_DrawCall& dc = m_DrawCalls[i];
				MaterialAsset* material = dc.renderable->GetMaterial();

				if (!material || material->IsTransparent())
					continue;
				if (IsCpuDrivenDraw(dc, material))
					continue;

				uint32_t idx = m_Indirect_DrawCount++;
				if (idx >= MAX_ARRAY_SIZE)
					break;

				ShaderBuffer_IndirectDrawArgs& args = m_Indirect_DrawArgs[idx];
				args.index_count          = renderable->GetIndexCount(dc.lodIndex);
				args.instance_count       = dc.instanceCount;
				args.first_index          = renderable->GetIndexOffset(dc.lodIndex);
				args.vertex_offset        = static_cast<int32_t>(renderable->GetVertexOffset(dc.lodIndex));
				args.first_instance       = dc.instanceIndex;

				// per-draw data (aabb_index includes the frame offset into the shared aabb buffer)
				uint32_t aabb_frame_offset = m_Frame_Resource_Index * MAX_ARRAY_SIZE;
				ShaderBuffer_DrawData& data = m_Indirect_DrawData[idx];
				Entity* entity          = renderable->GetEntity();
				data.transform          = entity->GetMatrix();
				data.transform_previous = entity->GetMatrixPrevious();
				data.material_index     = material->GetIndex();
				data.is_transparent     = 0;
				data.aabb_index         = aabb_frame_offset + m_DrawCalls_Prepass_Count + idx;
				data.padding            = 0;
			}
		}

		// select occluders (top N by screen area, with temporal hysteresis)
		{
			static std::unordered_set<Renderable*> previous_occluders;

			auto compute_screen_space_area = [&](const BoundingBox& aabb_world) -> float
			{
				float area = 0.0f;
				if (Camera* camera = Scene::GetCamera())
				{
					xMath::Rectangle rect_screen = camera->WorldToScreenCoordinates(aabb_world);
					area = xMath::Clamp(rect_screen.width * rect_screen.height, 0.0f, std::numeric_limits<float>::max());
				}
				return area;
			};

			struct DrawCallArea
			{
				uint32_t index;
				float area;
			};
			static std::vector<DrawCallArea> areas;
			areas.clear();
			areas.reserve(m_DrawCalls_Prepass_Count);

			for (uint32_t i = 0; i < m_DrawCalls_Prepass_Count; i++)
			{
				Renderer_DrawCall& draw_call = m_DrawCalls_Prepass[i];
				Renderable* renderable = draw_call.renderable;
				Material* material = renderable->GetMaterial();

				if (!material || material->IsTransparent() || renderable->HasInstancing() || !draw_call.cameraVisible)
					continue;

				float screen_area = compute_screen_space_area(renderable->GetBoundingBox());

				// temporal hysteresis: bonus for previous occluders
				if (previous_occluders.contains(renderable))
				{
					screen_area *= 1.5f;
				}

				areas.push_back({.index = i, .area = screen_area });
			}

			std::ranges::sort(areas.begin(), areas.end(), [](const DrawCallArea& a, const DrawCallArea& b)
			{
				return a.area > b.area;
			});

			const uint32_t max_occluders = 64;
			uint32_t occluder_count = xMath::Min(max_occluders, static_cast<uint32_t>(areas.size()));

			previous_occluders.clear();
			for (uint32_t i = 0; i < occluder_count; i++)
			{
				m_DrawCalls_Prepass[areas[i].index].isOccluder = true;
				previous_occluders.insert(m_DrawCalls_Prepass[areas[i].index].renderable);
			}
		}*/
	} 

	void Renderer::UpdateCameraUBO(uint32_t frameIndex)
	{
		Camera* camera = Scene::GetCamera();
		if (!camera)
		{
			camera = m_Camera;
		}
		else
		{
			m_Camera = camera;
		}

		if (!camera || !m_CameraUboMapped[frameIndex])
			return;

		// Prepare data for the GPU
		CameraShaderData data;
		data.view                   = camera->GetView();
		data.projection             = camera->GetProjection();
		data.viewProjection         = data.projection * data.view;
		data.inverseViewProjection  = data.viewProjection.GetInverse();
		data.positionWorld          = camera->GetEyePosition();
		data.padding                = 0.0f;

		// Copy to mapped Vulkan buffer
		memcpy(m_CameraUboMapped[frameIndex], &data, sizeof(CameraShaderData));
	}

	void Renderer::UpdateFrameConstantBuffer(CommandList *cmdList)
	{
		/* TODO: This function is currently unused, but will be the basis for per-frame shader data updates once the scene and material systems are integrated. */

		(void)cmdList;

		/*
		// matrices
		{
			if (Camera *camera = Scene::GetCamera())
			{
				if (s_NearPlane != camera->GetNearPlane() || s_FarPlane != camera->GetFarPlane())
				{
					s_NearPlane                    = camera->GetNearPlane();
					s_FarPlane                     = camera->GetFarPlane();
					s_OrthoProjection_Dirty.SetDirty();
				}

				m_Cb_Frame_Cpu.view_previous       = m_Cb_Frame_Cpu.view;
				m_Cb_Frame_Cpu.view                = camera->GetViewMatrix();
				m_Cb_Frame_Cpu.view_inv            = Matrix::Invert(m_Cb_Frame_Cpu.view);
				m_Cb_Frame_Cpu.projection_previous = m_Cb_Frame_Cpu.projection;
				m_Cb_Frame_Cpu.projection          = camera->GetProjectionMatrix();
				m_Cb_Frame_Cpu.projection_inv      = Matrix::Invert(m_Cb_Frame_Cpu.projection);
			}

			if (s_OrthoProjection_Dirty.IsDirty())
			{ 
				// near = 0 for ortho (avoids NaN in [3,2] element)
				Matrix projection_ortho              = Matrix::CreateOrthographicLH(s_Viewport.width, s_Viewport.height, 0.0f, s_FarPlane);
				m_Cb_Frame_Cpu.view_projection_ortho = Matrix::CreateLookAtLH(Vec3(0, 0, -s_NearPlane), Vec3::Forward, Vec3::Up) * projection_ortho;
				s_OrthoProjection_Dirty.Check();
			}
		}

		/*
		// taa jitter
		Renderer_AntiAliasing_Upsampling upsampling_mode = cvar_antialiasing_upsampling.GetValueAs<Renderer_AntiAliasing_Upsampling>();
		{
			if (upsampling_mode == Renderer_AntiAliasing_Upsampling::AA_Fsr_Upscale_Fsr)
			{
				VendorTechnology::FSR3_GenerateJitterSample(&s_JitterOffset.x, &s_JitterOffset.y);
				m_Cb_Frame_Cpu.projection *= Matrix::CreateTranslation(Vec3(s_JitterOffset.x, s_JitterOffset.y, 0.0f));
			}
			else if (upsampling_mode == Renderer_AntiAliasing_Upsampling::AA_Xess_Upscale_Xess)
			{
				VendorTechnology::XeSS_GenerateJitterSample(&s_JitterOffset.x, &s_JitterOffset.y);
				m_Cb_Frame_Cpu.projection *= Matrix::CreateTranslation(Vec3(s_JitterOffset.x, s_JitterOffset.y, 0.0f));
			}
			else
			{
				s_JitterOffset = Vec2::Zero;
			}
		}
		#1#

		m_Cb_Frame_Cpu.view_projection_previous = m_Cb_Frame_Cpu.view_projection;
		m_Cb_Frame_Cpu.view_projection          = m_Cb_Frame_Cpu.view * m_Cb_Frame_Cpu.projection;
		m_Cb_Frame_Cpu.view_projection_inv      = Matrix::Invert(m_Cb_Frame_Cpu.view_projection);
		if (Camera* camera = Scene::GetCamera())
		{
			m_Cb_Frame_Cpu.view_projection_previous_unjittered = m_Cb_Frame_Cpu.view_projection_unjittered;
			m_Cb_Frame_Cpu.view_projection_unjittered          = m_Cb_Frame_Cpu.view * camera->GetProjectionMatrix();
			m_Cb_Frame_Cpu.camera_near                         = camera->GetNearPlane();
			m_Cb_Frame_Cpu.camera_far                          = camera->GetFarPlane();
			m_Cb_Frame_Cpu.camera_position_previous            = m_Cb_Frame_Cpu.camera_position;
			m_Cb_Frame_Cpu.camera_position                     = camera->GetEntity()->GetPosition();
			m_Cb_Frame_Cpu.camera_forward                      = camera->GetEntity()->GetForward();
			m_Cb_Frame_Cpu.camera_right                        = camera->GetEntity()->GetRight();
			m_Cb_Frame_Cpu.camera_fov                          = camera->GetFovHorizontalDeg() * DEG_TO_RAD;
			m_Cb_Frame_Cpu.camera_aperture                     = camera->GetAperture();
			m_Cb_Frame_Cpu.camera_last_movement_time           = Length2((m_Cb_Frame_Cpu.camera_position - m_Cb_Frame_Cpu.camera_position_previous)) != 0.0f
				? static_cast<float>(Timer::GetTimeSec()) : m_Cb_Frame_Cpu.camera_last_movement_time;
		}
		m_Cb_Frame_Cpu.resolution_output   = m_resolution_output;
		m_Cb_Frame_Cpu.resolution_render   = m_resolution_render;
		m_Cb_Frame_Cpu.taa_jitter_previous = m_Cb_Frame_Cpu.taa_jitter_current;
		m_Cb_Frame_Cpu.taa_jitter_current  = s_JitterOffset;
		m_Cb_Frame_Cpu.time                = Timer::GetTimeSec();
		m_Cb_Frame_Cpu.delta_time          = static_cast<float>(DeltaTime::GetSeconds());
		m_Cb_Frame_Cpu.frame               = static_cast<uint32_t>(m_FrameNumber);
		m_Cb_Frame_Cpu.resolution_scale    = cvar_resolution_scale.GetValue();
		m_Cb_Frame_Cpu.hdr_enabled         = cvar_hdr.GetValueAs<bool>() ? 1.0f : 0.0f;
		m_Cb_Frame_Cpu.hdr_max_nits        = Monitor::GetLuminanceMax();
		m_Cb_Frame_Cpu.gamma               = cvar_gamma.GetValue();
		m_Cb_Frame_Cpu.camera_exposure     = Scene::GetCamera() ? Scene::GetCamera()->GetExposure() : 1.0f;

		m_Cb_Frame_Cpu.cloud_coverage = cvar_cloud_coverage.GetValue();
		m_Cb_Frame_Cpu.cloud_shadows  = cvar_cloud_shadows.GetValue();
		// feature bits (must match common_resources.hlsl)
		m_Cb_Frame_Cpu.SetBit(cvar_ssao.GetValueAs<bool>(),                   1 << 1);
		m_Cb_Frame_Cpu.SetBit(cvar_restir_pt.GetValueAs<bool>(),              1 << 3);

		GetBuffer(Renderer_Buffer::ConstantFrame)->Update(cmdList, &m_Cb_Frame_Cpu);
		*/

	}

	uint32_t Renderer::WriteDrawData(const xMath::Matrix &transform, const xMath::Matrix &prevTransform, uint32_t matIndex, uint32_t isTransparent)
	{
		// TODO: Write the transform matrix into the GPU draw-data structured buffer and return its index.
		if (m_DrawData_Count >= static_cast<uint32_t>(m_DrawData_CPU.size()))
		{
			static bool s_LoggedDrawDataOverflow = false;
			if (!s_LoggedDrawDataOverflow)
			{
				SEDX_CORE_ERROR_TAG("Renderer", "WriteDrawData overflow: draw data capacity reached ({})", static_cast<uint32_t>(m_DrawData_CPU.size()));
				s_LoggedDrawDataOverflow = true;
			}

			return m_ResourceIndex * RENDERER_MAX_DRAW_CALLS;
		}

		SEDX_CORE_ASSERT(m_DrawData_Count < RENDERER_MAX_DRAW_CALLS);
		uint32_t index = m_DrawData_Count++;

		ShaderBuffer_DrawData& entry    = m_DrawData_CPU[index];
		entry.transform					= transform;
		entry.transform_previous		= prevTransform;
		entry.material_index			= matIndex;
		entry.is_transparent			= isTransparent;
		entry.aabb_index				= 0;
		entry.padding					= 0;

		// the draw data buffer is a single large allocation partitioned into per-frame regions;
		// each frame writes to its own region so there is no write-after-read race with the gpu
		uint32_t globalIndex = m_ResourceIndex * RENDERER_MAX_DRAW_CALLS + index;

		Buffer* buffer = GetBuffer(Renderer_Buffer::DrawData);
		if (!buffer)
		{
			static bool s_LoggedMissingDrawDataBuffer = false;
			if (!s_LoggedMissingDrawDataBuffer)
			{
				SEDX_CORE_ERROR_TAG("Renderer", "WriteDrawData skipped: DrawData buffer is null");
				s_LoggedMissingDrawDataBuffer = true;
			}
			return globalIndex;
		}

		void* mapped = buffer->GetMappedData();
		if (!mapped)
		{
			mapped = buffer->Map();
		}

		if (mapped)
		{
			void* dst = static_cast<char*>(mapped) + globalIndex * sizeof(ShaderBuffer_DrawData);
			memcpy(dst, &entry, sizeof(ShaderBuffer_DrawData));
		}
		else
		{
			static bool s_LoggedUnmappedDrawDataBuffer = false;
			if (!s_LoggedUnmappedDrawDataBuffer)
			{
				SEDX_CORE_ERROR_TAG("Renderer", "WriteDrawData skipped: DrawData buffer could not be mapped");
				s_LoggedUnmappedDrawDataBuffer = true;
			}
		}

		return globalIndex;
	}

#pragma endregion
		
	GPUMemoryStats Renderer::GetGPUMemoryStats()
	{
		return MemoryAllocator::GetMemoryStats();
	}

	void Renderer::UpdateMaterials(CommandList *cmdList)
	{
		static std::array<ShaderBuffer_Material, MAX_ARRAY_SIZE> properties;
		static std::unordered_set<uint64_t> uniqueMaterialIds;
		uint32_t count = 0;
	
		auto update_material = [&count](MaterialAsset* material)
		{
			if (uniqueMaterialIds.contains(material->GetObjectId()))
				return;
	
			uniqueMaterialIds.insert(material->GetObjectId());
			{
				SEDX_CORE_ASSERT(count < MAX_ARRAY_SIZE, "Exceeded maximum array size for material properties");

				properties[count].local_width           = material->GetProperty(MaterialClass::SceneWidth);
				properties[count].local_height          = material->GetProperty(MaterialClass::SceneHeight);
				properties[count].color.x               = material->GetProperty(MaterialClass::ColorR);
				properties[count].color.y               = material->GetProperty(MaterialClass::ColorG);
				properties[count].color.z               = material->GetProperty(MaterialClass::ColorB);
				properties[count].color.w               = material->GetProperty(MaterialClass::ColorA);
				properties[count].tiling_uv.x           = material->GetProperty(MaterialClass::TextureTilingX);
				properties[count].tiling_uv.y           = material->GetProperty(MaterialClass::TextureTilingY);
				properties[count].offset_uv.x           = material->GetProperty(MaterialClass::TextureOffsetX);
				properties[count].offset_uv.y           = material->GetProperty(MaterialClass::TextureOffsetY);
				properties[count].invert_uv.x           = material->GetProperty(MaterialClass::TextureInvertX);
				properties[count].invert_uv.y           = material->GetProperty(MaterialClass::TextureInvertY);
				properties[count].roughness_mul         = material->GetProperty(MaterialClass::Roughness);
				properties[count].metallic_mul          = material->GetProperty(MaterialClass::Metalness);
				properties[count].normal_mul            = material->GetProperty(MaterialClass::Normal);
				properties[count].height_mul            = material->GetProperty(MaterialClass::Height);
				properties[count].anisotropic           = material->GetProperty(MaterialClass::Anisotropic);
				properties[count].anisotropic_rotation  = material->GetProperty(MaterialClass::AnisotropicRotation);
				properties[count].clearcoat             = material->GetProperty(MaterialClass::Clearcoat);
				properties[count].clearcoat_roughness   = material->GetProperty(MaterialClass::Clearcoat_Roughness);
				properties[count].sheen                 = material->GetProperty(MaterialClass::Sheen);
				properties[count].subsurface_scattering = material->GetProperty(MaterialClass::SubsurfaceScattering);
				properties[count].world_space_uv        = material->GetProperty(MaterialClass::WorldSpaceUv);

				// flags
				properties[count].flags  = material->HasTextureOfType(MaterialTextureType::Height)             ? (1U << 0)  : 0;
				properties[count].flags |= material->HasTextureOfType(MaterialTextureType::Normal)             ? (1U << 1)  : 0;
				properties[count].flags |= material->HasTextureOfType(MaterialTextureType::Color)              ? (1U << 2)  : 0;
				properties[count].flags |= material->HasTextureOfType(MaterialTextureType::Roughness)          ? (1U << 3)  : 0;
				properties[count].flags |= material->HasTextureOfType(MaterialTextureType::Metalness)          ? (1U << 4)  : 0;
				properties[count].flags |= material->HasTextureOfType(MaterialTextureType::AlphaMask)          ? (1U << 5)  : 0;
				properties[count].flags |= material->HasTextureOfType(MaterialTextureType::Emission)           ? (1U << 6)  : 0;
				properties[count].flags |= material->HasTextureOfType(MaterialTextureType::Occlusion)          ? (1U << 7)  : 0;
				properties[count].flags |= material->GetProperty(MaterialProperty::Terrain)                  ? (1U << 8)  : 0;
				properties[count].flags |= material->GetProperty(MaterialProperty::WindAnimation)              ? (1U << 9)  : 0;
				properties[count].flags |= material->GetProperty(MaterialProperty::ColorVariationFromInstance) ? (1U << 10) : 0;
				properties[count].flags |= material->GetProperty(MaterialProperty::GrassBlades)               ? (1U << 11) : 0;
				properties[count].flags |= material->GetProperty(MaterialProperty::Water)                    ? (1U << 13) : 0;
				properties[count].flags |= material->GetProperty(MaterialProperty::Tessellation)               ? (1U << 14) : 0;
				properties[count].flags |= material->GetProperty(MaterialProperty::EmissiveFromAlbedo)         ? (1U << 15) : 0;
				// keep in sync with Surface struct in common_structs.hlsl
			}
	
			// textures
			{
				for (uint32_t type = 0; type < static_cast<uint32_t>(MaterialTextureType::MaxEnum); type++)
				{
					for (uint32_t slot = 0; slot < MaterialAsset::SLOTS_PER_TEXTURE; slot++)
					{
						uint32_t bindlessIndex = count + (type * MaterialAsset::SLOTS_PER_TEXTURE) + slot;
						m_Bindless_Textures[bindlessIndex] = material->GetTexture(static_cast<MaterialTextureType>(type), slot);
					}
				}
			}
	
			material->SetIndex(count);

			count += static_cast<uint32_t>(MaterialTextureType::MaxEnum) * MaterialAsset::SLOTS_PER_TEXTURE;
		};
	
		auto update_entities = [update_material]()
		{
			for (Entity* entity : Scene::GetEntities())
			{
				if (!entity)
					continue;

				if (auto* renderable = entity->GetComponent<Renderable>())
				{
					if (MaterialAsset* material = renderable->GetMaterialAsset())
					{
						update_material(material);
					}
				}
			}
		};
	
		// cpu
		{
			properties.fill(ShaderBuffer_Material{});
			m_Bindless_Textures.fill(nullptr);
			uniqueMaterialIds.clear();
			update_entities();
		}
	
		// GPU
		{
			Buffer* buffer = Renderer::GetBuffer(Renderer_Buffer::MaterialParameters);
			if (!buffer)
			{
				SEDX_CORE_ERROR_TAG("Renderer", "UpdateMaterials: MaterialParameters buffer is null — skipping GPU upload");
				return;
			}
			buffer->ResetOffset();
			if (count > 0)
			{
				buffer->Update(cmdList, &properties[0], buffer->GetStride() * count);
			}
		}
	}

	void Renderer::UpdateLights(CommandList *cmdList)
	{
		const Entity* cameraEntity = Scene::GetCamera() ? Scene::GetCamera()->GetEntity() : nullptr;
		const Vec3 cameraPos    = cameraEntity ? cameraEntity->GetPosition() : Vec3(0.0f, 0.0f, 0.0f);
	
		m_Bindless_Lights.fill(ShaderBuffer_Light());
		
		m_Count_ActiveLights = 0; 
		Light* firstDirectional = nullptr;
	
		auto fill_light = [&](Light* lightComponent)
		{
			const uint32_t index = m_Count_ActiveLights++;
			
			lightComponent->SetIndex(index);
			ShaderBuffer_Light& lightBufferEntry = m_Bindless_Lights[index];
	
			for (uint32_t i = 0; i < lightComponent->GetSliceCount(); i++)
			{
				lightBufferEntry.view_projection[i] = lightComponent->GetViewProjectionMatrix(i);
			}
	
			lightBufferEntry.screen_space_shadows_slice_index  = lightComponent->GetScreenSpaceShadowsSliceIndex();
			lightBufferEntry.intensity                         = lightComponent->GetIntensityWatt();
			lightBufferEntry.range                             = lightComponent->GetRange();
			lightBufferEntry.angle                             = lightComponent->GetAngle();
			{ const Standard& lc = lightComponent->GetLightColor(); lightBufferEntry.color = xMath::Color(lc.r, lc.g, lc.b, lc.a); }
			lightBufferEntry.position                          = lightComponent->GetEntity()->GetPosition();
			lightBufferEntry.direction                         = lightComponent->GetEntity()->GetForward();
			lightBufferEntry.area_width                        = lightComponent->GetAreaWidth();
			lightBufferEntry.area_height                       = lightComponent->GetAreaHeight();
			lightBufferEntry.flags                             = 0;
			lightBufferEntry.flags                            |= lightComponent->GetLightType() == LightType::Directional ? (1 << 0) : 0;
			lightBufferEntry.flags                            |= lightComponent->GetLightType() == LightType::Point       ? (1 << 1) : 0;
			lightBufferEntry.flags                            |= lightComponent->GetLightType() == LightType::Spot        ? (1 << 2) : 0;
			lightBufferEntry.flags                            |= lightComponent->GetFlag(LightFlags::Shadows)             ? (1 << 3) : 0;
			lightBufferEntry.flags                            |= lightComponent->GetFlag(LightFlags::ShadowsScreenSpace)  ? (1 << 4) : 0;
			lightBufferEntry.flags                            |= lightComponent->GetFlag(LightFlags::Volumetric)          ? (1 << 5) : 0;
			lightBufferEntry.flags                            |= lightComponent->GetLightType() == LightType::Area        ? (1 << 6) : 0;
	
			for (uint32_t i = 0; i < 6; i++)
			{
				if (i < lightComponent->GetSliceCount())
				{
					lightBufferEntry.atlas_offsets[i]     = lightComponent->GetAtlasOffset(i);
					lightBufferEntry.atlas_scales[i]      = lightComponent->GetAtlasScale(i);
					const xMath::Rectangle& rect          = lightComponent->GetAtlasRectangle(i);
					lightBufferEntry.atlas_texel_sizes[i] = Vec2(1.0f / rect.width, 1.0f / rect.height);
				}
				else
				{
					lightBufferEntry.atlas_offsets[i] = Vec2::ZERO;
					lightBufferEntry.atlas_scales[i] = Vec2::ZERO;
					lightBufferEntry.atlas_texel_sizes[i] = Vec2::ZERO;
				}
			}
		};
	
		// directional light always goes in slot 0
		for (Entity* entity : Scene::GetEntities())
		{
			if (!entity)
				continue;

			Light* lightComponent = entity->GetComponent<Light>();
			if (!lightComponent)
				continue;

			if (lightComponent->GetLightType() == LightType::Directional)
			{
				firstDirectional = lightComponent;

				// slot 0 is always the sun, even if disabled
				fill_light(lightComponent);
				if (!lightComponent->GetEntity()->GetActive())
				{
					m_Bindless_Lights[0].intensity = 0.0f;
				}
				break;
			}
		}
	
		// remaining lights
		for (Entity* entity : Scene::GetEntities())
		{
			if (Light* lightComponent = entity->GetComponent<Light>())
			{
				if (lightComponent == firstDirectional)
					continue;
	
				lightComponent->SetIndex(std::numeric_limits<uint32_t>::max());
	
				if (!lightComponent->GetEntity()->GetActive())
					continue;
	
				if (lightComponent->GetIntensityWatt() <= 0.0f)
					continue;
	
				if (Camera* camera = Scene::GetCamera())
				{
					if (!camera->IsInViewFrustum(lightComponent->GetBoundingBox()))
						continue;
				}
	
				fill_light(lightComponent);
			}
		}
	
		// gpu upload
		Buffer* buffer = GetBuffer(Renderer_Buffer::LightParameters);
		if (!buffer)
		{
			SEDX_CORE_ERROR_TAG("Renderer", "UpdateLights: LightParameters buffer is null — skipping GPU upload");
			return;
		}
		buffer->ResetOffset();

		if (m_Count_ActiveLights > 0)
		{
			buffer->Update(cmdList, &m_Bindless_Lights[0], buffer->GetStride() * m_Count_ActiveLights);
		}
	}

	void Renderer::UpdateBoundingBoxes(CommandList *cmdList)
	{
		m_Bindless_Aabbs.fill(ShaderBuffer_Aabb());

		// prepass aabbs (must match the indexing in indirect_cull.hlsl)
		for (uint32_t i = 0; i < m_DrawCalls_Prepass_Count; i++)
		{
			const Renderer_DrawCall& drawCall = m_DrawCalls_Prepass[i];
			Renderable* renderable = drawCall.renderable;
			const BoundingBox& aabb = renderable->GetBoundingBox();
			m_Bindless_Aabbs[i].min = aabb.GetMin();
			m_Bindless_Aabbs[i].max = aabb.GetMax();
			m_Bindless_Aabbs[i].is_occluder = drawCall.isOccluder;
		}

		// indirect draw aabbs (stored right after prepass aabbs)
		{
			uint32_t indirectIdx = 0;
			for (uint32_t i = 0; i < m_DrawCall_Count && indirectIdx < m_Indirect_DrawCount; i++)
			{
				const Renderer_DrawCall& dc = m_DrawCalls[i];
				MaterialAsset* material = dc.renderable->GetMaterialAsset();

				if (!material || material->IsTransparent())
					continue;

				if (IsCpuDrivenDraw(dc, material))
					continue;

				uint32_t aabbSlot = m_DrawCalls_Prepass_Count + indirectIdx;
				if (aabbSlot < MAX_ARRAY_SIZE)
				{
					const BoundingBox& aabb       = dc.renderable->GetBoundingBox();
					m_Bindless_Aabbs[aabbSlot].min = aabb.GetMin();
					m_Bindless_Aabbs[aabbSlot].max = aabb.GetMax();
				}
				indirectIdx++;
			}
		}

		// gpu upload to the current frame's region within the shared aabb buffer
		uint32_t totalAabbCount = m_DrawCalls_Prepass_Count + m_Indirect_DrawCount;
		if (totalAabbCount > 0)
		{
			Buffer* buffer = GetBuffer(Renderer_Buffer::AABBs);
			if (!buffer)
			{
				SEDX_CORE_ERROR_TAG("Renderer", "UpdateBoundingBoxes: AABBs buffer is null — skipping GPU upload");
				return;
			}
			uint32_t frameByteOffset = m_FrameResource_Index * MAX_ARRAY_SIZE * static_cast<uint32_t>(sizeof(ShaderBuffer_Aabb));
			uint32_t uploadSize = static_cast<uint32_t>(sizeof(ShaderBuffer_Aabb)) * totalAabbCount;
			CommandList::UpdateBuffer(buffer, frameByteOffset, uploadSize, &m_Bindless_Aabbs[0]);
		}
	}

} // namespace SceneryEditorX

// --------------------------------------------------------------

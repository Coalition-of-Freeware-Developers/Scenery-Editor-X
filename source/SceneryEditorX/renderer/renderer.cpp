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

#include "SceneryEditorX/scene/scene.h"
#include "slang/slang-com-ptr.h"
#include "slang/slang.h"
#include "vulkan/swapchain.h"
#include "vulkan/uniform_buffer_set.h"
#include "vulkan/asset/asset_manager.h"
#include "vulkan/debug/graphics_debug.h"
#include "vulkan/pipeline/pipeline.h"
#include "vulkan/shader/shader_manager.h"
#include <array>
#include <SDL3/SDL.h>
#include <SceneryEditorX/core/application/application.h>
#include <glm/glm.hpp>
#include <volk/volk.h>

// --------------------------------------------------------------

namespace SceneryEditorX
{

#pragma region Static Renderer Properties

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

    RendererProperties *Renderer::s_Data = nullptr;
    static Ref<Swapchain> s_Swapchain = nullptr;
    std::atomic<bool> Renderer::s_ResourcesInitialized = false;
    uint32_t Renderer::m_ResourceIndex = 0;
    Scope<AssetManager> Renderer::s_AssetManager = nullptr;

    // Bindless draw data
    uint32_t Renderer::m_DrawDataCount = 0;
    CommandList *Renderer::m_CmdList_Compute = nullptr;
    CommandList *Renderer::m_CmdList_Present = nullptr;

    Scope<FrameSync> Renderer::s_FrameSync = nullptr;
    Scope<CommandPool> Renderer::s_CommandPool = nullptr;

    std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> Renderer::s_CommandBuffers = {};
    uint32_t Renderer::s_CurrentFrameIndex = 0;
    uint64_t Renderer::s_FrameNumber = 0;
    uint32_t Renderer::s_SwapchainImageIndex = 0;
    bool Renderer::s_FrameInProgress = false;

    // Basic forward pipeline
    VkPipeline        Renderer::s_BasicPipeline        = VK_NULL_HANDLE;
    VkPipelineLayout  Renderer::s_BasicPipelineLayout  = VK_NULL_HANDLE;
    Scope<ShaderManager> Renderer::s_BasicShaderManager = nullptr;
    std::array<VkBuffer,        MAX_FRAMES_IN_FLIGHT> Renderer::s_BasicShaderDataBuffers     = {};
    std::array<VmaAllocation,   MAX_FRAMES_IN_FLIGHT> Renderer::s_BasicShaderDataAllocations = {};
    std::array<void*,           MAX_FRAMES_IN_FLIGHT> Renderer::s_BasicShaderDataMapped      = {};
    std::array<VkDeviceAddress, MAX_FRAMES_IN_FLIGHT> Renderer::s_BasicShaderDataAddresses   = {};

    // Resolution & viewport (internal state)
    static xMath::Vec2 s_RendererResolution(0.0f, 0.0f);
    static xMath::Vec2 s_OutputResolution(0.0f, 0.0f);
    static Viewport s_Viewport = Viewport(0, 0, 0, 0);
    static bool s_OrthoProjection_Dirty = true;
    static Scope<UniformBufferSet> s_UniformBuffers = nullptr;
    static VkSurfaceCapabilitiesKHR s_SurfaceCaps = {};
    static xMath::Vec2 s_JitterOffset(0.0f, 0.0f);
    static float s_NearPlane = 0.0f;
    static float s_FarPlane  = 1.0f;
	static bool s_DirtyOrthographicProjection   = true;
	const uint8_t SWAPCHAIN_BUFFER_COUNT = 2;
	const uint32_t RESOLUTION_SHADOW_MIN = 128;
	constexpr uint32_t renderer_resource_frame_lifetime = MAX_FRAMES_IN_FLIGHT;

    static std::vector<Ref<Fence>> s_FenceRefs;
    static std::vector<VkFence> s_FenceHandles;

    static std::vector<Ref<Semaphore>> s_PresentSemaphoreRefs;
    static std::vector<VkSemaphore> s_PresentSemaphoreHandles;

    static std::vector<Ref<Semaphore>> s_RenderSemaphoreRefs;
    static std::vector<VkSemaphore> s_RenderSemaphoreHandles;

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
	
#pragma endregion

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

        s_Data = new RendererProperties;

        /*
		if (Debugging::IsRenderdocEnabled())
        {
            RenderDoc::OnPreDeviceCreation();
        }
        */

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// SwapChain                                                                                                     ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        if (!Window::IsVisible())
		{
            SEDX_CORE_ERROR_TAG("Swapchain", "Window is not visible or is minimized/hidden. Swapchain creation aborted.");
            return;
        }

        s_Swapchain = CreateRef<Swapchain>();
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

        /*
        // Create the swapchain now that render context is initialized
        if (RenderContext::Get() && s_Swapchain->GetSurface() != VK_NULL_HANDLE)
        {
            s_Swapchain->CreateSwapchain();
            if (s_Swapchain->Get() == VK_NULL_HANDLE)
            {
                SEDX_CORE_ERROR_TAG("Renderer", "Failed to create swapchain");
                return;
            }
        }
        else
        {
            SEDX_CORE_WARN_TAG("Renderer", "Surface not available, swapchain creation deferred");
            return;
        }
        */

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Frame Resources                                                                                               ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        
        s_AssetManager = CreateScope<AssetManager>();
        SEDX_CORE_TRACE_TAG("Renderer", "Created AssetManager");

        CreateFrameResources();
        CreateModels();
        CreateShaders();

        // Query surface capabilities
        VkPhysicalDevice physicalDevice = RenderContext::Get()->GetDevice()->GetPhysicalDevice();
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, s_Swapchain->GetSurface(), &s_SurfaceCaps);

        std::vector<RenderContext::Renderable> renderables;
        Asset asset;
        renderables.push_back({.descriptorSet = asset.GetDescriptorSet(),
                               .buffer = asset.GetModelBuffer(),
                               .vertexByteSize = asset.GetModelVertexSize(),
                               .indexByteSize = asset.GetModelIndexSize(),
                               .indexCount = asset.GetModelIndexCount()});

        RenderContext::Get()->renderables = &renderables;
        RenderContext::Get()->s_ShaderDataBuffers = s_UniformBuffers ? &s_UniformBuffers->Buffers() : nullptr;
        RenderContext::Get()->s_CommandBuffers = &s_CommandBuffers;
        // Provide pointers to the internal handle vectors so other subsystems can read them (null-safe)
        RenderContext::Get()->s_Fences = s_FenceHandles.empty() ? nullptr : &s_FenceHandles;
        RenderContext::Get()->s_PresentSemaphores = s_PresentSemaphoreHandles.empty() ? nullptr : &s_PresentSemaphoreHandles;
        RenderContext::Get()->s_RenderSemaphores = s_RenderSemaphoreHandles.empty() ? nullptr : &s_RenderSemaphoreHandles;

        s_ResourcesInitialized = true;
        SEDX_CORE_INFO_TAG("Renderer", "=== Renderer Initialization Complete ===");
    }

    void Renderer::Shutdown()
    {
        SEDX_CORE_INFO_TAG("Renderer", "=== Shutting Down Renderer ===");

        Ref<Device> device = RenderContext::Get()->GetDevice();

        // Wait for all GPU work to complete
        device->GetQueueManager()->WaitIdleAll();

        // Destroy basic forward pipeline resources
        {
            VkDevice dev = device->GetLogicalDevice();
            if (s_BasicPipeline != VK_NULL_HANDLE)
            {
                vkDestroyPipeline(dev, s_BasicPipeline, nullptr);
                s_BasicPipeline = VK_NULL_HANDLE;
            }
            if (s_BasicPipelineLayout != VK_NULL_HANDLE)
            {
                vkDestroyPipelineLayout(dev, s_BasicPipelineLayout, nullptr);
                s_BasicPipelineLayout = VK_NULL_HANDLE;
            }
            s_BasicShaderManager.reset();

            VmaAllocator vma = device->GetMemoryAllocator().GetAllocator();
            for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
            {
                if (s_BasicShaderDataAllocations[i] != VK_NULL_HANDLE)
                {
                    vmaDestroyBuffer(vma, s_BasicShaderDataBuffers[i], s_BasicShaderDataAllocations[i]);
                    s_BasicShaderDataBuffers[i]     = VK_NULL_HANDLE;
                    s_BasicShaderDataAllocations[i] = VK_NULL_HANDLE;
                    s_BasicShaderDataMapped[i]      = nullptr;
                    s_BasicShaderDataAddresses[i]   = 0;
                }
            }
        }

        // Destroy frame resources
        DestroyFrameResources();
        s_AssetManager->DestroyAll();
        s_AssetManager.reset();

        // Destroy swapchain
        if (s_Swapchain)
        {
            s_Swapchain.Reset();
        }

        // Cleanup renderer data
        delete s_Data;
        s_Data = nullptr;

        s_ResourcesInitialized = false;
        SEDX_CORE_TRACE_TAG("Renderer", "=== Renderer Shutdown Complete ===");
    }

    void Renderer::Tick()
    {
        Ref<Device> device = RenderContext::Get()->GetDevice();
        SEDX_CORE_ASSERT(device.IsValid(), "Device is not valid in Renderer::Tick");

        Ref<QueueManager> queueManager = device->GetQueueManager();
        SEDX_CORE_ASSERT(queueManager.IsValid(), "QueueManager is not valid in Renderer::Tick");

        device->GetMemoryAllocator().Tick(s_FrameNumber);
        bool can_render = !Window::IsMinimized() && s_ResourcesInitialized;

        // prevent write-after-present hazards when idle (skip first frame, nothing to wait for)
        if (!can_render && s_FrameNumber > 0)
        {
            if (Ref<Queue> *queue = queueManager->GetQueue(QueueType::Graphics); queue && *queue)
            {
                Queue::WaitIdle(*queue->Get());
            }
        }

        m_CmdList_Present = queueManager->NextCommandList();
        SEDX_CORE_ASSERT(m_CmdList_Present != nullptr, "Failed to acquire present command list");
        m_CmdList_Present->Begin();

        m_CmdList_Compute = nullptr;
        if (can_render)
        {
            m_CmdList_Compute = queueManager->NextCommandList();
            SEDX_CORE_ASSERT(m_CmdList_Compute != nullptr, "Failed to acquire compute command list");
            if (m_CmdList_Compute)
            {
                m_CmdList_Compute->Begin();
            }
        }

        m_DrawDataCount = 0;

        if (can_render)
        {
			bool isLoading = false;

            UpdateDrawCalls(m_CmdList_Present);

            // periodic resource cleanup
            {
                m_ResourceIndex++;
                if (bool isSyncPoint = m_ResourceIndex == renderer_resource_frame_lifetime)
                {
                    m_ResourceIndex = 0;

                    if (QueueManager::NeedToParseDeletionQueue())
                    {
                        QueueManager::WaitIdleAll();
                        QueueManager::ParseDeletionQueue();
                    }

                    // TODO: GetBuffer(Renderer_Buffer::ConstantFrame)->ResetOffset(); // ResetOffset not yet implemented on Buffer
                }
            }

			/*
            // bindless resource updates
            if (!isLoading)
            {
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
                {
                    UpdateBoundingBoxes(m_CmdList_Present);

                    static bool aabbs_descriptor_set = false;
                    if (!aabbs_descriptor_set)
                    {
                        RHI_Device::UpdateBindlessAABBs(GetBuffer(Renderer_Buffer::AABBs));
                        aabbs_descriptor_set = true;
                    }
                }

                // draw data
                {
                    if (m_DrawDataCount > 0)
                    {
                        Buffer *buffer = GetBuffer(Renderer_Buffer::DrawData);
                        uint32_t frame_byte_offset = m_frame_resource_index * renderer_max_draw_calls *
                                                     static_cast<uint32_t>(sizeof(Sb_DrawData));
                        uint32_t upload_size = static_cast<uint32_t>(sizeof(Sb_DrawData)) * m_DrawDataCount;
                        m_CmdList_Present->UpdateBuffer(buffer, frame_byte_offset, upload_size, &m_draw_data_cpu[0]);
                    }

                    // the descriptor points to a single large buffer that holds all frames' draw data
                    // at different offsets, so it only needs to be set once; this eliminates the race
                    // where vkUpdateDescriptorSets (host-side, instantly visible under UPDATE_AFTER_BIND)
                    // would change the buffer pointer while the previous frame's phase 3 transparent pass
                    // was still reading from it on the gpu
                    static bool draw_data_descriptor_set = false;
                    if (!draw_data_descriptor_set)
                    {
                        RHI_Device::UpdateBindlessDrawData(GetBuffer(Renderer_Buffer::DrawData));
                        draw_data_descriptor_set = true;
                    }
                }

                // geometry buffers (vertex pulling via bindless structured buffers)
                {
                    static Buffer *last_vertex_buffer = nullptr;
                    Buffer *current_vertex = GeometryBuffer::GetVertexBuffer();
                    if (current_vertex && current_vertex != last_vertex_buffer)
                    {
                        RHI_Device::UpdateBindlessGeometryVertices(current_vertex);
                        last_vertex_buffer = current_vertex;
                    }

                    static Buffer *last_index_buffer = nullptr;
                    Buffer *current_index = GeometryBuffer::GetIndexBuffer();
                    if (current_index && current_index != last_index_buffer)
                    {
                        RHI_Device::UpdateBindlessGeometryIndices(current_index);
                        last_index_buffer = current_index;
                    }
                }

                // dummy instance buffer (vertex pulling identity instances)
                {
                    static bool instances_descriptor_set = false;
                    if (!instances_descriptor_set)
                    {
                        Device::UpdateBindlessInstances(GetBuffer(Renderer_Buffer::DummyInstance));
                        instances_descriptor_set = true;
                    }
                }

                // indirect draw buffers
                if (m_indirect_draw_count > 0)
                {
                    Buffer *args_buffer = GetBuffer(Renderer_Buffer::IndirectDrawArgs);
                    args_buffer->ResetOffset();
                    args_buffer->Update(m_CmdList_Present, &m_indirect_draw_args[0],
                                        args_buffer->GetStride() * m_indirect_draw_count);

                    Buffer *data_buffer = GetBuffer(Renderer_Buffer::IndirectDrawData);
                    data_buffer->ResetOffset();
                    data_buffer->Update(m_CmdList_Present, &m_indirect_draw_data[0],
                                        data_buffer->GetStride() * m_indirect_draw_count);

                    // reset count, the cull shader atomically increments it
                    uint32_t zero = 0;
                    Buffer *count_buffer = GetBuffer(Renderer_Buffer::IndirectDrawCount);
                    count_buffer->ResetOffset();
                    count_buffer->Update(m_CmdList_Present, &zero, sizeof(uint32_t));
                }
            }*/
        }

		/*
		UpdateFrameConstantBuffer(m_CmdList_Present);
		UpdatePersistentLines();
		AddLinesToBeRendered();
		
		if (can_render)
		{
		    BlitToBackBuffer(m_CmdList_Present, GetRenderTarget(Renderer_RenderTarget::frame_output));
		}

        SubmitAndPresent();

		m_lines_vertices.clear();
		m_icons.clear();

		// only count frames that actually rendered
		if (can_render)
		{
		    s_FrameNumber++;
		    if (s_FrameNumber == 1)
		    {
		        Event(EventType::AppTick);
		    }
		}
		*/
    }   

#pragma endregion

#pragma region Frame Rendering Methods

    void Renderer::CreateFrameResources()
    {
        SEDX_CORE_TRACE_TAG("Renderer", "Creating frame resources for {} frames in flight", MAX_FRAMES_IN_FLIGHT);

        // Get queue family index from queue manager
        uint32_t queueFamily = RenderContext::Get()->GetDevice()->GetQueueManager()->GetFamilyIndexByType(Graphics);

        // Create command pool
        s_CommandPool = CreateScope<CommandPool>(queueFamily, CommandPoolType::Resettable);
        SEDX_CORE_TRACE_TAG("Renderer", "Created command pool");

        // Allocate command buffers
        auto allocatedBuffers = s_CommandPool->Allocate(MAX_FRAMES_IN_FLIGHT);
        for (size_t i = 0; i < allocatedBuffers.size() && i < s_CommandBuffers.size(); ++i)
        {
            s_CommandBuffers[i] = allocatedBuffers[i];
        }
        SEDX_CORE_TRACE_TAG("Renderer", "Allocated {} command buffers", MAX_FRAMES_IN_FLIGHT);

        // Create per-frame fences and semaphores and keep wrapper refs alive.
        s_FenceRefs.clear();
        s_FenceHandles.clear();
        s_PresentSemaphoreRefs.clear();
        s_PresentSemaphoreHandles.clear();
        s_RenderSemaphoreRefs.clear();
        s_RenderSemaphoreHandles.clear();

        // Create fences (one per frame in flight)
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

        // Create present semaphores (one per frame in flight)
        s_PresentSemaphoreRefs.reserve(MAX_FRAMES_IN_FLIGHT);
        s_PresentSemaphoreHandles.reserve(MAX_FRAMES_IN_FLIGHT);
        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            Ref<Semaphore> sem = CreateRef<Semaphore>();
            sem->CreateSyncObject();
            s_PresentSemaphoreRefs.push_back(sem);
            s_PresentSemaphoreHandles.push_back(sem->GetSemaphore());
            Debugging::SetResourceName(sem.Get()->GetSemaphore(), ResourceType::Semaphore, "PresentSemaphore");
        }

        const uint32_t swapchainImageCount = static_cast<uint32_t>(s_Swapchain->GetImages().size());

        // Create render semaphores (one per swapchain image)
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

        s_FrameSync = CreateScope<FrameSync>(SyncType::Fence); // keep a simple FrameSync in case other systems expect it
        SEDX_CORE_TRACE_TAG("Renderer", "Created frame sync objects (fences: {}, present semaphores: {}, render semaphores: {})",
            static_cast<uint32_t>(s_FenceHandles.size()),
            static_cast<uint32_t>(s_PresentSemaphoreHandles.size()),
            static_cast<uint32_t>(s_RenderSemaphoreHandles.size()));

        s_FrameSync->SetUserCmdList(nullptr);
        SEDX_CORE_TRACE_TAG("Renderer", " Frame resources created");
    }

    void Renderer::DestroyFrameResources()
    {
        SEDX_CORE_TRACE_TAG("Renderer", "Destroying frame resources");

        // Destroy semaphores and fences through wrapper Destroy() so they get scheduled for deletion properly.
        for (auto &semRef : s_PresentSemaphoreRefs)
        {
            if (semRef)
            {
                semRef->Destroy();
                semRef.Reset();
            }
        }
        s_PresentSemaphoreHandles.clear();
        s_PresentSemaphoreRefs.clear();
        SEDX_CORE_TRACE_TAG("Renderer", " Destroyed present semaphores");

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
        SEDX_CORE_TRACE_TAG("Renderer", " Destroyed render semaphores");

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
        SEDX_CORE_TRACE_TAG("Renderer", " Destroyed fences");

        // Command buffers are freed when command pool is destroyed
        s_CommandBuffers.fill(VK_NULL_HANDLE);
        SEDX_CORE_TRACE_TAG("Renderer", " Freed command buffers ({} buffers)", MAX_FRAMES_IN_FLIGHT);

        // Destroy command pool
        if (s_CommandPool)
        {
            s_CommandPool->Destroy();
            s_CommandPool.reset();
            SEDX_CORE_TRACE_TAG("Renderer", " Destroyed command pool");
        }

        // Reset simple FrameSync wrapper
        s_FrameSync.reset();
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

    bool Renderer::BeginFrame()
    {
        SEDX_CORE_TRACE_TAG("Renderer", "Beginning frame {}", s_FrameNumber);

        // Check if we can render
        if (!s_ResourcesInitialized)
        {
            SEDX_CORE_WARN_TAG("Renderer", "BeginFrame called but renderer not initialized");
            return false;
        }

        // Skip if window is minimized
        const uint32_t minRenderDimension = 64;
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
                s_Swapchain->Recreate();
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
            SEDX_CORE_TRACE_TAG("Renderer", "Waiting for fence of frame {} (fence handle: {})",
                                s_CurrentFrameIndex, static_cast<void *>(s_FenceHandles[s_CurrentFrameIndex]));
            if (s_CurrentFrameIndex < s_FenceHandles.size() && s_FenceHandles[s_CurrentFrameIndex] != VK_NULL_HANDLE)
            {
                VkDevice device = RenderContext::Get()->GetDevice()->GetLogicalDevice();
                vkWaitForFences(device, 1, &s_FenceHandles[s_CurrentFrameIndex], VK_TRUE, UINT64_MAX);
                vkResetFences(device, 1, &s_FenceHandles[s_CurrentFrameIndex]);
                SEDX_CORE_TRACE_TAG("Renderer", "Fence wait and reset complete for frame {}", s_CurrentFrameIndex);
            }
        }

        if (s_CurrentFrameIndex >= s_PresentSemaphoreHandles.size())
        {
            SEDX_CORE_ERROR_TAG("Renderer", "Current frame index {} out of bounds for present semaphores (size {})", s_CurrentFrameIndex, s_PresentSemaphoreHandles.size());
            return false;
        }

        // Acquire image and signal the exact semaphore that submit waits on for this frame.
        {
            VkResult acquireResult = vkAcquireNextImageKHR(
                RenderContext::Get()->GetDevice()->GetLogicalDevice(),
                s_Swapchain->Get(),
                UINT64_MAX,
                s_PresentSemaphoreHandles[s_CurrentFrameIndex],
                VK_NULL_HANDLE,
                &s_SwapchainImageIndex);

            if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR || acquireResult == VK_SUBOPTIMAL_KHR)
            {
                s_Swapchain->Recreate();
                return false;
            }

            if (acquireResult != VK_SUCCESS)
            {
                SEDX_CORE_WARN_TAG("Renderer", "vkAcquireNextImageKHR failed with result: {}", static_cast<int>(acquireResult));
                return false;
            }

            if (s_SwapchainImageIndex >= s_Swapchain->GetImages().size())
            {
                SEDX_CORE_WARN_TAG("Renderer", "AcquireNextImage returned invalid index: {}", s_SwapchainImageIndex);
                return false;
            }

            SEDX_CORE_TRACE_TAG("Renderer", "Acquired swapchain image index: {}", s_SwapchainImageIndex);
        }

        // Begin command buffer recording
        if (VkCommandBuffer cb = s_CommandBuffers[s_CurrentFrameIndex]; cb != VK_NULL_HANDLE)
        {
            vkResetCommandBuffer(cb, 0);
			SEDX_CORE_TRACE_TAG("Renderer", "Command buffer reset for frame {}", s_CurrentFrameIndex);

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            VkResult result = vkBeginCommandBuffer(cb, &beginInfo);
			SEDX_VK_RESULT_ASSERT(result, "vkBeginCommandBuffer failed");
        }

		SEDX_CORE_TRACE_TAG("Renderer", "Command buffer recording begun for frame {}", s_CurrentFrameIndex);
        s_FrameInProgress = true;
        return true;
    }

    void Renderer::EndFrame()
    {
        SEDX_CORE_TRACE_TAG("Renderer", "Ending frame {}", s_FrameNumber);

        if (!s_FrameInProgress)
        {
            SEDX_CORE_WARN_TAG("Renderer", "EndFrame called but no frame in progress");
            return;
        }

        VkCommandBuffer cb = s_CommandBuffers[s_CurrentFrameIndex];
        if (cb != VK_NULL_HANDLE)
        {
            // Record the actual render commands for this frame
            RecordRenderCommands(cb, s_SwapchainImageIndex);

            // Transition swapchain image to present layout
            if (s_Swapchain)
            {
                auto &swapchainImages = s_Swapchain->GetImages();
                if (s_SwapchainImageIndex < swapchainImages.size())
                {
                    VkImageMemoryBarrier2 barrierPresent{
                        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        .dstAccessMask = 0,
                        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                        .image = swapchainImages[s_SwapchainImageIndex],
                        .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1}};

                    VkDependencyInfo dependencyInfo{};
                    dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
                    dependencyInfo.imageMemoryBarrierCount = 1;
                    dependencyInfo.pImageMemoryBarriers = &barrierPresent;

                    vkCmdPipelineBarrier2(cb, &dependencyInfo);
                }
            }

            // End command buffer recording
            VkResult result = vkEndCommandBuffer(cb);
			SEDX_VK_RESULT_ASSERT(result, "vkEndCommandBuffer failed");
        }

        s_FrameInProgress = false;
    }

    void Renderer::SubmitAndPresent()
    {
        SEDX_CORE_TRACE_TAG("Renderer", "Submitting command buffer and presenting frame {}", s_FrameNumber);

        VkCommandBuffer cb = s_CommandBuffers[s_CurrentFrameIndex];
        if (cb == VK_NULL_HANDLE)
        {
			SEDX_CORE_TRACE_TAG("Renderer", "No command buffer available for frame {}", s_FrameNumber);
            return;
        }

        if (s_PresentSemaphoreHandles.empty() || s_RenderSemaphoreHandles.empty() || s_FenceHandles.empty())
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

        // Ensure indices are valid
        if (s_CurrentFrameIndex >= s_PresentSemaphoreHandles.size())
        {
            SEDX_CORE_ERROR_TAG("Renderer", "Current frame index {} out of bounds for present semaphores (size {})", s_CurrentFrameIndex, s_PresentSemaphoreHandles.size());
            return;
        }
        if (s_SwapchainImageIndex >= s_RenderSemaphoreHandles.size())
        {
            SEDX_CORE_ERROR_TAG("Renderer", "Swapchain image index {} out of bounds for render semaphores (size {})", s_SwapchainImageIndex, s_RenderSemaphoreHandles.size());
            return;
        }
		SEDX_CORE_TRACE_TAG("Renderer", "Submitting command buffer for frame {}, waiting on present semaphore {}, signaling render semaphore {}",
            s_FrameNumber,
            static_cast<void *>(s_PresentSemaphoreHandles[s_CurrentFrameIndex]),
            static_cast<void *>(s_RenderSemaphoreHandles[s_SwapchainImageIndex]));

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &s_PresentSemaphoreHandles[s_CurrentFrameIndex];
        submitInfo.pWaitDstStageMask = &waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cb;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &s_RenderSemaphoreHandles[s_SwapchainImageIndex];

        VkResult submitResult = vkQueueSubmit(graphicsQueue, 1, &submitInfo, s_FenceHandles[s_CurrentFrameIndex]);
		SEDX_VK_RESULT_ASSERT(submitResult, "vkQueueSubmit failed");

        // Present the rendered image
        VkSwapchainKHR swapchainHandle = s_Swapchain->Get();
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &s_RenderSemaphoreHandles[s_SwapchainImageIndex];
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchainHandle;
        presentInfo.pImageIndices = &s_SwapchainImageIndex;
		SEDX_CORE_TRACE_TAG("Renderer", "Presenting swapchain image index {} for frame {}", s_SwapchainImageIndex, s_FrameNumber);

        VkResult presentResult = vkQueuePresentKHR(graphicsQueue, &presentInfo);
        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
        {
            // Swapchain needs recreation (e.g., window resize)
            s_Swapchain->Recreate();
            SEDX_CORE_TRACE_TAG("Renderer", "Swapchain recreated after present (result: {})", static_cast<int>(presentResult));
        }
        else if (presentResult != VK_SUCCESS)
        {
            SEDX_CORE_ERROR_TAG("Renderer", "vkQueuePresentKHR failed: {}", static_cast<int>(presentResult));
        }

        // Advance to next frame-in-flight
        s_CurrentFrameIndex = (s_CurrentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
        s_FrameNumber++;

		SEDX_CORE_TRACE_TAG("Renderer", "Frame {} submitted and presented, advancing to frame index {}", s_FrameNumber, s_CurrentFrameIndex);
    }

    void Renderer::DrawFrame(CommandList *cmdList, CommandList *computeCmdList)
    {
        /* 
         * TODO: This method is currently not used, but will be the main entry point for recording 
         * draw calls once the renderer is fully modularized and other systems are integrated to call it.
         */
        SEDX_CORE_TRACE_TAG("Renderer", "DrawFrame called for frame {}", s_FrameNumber);
        // This method will be called by modules to record their draw commands
        // For now, placeholder implementation
        
        VkCommandBuffer cb = s_CommandBuffers[s_CurrentFrameIndex];
        if (cb == VK_NULL_HANDLE || !s_Swapchain)
        {
            return;
        }
        SEDX_CORE_TRACE_TAG("Renderer", "Recording draw commands using command buffer for frame {}", s_FrameNumber);

        // Record render commands
        RecordRenderCommands(cb, s_SwapchainImageIndex);
        SEDX_CORE_TRACE_TAG("Renderer", "Draw commands recorded for frame {}", s_FrameNumber);
    }

    void Renderer::BlitToBackBuffer(CommandList* cmdList, ImageResource* texture)
    {
        // TODO: Implement blit-to-swapchain once Swapchain exposes an ImageResource interface
        (void)cmdList;
        (void)texture;
    }

#pragma endregion

#pragma region Render Context Management

    Ref<RenderContext> Renderer::GetRenderContext()
    {
        return RenderContext::Get();
    }

    uint32_t Renderer::GetCurrentFrameIndex()
    {
        return s_CurrentFrameIndex;
    }

    uint64_t Renderer::GetFrameNumber()
    {
        return s_FrameNumber;
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

        Timer workTimer;

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
        return s_SwapchainImageIndex;
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
            s_OrthoProjection_Dirty = true;
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
        if (constexpr float epsilon = 1e-5f; std::abs(s_RendererResolution.x - static_cast<float>(width)) < epsilon &&
                                             std::abs(s_RendererResolution.y - static_cast<float>(height)) < epsilon)
            return;

        s_RendererResolution.x = static_cast<float>(width);
        s_RendererResolution.y = static_cast<float>(height);

        if (recreateResources && s_ResourcesInitialized)
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

        if (recreateResources && s_ResourcesInitialized)
        {
            CreateRenderTargets(false, true, false);
        }

        SEDX_CORE_TRACE_TAG("Renderer", "Output resolution set to {}x{}", width, height);
    }

#pragma endregion

#pragma region Command Buffer Access 

    VkCommandBuffer Renderer::GetCurrentCommandBuffer()
    {
        return s_CommandBuffers[s_CurrentFrameIndex];
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
        SEDX_CORE_ASSERT(queuePtr && *queuePtr, "Graphics queue not available");
        // DIAGNOSTIC: Log current working directory
        std::filesystem::path cwd = std::filesystem::current_path();
        SEDX_CORE_ERROR_TAG("Renderer", "Current working directory: {}", cwd.string());

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

        // Dereference the pointer to access the Ref, then call GetQueue()
        const bool assetAdded = s_AssetManager->AddAsset(allocator, s_CommandPool->GetPool(), (*queuePtr)->GetQueue(),
                                                         modelPath.string(), texFiles, bufferAllocCI);
        if (!assetAdded || s_AssetManager->Count() == 0)
        {
            SEDX_CORE_ERROR_TAG("Renderer", "Failed to load model asset: {}", modelPath.string());
            return;
        }

        const Asset &asset = s_AssetManager->GetAsset(0);
        VkBuffer vBuffer = asset.GetModelBuffer();
        VkDeviceSize vBufSize = asset.GetModelVertexSize();
        VkDeviceSize iBufSize = asset.GetModelIndexSize();
        VkDeviceSize indexCount = asset.GetModelIndexCount();
        SEDX_CORE_TRACE_TAG("Renderer", "Loaded model asset: vertex buffer {}, vertex size {}, index size {}, index count {}",
                            static_cast<void *>(vBuffer), vBufSize, iBufSize, indexCount);

        // Create uniform buffers as static resource (per-frame) managed by UniformBufferSet RAII helper
        s_UniformBuffers = CreateScope<UniformBufferSet>(allocator);
        if (s_UniformBuffers)
        {
            s_UniformBuffers->Create();
        }
    }

    void Renderer::CreateShaders()
    {
        SEDX_CORE_TRACE_TAG("Renderer", "Creating shaders and initializing Slang shader compiler");
        // Initialize Slang shader compiler
        Slang::ComPtr<slang::IGlobalSession> slangGlobalSession;
        slang::createGlobalSession(slangGlobalSession.writeRef());
        auto slangTargets{std::to_array<slang::TargetDesc>(
            {{.format = SLANG_SPIRV, .profile = slangGlobalSession->findProfile("spirv_1_4")}})};
        auto slangOptions{std::to_array<slang::CompilerOptionEntry>(
            {{.name = slang::CompilerOptionName::EmitSpirvDirectly,
              .value = {.kind = slang::CompilerOptionValueKind::Int, .intValue0 = 1}}})};

        slang::SessionDesc slangSessionDesc = {};
        slangSessionDesc.targets = slangTargets.data();
        slangSessionDesc.targetCount = static_cast<SlangInt>(slangTargets.size());
        slangSessionDesc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;
        slangSessionDesc.compilerOptionEntries = slangOptions.data();
        slangSessionDesc.compilerOptionEntryCount = static_cast<uint32_t>(slangOptions.size());

        // Load shader
        Slang::ComPtr<slang::ISession> slangSession;
        Slang::ComPtr<slang::IBlob> diagnosticsBlob; // Blob to capture any diagnostics from shader compilation
        slangGlobalSession->createSession(slangSessionDesc, slangSession.writeRef());
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
        Slang::ComPtr<ISlangBlob> spirv;
        slangModule->getTargetCode(0, spirv.writeRef());

        // Persist shader modules as a static member so they outlive this function.
        s_BasicShaderManager = CreateScope<ShaderManager>(spirv->getBufferPointer(), spirv->getBufferSize());
        if (!s_BasicShaderManager || !s_BasicShaderManager->IsCompiled())
        {
            SEDX_CORE_ERROR_TAG("Renderer", "Shader compilation produced no modules");
            return;
        }

        if (!s_AssetManager || s_AssetManager->Count() == 0)
        {
            SEDX_CORE_ERROR_TAG("Renderer", "No assets loaded; pipeline creation deferred");
            return;
        }

        const Asset& asset = s_AssetManager->GetAsset(0);
        VkDevice dev = RenderContext::Get()->GetDevice()->GetLogicalDevice();

        // The Slang shader uses `uniform ShaderData *shaderData` (pointer), which Slang
        // compiles to a push-constant block holding an 8-byte buffer device address.
        VkPushConstantRange pushConst{};
        pushConst.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConst.offset     = 0;
        pushConst.size       = sizeof(VkDeviceAddress);

        // Set 0 = texture sampler array (populated by Asset)
        VkDescriptorSetLayout textureLayout = asset.GetDescriptorLayout();
        VkPipelineLayoutCreateInfo layoutCI{};
        layoutCI.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutCI.setLayoutCount         = 1;
        layoutCI.pSetLayouts            = &textureLayout;
        layoutCI.pushConstantRangeCount = 1;
        layoutCI.pPushConstantRanges    = &pushConst;

        SEDX_VK_RESULT_ASSERT(vkCreatePipelineLayout(dev, &layoutCI, nullptr, &s_BasicPipelineLayout),
                              "Failed to create basic pipeline layout");

        Pipeline::GraphicsCreateInfo pipeCI{};
        pipeCI.device           = dev;
        pipeCI.layout           = s_BasicPipelineLayout;
        pipeCI.shaderManager    = s_BasicShaderManager.get();
        pipeCI.vertexBinding    = Asset::GetVertexBindingDescription();
        pipeCI.vertexAttributes = Asset::GetVertexAttributeDescriptions();
        pipeCI.colorFormat      = s_Swapchain->GetImageFormat();
        pipeCI.depthFormat      = s_Swapchain->GetDepthFormat();

        s_BasicPipeline = Pipeline::CreateGraphics(pipeCI);
        if (s_BasicPipeline == VK_NULL_HANDLE)
        {
            SEDX_CORE_ERROR_TAG("Renderer", "Pipeline::CreateGraphics failed");
            return;
        }
        SEDX_CORE_INFO_TAG("Renderer", "Basic graphics pipeline created successfully");

        // Per-frame shader-data buffers (device-addressable, host-visible, mapped).
        // Layout must match the Slang shader's ShaderData struct exactly.
        struct BasicShaderData
        {
            Mat4 projection;
            Mat4 view;
            Mat4 model[3];
            Vec4 lightPos;
            uint32_t  selected;
            uint32_t  _pad[3];
        };

        VmaAllocator vma = RenderContext::Get()->GetDevice()->GetMemoryAllocator().GetAllocator();
        const float aspect = static_cast<float>(s_Swapchain->GetExtent().width) /
                             static_cast<float>(s_Swapchain->GetExtent().height);

        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            VkBufferCreateInfo uboCI{};
            uboCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            uboCI.size  = sizeof(BasicShaderData);
            uboCI.usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

            VmaAllocationCreateInfo allocCI{};
            allocCI.usage = VMA_MEMORY_USAGE_AUTO;
            allocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                            VMA_ALLOCATION_CREATE_MAPPED_BIT;

            VmaAllocationInfo allocInfo{};
            if (vmaCreateBuffer(vma, &uboCI, &allocCI,
                                &s_BasicShaderDataBuffers[i],
                                &s_BasicShaderDataAllocations[i],
                                &allocInfo) != VK_SUCCESS)
            {
                SEDX_CORE_ERROR_TAG("Renderer", "Failed to create shader-data buffer {}", i);
                continue;
            }
            s_BasicShaderDataMapped[i] = allocInfo.pMappedData;

            VkBufferDeviceAddressInfo bdaInfo{};
            bdaInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
            bdaInfo.buffer = s_BasicShaderDataBuffers[i];
            s_BasicShaderDataAddresses[i] = vkGetBufferDeviceAddress(dev, &bdaInfo);

            // Initial transforms: three Suzanne instances spread on the X axis.
            // Matrices built manually to avoid depending on GLM's compiled helper library.
            BasicShaderData sd{};

            // Perspective matrix (column-major, right-handed, Vulkan ZO, Y flipped)
            {
                constexpr float pi      = 3.14159265358979323846f;
                const float     tanHalf = std::tan((pi / 4.0f) * 0.5f); // tan(45°/2)
                const float     invTan  = 1.0f / tanHalf;
                const float     zNear   = 0.1f;
                const float     zFar    = 100.0f;
                Mat4& p            = sd.projection;
                p                       = Mat4(0.0f);
                p[0][0]                 =  invTan / aspect;
                p[1][1]                 = -invTan;          // Vulkan: Y points down
                p[2][2]                 =  zFar / (zNear - zFar);
                p[2][3]                 = -1.0f;
                p[3][2]                 = -(zFar * zNear) / (zFar - zNear);
            }

            // LookAt matrix (eye at (0,0,-5), centre (0,0,0), up (0,1,0))
            {
                const Vec3 eye    = { 0.0f,  0.0f, -5.0f };
                const Vec3 center = { 0.0f,  0.0f,  0.0f };
                const Vec3 up     = { 0.0f,  1.0f,  0.0f };
                const Vec3 fwd    = xMath::Normalize(center - eye);
                const Vec3 right  = xMath::Normalize(xMath::Cross(fwd, up));
                const Vec3 newUp  = xMath::Cross(right, fwd);
                Mat4& v           = sd.view;
                v                      = Mat4(1.0f);
                v[0][0] =  right.x;  v[1][0] =  right.y;  v[2][0] =  right.z;
                v[0][1] =  newUp.x;  v[1][1] =  newUp.y;  v[2][1] =  newUp.z;
                v[0][2] = -fwd.x;    v[1][2] = -fwd.y;    v[2][2] = -fwd.z;
                v[3][0] = -xMath::Dot(right, eye);
                v[3][1] = -xMath::Dot(newUp, eye);
                v[3][2] =  xMath::Dot(fwd,   eye);
            }

            // Identity for model[0], translations for model[1] and model[2]
            sd.model[0]      = Mat4(1.0f);
            sd.model[1]      = Mat4(1.0f); sd.model[1][3] = Vec4(-3.0f, 0.0f, 0.0f, 1.0f);
            sd.model[2]      = Mat4(1.0f); sd.model[2][3] = Vec4( 3.0f, 0.0f, 0.0f, 1.0f);
            sd.lightPos      = Vec4(0.0f, 5.0f, 5.0f, 1.0f);
            sd.selected      = 0;

            if (s_BasicShaderDataMapped[i])
            {
                std::memcpy(s_BasicShaderDataMapped[i], &sd, sizeof(sd));
            }
        }
    }

#pragma endregion
	
    GPUMemoryStats Renderer::GetGPUMemoryStats()
    {
        return MemoryAllocator::GetMemoryStats();
    }

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
                .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1}},
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
                                  .layerCount = 1}}};
        SEDX_CORE_TRACE_TAG("Renderer",
                            "Transitioning swapchain image {} and depth image to attachment optimal layout",
                            imageIndex);

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
                                                      .clearValue{.color{{0.0f, 0.0f, 0.0f, 1.0f}}}};
        SEDX_CORE_TRACE_TAG("Renderer", "Configured color attachment for dynamic rendering");

        VkRenderingAttachmentInfo depthAttachmentInfo{.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                                                      .imageView = depthImageView,
                                                      .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
                                                      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                      .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                                      .clearValue = {.depthStencil = {1.0f, 0}}};
        SEDX_CORE_TRACE_TAG("Renderer", "Configured depth attachment for dynamic rendering");

        VkExtent2D extent = s_Swapchain->GetExtent();
        VkRenderingInfo renderingInfo{.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
                                      .renderArea{.extent{.width = extent.width, .height = extent.height}},
                                      .layerCount = 1,
                                      .colorAttachmentCount = 1,
                                      .pColorAttachments = &colorAttachmentInfo,
                                      .pDepthAttachment = &depthAttachmentInfo};

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
        // Per-frame shader-data update: rewrite model matrices with a Y-axis rotation
        // driven by the frame counter. This fixes the static view bug by refreshing
        // the buffer each frame and produces visible animation to confirm the update path works.
        if (s_BasicShaderDataMapped[s_CurrentFrameIndex])
        {
            struct BasicShaderData
            {
                Mat4     projection;
                Mat4     view;
                Mat4     model[3];
                Vec4     lightPos;
                uint32_t selected;
                uint32_t _pad[3];
            };

            auto* pSd = static_cast<BasicShaderData*>(s_BasicShaderDataMapped[s_CurrentFrameIndex]);

            // Y-axis rotation (column-major [col][row]):
            //   | cosA   0   sinA  0 |
            //   |    0   1      0  0 |
            //   | -sinA  0   cosA  0 |
            //   |    0   0      0  1 |
            const float angle = static_cast<float>(s_FrameNumber) * 0.005f;
            const float cosA  = std::cos(angle);
            const float sinA  = std::sin(angle);
            Mat4 rot(1.0f);
            rot[0][0] =  cosA;  rot[2][0] = sinA;
            rot[0][2] = -sinA;  rot[2][2] = cosA;

            pSd->model[0] = rot;

            Mat4 t1(1.0f); t1[3] = Vec4(-3.0f, 0.0f, 0.0f, 1.0f);
            pSd->model[1] = t1 * rot;

            Mat4 t2(1.0f); t2[3] = Vec4( 3.0f, 0.0f, 0.0f, 1.0f);
            pSd->model[2] = t2 * rot;
        }

        if (s_BasicPipeline != VK_NULL_HANDLE && s_AssetManager   && s_AssetManager->Count() > 0 &&
            s_BasicShaderDataAddresses[s_CurrentFrameIndex] != 0)
        {
            const Asset& asset = s_AssetManager->GetAsset(0);

            vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, s_BasicPipeline);

            VkDescriptorSet descSet = asset.GetDescriptorSet();
            vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    s_BasicPipelineLayout,
                                    0, 1, &descSet,
                                    0, nullptr);

            // Push the device address of this frame's shader-data buffer.
            VkDeviceAddress addr = s_BasicShaderDataAddresses[s_CurrentFrameIndex];
            vkCmdPushConstants(cb, s_BasicPipelineLayout,
                               VK_SHADER_STAGE_VERTEX_BIT,
                               0, sizeof(VkDeviceAddress), &addr);

            // The Model packs vertices then indices into a single VkBuffer.
            VkBuffer     vertBuf    = asset.GetModelBuffer();
            VkDeviceSize vertOffset = 0;
            vkCmdBindVertexBuffers(cb, 0, 1, &vertBuf, &vertOffset);
            vkCmdBindIndexBuffer(cb, vertBuf, asset.GetModelVertexSize(), VK_INDEX_TYPE_UINT16);

            // Draw 3 instances (one per model[0..2] in the shader's ShaderData).
            vkCmdDrawIndexed(cb, asset.GetModelIndexCount(), 3, 0, 0, 0);
        }

        vkCmdEndRendering(cb);
        SEDX_CORE_TRACE_TAG("Renderer", "Dynamic rendering ended");

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

    void Renderer::UpdateDrawCalls(CommandList* cmdList)
    {
        // TODO: Implement draw call collection and sorting when the scene and material systems are integrated
        (void)cmdList;
    }

    /*
    void Renderer::UpdateFrameConstantBuffer(CommandList* cmdList)
    {
        // matrices
        {
            if (Camera* camera = Scene::GetCamera())
            {
                if (near_plane != camera->GetNearPlane() || far_plane != camera->GetFarPlane())
                {
                    near_plane                    = camera->GetNearPlane();
                    far_plane                     = camera->GetFarPlane();
                    dirty_orthographic_projection = true;
                }

                m_cb_frame_cpu.view_previous       = m_cb_frame_cpu.view;
                m_cb_frame_cpu.view                = camera->GetViewMatrix();
                m_cb_frame_cpu.view_inv            = Matrix::Invert(m_cb_frame_cpu.view);
                m_cb_frame_cpu.projection_previous = m_cb_frame_cpu.projection;
                m_cb_frame_cpu.projection          = camera->GetProjectionMatrix();
                m_cb_frame_cpu.projection_inv      = Matrix::Invert(m_cb_frame_cpu.projection);
            }

            if (dirty_orthographic_projection)
            { 
                // near = 0 for ortho (avoids NaN in [3,2] element)
                Matrix projection_ortho              = Matrix::CreateOrthographicLH(m_viewport.width, m_viewport.height, 0.0f, far_plane);
                m_cb_frame_cpu.view_projection_ortho = Matrix::CreateLookAtLH(Vector3(0, 0, -near_plane), Vector3::Forward, Vector3::Up) * projection_ortho;
                dirty_orthographic_projection        = false;
            }
        }

        // taa jitter
        Renderer_AntiAliasing_Upsampling upsampling_mode = cvar_antialiasing_upsampling.GetValueAs<Renderer_AntiAliasing_Upsampling>();
        {
            if (upsampling_mode == Renderer_AntiAliasing_Upsampling::AA_Fsr_Upscale_Fsr)
            {
                RHI_VendorTechnology::FSR3_GenerateJitterSample(&jitter_offset.x, &jitter_offset.y);
                m_cb_frame_cpu.projection *= Matrix::CreateTranslation(Vector3(jitter_offset.x, jitter_offset.y, 0.0f));
            }
            else if (upsampling_mode == Renderer_AntiAliasing_Upsampling::AA_Xess_Upscale_Xess)
            {
                RHI_VendorTechnology::XeSS_GenerateJitterSample(&jitter_offset.x, &jitter_offset.y);
                m_cb_frame_cpu.projection *= Matrix::CreateTranslation(Vector3(jitter_offset.x, jitter_offset.y, 0.0f));
            }
            else
            {
                jitter_offset = Vector2::Zero;
            }
        }

        m_cb_frame_cpu.view_projection_previous = m_cb_frame_cpu.view_projection;
        m_cb_frame_cpu.view_projection          = m_cb_frame_cpu.view * m_cb_frame_cpu.projection;
        m_cb_frame_cpu.view_projection_inv      = Matrix::Invert(m_cb_frame_cpu.view_projection);
        if (Camera* camera = Scene::GetCamera())
        {
            m_cb_frame_cpu.view_projection_previous_unjittered = m_cb_frame_cpu.view_projection_unjittered;
            m_cb_frame_cpu.view_projection_unjittered          = m_cb_frame_cpu.view * camera->GetProjectionMatrix();
            m_cb_frame_cpu.camera_near                         = camera->GetNearPlane();
            m_cb_frame_cpu.camera_far                          = camera->GetFarPlane();
            m_cb_frame_cpu.camera_position_previous            = m_cb_frame_cpu.camera_position;
            m_cb_frame_cpu.camera_position                     = camera->GetEntity()->GetPosition();
            m_cb_frame_cpu.camera_forward                      = camera->GetEntity()->GetForward();
            m_cb_frame_cpu.camera_right                        = camera->GetEntity()->GetRight();
            m_cb_frame_cpu.camera_fov                          = camera->GetFovHorizontalRad();
            m_cb_frame_cpu.camera_aperture                     = camera->GetAperture();
            m_cb_frame_cpu.camera_last_movement_time           = (m_cb_frame_cpu.camera_position - m_cb_frame_cpu.camera_position_previous).LengthSquared() != 0.0f
                ? static_cast<float>(Timer::GetTimeSec()) : m_cb_frame_cpu.camera_last_movement_time;
        }
        m_cb_frame_cpu.resolution_output   = m_resolution_output;
        m_cb_frame_cpu.resolution_render   = m_resolution_render;
        m_cb_frame_cpu.taa_jitter_previous = m_cb_frame_cpu.taa_jitter_current;
        m_cb_frame_cpu.taa_jitter_current  = jitter_offset;
        m_cb_frame_cpu.time                = Timer::GetTimeSec();
        m_cb_frame_cpu.delta_time          = static_cast<float>(Timer::GetDeltaTimeSec());
        m_cb_frame_cpu.frame               = static_cast<uint32_t>(frame_num);
        m_cb_frame_cpu.resolution_scale    = cvar_resolution_scale.GetValue();
        m_cb_frame_cpu.hdr_enabled         = cvar_hdr.GetValueAs<bool>() ? 1.0f : 0.0f;
        m_cb_frame_cpu.hdr_max_nits        = Display::GetLuminanceMax();
        m_cb_frame_cpu.gamma               = cvar_gamma.GetValue();
        m_cb_frame_cpu.camera_exposure     = World::GetCamera() ? World::GetCamera()->GetExposure() : 1.0f;

        m_cb_frame_cpu.cloud_coverage = cvar_cloud_coverage.GetValue();
        m_cb_frame_cpu.cloud_shadows  = cvar_cloud_shadows.GetValue();
        // feature bits (must match common_resources.hlsl)
        m_cb_frame_cpu.set_bit(cvar_ray_traced_reflections.GetValueAs<bool>(), 1 << 0);
        m_cb_frame_cpu.set_bit(cvar_ssao.GetValueAs<bool>(),                   1 << 1);
        m_cb_frame_cpu.set_bit(cvar_ray_traced_shadows.GetValueAs<bool>(),     1 << 2);
        m_cb_frame_cpu.set_bit(cvar_restir_pt.GetValueAs<bool>(),              1 << 3);

        GetBuffer(Renderer_Buffer::ConstantFrame)->Update(cmdList, &m_cb_frame_cpu);
    }
    */

    /*
    uint32_t Renderer::WriteDrawData(const xMath::Matrix& transform, const xMath::Matrix& transform_previous, uint32_t material_index, uint32_t is_transparent)
    {
        SEDX_CORE_ASSERT(m_DrawDataCount < renderer_max_draw_calls);
        uint32_t index = m_DrawDataCount++;

        Sb_DrawData& entry       = m_draw_data_cpu[index];
        entry.transform          = transform;
        entry.transform_previous = transform_previous;
        entry.material_index     = material_index;
        entry.is_transparent     = is_transparent;
        entry.aabb_index         = 0;
        entry.padding            = 0;

        // the draw data buffer is a single large allocation partitioned into per-frame regions;
        // each frame writes to its own region so there is no write-after-read race with the gpu
        uint32_t global_index = m_frame_resource_index * renderer_max_draw_calls + index;

        Buffer* buffer = GetBuffer(Renderer_Buffer::DrawData);
        if (void* mapped = buffer->GetMappedData())
        {
            void* dst = static_cast<char*>(mapped) + global_index * sizeof(Sb_DrawData);
            memcpy(dst, &entry, sizeof(Sb_DrawData));
        }

        return global_index;
    }
    */

    /*
    void Renderer::UpdateDrawCalls(CommandList* cmdList)
    {
        m_draw_call_count          = 0;
        m_draw_calls_prepass_count = 0;
        m_DrawDataCount          = 0;
        m_transparents_present     = false;
        /*if (ProgressTracker::IsLoading())
            return;#1#

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
                        m_transparents_present = true;
                    }

                    uint32_t draw_data_index = WriteDrawData(
                        entity->GetMatrix(),
                        entity->GetMatrixPrevious(),
                        material->GetIndex(),
                        material->IsTransparent() ? 1 : 0
                    );

                    Renderer_DrawCall& draw_call = m_draw_calls[m_draw_call_count++];
                    draw_call.renderable         = renderable;
                    draw_call.distance_squared   = renderable->GetDistanceSquared();
                    draw_call.lod_index          = renderable->GetLodIndex();
                    draw_call.is_occluder        = false;
                    draw_call.camera_visible     = renderable->IsVisible();
                    draw_call.instance_index     = 0;
                    draw_call.instance_count     = renderable->GetInstanceCount();
                    draw_call.draw_data_index    = draw_data_index;
                }
            }

            // sort: opaque before transparent, then material, then distance
            sort(m_draw_calls.begin(), m_draw_calls.begin() + m_draw_call_count, [](const Renderer_DrawCall& a, const Renderer_DrawCall& b)
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
            for (uint32_t i = 0; i < m_draw_call_count; ++i)
            {
                const Renderer_DrawCall& dc = m_draw_calls[i];
                if (!dc.renderable->GetMaterial()->IsTransparent() && dc.camera_visible)
                {
                    m_draw_calls_prepass[m_draw_calls_prepass_count++] = dc;
                }
            }

            sort(m_draw_calls_prepass.begin(), m_draw_calls_prepass.begin() + m_draw_calls_prepass_count, [](const Renderer_DrawCall& a, const Renderer_DrawCall& b)
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
            m_indirect_draw_count = 0;
            for (uint32_t i = 0; i < m_draw_call_count; i++)
            {
                const Renderer_DrawCall& dc = m_draw_calls[i];
                Renderable* renderable      = dc.renderable;
                Material* material          = renderable->GetMaterial();

                if (!material || material->IsTransparent())
                    continue;
                if (IsCpuDrivenDraw(dc, material))
                    continue;

                uint32_t idx = m_indirect_draw_count++;
                if (idx >= MAX_ARRAY_SIZE)
                    break;

                Sb_IndirectDrawArgs& args = m_indirect_draw_args[idx];
                args.index_count          = renderable->GetIndexCount(dc.lod_index);
                args.instance_count       = dc.instance_count;
                args.first_index          = renderable->GetIndexOffset(dc.lod_index);
                args.vertex_offset        = static_cast<int32_t>(renderable->GetVertexOffset(dc.lod_index));
                args.first_instance       = dc.instance_index;

                // per-draw data (aabb_index includes the frame offset into the shared aabb buffer)
                uint32_t aabb_frame_offset = m_frame_resource_index * MAX_ARRAY_SIZE;
                Sb_DrawData& data       = m_indirect_draw_data[idx];
                Entity* entity          = renderable->GetEntity();
                data.transform          = entity->GetMatrix();
                data.transform_previous = entity->GetMatrixPrevious();
                data.material_index     = material->GetIndex();
                data.is_transparent     = 0;
                data.aabb_index         = aabb_frame_offset + m_draw_calls_prepass_count + idx;
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
            areas.reserve(m_draw_calls_prepass_count);

            for (uint32_t i = 0; i < m_draw_calls_prepass_count; i++)
            {
                Renderer_DrawCall& draw_call = m_draw_calls_prepass[i];
                Renderable* renderable = draw_call.renderable;
                Material* material = renderable->GetMaterial();

                if (!material || material->IsTransparent() || renderable->HasInstancing() || !draw_call.camera_visible)
                    continue;

                float screen_area = compute_screen_space_area(renderable->GetBoundingBox());

                // temporal hysteresis: bonus for previous occluders
                if (previous_occluders.find(renderable) != previous_occluders.end())
                {
                    screen_area *= 1.5f;
                }

                areas.push_back({ i, screen_area });
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
                m_draw_calls_prepass[areas[i].index].is_occluder = true;
                previous_occluders.insert(m_draw_calls_prepass[areas[i].index].renderable);
            }
        }
    }
    */

#pragma endregion

} // namespace SceneryEditorX

// --------------------------------------------------------------

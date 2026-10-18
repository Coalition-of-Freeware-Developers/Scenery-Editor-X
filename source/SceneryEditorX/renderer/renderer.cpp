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
#include <SceneryEditorX/scene/material.h>
#include "vulkan/bindless_manager.h"
#include "vulkan/descriptor_pool_manager.h"
#include "vulkan/swapchain.h"
#include "vulkan/uniform_buffer_set.h"
#include "vulkan/debug/graphics_debug.h"
#include "vulkan/debug/render_doc.h"
#include "vulkan/pipeline/pipeline.h"
#include "vulkan/pipeline/pipeline_cache.h"
#include "vulkan/pipeline/pipeline_spec.h"
#include "vulkan/shader/shader_manager.h"
#include <array>
#include <cstddef>
#include <mutex>
#include <sstream>
#include <thread>
#include <Editor/modules/scene_render.h>
#include <SDL3/SDL.h>
#include <SceneryEditorX/asset/model.h>
#include <SceneryEditorX/asset/manager/asset_manager.h>
#include <SceneryEditorX/core/application/application.h>
#include <SceneryEditorX/core/frame_profiler.h>
#include <SceneryEditorX/core/window/monitor_data.h>
#include <SceneryEditorX/renderer/gbuffer.h>
#include <SceneryEditorX/renderer/gpu_scene.h>
#include <SceneryEditorX/renderer/ui/ui_impl.h>
#include <SceneryEditorX/scene/camera.h>
#include <SceneryEditorX/scene/entity.h>
#include <SceneryEditorX/scene/scene.h>
#include <SceneryEditorX/scene/components/component_sets.h>
#include <SceneryEditorX/scene/components/renderable.h>
#include <algorithm>
#include <chrono>
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

			// ProduceFrame currently records into a QueueManager command list that is not
			// submitted by SubmitAndPresent(). Seal it so the list returns to Idle and can
			// be reused next frame, preventing command-list pool starvation.
			if (m_CmdList_Present && m_CmdList_Present->GetState() == CommandState::Recording)
			{
				m_CmdList_Present->Seal();
			}
*/

	RendererProperties *Renderer::m_Data = nullptr;
	static Ref<Swapchain> s_Swapchain = nullptr;
	static uint64_t s_FrameNumber = 0;

	std::atomic<bool> Renderer::m_ResourcesInitialized = false;

	// Bindless draw data
	std::array<ShaderBuffer_DrawData, RENDERER_MAX_DRAW_CALLS> Renderer::m_DrawData_CPU;
	uint32_t Renderer::m_DrawData_Count = 0;
	Renderer::PassState Renderer::m_PassState = {};

	CommandList *Renderer::m_CmdList_Compute = nullptr;
	CommandList *Renderer::m_CmdList_Present = nullptr;
	CommandList *Renderer::m_CmdList_Frame   = nullptr; // per-frame UI command list, submitted alongside swapchain CB
	Scope<AssetManager> Renderer::s_AssetManager = nullptr;
	uint32_t Renderer::m_ResourceIndex = 0;

	// First draw-data slot of the region this frame's draws were written to (WriteDrawData), so the bulk upload
	// lands on the same entries the shaders index instead of a region another frame in flight is reading.
	static uint32_t s_DrawDataRegionStart = 0;

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
	bool Renderer::m_Has_DirectionalLight = false;
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

	namespace
	{
		/**
		 * @struct GpuMatrix
		 * @brief 4x4 column-vector matrix (clip = M * position) stored row-major: element (r, c) at m[r * 4 + c].
		 *
		 * This is the memory layout the shaders expect: Slang reads it with its column-major layout, so the
		 * shaders' mul(v, M) applies M exactly as written here. Kept apart from xMath::Mat4, whose member layout
		 * depends on whether XMATH_MATRIX_IS_ROW_MAJOR is defined before mat4.h is included.
		 */
		struct GpuMatrix
		{
			std::array<float, 16> m{};

			float& operator()(const int row, const int column) { return m[row * 4 + column]; }
			float operator()(const int row, const int column) const { return m[row * 4 + column]; }

			static GpuMatrix Rows(const std::array<float, 16>& rows)
			{
				GpuMatrix result;
				result.m = rows;
				return result;
			}

			GpuMatrix operator*(const GpuMatrix& rhs) const
			{
				GpuMatrix result;
				for (int row = 0; row < 4; ++row)
				{
					for (int column = 0; column < 4; ++column)
					{
						float sum = 0.0f;
						for (int k = 0; k < 4; ++k)
							sum += (*this)(row, k) * rhs(k, column);
						result(row, column) = sum;
					}
				}
				return result;
			}

			/* @brief General inverse by cofactor expansion; returns identity for a singular matrix. */
			GpuMatrix Inverse() const
			{
				const std::array<float, 16>& a = m;
				std::array<float, 16> inv{};
				inv[0]  =  a[5] * a[10] * a[15] - a[5] * a[11] * a[14] - a[9] * a[6] * a[15] + a[9] * a[7] * a[14] + a[13] * a[6] * a[11] - a[13] * a[7] * a[10];
				inv[4]  = -a[4] * a[10] * a[15] + a[4] * a[11] * a[14] + a[8] * a[6] * a[15] - a[8] * a[7] * a[14] - a[12] * a[6] * a[11] + a[12] * a[7] * a[10];
				inv[8]  =  a[4] * a[9]  * a[15] - a[4] * a[11] * a[13] - a[8] * a[5] * a[15] + a[8] * a[7] * a[13] + a[12] * a[5] * a[11] - a[12] * a[7] * a[9];
				inv[12] = -a[4] * a[9]  * a[14] + a[4] * a[10] * a[13] + a[8] * a[5] * a[14] - a[8] * a[6] * a[13] - a[12] * a[5] * a[10] + a[12] * a[6] * a[9];
				inv[1]  = -a[1] * a[10] * a[15] + a[1] * a[11] * a[14] + a[9] * a[2] * a[15] - a[9] * a[3] * a[14] - a[13] * a[2] * a[11] + a[13] * a[3] * a[10];
				inv[5]  =  a[0] * a[10] * a[15] - a[0] * a[11] * a[14] - a[8] * a[2] * a[15] + a[8] * a[3] * a[14] + a[12] * a[2] * a[11] - a[12] * a[3] * a[10];
				inv[9]  = -a[0] * a[9]  * a[15] + a[0] * a[11] * a[13] + a[8] * a[1] * a[15] - a[8] * a[3] * a[13] - a[12] * a[1] * a[11] + a[12] * a[3] * a[9];
				inv[13] =  a[0] * a[9]  * a[14] - a[0] * a[10] * a[13] - a[8] * a[1] * a[14] + a[8] * a[2] * a[13] + a[12] * a[1] * a[10] - a[12] * a[2] * a[9];
				inv[2]  =  a[1] * a[6]  * a[15] - a[1] * a[7]  * a[14] - a[5] * a[2] * a[15] + a[5] * a[3] * a[14] + a[13] * a[2] * a[7]  - a[13] * a[3] * a[6];
				inv[6]  = -a[0] * a[6]  * a[15] + a[0] * a[7]  * a[14] + a[4] * a[2] * a[15] - a[4] * a[3] * a[14] - a[12] * a[2] * a[7]  + a[12] * a[3] * a[6];
				inv[10] =  a[0] * a[5]  * a[15] - a[0] * a[7]  * a[13] - a[4] * a[1] * a[15] + a[4] * a[3] * a[13] + a[12] * a[1] * a[7]  - a[12] * a[3] * a[5];
				inv[14] = -a[0] * a[5]  * a[14] + a[0] * a[6]  * a[13] + a[4] * a[1] * a[14] - a[4] * a[2] * a[13] - a[12] * a[1] * a[6]  + a[12] * a[2] * a[5];
				inv[3]  = -a[1] * a[6]  * a[11] + a[1] * a[7]  * a[10] + a[5] * a[2] * a[11] - a[5] * a[3] * a[10] - a[9]  * a[2] * a[7]  + a[9]  * a[3] * a[6];
				inv[7]  =  a[0] * a[6]  * a[11] - a[0] * a[7]  * a[10] - a[4] * a[2] * a[11] + a[4] * a[3] * a[10] + a[8]  * a[2] * a[7]  - a[8]  * a[3] * a[6];
				inv[11] = -a[0] * a[5]  * a[11] + a[0] * a[7]  * a[9]  + a[4] * a[1] * a[11] - a[4] * a[3] * a[9]  - a[8]  * a[1] * a[7]  + a[8]  * a[3] * a[5];
				inv[15] =  a[0] * a[5]  * a[10] - a[0] * a[6]  * a[9]  - a[4] * a[1] * a[10] + a[4] * a[2] * a[9]  + a[8]  * a[1] * a[6]  - a[8]  * a[2] * a[5];

				const float determinant = a[0] * inv[0] + a[1] * inv[4] + a[2] * inv[8] + a[3] * inv[12];
				if (std::abs(determinant) < 1e-20f)
					return Rows({ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 });

				const float inverseDeterminant = 1.0f / determinant;
				for (float& value : inv)
					value *= inverseDeterminant;

				return Rows(inv);
			}
		};

		/* @brief Writes a GpuMatrix into a constant-buffer matrix slot byte for byte. */
		void StoreGpuMatrix(xMath::Matrix& destination, const GpuMatrix& source)
		{
			static_assert(sizeof(xMath::Matrix) == sizeof(source.m), "xMath::Matrix must be 16 floats");
			std::memcpy(&destination, source.m.data(), sizeof(source.m));
		}

		/**
		 * @struct SceneCameraView
		 * @brief The active scene's primary camera expressed in the renderer's GPU conventions.
		 */
		struct SceneCameraView
		{
			GpuMatrix view;
			GpuMatrix projection;
			xMath::Vec3 position;
			xMath::Vec3 forward;
			xMath::Vec3 right;
			float nearPlane     = 0.1f;
			float farPlane      = 10000.0f;
			float fovHorizontal = 0.0f;
		};

		/**
		 * @brief Builds the view and projection of the active scene's primary camera entity.
		 *
		 * The view follows the camera controller (Camera::ProcessInput): right-handed, +Y up, forward taken from
		 * the transform's pitch (x) and yaw (y). The projection is reverse-Z (near -> 1, far -> 0) with D3D-style
		 * clip space (+Y up), which CommandList::SetViewport maps to Vulkan by flipping the viewport.
		 * @param aspect Width / height of the render target.
		 * @param out Receives the camera view.
		 * @return False when the scene has no camera entity.
		 */
		bool BuildSceneCameraView(const float aspect, SceneCameraView& out)
		{
			Scene* scene = Scene::GetActive();
			if (!scene)
				return false;

			auto cameras = scene->GetRegistry().view<TransformComponent, CameraComponent>();
			entt::entity selected = entt::null;
			for (const entt::entity entity : cameras)
			{
				if (selected == entt::null)
					selected = entity;

				if (cameras.get<CameraComponent>(entity).isPrimary)
				{
					selected = entity;
					break;
				}
			}

			if (selected == entt::null)
				return false;

			const TransformComponent& transform = cameras.get<TransformComponent>(selected);
			const CameraComponent& camera       = cameras.get<CameraComponent>(selected);

			const xMath::Vec3 euler   = transform.GetRotationEuler();
			const float cosPitch      = std::cos(euler.x);
			const xMath::Vec3 forward = xMath::Normalize(xMath::Vec3(cosPitch * std::sin(euler.y), std::sin(euler.x), cosPitch * std::cos(euler.y)));
			const xMath::Vec3 right   = xMath::Normalize(xMath::Cross(forward, xMath::Vec3(0.0f, 1.0f, 0.0f)));
			const xMath::Vec3 up      = xMath::Cross(right, forward);
			const xMath::Vec3 eye     = transform.translation;

			// the camera looks down its local -Z
			out.view = GpuMatrix::Rows({
				right.x,    right.y,    right.z,   -xMath::Dot(right, eye),
				up.x,       up.y,       up.z,      -xMath::Dot(up, eye),
				-forward.x, -forward.y, -forward.z, xMath::Dot(forward, eye),
				0.0f,       0.0f,       0.0f,       1.0f });

			const float nearPlane = std::max(camera.nearPlane, 0.001f);
			const float farPlane  = std::max(camera.farPlane, nearPlane + 1.0f);
			const float xScale    = 1.0f / std::tan(camera.horizontalFov_Rad * 0.5f);
			const float yScale    = xScale * aspect;
			out.projection = GpuMatrix::Rows({
				xScale, 0.0f,   0.0f,                                0.0f,
				0.0f,   yScale, 0.0f,                                0.0f,
				0.0f,   0.0f,   nearPlane / (farPlane - nearPlane),  (farPlane * nearPlane) / (farPlane - nearPlane),
				0.0f,   0.0f,   -1.0f,                               0.0f });

			out.position      = eye;
			out.forward       = forward;
			out.right         = right;
			out.nearPlane     = nearPlane;
			out.farPlane      = farPlane;
			out.fovHorizontal = camera.horizontalFov_Rad;
			return true;
		}

		/**
		 * @struct ViewFrustum
		 * @brief The clip planes of a GpuMatrix view-projection (clip = M * p, reverse-Z with 0 <= z <= w), for CPU
		 * culling in the same conventions as gpu_cull.slang. Planes are combinations of M's rows, unnormalized.
		 */
		struct ViewFrustum
		{
			std::array<std::array<float, 4>, 6> planes{};

			ViewFrustum() = default;

			explicit ViewFrustum(const GpuMatrix &m)
			{
				const auto plane = [&m](const int row, const float sign)
				{
					return std::array<float, 4>{ m(3, 0) + sign * m(row, 0), m(3, 1) + sign * m(row, 1), m(3, 2) + sign * m(row, 2), m(3, 3) + sign * m(row, 3) };
				};
				planes = { plane(0, 1.0f), plane(0, -1.0f), plane(1, 1.0f), plane(1, -1.0f), plane(2, -1.0f),
						   std::array<float, 4>{ m(2, 0), m(2, 1), m(2, 2), m(2, 3) } };
			}

			/* @brief Whether a local box, placed by a row-vector world matrix, can be inside the frustum. */
			[[nodiscard]] bool IntersectsBox(const xMath::BoundingBox &local, const xMath::Matrix &world) const
			{
				const xMath::Vec3 lo = local.GetMin();
				const xMath::Vec3 hi = local.GetMax();
				const float c[3] = { (lo.x + hi.x) * 0.5f, (lo.y + hi.y) * 0.5f, (lo.z + hi.z) * 0.5f };
				const float e[3] = { (hi.x - lo.x) * 0.5f, (hi.y - lo.y) * 0.5f, (hi.z - lo.z) * 0.5f };

				// world center and half extents (row-vector: p' = p * M; the absolute 3x3 bounds the rotated box)
				const float center[3] = {
					c[0] * world.m00 + c[1] * world.m10 + c[2] * world.m20 + world.m30,
					c[0] * world.m01 + c[1] * world.m11 + c[2] * world.m21 + world.m31,
					c[0] * world.m02 + c[1] * world.m12 + c[2] * world.m22 + world.m32 };
				const float extent[3] = {
					e[0] * std::abs(world.m00) + e[1] * std::abs(world.m10) + e[2] * std::abs(world.m20),
					e[0] * std::abs(world.m01) + e[1] * std::abs(world.m11) + e[2] * std::abs(world.m21),
					e[0] * std::abs(world.m02) + e[1] * std::abs(world.m12) + e[2] * std::abs(world.m22) };

				for (const std::array<float, 4> &p : planes)
				{
					const float distance = p[0] * center[0] + p[1] * center[1] + p[2] * center[2] + p[3];
					const float radius = std::abs(p[0]) * extent[0] + std::abs(p[1]) * extent[1] + std::abs(p[2]) * extent[2];
					if (distance + radius < 0.0f)
						return false;
				}
				return true;
			}
		};
	}

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

		{
			// Get window dimensions
			uint32_t width = Window::GetWidth();
			uint32_t height = Window::GetHeight();

			SetOutputResolution(width, height, false);
			SetRendererResolution(1920, 1080, false);
			SetViewport(static_cast<float>(width), static_cast<float>(height));
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
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Frame Resources                                                                                               ///
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		s_AssetManager = CreateScope<AssetManager>();

		CreateFrameResources();

#pragma region Temporary Debug 

		// Temporary debug trace: log sync vectors sizes after frame resources creation
		SEDX_CORE_TRACE_TAG("Renderer", "Trace: After CreateFrameResources sizes - fences={}, render={}",
			static_cast<uint32_t>(s_FenceHandles.size()), static_cast<uint32_t>(s_RenderSemaphoreHandles.size()));

#pragma endregion

		/*
		ThreadPool::Submit([]()
		{
		*/

			// Bootstrap path: initialize critical resources synchronously so first frames
			// always have quad mesh + essential shaders available.
			m_ResourcesInitialized = false;
			CreateStandardMeshes();
			CreateStandardTextures();
			CreateStandardMaterials();
			CreateFonts();
			LoadShaders();
			m_ResourcesInitialized = true;

		/*
		});
		*/

		// Allocates every GetStructuredBuffers() slot read each frame (UpdateMaterials, UpdateLights,
		// UpdateBoundingBoxes, WriteDrawData, ...) plus the per-frame indirect buffers, exactly once.
		CreateBuffers();
		CreateDepthStencilStates();
		CreateRasterizerStates();
		CreateBlendStates();
		CreateRenderTargets(true, true, true);
		CreateSamplers();
		GeometryBuffer::Initialize();

		// Create the bindless descriptor sets now: Tick() writes them (draw data, samplers, aabbs, ...) before the
		// first pipeline is built, and those one-time writes are dropped while the sets don't exist yet.
		BindlessManager::Init();

		// Query surface capabilities
		VkPhysicalDevice physicalDevice = RenderContext::Get()->GetDevice()->GetPhysicalDevice();
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, s_Swapchain->GetSurface(), &s_SurfaceCaps);

		// Draw calls are handled by the active pass/scene rendering path.
		RenderContext::Get()->renderables = nullptr;
		RenderContext::Get()->s_ShaderDataBuffers = nullptr;
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

		// Release the immediate-execution command lists/pools now, while the device and loggers are alive.
		CommandList::ShutdownImmediateExecution();

		// Release cached pipelines and descriptor-set layouts while the device, the shared descriptor pool
		// and the deletion queue are alive; command lists still pointing at them keep only empty objects.
		PipelineCache::Clear();

		// Release bindless/cached descriptor sets and layouts, then the shared descriptor pool, while the
		// logical device is still valid (BindlessManager::Shutdown ends with DescriptorPoolManager::Shutdown).
		BindlessManager::Shutdown();

		// Destroy camera resources
		{
			VkDevice dev = device->GetLogicalDevice();
			VmaAllocator vma = device->GetMemoryAllocator().GetAllocator();

			for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
			{
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

		ShaderManager::ClearAll();

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

		// Per-frame indirect-draw buffers live in a static array; release them while the allocator is
		// alive instead of in the CRT static-destructor phase.
		for (IndirectFrameResource& frameResource : m_FrameResources)
		{
			for (Ref<Buffer>* buffer : { &frameResource.m_DrawArgs, &frameResource.m_DrawData, &frameResource.m_DrawArgs_Out,
										 &frameResource.m_DrawData_Out, &frameResource.m_DrawCount })
			{
				if (*buffer)
				{
					(*buffer)->Destroy();
					buffer->Reset();
				}
			}
		}

		// Destroy frame resources
		GpuScene::Shutdown();
		GeometryBuffer::Shutdown();
		DestroyResources();
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

		// Everything released above is still in the deferred deletion queue. RenderContext::Shutdown()
		// (via Device::Destroy) flushes it while the device is alive and then closes it with
		// QueueManager::NotifyShutdown(), so CRT static-dtor phase enqueues are dropped.
	}

	void Renderer::Tick()
	{
		SEDX_PERF_SCOPE("Render: Tick (CPU)");

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
				m_Camera->Tick();

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
		if (!m_CmdList_Present)
		{
			SEDX_CORE_ERROR_TAG("Renderer", "Failed to acquire present command list");
			return;
		}
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
		// Keep compute command-list acquisition disabled while running the minimal
		// bootstrap path. Acquiring a second list every frame without submitting it
		// can starve QueueManager::NextCommandList() and trigger "no healthy list".
		const bool needsComputeCommandList = false;
		m_CmdList_Compute = nullptr;
		if (canRender && needsComputeCommandList)
		{	
			if (Ref<Queue> *queue = queueManager->GetQueue(QueueType::Compute); queue && *queue)
			{
				Queue::WaitIdle(*queue->Get());
			}

			m_CmdList_Compute = queueManager->NextCommandList();
			if (!m_CmdList_Compute)
			{
				SEDX_CORE_WARN_TAG("Renderer", "Failed to acquire compute command list; continuing without compute list this frame");
			}
			else
			{
				m_CmdList_Compute->Begin();
			}
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

			{
				SEDX_PERF_SCOPE("Render: UpdateDrawCalls");
				UpdateDrawCalls(m_CmdList_Present);
				Perf::SetCounter("entities", static_cast<double>(Scene::GetEntities().size()));
				Perf::SetCounter("cpu draw calls", static_cast<double>(m_DrawCall_Count));
			}

			// retire cached pipelines that stopped being used (e.g. built for render targets replaced on resize)
			PipelineCache::Tick();

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
					if (Buffer *constantFrameBuffer = GetBuffer(Renderer_Buffer::ConstantFrame))
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

				{
					SEDX_PERF_SCOPE("Render: Lights");
					if (initialize || Scene::UpdateSceneChanges(ComponentType::Light).IsDirty())
					{
						SEDX_PERF_SCOPE("Render: Lights changed (upload)");
						UpdateShadowAtlas();
						UpdateLights(m_CmdList_Present);
						BindlessManager::UpdateBuffer(BindlessResource::LightParameters, GetBuffer(Renderer_Buffer::LightParameters));
					}
				}

#pragma endregion
#pragma region Materials

				{
					SEDX_PERF_SCOPE("Render: Materials");
					if (initialize || Scene::UpdateSceneChanges(ComponentType::Material).IsDirty())
					{
						UpdateMaterials(m_CmdList_Present);
						BindlessManager::UpdateImages(&m_Bindless_Textures, m_Bindless_TextureCount);
						BindlessManager::UpdateBuffer(BindlessResource::MaterialParameters, GetBuffer(Renderer_Buffer::MaterialParameters));
						GpuScene::OnMaterialsChanged(); // material slots were reassigned
					}
				}

#pragma endregion
#pragma region Samplers

				if (m_BindlessSamplers_Dirty.IsDirty())
				{
					// resources.slang: samplers_comparison (set 6) holds Compare_depth, samplers (set 7) the
					// regular ones in Renderer_Sampler order after it (sampler_point_clamp = 0 ... anisotropic = 7)
					auto &samplers = Renderer::GetSamplers();
					constexpr uint32_t comparisonIndex = static_cast<uint32_t>(Renderer_Sampler::Compare_depth);
					static_assert(comparisonIndex == 0, "the regular samplers must follow the comparison sampler");
					BindlessManager::UpdateSamplers(BindlessResource::SamplersComparison, &samplers[comparisonIndex], 1);
					BindlessManager::UpdateSamplers(BindlessResource::SamplersRegular, samplers.data() + 1, static_cast<uint32_t>(samplers.size()) - 1);
					m_BindlessSamplers_Dirty.Check();
				}

#pragma endregion
#pragma region AABBs (always, they change with entity transforms)

				{
					SEDX_PERF_SCOPE("Render: AABBs");
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
						uint32_t frame_byte_offset = s_DrawDataRegionStart * static_cast<uint32_t>(sizeof(ShaderBuffer_DrawData));
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
					// by generation: a reallocated buffer can land at the address of the one it replaced
					static uint64_t boundGeneration = 0;
					Buffer *currentVertex = GeometryBuffer::GetVertexBuffer();
					Buffer *currentIndex = GeometryBuffer::GetIndexBuffer();
					if (currentVertex && currentIndex && GeometryBuffer::GetGeneration() != boundGeneration)
					{
						BindlessManager::UpdateBuffer(BindlessResource::GeometryVertices, currentVertex);
						BindlessManager::UpdateBuffer(BindlessResource::GeometryIndices, currentIndex);
						boundGeneration = GeometryBuffer::GetGeneration();
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
#pragma region GPU scene

				// Instances, batches and draw commands; before anything binds this frame (it may grow bindless buffers)
				GpuScene::Update(m_CmdList_Present);
				{
					const GpuScene::Stats &stats = GpuScene::GetStats();
					Perf::SetCounter("gpu instances", stats.instances);
					Perf::SetCounter("gpu enabled", stats.enabledInstances);
					Perf::SetCounter("gpu visible", stats.visibleInstances);
					Perf::SetCounter("gpu batches", stats.batches);
					Perf::SetCounter("gpu commands drawn", stats.drawnCommands);
					Perf::SetCounter("gpu uploads", stats.uploadedInstances);
				}

#pragma endregion

			}

#pragma endregion
				
			UpdateFrameConstantBuffer(m_CmdList_Present);
			{
				SEDX_PERF_SCOPE("Render: Debug lines");
				UpdatePersistentLines();
				AddLinesToBeRendered();
			}

			//SubmitAndPresent();

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


			// Wire the modern pass-based renderer into the active frame loop.
			// This keeps the existing legacy path intact while enabling
			// `Pass_Depth_Prepass()` and `Pass_Grid()` execution.
			if (m_CmdList_Present)
			{
				ImageResource* rtRender = GetRenderTarget(Renderer_RenderTarget::frame_render);
				ImageResource* rtOutput = GetRenderTarget(Renderer_RenderTarget::frame_output);

				const bool hasRtRender = rtRender && rtRender->Get() && *rtRender->Get() != VK_NULL_HANDLE;
				const bool hasRtOutput = rtOutput && rtOutput->Get() && *rtOutput->Get() != VK_NULL_HANDLE;

				if (hasRtRender && hasRtOutput)
				{
					SEDX_PERF_SCOPE("Render: Record passes");
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

			/*
			if (m_CmdList_Present)
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

			// UI rendering is done inside RecordRenderCommands (inside the dynamic render pass),
			// so no separate UI command list is needed here.
			*/

		}
	}   

#pragma endregion

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

				// Destroy frame-sync resources before swapchain recreation and recreate afterward
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
		const bool hasFrameFence = m_CurrentFrameIndex < s_FenceHandles.size() && s_FenceHandles[m_CurrentFrameIndex] != VK_NULL_HANDLE;
		if (hasFrameFence)
		{
			SEDX_CORE_TRACE_TAG("Renderer", "Waiting for fence of frame {} (fence handle: {})", m_CurrentFrameIndex,
				static_cast<void *>(s_FenceHandles[m_CurrentFrameIndex]));
			VkDevice device = RenderContext::Get()->GetDevice()->GetLogicalDevice();
			vkWaitForFences(device, 1, &s_FenceHandles[m_CurrentFrameIndex], VK_TRUE, UINT64_MAX);
		}

		// Acquire image through the swapchain flow.
		s_Swapchain->AcquireNextImage();
		if (!s_Swapchain->IsImageAcquired())
			return false; // Fence left signaled: nothing will be submitted for this frame

		m_SwapchainImageIndex = s_Swapchain->GetImageIndex();
		if (m_SwapchainImageIndex >= s_Swapchain->GetImages().size())
		{
			SEDX_CORE_WARN_TAG("Renderer", "Swapchain AcquireNextImage returned invalid index: {}", m_SwapchainImageIndex);
			return false;
		}

		// Reset only once this frame is certain to submit; resetting before a failed acquire would leave
		// the fence unsignaled forever and the next wait on it would hang the render thread.
		if (hasFrameFence)
		{
			VkDevice device = RenderContext::Get()->GetDevice()->GetLogicalDevice();
			vkResetFences(device, 1, &s_FenceHandles[m_CurrentFrameIndex]);
			SEDX_CORE_TRACE_TAG("Renderer", "Fence wait and reset complete for frame {}", m_CurrentFrameIndex);
		}

		SEDX_CORE_TRACE_TAG("Renderer", "Acquired swapchain image index: {}", m_SwapchainImageIndex);

		// Begin command buffer recording
		VkCommandBuffer cb = m_CommandBuffers[m_CurrentFrameIndex];
		if (cb == VK_NULL_HANDLE)
		{
			SEDX_CORE_ERROR_TAG("Renderer", "No command buffer allocated for frame index {}", m_CurrentFrameIndex);
			return false;
		}

		vkResetCommandBuffer(cb, 0);
		SEDX_CORE_TRACE_TAG("Renderer", "Command buffer reset for frame {}", m_CurrentFrameIndex);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VkResult result = vkBeginCommandBuffer(cb, &beginInfo);
		SEDX_VK_RESULT_ASSERT(result, "vkBeginCommandBuffer failed");

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

		// The render passes were recorded into m_CmdList_Present, which is submitted ahead of the frame
		// command buffer. End its work with frame_output in a known shader-read layout so the blit recorded
		// below (which takes its old layout from the tracker) matches what the GPU will actually see.
		if (m_CmdList_Present && m_CmdList_Present->GetState() == CommandState::Recording)
		{
			m_CmdList_Present->EndRenderPass();

			ImageResource* frameOutput = GetRenderTarget(Renderer_RenderTarget::frame_output);
			if (frameOutput && frameOutput->Get() && *frameOutput->Get() != VK_NULL_HANDLE)
				frameOutput->SetLayout(Layout::ImageLayout::ShaderRead, m_CmdList_Present, 0, 0);

			m_CmdList_Present->FlushBarriers();
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
		auto is_likely_uninitialized = [](uint64_t val) 
		{
			// Common MSVC uninitialized patterns: 0xCCCCCCCCCCCCCCCC, 0xCDCDCDCDCDCDCDCD
			return val == 0xCCCCCCCCCCCCCCCCULL || val == 0xCDCDCDCDCDCDCDCDULL;
		};

		// Validate device and logical device
		VkDevice vkDevice = VK_NULL_HANDLE;
		if (device.IsValid())
			vkDevice = device->GetDevice();

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

		SEDX_CORE_VERIFY(!is_likely_uninitialized(presentSem), "Present semaphore appears uninitialized: 0x{:x}", presentSem);
		SEDX_CORE_VERIFY(!is_likely_uninitialized(renderSem), "Render semaphore appears uninitialized: 0x{:x}", renderSem);
		SEDX_CORE_VERIFY(!is_likely_uninitialized(fenceHandle), "Fence handle appears uninitialized: 0x{:x}", fenceHandle);
		SEDX_CORE_VERIFY(!is_likely_uninitialized(cbHandle), "Command buffer appears uninitialized: 0x{:x}", cbHandle);

		// Submit the render-pass list (uploads + scene passes recorded in Tick) ahead of the swapchain CB
		// that blits frame_output and draws the UI. Submission order keeps the blit after the passes; the
		// per-frame fence covers both, which is what lets Seal() hand the list back to the pool as Idle.
		VkCommandBuffer frameCBs[2] = {};
		uint32_t frameCBCount = 0;
		if (m_CmdList_Present && m_CmdList_Present->GetState() == CommandState::Recording)
		{
			m_CmdList_Present->Seal();
			frameCBs[frameCBCount++] = m_CmdList_Present->GetCommandBuffer();
		}
		frameCBs[frameCBCount++] = cb;

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

	// @Note: This was the prior function for creating frame resources before Blit implementation.
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

		bool begun = false;
		{
			SEDX_PERF_SCOPE("Render: BeginFrame (GPU wait)");
			begun = BeginFrame();
		}
		if (begun)
		{
			Tick();
			{
				SEDX_PERF_SCOPE("Render: EndFrame+Present");
				EndFrame();
				SubmitAndPresent();
			}
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

#pragma endregion

	void Renderer::LoadShaders()
	{
		SEDX_CORE_TRACE_TAG("Renderer", "Registering renderer shaders and creating camera resources");

		ShaderManager::CreateShaders();

		if (m_CameraDescriptorSetLayout == VK_NULL_HANDLE)
		{
			CreateCameraResources();
		}

		// Keep legacy RenderContext pipeline pointers cleared while the modern pass-based
		// renderer and asset-driven path are active.
		if (Ref<RenderContext> context = RenderContext::Get(); context)
		{
			context->pipeline = VK_NULL_HANDLE;
			context->pipelineLayout = VK_NULL_HANDLE;
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

		// With the editor UI up, frame_output is shown inside the viewport panel (sampled in SHADER_READ_ONLY_OPTIMAL,
		// where EndFrame left it), so the swapchain only gets cleared and the UI drawn on top. Without a UI the
		// scene is blitted to the whole back buffer.
		const bool drawUI = UI::IsBackendInitialized() && ImGui::GetCurrentContext() && ImGui::GetDrawData();

		ImageResource* frameOutput = GetRenderTarget(Renderer_RenderTarget::frame_output);
		const bool hasFrameOutput = !drawUI && frameOutput && frameOutput->Get() && *frameOutput->Get() != VK_NULL_HANDLE;

		VkImageMemoryBarrier2 swapchainToTransfer{};
		swapchainToTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		swapchainToTransfer.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
		swapchainToTransfer.srcAccessMask = 0;
		swapchainToTransfer.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT | VK_PIPELINE_STAGE_2_CLEAR_BIT;
		swapchainToTransfer.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		swapchainToTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		swapchainToTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		swapchainToTransfer.image = swapchainImages[imageIndex];
		swapchainToTransfer.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		swapchainToTransfer.subresourceRange.baseMipLevel = 0;
		swapchainToTransfer.subresourceRange.levelCount = 1;
		swapchainToTransfer.subresourceRange.baseArrayLayer = 0;
		swapchainToTransfer.subresourceRange.layerCount = 1;

		// With the UI drawing the whole back buffer, the swapchain image goes straight to attachment layout
		// below and the pass's CLEAR load op replaces the transfer clear.
		if (!drawUI)
		{
			VkDependencyInfo toTransferInfo{};
			toTransferInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			toTransferInfo.imageMemoryBarrierCount = 1;
			toTransferInfo.pImageMemoryBarriers = &swapchainToTransfer;
			vkCmdPipelineBarrier2(cb, &toTransferInfo);
		}

		if (hasFrameOutput)
		{
			const Layout::ImageLayout trackedFrameOutputLayout = frameOutput->GetLayout(0);
			const VkImageLayout oldFrameOutputLayout = (trackedFrameOutputLayout == Layout::ImageLayout::General)
				? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			const VkAccessFlags2 oldFrameOutputAccess = (trackedFrameOutputLayout == Layout::ImageLayout::General)
				? (VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT) : VK_ACCESS_2_SHADER_READ_BIT;

			VkImageMemoryBarrier2 toBlit[2]{};

			toBlit[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			toBlit[0].srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
			toBlit[0].srcAccessMask = oldFrameOutputAccess;
			toBlit[0].dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
			toBlit[0].dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
			toBlit[0].oldLayout = oldFrameOutputLayout;
			toBlit[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			toBlit[0].image = *frameOutput->Get();
			toBlit[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			toBlit[0].subresourceRange.baseMipLevel = 0;
			toBlit[0].subresourceRange.levelCount = 1;
			toBlit[0].subresourceRange.baseArrayLayer = 0;
			toBlit[0].subresourceRange.layerCount = 1;

			toBlit[1] = swapchainToTransfer;
			toBlit[1].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			toBlit[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

			VkDependencyInfo toBlitInfo{};
			toBlitInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			toBlitInfo.imageMemoryBarrierCount = 2;
			toBlitInfo.pImageMemoryBarriers = toBlit;
			vkCmdPipelineBarrier2(cb, &toBlitInfo);

			VkExtent2D extent = s_Swapchain->GetExtent();
			VkImageBlit blit{};
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = 0;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;
			blit.srcOffsets[0] = {.x = 0, .y = 0, .z = 0};
			blit.srcOffsets[1] = {
				.x = static_cast<int32_t>(frameOutput->GetWidth()),
				.y = static_cast<int32_t>(frameOutput->GetHeight()),
				.z = 1
			};
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = 0;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;
			blit.dstOffsets[0] = {.x = 0, .y = 0, .z = 0};
			blit.dstOffsets[1] = {
				.x = static_cast<int32_t>(extent.width),
				.y = static_cast<int32_t>(extent.height),
				.z = 1
			};

			vkCmdBlitImage(cb, *frameOutput->Get(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				swapchainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit, VK_FILTER_LINEAR);

			VkImageMemoryBarrier2 restoreFrameOutput{};
			restoreFrameOutput.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			restoreFrameOutput.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
			restoreFrameOutput.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
			restoreFrameOutput.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
			restoreFrameOutput.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
			restoreFrameOutput.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			restoreFrameOutput.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			restoreFrameOutput.image = *frameOutput->Get();
			restoreFrameOutput.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			restoreFrameOutput.subresourceRange.baseMipLevel = 0;
			restoreFrameOutput.subresourceRange.levelCount = 1;
			restoreFrameOutput.subresourceRange.baseArrayLayer = 0;
			restoreFrameOutput.subresourceRange.layerCount = 1;

			VkDependencyInfo restoreFrameOutputInfo{};
			restoreFrameOutputInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			restoreFrameOutputInfo.imageMemoryBarrierCount = 1;
			restoreFrameOutputInfo.pImageMemoryBarriers = &restoreFrameOutput;
			vkCmdPipelineBarrier2(cb, &restoreFrameOutputInfo);
		}
		else if (!drawUI)
		{
			VkClearColorValue clearColor{};
			clearColor.float32[0] = 0.0f;
			clearColor.float32[1] = 0.0f;
			clearColor.float32[2] = 0.0f;
			clearColor.float32[3] = 1.0f;

			VkImageSubresourceRange range{};
			range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			range.baseMipLevel = 0;
			range.levelCount = 1;
			range.baseArrayLayer = 0;
			range.layerCount = 1;

			vkCmdClearColorImage(cb, swapchainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &range);
		}

		VkImageMemoryBarrier2 swapchainToAttachment{};
		swapchainToAttachment.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		if (drawUI)
		{
			// Contents are discarded; the acquire semaphore wait covers prior presentation reads.
			swapchainToAttachment.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			swapchainToAttachment.srcAccessMask = VK_ACCESS_2_NONE;
			swapchainToAttachment.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		}
		else
		{
			swapchainToAttachment.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT | VK_PIPELINE_STAGE_2_CLEAR_BIT;
			swapchainToAttachment.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			swapchainToAttachment.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		}
		swapchainToAttachment.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		swapchainToAttachment.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;
		swapchainToAttachment.newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
		swapchainToAttachment.image = swapchainImages[imageIndex];
		swapchainToAttachment.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		swapchainToAttachment.subresourceRange.baseMipLevel = 0;
		swapchainToAttachment.subresourceRange.levelCount = 1;
		swapchainToAttachment.subresourceRange.baseArrayLayer = 0;
		swapchainToAttachment.subresourceRange.layerCount = 1;

		VkDependencyInfo toAttachmentInfo{};
		toAttachmentInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		toAttachmentInfo.imageMemoryBarrierCount = 1;
		toAttachmentInfo.pImageMemoryBarriers = &swapchainToAttachment;
		vkCmdPipelineBarrier2(cb, &toAttachmentInfo);

		VkRenderingAttachmentInfo colorAttachmentInfo{};
		colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		colorAttachmentInfo.imageView = swapchainImageViews[imageIndex];
		colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
		colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentInfo.clearValue = {.color = {{0.0f, 0.0f, 0.0f, 1.0f}}};		

		VkExtent2D extent = s_Swapchain->GetExtent();
		VkRenderingInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.renderArea.offset = {.x = 0, .y = 0};
		renderingInfo.renderArea.extent = {.width = extent.width, .height = extent.height};
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colorAttachmentInfo;

		vkCmdBeginRendering(cb, &renderingInfo);

		VkViewport vp{};
		vp.x = 0.0f;
		vp.y = 0.0f;
		vp.width = static_cast<float>(extent.width);
		vp.height = static_cast<float>(extent.height);
		vp.minDepth = 0.0f;
		vp.maxDepth = 1.0f;
		vkCmdSetViewport(cb, 0, 1, &vp);

		VkRect2D scissor{};
		scissor.offset = {.x = 0, .y = 0};
		scissor.extent = {.width = extent.width, .height = extent.height};
		vkCmdSetScissor(cb, 0, 1, &scissor);

		if (drawUI)
		{
			UI::RenderMainViewport(cb);
		}

		vkCmdEndRendering(cb);

		VkImageMemoryBarrier2 toPresent{};
		toPresent.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		toPresent.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		toPresent.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		toPresent.dstStageMask = VK_PIPELINE_STAGE_2_NONE;
		toPresent.dstAccessMask = 0;
		toPresent.oldLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
		toPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		toPresent.image = swapchainImages[imageIndex];
		toPresent.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		toPresent.subresourceRange.baseMipLevel = 0;
		toPresent.subresourceRange.levelCount = 1;
		toPresent.subresourceRange.baseArrayLayer = 0;
		toPresent.subresourceRange.layerCount = 1;

		VkDependencyInfo toPresentInfo{};
		toPresentInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		toPresentInfo.imageMemoryBarrierCount = 1;
		toPresentInfo.pImageMemoryBarriers = &toPresent;
		vkCmdPipelineBarrier2(cb, &toPresentInfo);
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
		for (Entity* entity : Scene::GetLightEntities())
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
		(void)cmdList;

		m_DrawCall_Count          = 0;
		m_DrawCalls_Prepass_Count = 0;
		m_Transparents_Present    = false;

		// The per-draw indirect path (indirect_cull) stays off: GpuScene draws the deferred geometry instead.
		m_Indirect_DrawCount = 0;

		// The camera frustum in the GPU conventions (the scene Camera's own projection is GL-style)
		ViewFrustum frustum;
		bool hasFrustum = false;
		{
			const xMath::Vec2 resolution = GetRendererResolution();
			SceneCameraView camera;
			if (BuildSceneCameraView(resolution.y > 0.0f ? resolution.x / resolution.y : 1.0f, camera))
			{
				frustum = ViewFrustum(camera.projection * camera.view);
				hasFrustum = true;
			}
		}

		// collect draw calls: renderables the GPU scene doesn't draw, plus the blended part of the ones it does
		for (Entity* entity : Scene::GetRenderableEntities())
		{
			if (!entity || !entity->GetActive())
				continue;

			Renderable* renderable = entity->GetComponent<Renderable>();
			if (!renderable || renderable->GetLodCount() == 0)
				continue;

			MaterialAsset* material = renderable->GetMaterialAsset();
			if (!material)
				continue;

			const bool gpuDriven = GpuScene::IsGpuDriven(renderable);
			if (gpuDriven && material->GetProperty(MaterialProperty::BlendMode) <= 0.0f)
				continue;

			if (m_DrawCall_Count >= static_cast<uint32_t>(m_DrawCalls.size()))
			{
				static bool s_LoggedDrawCallOverflow = false;
				if (!s_LoggedDrawCallOverflow)
				{
					SEDX_CORE_WARN_TAG("Renderer", "UpdateDrawCalls: draw call capacity reached ({})", static_cast<uint32_t>(m_DrawCalls.size()));
					s_LoggedDrawCallOverflow = true;
				}
				break;
			}

			// frustum culling on the world bounds (the renderable's own box is only updated when its mesh is set)
			const xMath::Matrix world = entity->GetMatrix();
			if (hasFrustum && !renderable->HasInstancing() && !frustum.IntersectsBox(renderable->GetBoundingBoxMesh(), world))
				continue;

			const bool isTransparent = material->IsTransparent();
			m_Transparents_Present  |= isTransparent;

			const uint32_t drawDataIndex = WriteDrawData(world, entity->GetMatrixPrevious(), material->GetIndex(), isTransparent ? 1 : 0);

			Renderer_DrawCall& drawCall = m_DrawCalls[m_DrawCall_Count++];
			drawCall.renderable         = renderable;
			drawCall.lodIndex           = std::min(renderable->GetLodIndex(), renderable->GetLodCount() - 1);
			drawCall.isOccluder         = false;
			drawCall.cameraVisible      = true;
			drawCall.gpuDriven          = gpuDriven;
			drawCall.instanceIndex      = 0;
			drawCall.instanceCount      = renderable->GetInstanceCount();
			drawCall.drawData_Index     = drawDataIndex;
		}

		// sort: opaque before transparent, then by material to minimize state changes
		std::sort(m_DrawCalls.begin(), m_DrawCalls.begin() + m_DrawCall_Count, [](const Renderer_DrawCall& a, const Renderer_DrawCall& b)
		{
			const bool aTransparent = a.renderable->GetMaterialAsset()->IsTransparent();
			const bool bTransparent = b.renderable->GetMaterialAsset()->IsTransparent();
			if (aTransparent != bTransparent)
				return !aTransparent;

			return a.renderable->GetMaterialAsset() < b.renderable->GetMaterialAsset();
		});

		// prepass: visible opaques the CPU draws, non-alpha-tested first
		for (uint32_t i = 0; i < m_DrawCall_Count; ++i)
		{
			const Renderer_DrawCall& drawCall = m_DrawCalls[i];
			if (!drawCall.gpuDriven && !drawCall.renderable->GetMaterialAsset()->IsTransparent() && drawCall.cameraVisible)
			{
				m_DrawCalls_Prepass[m_DrawCalls_Prepass_Count++] = drawCall;
			}
		}

		std::stable_sort(m_DrawCalls_Prepass.begin(), m_DrawCalls_Prepass.begin() + m_DrawCalls_Prepass_Count, [](const Renderer_DrawCall& a, const Renderer_DrawCall& b)
		{
			return !a.renderable->GetMaterialAsset()->IsAlphaTested() && b.renderable->GetMaterialAsset()->IsAlphaTested();
		});
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
		Buffer* frameBuffer = GetBuffer(Renderer_Buffer::ConstantFrame);
		if (!cmdList || !frameBuffer)
			return;

		ConstantBuffer_Frame& frame = m_Cb_Frame_Cpu;

		// time
		using Clock = std::chrono::steady_clock;
		static const Clock::time_point s_StartTime = Clock::now();
		static Clock::time_point s_LastTime        = s_StartTime;
		const Clock::time_point now                = Clock::now();
		const double timeSeconds                   = std::chrono::duration<double>(now - s_StartTime).count();
		const float deltaSeconds                   = static_cast<float>(std::chrono::duration<double>(now - s_LastTime).count());
		s_LastTime                                 = now;

		// camera
		static bool s_HasPreviousFrame = false;
		const xMath::Vec2 resolutionRender = GetRendererResolution();
		const float aspect = resolutionRender.y > 0.0f ? resolutionRender.x / resolutionRender.y : 1.0f;

		SceneCameraView camera;
		if (BuildSceneCameraView(aspect, camera))
		{
			const GpuMatrix viewProjection = camera.projection * camera.view;

			StoreGpuMatrix(frame.view, camera.view);
			StoreGpuMatrix(frame.view_inv, camera.view.Inverse());
			StoreGpuMatrix(frame.projection, camera.projection);
			StoreGpuMatrix(frame.projection_inv, camera.projection.Inverse());
			StoreGpuMatrix(frame.view_projection, viewProjection);
			StoreGpuMatrix(frame.view_projection_inv, viewProjection.Inverse());
			frame.view_projection_unjittered = frame.view_projection; // no TAA jitter in the bootstrap path

			if (xMath::Length2(frame.camera_position - camera.position) != 0.0f)
				frame.camera_last_movement_time = static_cast<float>(timeSeconds);

			frame.camera_position = camera.position;
			frame.camera_forward  = camera.forward;
			frame.camera_right    = camera.right;
			frame.camera_near     = camera.nearPlane;
			frame.camera_far      = camera.farPlane;
			frame.camera_fov      = camera.fovHorizontal;
		}

		// the first frame has no history, so it is its own previous frame (no motion)
		if (!s_HasPreviousFrame)
		{
			frame.view_previous                       = frame.view;
			frame.projection_previous                 = frame.projection;
			frame.view_projection_previous            = frame.view_projection;
			frame.view_projection_previous_unjittered = frame.view_projection_unjittered;
			frame.camera_position_previous            = frame.camera_position;
			s_HasPreviousFrame                        = true;
		}

		frame.view_projection_ortho = xMath::Matrix::IDENTITY;
		frame.resolution_render     = resolutionRender;
		frame.resolution_output     = GetOutputResolution();
		frame.taa_jitter_previous   = frame.taa_jitter_current;
		frame.taa_jitter_current    = xMath::Vec2(0.0f, 0.0f);
		frame.camera_aperture       = 2.8f;
		frame.time                  = timeSeconds;
		frame.delta_time            = deltaSeconds;
		frame.frame                 = static_cast<uint32_t>(m_FrameNumber);
		frame.options               = 0; // no ray-traced reflections, no SSAO in the bootstrap path
		frame.resolution_scale      = 1.0f;
		frame.hdr_enabled           = 0.0f;
		frame.hdr_max_nits          = 1000.0f;
		frame.gamma                 = 2.2f;
		frame.camera_exposure       = 1.0f;
		frame.wind                  = Scene::GetWind();
		frame.cloud_coverage        = 0.0f;
		frame.cloud_shadows         = 0.0f;

		frameBuffer->Update(cmdList, &frame);

		// becomes the "previous" state for the next frame
		frame.view_previous                       = frame.view;
		frame.projection_previous                 = frame.projection;
		frame.view_projection_previous            = frame.view_projection;
		frame.view_projection_previous_unjittered = frame.view_projection_unjittered;
		frame.camera_position_previous            = frame.camera_position;
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
		s_DrawDataRegionStart = m_ResourceIndex * RENDERER_MAX_DRAW_CALLS;

		// Entity matrices are row-vector (translation in the last row); the shaders want the column-vector form
		// stored row-major (see GpuMatrix), which is the transpose in xMath::Matrix's row-major storage.
		ShaderBuffer_DrawData& entry    = m_DrawData_CPU[index];
		entry.transform					= transform.Transposed();
		entry.transform_previous		= prevTransform.Transposed();
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
		static std::array<ShaderBuffer_Material, MAX_MATERIAL_TEXTURES> properties;
		static std::unordered_set<const MaterialAsset*> uniqueMaterials;
		static bool reportedFull = false;
		static uint32_t previousCount = 0; // slots written last time (the tables hold MAX_MATERIAL_TEXTURES entries)
		uint32_t count = 0;
		uint32_t dropped = 0;
		constexpr uint32_t slotsPerMaterial = 9 * MaterialAsset::SLOTS_PER_TEXTURE;

		auto update_material = [&count, &dropped](MaterialAsset* material)
		{
			// keyed by pointer: object ids come from a clock and can repeat for materials made back to back
			if (!uniqueMaterials.insert(material).second)
				return;

			// A full table draws the rest with the first material instead of writing past the arrays
			if (count + slotsPerMaterial > MAX_MATERIAL_TEXTURES)
			{
				material->SetIndex(0);
				++dropped;
				return;
			}
			{

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
				properties[count].flags |= material->GetProperty(MaterialProperty::Terrain)					   ? (1U << 8)  : 0;
				properties[count].flags |= material->GetProperty(MaterialProperty::WindAnimation)              ? (1U << 9)  : 0;
				properties[count].flags |= material->GetProperty(MaterialProperty::ColorVariationFromInstance) ? (1U << 10) : 0;
				properties[count].flags |= material->GetProperty(MaterialProperty::GrassBlades)                ? (1U << 11) : 0;
				properties[count].flags |= material->GetProperty(MaterialProperty::Water)                      ? (1U << 13) : 0;
				properties[count].flags |= material->GetProperty(MaterialProperty::Tessellation)               ? (1U << 14) : 0;
				properties[count].flags |= material->GetProperty(MaterialProperty::EmissiveFromAlbedo)         ? (1U << 15) : 0;
				// keep in sync with Surface struct in common_structs.slang

				properties[count].alpha_cutoff = material->GetProperty(MaterialProperty::AlphaCutoff);
				properties[count].blend_mode   = material->GetProperty(MaterialProperty::BlendMode);
			}
	
			// textures, in the order of the material_texture_index_* slots in resources.slang
			// (albedo, roughness, metalness, normal, occlusion, emission, height, mask, packed)
			static constexpr std::array<MaterialTextureType, 9> shaderTextureSlots =
			{
				MaterialTextureType::Color, MaterialTextureType::Roughness, MaterialTextureType::Metalness,
				MaterialTextureType::Normal, MaterialTextureType::Occlusion, MaterialTextureType::Emission,
				MaterialTextureType::Height, MaterialTextureType::AlphaMask, MaterialTextureType::None // packed occlusion/roughness/metalness
			};
			{
				for (uint32_t type = 0; type < static_cast<uint32_t>(shaderTextureSlots.size()); type++)
				{
					for (uint32_t slot = 0; slot < MaterialAsset::SLOTS_PER_TEXTURE; slot++)
					{
						uint32_t bindlessIndex = count + (type * MaterialAsset::SLOTS_PER_TEXTURE) + slot;
						m_Bindless_Textures[bindlessIndex] = material->GetTexture(shaderTextureSlots[type], slot);
					}
				}
			}
	
			material->SetIndex(count);
	
			count += static_cast<uint32_t>(shaderTextureSlots.size()) * MaterialAsset::SLOTS_PER_TEXTURE;
		};
	
		auto update_entities = [update_material]()
		{
			for (MaterialAsset* material : Scene::GetRenderableMaterials())
				update_material(material);
		};
	
		// cpu
		{
			std::fill_n(properties.begin(), previousCount, ShaderBuffer_Material{});
			std::fill_n(m_Bindless_Textures.begin(), previousCount, nullptr);
			uniqueMaterials.clear();
			update_entities();
			m_Bindless_TextureCount = count;
			previousCount = count;
			if (dropped > 0 && !reportedFull)
			{
				SEDX_CORE_ERROR_TAG("Renderer", "UpdateMaterials: the material table is full ({} materials); {} more draw with the first material",
					count / slotsPerMaterial, dropped);
				reportedFull = true;
			}
			else if (dropped == 0)
				reportedFull = false;
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
				buffer->Update(cmdList, properties.data(), buffer->GetStride() * count);
			}
		}
	}

	void Renderer::UpdateLights(CommandList *cmdList)
	{
		const Entity* cameraEntity = Scene::GetCamera() ? Scene::GetCamera()->GetEntity() : nullptr;
		const Vec3 cameraPos    = cameraEntity ? cameraEntity->GetPosition() : Vec3(0.0f, 0.0f, 0.0f);
	
		// Only the slots written below are uploaded; clearing all MAX_ARRAY_SIZE entries was 8 MB per update.
		const uint32_t previousLightCount = m_Count_ActiveLights;
		m_Count_ActiveLights = 0;  
		Light* firstDirectional = nullptr;
	
		static bool reportedFullLights = false;
		auto fill_light = [&](Light* lightComponent)
		{
			if (m_Count_ActiveLights >= MAX_ARRAY_SIZE)
			{
				// Beyond the light table: leave the light out rather than write past it
				if (!reportedFullLights)
					SEDX_CORE_ERROR_TAG("Renderer", "UpdateLights: more than {} lights; the rest are not drawn", MAX_ARRAY_SIZE);
				reportedFullLights = true;
				return;
			}
			const uint32_t index = m_Count_ActiveLights++;

			lightComponent->SetIndex(index);
			ShaderBuffer_Light& lightBufferEntry = m_Bindless_Lights[index];
			lightBufferEntry = ShaderBuffer_Light();

	
			for (uint32_t i = 0; i < lightComponent->GetSliceCount(); i++)
			{
				lightBufferEntry.view_projection[i] = lightComponent->GetViewProjectionMatrix(i);
			}
	
			lightBufferEntry.screen_space_shadows_slice_index  = lightComponent->GetScreenSpaceShadowsSliceIndex();
			lightBufferEntry.intensity                         = lightComponent->GetIntensityWatt();
			lightBufferEntry.range                             = lightComponent->GetRange();
			lightBufferEntry.angle                             = lightComponent->GetAngle();
			{
				const Standard& lc = lightComponent->GetLightColor();
				lightBufferEntry.color = xMath::Vec4(lc.r, lc.g, lc.b, lc.a);
			}
			// World space from the entity's world matrix (row-vector: row 2 is local +Z, row 3 the translation);
			// Entity::GetPosition / GetForward don't apply parent rotations, which lights in animated groups have.
			{
				const xMath::Matrix world = lightComponent->GetEntity()->GetMatrix();
				lightBufferEntry.position  = Vec3(world.m30, world.m31, world.m32);
				lightBufferEntry.direction = xMath::Normalize(Vec3(world.m20, world.m21, world.m22));
			}
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
		const std::vector<Entity*> &lightEntities = Scene::GetLightEntities();
		for (Entity* entity : lightEntities)
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
					m_Bindless_Lights[0].intensity = 0.0f;

				break;
			}
		}
	
		// remaining lights
		for (Entity* entity : lightEntities)
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

				// Frustum culling is skipped until the CPU camera (Camera) and the GPU camera agree on conventions
				// (same as UpdateDrawCalls); the light's bounding box also ignores parent rotations.
				if (m_Count_ActiveLights >= static_cast<uint32_t>(m_Bindless_Lights.size()))
					break;

				fill_light(lightComponent);
			}
		}

		m_Has_DirectionalLight = firstDirectional != nullptr;

		// slots that held lights last time and are unused now
		for (uint32_t i = m_Count_ActiveLights; i < previousLightCount && i < static_cast<uint32_t>(m_Bindless_Lights.size()); ++i)
			m_Bindless_Lights[i] = ShaderBuffer_Light();

	
		// gpu upload
		Buffer* buffer = GetBuffer(Renderer_Buffer::LightParameters);
		if (!buffer)
		{
			SEDX_CORE_ERROR_TAG("Renderer", "UpdateLights: LightParameters buffer is null — skipping GPU upload");
			return;
		}
		buffer->ResetOffset();

		if (m_Count_ActiveLights > 0)
			buffer->Update(cmdList, m_Bindless_Lights.data(), buffer->GetStride() * m_Count_ActiveLights);
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
		if (uint32_t totalAabbCount = m_DrawCalls_Prepass_Count + m_Indirect_DrawCount; totalAabbCount > 0)
		{
			Buffer* buffer = GetBuffer(Renderer_Buffer::AABBs);
			if (!buffer)
			{
				SEDX_CORE_ERROR_TAG("Renderer", "UpdateBoundingBoxes: AABBs buffer is null — skipping GPU upload");
				return;
			}

			uint32_t frameByteOffset = m_FrameResource_Index * MAX_ARRAY_SIZE * static_cast<uint32_t>(sizeof(ShaderBuffer_Aabb));
			uint32_t uploadSize = static_cast<uint32_t>(sizeof(ShaderBuffer_Aabb)) * totalAabbCount;
			CommandList::UpdateBuffer(buffer, frameByteOffset, uploadSize, m_Bindless_Aabbs.data());
		}
	}

} // namespace SceneryEditorX

// --------------------------------------------------------------

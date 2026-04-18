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
 * renderer.h
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include "renderer_buffers.h"
#include "font/font.h"
#include "vulkan/blend_states.h"
#include "vulkan/command_list.h"
#include "vulkan/command_pool.h"
#include "vulkan/image_resource.h"
#include "vulkan/push_constant_buffer.h"
#include "vulkan/render_context.h"
#include "vulkan/sampler.h"
#include "vulkan/viewport.h"
#include "vulkan/sync/frame_sync.h"
#include <array>
#include <utility>
#include <SceneryEditorX/core/identifiers/flag.h>
#include <SceneryEditorX/core/threading/render_thread.h>
#include <SceneryEditorX/renderer/gpu_stats.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct PersistentLine;
	class MaterialAsset;
	class Mesh;
	enum class MeshType : uint8_t;
	class AssetManager;
	struct RendererProperties;
	class Swapchain;
	class ShaderManager;
	class Pipeline;
	class Camera;

	/**
	 * @struct ShadowSlice
	 * @brief Represents a slice of a shadow map for a specific light source.
	 */
	struct ShadowSlice
	{
		Light* light;
		uint32_t slice_Index;
		uint32_t res;
		xMath::Rectangle rect;
	};

	/**
	 * @brief Static renderer class managing Vulkan rendering lifecycle.
	 * 
	 * The Renderer integrates with the Application main loop, providing:
	 * - Frame synchronization (fences, semaphores)
	 * - Command buffer management
	 * - Swapchain presentation
	 * 
	 * Usage pattern (called by Application::Run):
	 *   Renderer::BeginFrame();  // Acquire swapchain image, wait for fence
	 *   // ... module Tick() calls and rendering ...
	 *   Renderer::EndFrame();    // End command recording
	 *   Renderer::SubmitAndPresent(); // Submit to GPU and present
	 */
	class Renderer 
	{
	public:
		Renderer() = default;
		~Renderer() = default;

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Lifecycle Methods - Called by Application                                                                     ///
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * @brief Initialize the renderer subsystem.
		 * 
		 * Creates Vulkan instance, device, swapchain, command pools, sync objects,
		 * and all per-frame resources. Must be called once before any other Renderer method.
		 */
		static void Init();

		/**
		 * @brief Shutdown the renderer and release all resources.
		 * 
		 * Waits for GPU idle, destroys all Vulkan objects in correct order.
		 * After calling Shutdown, Init must be called again before rendering.
		 */
		static void Shutdown();

		/**
		 * @brief Per-frame tick for memory/resource management.
		 * 
		 * Performs housekeeping tasks like memory allocator updates.
		 * Called once per frame by Application.
		 */
		static void Tick();

		/**
		 * @brief Submit a function to be executed on the render thread.
		 * @tparam FuncT The type of the function to submit.
		 * @param func The function to submit.
		 */
		template<typename FuncT>
		static void Submit(FuncT&& func)
		{
			/*
			auto renderCmd = [](void* ptr) {
				auto pFunc = (FuncT*)ptr;
				(*pFunc)();

				/**
				 * NOTE: Instead of destroying we could try and enforce all items to be trivially destructible
				 * however some items like uniforms which contain std::strings still exist for now
				 * static_assert(std::is_trivially_destructible_v<FuncT>, "FuncT must be trivially destructible");
				 #1#
				pFunc->~FuncT();
			};
			auto storageBuffer = QueueManager::AllocateQueue(renderCmd, sizeof(func));
			new (storageBuffer) FuncT(std::forward<FuncT>(func));
			*/

			// QueueManager::AllocateQueue API is queue-type allocation only.
			// Keep this helper deterministic and compatible by executing the callable inline.
			std::forward<FuncT>(func)();
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Frame Rendering Methods - Called each frame in sequence                                                       ///
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * @brief Begin a new frame.
		 * 
		 * Acquires the next swapchain image, waits for the previous frame's fence,
		 * resets the fence, and begins command buffer recording. This must be called
		 * at the start of each frame before any draw calls.
		 * 
		 * @return true if frame can proceed, false if rendering should be skipped
		 *         (e.g., window minimized or swapchain out of date)
		 */
		static bool BeginFrame();

		/**
		 * @brief End command buffer recording for the current frame.
		 * 
		 * Finalizes command buffer recording and transitions swapchain image
		 * to present layout. Must be called after all draw commands are recorded.
		 */
		static void EndFrame();

		/**
		 * @brief Submit command buffers and present the swapchain image.
		 * 
		 * Submits the recorded command buffer to the graphics queue with proper
		 * synchronization, then presents the image. Handles swapchain recreation
		 * if VK_ERROR_OUT_OF_DATE_KHR is returned.
		 */
		static void SubmitAndPresent();

		/**
		 * @brief Record draw commands to the current frame's command buffer.
		 * @param cmdList Graphics command list for 3D rendering
		 * @param computeCmdList Compute command list (optional, may be nullptr)
		 */
		static void DrawFrame(CommandList *cmdList, CommandList *computeCmdList);

		/**
		 * @brief Blit a texture to the back buffer.
		 * @param cmdList Graphics command list for 3D rendering
		 * @param img Texture to blit
		 */
		static void BlitToBackBuffer(CommandList* cmdList, ImageResource* img);

		/**
		 * @brief Get a render target by type.
		 * @param type The type of render target to retrieve.
		 * @return Pointer to the requested render target.
		 */
		static ImageResource* GetRenderTarget(Renderer_RenderTarget type);

		/**
		 * @brief Write draw data for a mesh instance.
		 * @param transform The current transformation matrix of the mesh.
		 * @param prevTransform The previous transformation matrix of the mesh (for motion blur, etc.).
		 * @param matIndex The index of the material to use.
		 * @param isTransparent Whether the mesh is transparent.
		 * @return Index of the written draw data.
		 */
		static uint32_t WriteDrawData(const xMath::Matrix& transform, const xMath::Matrix &prevTransform = xMath::Matrix::IDENTITY, uint32_t matIndex = 0, uint32_t isTransparent = 0);

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Render Context Management                                                                                     ///
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * @brief Retrieve the global render context instance.
		 * @return Shared reference to RenderContext.
		 */
		static Ref<RenderContext> GetRenderContext();

		/**
		 * @brief Get the current frame-in-flight index (ring buffer slot).
		 * @return Frame index (0 to MAX_FRAMES_IN_FLIGHT-1).
		 */
		static uint32_t GetCurrentFrameIndex();

		/**
		 * @brief Get the total frame number since renderer initialization.
		 * @return Total frame count.
		 */
		static uint64_t GetFrameNumber();

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Render Thread Operations                                                                                      ///
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * @brief Function executed by the render thread.
		 * @param renderThread Pointer to the RenderThread instance.
		 */
		static void RenderThreadFunc(RenderThread* renderThread);

		/**
		 * @brief Wait for the render thread to complete its work and then render the next frame.
		 * @param renderThread Pointer to the RenderThread instance.
		 */
		static void WaitAndRender(RenderThread *renderThread);

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Swapchain Management                                                                                          ///
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * @brief Get the SwapChain instance managed by the Renderer.
		 * @return Pointer to the active SwapChain, or nullptr if not initialized.
		 */
		static Swapchain* GetSwapChain();

		/**
		 * @brief Get the current swapchain image index.
		 * @return Index of the acquired swapchain image.
		 */
		static uint32_t GetSwapchainImageIndex();

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Viewport & Image Management                                                                                   ///
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * @brief Get the current viewport settings.
		 * @return Reference to the current Viewport.
		 */
		static const Viewport &GetViewport();

		/**
		 * @brief Set the current viewport settings.
		 * @param width The width of the viewport.
		 * @param height The height of the viewport.
		 */
		static void SetViewport(float width, float height);

		/**
		 * @brief Get the current renderer resolution.
		 * @return Reference to the current renderer resolution.
		 */
		static const Vec2 &GetRendererResolution();

		/**
		 * @brief Set the renderer resolution.
		 * @param width The width of the renderer resolution.
		 * @param height The height of the renderer resolution.
		 * @param recreateResources Whether to recreate resources after changing the resolution.
		 */
		static void SetRendererResolution(uint32_t width, uint32_t height, bool recreateResources = true);

		/**
		 * @brief Get the current output resolution.
		 * @return Reference to the current output resolution.
		 */
		static const Vec2 &GetOutputResolution();

		/**
		 * @brief Set the output resolution.
		 * @param width The width of the output resolution.
		 * @param height The height of the output resolution.
		 * @param recreateResources Whether to recreate resources after changing the resolution.
		 */
		static void SetOutputResolution(uint32_t width, uint32_t height, bool recreateResources = true);

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Command Buffer Access                                                                                         ///
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * @brief Get the command buffer for the current frame.
		 * @return VkCommandBuffer for recording draw commands.
		 */
		static VkCommandBuffer GetCurrentCommandBuffer();

		/**
		 * @brief Get the active graphics-present command list for the current frame.
		 * @return Pointer to the current present command list, or nullptr if unavailable.
		 */
		static CommandList* GetCommandListPresent();

		/**
		 * @brief Returns nullptr — UI rendering is now performed inside RecordRenderCommands
		 *        via SetExternalRecordingBuffer, so no separate frame command list is needed.
		 * @deprecated Use GetCommandListPresent() with SetExternalRecordingBuffer instead.
		 */
		[[deprecated]] static CommandList* GetCommandListFrame();
		
		/* 
		 * @brief Create shader modules and pipelines. 
		 */
		static void LoadShaders();

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Util Functions																								  ///
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		/* 
		 * @brief Capture a screenshot of the current frame and save it to disk. 
		 */
		static void Screenshot();

		/**
		 * @brief Returns a pointer to the standard mesh for the given type.
		 * Meshes are GPU-resident and available after renderer standard resource initialization.
		 *
		 * @param type The type of standard mesh to retrieve.
		 * @return Pointer to the requested standard mesh.
		 */
		static Mesh* GetStandardMesh(MeshType type);

		/**
		 * @brief Returns a pointer to the standard material for the given type.
		 * @return Pointer to the requested standard material.
		 */
		static Ref<MaterialAsset>& GetStandardMaterial();
		
		/**
		 * @brief Returns the pre-built Font object for the given preset.
		 * @return Reference to the standard font
		 */
		static Ref<Font>& GetFont();

		/* 
		 * @brief Retrieve current GPU memory usage statistics. 
		 * @return GPUMemoryStats struct containing current memory usage details.
		 */
		static GPUMemoryStats GetGPUMemoryStats();

		/**
		 * @brief Set the active camera used to drive view / projection for every frame.
		 * @param camera Raw pointer to a Camera instance; the Renderer does not take ownership.
		 *               Pass nullptr to revert to the identity-matrix fallback.
		 */
		static void SetCamera(Camera* camera);

		/* 
		 * @brief Returns the currently active camera, or nullptr if none has been set. 
		 */
		static Camera* GetCamera();

		/**
		 * @brief Get a structured buffer by type.
		 * @param type The type of buffer to retrieve.
		 * @return Pointer to the requested Buffer.
		 */
		static Buffer *GetBuffer(Renderer_Buffer type);

		/**
		 * @brief Update the camera uniform buffer object (UBO) for the current frame.
		 * @param frameIndex Index of the current frame in flight (0 to MAX_FRAMES_IN_FLIGHT - 1) for double/triple buffering.
		 * @note This should be called once per frame after setting the camera and before recording draw commands.
		 * @note This is used to determine which UBO instance to update in a ring buffer setup. 
		 * @note The camera data should be updated before recording draw commands that use it.
		 */
		static void UpdateCameraUBO(uint32_t frameIndex);

		/**
		 * @brief Update the shadow atlas render target based on current shadow-casting lights and their required resolutions.
		 */
		static void UpdateShadowAtlas();

		/**
		 * @brief Create depth-stencil states used for depth testing and stencil operations in the renderer.
		 */
		static void CreateDepthStencilStates();

		/**
		 * @brief Create rasterizer states used for configuring how polygons are rasterized in the renderer.
		 */
		static void CreateRasterizerStates();

		/**
		 * @brief Create blend states used for configuring how colors are blended in the renderer.
		 */
		static void CreateBlendStates();

		/**
		 * @brief Returns a pointer to the standard texture for the given type.
		 * @param type The type of standard texture to retrieve.
		 * @return Pointer to the requested standard texture.
		 */
		static ImageResource* GetStandardTexture(Renderer_StandardTexture type);

	private:

		/**
		 * @brief Create render targets (swapchain images, depth buffer) based on current swapchain configuration.
		 * @param createRender Whether to create render targets (color/depth) for the swapchain images.
		 * @param createOutput Whether to create output render targets.
		 * @param createDynamic Whether to create dynamic render targets.
		 */
		static void CreateRenderTargets(const bool createRender, const bool createOutput, const bool createDynamic);

		/* 
		 * @brief Update optional render targets based on current renderer configuration. 
		 */
		static void UpdateOptionalRenderTargets();

		/* 
		 * @brief Create per-frame resources such as command buffers and synchronization objects. 
		 */
		static void CreateFrameResources();

		/* 
		 * @brief Destroy per-frame resources such as command buffers and synchronization objects. 
		 * @note This should be called during renderer shutdown and whenever the number of frames in flight changes (e.g., swapchain recreation).
		 */
		static void DestroyFrameResources();

		/**
		 * @brief Record draw commands for the current frame using the provided command lists.
		 * @param cb Command buffer to record into (retrieved via GetCurrentCommandBuffer())
		 * @param imageIndex Index of the swapchain image being rendered to (for resource binding)
		 */
		static void RecordRenderCommands(VkCommandBuffer cb, uint32_t imageIndex);

		/**
		 * @brief Creates all Vulkan samplers used by the renderer.
		 *
		 * Non-anisotropic samplers are created once on first call and guarded by Ref validity.
		 * The anisotropic sampler is recreated whenever the output/render resolution ratio
		 * changes so that the negative mip LOD bias can be updated accordingly.
		 */
		static void CreateSamplers();

		/**
		 * @brief Creates standard meshes used by the renderer.
		 */
		static void CreateStandardMeshes();

		/**
		 * @brief Creates fonts used by the renderer.
		 */
		static void CreateFonts();

		/**
		 * @brief Registers and creates standard renderer shaders through ShaderManager.
		 */
		static void CreateStandardShaders();

		/* 
		 * @brief Creates standard materials used by the renderer. 
		 */
		static void CreateStandardMaterials();

		/* 
		 * @brief Creates standard textures used by the renderer. 
		 */
		static void CreateStandardTextures();

		/**
		 * @brief Creates structured buffers used for per-frame data and other purposes.
		 */
		static void CreateBuffers();

		/**
		 * @brief Get a sampler by type.
		 * @param type The type of sampler to retrieve.
		 * @return Pointer to the requested sampler.
		 */
		static Sampler* GetSampler(Renderer_Sampler type);

		/**
		 * @brief Get a render target by type.
		 * @return Reference to the array of render targets, indexed by Renderer_RenderTarget enum.
		 */
		static std::array<Ref<ImageResource>, static_cast<uint32_t>(Renderer_RenderTarget::MaxEnum)>& GetRenderTargets();

		/**
		 * @brief Get a structured buffer by type.
		 * @return Reference to the array of structured buffers, indexed by Renderer_Buffer enum.
		 */
		static std::array<Ref<Buffer>,  static_cast<uint32_t>(Renderer_Buffer::MaxEnum)>& GetStructuredBuffers();

		/**
		 * @brief Get a sampler by type.
		 * @return Reference to the array of samplers, indexed by Renderer_Sampler enum.
		 */
		static std::array<Ref<Sampler>, static_cast<uint32_t>(Renderer_Sampler::MaxEnum)>& GetSamplers();

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Render Passes                                                                                                 ///
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * @brief Record the render passes for the current frame.
		 * @param graphicsPresent Command list for graphics and presentation operations.
		 * @param compute Command list for compute operations (optional, may be nullptr).
		 */
		static void ProduceFrame(CommandList* graphicsPresent, CommandList* compute);

#pragma region LUT generation passes
		/**
		 * @brief Generate the BRDF specular lookup table.
		 * @param cmdList Command list to record the pass into.
		 */
		static void Pass_Lut_BrdfSpecular(CommandList *cmdList);

		/**
		 * @brief Generate the atmospheric scattering lookup table.
		 * @param cmdList Command list to record the pass into.
		 */
		static void Pass_Lut_AtmosphericScattering(CommandList *cmdList);

#pragma endregion
#pragma region Noise generation passes
		/**
		 * @brief Generate the cloud noise texture.
		 * @param cmdList Command list to record the pass into.
		 */
		static void Pass_CloudNoise(CommandList *cmdList);

#pragma endregion
#pragma region Per-frame Sky & Atmosphere

		/**
		 * @brief Render the skysphere for the current frame.
		 * @param cmdList Command list to record the pass into.
		 */
		static void Pass_Skysphere(CommandList *cmdList);

		/**
		 * @brief Render the cloud shadows for the current frame.
		 * @param cmdList Command list to record the pass into.
		 */
		static void Pass_CloudShadow(CommandList *cmdList);

#pragma endregion
#pragma region Variable - rate shading
		/**
		 * @brief Perform variable-rate shading for the current frame.
		 * @param cmdList Command list to record the pass into.
		 */
		static void Pass_VariableRateShading(CommandList *cmdList);

#pragma endregion
#pragma region Geometry passes

		/**
		 * @brief Generate the hierarchical Z-buffer for the current frame.
		 * @param cmdList Command list to record the pass into.
		 */
		static void Pass_HiZ(CommandList *cmdList);

		/**
		 * @brief Perform indirect culling for the current frame.
		 * @param cmdList Command list to record the pass into.
		 */
		static void Pass_IndirectCull(CommandList *cmdList);

		/**
		 * @brief Perform the depth pre-pass for the current frame.
		 * @param cmdList Command list to record the pass into.
		 */
		static void Pass_Depth_Prepass(CommandList *cmdList);

		/**
		 * @brief Render the G-buffer for the current frame.
		 * @param cmdList Command list to record the pass into.
		 * @param isTransparentPass Indicates whether this is a transparent pass.
		 */
		static void Pass_GBuffer(CommandList *cmdList, const bool isTransparentPass);

#pragma endregion
#pragma region Shadow passes

		/**
		 * @brief Render the shadow maps for the current frame.
		 * @param cmdList Command list to record the pass into.
		 */
		static void Pass_ShadowMaps(CommandList *cmdList);

#pragma endregion
#pragma region Lighting passes

		static void Pass_Light(CommandList *cmdList, const bool isTransparentPass);
		static void Pass_Light_Composition(CommandList *cmdList, const bool isTransparentPass);
		static void Pass_Light_ImageBased(CommandList *cmdList);
		static void Pass_Light_Reflections(CommandList *cmdList);

#pragma endregion
#pragma region Particle passes

		static void Pass_Particles(CommandList *cmdList);

#pragma endregion
#pragma region Transparency

		static void Pass_TransparencyReflectionRefraction(CommandList *cmdList);

#pragma endregion
#pragma region Upscale / Anti-aliasing

		static void Pass_AA_Upscale(CommandList *cmdList);

#pragma endregion
#pragma region Post-processing

		static void Pass_PostProcess(CommandList *cmdList);

#pragma endregion
#pragma region Output / utility
		static void Pass_Output(CommandList *cmdList, ImageResource *in, ImageResource *out);
		static void Pass_Blit(CommandList *cmdList, ImageResource *in, ImageResource *out);
		static void Pass_Downscale(CommandList *cmdList, ImageResource *img, Renderer_DownsampleFilter filter);
		static void Pass_Grid(CommandList *cmdList, ImageResource *out);
		static void Pass_Text(CommandList *cmdList, ImageResource *out);

		template <typename F = std::nullptr_t>
		static void Pass_Compute(CommandList *cmdList, const char *name, Renderer_Shader shaderEnum, ImageResource *in, ImageResource *out, F setup);

#pragma endregion
#pragma region Screen-space effects (async-compute passes)

		static void Pass_ScreenSpaceAO(CommandList *cmdList);
		static void Pass_ScreenSpaceShadows(CommandList *cmdList);

#pragma endregion

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Pipeline State Helpers                                                                                        ///
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * @brief Returns the pre-built RasterizerState object for the given preset.
		 * Objects are created once during renderer init and are never mutated.
		 */
		static RasterizerState *GetRasterizerState(Renderer_RasterizerState type);

		/**
		 * @brief Returns the pre-built BlendState object for the given preset.
		 */
		static BlendState *GetBlendState(Renderer_BlendState type);

		/**
		 * @brief Returns the pre-built DepthStencilState object for the given preset.
		 */
		static DepthStencilState *GetDepthStencilState(Renderer_DepthStencilState type);

		/**
		 * @brief Returns true when the given draw call should be submitted via the CPU-driven path.
		 * GPU-indirect draws are handled separately and should be skipped in CPU loops.
		 */
		static bool IsCpuDrivenDraw(const Renderer_DrawCall &drawCall, const class MaterialAsset *material);

		/**
		 * @brief Rotates the per-frame buffers to avoid CPU-GPU race conditions without stalling.
		 * @note This allows the CPU to write to one buffer while the GPU reads from another, with a safe number of buffers in flight as a cushion.
		 */
		static void RotateFrameBuffers();

		/**
		 * @brief Binds the common per-frame textures (noise, depth, etc.) that every pass needs.
		 */
		static void SetCommonTextures(CommandList *cmdList);

		/**
		 * @brief Writes per-frame data (camera matrices, lighting info, etc.) into the GPU frame-constant buffer.
		 * @param cmdList 
		 */
		static void UpdateFrameConstantBuffer(CommandList *cmdList);

		/**
		 * @brief Updates the draw calls for the current frame.
		 * @param cmdList The command list to record the draw calls into.
		 */
		static void UpdateDrawCalls(CommandList *cmdList);

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Debug Primitives																							  ///
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * @brief Draw a line between two points with specified colors and duration.
		 *
		 * This function adds a line to the renderer's debug draw list. If duration_sec is greater than 0, 
		 * the line will persist for that many seconds; otherwise, it will only be drawn for the current frame.
		 * @param from Starting point of the line in world space.
		 * @param to Ending point of the line in world space.
		 * @param color_from Color at the starting point of the line.
		 * @param color_to Color at the ending point of the line.
		 * @param duration_sec Duration in seconds for which the line should persist. If 0 or less, the line will only be drawn for the current frame.
		 */
		static void DrawLine(const Vec3 &from, const Vec3 &to, const Color &color_from, const Color &color_to, float duration_sec);

		/**
		 * @brief Draw a line between two points with default color and duration.
		 * @param from Starting point of the line in world space.
		 * @param to Ending point of the line in world space.
		 */
		static void DrawLine(const Vec3 &from, const Vec3 &to);

		/**
		 * @brief Draw a line between two points with a specified color and duration.
		 * @param from Starting point of the line in world space.
		 * @param to Ending point of the line in world space.
		 * @param color Color of the line.
		 * @param duration_sec Duration in seconds for which the line should persist. If 0 or less, the line will only be drawn for the current frame.
		 */
		static void DrawLine(const Vec3 &from, const Vec3 &to, const Color &color, float duration_sec);

		/**
		 * @brief Draw a triangle with specified vertices, color, and duration.
		 * @param v0 First vertex of the triangle in world space.
		 * @param v1 Second vertex of the triangle in world space.
		 * @param v2 Third vertex of the triangle in world space.
		 * @param color Color of the triangle (applied to all vertices).
		 * @param duration_sec Duration in seconds for which the triangle should persist. If 0 or less, it will only be drawn for the current frame.
		 */
		static void DrawTriangle(const Vec3 &v0, const Vec3 &v1, const Vec3 &v2, const Color &color, float duration_sec);

		/**
		 * @brief Draw a box with specified bounding box, color, and duration.
		 * @param box The bounding box to draw.
		 * @param color Color of the box.
		 * @param duration_sec Duration in seconds for which the box should persist. If 0 or less, it will only be drawn for the current frame.
		 */
		static void DrawBox(const BoundingBox &box, const Color &color, float duration_sec);

		/**
		 * @brief Draw a circle with specified center, axis, radius, segment count, color, and duration.
		 * @param center Center of the circle in world space.
		 * @param axis Normal vector of the circle's plane.
		 * @param radius Radius of the circle.
		 * @param segment_count Number of segments to approximate the circle.
		 * @param color Color of the circle.
		 * @param duration_sec Duration in seconds for which the circle should persist. If 0 or less, it will only be drawn for the current frame.
		 */
		static void DrawCircle(const Vec3 &center, const Vec3 &axis, float radius, uint32_t segment_count, const Color &color, float duration_sec);

		/**
		 * @brief Draw a sphere with specified center, radius, segment count, color, and duration.
		 * @param center Center of the sphere in world space.
		 * @param radius Radius of the sphere.
		 * @param segment_count Number of segments to approximate the sphere.
		 * @param color Color of the sphere.
		 * @param duration_sec Duration in seconds for which the sphere should persist. If 0 or less, it will only be drawn for the current frame.
		 */
		static void DrawSphere(const Vec3 &center, float radius, uint32_t segment_count, const Color &color, float duration_sec);

		/**
		 * @brief Draw a directional arrow with specified start and end points, arrow size, color, and duration.
		 * @param start Starting point of the arrow in world space.
		 * @param end Ending point of the arrow in world space.
		 * @param arrow_size Size of the arrowhead.
		 * @param color Color of the arrow.
		 * @param duration_sec Duration in seconds for which the arrow should persist. If 0 or less, it will only be drawn for the current frame.
		 */
		static void DrawDirectionalArrow(const Vec3 &start, const Vec3 &end, float arrow_size, const Color &color, float duration_sec);

		/**
		 * @brief Draw a plane with specified plane equation, color, and duration.
		 * @param plane The plane to draw.
		 * @param color Color of the plane.
		 * @param duration_sec Duration in seconds for which the plane should persist. If 0 or less, it will only be drawn for the current frame.
		 */
		static void DrawPlane(const xMath::Plane &plane, const Color &color, float duration_sec);

		/**
		 * @brief Update the persistent lines, removing any that have expired.
		 */
		static void UpdatePersistentLines();

		/**
		 * @brief Add lines to be rendered for the current frame.
		 */
		static void AddLinesToBeRendered();

		// line and icon rendering
		static Ref<Buffer> m_Lines_VertexBuffer;
		static std::vector<Vertex_PosCol> m_Lines_Vertices;
		static std::vector<PersistentLine> m_Persistent_Lines;
		static std::vector<std::tuple<ImageResource*, xMath::Vec3>> m_Icons;

		// -------------------------------------------------------

		/**
		 * @brief Update materials for the current frame.
		 * @param cmdList Command list to record the update commands.
		 */
		static void UpdateMaterials(CommandList* cmdList);

		/**
		 * @brief Update lights for the current frame.
		 * @param cmdList Command list to record the update commands.
		 */
		static void UpdateLights(CommandList* cmdList);

		/**
		 * @brief Update bounding boxes for the current frame.
		 * @param cmdList Command list to record the update commands.
		 */
		static void UpdateBoundingBoxes(CommandList* cmdList);

		CommandList *m_CurrentCmdList; // Set at the beginning of each frame, used for resource updates and utility functions.
		AssetManager *m_AssetManager;  // Set during Init, used for loading models, textures, etc.

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Static State																								  ///
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		static RendererProperties *m_Data;
		static std::atomic<bool> m_ResourcesInitialized; // Flag to indicate when resources are ready for use, set to true at the end of Init()

		// Bindless
		// bindless draw data
		static std::array<ShaderBuffer_DrawData, RENDERER_MAX_DRAW_CALLS> m_DrawData_CPU; // Staging area for draw data written by the CPU; copied to GPU buffer each frame
		static std::mutex m_MutexRenderables; // Mutex to protect access to m_DrawData_CPU and m_DrawData_Count during scene submission from multiple threads
		static uint32_t m_DrawData_Count; // Number of draw data entries written for the current frame; used to determine how many to copy to GPU and how many draw calls to issue

		// Array of pointers to all textures used by the renderer, indexed by material parameters; bound as a bindless array in shaders
		static std::array<ImageResource*, MAX_ARRAY_SIZE> m_Bindless_Textures;

		// Array of light data for all active lights in the scene, indexed by a per-light index; bound as a bindless array in shaders
		static std::array<ShaderBuffer_Light, MAX_ARRAY_SIZE> m_Bindless_Lights; 

		// Array of AABB data for all renderables, indexed by a per-renderable index; used for GPU-driven culling and other operations
		static std::array<ShaderBuffer_Aabb, MAX_ARRAY_SIZE> m_Bindless_Aabbs;

		static Flag m_BindlessSamplers_Dirty; // Flag to indicate when bindless samplers need to be updated in shaders, set whenever sampler states change (e.g., anisotropy level changes)

		/**
		 * @struct PassState
		 * @brief Tracks one-shot initialization and feature-toggle state for various render passes and resources. 
		 */
		struct PassState
		{
			// one-shot initialization (run once, never again unless reset)
			bool m_BRDF_LutProduced        = false;
			bool m_Atmosphere_LutProduced  = false;
			bool m_CloudNoiseProduced      = false;

			// feature-toggle clear flags (set when feature disabled, reset when re-enabled)
			bool m_ClearedReflections     = false;
			bool m_ClearedRtReflections   = false;
			bool m_ClearedRtShadows       = false;
			bool m_ClearedRestir          = false;

			// skysphere convergence tracking
			bool     m_SkyFirstFrame           = true;
			bool     m_SkyHadDirectionalLight  = false;
			float    m_SkyLastCoverage         = -1.0f;
			uint32_t m_SkyFramesRemaining      = 0;

			// vrs
			ImageResource* m_VrsLastClearedTexture = nullptr;
			void Reset() { *this = PassState(); }

		};

		static PassState m_PassState; // Tracks one-shot initialization and feature-toggle state for various render passes and resources; used to conditionally execute certain passes or initialization steps
		static PushConstantBuffer_Pass m_Pcb_Pass_Cpu; // Per-pass push constant staging buffer (written by passes, uploaded by PushConstants())
		static ConstantBuffer_Frame m_Cb_Frame_Cpu; // Staging area for per-frame constants written by the CPU; copied to GPU buffer each frame

		// CPU-side draw call arrays (populated by scene submission, consumed by passes)
		static std::array<Renderer_DrawCall, RENDERER_MAX_DRAW_CALLS> m_DrawCalls;
		static std::array<Renderer_DrawCall, RENDERER_MAX_DRAW_CALLS> m_DrawCalls_Prepass;
		static uint32_t m_DrawCall_Count;
		static uint32_t m_DrawCalls_Prepass_Count;
		static uint32_t m_Indirect_DrawCount;
		static bool     m_Transparents_Present;
		static bool     m_Is_Hiz_Suppressed;

		static CommandList *m_CmdList_Compute;
		static CommandList *m_CmdList_Present;
		// @brief Kept as nullptr — UI is now rendered via SetExternalRecordingBuffer inside RecordRenderCommands.
		static CommandList *m_CmdList_Frame;
		static Scope<AssetManager> s_AssetManager;
		static uint32_t m_ResourceIndex;

		// Frame synchronization
		static Scope<FrameSync> m_FrameSync;
		static Scope<CommandPool> m_CommandPool;
		static std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> m_CommandBuffers;

		// Frame tracking
		static uint32_t m_CurrentFrameIndex;     // Ring buffer index (0 to MAX_FRAMES_IN_FLIGHT-1)
		static uint64_t m_FrameNumber;           // Total frames rendered
		static uint32_t m_SwapchainImageIndex;   // Current swapchain image
		static bool m_FrameInProgress;           // True between BeginFrame and EndFrame

		/**
		 * @struct IndirectFrameResource
		 * @brief Holds per-frame GPU resources to avoid race conditions between in-flight frames.
		 */
		struct IndirectFrameResource
		{
			Ref<Buffer> m_DrawArgs;
			Ref<Buffer> m_DrawData;
			Ref<Buffer> m_DrawArgs_Out;
			Ref<Buffer> m_DrawData_Out;
			Ref<Buffer> m_DrawCount;
		};
		static std::array<IndirectFrameResource, DRAW_DATA_BUFFER_COUNT> m_FrameResources;
		static uint32_t m_FrameResource_Index;
		static std::array<ShaderBuffer_IndirectDrawArgs, MAX_ARRAY_SIZE> m_Indirect_DrawArgs;
		static std::array<ShaderBuffer_DrawData, MAX_ARRAY_SIZE> m_Indirect_DrawData;
		static std::vector<ShadowSlice> m_ShadowSlices;

		/* Active camera providing view / projection for every frame */
		static Camera* m_Camera;
		static xMath::Frustum m_Frustum;

		/* Per-frame camera uniform buffers (host-visible, mapped), matching CameraShaderData */
		static VkDescriptorSetLayout m_CameraDescriptorSetLayout;
		static VkDescriptorPool m_CameraDescriptorPool;
		static std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> m_CameraDescriptorSets;
		static std::array<VkBuffer, MAX_FRAMES_IN_FLIGHT> m_CameraUboBuffers;
		static std::array<VmaAllocation, MAX_FRAMES_IN_FLIGHT> m_CameraUboAllocations;
		static std::array<void*, MAX_FRAMES_IN_FLIGHT> m_CameraUboMapped;

		static uint32_t m_Count_ActiveLights;

		/* 
		 * @brief Creates per-frame camera UBOs, descriptor pool and sets. 
		 */
		static void CreateCameraResources();
	};

}

/**
 * NOTE: Changing from deferred queue-based execution to immediate inline execution
 * fundamentally alters the threading model and removes the render command queue mechanism.
 * This breaks the intended design of deferring render commands to a dedicated render thread.
 * 
 * If QueueManager is being removed or refactored, this change should be clearly documented
 * and all callers should be audited to ensure thread safety.
 * 
 * TODO: If this is a temporary workaround, document the expected final solution and ensure
 * that all render command invocations are thread-safe or only called from the render thread.
 */

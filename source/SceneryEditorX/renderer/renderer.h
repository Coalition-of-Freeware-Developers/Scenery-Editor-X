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
#include "renderer_declarations.h"
#include "font/font.h"
#include "vulkan/blend_states.h"
#include "vulkan/command_list.h"
#include "vulkan/command_pool.h"
#include "vulkan/depth_stencil.h"
#include "vulkan/image_resource.h"
#include "vulkan/push_constant_buffer.h"
#include "vulkan/rasterizer.h"
#include "vulkan/render_context.h"
#include "vulkan/sampler.h"
#include "vulkan/viewport.h"
#include "vulkan/sync/frame_sync.h"
#include <array>
#include <SceneryEditorX/asset/model.h>
#include <SceneryEditorX/core/threading/render_thread.h>
#include <SceneryEditorX/core/window/window.h>
#include <SceneryEditorX/renderer/gpu_stats.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class MaterialAsset;
	class Mesh;
	enum class MeshType : uint8_t;
	class AssetManager;
	struct RendererProperties;
	class Swapchain;
	class ShaderManager;
	class Camera;

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
			auto renderCmd = [](void* ptr) {
				auto pFunc = (FuncT*)ptr;
				(*pFunc)();

				// NOTE: Instead of destroying we could try and enforce all items to be trivally destructible
				// however some items like uniforms which contain std::strings still exist for now
				// static_assert(std::is_trivially_destructible_v<FuncT>, "FuncT must be trivially destructible");
				pFunc->~FuncT();
			};
			auto storageBuffer = QueueManager::AllocateQueue(renderCmd, sizeof(func));
			new (storageBuffer) FuncT(std::forward<FuncT>(func));
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
		 * @param texture Texture to blit
		 */
		static void BlitToBackBuffer(CommandList *cmdList, ImageResource *texture);

		/**
		 * @brief Get a render target by type.
		 * @param type The type of render target to retrieve.
		 * @return Pointer to the requested render target.
		 */
		static ImageResource *GetRenderTarget(Renderer_RenderTarget type);

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
		static Swapchain *GetSwapChain();

		/**
		 * @brief Get the current swapchain image index.
		 * @return Index of the acquired swapchain image.
		 */
		static uint32_t GetSwapchainImageIndex();

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Viewport & Image Management                                                                                   ///
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		static const Viewport &GetViewport();
		static void SetViewport(float width, float height);

		// Resolution Render
		static const Vec2 &GetRendererResolution();
		static void SetRendererResolution(uint32_t width, uint32_t height, bool recreateResources = true);

		// Resolution Output
		static const Vec2 &GetOutputResolution();
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
		 * @brief Create models and upload to GPU. This is separate from shader creation to allow for better error handling and resource management.
		 */
		static void CreateModels();

		/* @brief Create shader modules and pipelines. */
		static void CreateShaders();

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Util Functions																								  ///
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		static void Screenshot();

		/**
		 * @brief Returns a pointer to the standard mesh for the given type.
		 * Meshes are GPU-resident and available after CreateModels().
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

		/**
		 * @brief Returns the currently active camera, or nullptr if none has been set.
		 */
		static Camera* GetCamera();

		/**
		 * @brief Update the camera uniform buffer object (UBO) for the current frame.
		 * @param frameIndex Index of the current frame in flight (0 to MAX_FRAMES_IN_FLIGHT - 1) for double/triple buffering.
		 * @note This should be called once per frame after setting the camera and before recording draw commands.
		 * @note This is used to determine which UBO instance to update in a ring buffer setup. 
		 * @note The camera data should be updated before recording draw commands that use it.
		 */
		static void UpdateCameraUBO(uint32_t frameIndex);

	private:

		/**
		 * @brief Create render targets (swapchain images, depth buffer) based on current swapchain configuration.
		 * @param createRender Whether to create render targets (color/depth) for the swapchain images.
		 * @param createOutput Whether to create output render targets.
		 * @param createDynamic Whether to create dynamic render targets.
		 */
		static void CreateRenderTargets(const bool createRender, const bool createOutput, const bool createDynamic);

		/* @brief Update optional render targets based on current renderer configuration. */
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
		 * @brief Get a structured buffer by type.
		 * @param type The type of buffer to retrieve.
		 * @return Pointer to the requested Buffer.
		 */
		static Buffer *GetBuffer(Renderer_Buffer type);

		/**
		 * @brief Get a shader by type.
		 * @param type The type of shader to retrieve.
		 * @return Pointer to the requested shader.
		 */
		static Shader *GetShader(Renderer_Shader type);

		/**
		 * @brief Marks a shader as available, creating it if it doesn't already exist.
		 * @param type The type of shader to mark as available.
		 */
		static void SetShaderAvailable(Renderer_Shader type);

		/**
		 * @brief Creates all Vulkan samplers used by the renderer.
		 *
		 * Non-anisotropic samplers are created once on first call and guarded by Ref validity.
		 * The anisotropic sampler is recreated whenever the output/render resolution ratio
		 * changes so that the negative mip LOD bias can be updated accordingly.
		 */
		static void CreateSamplers();

		/**
		 * @brief Creates standard materials used by the renderer.
		 */
		static void CreateStandardMaterials();

		static void CreateStandardTextures();

		/**
		 * @brief Get a sampler by type.
		 * @param type The type of sampler to retrieve.
		 * @return Pointer to the requested sampler.
		 */
		static Sampler *GetSampler(Renderer_Sampler type);

		static std::array<Ref<ImageResource>, static_cast<uint32_t>(Renderer_RenderTarget::MaxEnum)>& GetRenderTargets();
		static std::array<Ref<Shader>,  static_cast<uint32_t>(Renderer_Shader::MaxEnum)>& GetShaders();
		static std::array<Ref<Buffer>,  static_cast<uint32_t>(Renderer_Buffer::MaxEnum)>& GetStructuredBuffers();
		static std::array<Ref<Sampler>, static_cast<uint32_t>(Renderer_Sampler::MaxEnum)>& GetSamplers();

		static ImageResource *GetStandardTexture(Renderer_StandardTexture type);

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Render Passes                                                                                                 ///
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		static void ProduceFrame(CommandList* graphicsPresent, CommandList* compute);

		// One-shot LUT generation passes
		static void Pass_Lut_BrdfSpecular(CommandList *cmdList);
		static void Pass_Lut_AtmosphericScattering(CommandList *cmdList);

		// One-shot noise generation passes
		static void Pass_CloudNoise(CommandList *cmdList);

		// Per-frame sky and atmosphere
		static void Pass_Skysphere(CommandList *cmdList);
		static void Pass_CloudShadow(CommandList *cmdList);

		// Variable-rate shading
		static void Pass_VariableRateShading(CommandList *cmdList);

		// Geometry passes
		static void Pass_HiZ(CommandList *cmdList);
		static void Pass_IndirectCull(CommandList *cmdList);
		static void Pass_Depth_Prepass(CommandList *cmdList);
		static void Pass_GBuffer(CommandList *cmdList, const bool isTransparentPass);

		// Shadow passes
		static void Pass_ShadowMaps(CommandList *cmdList);

		// Lighting passes
		static void Pass_Light(CommandList *cmdList, const bool isTransparentPass);
		static void Pass_Light_Composition(CommandList *cmdList, const bool isTransparentPass);
		static void Pass_Light_ImageBased(CommandList *cmdList);
		static void Pass_Light_Reflections(CommandList *cmdList);

		// Particles
		static void Pass_Particles(CommandList *cmdList);

		// Transparency
		static void Pass_TransparencyReflectionRefraction(CommandList *cmdList);

		// Upscale / Anti-aliasing
		static void Pass_AA_Upscale(CommandList *cmdList);

		// Post-processing
		static void Pass_PostProcess(CommandList *cmdList);

		// Output / utility
		static void Pass_Output(CommandList *cmdList, ImageResource *in, ImageResource *out);
		static void Pass_Blit(CommandList *cmdList, ImageResource *in, ImageResource *out);
		static void Pass_Downscale(CommandList *cmdList, ImageResource *img, Renderer_DownsampleFilter filter);
		static void Pass_Grid(CommandList *cmdList, ImageResource *out);
		static void Pass_Text(CommandList *cmdList, ImageResource *out);

		template <typename F = std::nullptr_t>
		static void Pass_Compute(CommandList *cmdList, const char *name, Renderer_Shader shaderEnum, ImageResource *in, ImageResource *out, F setup);

		// Screen-space effects (async-compute passes)
		static void Pass_ScreenSpaceAO(CommandList *cmdList);
		static void Pass_ScreenSpaceShadows(CommandList *cmdList);

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Pipeline State Helpers                                                                                        ///
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * @brief Returns the pre-built RasterizerState object for the given preset.
		 * Objects are created once during renderer init and are never mutated.
		 */
		static RasterizerState* GetRasterizerState(Renderer_RasterizerState type);

		/**
		 * @brief Returns the pre-built BlendState object for the given preset.
		 */
		static BlendState* GetBlendState(Renderer_BlendState type);

		/**
		 * @brief Returns the pre-built DepthStencilState object for the given preset.
		 */
		static DepthStencilState* GetDepthStencilState(Renderer_DepthStencilState type);

		/**
		 * @brief Writes per-draw transform and material data into the GPU draw-data buffer.
		 * @return Index of the written draw-data slot (passed as push constant draw_index).
		 */
		static uint32_t WriteDrawData(const xMath::Matrix& transform);

		/**
		 * @brief Returns true when the given draw call should be submitted via the CPU-driven path.
		 * GPU-indirect draws are handled separately and should be skipped in CPU loops.
		 */
		static bool IsCpuDrivenDraw(const struct Renderer_DrawCall& drawCall, const class Material* material);

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
		 * @brief Writes per-draw transform and material data into the GPU draw-data buffer.
		 * @param transform Current frame transform matrix
		 * @param prevTransform Previous frame transform matrix
		 * @param matIdx Material index
		 * @param isTransparent Flag indicating if the draw call is transparent
		 * @return Index of the written draw-data slot (passed as push constant draw_index).
		 */
		static uint32_t WriteDrawData(const xMath::Matrix &transform, const xMath::Matrix &prevTransform, uint32_t matIdx, uint32_t isTransparent);

		/**
		 * @brief 
		 * @param cmdList 
		 */
		static void UpdateDrawCalls(CommandList *cmdList);

		// -------------------------------------------------------
		
		CommandList *m_CurrentCmdList;
		AssetManager *m_AssetManager;

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Static State																								  ///
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		static RendererProperties *m_Data;
		static std::atomic<bool> m_ResourcesInitialized;
		static Scope<Model> m_TestModel;

		// Bindless
		static std::array<ImageResource*, MAX_ARRAY_SIZE> m_Bindless_Textures;
		//static std::array<Sb_Light, MAX_ARRAY_SIZE> m_Bindless_Lights;
		//static std::array<Sb_Aabb, MAX_ARRAY_SIZE> m_Bindless_Aabbs;
		static bool m_BindlessSamplers_Dirty;

		// one-shot and feature-toggle state
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
			void Reset()
			{
				*this = PassState();
			}

		};
		static PassState m_PassState;

		// Per-pass push constant staging buffer (written by passes, uploaded by PushConstants())
		static PushConstantBuffer_Pass m_Pcb_Pass_Cpu;

		// CPU-side draw call arrays (populated by scene submission, consumed by passes)
		static std::array<Renderer_DrawCall, RENDERER_MAX_DRAW_CALLS> m_DrawCalls;
		static std::array<Renderer_DrawCall, RENDERER_MAX_DRAW_CALLS> m_DrawCalls_Prepass;
		static uint32_t m_DrawCall_Count;
		static uint32_t m_DrawCalls_Prepass_Count;
		static uint32_t m_Indirect_DrawCount;
		static bool     m_Transparents_Present;
		static bool     m_Is_Hiz_Suppressed;

		// CPU-Side draw data staging
		//static std::array<Sb_DrawData, renderer_max_draw_calls> m_DrawData_CPU;
		static uint32_t m_DrawDataCount;
		static std::mutex m_MutexRenderables;

		static CommandList *m_CmdList_Compute;
		static CommandList *m_CmdList_Present;
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

		/* Basic forward-rendering pipeline (active until the full deferred pipeline is wired up) */
		static VkPipeline m_BasicPipeline;
		static VkPipelineLayout m_BasicPipelineLayout;
		static Scope<ShaderManager> m_BasicShaderManager;
		static std::array<VkBuffer,        MAX_FRAMES_IN_FLIGHT> m_BasicShaderDataBuffers;
		static std::array<VmaAllocation,   MAX_FRAMES_IN_FLIGHT> m_BasicShaderDataAllocations;
		static std::array<void*,           MAX_FRAMES_IN_FLIGHT> m_BasicShaderDataMapped;
		static std::array<VkDeviceAddress, MAX_FRAMES_IN_FLIGHT> m_BasicShaderDataAddresses;

		/* Infinite grid bootstrap pipeline and geometry */
		static VkPipeline m_GridPipeline;
		static VkPipelineLayout m_GridPipelineLayout;
		static Scope<ShaderManager> m_GridShaderManager;
		static VkBuffer m_GridVertexBuffer;
		static VmaAllocation m_GridVertexAllocation;
		static VkBuffer m_GridIndexBuffer;
		static VmaAllocation m_GridIndexAllocation;
		static uint32_t m_GridIndexCount;


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

		/* @brief Creates per-frame camera UBOs, descriptor pool and sets. Called by CreateShaders. */
		static void CreateCameraResources();
	};


}

// -------------------------------------------------------

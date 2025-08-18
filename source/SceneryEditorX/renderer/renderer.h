/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* renderer.h
* -------------------------------------------------------
* Created: 7/6/2025
* -------------------------------------------------------
*/
#pragma once
#include "command_queue.h"
#include "compute_pass.h"
#include "SceneryEditorX/core/application/application.h"
#include "SceneryEditorX/core/threading/render_thread.h"
#include "SceneryEditorX/scene/material.h"
#include "SceneryEditorX/scene/scene.h"

#include "vulkan/vk_cmd_buffers.h"
#include "vulkan/vk_render_pass.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{

    /// Forward declaration to break circular dependency
    class RenderContext;

    /**
     * @class Renderer
     * @brief Central static façade for all high-level rendering operations.
     *
     * The Renderer class manages frame lifecycle, command submission, shader hot-reload
     * propagation, descriptor and sampler utilities, render/compute pass orchestration,
     * and deferred resource destruction on the render thread. Most functions are exposed
     * as GLOBAL (static) entry points to allow easy invocation across systems while
     * ensuring ordered execution on the render thread where required.
     *
     * Core Responsibilities:
     *  - Frame begin/end and swapchain presentation sequencing
     *  - Thread-safe deferred command submission through a linear command queue
     *  - Render resource lifetime management (safe destruction after GPU usage)
     *  - Shader dependency tracking for pipelines/materials and hot-reload propagation
     *  - Utility functions for descriptor set allocation and sampler creation
     *  - Dispatch and management of render & compute passes
     *
     * Threading Model:
     *  - CPU/game logic threads enqueue work via Submit()/SubmitResourceFree()
     *  - The render thread drains command queues in a deterministic order
     *  - Resource destruction is deferred per-frame to avoid GPU-use-after-free
     *
     * @note All functions marked GLOBAL are static; the class behaves as a singleton façade.
     */
    class Renderer
	{
	public:
        /**
         * @typedef RenderCommandFn
         * @brief Function pointer signature used to execute a recorded render command.
         *
         * Each submitted lambda/functor is placement-new constructed into a transient
         * linear buffer inside a CommandQueue. When executed, the queue invokes the
         * associated RenderCommandFn passing the pointer to the stored callable object.
         * The callable is then manually destroyed (unless trivially destructible).
         *
         * @param void* Opaque pointer to the callable object storage.
         *
         * @note: This function pointer type is used internally by the Renderer to
         *       execute submitted commands on the render thread.
         *
         * @code
         * Renderer::RenderCommandFn cmdFn = [](void* ptr)
         * {
         *	auto pFunc = (FuncT*)ptr;
         *	(*pFunc)(); // Call the submitted function
         *	pFunc->~FuncT(); // Explicitly destroy the callable
         * };
         * @endcode
         */
        typedef void (*RenderCommandFn)(void *);

		/**
		 * @brief Retrieve the global render context instance.
		 * @return Shared reference to RenderContext.
		 */
		GLOBAL Ref<RenderContext> GetContext();

        /// -------------------------------------------------------

        GLOBAL void Init();
		GLOBAL void Shutdown();
        GLOBAL void BeginFrame();
        GLOBAL void EndFrame();
        GLOBAL void SubmitFrame();

        /// -------------------------------------------------------

        /**
         * @brief Submit a callable to execute on the render thread command queue.
         *
         * The callable is copied (or moved) into a linear transient buffer; execution
         * occurs later on the render thread. The callable is explicitly destroyed
         * after invocation to permit non-trivially-destructible closures (e.g. with
         * std::string members).
         *
         * @tparam FuncT Callable type (lambda/functor) - must be invocable with ().
         * @param func Callable instance (forwarded).
         *
         * @note Avoid capturing large objects by value; prefer lightweight handles.
         * @threadsafe Yes.
         */
        template<typename FuncT>
		static void Submit(FuncT&& func)
		{
			auto renderCmd = [](void* ptr)
            {
				auto pFunc = (FuncT*)ptr;
				(*pFunc)();

				/**
				 * @note: Instead of destroying we could try and enforce all items to be trivally destructible
				 * however some items like uniforms which contain std::strings still exist for now.
				 * static_assert(std::is_trivially_destructible_v<FuncT>, "FuncT must be trivially destructible");
 				 */
				pFunc->~FuncT();
			};
			auto storageBuffer = GetCommandQueue().Allocate(renderCmd, sizeof(func));
			new (storageBuffer) FuncT(std::forward<FuncT>(func));
		}

        /// -------------------------------------------------------

        /**
		 * @brief Submit a callable that frees GPU resources at a safe time.
		 *
		 * If called from the render thread the resource-free command is enqueued
		 * directly into the current frame's release queue; otherwise it is marshalled
		 * via Submit() to ensure correct thread context.
		 *
		 * Use this for destroying Vulkan objects / buffers / images that must survive
		 * until the GPU finishes referencing them.
		 *
		 * @tparam FuncT Callable type.
		 * @param func Callable responsible for performing resource destruction.
		 */
		template<typename FuncT>
		static void SubmitResourceFree(FuncT&& func)
		{
			auto renderCmd = [](void* ptr)
            {
				auto pFunc = (FuncT*)ptr;
				(*pFunc)();

				/**
				 * @note: Instead of destroying we could try and enforce all items to be trivally destructible
				 * however some items like uniforms which contain std::strings still exist for now.
				 * static_assert(std::is_trivially_destructible_v<FuncT>, "FuncT must be trivially destructible");
				 */
				pFunc->~FuncT();
			};

			if (RenderThread::IsCurrentThreadRT())
			{
				const uint64_t index = GetCurrentFrameIndex();
				auto storageBuffer = GetRenderResourceReleaseQueue(index).Allocate(renderCmd, sizeof(func));
				new (storageBuffer) FuncT(std::forward<FuncT>((FuncT&&)func));
			}
			else
			{
				const uint64_t index = GetCurrentFrameIndex();
				Submit([renderCmd, func, index]()
				{
					auto storageBuffer = GetRenderResourceReleaseQueue(index).Allocate(renderCmd, sizeof(func));
					new (storageBuffer) FuncT(std::forward<FuncT>((FuncT&&)func));
				});
			}
		}

        /// -------------------------------------------------------

        /**
        * @brief Access current frame's aggregated immutable render data.
        * @return Reference to the active RenderData structure.
        */
        GLOBAL RenderData &GetRenderData();

        /**
        * @brief Replace the active frame RenderData.
        * @param renderData New data snapshot copied into internal storage.
        */
        GLOBAL void SetRenderData(const RenderData &renderData);

        /**
        * @brief Entry point executed by the render thread main loop.
        * @param renderThread Pointer to the owning RenderThread.
        */
        GLOBAL void RenderThreadFunc(RenderThread *renderThread);

        /**
        * @brief Wait for required synchronization then perform rendering.
        * @param renderThread Pointer to render thread context.
        */
        GLOBAL void WaitAndRender(RenderThread *renderThread);

        /**
        * @brief Swap front/back command queues to prepare for next frame submission.
        *
        * Usually invoked once per frame during SubmitFrame().
        */
        GLOBAL void SwapQueues();

        /// -------------------------------------------------------

        /**
		 * @brief Get the active swapchain instance.
		 * @return Pointer to swapchain (could be null before Init()).
		 */
		GLOBAL SwapChain *GetSwapChain();

        /// -------------------------------------------------------

        /**
         * @brief Get the graphics queue family index used for primary rendering.
         * @return Queue family index.
         */
        GLOBAL uint32_t GetRenderQueueIndex();

        /**
         * @brief Get per-frame submission counter (monotonically increasing).
         * @return Submission index.
         */
        GLOBAL uint32_t GetRenderQueueSubmissionIndex();

        /**
         * @brief Get the current frame-in-flight index (ring buffer slot).
         * @return Frame index.
         */
        GLOBAL uint64_t GetCurrentFrameIndex();

        /// -------------------------------------------------------

        /**
         * @brief Build a VkDescriptorSetAllocateInfo helper struct.
         *
         * @param layouts Pointer to first layout in an array.
         * @param count Number of descriptor sets to allocate.
         * @param pool Optional explicit pool override.
         * @return Populated VkDescriptorSetAllocateInfo ready for vkAllocateDescriptorSets.
         */
        VkDescriptorSetAllocateInfo DescriptorSetAllocInfo(const VkDescriptorSetLayout* layouts, uint32_t count = 1, VkDescriptorPool pool = nullptr);

        /// -------------------------------------------------------

        /**
         * @brief Create a Vulkan sampler object.
         * @param samplerCreateInfo Mutable create info (fields may be adjusted internally).
         * @return Created VkSampler handle.
         */
        GLOBAL VkSampler CreateSampler(VkSamplerCreateInfo &samplerCreateInfo);

        /**
         * @brief Destroy a Vulkan sampler previously created via CreateSampler().
         * @param sampler Sampler handle to destroy (ignored if VK_NULL_HANDLE).
         */
        GLOBAL void DestroySampler(VkSampler sampler);

        /// -------------------------------------------------------

		/**
		 * @brief Access the global shader library.
		 * @return Shared reference to ShaderLibrary.
		 */
		GLOBAL Ref<ShaderLibrary> GetShaderLibrary();

		/**
		 * @brief Get the per-frame resource release queue (deferred destruction).
		 * @param index Frame index slot.
		 * @return Reference to CommandQueue for that frame.
		 */
		GLOBAL CommandQueue &GetRenderResourceReleaseQueue(uint32_t index);
		
		/// -------------------------------------------------------
		
		/**
		 * @brief Get the current frame index as seen from the render thread (may differ from CPU).
		 * @return Render thread frame index.
		 */
		GLOBAL uint32_t GetCurrentRenderThreadFrameIndex();
		
		/**
		 * @brief Number of descriptor allocations performed this frame (for diagnostics).
		 * @param frameIndex Optional frame override (0 = current).
		 * @return Allocation count.
		 */
		GLOBAL uint32_t GetDescriptorAllocationCount(uint32_t frameIndex = 0);
		
		/**
		 * @brief Submit a fullscreen triangle/quad draw call with provided pipeline & material.
		 * @param ref Active command buffer reference.
		 * @param pipeline Graphics pipeline to bind.
		 * @param material Material providing descriptor sets / push constants.
		 */
		GLOBAL void SubmitFullscreenQuad(Ref<CommandBuffer> & ref, Ref<Pipeline> & pipeline, Ref<Material> & material);
		
		/**
		 * @brief Capture a screenshot of the current swapchain image.
		 * @param file_path Output file path (extension influences format if format empty).
		 * @param immediateDispatch If true, executes immediately instead of deferred.
		 * @param format Image format string ("png", "jpg", etc.).
		 */
		GLOBAL void Screenshot(const std::string &file_path, bool immediateDispatch = false, std::string format = "png");
		
		/**
		 * @brief Get a cached 1x1 white texture (utility).
		 * @return Shared reference to Texture2D.
		 */
		GLOBAL Ref<Texture2D> GetWhiteTexture();
		
		/**
		 * @brief Get a cached 1x1 black texture.
		 */
		GLOBAL Ref<Texture2D> GetBlackTexture();
		
		/**
		 * @brief Retrieve Hilbert LUT texture (used for sampling patterns / blue noise / etc).
		 */
		GLOBAL Ref<Texture2D> GetHilbertLut();
		
		/**
		 * @brief Retrieve precomputed BRDF integration LUT texture.
		 */
		GLOBAL Ref<Texture2D> GetBRDFLutTexture();
		
		/**
		 * @brief Retrieve a black cube map texture (fallback environment).
		 */
		GLOBAL Ref<TextureCube> GetBlackCubeTexture();
		
		/**
		 * @brief Retrieve an empty environment asset (neutral lighting).
		 */
		GLOBAL Ref<Environment> GetEmptyEnvironment();
		
		/**
		 * @brief Register a shader -> pipeline dependency for hot reload propagation.
		 * @param shader Shader reference.
		 * @param pipeline Dependent graphics pipeline.
		 */
		void RegisterShaderDependency(Ref<Shader> &shader, Ref<Pipeline> &pipeline);
		
		/**
		 * @brief Register a shader -> material dependency.
		 * @param shader Shader reference.
		 * @param material Dependent material.
		 */
		void RegisterShaderDependency(Ref<Shader> &shader, Ref<Material> &material);
		
		/**
		 * @brief Register a shader -> compute pipeline dependency.
		 * @param shader Shader reference.
		 * @param computePipeline Dependent compute pipeline.
		 */
		void RegisterShaderDependency(Ref<Shader> &shader, Ref<ComputePipeline> &computePipeline);
		
		/**
		 * @brief Callback invoked when a shader finishes reloading.
		 * @param hash Shader unique identifier hash.
		 */
		void OnShaderReloaded(size_t hash);
		
		/**
		 * @brief Process all shaders marked dirty and propagate changes to dependents.
		 * @return True if any shader refresh occurred.
		 */
		GLOBAL bool UpdateDirtyShaders();
		
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Render Pass
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		/**
		 * @brief Begin a render pass for the current frame.
		 * @param CommandBuffer Command buffer reference.
		 * @param renderPass RenderPass abstraction to prepare/begin.
		 * @param explicitClear If true, forces explicit attachment clears.
		 */
		GLOBAL void BeginFrame(Ref<CommandBuffer> CommandBuffer, Ref<RenderPass> renderPass, bool explicitClear = false);

		/**
		 * @brief End the active render pass (frame scope).
		 * @param CommandBuffer Command buffer reference.
		 */
		GLOBAL void EndFrame(Ref<CommandBuffer> CommandBuffer);

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Compute Pass
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Begin a compute pass (descriptor/pipeline preparation).
         * @param CommandBuffer Command buffer reference.
         * @param computePass Compute pass object.
         */
        GLOBAL void BeginComputePass(Ref<CommandBuffer> CommandBuffer, Ref<ComputePass> computePass);

        /**
		 * @brief End a previously begun compute pass.
		 * @param CommandBuffer Command buffer reference.
		 * @param computePass Compute pass object.
		 */
		GLOBAL void EndComputePass(Ref<CommandBuffer> CommandBuffer, Ref<ComputePass> computePass);

		/**
		 * @brief Dispatch a compute workload.
		 * @param CommandBuffer Command buffer reference.
		 * @param computePass Active compute pass.
		 * @param material Material providing pipeline + descriptors.
		 * @param workGroups 3D work group counts.
		 * @param constants Optional push constant buffer.
		 */
		GLOBAL void DispatchCompute(Ref<CommandBuffer> CommandBuffer, Ref<ComputePass> computePass, Ref<Material> material, const UVec3& workGroups, Buffer constants = Buffer());

		/**
		 * @brief Clear an image with specified clear value.
		 * @param CommandBuffer Command buffer reference.
		 * @param image Target image.
		 * @param clearValue Clear color/depth/stencil specification.
		 * @param subresourceRange Optional subresource range (defaults to whole image).
		 */
		GLOBAL void ClearImage(Ref<CommandBuffer> CommandBuffer, Ref<Image2D> image, const ImageClearValue& clearValue, ImageSubresourceRange subresourceRange = ImageSubresourceRange());

		/**
		 * @brief Copy contents of a source image to a destination image.
		 * @param CommandBuffer Command buffer reference.
		 * @param sourceImage Source image.
		 * @param destinationImage Destination image.
		 */
		GLOBAL void CopyImage(Ref<CommandBuffer> CommandBuffer, Ref<Image2D> sourceImage, Ref<Image2D> destinationImage);

    private:
		/**
		 * @brief Internal access to the active command queue (front buffer).
		 * @return Reference to CommandQueue used for recording submitted lambdas.
		 */
		INTERNAL CommandQueue &GetCommandQueue();

		/**
		 * @brief Active swapchain reference (lifetime managed by renderer).
		 */
		Ref<SwapChain> m_SwapChain;
	};

}

/// -------------------------------------------------------

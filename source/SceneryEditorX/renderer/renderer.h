/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* renderer.h
* -------------------------------------------------------
* Created: 7/6/2025
* -------------------------------------------------------
*/
#pragma once
#include "blend_state.h"
#include "compute_pass.h"
#include "rasterizer.h"
#include "render_dispatcher.h"
#include "texture.h"
#include "viewport.h"
#include "SceneryEditorX/asset/mesh/mesh.h"
#include "SceneryEditorX/core/application/application.h"
#include "SceneryEditorX/scene/material.h"
#include "SceneryEditorX/scene/scene.h"
#include "SceneryEditorX/utils/pointers.h"
#include "buffers/index_buffer.h"
#include "fonts/font.h"

#include "shaders/shader.h"

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
     * as static (static) entry points to allow easy invocation across systems while
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
		// Async submission helpers (now routed through RenderDispatcher)
		template<typename FuncT>
		static inline void Submit(FuncT&& func)
		{
			RenderDispatcher::Enqueue(std::forward<FuncT>(func));
		}

		template<typename FuncT>
		static inline void SubmitResourceFree(FuncT&& func)
		{
			RenderDispatcher::EnqueueResourceFree(std::forward<FuncT>(func));
		}

		/**
		 * @brief Retrieve the global render context instance.
		 * @return Shared reference to RenderContext.
		 */
		static Ref<RenderContext> GetContext();

        /// -------------------------------------------------------

        static void Init();
		static void Shutdown();

        /// -------------------------------------------------------

        static void BeginFrame();
        static void EndFrame();
        static void SubmitFrame();

        /// -------------------------------------------------------

        /**
        * @brief Access current frame's aggregated immutable render data.
        * @return Reference to the active RenderData structure.
        */
        static RenderData &GetRenderData();

        /**
        * @brief Replace the active frame RenderData.
        * @param renderData New data snapshot copied into internal storage.
        */
        static void SetRenderData(const RenderData &renderData);

        /// -------------------------------------------------------

        /**
         * @brief Get the current frame-in-flight index (ring buffer slot).
         * @return Frame index.
         */
        static uint64_t GetCurrentFrameIndex();

        /// -------------------------------------------------------

        /**
         * @brief Build a VkDescriptorSetAllocateInfo helper struct.
         *
         * @param layouts Pointer to first layout in an array.
         * @param count Number of descriptor sets to allocate.
         * @param pool Optional explicit pool override.
         * @return Populated VkDescriptorSetAllocateInfo ready for vkAllocateDescriptorSets.
         */
        //VkDescriptorSetAllocateInfo DescriptorSetAllocInfo(const VkDescriptorSetLayout* layouts, uint32_t count = 1, VkDescriptorPool pool = nullptr);

        /**
         * @brief Allocate one or more descriptor sets from the current frame's pool.
         * @return Allocated VkDescriptorSet handles (empty if allocation failed).
         */
        //Ref<Font> &GetFont();

        /**
         * @brief Get a reference to the standard material (used for untextured meshes).
         * @return Shared reference to the standard material.
         */
        //Ref<Material> &GetStandardMaterial();

        /// -------------------------------------------------------

        /**
         * @brief Create a Vulkan sampler object.
         * @param samplerCreateInfo Mutable create info (fields may be adjusted internally).
         * @return Created VkSampler handle.
         */
        static VkSampler CreateSampler(VkSamplerCreateInfo &samplerCreateInfo);

        /**
         * @brief Destroy a Vulkan sampler previously created via CreateSampler().
         * @param sampler Sampler handle to destroy (ignored if VK_NULL_HANDLE).
         */
        static void DestroySampler(VkSampler sampler);

        /// -------------------------------------------------------

		/**
		 * @brief Access the global shader library.
		 * @return Shared reference to ShaderLibrary.
		 */
		//static Ref<ShaderLibrary> GetShaderLibrary();

		/**
		 * @brief Get the current frame index as seen from the render thread (may differ from CPU).
		 * @return Render thread frame index.
		 */
		static uint32_t GetCurrentRenderThreadFrameIndex();

		/**
		 * @brief Number of descriptor allocations performed this frame (for diagnostics).
		 * @param frameIndex Optional frame override (0 = current).
		 * @return Allocation count.
		 */
		//static uint32_t GetDescriptorAllocationCount(uint32_t frameIndex = 0);

		/**
		 * @brief Submit a fullscreen triangle/quad draw call with provided pipeline & material.
		 * @param ref Active command buffer reference.
		 * @param pipeline Graphics pipeline to bind.
		 * @param material Material providing descriptor sets / push constants.
		 */
		//static void SubmitFullscreenQuad(Ref<CommandBuffer> & ref, Ref<Pipeline> & pipeline, Ref<Material> & material);

		/**
		 * @brief Capture a screenshot of the current swapchain image.
		 * @param file_path Output file path (extension influences format if format empty).
		 * @param immediateDispatch If true, executes immediately instead of deferred.
		 * @param format Image format string ("png", "jpg", etc.).
		 */
		//static void Screenshot(const std::string &file_path, bool immediateDispatch = false, std::string format = "png");

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Viewport & Image Management
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        // viewport
        static const Viewport &GetViewport();
        static void SetViewport(float width, float height);

        // resolution render
        static const Vec2 &GetResolutionRender();
        static void SetResolutionRender(uint32_t width, uint32_t height, bool recreate_resources = true);

        // resolution output
        static const Vec2 &GetResolutionOutput();
        static void SetResolutionOutput(uint32_t width, uint32_t height, bool recreate_resources = true);

        /// -------------------------------------------------------

        //static void RenderStaticMesh(Ref<CommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<StaticMesh> mesh, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<MaterialTable> materialTable, Ref<VertexBuffer> transformBuffer, uint32_t transformOffset, uint32_t instanceCount);
		//static void RenderSubmeshInstanced(Ref<CommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Mesh> mesh, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<MaterialTable> materialTable, Ref<VertexBuffer> transformBuffer, uint32_t transformOffset, uint32_t boneTransformsOffset, uint32_t instanceCount);
		//static void RenderMeshWithMaterial(Ref<CommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Mesh> mesh, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<VertexBuffer> transformBuffer, uint32_t transformOffset, uint32_t boneTransformsOffset, uint32_t instanceCount, Ref<Material> material, Buffer additionalUniforms = Buffer());
		//static void RenderStaticMeshWithMaterial(Ref<CommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<StaticMesh> mesh, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<VertexBuffer> transformBuffer, uint32_t transformOffset, uint32_t instanceCount, Ref<Material> material, Buffer additionalUniforms = Buffer());
		//static void RenderQuad(Ref<CommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, const Mat4& transform);
		//static void SubmitFullscreenQuad(Ref<CommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material);
		//static void SubmitFullscreenQuadWithOverrides(Ref<CommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Buffer vertexShaderOverrides, Buffer fragmentShaderOverrides);
		//static void LightCulling(Ref<CommandBuffer> renderCommandBuffer, Ref<ComputePass> computePass, Ref<Material> material, const UVec3& workGroups);
		//static void RenderGeometry(Ref<CommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, const Mat4& transform, uint32_t indexCount = 0);
		//static void SubmitQuad(Ref<CommandBuffer> renderCommandBuffer, Ref<Material> material, const Mat4& transform = Mat4(1.0f));
		//static void BlitImage(Ref<CommandBuffer> renderCommandBuffer, Ref<Image2D> sourceImage, Ref<Image2D> destinationImage);

        /// -------------------------------------------------------

		//static Ref<Texture2D> GetWhiteTexture();
		//static Ref<Texture2D> GetBlackTexture();
		//static Ref<Texture2D> GetHilbertLut();
		//static Ref<Texture2D> GetBRDFLutTexture();
		//static Ref<TextureCube> GetBlackCubeTexture();
		//static Ref<Environment> GetEmptyEnvironment();

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Shader Management
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * @brief Register a shader -> pipeline dependency for hot reload propagation.
		 * @param type
		 * @param shader Shader reference.
		 * @param pipeline Dependent graphics pipeline.
		 */
		//void RegisterShaderDependency(Ref<Shader> &shader, Ref<Pipeline> &pipeline);

		/**
		 * @brief Register a shader -> material dependency.
		 * @param shader Shader reference.
		 * @param material Dependent material.
		 */
		//void RegisterShaderDependency(Ref<Shader> &shader, Ref<Material> &material);

		/**
		 * @brief Register a shader -> compute pipeline dependency.
		 * @param shader Shader reference.
		 * @param computePipeline Dependent compute pipeline.
		 */
		//void RegisterShaderDependency(Ref<Shader> &shader, Ref<ComputePipeline> &computePipeline);

		/**
		 * @brief Callback invoked when a shader finishes reloading.
		 * @param hash Shader unique identifier hash.
		 */
		//void OnShaderReloaded(size_t hash);

		/**
		 * @brief Process all shaders marked dirty and propagate changes to dependents.
		 * @return True if any shader refresh occurred.
		 */
		//static bool UpdateDirtyShaders();

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Render Pass
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * @brief Begin a render pass for the current frame.
		 * @param CommandBuffer Command buffer reference.
		 * @param renderPass RenderPass abstraction to prepare/begin.
		 * @param explicitClear If true, forces explicit attachment clears.
		 */
		//static void BeginFrame(Ref<CommandBuffer> CommandBuffer, Ref<RenderPass> renderPass, bool explicitClear = false);

		/**
		 * @brief End the active render pass (frame scope).
		 * @param CommandBuffer Command buffer reference.
		 */
		//static void EndFrame(Ref<CommandBuffer> CommandBuffer);

        /**
         * @brief Get a reference to a standard render target texture.
         *
         * @param type Render target type enum.
         * @return Shared reference to the requested Texture2D (can be null if not created).
         */
        static Ref<Texture2D> *GetRenderTarget(RenderTarget type);

        void SwapVisibilityBuffers();
        Texture2D *GetStandardTexture(StandardTexture type);

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Compute Pass
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Begin a compute pass (descriptor/pipeline preparation).
         * @param CommandBuffer Command buffer reference.
         * @param computePass Compute pass object.
         */
        //static void BeginComputePass(Ref<CommandBuffer> CommandBuffer, Ref<ComputePass> computePass);

        /**
		 * @brief End a previously begun compute pass.
		 * @param CommandBuffer Command buffer reference.
		 * @param computePass Compute pass object.
		 */
		//static void EndComputePass(Ref<CommandBuffer> CommandBuffer, Ref<ComputePass> computePass);

		/**
		 * @brief Dispatch a compute workload.
		 * @param CommandBuffer Command buffer reference.
		 * @param computePass Active compute pass.
		 * @param material Material providing pipeline + descriptors.
		 * @param workGroups 3D work group counts.
		 * @param constants Optional push constant buffer.
		 */
		//static void DispatchCompute(Ref<CommandBuffer> CommandBuffer, Ref<ComputePass> computePass, Ref<Material> material, const UVec3& workGroups, Buffer constants = Buffer());

		/**
		 * @brief Clear an image with specified clear value.
		 * @param CommandBuffer Command buffer reference.
		 * @param image Target image.
		 * @param clearValue Clear color/depth/stencil specification.
		 * @param subresourceRange Optional subresource range (defaults to whole image).
		 */
		//static void ClearImage(Ref<CommandBuffer> CommandBuffer, Ref<Image2D> image, const ImageClearValue& clearValue, ImageSubresourceRange subresourceRange = ImageSubresourceRange());

		/**
		 * @brief Copy contents of a source image to a destination image.
		 * @param CommandBuffer Command buffer reference.
		 * @param sourceImage Source image.
		 * @param destinationImage Destination image.
		 */
		//static void CopyImage(Ref<CommandBuffer> CommandBuffer, Ref<Image2D> sourceImage, Ref<Image2D> destinationImage);

		/**
		 * @brief Get number of nanoseconds required for a timestamp query to be incremented by 1
		 *
		 * @return The timestamp period in milliseconds.
		 * @see https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#queries-timestamps
		 */
        [[nodiscard]] double GetTimestampPeriodInMS() const;

    private:
		/**
		 * @brief Active swapchain reference (lifetime managed by renderer).
		 */
		Ref<SwapChain> m_SwapChain;

        /// -------------------------------------------------------

        static void CreateRenderTargets(bool create_render, bool create_output, bool create_dynamic);
        static void CreateDepthStencilStates();
        static void CreateRasterizerStates();
        static void CreateBlendModes();
        static void CreateShaders();
        static void CreateSamplers();
        static void CreateFonts();
        static void CreateStandardMeshes();
        static void CreateStandardTextures();
        static void CreateStandardMaterials();

        /// -------------------------------------------------------

	    std::array<Ref<Texture2D>, static_cast<uint32_t>(RenderTarget::MaxEnum)> &GetRenderTargets();
        std::array<Ref<Shader>, static_cast<uint32_t>(ShaderType::MaxEnum)> &GetShaders();

        Rasterizer *GetRasterizerState(RasterizerState type);
        BlendState *GetBlendState(const BlendMode type);

    };

}

/// -------------------------------------------------------

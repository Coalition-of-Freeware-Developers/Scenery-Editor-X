/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_render_pass.h
* -------------------------------------------------------
* Created: 16/4/2025
* -------------------------------------------------------
*/
#pragma once
#include <vulkan/vulkan.h>
#include "vk_descriptor_set_manager.h"
#include "vk_pipeline.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{

	/**
	 * @struct RenderSpec
	 * @brief Specifies the configuration for a Vulkan render pass.
	 *
	 * The RenderSpec structure holds the pipeline reference and a debug name
	 * for identifying and configuring a Vulkan render pass within the graphics engine.
	 */
	struct RenderSpec
	{
	    /**
	     * @brief Reference to the Vulkan pipeline used by this render pass.
	     */
	    Ref<Pipeline> Pipeline;

        /**
         * @brief Reference to the Vulkan swap chain used for rendering.
         */
        Vec4 idColor;

	    /**
	     * @brief Debug name for identifying the render pass instance.
	     */
	    std::string debugName;
	};

    /// -------------------------------------------------------

    /// -------------------------------------------------------

    /**
	 * @class RenderPass
	 * @brief Manages the Vulkan render pass and associated resources
	 *
	 * This class encapsulates the creation and management of a Vulkan render pass,
	 * including framebuffers, command buffers, and synchronization primitives.
	 * It provides methods for setting up the rendering pipeline and handling
	 * resource allocation and deallocation.
	 *
	 * @note This class is part of the Scenery Editor X graphics engine.
	 */
	class RenderPass : public RefCounted
	{
	public:
        explicit RenderPass(const RenderSpec &spec);
	    virtual ~RenderPass() override;

	    RenderSpec& GetSpecification() { return renderSpec; }
	    const RenderSpec& GetSpecification() const { return renderSpec; }

        /// -------------------------------------------------------

	    void AddInput(std::string_view name, Ref<UniformBufferSet> &uniformBufferSet);
        void AddInput(std::string_view name, Ref<UniformBuffer> &uniformBuffer);
	    void AddInput(std::string_view name, Ref<StorageBufferSet> &storageBufferSet);
        void AddInput(std::string_view name, Ref<StorageBuffer> &storageBuffer);
	    void AddInput(std::string_view name, Ref<Texture2D> &texture);
	    void AddInput(std::string_view name, Ref<TextureCube> &textureCube);
	    void AddInput(std::string_view name, Ref<Image2D> &image);

        /// -------------------------------------------------------
        /// -------------------------------------------------------

		Ref<Image2D> GetOutput(uint32_t index);
        Ref<Image2D> GetDepthOutput();
        uint32_t GetFirstSetIndex() const;
        Ref<Framebuffer> GetTargetFramebuffer() const;
        Ref<Pipeline> GetPipeline() const;

        /// -------------------------------------------------------

	    bool Validate();
		void Bake();
		bool Baked() const { return (bool)m_DescriptorSetManager.GetDescriptorPool(); }
		void Prepare();

        /// -------------------------------------------------------

		bool HasDescriptorSets() const;
		const std::vector<VkDescriptorSet>& GetDescriptorSets(uint32_t frameIndex) const;

		bool IsInputValid(std::string_view name) const;
		const RenderPassInputDeclaration* GetInputDeclaration(std::string_view name) const;

	private:
        RenderSpec renderSpec;                  /// Render specification containing pipeline and debug name
        RenderData renderData;					/// Render data containing information about the render pass
	    DescriptorSetManager m_DescriptorSetManager;

        bool IsInvalidated(uint32_t set, uint32_t binding) const;

	};

}

/// -------------------------------------------------------

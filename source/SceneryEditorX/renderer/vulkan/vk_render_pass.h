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
#include <SceneryEditorX/renderer/buffers/framebuffer.h>
#include <SceneryEditorX/renderer/vulkan/vk_descriptor_set_manager.h>
#include <SceneryEditorX/renderer/vulkan/vk_pipeline.h>
#include <SceneryEditorX/scene/texture.h>
#include <vulkan/vulkan.h>

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

		virtual RenderSpec& GetSpecification() { return renderSpec; }
		virtual const RenderSpec& GetSpecification() const { return renderSpec; }

        //void AddInput(std::string_view name, const Ref<UniformBufferSet> &uniformBufferSet);
        void AddInput(std::string_view name, const Ref<UniformBuffer> &uniformBuffer);
        //void AddInput(std::string_view name, const Ref<StorageBufferSet> &storageBufferSet);
        void AddInput(std::string_view name, const Ref<StorageBuffer> &storageBuffer);
        void AddInput(std::string_view name, const Ref<TextureAsset> &texture);

        void CreateRenderPass();

		virtual Ref<Image2D> GetOutput(uint32_t index);
        virtual Ref<Image2D> GetDepthOutput();
        virtual uint32_t GetFirstSetIndex() const;

        virtual Ref<Framebuffer> GetTargetFramebuffer() const;
        virtual Ref<Pipeline> GetPipeline() const;

        [[nodiscard]] VkRenderPass GetRenderPass() const { return renderPass; }
		//VkDescriptorPool GetDescriptorPool() const { return descriptors->descriptorPool; }

	    virtual bool Validate();
		virtual void Bake();
		//virtual bool Baked() const { return static_cast<bool>(m_DescriptorSetManager.GetDescriptorPool()); }
		virtual void Prepare();

		bool HasDescriptorSets() const;
		const std::vector<VkDescriptorSet>& GetDescriptorSets(uint32_t frameIndex) const;

		bool IsInputValid(std::string_view name) const;
		//const RenderPassInputDeclaration* GetInputDeclaration(std::string_view name);

	private:
        //Ref<SwapChain> vkSwapChain;			///< Reference to the Vulkan swap chain
        //Ref<MemoryAllocator> allocator;		///< Reference to the memory allocator for Vulkan resources
        //Ref<Descriptors> descriptors;			///< Reference to the Vulkan descriptor set manager
        //Ref<CommandBuffer> cmdBuffer;			///< Reference to the Vulkan command buffer manager
        //Ref<UniformBuffer> uniformBuffer;		///< Reference to the uniform buffer used in the render pass
        RenderData renderData;					///< Render data containing information about the render pass
        VkRenderPass renderPass = nullptr;		///< Vulkan render pass handle

		/// ----------------------------------

        VkQueue graphicsQueue = nullptr;
        VkQueue presentQueue = nullptr;

        RenderSpec renderSpec;
        //DescriptorSetManager m_DescriptorSetManager;

		/// ----------------------------------

        void CreateDescriptorSets() const;
        bool IsInvalidated(uint32_t set, uint32_t binding) const;
        [[nodiscard]] VkCommandBuffer BeginSingleTimeCommands() const;
        void EndSingleTimeCommands(VkCommandBuffer commandBuffer) const;
        void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const;
        void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;
        void GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) const;
        void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
						 VkImage &image, VkDeviceMemory &imageMemory) const;

	};

}

/// -------------------------------------------------------

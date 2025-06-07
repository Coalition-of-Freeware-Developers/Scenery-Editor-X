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
#include <GraphicsEngine/buffers/uniform_buffer.h>
#include <GraphicsEngine/vulkan/vk_allocator.h>
#include <GraphicsEngine/vulkan/vk_cmd_buffers.h>
#include <GraphicsEngine/vulkan/vk_descriptors.h>
#include <GraphicsEngine/vulkan/vk_swapchain.h>
#include <SceneryEditorX/scene/texture.h>

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
	    Ref<Pipeline> vkPipeline;
	
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
	class RenderPass
	{
	public:
        RenderPass(const RenderSpec& spec);
	    virtual ~RenderPass();

		void AddInput(std::string_view name, Ref<UniformBuffer> uniformBuffer);
		//void AddInput(std::string_view name, Ref<UniformBufferSet> uniformBufferSet);

	    //void AddInput(std::string_view name, Ref<StorageBuffer> storageBuffer);
        //void AddInput(std::string_view name, Ref<StorageBuffer> storageBufferSet);

		//void AddInput(std::string_view name, Ref<TextureAsset> texture);

        void CreateRenderPass();

        [[nodiscard]] VkRenderPass GetRenderPass() const { return renderPass; }
		//VkDescriptorPool GetDescriptorPool() const { return descriptors->descriptorPool; }

	private:
        Ref<GraphicsEngine> *gfxEngine;		///< Pointer to the graphics engine reference
        Ref<SwapChain> vkSwapChain;			///< Reference to the Vulkan swap chain
        Ref<MemoryAllocator> allocator;		///< Reference to the memory allocator for Vulkan resources
        Ref<Descriptors> descriptors;		///< Reference to the Vulkan descriptor set manager
        Ref<CommandBuffer> cmdBuffer;		///< Reference to the Vulkan command buffer manager
        Ref<UniformBuffer> uniformBuffer;	///< Reference to the uniform buffer used in the render pass
        RenderData renderData;				///< Render data containing information about the render pass
        VkRenderPass renderPass = nullptr;	///< Vulkan render pass handle

		/// ----------------------------------

        VkQueue graphicsQueue = nullptr;
        VkQueue presentQueue = nullptr;

		/// ----------------------------------

        void CreateDescriptorSets() const;

        [[nodiscard]] VkCommandBuffer BeginSingleTimeCommands() const;
        //void EndSingleTimeCommands(VkCommandBuffer commandBuffer) const;
        //void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const;
        //void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;
        void GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) const;
        void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
						 VkImage &image, VkDeviceMemory &imageMemory) const;

	};

} // namespace SceneryEditorX

// -------------------------------------------------------

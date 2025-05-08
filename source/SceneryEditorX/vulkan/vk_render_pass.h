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
#include <SceneryEditorX/vulkan/vk_allocator.h>
#include <SceneryEditorX/vulkan/vk_core.h>
#include <SceneryEditorX/vulkan/vk_device.h>
#include <SceneryEditorX/vulkan/vk_swapchain.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

	class RenderPass
	{
	public:
	    RenderPass() = default;
	    virtual ~RenderPass();
        void CreateRenderPass();
	    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory) const;
        void UpdateUniformBuffer(uint32_t currentImage) const;

        [[nodiscard]] VkRenderPass GetRenderPass() const { return renderPass; }

	private:
        GraphicsEngine *renderer;
        Ref<VulkanDevice> vkDevice;
        Ref<SwapChain> vkSwapChain;
        //Ref<MemoryAllocator> allocator;
        RenderData renderData;
	    VkRenderPass renderPass = nullptr;
        VkAllocationCallbacks *allocator = nullptr;

		// ----------------------------------

        VkQueue graphicsQueue = nullptr;
        VkQueue presentQueue = nullptr;

		// ----------------------------------

        VkCommandPool cmdPool;

        void CreateDescriptorSets();

        VkCommandBuffer BeginSingleTimeCommands() const;

        void EndSingleTimeCommands(VkCommandBuffer commandBuffer) const;
        void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const;
        void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;

        void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format,
						 VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory);

	};

} // namespace SceneryEditorX

// -------------------------------------------------------

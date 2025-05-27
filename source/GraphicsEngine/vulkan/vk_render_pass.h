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

// -------------------------------------------------------

namespace SceneryEditorX
{

    struct RenderSpec
	{
        Ref<Pipeline> vkPipeline;
        std::string DebugName;
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

		void AddInput(std::string_view name, Ref<TextureAsset> texture);

        void CreateRenderPass();

        [[nodiscard]] VkRenderPass GetRenderPass() const { return renderPass; }
		//VkDescriptorPool GetDescriptorPool() const { return descriptors->descriptorPool; }
	private:
        GraphicsEngine *gfxEngine;
        Ref<SwapChain> vkSwapChain;
        Ref<MemoryAllocator> allocator;
        Ref<Descriptors> descriptors;
        Ref<CommandBuffer> cmdBuffer;
        Ref<UniformBuffer> uniformBuffer;
        RenderData renderData;
	    VkRenderPass renderPass = nullptr;
        //VkAllocationCallbacks *allocator = nullptr;

		/// ----------------------------------

        VkQueue graphicsQueue = nullptr;
        VkQueue presentQueue = nullptr;

		/// ----------------------------------

        void CreateDescriptorSets() const;

        [[nodiscard]] VkCommandBuffer BeginSingleTimeCommands() const;
        void EndSingleTimeCommands(VkCommandBuffer commandBuffer) const;
        void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const;
        void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;
        void GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) const;
        void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
						 VkImage &image, VkDeviceMemory &imageMemory) const;

	};

} // namespace SceneryEditorX

// -------------------------------------------------------

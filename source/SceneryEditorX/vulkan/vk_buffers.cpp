/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_buffers.cpp
* -------------------------------------------------------
* Created: 5/5/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/vulkan/vk_allocator.h>
#include <SceneryEditorX/vulkan/vk_buffers.h>
#include <SceneryEditorX/vulkan/vk_core.h>
#include <SceneryEditorX/renderer/render_data.h>

// ----------------------------------------------------------

namespace SceneryEditorX
{
	UniformBuffer::~UniformBuffer()
    {
		for (size_t i = 0; i < RenderData::framesInFlight; i++)
		{
            vkDestroyBuffer(vkDevice->GetDevice(), uniformBuffers[i], nullptr);
            vkFreeMemory(vkDevice->GetDevice(), uniformBuffersMemory[i], nullptr);
		}
	}

	/**
	 * @brief Updates the uniform buffer for the current frame
	 * 
	 * This method updates the model-view-projection matrices for rendering.
	 * It calculates:
	 * 1. The model matrix with rotation based on elapsed time
	 * 2. The view matrix (camera position/orientation)
	 * 3. The projection matrix with aspect ratio correction
	 * 
	 * The updated matrices are uploaded to the GPU in the uniform buffer
	 * corresponding to the current frame being rendered.
	 * 
	 * @param currentImage Index of the current frame's uniform buffer to update
	 */
    void UniformBuffer::UpdateUniformBuffer(uint32_t currentImage) const
    {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float>(currentTime - startTime).count();

        UBO uniformBuff;
        uniformBuff.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        uniformBuff.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        uniformBuff.proj = glm::perspective(glm::radians(45.0f),vkSwapChain->GetSwapExtent().width / static_cast<float>(vkSwapChain->GetSwapExtent().height),0.1f,10.0f);
        uniformBuff.proj[1][1] *= -1;

        void *data;

        vkMapMemory(vkDevice->GetDevice(), uniformBuffersMemory[currentImage], 0, sizeof(uniformBuff), 0, &data);
        memcpy(data, &uniformBuff, sizeof(uniformBuff));
        vkUnmapMemory(vkDevice->GetDevice(), uniformBuffersMemory[currentImage]);
    }



    void UniformBuffer::CreateUniformBuffers()
    {

        uniformBuffers.resize(RenderData::framesInFlight);
        uniformBuffersMemory.resize(RenderData::framesInFlight);

        for (size_t i = 0; i < RenderData::framesInFlight; i++)
        {
            VkDeviceSize bufferSize = sizeof(UniformBuffer);
            CreateBuffer(bufferSize,
                         VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         uniformBuffers[i], uniformBuffersMemory[i]);
        }
    }

	void UniformBuffer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory) const
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(vkDevice->GetDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
        {
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to create buffer!");
        }

        // -------------------------------------------------------

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(vkDevice->GetDevice(), buffer, &memRequirements);

        // -------------------------------------------------------

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = vkDevice->FindMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(vkDevice->GetDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
        {
            SEDX_CORE_ERROR_TAG("Memory Allocator", "Failed to allocate buffer memory!");
        }

        vkBindBufferMemory(vkDevice->GetDevice(), buffer, bufferMemory, 0);
    }

    // ----------------------------------------------------------

    IndexBuffer::~IndexBuffer()
    {
		for (size_t i = 0; i < RenderData::framesInFlight; i++)
		{
            vkDestroyBuffer(vkDevice->GetDevice(), indexBuffer, nullptr);
            vkFreeMemory(vkDevice->GetDevice(), indexBufferMemory, nullptr);
		}
    }

    void IndexBuffer::CreateIndexBuffer()
    {
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        CreateBuffer(bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingBuffer, stagingBufferMemory);

        void *data;

        vkMapMemory(vkDevice->GetDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), (size_t)bufferSize);
        vkUnmapMemory(vkDevice->GetDevice(), stagingBufferMemory);

        CreateBuffer(bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     indexBuffer, indexBufferMemory);

        CopyBuffer(stagingBuffer, indexBuffer, bufferSize);

        vkDestroyBuffer(vkDevice->GetDevice(), stagingBuffer, nullptr);
        vkFreeMemory(vkDevice->GetDevice(), stagingBufferMemory, nullptr);
    }

    // ----------------------------------------------------------

    VertexBuffer::~VertexBuffer()
    {
		for (size_t i = 0; i < RenderData::framesInFlight; i++)
		{
            vkDestroyBuffer(vkDevice->GetDevice(), vertexBuffer, nullptr);
            vkFreeMemory(vkDevice->GetDevice(), vertexBufferMemory, nullptr);
		}
    }

    void VertexBuffer::CreateVertexBuffer()
    {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        CreateBuffer(bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingBuffer,
                     stagingBufferMemory);

        void *data;
        vkMapMemory(vkDevice->GetDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t)bufferSize);
        vkUnmapMemory(vkDevice->GetDevice(), stagingBufferMemory);

        CreateBuffer(bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     vertexBuffer,
                     vertexBufferMemory);

        CopyBuffer(stagingBuffer, vertexBuffer, bufferSize);

        vkDestroyBuffer(vkDevice->GetDevice(), stagingBuffer, nullptr);
        vkFreeMemory(vkDevice->GetDevice(), stagingBufferMemory, nullptr);
    }

/*
    GLOBAL void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const
    {
        VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        EndSingleTimeCommands(commandBuffer);
    }

	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const
    {
        VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {.x = 0, .y = 0, .z = 0};
        region.imageExtent = {.width = width, .height = height, .depth = 1};

        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        EndSingleTimeCommands(commandBuffer);
    }

	VkCommandBuffer RenderPass::BeginSingleTimeCommands() const
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = cmdPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(vkDevice->GetDevice(), &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void RenderPass::EndSingleTimeCommands(VkCommandBuffer commandBuffer) const
    {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(vkDevice->GetDevice(), cmdPool, 1, &commandBuffer);
    }
	*/

    Buffer CreateBuffer(uint32_t size, BufferUsageFlags usage, MemoryFlags memory, const std::string &name)
    {
        VmaAllocator vmaAllocator = GraphicsEngine::GetCurrentDevice()->GetMemoryAllocator();

		// ---------------------------------------------------------

		if (usage & BufferUsage::Vertex)
            usage |= BufferUsage::TransferDst;

        if (usage & BufferUsage::Index)
            usage |= BufferUsage::TransferDst;

        if (usage & BufferUsage::Storage)
		{
		    usage |= BufferUsage::Address;
            size += size % GraphicsEngine::GetCurrentDevice()->GetPhysicalDevice()->Selected().deviceProperties.limits.minStorageBufferOffsetAlignment;
		}

		if (usage & BufferUsage::AccelerationStructureInput)
		{
		    usage |= BufferUsage::Address;
		    usage |= BufferUsage::TransferDst;
		}

		if (usage & BufferUsage::AccelerationStructure)
            usage |= BufferUsage::Address;

		const Ref<BufferResource> resource = CreateRef<BufferResource>(size, usage, memory, name);

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = static_cast<VkBufferUsageFlagBits>(usage);
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        if (memory & MemoryType::CPU)
            allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

        auto result = vmaCreateBuffer(vmaAllocator, &bufferInfo, &allocInfo, &resource->buffer, &resource->allocation, nullptr);
        SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to create buffer!");

        Buffer buffer = {
            .resource = resource,
            .size = size,
            .usage = usage,
            .memory = memory,
        };

        if (usage & BufferUsage::Storage)
        {
            BindlessResources::bindlessDescriptorSet = bindlessDescript;

            resource->resourceID = ImageID::availBufferRID.back();
            ImageID::availBufferRID.pop_back();

            VkDescriptorBufferInfo descriptorInfo = {};
            VkWriteDescriptorSet write = {};
            descriptorInfo.buffer = resource->buffer;
            descriptorInfo.offset = 0;
            descriptorInfo.range = size;
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = BindlessResources::bindlessDescriptorSet;
            write.dstBinding = 1;
            write.dstArrayElement = buffer.ResourceID();
            write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            write.descriptorCount = 1;
            write.pBufferInfo = &descriptorInfo;
            vkUpdateDescriptorSets(GraphicsEngine::GetCurrentDevice()->GetDevice(), 1, &write, 0, nullptr);
        }

        return buffer;
    }

} // namespace SceneryEditorX

// ----------------------------------------------------------

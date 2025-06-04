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
#include <GraphicsEngine/buffers/buffer_data.h>
#include <GraphicsEngine/vulkan/image_data.h>
#include <GraphicsEngine/vulkan/render_data.h>
#include <GraphicsEngine/vulkan/vk_buffers.h>
#include <GraphicsEngine/vulkan/vk_core.h>

/// ----------------------------------------------------------

namespace SceneryEditorX
{
    VkCommandBuffer BeginSingleTimeCommands()
    {
        const VkDevice vkDevice = GraphicsEngine::GetCurrentDevice()->GetDevice();
        const VkCommandPool cmdPool = GraphicsEngine::Get()->GetCommandBuffer()->GetCommandPool()->GetComputeCmdPool();

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = cmdPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(vkDevice, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    GLOBAL void EndSingleTimeCommands(VkCommandBuffer commandBuffer)
    {
        vkEndCommandBuffer(commandBuffer);

        const VkDevice vkDevice = GraphicsEngine::GetCurrentDevice()->GetDevice();
        const VkQueue graphicsQueue = GraphicsEngine::GetCurrentDevice()->GetGraphicsQueue();
        const VkCommandPool cmdPool = GraphicsEngine::Get()->GetCommandBuffer()->GetCommandPool()->GetComputeCmdPool();

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(vkDevice, cmdPool, 1, &commandBuffer);
    }

	/**
	 * @brief Creates a Vulkan buffer with specified parameters
	 * 
	 * This function handles the creation of a Vulkan buffer with appropriate memory allocation using VMA.
	 * It automatically applies usage flags based on the buffer's intended purpose and handles memory
	 * alignment requirements for different buffer types.
	 * 
	 * The function performs the following:
	 * 1. Adjusts usage flags based on the provided buffer usage type
	 * 2. Adds transfer destination flags for vertex and index buffers
	 * 3. Handles special requirements for storage buffers and acceleration structures
	 * 4. Creates the buffer with VMA memory allocation
	 * 5. Sets up descriptor updates for storage buffers
	 * 
	 * @param size   Size of the buffer in bytes
	 * @param usage  Flags specifying how the buffer will be used
	 * @param memory Memory property flags for the buffer allocation
	 * @param name   Optional name for the buffer (for debugging purposes)
	 * 
	 * @return       A Buffer structure containing the created buffer and its metadata
	 */
    Buffer CreateBuffer(uint64_t size, BufferUsageFlags usage, MemoryFlags memory, const std::string &name)
	{
	    /// Get the allocator from the current device
	    const VmaAllocator vmaAllocator = GraphicsEngine::GetCurrentDevice()->GetMemoryAllocator();
	
	    // ---------------------------------------------------------
	    
	    /// Add transfer destination flag for vertex buffers
	    if (usage & BufferUsage::Vertex)
	        usage |= BufferUsage::TransferDst;
	    
	    /// Add transfer destination flag for index buffers
	    if (usage & BufferUsage::Index)
	        usage |= BufferUsage::TransferDst;
	    
	    /// Handle storage buffers - add address flag and align size
	    if (usage & BufferUsage::Storage)
	    {
	        usage |= BufferUsage::Address;
	        /// Align storage buffer size to minimum required alignment
	        size += size % GraphicsEngine::GetCurrentDevice()
	                           ->GetPhysicalDevice()
	                           ->Selected()
	                           .deviceProperties.limits.minStorageBufferOffsetAlignment;
	    }
	    
	    /// Handle acceleration structure input buffers
	    if (usage & BufferUsage::AccelerationStructureInput)
	    {
	        usage |= BufferUsage::Address;
	        usage |= BufferUsage::TransferDst;
	    }
	    
	    /// Handle acceleration structure buffers
	    if (usage & BufferUsage::AccelerationStructure)
	        usage |= BufferUsage::Address;
	    
	    /// Create buffer resource
	    Ref<BufferResource> resource = CreateRef<BufferResource>();
	    
	    /// Configure buffer creation info
	    VkBufferCreateInfo bufferInfo{};
	    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	    bufferInfo.size = size;
	    bufferInfo.usage = static_cast<VkBufferUsageFlagBits>(usage);
	    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	    
	    /// Configure memory allocation info
	    VmaAllocationCreateInfo allocInfo = {};
	    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	    /// Enable memory mapping for CPU-accessible buffers
	    if (memory & CPU)
	        allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	    
	    /// Create the buffer with VMA
	    SEDX_ASSERT(vmaCreateBuffer(vmaAllocator, &bufferInfo, &allocInfo, &resource->buffer, &resource->allocation, nullptr));
	    
	    /// Create and populate the buffer wrapper
	    Buffer buffer = {
	        .bufferResource = resource,
	        .size = size,
	        .usage = usage,
	        .memory = memory,
	    };

	    /// Handle storage buffer descriptors for bindless access
	    if (usage & BufferUsage::Storage)
	    {
	        BindlessResources bindlessDescript;
	        
	        /// Get a resource ID from the available pool
	        resource->resourceID = ImageID::availBufferRID.back();
	        ImageID::availBufferRID.pop_back();
	        
	        /// Set up descriptor info for the storage buffer
	        VkDescriptorBufferInfo descriptorInfo;
	        VkWriteDescriptorSet write = {};
	        descriptorInfo.buffer = resource->buffer;
	        descriptorInfo.offset = 0;
	        descriptorInfo.range = size;
	        
	        /// Configure descriptor write operation
	        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	        write.dstSet = bindlessDescript.bindlessDescriptorSet;
	        write.dstBinding = 1;
	        write.dstArrayElement = buffer.ResourceID();
	        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	        write.descriptorCount = 1;
	        write.pBufferInfo = &descriptorInfo;
	        
	        /// Update descriptor set with buffer info
	        vkUpdateDescriptorSets(GraphicsEngine::GetCurrentDevice()->GetDevice(), 1, &write, 0, nullptr);
	    }
	    
	    return buffer;
	}

    void *MapBuffer(const Buffer &buffer)
    {
        SEDX_ASSERT(buffer.memory & MemoryType::CPU, "Buffer not accessible to the CPU.");
		void* data = nullptr; /// Initialize data
		/// Use VMA to map memory
		vmaMapMemory(GraphicsEngine::GetCurrentDevice()->GetMemoryAllocator(), buffer.bufferResource->allocation, &data);
        return data; /// Return the mapped pointer directly
	}

    /// ----------------------------------------------------------

	/**
	 * @fn CreateStagingBuffer
	 * @brief Creates a host-visible staging buffer for CPU-to-GPU data transfer
	 * 
	 * @details This method creates a buffer with transfer source usage and CPU-accessible memory,
	 * which is optimized for staging operations where data needs to be transferred from the CPU
	 * to the GPU. Staging buffers are commonly used in graphics pipelines for uploading textures,
	 * mesh data, and other GPU resources.
	 * 
	 * The staging buffer is allocated with @enum VK_BUFFER_USAGE_TRANSFER_SRC_BIT and mapped memory
	 * that can be written to by the CPU. After writing data to this buffer, use a command buffer 
	 * to copy the data to a GPU-local device buffer for optimal rendering performance.
	 * 
	 * @param size Size of the buffer in bytes
	 * @param name Optional debug name for the buffer (defaults to "Staging Buffer" if empty)
	 * @return Buffer A configured buffer object ready for staging operations
	 * 
	 * @see @fn CreateBuffer
	 */
    Buffer VulkanDevice::CreateStagingBuffer(const uint32_t size, const std::string &name)
    {
        return CreateBuffer(size, BufferUsage::TransferSrc, MemoryType::CPU, name.empty() ? "Staging Buffer" : name);
    }

    GLOBAL void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
    {
        Ref<CommandBuffer> commandBuffer = GraphicsEngine::Get()->GetCommandBuffer();
        //VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer->GetActiveCmdBuffer(), srcBuffer, dstBuffer, 1, &copyRegion);

        EndSingleTimeCommands(commandBuffer->GetActiveCmdBuffer());
    }

    GLOBAL void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
    {
        Ref<CommandBuffer> commandBuffer = GraphicsEngine::Get()->GetCommandBuffer();
        //VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

        VkBufferImageCopy region;
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {.x = 0, .y = 0, .z = 0};
        /// Assuming WindowData::width and WindowData::height are accessible and are the intended values.
        /// If width and height parameters are intended, use them instead.
        /// For now, using the parameters passed to the function.
        region.imageExtent = {.width = width, .height = height, .depth = 1};

        vkCmdCopyBufferToImage(commandBuffer->GetActiveCmdBuffer(), buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        EndSingleTimeCommands(commandBuffer->GetActiveCmdBuffer());
    }

} // namespace SceneryEditorX

// ----------------------------------------------------------

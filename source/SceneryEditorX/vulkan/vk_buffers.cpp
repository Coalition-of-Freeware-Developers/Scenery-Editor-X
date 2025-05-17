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
#include <SceneryEditorX/core/memory.h>
#include <SceneryEditorX/vulkan/render_data.h>
#include <SceneryEditorX/vulkan/vk_allocator.h>
#include <SceneryEditorX/vulkan/vk_buffers.h>
#include <SceneryEditorX/vulkan/vk_core.h>

// ----------------------------------------------------------

namespace SceneryEditorX
{

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
    GLOBAL Buffer CreateBuffer(uint64_t size, BufferUsageFlags usage, MemoryFlags memory, const std::string &name)
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

    INTERNAL void *MapBuffer(Buffer &buffer)
    {
        SEDX_ASSERT(buffer.memory & MemoryType::CPU, "Buffer not accessible to the CPU.");
		void *mappedData;
		vmaMapMemory(GraphicsEngine::GetCurrentDevice()->GetMemoryAllocator(), buffer.bufferResource->allocation, &mappedData);
		return mappedData;
	}

    // ----------------------------------------------------------

	UniformBuffer::UniformBuffer()
    {
        CreateUniformBuffers();
    }

    /**
	 * @brief Destroys uniform buffer resources
	 * 
	 * This destructor properly cleans up Vulkan resources allocated for uniform buffers:
	 * 1. Iterates through all buffers created for each frame in flight
	 * 2. Destroys each VkBuffer handle
	 * 3. Frees the associated device memory allocation
	 * 
	 * This ensures proper resource cleanup and prevents memory leaks when the
	 * UniformBuffer object is destroyed.
	 */
	UniformBuffer::~UniformBuffer()
    {
        for (size_t i = 0; i < renderData.framesInFlight; i++)
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

        UBO uniformBuff{};
        uniformBuff.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        uniformBuff.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        uniformBuff.proj = glm::perspective(glm::radians(45.0f),static_cast<float>(gfxEngine->get()->GetSwapChain()->GetSwapExtent().width) / static_cast<float>(gfxEngine->get()->GetSwapChain()->GetSwapExtent().height),0.1f,10.0f);
        uniformBuff.proj[1][1] *= -1;

        void *data;

        vkMapMemory(vkDevice->GetDevice(), uniformBuffersMemory[currentImage], 0, sizeof(uniformBuff), 0, &data);
        memcpy(data, &uniformBuff, sizeof(uniformBuff));
        vkUnmapMemory(vkDevice->GetDevice(), uniformBuffersMemory[currentImage]);
    }

    /**
     * @brief Creates uniform buffers for each frame in flight
     * 
     * This method initializes uniform buffers needed for shader uniform data:
     * 1. Resizes storage vectors to match the number of frames in flight
     * 2. Allocates one uniform buffer per frame with appropriate memory flags
     * 3. Ensures buffers are host-visible and coherent for efficient CPU updates
     * 
     * The uniform buffers are sized according to the UniformBuffer structure size and
     * are configured for efficient CPU-to-GPU data transfer. Each buffer in the sequence
     * corresponds to a specific frame in flight, preventing race conditions during rendering.
     */
    void UniformBuffer::CreateUniformBuffers()
    {
        uniformBuffers.resize(renderData.framesInFlight);
        uniformBuffersMemory.resize(renderData.framesInFlight);

        for (size_t i = 0; i < renderData.framesInFlight; i++)
        {
            VkDeviceSize bufferSize = sizeof(UniformBuffer);
            CreateBuffer(bufferSize, BufferUsage::Uniform, MemoryType::CPU, uniformBuffers[i], uniformBuffersMemory[i]);
        }
    }

	/**
	 * @brief Creates a Vulkan buffer with specified parameters
	 * 
	 * This method creates a Vulkan buffer and allocates device memory for it.
	 * The process involves:
	 * 1. Setting up buffer creation parameters
	 * 2. Creating the buffer handle
	 * 3. Retrieving memory requirements
	 * 4. Allocating device memory based on those requirements
	 * 5. Binding the memory to the buffer
	 * 
	 * The method handles error checking at critical steps and reports failures
	 * through the engine's logging system.
	 * 
	 * @param size Size of the buffer in bytes
	 * @param usage VkBufferUsageFlags indicating how the buffer will be used (e.g., uniform buffer)
	 * @param properties Memory property flags (e.g., host visible, device local)
	 * @param buffer Reference to store the created VkBuffer handle 
	 * @param bufferMemory Reference to store the allocated VkDeviceMemory handle
	 */
	void UniformBuffer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory) const
	{
	    VkBufferCreateInfo bufferInfo{};
	    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	    bufferInfo.size = size;
	    bufferInfo.usage = usage;
	    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	
	    if (vkCreateBuffer(vkDevice->GetDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to create buffer!");

        // -------------------------------------------------------
	
	    VkMemoryRequirements memRequirements;
	    vkGetBufferMemoryRequirements(vkDevice->GetDevice(), buffer, &memRequirements);
	
	    // -------------------------------------------------------
	
	    VkMemoryAllocateInfo allocInfo{};
	    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	    allocInfo.allocationSize = memRequirements.size;
	    allocInfo.memoryTypeIndex = vkDevice->FindMemoryType(memRequirements.memoryTypeBits, properties);
	
	    if (vkAllocateMemory(vkDevice->GetDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
            SEDX_CORE_ERROR_TAG("Memory Allocator", "Failed to allocate buffer memory!");

        vkBindBufferMemory(vkDevice->GetDevice(), buffer, bufferMemory, 0);
	}

    /// ----------------------------------------------------------

    void IndexBuffer::CreateIndexBuffer() const
    {
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
        VkBuffer stagingBuffer = nullptr;
        VkDeviceMemory stagingBufferMemory = nullptr;

		/// -----------------------------------

        CreateBuffer(bufferSize, BufferUsage::TransferSrc, MemoryType::CPU, "IndexStaging#");

        void *data;

        vkMapMemory(vkDevice->GetDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), (size_t)bufferSize);
        vkUnmapMemory(vkDevice->GetDevice(), stagingBufferMemory);

		//TODO: Add back the UUID when fully implemented.
        CreateBuffer(bufferSize, BufferUsage::Index | BufferUsage::AccelerationStructureInput | BufferUsage::Storage, MemoryType::GPU, "IndexBuffer#" /*+ std::to_string(asset->uuid)*/);

        CopyBuffer(stagingBuffer, indexBuffer, bufferSize);

		/// -----------------------------------

        vkDestroyBuffer(vkDevice->GetDevice(), stagingBuffer, nullptr);
        vkFreeMemory(vkDevice->GetDevice(), stagingBufferMemory, nullptr);
    }

    IndexBuffer::IndexBuffer()
    {
        CreateIndexBuffer();
    }

    /**
     * @brief Destroys index buffer resources
     * 
     * This destructor properly cleans up Vulkan resources allocated for index buffers.
     * It iterates through all frames in flight and ensures proper destruction of:
     * - The VkBuffer handle with vkDestroyBuffer
     * - The associated device memory with vkFreeMemory
     * 
     * Note: Current implementation appears to destroy the same buffer multiple times
     * rather than using separate buffers per frame. This should be revisited to ensure
     * proper cleanup of frame-specific resources if multiple buffers are used.
     */
    IndexBuffer::~IndexBuffer()
    {
        for (size_t i = 0; i < renderData.framesInFlight; i++)
        {
            vkDestroyBuffer(vkDevice->GetDevice(), indexBuffer, nullptr);
            vkFreeMemory(vkDevice->GetDevice(), indexBufferMemory, nullptr);
        }
    }

    /// ----------------------------------------------------------

    VertexBuffer::VertexBuffer()
    {
		CreateVertexBuffer();
    }

    VertexBuffer::~VertexBuffer()
    {
        for (size_t i = 0; i < renderData.framesInFlight; i++)
		{
            vkDestroyBuffer(vkDevice->GetDevice(), vertexBuffer, nullptr);
            vkFreeMemory(vkDevice->GetDevice(), vertexBufferMemory, nullptr);
		}
    }

    void VertexBuffer::CreateVertexBuffer() const
    {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
        VkBuffer stagingBuffer = nullptr;
        VkDeviceMemory stagingBufferMemory = nullptr;

	    /// --------------------------------------

        CreateBuffer(bufferSize, BufferUsage::TransferSrc, MemoryType::CPU, "VertexStaging#");

        void *data;
        vkMapMemory(vkDevice->GetDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t)bufferSize);
        vkUnmapMemory(vkDevice->GetDevice(), stagingBufferMemory);

		/// --------------------------------------

        CreateBuffer(bufferSize, BufferUsage::Vertex | BufferUsage::TransferDst, MemoryType::GPU, "VertexBuffer#");

        CopyBuffer(stagingBuffer, vertexBuffer, bufferSize);

		/// --------------------------------------

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

	/*
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
        return Buffer();
    }

    void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
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

    void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
    {

    }

} // namespace SceneryEditorX

// ----------------------------------------------------------

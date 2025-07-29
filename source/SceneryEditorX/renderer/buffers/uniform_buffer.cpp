/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* uniform_buffer.cpp
* -------------------------------------------------------
* Created: 18/5/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/renderer/buffers/uniform_buffer.h>
#include <SceneryEditorX/renderer/render_context.h>

/// --------------------------------------------

namespace SceneryEditorX
{
    /**
     * @brief Constructor for UniformBuffer
     * 
     * Initializes the UniformBuffer instance by setting up the
     * memory allocator reference from the graphics engine.
     */
    UniformBuffer::UniformBuffer(uint32_t size) : size(size)
    {
        localMemAlloc = hnew uint8_t[size];

    }

    /**
     * @brief Destructor for UniformBuffer
     * 
     * Cleans up resources by destroying all uniform buffers and freeing their 
     * associated memory allocations. This ensures proper resource cleanup when
     * the UniformBuffer object is destroyed.
     * 
     * The method iterates through each frame's buffer (based on framesInFlight)
     * and calls the memory allocator's DestroyBuffer method to release both 
     * the Vulkan buffer objects and their backing memory allocations.
     */
    UniformBuffer::~UniformBuffer()
    {
        if (!allocation)
			return;

        for (size_t i = 0; i < uniformBuffers.size(); i++)
        {
            if (uniformBuffers[i] != VK_NULL_HANDLE)
            {
                MemoryAllocator allocation("UniformBuffer");
                allocation.DestroyBuffer(uniformBuffers[i], uniformBuffersAllocation[i]);
            }
        }

		allocation = nullptr;

		delete[] localMemAlloc;
        localMemAlloc = nullptr;
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
	/*
	void UniformBuffer::Update(const uint32_t currentImage) const
	{
	    INTERNAL auto startTime = std::chrono::high_resolution_clock::now();
	    const auto currentTime = std::chrono::high_resolution_clock::now();
	    const float time = std::chrono::duration<float>(currentTime - startTime).count();
	
	    UBO uniformBuff = {};
	    uniformBuff.model = glm::rotate(Mat4(1.0f), time * glm::radians(90.0f), Vec3(0.0f, 0.0f, 1.0f));
	    uniformBuff.view = glm::lookAt(Vec3(2.0f, 2.0f, 2.0f),
									   Vec3(0.0f, 0.0f, 0.0f),
									   Vec3(0.0f, 0.0f, 1.0f));
	    uniformBuff.proj = glm::perspective(glm::radians(45.0f),
	                         (float)RenderContext::Get()->GetSwapChain()->GetSwapExtent().width /
	                             (float)gfxEngine->Get()->GetSwapChain()->GetSwapExtent().height, 0.1f, 10.0f);
	    uniformBuff.proj[1][1] *= -1;

        /// Check if currentImage is within valid range
        if (currentImage < uniformBuffersAllocation.size())
        {
            void *data = MemoryAllocator::MapMemory<void>(uniformBuffersAllocation[currentImage]);
            memcpy(data, &uniformBuff, sizeof(uniformBuff));
            MemoryAllocator::UnmapMemory(uniformBuffersAllocation[currentImage]);
        }
        else
            SEDX_CORE_ERROR("Attempting to update uniform buffer with invalid frame index");
    }
    */
	
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
        RenderData renderData;
        uniformBuffers.resize(renderData.framesInFlight);
        uniformBuffersMemory.resize(renderData.framesInFlight);
        uniformBuffersAllocation.resize(renderData.framesInFlight);
	
	    for (size_t i = 0; i < renderData.framesInFlight; i++)
	    {
            constexpr VkDeviceSize bufferSize = sizeof(UBO);
            
            /// Create buffer info for a uniform buffer
            VkBufferCreateInfo bufferInfo;
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = bufferSize;
            bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            
            /// Allocate the buffer using the memory allocator
            uniformBuffersAllocation[i] = allocation;
            //AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, uniformBuffers[i]);
	    }
	}

    void UniformBuffer::SetRenderThreadData(const void *data, uint32_t size, uint32_t offset)
    {
        MemoryAllocator allocator("UniformBuffer");
        uint8_t *pData = allocator.MapMemory<uint8_t>(allocation);
        memcpy(pData, (const uint8_t *)data + offset, size);
        MemoryAllocator::UnmapMemory(allocation);
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
	void UniformBuffer::Create(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory)
    {
        const auto device = RenderContext::GetCurrentDevice()->GetDevice();

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

        /// -------------------------------------------------------

        VkMemoryAllocateInfo allocInfo;
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.pNext = nullptr;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = 0;

		CreateBuffer(size, BufferUsage::Uniform, MemoryType::GPU, "UniformBuffer");

	    VkBufferCreateInfo bufferInfo{};
	    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	    bufferInfo.size = size;
	    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		MemoryAllocator allocator("UniformBuffer");
	    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
	        SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to create buffer!");
	
	    /// -------------------------------------------------------

	    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
	        SEDX_CORE_ERROR_TAG("Memory Allocator", "Failed to allocate buffer memory!");
	
	    vkBindBufferMemory(device, buffer, bufferMemory, 0);
	}

}

/// -------------------------------------------------------

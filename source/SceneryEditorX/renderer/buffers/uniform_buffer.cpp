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
#include <SceneryEditorX/renderer/render_context.h>
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/renderer/buffers/uniform_buffer.h>

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

		InvalidateRenderThread();
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
    // Legacy GLM-based Update implementation removed. Use renderer subsystem specific
    // update paths and camera/projection helpers instead of direct per-frame manual code here.

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

    // ReSharper disable once CppParameterMayBeConst
    // ReSharper disable once CppMemberFunctionMayBeConst
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

    void UniformBuffer::Release()
    {
        if (!allocation)
            return;

        Renderer::SubmitResourceFree([buffer = buffer, memoryAlloc = allocation]() {
            MemoryAllocator allocator("UniformBuffer");
            allocator.DestroyBuffer(buffer, memoryAlloc);
        });

        buffer = nullptr;
        allocation = nullptr;

        delete[] localMemAlloc;
        localMemAlloc = nullptr;
    }

    void UniformBuffer::InvalidateRenderThread()
	{
        VkDevice device = RenderContext::GetCurrentDevice()->GetDevice();

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.pNext = nullptr;
        allocInfo.allocationSize = 0;
        allocInfo.memoryTypeIndex = 0;

        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bufferInfo.size = size;

        MemoryAllocator allocator("UniformBuffer");
        allocation = allocator.AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, buffer);

        descriptorInfo.buffer = buffer;
        descriptorInfo.offset = 0;
        descriptorInfo.range = size;

		/// Invalidate the render thread data to ensure it is up-to-date
		if (localMemAlloc)
		{
			MemoryAllocator allocator("UniformBuffer");
            MemoryAllocator::UnmapMemory(allocation);
			allocation = nullptr;
		}
    }

}

/// -------------------------------------------------------

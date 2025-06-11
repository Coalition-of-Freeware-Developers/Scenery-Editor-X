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
#include <GraphicsEngine/buffers/buffer_data.h>
#include <GraphicsEngine/buffers/uniform_buffer.h>
#include <GraphicsEngine/vulkan/vk_buffers.h>

/// --------------------------------------------

namespace SceneryEditorX
{
    /**
     * @brief Constructor for UniformBuffer
     * 
     * Initializes the UniformBuffer instance by setting up the
     * memory allocator reference from the graphics engine.
     */
    UniformBuffer::UniformBuffer(uint32_t index) : index(index)
    {
        allocator = CreateRef<MemoryAllocator>();
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
        for (size_t i = 0; i < uniformBuffers.size(); i++)
        {
            if (uniformBuffers[i].resource && uniformBuffers[i].resource->buffer != VK_NULL_HANDLE)
            {
                allocator->DestroyBuffer(uniformBuffers[i].resource->buffer, uniformBuffersAllocation[i]);
            }
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
	void UniformBuffer::Update(const uint32_t currentImage) const
	{
	    INTERNAL auto startTime = std::chrono::high_resolution_clock::now();
	    const auto currentTime = std::chrono::high_resolution_clock::now();
	    const float time = std::chrono::duration<float>(currentTime - startTime).count();
	
	    UBO uniformBuff = {};
	    uniformBuff.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), Vec3(0.0f, 0.0f, 1.0f));
	    uniformBuff.view = glm::lookAt(Vec3(2.0f, 2.0f, 2.0f),
									   Vec3(0.0f, 0.0f, 0.0f),
									   Vec3(0.0f, 0.0f, 1.0f));
	    uniformBuff.proj = glm::perspective(glm::radians(45.0f), 1.0f,  // Using a default aspect ratio as swapChain not available
	                         0.1f, 10.0f);
	    uniformBuff.proj[1][1] *= -1;

        /// Check if currentImage is within valid range
        if (currentImage < uniformBuffersAllocation.size())
        {
            void *data = allocator->MapMemory<void>(uniformBuffersAllocation[currentImage]);
            memcpy(data, &uniformBuff, sizeof(uniformBuff));
            MemoryAllocator::UnmapMemory(uniformBuffersAllocation[currentImage]);
        }
        else
        {
            SEDX_CORE_ERROR("Attempting to update uniform buffer with invalid frame index");
        }
	}

    /**
     * @brief Retrieves the Vulkan buffer handle for a specific frame index
     * 
     * This method provides access to the uniform buffer for a particular frame
     * in the frame cycle. It performs bounds checking to ensure that only valid
     * buffer indices are accessed.
     * 
     * @param index The frame index to retrieve the buffer for (should be < framesInFlight)
     * @return Buffer The uniform buffer object for the specified frame, or 
     *         a null buffer (VK_NULL_HANDLE) if the index is out of range
     */
    Buffer UniformBuffer::GetBuffer(uint32_t index) const
    {
        if (index < uniformBuffers.size())
            return uniformBuffers[index];
        
        /// Return a Buffer with a null resource
        return Buffer{};
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
        RenderData renderData = {};
        uniformBuffers.resize(renderData.framesInFlight);
        uniformBuffersMemory.resize(renderData.framesInFlight);
        uniformBuffersAllocation.resize(renderData.framesInFlight);
	
	    for (size_t i = 0; i < renderData.framesInFlight; i++)
	    {
            constexpr VkDeviceSize bufferSize = sizeof(UBO);
            uniformBuffers[i] = CreateBuffer(bufferSize, BufferUsage::Uniform, MemoryType::CPU, "Uniform Buffer " + std::to_string(i));
            uniformBuffersAllocation[i] = uniformBuffers[i].resource->allocation;
	    }
	}
}

/// -------------------------------------------------------

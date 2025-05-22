/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_index_buffer.cpp
* -------------------------------------------------------
* Created: 18/5/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/vulkan/buffers/buffer_data.h>
#include <SceneryEditorX/vulkan/buffers/index_buffer.h>
#include <SceneryEditorX/vulkan/vk_buffers.h>

/// --------------------------------------------

namespace SceneryEditorX
{

    void IndexBuffer::CreateIndexBuffer() const
    {
        //const VkDevice vkDevice = gfxEngine->get()->GetLogicDevice()->GetDevice();
        const VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
        const VkBuffer stagingBuffer = nullptr;
        const VmaAllocation stagingBufferAllocation = nullptr;

		/// -----------------------------------

        CreateBuffer(bufferSize, BufferUsage::TransferSrc, MemoryType::CPU, "IndexStaging#");

        void* data = allocator->MapMemory<void>(stagingBufferAllocation);
        memcpy(data, indices.data(), (size_t)bufferSize);
        allocator->UnmapMemory(stagingBufferAllocation);

		//TODO: Add back the UUID when fully implemented.
        CreateBuffer(bufferSize, BufferUsage::Index | BufferUsage::AccelerationStructureInput | BufferUsage::Storage, MemoryType::GPU, "IndexBuffer#" /*+ std::to_string(asset->uuid)*/);

        CopyBuffer(stagingBuffer, indexBuffer, bufferSize);

		/// -----------------------------------

        allocator->DestroyBuffer(stagingBuffer, stagingBufferAllocation);
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
        //const VkDevice vkDevice = gfxEngine->get()->GetLogicDevice()->GetDevice();
        for (size_t i = 0; i < RenderData::framesInFlight; i++)
        {
            allocator->DestroyBuffer(indexBuffer, indexBufferAllocation);
        }
        indexBuffer = nullptr;
        indexBufferAllocation = nullptr;
    }

}

/// --------------------------------------------

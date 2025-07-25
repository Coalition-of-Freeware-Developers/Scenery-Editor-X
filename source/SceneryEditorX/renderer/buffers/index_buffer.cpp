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
#include <SceneryEditorX/renderer/buffers/index_buffer.h>
#include <SceneryEditorX/renderer/vulkan/vk_buffers.h>

/// --------------------------------------------

namespace SceneryEditorX
{
    void IndexBuffer::Create() const
    {
        const VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
        const VkBuffer stagingBuffer = nullptr;
        const VmaAllocation stagingBufferAllocation = nullptr;

		/// --------------------------------------------

        CreateBuffer(bufferSize, BufferUsage::TransferSrc, MemoryType::CPU, "IndexStaging#");

        void* data = allocator->MapMemory<void>(stagingBufferAllocation);
        memcpy(data, indices.data(), (size_t)bufferSize);
        MemoryAllocator::UnmapMemory(stagingBufferAllocation);

		//TODO: Add back the UUID when fully implemented.
        CreateBuffer(bufferSize, BufferUsage::Index | BufferUsage::AccelerationStructureInput | BufferUsage::Storage, MemoryType::GPU, "IndexBuffer#" /*+ std::to_string(asset->uuid)*/);

        CopyBuffer(stagingBuffer, indexBuffer, bufferSize);

		/// --------------------------------------------

        allocator->DestroyBuffer(stagingBuffer, stagingBufferAllocation);
    }

    /// --------------------------------------------

    IndexBuffer::IndexBuffer() = default;

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
        RenderData renderData;
        for (size_t i = 0; i < renderData.framesInFlight; i++)
        {
            allocator->DestroyBuffer(indexBuffer, indexBufferAllocation);
        }
        indexBuffer = nullptr;
        indexBufferAllocation = nullptr;
    }

    Ref<IndexBuffer> IndexBuffer::Create(const void* data, uint64_t size)
    {
        // Create a new IndexBuffer instance
        Ref<IndexBuffer> indexBuffer = CreateRef<IndexBuffer>();
        
        // Set up the indices data from the raw data
        const uint32_t* indexData = static_cast<const uint32_t*>(data);
        uint32_t indexCount = static_cast<uint32_t>(size / sizeof(uint32_t));
        
        indexBuffer->indices.resize(indexCount);
        memcpy(indexBuffer->indices.data(), indexData, size);
        
        // Create the actual buffer on the GPU
        indexBuffer->Create();
        
        return indexBuffer;
    }

}

/// --------------------------------------------

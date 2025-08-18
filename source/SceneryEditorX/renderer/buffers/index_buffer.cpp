/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* index_buffer.cpp
* -------------------------------------------------------
* Created: 18/5/2025
* -------------------------------------------------------
*/
#include "index_buffer.h"
#include "SceneryEditorX/renderer/vulkan/vk_buffers.h"

/// --------------------------------------------

namespace SceneryEditorX
{

    ///TODO: Evaluate if this is how it should be done or the other methods bellow.
    ///TODO: Need to refactor to properly utilize the custom MemoryAllocator and Buffer functions.
    /*
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
        CreateBuffer(bufferSize, BufferUsage::Index | BufferUsage::AccelerationStructureInput | BufferUsage::Storage, MemoryType::GPU, "IndexBuffer#" /*+ std::to_string(asset->uuid)#1#);

        CopyBuffer(stagingBuffer, indexBuffer, bufferSize);

        allocator->DestroyBuffer(stagingBuffer, stagingBufferAllocation);
    }
    */

    /// --------------------------------------------

    IndexBuffer::IndexBuffer(uint64_t size) : indices(size / sizeof(uint32_t))
    {
        uint64_t bufferSize = size;
        m_GpuBuffer = CreateBuffer(bufferSize, BufferUsage::Index, MemoryType::GPU, "IndexBuffer");
    }

    /// --------------------------------------------

    IndexBuffer::IndexBuffer(const void *data, uint64_t size) : indices(size / sizeof(uint32_t))
    {
        m_LocalData.Allocate(size);
        if (data && size)
            memcpy(m_LocalData.data, data, size);

        Ref<IndexBuffer> instance(this);
        Renderer::Submit([instance]() mutable
        {
            uint64_t bufferSize = instance->m_LocalData.size;
            Buffer staging = CreateBuffer(bufferSize, BufferUsage::TransferSrc, MemoryType::CPU, "IndexStaging");
            if (void* mapped = MapBuffer(staging))
            {
                memcpy(mapped, instance->m_LocalData.data, bufferSize);
                UnmapBuffer(staging);
            }
            instance->m_GpuBuffer = CreateBuffer(bufferSize, BufferUsage::Index, MemoryType::GPU, "IndexBuffer");
            CopyBuffer(staging.resource->buffer, instance->m_GpuBuffer.resource->buffer, bufferSize);
        });
    }

}

/// --------------------------------------------

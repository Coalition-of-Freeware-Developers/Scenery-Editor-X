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
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/renderer/vulkan/vk_data.h>
#include <SceneryEditorX/renderer/vulkan/vk_buffers.h>
#include <SceneryEditorX/renderer/buffers/index_buffer.h>

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

    IndexBuffer::IndexBuffer(uint64_t size) : indices(size)
    {
        const VkDeviceSize bufferSize = size;
        const VkBuffer stagingBuffer = nullptr;
        const VmaAllocation stagingBufferAllocation = nullptr;

        /// --------------------------------------------

        CreateBuffer(bufferSize, BufferUsage::TransferSrc, MemoryType::CPU, "IndexStaging#");

        void *data = allocator->MapMemory<void>(stagingBufferAllocation);
        memcpy(data, indices.data(), bufferSize);
        MemoryAllocator::UnmapMemory(stagingBufferAllocation);

        ///TODO: Add back the UUID when fully implemented.
        CreateBuffer(bufferSize, BufferUsage::Index | BufferUsage::AccelerationStructureInput | BufferUsage::Storage,
			MemoryType::GPU,"IndexBuffer#" /*+ std::to_string(asset->uuid)*/);

        CopyBuffer(stagingBuffer, indexBuffer, bufferSize);

        allocator->DestroyBuffer(stagingBuffer, stagingBufferAllocation);
    }

    /// --------------------------------------------

    IndexBuffer::IndexBuffer(const void *data, uint64_t size) : indices(size)
    {
        m_LocalData = Buffer::Copy(data, size);
		Ref<IndexBuffer> instance(this);

        /// --------------------------------------------

        Renderer::Submit([instance]() mutable
        {
        auto device = RenderContext::GetCurrentDevice();
        auto memoryAllocator = CreateRef<MemoryAllocator>("IndexBuffer");
#define USE_STAGING 1
#if USE_STAGING
            VkBufferCreateInfo bufferCreateInfo{};
            bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferCreateInfo.size = sizeof(instance->indices[0]) * instance->indices.size();
            bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            VkBuffer stagingBuffer;
            VmaAllocation stagingBufferAllocation = memoryAllocator->AllocateBuffer(bufferCreateInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, stagingBuffer);

            /// Copy data to staging buffer
            uint8_t *destData = memoryAllocator->MapMemory<uint8_t>(stagingBufferAllocation);
            memcpy(destData, instance->m_LocalData.data, instance->m_LocalData.size);
            MemoryAllocator::UnmapMemory(stagingBufferAllocation);

            VkBufferCreateInfo indexBufferCreateInfo = {};
            indexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            indexBufferCreateInfo.size = sizeof(instance->indices[0]) * instance->indices.size();
            indexBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            instance->indexBufferAllocation = memoryAllocator->AllocateBuffer(indexBufferCreateInfo, VMA_MEMORY_USAGE_GPU_ONLY, instance->indexBuffer);
            instance->allocator = memoryAllocator;

            VkCommandBuffer copyCmd = device->GetCommandBuffer(true);

            VkBufferCopy copyRegion = {};
            copyRegion.size = instance->m_LocalData.size;
            vkCmdCopyBuffer(copyCmd, stagingBuffer, instance->indexBuffer, 1, &copyRegion);

            device->FlushCmdBuffer(copyCmd);

            memoryAllocator->DestroyBuffer(stagingBuffer, stagingBufferAllocation);
#else
            VkBufferCreateInfo indexbufferCreateInfo = {};
            indexbufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            indexbufferCreateInfo.size = instance->m_Size;
            indexbufferCreateInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

            auto bufferAlloc = memoryAllocator->AllocateBuffer(indexbufferCreateInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, instance->m_VulkanBuffer);

            void *dstBuffer = memoryAllocator->MapMemory<void>(bufferAlloc);
            memcpy(dstBuffer, instance->m_LocalData.Data, instance->m_Size);
            memoryAllocator->UnmapMemory(bufferAlloc);
#endif
        });
    };

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

    /// --------------------------------------------

    /*
    Ref<IndexBuffer> IndexBuffer::Create(const void* data, uint64_t size)
    {
        /// Create a new IndexBuffer instance
        Ref<IndexBuffer> indexBuffer = CreateRef<IndexBuffer>();
        
        /// Set up the indices data from the raw data
        const uint32_t* indexData = static_cast<const uint32_t*>(data);
        uint32_t indexCount = static_cast<uint32_t>(size / sizeof(uint32_t));
        
        indexBuffer->indices.resize(indexCount);
        memcpy(indexBuffer->indices.data(), indexData, size);
        
        /// Create the actual buffer on the GPU
        CreateRef<IndexBuffer>(indexBuffer);
        return indexBuffer;
    }
    */

}

/// --------------------------------------------

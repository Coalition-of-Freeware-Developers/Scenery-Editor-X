/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* storage_buffer.cpp
* -------------------------------------------------------
* Created: 22/6/2025
* -------------------------------------------------------
*/
#include "storage_buffer.h"
#include "SceneryEditorX/renderer/render_context.h"
#include "SceneryEditorX/renderer/renderer.h"

/// -----------------------------------

namespace SceneryEditorX
{
	StorageBuffer::StorageBuffer(uint32_t size, const StorageBufferSpec &spec) : m_spec(spec), size(size)
	{
        Ref<StorageBuffer> instance(this);
        Renderer::Submit([instance]() mutable { instance->InvalidateRenderThread(); });
	}
	
	StorageBuffer::~StorageBuffer()
	{
        Release();
	}
	
	void StorageBuffer::SetData(const void *data, uint32_t size, uint32_t offset)
	{
        memcpy(localStorage, data, size);
        Ref<StorageBuffer> instance(this);
        Renderer::Submit([instance, size, offset]() mutable { instance->SetRenderThreadData(instance->localStorage, size, offset); });
	}
	
	void StorageBuffer::SetRenderThreadData(const void *data, uint32_t size, uint32_t offset) const
    {
        /// Cannot call SetData if GPU only
        SEDX_CORE_VERIFY(!m_spec.GPUOnly);

	#if NO_STAGING
	        MemoryAllocator allocator("StorageBuffer");
	        uint8_t *pData = allocator.MapMemory<uint8_t>(alloctor);
	        memcpy(pData, data, size);
	        MemoryAllocator::UnmapMemory(alloctor);
	#else
	
	        MemoryAllocator allocator("Staging");
	        uint8_t *pData = allocator.MapMemory<uint8_t>(m_StagingAlloc);
	        memcpy(pData, data, size);
	        MemoryAllocator::UnmapMemory(m_StagingAlloc);
	
	        VkCommandBuffer commandBuffer = RenderContext::GetCurrentDevice()->GetCommandBuffer(true);
	        VkBufferCopy copyRegion = {0, offset, size};
	        vkCmdCopyBuffer(commandBuffer, stagingBuffer, m_buffer, 1, &copyRegion);
	
	        RenderContext::GetCurrentDevice()->FlushCmdBuffer(commandBuffer);
	#endif
	}
	
	void StorageBuffer::Resize(uint32_t newSize)
	{
        size = newSize;
        Ref<StorageBuffer> instance(this);
        Renderer::Submit([instance]() mutable { instance->InvalidateRenderThread(); });
	}
	
	void StorageBuffer::Release()
	{
        if (!alloctor)
            return;

        Renderer::SubmitResourceFree([buffer = m_buffer, memoryAlloc = alloctor, stagingAlloc = m_StagingAlloc, stagingBuffer = stagingBuffer]() {
            MemoryAllocator allocator("StorageBuffer");
            allocator.DestroyBuffer(buffer, memoryAlloc);
            if (stagingBuffer)
                allocator.DestroyBuffer(stagingBuffer, stagingAlloc);
        });

        m_buffer = nullptr;
        alloctor = nullptr;
	}
	
	void StorageBuffer::InvalidateRenderThread()
	{
        Release();

        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        bufferInfo.size = size;

        MemoryAllocator allocator("StorageBuffer");
        alloctor = allocator.AllocateBuffer(bufferInfo, m_spec.GPUOnly ? VMA_MEMORY_USAGE_GPU_ONLY : VMA_MEMORY_USAGE_CPU_TO_GPU, m_buffer);

        m_descriptInfo.buffer = m_buffer;
        m_descriptInfo.offset = 0;
        m_descriptInfo.range = size;

#if 0
		VkBufferCreateInfo stagingBufferInfo = {};
		stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		stagingBufferInfo.size = size;

		m_StagingAlloc = allocator.AllocateBuffer(stagingBufferInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, stagingBuffer);
#endif
	}
	
}

/// ----------------------------------

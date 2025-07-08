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
#include <SceneryEditorX/renderer/render_context.h>
#include <SceneryEditorX/renderer/buffers/storage_buffer.h>

/// -----------------------------------

namespace SceneryEditorX
{
	StorageBuffer::StorageBuffer(uint32_t size, StorageBufferSpec spec) : m_spec(std::move(spec)), size(size)
	{
        Ref<StorageBuffer> instance(this);
        Renderer::Submit([instance]() mutable { instance->Invalidate_RenderThread(); });
	}
	
	StorageBuffer::~StorageBuffer()
	{
        Release();
	}
	
	void StorageBuffer::SetData(const void *data, uint32_t size, uint32_t offset)
	{
        memcpy(localStorage, data, size);
        Ref<StorageBuffer> instance(this);
        Renderer::Submit([instance, size, offset]() mutable { instance->SetData_RenderThread(instance->localStorage, size, offset); });
	}
	
	void StorageBuffer::SetData_RenderThread(const void *data, uint32_t size, uint32_t offset)
	{
        /// Cannot call SetData if GPU only
        SEDX_CORE_VERIFY(!m_spec.GPUOnly);

	#if NO_STAGING
	        MemoryAllocator allocator("StorageBuffer");
	        uint8_t *pData = allocator.MapMemory<uint8_t>(memAlloc);
	        memcpy(pData, data, size);
	        MemoryAllocator::UnmapMemory(memAlloc);
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
        Renderer::Submit([instance]() mutable { instance->Invalidate_RenderThread(); });
	}
	
	void StorageBuffer::Release()
	{
        if (!memAlloc)
            return;

        Renderer::SubmitResourceFree([buffer = m_buffer, memoryAlloc = memAlloc, stagingAlloc = m_StagingAlloc, stagingBuffer = stagingBuffer]() {
            MemoryAllocator allocator("StorageBuffer");
            allocator.DestroyBuffer(buffer, memoryAlloc);
            if (stagingBuffer)
                allocator.DestroyBuffer(stagingBuffer, stagingAlloc);
        });

        m_buffer = nullptr;
        memAlloc = nullptr;
	}
	
	void StorageBuffer::Invalidate_RenderThread()
	{
        Release();

        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        bufferInfo.size = size;

        MemoryAllocator allocator("StorageBuffer");
        memAlloc = allocator.AllocateBuffer(bufferInfo, m_spec.GPUOnly ? VMA_MEMORY_USAGE_GPU_ONLY : VMA_MEMORY_USAGE_CPU_TO_GPU, m_buffer);

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

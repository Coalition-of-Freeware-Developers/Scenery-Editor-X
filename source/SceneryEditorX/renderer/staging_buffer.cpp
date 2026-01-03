/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* staging_buffer.cpp
* -------------------------------------------------------
* Created: 22/12/2025
* -------------------------------------------------------
*/
#include "staging_buffer.h"

#include "renderer.h"

// -------------------------------------------------------

namespace SceneryEditorX
{
	
	StagingBuffer::StagingBuffer(uint32_t size, StagingBufferSpec spec) : m_Spec(std::move(spec)), m_Size(size)
	{
        Allocate();
	}

	StagingBuffer::~StagingBuffer()
	{
        SEDX_CORE_TRACE_TAG("Staging Buffer", "Destroying staging buffer with size of{}", GetSize());
        m_Buffer.Release();	// Release local data
	}

	void StagingBuffer::SetData(const void *data, uint32_t size, uint32_t offset)
	{
        Buffer copy;
        if (size)
        {
            copy.Allocate(size);
            memcpy(copy.data, data, size);
        }

        Ref<StagingBuffer> instance(this);
        Renderer::Submit([instance, size, offset, copy]() mutable
        {
            if (size == 0)
                return;
            instance->SetRenderThreadData(copy.data, size, offset);
        });
	}

	void StagingBuffer::SetRenderThreadData(const void *data, uint32_t size, uint32_t offset) const
	{
        SEDX_CORE_ASSERT(offset + size <= m_Size, "StagingBuffer::SetRenderThreadData out of range");
        if (m_Buffer.memory & MemoryType::CPU)
        {
            if (void *mapped = MapBuffer(m_Buffer))
            {
                memcpy(static_cast<uint8_t *>(mapped) + offset, data, size);
                UnmapBuffer(m_Buffer);
            }
        }
        else
        {
            Buffer staging = CreateBuffer(size, BufferUsage::TransferSrc, MemoryType::CPU, "StorageStaging");
            if (void *mapped = MapBuffer(staging))
            {
                memcpy(mapped, data, size);
                UnmapBuffer(staging);
                CopyBufferRegion(staging.resource->buffer, m_Buffer.resource->buffer, size, 0, offset);
            }
        }
	}

	void StagingBuffer::Resize(uint32_t newSize)
	{
        m_Size = newSize;
        Allocate();
	}

	Ref<StagingBuffer> StagingBuffer::Get()
	{
        uint64_t frame = Renderer::GetCurrentFrameIndex();
        return Get(frame);
	}

	Ref<StagingBuffer> StagingBuffer::Get(uint64_t frame)
	{

	}

	Ref<StagingBuffer> StagingBuffer::GetRenderThread()
	{
        uint64_t frame = Renderer::GetCurrentRenderThreadFrameIndex();
        return Get(frame);
	}

    void StagingBuffer::Allocate()
    {
        Ref<StagingBuffer> instance(this);
        Renderer::Submit([instance]() mutable { instance->AllocateRenderThread(); });
    }

    void StagingBuffer::AllocateRenderThread()
    {
        MemoryFlags mem = m_Spec.GPUOnly ? MemoryType::GPU : MemoryType::CPU;
        m_Buffer = CreateBuffer(m_Size, BufferUsage::Storage, mem, m_Spec.debugName.empty() ? "StorageBuffer" : m_Spec.debugName);
        UpdateDescriptor();
    }

    void StagingBuffer::UpdateDescriptor()
    {
        m_DescriptorInfo.buffer = m_Buffer.resource->buffer;
        m_DescriptorInfo.offset = 0;
        m_DescriptorInfo.range = m_Size;
    }

} // namespace SceneryEditorX

// -------------------------------------------------------

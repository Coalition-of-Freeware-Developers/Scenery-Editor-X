/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* storage_buffer.cpp
* -------------------------------------------------------
* Created: 7/7/2025
* -------------------------------------------------------
*/
#include "storage_buffer.h"
#include <SceneryEditorX/renderer/renderer.h>
#include "SceneryEditorX/renderer/vulkan/vk_buffers.h"

/// ----------------------------------------------------------

namespace SceneryEditorX
{
	StorageBuffer::StorageBuffer(uint32_t size, const StorageBufferSpec &spec) : m_Spec(spec), m_Size(size)
	{
		Allocate();
	}

	void StorageBuffer::Allocate()
	{
		Ref<StorageBuffer> instance(this);
		Renderer::Submit([instance]() mutable { instance->AllocateRenderThread(); });
	}

	void StorageBuffer::AllocateRenderThread()
	{
		MemoryFlags mem = m_Spec.GPUOnly ? MemoryType::GPU : MemoryType::CPU;
		m_Buffer = CreateBuffer(m_Size, BufferUsage::Storage, mem, m_Spec.debugName.empty() ? "StorageBuffer" : m_Spec.debugName);
		UpdateDescriptor();
	}

	void StorageBuffer::UpdateDescriptor()
	{
		m_DescriptorInfo.buffer = m_Buffer.resource->buffer;
		m_DescriptorInfo.offset = 0;
		m_DescriptorInfo.range = m_Size;
	}

	void StorageBuffer::SetRenderThreadData(const void* data, uint32_t size, uint32_t offset)
	{
		SEDX_CORE_ASSERT(offset + size <= m_Size, "StorageBuffer::SetRenderThreadData out of range");
		if (m_Buffer.memory & MemoryType::CPU)
		{
			if (void* mapped = MapBuffer(m_Buffer))
			{
				memcpy(static_cast<uint8_t*>(mapped) + offset, data, size);
				UnmapBuffer(m_Buffer);
			}
		}
		else
		{
			Buffer staging = CreateBuffer(size, BufferUsage::TransferSrc, MemoryType::CPU, "StorageStaging");
			if (void* mapped = MapBuffer(staging))
			{
				memcpy(mapped, data, size);
				UnmapBuffer(staging);
				CopyBufferRegion(staging.resource->buffer, m_Buffer.resource->buffer, size, 0, offset);
			}
		}
	}

	void StorageBuffer::SetData(const void* data, uint32_t size, uint32_t offset)
	{
		Buffer copy;
	    if (size)
		{
		    copy.Allocate(size); memcpy(copy.data, data, size);
		}

		Ref<StorageBuffer> instance(this);
		Renderer::Submit([instance, size, offset, copy]() mutable {
		    if (size == 0) return; instance->SetRenderThreadData(copy.data, size, offset);
		});
	}

	void StorageBuffer::Resize(uint32_t newSize)
	{
		m_Size = newSize;
		Allocate();
	}

    /// -------------------------------------------------------

    StorageBufferSet::StorageBufferSet(const StorageBufferSpec &spec, uint32_t size, uint32_t framesInFlight)
        : m_spec(spec), m_framesInFlight(framesInFlight)
    {
        if (framesInFlight == 0)
            m_framesInFlight = Renderer::GetRenderData().framesInFlight;

        for (uint32_t frame = 0; frame < m_framesInFlight; frame++)
            storageBuffers[frame] = CreateRef<StorageBuffer>(size, const_cast<StorageBufferSpec &>(m_spec));
    }

    Ref<StorageBuffer> StorageBufferSet::Get()
    {
        uint32_t frame = Renderer::GetCurrentFrameIndex();
        return Get(frame);
    }

    Ref<StorageBuffer> StorageBufferSet::Get(uint32_t frame)
    {
        SEDX_CORE_ASSERT(storageBuffers.contains(frame), "Frame index out of range");
        return storageBuffers.at(frame);
    }

    Ref<StorageBuffer> StorageBufferSet::GetRenderThread()
    {
        uint32_t frame = Renderer::GetCurrentRenderThreadFrameIndex();
        return Get(frame);
    }

    void StorageBufferSet::Set(Ref<StorageBuffer> storageBuffer, uint32_t frame)
    {
        storageBuffers[frame] = storageBuffer;
    }

    void StorageBufferSet::Resize(uint32_t newSize) const
    {
        for (uint32_t frame = 0; frame < m_framesInFlight; frame++)
            storageBuffers.at(frame)->Resize(newSize);
    }

}

/// ----------------------------------------------------------

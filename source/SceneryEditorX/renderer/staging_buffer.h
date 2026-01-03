/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* staging_buffer.h
* -------------------------------------------------------
* Created: 22/12/2025
* -------------------------------------------------------
*/
#pragma once
#include "memory_allocator.h"

// -------------------------------------------------------

namespace SceneryEditorX
{
    struct StagingBufferSpec
	{
        bool GPUOnly = true;
		std::string debugName;
    };

    class StagingBuffer : public RefCounted
	{
	public:
        StagingBuffer(uint32_t size, StagingBufferSpec spec);
        virtual ~StagingBuffer() override;

        void SetData(const void *data, uint32_t size, uint32_t offset = 0);
        void SetRenderThreadData(const void *data, uint32_t size, uint32_t offset = 0) const;
        void Resize(uint32_t newSize);

        Ref<StagingBuffer> Get();
        Ref<StagingBuffer> Get(uint64_t frame);
        Ref<StagingBuffer> GetRenderThread();

        VkBuffer GetBuffer() const { return m_Buffer.resource ? m_Buffer.resource->buffer : VK_NULL_HANDLE; }
		uint32_t GetSize() const { return m_Size; }

	private:
        void Allocate();
        void AllocateRenderThread();
        void UpdateDescriptor();

        StagingBufferSpec m_Spec;
        uint32_t m_Size = 0;
        Buffer m_Buffer;
        VmaAllocation m_MemoryAllocation;
        VkDescriptorBufferInfo m_DescriptorInfo{};
	};

}

// -------------------------------------------------------


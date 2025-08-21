/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* storage_buffer.h
* -------------------------------------------------------
* Created: 22/6/2025
* -------------------------------------------------------
*/
#pragma once
#include "SceneryEditorX/renderer/vulkan/vk_buffers.h"

/// -----------------------------------

namespace SceneryEditorX
{

	struct StorageBufferSpec
	{
	    bool GPUOnly = true;
	    std::string debugName;
	};

    /// -------------------------------------------------------

    class StorageBuffer : public RefCounted
    {
    public:
        StorageBuffer(uint32_t size, const StorageBufferSpec &spec);
        virtual ~StorageBuffer() override; // RAII via Buffer

        void SetData(const void* data, uint32_t size, uint32_t offset = 0);
        void SetRenderThreadData(const void* data, uint32_t size, uint32_t offset = 0);
        void Resize(uint32_t newSize);

        [[nodiscard]] VkBuffer GetBuffer() const { return m_Buffer.resource ? m_Buffer.resource->buffer : VK_NULL_HANDLE; }
        [[nodiscard]] uint32_t GetSize() const { return m_Size; }
        [[nodiscard]] const VkDescriptorBufferInfo& GetDescriptorInfo() const { return m_DescriptorInfo; }

    private:
        void Allocate();
        void AllocateRenderThread();
        void UpdateDescriptor();

        StorageBufferSpec m_Spec;
        uint32_t m_Size = 0;
        Buffer m_Buffer; // underlying storage buffer
        VkDescriptorBufferInfo m_DescriptorInfo{};
    };

    /// ----------------------------------------------------------

    class StorageBufferSet : public RefCounted
    {
    public:
        explicit StorageBufferSet(const StorageBufferSpec &spec, uint32_t size, uint32_t framesInFlight);
        virtual ~StorageBufferSet() override;

        Ref<StorageBuffer> Get();
        Ref<StorageBuffer> Get(uint32_t frame);
        Ref<StorageBuffer> GetRenderThread();
        void Set(Ref<StorageBuffer> storageBuffer, uint32_t frame);
        void Resize(uint32_t newSize) const;

        /// -------------------------------------------------------

        //static Ref<StorageBufferSet> Create(const StorageBufferSpec &spec, uint32_t size, uint32_t framesInFlight = 0);

        /// -------------------------------------------------------

    private:
        StorageBufferSpec m_spec;
        uint32_t m_framesInFlight = 0;
        std::map<uint32_t, Ref<StorageBuffer>> storageBuffers;
    };

}

/// ----------------------------------

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
#include "SceneryEditorX/renderer/renderer.h"
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
        ~StorageBuffer() override = default; // RAII via Buffer

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

    /*
    class StorageBufferSet : public RefCounted
    {
    public:
        explicit StorageBufferSet(const StorageBufferSpec &spec, uint32_t size, uint32_t framesInFlight)
            : m_spec(spec), m_framesInFlight(framesInFlight)
        {
            if (framesInFlight == 0)
                m_framesInFlight = Renderer::GetRenderData().framesInFlight;

            for (uint32_t frame = 0; frame < m_framesInFlight; frame++)
                storageBuffers[frame] = CreateRef<StorageBuffer>(size, const_cast<StorageBufferSpec&>(m_spec));
        }

        ~StorageBufferSet() override = default;

        /// -------------------------------------------------------

        Ref<StorageBuffer> Get()
        {
            uint32_t frame = Renderer::GetCurrentFrameIndex();
            return Get(frame);
        }

        /// -------------------------------------------------------

        Ref<StorageBuffer> GetRenderThread()
        {
            uint32_t frame = Renderer::GetCurrentRenderThreadFrameIndex();
            return Get(frame);
        }

        /// -------------------------------------------------------

        Ref<StorageBuffer> Get(uint32_t frame)
        {
            SEDX_CORE_ASSERT(storageBuffers.contains(frame), "Frame index out of range");
            return storageBuffers.at(frame);
        }

        /// -------------------------------------------------------

        void Set(Ref<StorageBuffer> storageBuffer, uint32_t frame)
        {
            storageBuffers[frame] = storageBuffer;
        }

        /// -------------------------------------------------------

        void Resize(uint32_t newSize) const
        {
            for (uint32_t frame = 0; frame < m_framesInFlight; frame++)
                storageBuffers.at(frame)->Resize(newSize);
        }

        /// -------------------------------------------------------

        //static Ref<StorageBufferSet> Create(const StorageBufferSpec &spec, uint32_t size, uint32_t framesInFlight = 0);

        /// -------------------------------------------------------

    private:
        StorageBufferSpec m_spec;
        uint32_t m_framesInFlight = 0;
        std::map<uint32_t, Ref<StorageBuffer>> storageBuffers;
    };
    */

}

/// ----------------------------------

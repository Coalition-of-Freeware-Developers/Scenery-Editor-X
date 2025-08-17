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
#include "SceneryEditorX/renderer/vulkan/vk_allocator.h"

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
        ~StorageBuffer();

        void SetData(const void *data, uint32_t size, uint32_t offset = 0);
        void SetRenderThreadData(const void *data, uint32_t size, uint32_t offset = 0) const;
        void Resize(uint32_t newSize);
		VkBuffer GetBuffer() const { return m_buffer; }
		const VkDescriptorBufferInfo &GetDescriptorBufferInfo() const { return m_descriptInfo; }

    private:
        void Release();
        void InvalidateRenderThread();

        StorageBufferSpec m_spec;
        VkBuffer m_buffer {};
        VkDescriptorBufferInfo m_descriptInfo {};
        VmaAllocation alloctor = nullptr;
        uint32_t size = 0;
        std::string name;

        VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
        VmaAllocation m_StagingAlloc = nullptr;
        VkBuffer stagingBuffer = nullptr;
        uint8_t* localStorage = nullptr;
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

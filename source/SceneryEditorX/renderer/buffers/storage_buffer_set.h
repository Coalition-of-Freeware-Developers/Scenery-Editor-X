/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* storage_buffer_set.h
* -------------------------------------------------------
* Created: 26/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <map>
#include "storage_buffer.h"
#include "SceneryEditorX/logging/asserts.h"
#include "SceneryEditorX/renderer/renderer.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{

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

}

/// -------------------------------------------------------

/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* uniform_buffer_set.h
* -------------------------------------------------------
* Created: 26/7/2025
* -------------------------------------------------------
*/
#pragma once
#include "uniform_buffer.h"
#include "SceneryEditorX/renderer/renderer.h"
#include "SceneryEditorX/renderer/vulkan/vk_data.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{

	class UniformBufferSet : public RefCounted
    {
	public:
        UniformBufferSet(uint32_t size, uint32_t framesInFlight = 0) : m_framesInFlight(framesInFlight)
        {
            if (framesInFlight == 0)
                m_framesInFlight = Renderer::GetRenderData().framesInFlight;

            for (uint32_t frame = 0; frame < m_framesInFlight; frame++)
                m_UniformBuffers[frame] = CreateRef<UniformBuffer>(size);
        }

	    virtual ~UniformBufferSet() = default;

		/// ----------------------------------------------------------

        virtual Ref<UniformBuffer> Get()
        {
            const uint32_t frame = Renderer::GetCurrentFrameIndex();
            return Get(frame);
        }

        virtual Ref<UniformBuffer> GetRenderThread()
        {
            const uint32_t frame = Renderer::GetCurrentRenderThreadFrameIndex();
            return Get(frame);
        }

		virtual Ref<UniformBuffer> Get(uint32_t frame)
        {
            SEDX_CORE_ASSERT(m_UniformBuffers.contains(frame));
            return m_UniformBuffers.at(frame);
        }

        virtual void Set(Ref<UniformBuffer> uniformBuffer, uint32_t frame = 0)
        {
            m_UniformBuffers[frame] = uniformBuffer;
        }

	private:
        uint32_t m_framesInFlight = 0;
        std::map<uint32_t, Ref<UniformBuffer>> m_UniformBuffers;

    };

}

/// -------------------------------------------------------

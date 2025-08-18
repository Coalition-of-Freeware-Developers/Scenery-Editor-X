/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* uniform_buffer.cpp (refactored)
* -------------------------------------------------------
*/
#include "uniform_buffer.h"
#include "SceneryEditorX/logging/asserts.h"
#include "SceneryEditorX/renderer/renderer.h"
#include "SceneryEditorX/renderer/vulkan/vk_buffers.h"
#include "SceneryEditorX/renderer/vulkan/vk_data.h"

/// ----------------------------------------------------------

namespace SceneryEditorX
{

    UniformBuffer::UniformBuffer(uint32_t size) : m_Size(size)
    {
        Allocate();
    }

    void UniformBuffer::Allocate()
    {
        Ref<UniformBuffer> instance(this);
        Renderer::Submit([instance]() mutable { instance->AllocateRenderThread(); });
    }

    void UniformBuffer::AllocateRenderThread()
    {
        RenderData renderData;
        m_Buffers.clear();
        m_Buffers.reserve(renderData.framesInFlight);
        for (uint32_t i = 0; i < renderData.framesInFlight; ++i)
            m_Buffers.emplace_back(CreateBuffer(m_Size, BufferUsage::Uniform, MemoryType::CPU, "UniformBuffer"));

        UpdateDescriptors();
    }

    void UniformBuffer::UpdateDescriptors()
    {
        m_DescriptorInfos.resize(m_Buffers.size());
        for (size_t i = 0; i < m_Buffers.size(); ++i)
        {
            m_DescriptorInfos[i].buffer = m_Buffers[i].resource->buffer;
            m_DescriptorInfos[i].offset = 0;
            m_DescriptorInfos[i].range = m_Size;
        }
    }

    void UniformBuffer::SetRenderThreadData(const void *data, uint32_t size, uint32_t offset)
    {
        uint32_t frame = Renderer::GetCurrentFrameIndex(); // fallback if render-thread specific function unavailable
        SEDX_CORE_ASSERT(frame < m_Buffers.size());

        auto &buffer = m_Buffers[frame];
        SEDX_CORE_ASSERT(buffer.memory & MemoryType::CPU, "Uniform buffer not CPU accessible");
        SEDX_CORE_ASSERT(offset + size <= m_Size, "UniformBuffer write out of range");

        if (void* mapped = MapBuffer(buffer))
        {
            memcpy(static_cast<uint8_t*>(mapped) + offset, data, size);
            UnmapBuffer(buffer);
        }
    }

    void UniformBuffer::SetData(const void *data, uint32_t size, uint32_t offset)
    {
        Buffer local;
        if (size)
        {
            local.Allocate(size); memcpy(local.data, data, size);
        }

        Ref<UniformBuffer> instance(this);
        Renderer::Submit([instance, size, offset, local]() mutable {
            if (size == 0) return; instance->SetRenderThreadData(local.data, size, offset);
        });
    }

    const VkDescriptorBufferInfo &UniformBuffer::GetCurrentFrameDescriptor() const
    {
        const uint32_t frame = (uint32_t)Renderer::GetCurrentFrameIndex();
        SEDX_CORE_ASSERT(frame < m_DescriptorInfos.size());
        return m_DescriptorInfos[frame];
    }

    /// ----------------------------------------------------------

	UniformBufferSet::UniformBufferSet(uint32_t size, uint32_t framesInFlight) : m_framesInFlight(framesInFlight)
    {
        if (framesInFlight == 0)
            m_framesInFlight = Renderer::GetRenderData().framesInFlight;

        for (uint32_t frame = 0; frame < m_framesInFlight; frame++)
            m_UniformBuffers[frame] = CreateRef<UniformBuffer>(size);
    }

    Ref<UniformBuffer> UniformBufferSet::Get()
    {
        const uint32_t frame = Renderer::GetCurrentFrameIndex();
        return Get(frame);
    }

    Ref<UniformBuffer> UniformBufferSet::GetRenderThread()
    {
        const uint32_t frame = Renderer::GetCurrentRenderThreadFrameIndex();
        return Get(frame);
    }

    Ref<UniformBuffer> UniformBufferSet::Get(uint32_t frame)
    {
        SEDX_CORE_ASSERT(m_UniformBuffers.contains(frame));
        return m_UniformBuffers.at(frame);
    }

    void UniformBufferSet::Set(Ref<UniformBuffer> uniformBuffer, uint32_t frame)
    {
        m_UniformBuffers[frame] = uniformBuffer;
    }

}

/// ----------------------------------------------------------

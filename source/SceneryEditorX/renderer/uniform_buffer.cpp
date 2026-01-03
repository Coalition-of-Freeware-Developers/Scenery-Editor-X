/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* uniform_buffer.cpp
* -------------------------------------------------------
* Created: 10/6/2025
* -------------------------------------------------------
*/
#include "uniform_buffer.h"
#include "SceneryEditorX/logging/asserts.h"
#include "SceneryEditorX/renderer/renderer.h"
#include "SceneryEditorX/renderer/vulkan_buffers.h"
#include "SceneryEditorX/renderer/vulkan_data.h"
//#include "SceneryEditorX/renderer/bindless_descriptor_manager.h"

// ----------------------------------------------------------

namespace SceneryEditorX
{

    UniformBuffer::UniformBuffer(uint32_t size) : m_Size(size)
    {
        Invalidate_RenderThread();
    }

    UniformBuffer::~UniformBuffer()
    {
        Release();
    };

    void UniformBuffer::Release()
    {
        if (!m_MemoryAlloc)
            return;

        Renderer::SubmitResourceFree([buffer = m_Buffer, memoryAlloc = m_MemoryAlloc]()
        {
            MemoryAllocator allocator("UniformBuffer");
            allocator.DestroyBuffer(buffer, memoryAlloc);
        });

        m_Buffer = nullptr;
        m_MemoryAlloc = nullptr;

        delete[] m_LocalStorage;
        m_LocalStorage = nullptr;
    }

    void UniformBuffer::Invalidate_RenderThread()
    {
        Release();

        VkDevice device = RenderContext::GetCurrentDevice()->GetDevice();

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.pNext = nullptr;
        allocInfo.allocationSize = 0;
        allocInfo.memoryTypeIndex = 0;

        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bufferInfo.size = m_Size;

        MemoryAllocator allocator("UniformBuffer");
        m_MemoryAlloc = allocator.AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, m_Buffer);

        m_DescriptorInfo.buffer = m_Buffer;
        m_DescriptorInfo.offset = 0;
        m_DescriptorInfo.range = m_Size;

    }

    void UniformBuffer::SetRenderThreadData(const void *data, uint32_t size, uint32_t offset)
    {
        MemoryAllocator allocator("VulkanUniformBuffer");
        uint8_t *pData = allocator.MapMemory<uint8_t>(m_MemoryAlloc);
        memcpy(pData, (const uint8_t *)data + offset, size);
        allocator.UnmapMemory(m_MemoryAlloc);
    }

    void UniformBuffer::SetData(const void *data, uint32_t size, uint32_t offset)
    {
        // TODO: local storage should be potentially replaced with render thread storage
        memcpy(m_LocalStorage, data, size);
        Ref<UniformBuffer> instance = this;
        Renderer::Submit([instance, size, offset]() mutable
        {
            instance->SetRenderThreadData(instance->m_LocalStorage, size, offset);
        });
    }

    // ----------------------------------------------------------

	UniformBufferSet::UniformBufferSet(uint32_t size, uint32_t framesInFlight)
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

    Ref<UniformBuffer> UniformBufferSet::Get(uint32_t frame)
    {
        SEDX_CORE_ASSERT(m_UniformBuffers.contains(frame));
        return m_UniformBuffers.at(frame);
    }

    Ref<UniformBuffer> UniformBufferSet::GetRenderThread()
    {
        const uint32_t frame = Renderer::GetCurrentRenderThreadFrameIndex();
        return Get(frame);
    }

    void UniformBufferSet::Set(Ref<UniformBuffer> uniformBuffer, uint32_t frame)
	{
        m_UniformBuffers[frame] = uniformBuffer;
	}

}

// ----------------------------------------------------------

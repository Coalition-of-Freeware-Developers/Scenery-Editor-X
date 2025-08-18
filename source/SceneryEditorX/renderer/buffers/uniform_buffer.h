/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* uniform_buffer.h (refactored)
* -------------------------------------------------------
*/
#pragma once
#include "SceneryEditorX/renderer/renderer.h"
#include "SceneryEditorX/renderer/vulkan/vk_buffers.h"

/// ----------------------------------------------------------

namespace SceneryEditorX
{
    class UniformBuffer : public RefCounted
    {
    public:
        explicit UniformBuffer(uint32_t size);
        ~UniformBuffer() override = default; // Buffer RAII

        void SetData(const void* data, uint32_t size, uint32_t offset = 0);
        void SetRenderThreadData(const void* data, uint32_t size, uint32_t offset = 0);

        [[nodiscard]] VkBuffer GetBuffer(uint32_t frame) const { return m_Buffers[frame].resource->buffer; }
        [[nodiscard]] uint32_t GetSize() const { return m_Size; }
        [[nodiscard]] size_t GetBufferCount() const { return m_Buffers.size(); }
        [[nodiscard]] const VkDescriptorBufferInfo& GetDescriptor(uint32_t frame) const { return m_DescriptorInfos[frame]; }
        [[nodiscard]] const VkDescriptorBufferInfo& GetCurrentFrameDescriptor() const { return m_DescriptorInfos[Renderer::GetCurrentFrameIndex()]; }

    private:
        void Allocate();
        void AllocateRenderThread();
        void UpdateDescriptors();

        uint32_t m_Size = 0;
        std::vector<Buffer> m_Buffers; // per-frame uniform buffers
        std::vector<VkDescriptorBufferInfo> m_DescriptorInfos; // per-frame descriptors
    };

    /* Future: UniformBufferSet reimplementation using new Buffer API */

}

/// -------------------------------------------------------


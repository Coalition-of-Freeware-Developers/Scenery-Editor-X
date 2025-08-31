/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* uniform_buffer.h (refactored)
* -------------------------------------------------------
*/
#pragma once
#include <cstdint>
#include <vector>
#include <map>
#include "SceneryEditorX/renderer/vulkan/vk_buffers.h"


/// -------------------------------------------------------

namespace SceneryEditorX
{
	class UniformBuffer : public RefCounted
	{
	public:
	    explicit UniformBuffer(uint32_t size);
        virtual ~UniformBuffer() override = default; // Buffer RAII

	    void SetData(const void *data, uint32_t size, uint32_t offset = 0);
	    void SetRenderThreadData(const void *data, uint32_t size, uint32_t offset = 0);

	    [[nodiscard]] VkBuffer GetBuffer(uint32_t frame) const { return m_Buffers[frame].resource->buffer; }
	    [[nodiscard]] uint32_t GetSize() const { return m_Size; }
	    [[nodiscard]] size_t GetBufferCount() const { return m_Buffers.size(); }
	    [[nodiscard]] const VkDescriptorBufferInfo &GetDescriptor(uint32_t frame) const { return m_DescriptorInfos[frame]; }
	    [[nodiscard]] const VkDescriptorBufferInfo &GetCurrentFrameDescriptor() const;
		// Bindless indices (one per frame) or -1 if not registered / unsupported
		[[nodiscard]] int32_t GetBindlessIndex(uint32_t frame) const { return frame < m_BindlessIndices.size() ? m_BindlessIndices[frame] : -1; }
		[[nodiscard]] int32_t GetCurrentFrameBindlessIndex() const;

	private:
	    void Allocate();
	    void AllocateRenderThread();
	    void UpdateDescriptors();

	    uint32_t m_Size = 0;
	    std::vector<Buffer> m_Buffers;                         // per-frame uniform buffers
	    std::vector<VkDescriptorBufferInfo> m_DescriptorInfos; // per-frame descriptors
		std::vector<int32_t> m_BindlessIndices;                 // per-frame bindless indices
	};

    /// -------------------------------------------------------

	class UniformBufferSet : public RefCounted
	{
	public:
		UniformBufferSet(uint32_t size, uint32_t framesInFlight = 0);
		virtual ~UniformBufferSet() override;

        /// ----------------------------------------------------------

        Ref<UniformBuffer> Get();
        Ref<UniformBuffer> GetRenderThread();
        Ref<UniformBuffer> Get(uint32_t frame);
        void Set(Ref<UniformBuffer> uniformBuffer, uint32_t frame = 0);

    private:
        uint32_t m_framesInFlight = 0;
        std::map<uint32_t, Ref<UniformBuffer>> m_UniformBuffers;
	};

}

/// -------------------------------------------------------


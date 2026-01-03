/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* uniform_buffer.h
* -------------------------------------------------------
* Created: 10/6/2025
* -------------------------------------------------------
*/
#pragma once
#include "SceneryEditorX/renderer/vulkan_buffers.h"
#include <cstdint>
#include <map>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class UniformBuffer : public RefCounted
	{
	public:
	    UniformBuffer(uint32_t size);
        virtual ~UniformBuffer();

	    void SetData(const void *data, uint32_t size, uint32_t offset = 0);
	    void SetRenderThreadData(const void *data, uint32_t size, uint32_t offset = 0);

	    const VkDescriptorBufferInfo &GetDescriptorInfo() const { return m_DescriptorInfo; }

	private:
        void Release();
        void Invalidate_RenderThread();

	    VmaAllocation m_MemoryAlloc = nullptr;
        VkBuffer m_Buffer;
        VkDescriptorBufferInfo m_DescriptorInfo{};
        uint32_t m_Size = 0;
        std::string m_Name;
        VkShaderStageFlagBits m_ShaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
        uint8_t *m_LocalStorage = nullptr;
	};

    // -------------------------------------------------------

	class UniformBufferSet : public RefCounted
	{
	public:
		UniformBufferSet(uint32_t size, uint32_t framesInFlight);
		virtual ~UniformBufferSet() override = default;

        // ----------------------------------------------------------

        Ref<UniformBuffer> Get();
        Ref<UniformBuffer> Get(uint32_t frame);
        Ref<UniformBuffer> GetRenderThread();
        void Set(Ref<UniformBuffer> uniformBuffer, uint32_t frame = 0);

    private:
        uint32_t m_framesInFlight = 0;
        std::map<uint32_t, Ref<UniformBuffer>> m_UniformBuffers;
	};

}

// -------------------------------------------------------


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
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/renderer/vulkan/vk_allocator.h>

#include <utility>


/// -----------------------------------

namespace SceneryEditorX
{
	struct StorageBufferSpec
	{
	    bool GPUOnly = true;
	    std::string name;
	};
    
    class StorageBuffer : public RefCounted
    {
    public:
        StorageBuffer(uint32_t size, StorageBufferSpec spec);
        virtual ~StorageBuffer() override;

        virtual void SetData(const void *data, uint32_t size, uint32_t offset = 0);
        virtual void SetData_RenderThread(const void *data, uint32_t size, uint32_t offset = 0);
        virtual void Resize(uint32_t newSize);
		VkBuffer GetVulkanBuffer() const { return m_buffer; }
		const VkDescriptorBufferInfo &GetDescriptorBufferInfo() const { return m_descriptInfo; }

    private:
        VkBuffer m_buffer {};
        VkDescriptorBufferInfo m_descriptInfo {};

        void Release();
        void Invalidate_RenderThread();
        StorageBufferSpec m_spec;
        VmaAllocation memAlloc = nullptr;
        uint32_t size = 0;
        std::string name;

        VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
        VmaAllocation m_StagingAlloc = nullptr;
        VkBuffer stagingBuffer = nullptr;

        uint8_t *localStorage = nullptr;
    };

    class StorageBufferSet : public RefCounted
    {
    public:
        explicit StorageBufferSet(const StorageBufferSpec &spec,uint32_t size,uint32_t framesInFlight) : spec(spec), framesInFlight(framesInFlight)
        {
            if (framesInFlight == 0)
                framesInFlight = Renderer::GetRenderData().framesInFlight;

            for (uint32_t frame = 0; frame < framesInFlight; frame++)
                storageBuffers[frame] = CreateRef<StorageBuffer>(size, spec); 
        }

        virtual ~StorageBufferSet() override = default;

        virtual Ref<StorageBuffer> Get()
        {
            const uint32_t frame = Renderer::GetCurrentFrameIndex();
            return Get(frame);
        }

        virtual Ref<StorageBuffer> GetRenderThread()
        {
            const uint32_t frame = Renderer::GetCurrentRenderThreadFrameIndex();
            return Get(frame);
        }

        virtual Ref<StorageBuffer> Get(uint32_t frame)
        {
            SEDX_CORE_ASSERT(storageBuffers.contains(frame));
            return storageBuffers.at(frame);
        }

        virtual void Set(Ref<StorageBuffer> storageBuffer, uint32_t frame)
        {
            storageBuffers[frame] = std::move(storageBuffer);
        }

        virtual void Resize(uint32_t newSize)
        {
            for (uint32_t frame = 0; frame < framesInFlight; frame++)
                storageBuffers.at(frame)->Resize(newSize);
        }

        GLOBAL Ref<StorageBufferSet> Create(const StorageBufferSpec &spec, uint32_t size, uint32_t framesInFlight = 0);

    private:
        StorageBufferSpec spec;
        uint32_t framesInFlight = 0;
        std::map<uint32_t, Ref<StorageBuffer>> storageBuffers;
    };

}
/// ----------------------------------

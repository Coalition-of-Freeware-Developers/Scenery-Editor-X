/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* index_buffer.h
* -------------------------------------------------------
* Created: 18/5/2025
* -------------------------------------------------------
*/
#pragma once
#include "SceneryEditorX/renderer/vulkan/vk_buffers.h"
#include "SceneryEditorX/renderer/vulkan/vk_includes.h"

/// ----------------------------------------------------------

namespace SceneryEditorX
{
    /**
     * @class IndexBuffer
     * @brief Manages index data storage and configuration in Vulkan.
     */
    class IndexBuffer : public RefCounted
    {
    public:
        explicit IndexBuffer(uint64_t size);
        IndexBuffer(const void *data, uint64_t size = 0);
        virtual ~IndexBuffer() override = default; // RAII via Buffer

        void SetData(void* buffer, uint64_t size, uint64_t offset = 0);
		void Bind() const;

        uint32_t GetCount() const { return m_Size / sizeof(uint32_t); }
        uint64_t GetSize() const { return m_Size; }
        uint32_t GetRendererID() const;

    private:
        Buffer m_LocalData;                    ///< CPU-side staging copy
        Buffer m_GpuBuffer;                    ///< GPU index buffer
        uint64_t m_Size = 0;                   ///< Size of the index buffer in bytes
        std::vector<uint32_t> indices;         ///< Optional retained index data

        VkBuffer m_VulkanBuffer = nullptr;
        VmaAllocation m_MemoryAllocation;
    };
}

/// ----------------------------------------------------------

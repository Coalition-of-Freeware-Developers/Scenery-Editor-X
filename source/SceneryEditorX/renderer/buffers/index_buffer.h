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
#include "SceneryEditorX/renderer/renderer.h"
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

        /**
         * @brief Destructor for IndexBuffer
         *
         * Cleans up index buffer resources and associated memory.
         */
    	~IndexBuffer() override = default; // RAII via Buffer

        /**
         * @brief Static factory method to create an index buffer with raw data
         * @param data Pointer to index data
         * @param size Size of the data in bytes
         * @return Ref<IndexBuffer> Smart pointer to the created index buffer
         */
        static Ref<IndexBuffer> Create(const void* data, uint64_t size);

    private:
        Buffer m_LocalData;                    ///< CPU-side staging copy
        Buffer m_GpuBuffer;                    ///< GPU index buffer
        std::vector<uint32_t> indices;         ///< Optional retained index data
    };
}

/// ----------------------------------------------------------

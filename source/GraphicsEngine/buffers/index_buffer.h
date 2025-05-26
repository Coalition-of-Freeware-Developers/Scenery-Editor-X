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
#include <GraphicsEngine/vulkan/vk_core.h>

/// ----------------------------------------------------------

namespace SceneryEditorX
{

    /**
     * @class IndexBuffer
     * @brief Manages index data storage and configuration in Vulkan
     *
     * This class handles the creation and management of index buffers in Vulkan,
     * including memory allocation and transfer of index data to the GPU.
     * Index buffers store the indices that define how vertices are connected
     * to form geometry primitives (triangles).
     */
    class IndexBuffer
    {
    public:
        /**
         * @brief Constructor for IndexBuffer
         *
         * Initializes the IndexBuffer instance by getting references to required
         * graphics engine components.
         */
        IndexBuffer();

        /**
         * @brief Destructor for IndexBuffer
         *
         * Cleans up index buffer resources and associated memory.
         */
        ~IndexBuffer();

        /**
         * @brief Creates and initializes the index buffer on the GPU
         *
         * Allocates memory for the index buffer and transfers index data to it.
         * The buffer is created with appropriate usage flags for index data access
         * during rendering.
         */
        void CreateIndexBuffer() const;

    private:
        //Ref<GraphicsEngine> *gfxEngine;      ///< Pointer to the graphics engine reference
        Ref<MemoryAllocator> allocator;		 ///< Reference to the memory allocator
        std::vector<uint32_t> indices;		 ///< Storage for index data
        VkBuffer indexBuffer;				 ///< Handle to the Vulkan index buffer
        VmaAllocation indexBufferAllocation; ///< Handle to the allocated memory for the index buffer
    };

}

/// ----------------------------------------------------------

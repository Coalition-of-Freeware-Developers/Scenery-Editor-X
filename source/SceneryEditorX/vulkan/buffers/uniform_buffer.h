/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* uniform_buffer.h
* -------------------------------------------------------
* Created: 18/5/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/vulkan/render_data.h>
#include <SceneryEditorX/vulkan/vk_core.h>

/// ----------------------------------------------------------

namespace SceneryEditorX
{
    /**
     * @class UniformBuffer
     * @brief Manages uniform buffer objects for shader uniforms in Vulkan
     *
     * This class handles the creation, management, and updating of uniform buffers
     * used to pass transformation matrices and other uniform data to shaders.
     * It maintains a separate buffer for each frame in flight to prevent race conditions.
     */
    class UniformBuffer
    {
    public:
        /**
         * @struct UBO
         * @brief Uniform Buffer Object structure for shader uniforms
         *
         * Contains transformation matrices aligned for GPU memory access.
         * The alignas specifier ensures proper memory alignment for GPU access.
         */
        struct UBO
        {
            alignas(16) glm::mat4 model; ///< Model transformation matrix
            alignas(16) glm::mat4 view;  ///< View transformation matrix
            alignas(16) glm::mat4 proj;  ///< Projection transformation matrix
        };

        /**
         * @brief Constructor for UniformBuffer
         * 
         * Initializes the UniformBuffer instance by getting references to the 
         * required graphics engine components.
         */
        UniformBuffer();

        /**
         * @brief Destructor for UniformBuffer
         *
         * Cleans up resources associated with the uniform buffers.
         */
        ~UniformBuffer();

        /**
         * @brief Creates uniform buffers for each frame in the swap chain
         *
         * Allocates and initializes the uniform buffers used to pass transformation
         * matrices to shaders. Creates one buffer per swap chain image.
         */
        void CreateUniformBuffers();

        /**
         * @brief Creates a Vulkan buffer with specified parameters
         *
         * @param size Size of the buffer in bytes
         * @param usage Buffer usage flags indicating how the buffer will be used
         * @param properties Memory property flags for the buffer allocation
         * @param buffer Reference to store the created buffer handle
         * @param bufferMemory Reference to store the allocated memory handle
         */
        void CreateBuffer(VkDeviceSize size,
                          VkBufferUsageFlags usage,
                          VkMemoryPropertyFlags properties,
                          VkBuffer &buffer,
                          VkDeviceMemory &bufferMemory) const;

        /**
         * @brief Updates the contents of a uniform buffer for the current frame
         *
         * @param currentImage Index of the current swap chain image/frame
         * 
         * Updates transformation matrices in the uniform buffer for the specified
         * frame with the current camera and model state.
         */
        void UpdateUniformBuffer(uint32_t currentImage) const;

        /**
         * @brief Gets the buffer handle for the specified frame
         *
         * @param index Index of the frame's buffer to retrieve
         * @return VkBuffer The uniform buffer handle, or VK_NULL_HANDLE if invalid index
         */
        [[nodiscard]] VkBuffer GetBuffer(uint32_t index) const
        {
            if (index < uniformBuffers.size())
                return uniformBuffers[index];
            return VK_NULL_HANDLE;
        }

        /**
         * @brief Gets the buffer memory handle for the specified frame
         *
         * @param index Index of the frame's buffer memory to retrieve
         * @return VkDeviceMemory The uniform buffer memory handle, or VK_NULL_HANDLE if invalid index
         */
        [[nodiscard]] VkDeviceMemory GetBufferMemory(uint32_t index) const
        {
            if (index < uniformBuffersMemory.size())
                return uniformBuffersMemory[index];
            return VK_NULL_HANDLE;
        }

        /**
         * @brief Gets the total number of uniform buffers managed by this instance
         *
         * @return size_t The number of uniform buffers
         */
        [[nodiscard]] size_t GetBufferCount() const
        {
            return uniformBuffers.size();
        }

    private:
        Ref<GraphicsEngine> *gfxEngine;                   ///< Pointer to the graphics engine reference
        Ref<MemoryAllocator> allocator;                   ///< Reference to the memory allocator
        RenderData renderData;                            ///< Reference to the render data structure
        std::vector<VkBuffer> uniformBuffers;             ///< Array of uniform buffer handles (one per frame)
        std::vector<VkDeviceMemory> uniformBuffersMemory; ///< Array of memory handles for uniform buffers
    };

}

/// -----

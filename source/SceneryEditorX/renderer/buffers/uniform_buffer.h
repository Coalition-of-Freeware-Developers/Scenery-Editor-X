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
#include <SceneryEditorX/renderer/vulkan/vk_swapchain.h>

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
    class UniformBuffer : public RefCounted
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
        explicit UniformBuffer(uint32_t size);

        /**
         * @brief Destructor for UniformBuffer
         *
         * Cleans up resources associated with the uniform buffers.
         */
        virtual ~UniformBuffer() override;

        /**
         * @brief Creates uniform buffers for each frame in the swap chain
         *
         * Allocates and initializes the uniform buffers used to pass transformation
         * matrices to shaders. Creates one buffer per swap chain image.
         */
        void CreateUniformBuffers();

        /**
         * @brief Updates the contents of a uniform buffer for the current frame
         *
         * @param currentImage Index of the current swap chain image/frame
         * 
         * Updates transformation matrices in the uniform buffer for the specified
         * frame with the current camera and model state.
         */
        void Update(uint32_t currentImage) const;

        /**
         * @brief Gets the buffer handle for the specified frame
         *
         * @param index Index of the frame's buffer to retrieve
         * @return VkBuffer The uniform buffer handle, or VK_NULL_HANDLE if invalid index
         */
        [[nodiscard]] Buffer GetBuffer(uint32_t index) const;

        /**
         * @brief Gets the total number of uniform buffers managed by this instance
         *
         * @return size_t The number of uniform buffers
         */
        [[nodiscard]] size_t GetBufferCount() const { return uniformBuffers.size(); }

    private:
        Ref<MemoryAllocator> allocator;							///< Reference to the memory allocator
        std::vector<Buffer> uniformBuffers;						///< Array of uniform buffer handles (one per frame)
        std::vector<VkDeviceMemory> uniformBuffersMemory;		///< Array of memory handles for uniform buffers
        std::vector<VmaAllocation> uniformBuffersAllocation;	///< Array of allocation handles for uniform buffers (one per frame)
    };

}

/// ----------------------------------------------------------

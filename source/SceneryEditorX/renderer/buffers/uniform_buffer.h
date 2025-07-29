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
#include <SceneryEditorX/renderer/vulkan/vk_allocator.h>

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
            alignas(16) Mat4 model; ///< Model transformation matrix
            alignas(16) Mat4 view;  ///< View transformation matrix
            alignas(16) Mat4 proj;  ///< Projection transformation matrix
        };

        /**
         * @brief Constructor for UniformBuffer
         * 
         * Initializes the UniformBuffer instance by getting references to the 
         * required graphics engine components.
         */
        UniformBuffer(uint32_t size);

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
		 * @brief Sets the data for the uniform buffer
		 *
		 * Copies data into the uniform buffer for the current frame.
		 *
		 * @param data Pointer to the data to be copied into the uniform buffer
		 * @param size Size of the data in bytes to be copied
		 * @param offset Offset in bytes where the data should be copied within the buffer
		 */
		void SetData(const void *data, uint32_t size, uint32_t offset = 0);

        /**
         * @brief Sets the data for the uniform buffer on the render thread
         *
         * Copies data into the uniform buffer for the current frame.
         *
         * @param data Pointer to the data to be copied into the uniform buffer
         * @param size Size of the data in bytes to be copied
         * @param offset Offset in bytes where the data should be copied within the buffer
         */
        void SetRenderThreadData(const void *data, uint32_t size, uint32_t offset = 0);

        /**
         * @brief Creates a Vulkan buffer with specified parameters
         *
         * @param size Size of the buffer in bytes
         * @param usage Buffer usage flags indicating how the buffer will be used
         * @param properties Memory property flags for the buffer allocation
         * @param buffer Reference to store the created buffer handle
         * @param bufferMemory Reference to store the allocated memory handle
         */
        GLOBAL void Create(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory);

        /**
         * @brief Gets the name of the uniform buffer instance
         *
         * @return uint32_t The unique identifier for the uniform buffer instance
         */
        const VkDescriptorBufferInfo& GetDescriptorBufferInfo() const { return descriptorInfo; }

        /**
         * @brief Updates the contents of a uniform buffer for the current frame
         *
         * @param index
         * @param currentImage Index of the current swap chain image/frame
         * 
         * Updates transformation matrices in the uniform buffer for the specified
         * frame with the current camera and model state.
         */
        //void Update(uint32_t currentImage) const;

        /**
         * @brief Gets the buffer handle for the specified frame
         * @param index Index of the frame's buffer to retrieve
         * @return VkBuffer The uniform buffer handle, or VK_NULL_HANDLE if invalid index
         */
        [[nodiscard]] VkBuffer GetBuffer(const uint32_t index) const
        {
            if (index < uniformBuffers.size())
                return uniformBuffers[index];

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
        uint32_t name = 0;									///< Unique identifier for the uniform buffer instance
        uint32_t size = 0;                                  ///< Size of the uniform buffer in bytes
        uint8_t *localMemAlloc = nullptr;					///< Pointer to local memory allocation for the uniform buffer
        VmaAllocation allocation = nullptr;                 ///< Reference to the memory allocator
        VkDescriptorBufferInfo descriptorInfo = {};			///< Descriptor buffer info for binding the uniform buffer
        std::vector<VkBuffer> uniformBuffers;				///< Array of uniform buffer handles (one per frame)
        std::vector<VkDeviceMemory> uniformBuffersMemory;	///< Array of memory handles for uniform buffers
        std::vector<VmaAllocation> uniformBuffersAllocation;///< Array of allocation handles for uniform buffers (one per frame)
    };

}

/// ----------------------------------------------------------

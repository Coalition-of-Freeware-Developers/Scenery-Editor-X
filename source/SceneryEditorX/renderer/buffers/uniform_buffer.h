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
#include <SceneryEditorX/renderer/render_context.h>
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/renderer/vulkan/vk_allocator.h>
#include <utility>

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
         * @return size_t The number of uniform buffers
         */
        [[nodiscard]] size_t GetBufferCount() const { return uniformBuffers.size(); }

    private:
        uint32_t name;										///< Unique identifier for the uniform buffer instance
        uint32_t size = 0;                                  ///< Size of the uniform buffer in bytes
        uint8_t *localMemAlloc = nullptr;					///< Pointer to local memory allocation for the uniform buffer
        VmaAllocation allocation = nullptr;                 ///< Reference to the memory allocator
        VkDescriptorBufferInfo descriptorInfo = {};			///< Descriptor buffer info for binding the uniform buffer
        std::vector<VkBuffer> uniformBuffers;				///< Array of uniform buffer handles (one per frame)
        std::vector<VkDeviceMemory> uniformBuffersMemory;	///< Array of memory handles for uniform buffers
        std::vector<VmaAllocation> uniformBuffersAllocation;///< Array of allocation handles for uniform buffers (one per frame)
    };

	class UniformBufferSet : public RefCounted
    {
	public:
        UniformBufferSet(uint32_t size, uint32_t framesInFlight) : m_framesInFlight(framesInFlight)
        {
            if (framesInFlight == 0)
                m_framesInFlight = Renderer::GetRenderData().framesInFlight;

            for (uint32_t frame = 0; frame < m_framesInFlight; frame++)
                m_UniformBuffers[frame] = CreateRef<UniformBuffer>(size);
        }

	    virtual ~UniformBufferSet() override = default;

		/// ----------------------------------------------------------

        virtual Ref<UniformBuffer> Get()
        {
            const uint32_t frame = Renderer::GetCurrentFrameIndex();
            return Get(frame);
        }

        virtual Ref<UniformBuffer> Get_RenderThread()
        {
            const uint32_t frame = Renderer::GetCurrentRenderThreadFrameIndex();
            return Get(frame);
        }

		virtual Ref<UniformBuffer> Get(uint32_t frame)
        {
            SEDX_CORE_ASSERT(m_UniformBuffers.contains(frame));
            return m_UniformBuffers.at(frame);
        }

        virtual void Set(Ref<UniformBuffer> uniformBuffer, uint32_t frame = 0)
        {
            m_UniformBuffers[frame] = std::move(uniformBuffer);
        }

	private:
        uint32_t m_framesInFlight = 0;
        std::map<uint32_t, Ref<UniformBuffer>> m_UniformBuffers;

    };

} // namespace SceneryEditorX

/// ----------------------------------------------------------

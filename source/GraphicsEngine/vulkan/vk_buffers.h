/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_buffers.h
* -------------------------------------------------------
* Created: 5/5/2025
* -------------------------------------------------------
*/
#pragma once
#include <GraphicsEngine/vulkan/resource.h>
#include <SceneryEditorX/core/pointers.h>
#include <SceneryEditorX/logging/asserts.h>
#include <vma/vk_mem_alloc.h>

/// --------------------------------------------

namespace SceneryEditorX
{
	using Flags = uint32_t;

	enum MemoryType : uint8_t
    {
	    GPU = 0x00000001,
	    CPU = 0x00000002 | 0x00000004,
	};
	using MemoryFlags = Flags;

    // ReSharper disable twice IdentifierTypo
    namespace BufferUsage
    { 
		enum BufferUsageFlags
		{
		    TransferSrc = 0x00000001,
		    TransferDst = 0x00000002,
		    UniformTexel = 0x00000004,
		    StorageTexel = 0x00000008,
		    Uniform = 0x00000010,
		    Storage = 0x00000020,
		    Index = 0x00000040,
		    Vertex = 0x00000080,
		    Indirect = 0x00000100,
		    Address = 0x00020000,
		    VideoDecodeSrc = 0x00002000,
		    VideoDecodeDst = 0x00004000,
		    TransformFeedback = 0x00000800,
		    TransformFeedbackCounter = 0x00001000,
		    ConditionalRendering = 0x00000200,
		    AccelerationStructureInput = 0x00080000,
		    AccelerationStructure = 0x00100000,
		    ShaderBindingTable = 0x00000400,
		    SamplerDescriptor = 0x00200000,
		    ResourceDescriptor = 0x00400000,
		    PushDescriptors = 0x04000000,
            MicromapBuildInputReadOnly = 0x00800000,
		    MicromapStorage = 0x01000000,
		};

    }
    using BufferUsageFlags = Flags;

    /// -------------------------------------------------------

    /**
     * @brief Safely destroys a Vulkan buffer and frees its associated memory allocation
     * 
     * This utility function handles the clean destruction of a VkBuffer object and its
     * associated VMA allocation. It performs safety checks to ensure valid resources
     * before attempting destruction.
     * 
     * @param buffer The VkBuffer handle to destroy
     * @param allocation The VmaAllocation associated with the buffer to free
     */
    namespace VulkanMemoryUtils
    {
        void DestroyBuffer(VkBuffer buffer, VmaAllocation allocation);
    }

    /// -------------------------------------------------------

    /**
     * @struct BufferResource
     * @brief Resource wrapper for Vulkan buffers with memory management
     * 
     * BufferResource encapsulates a Vulkan buffer handle and its associated memory allocation.
     * It inherits from Resource to integrate with the engine's resource management system
     * and provides automatic cleanup through the destructor.
     */
    struct BufferResource : Resource
    {
        /**
         * @brief Vulkan buffer handle
         * 
         * The underlying VkBuffer object used by the graphics API
         */
        VkBuffer buffer = VK_NULL_HANDLE;

        /**
         * @brief VMA memory allocation associated with this buffer
         * 
         * Managed by the Vulkan Memory Allocator (VMA) library
         */
        VmaAllocation allocation;

        /**
         * @brief Native Vulkan device memory handle
         * 
         * Direct reference to the underlying VkDeviceMemory, typically managed by VMA
         */
        VkDeviceMemory memory = VK_NULL_HANDLE;

        /**
         * @brief Destroys the buffer and frees its memory allocation
         *
         * This destructor is responsible for cleaning up the Vulkan buffer and its associated
         * memory allocation. It ensures that resources are released properly to prevent memory leaks.
         *
         * @note This destructor is virtual to allow for polymorphic behavior in derived classes.
		 * @see VulkanMemoryUtils::DestroyBuffer
         */
        virtual ~BufferResource() override
        {
            VulkanMemoryUtils::DestroyBuffer(buffer, allocation);
        }

    };

    /// -------------------------------------------------------

    struct Buffer
    {
        Ref<BufferResource> resource;		///< The Vulkan buffer resource
        uint64_t size;						///< Size of the buffer in bytes
        BufferUsageFlags usage;				///< Usage flags for the buffer (e.g., vertex, index, uniform)
        MemoryFlags memory;					///< Memory type flags indicating where the buffer is allocated (e.g., GPU, CPU)
        [[nodiscard]] uint32_t ID() const;  ///< Unique identifier for the buffer, used for tracking and debugging
    };
	 
	/// -------------------------------------------------------

    /**
     * @brief Begins a single-use command buffer for immediate execution
     * 
     * @return VkCommandBuffer A command buffer ready to record commands
     * 
     * This function allocates a command buffer from a command pool and begins recording.
     * It's designed for operations that need to be executed immediately and only once,
     * such as resource transfers or one-time initialization tasks.
     * 
     * The command buffer must be ended and submitted using EndSingleTimeCommands()
     * after recording the desired commands.
     * 
     * @see EndSingleTimeCommands() to complete the execution
     */
    VkCommandBuffer BeginCommands();

    /**
     * @brief Creates a Vulkan buffer with specified parameters
     * 
     * @param size Size of the buffer in bytes
     * @param usage Flags specifying how the buffer will be used (vertex, index, uniform, etc.)
     * @param memory Memory type flags indicating where the buffer should be allocated (GPU, CPU, etc.)
     * @param name Optional debug name for the buffer resource
     * @return Buffer A buffer object containing the Vulkan buffer handle and allocation information
     * 
     * This function creates a Vulkan buffer with the specified size and usage flags,
     * allocating it in the appropriate memory type based on the memory parameter.
     * It abstracts away the details of buffer creation and memory allocation in Vulkan.
     */
    Buffer CreateBuffer(uint64_t size, BufferUsageFlags usage, MemoryFlags memory = MemoryType::GPU, const std::string& name = "");
    
    /**
     * @brief Copies data between two Vulkan buffers
     * 
     * @param srcBuffer Source buffer to copy from
     * @param dstBuffer Destination buffer to copy to
     * @param size Number of bytes to copy
     * 
     * This function performs a buffer-to-buffer copy operation through a command buffer,
     * typically used to transfer data from a staging buffer to a device-local buffer.
     * The operation is completed synchronously by creating, executing and destroying
     * a single-use command buffer.
     */
    void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    
    /**
     * @brief Copies buffer data to an image
     * 
     * @param buffer Source buffer containing image data
     * @param image Destination image to copy data to
     * @param width Width of the image in pixels
     * @param height Height of the image in pixels
     * 
     * This function transfers data from a buffer to an image, typically used when
     * loading texture data. It handles the necessary image layout transitions and
     * submits a copy command to the graphics queue. The image should be in the
     * appropriate layout for a transfer destination operation before calling this function.
     */
    void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	/**
     * @brief Maps a Vulkan buffer to host-accessible memory
     * 
     * @param buffer The buffer to map to CPU-accessible memory
     * @return void* A pointer to the mapped memory region
     * 
     * This function maps the specified Vulkan buffer to memory that can be
     * accessed by the CPU. It allows direct reading from or writing to the buffer
     * from the host application.
     * 
     * The buffer must have been created with appropriate memory flags that
     * support host access (e.g., MemoryType::CPU_VISIBLE). The function returns
     * a pointer to the mapped memory region which can be used for data transfer.
     * 
     * @note The caller is responsible for ensuring proper synchronization when
     * accessing the mapped memory. The memory should be unmapped when no longer needed.
     * 
     * @see UnmapBuffer() to unmap the memory when operations are complete
     */
    void *MapBuffer(BufferResource &buffer);

	/**
	 * @brief Unmaps a Vulkan buffer from host-accessible memory
	 *
	 * This function unmaps the memory previously mapped by MapBuffer().
	 * It ensures that any modifications made to the mapped memory region are
	 * properly flushed and the buffer is no longer accessible from the CPU.
	 *
	 * The buffer must have been mapped using MapBuffer() and must support host access.
	 * After calling this function, the pointer returned by MapBuffer() becomes invalid.
	 *
	 * @param buffer The buffer resource to unmap from CPU-accessible memory
	 *
	 * @see MapBuffer(BufferResource&) to map the buffer before unmapping
	 */
	void UnmapBuffer(BufferResource &buffer);


} // namespace SceneryEditorX

/// ----------------------------------------------------------

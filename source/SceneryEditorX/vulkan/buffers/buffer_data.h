/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* buffer_data.h
* -------------------------------------------------------
* Created: 20/4/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/logging/asserts.h>
#include <SceneryEditorX/vulkan/render_data.h>
#include <vma/vk_mem_alloc.h>

/// ---------------------------------------------------------

namespace SceneryEditorX
{
    class MemoryAllocator;

	using Flags = uint32_t;

	enum MemoryType : uint8_t
    {
	    GPU = 0x00000001,
	    CPU = 0x00000002 | 0x00000004,
	};
	using MemoryFlags = Flags;

    namespace BufferUsage
    {
		enum
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

    // -------------------------------------------------------

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

    // -------------------------------------------------------

    /*
    struct UniformBuffer
    {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };
	*/

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

    /**
     * @struct Buffer
     * @brief Wrapper class for Vulkan buffer objects with associated memory
     * 
     * This class encapsulates a Vulkan buffer along with its memory allocation details,
     * providing a convenient interface for buffer management. It stores metadata about
     * the buffer's size, usage flags, memory type, and mapped status.
     *
     * @note This class is designed to be used in conjunction with VulkanMemoryUtils for resource management.
     */
    struct Buffer
    {
        /**
         * @brief Logical device that owns this buffer.
         * 
         * Reference to the Vulkan device handle that created this buffer.
         */
        VkDevice device;

        /**
         * @brief Resource handle for the underlying buffer object.
         * 
         * Contains the actual VkBuffer handle and memory allocation.
         */
        Ref<BufferResource> bufferResource;

        /**
         * @brief Size of the buffer in bytes
         */
        VkDeviceSize size = 0;

        /**
         * @brief Memory alignment requirement for the buffer.
         * 
         * The buffer data will be aligned to this boundary in memory.
         */
        VkDeviceSize alignment = 0;

        /**
         * @brief Buffer usage flags defining how this buffer can be used.
         * 
         * Combination of BufferUsage flags (vertex, index, uniform, etc.)
         * @see BufferUsage
         */
        BufferUsageFlags usage;

        /**
         * @brief Memory type flags specifying where the buffer is allocated.
         * 
         * Determines if the buffer is in GPU-only memory, CPU-accessible memory, etc.
         * @see MemoryType
         */
        MemoryFlags memory;

        /**
         * @brief Pointer to mapped memory region.
         * 
         * Null when the buffer is not mapped to CPU-accessible memory.
         */
        void *mapped = nullptr;

        /**
         * @brief Usage flags to be filled by external source at buffer creation (to query at some later point).
         */
        VkBufferUsageFlags usageFlags;

        /**
         * @brief Memory property flags to be filled by external source at buffer creation (to query at some later point).
         */
        VkMemoryPropertyFlags memoryPropertyFlags;

        /**
         * @brief Get the Resource ID of the buffer resource.
         * @return uint32_t Resource ID.
         */
        [[nodiscard]] uint32_t ResourceID() const;
    };

    /**
     * @brief Get the Resource ID of the buffer resource.
     * @return uint32_t Resource ID.
     */
    inline uint32_t Buffer::ResourceID() const
    {
        SEDX_ASSERT(bufferResource->resourceID != -1, "Invalid buffer rid");
        return static_cast<uint32_t>(bufferResource->resourceID);
    }

}  // namespace SceneryEditorX

/// ---------------------------------------------------------

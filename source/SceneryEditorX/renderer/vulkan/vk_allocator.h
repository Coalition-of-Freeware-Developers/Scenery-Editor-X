/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_allocator.h
* -------------------------------------------------------
* Created: 8/4/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/renderer/vulkan/vk_buffers.h>
#include <vma/vk_mem_alloc.h>

/// -------------------------------------------------------

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1

/// -------------------------------------------------------

namespace SceneryEditorX
{
    /// Forward declarations
    class VulkanDevice;
    struct AppData;

    /// -------------------------------------------------------

	/**
	 * @struct MemoryPool
	 * @brief Represents a memory pool for efficient allocation.
	 *
	 * This structure encapsulates a Vulkan memory pool, its block size,
	 * minimum alignment, and the associated VMA pool create info.
	 * It is used to manage memory allocations for buffers and images
	 * in a more efficient manner by grouping similar allocations together.
	 */
    struct MemoryPool
    {
        VkDeviceSize blockSize = 0;
        VkDeviceSize minAlignment = 0;
        std::vector<VmaPool> pools;
        VmaPoolCreateInfo createInfo = {};

        MemoryPool() : createInfo({}) {}

        MemoryPool(const VkDeviceSize size, const VmaMemoryUsage usage) : blockSize(size), createInfo({})
        {
            createInfo.memoryTypeIndex = static_cast<uint32_t>(usage);
        }
    };

    /// ---------------------------------------------------------

    /// Constants for common sizes
    constexpr VkDeviceSize SMALL_BUFFER_SIZE = 1024 * 256;       // 256KB
    constexpr VkDeviceSize MEDIUM_BUFFER_SIZE = 1024 * 1 * 1024; // 1MB
    constexpr VkDeviceSize LARGE_BUFFER_SIZE = 1024 * 16 * 1024; // 16MB
    /// This is a default value and will be overridden by users settings.
    constexpr VkDeviceSize DEFAULT_CUSTOM_BUFFER_SIZE = 1024 * 16 * 1024; // 16MB

    /// ---------------------------------------------------------

    /**
	 * @class MemoryAllocator
	 * @brief Manages Vulkan memory allocations using VMA.
	 *
	 * This class provides a high-level interface for managing GPU memory allocations
	 * using the Vulkan Memory Allocator (VMA). It handles buffer and image allocations,
	 * defragmentation, and memory usage statistics.
	 */
    class MemoryAllocator : public RefCounted
    {
    public:
        MemoryAllocator() = default;
        explicit MemoryAllocator(std::string tag);
        virtual ~MemoryAllocator() override;

        /// Defragmentation methods
        void BeginDefragmentation(VmaDefragmentationFlags flags = 0);
        void EndDefragmentation();

        /// Method to mark an allocation as defragmentable
        void MarkForDefragmentation(VmaAllocation allocation);

		/// ---------------------------------------------------------

		/**
		 * @struct AllocationStats
		 * @brief Represents memory allocation statistics.
		 *
		 * This structure contains information about the total bytes allocated,
		 * used bytes, allocation count, and fragmentation ratio.
		 *
		 * @note This structure is used to monitor memory usage and
		 */
        struct AllocationStats
        {
            uint64_t totalBytes;
            uint64_t usedBytes;
            uint64_t allocationCount;
            float fragmentationRatio;
        };

        [[nodiscard]] AllocationStats GetStats();
        void PrintDetailedStats() const;
        void ResetStats();

        /**
         * @enum AllocationStrategy
         * @brief Defines strategies for memory allocation.
         */
        enum class AllocationStrategy : uint8_t
        {
            Default,        /// Let VMA decide
            SpeedOptimized, /// Optimize for fast allocation
            MemoryOptimized /// Optimize for minimal memory usage
        };

        void SetAllocationStrategy(AllocationStrategy strategy);
        void ApplyAllocationStrategy(VmaAllocationCreateInfo &createInfo) const;
        GLOBAL bool ContainsAllocation(VmaAllocation allocation);
        void DestroyBuffer(VkBuffer buffer, VmaAllocation allocation);

        /**
		 * @brief Gets the current custom buffer size
		 *
		 * @return The custom buffer size in bytes
		 */
        GLOBAL VkDeviceSize GetCustomBufferSize();

		/**
		 * @brief Sets the custom buffer size if the device supports it.
		 *
		 * @param size The desired buffer size in bytes.
		 * @param device Reference to the Vulkan device.
		 * @return true if set successfully, false if unsupported
		 */
		GLOBAL bool SetCustomBufferSize(VkDeviceSize size, VulkanDevice& device);

        /// ---------------------------------------------------------

		/**
		 * @struct MemoryBudget
		 * @brief Represents memory budget information.
		 *
		 * This structure contains information about the total memory,
		 * used memory, usage percentage, and whether the budget is exceeded.
		 *
		 * @note This structure is used to monitor memory usage and
		 * to trigger warnings if the usage exceeds a certain threshold.
		 */
		struct MemoryBudget
        {
            uint64_t totalBytes;
            uint64_t usedBytes;
            float usagePercentage;
            bool isOverBudget;
        };

        [[nodiscard]] MemoryBudget GetMemoryBudget() const;
        void SetMemoryUsageWarningThreshold(float percentage);

		/// ---------------------------------------------------------

        void SetBufferAlignment(VkDeviceSize alignment);

		/// ---------------------------------------------------------

		/**
		 * @struct BatchBufferAllocation
		 * @brief Represents a batch buffer allocation.
		 *
		 * This structure encapsulates a Vulkan buffer, its associated VMA allocation,
		 * and the size of the buffer. It is used for batch allocation of buffers
		 * to improve performance and reduce fragmentation.
		 *
		 * @note This structure is used internally by the MemoryAllocator class.
		 */
        struct BatchBufferAllocation
        {
            VkBuffer buffer;
            VmaAllocation allocation;
            VkDeviceSize size;
        };

        /// Batch allocation methods
        [[nodiscard]] std::vector<BatchBufferAllocation> AllocateBufferBatch(const std::vector<VkDeviceSize> &sizes, BufferUsageFlags usage, VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO) const;

        void FreeBufferBatch(const std::vector<BatchBufferAllocation> &allocations);

		/// ---------------------------------------------------------

        VmaAllocation AllocateBuffer(const VkBufferCreateInfo &bufferCreateInfo, VmaMemoryUsage usage, VkBuffer &outBuffer);
        VmaAllocation AllocateImage(const VkImageCreateInfo &imageCreateInfo, VmaMemoryUsage usage, VkImage &outImage, VkDeviceSize* allocatedSize = nullptr);

        void Free(VmaAllocation allocation);
        void DestroyImage(VkImage image, VmaAllocation allocation);

		/// ---------------------------------------------------------

		/**
		 * @tparam T
		 * @fn MapMemory
		 * @brief Maps memory for a given allocation.
		 *
		 * This function maps the memory associated with a given VMA allocation
		 * and returns a pointer to the mapped memory.
		 *
		 * @param allocation The VMA allocation to map.
		 * @return A pointer to the mapped memory.
		 */
		template<typename T>
		T* MapMemory(const VmaAllocation allocation)
		{
			T* mappedMemory;
            vmaMapMemory(GetAllocator(), allocation, static_cast<void **>(&mappedMemory));
			return mappedMemory;
		}

		/// ---------------------------------------------------------

        GLOBAL void UnmapMemory(VmaAllocation allocation);
        GLOBAL VmaAllocator GetAllocator();

		GLOBAL void Init(const Ref<VulkanDevice> &device, const uint32_t &apiVersion);
		GLOBAL void Shutdown();

		/// ---------------------------------------------------------

    private:
        Ref<VulkanDevice> vkDevice;
        std::string Tag_;
        std::vector<VmaAllocation> defragmentationCandidates;
        VmaDefragmentationContext defragmentationContext = nullptr;
        AllocationStrategy currentStrategy = AllocationStrategy::Default;

        /// Fixed-size pools for common allocation sizes
        std::unordered_map<VkDeviceSize, MemoryPool> bufferPools;
        std::unordered_map<VkDeviceSize, MemoryPool> imagePools;

        /// Helper methods for pool creation and retrieval
        VmaPool GetOrCreateBufferPool(VkDeviceSize size, VmaMemoryUsage usage);
        VmaPool GetOrCreateImagePool(VkDeviceSize size, VmaMemoryUsage usage);

        /// For thread safety
        std::mutex allocationMutex;
        std::mutex poolMutex;

        float memoryWarningThreshold = 0.9f; /// 90% usage generates warnings
        [[nodiscard]] bool CheckMemoryBudget() const;

        VkDeviceSize customBufferAlignment = 0;

        /// Helper function to align buffer sizes for better caching
        [[nodiscard]] VkDeviceSize AlignBufferSize(VkDeviceSize size) const;

		INTERNAL VkDeviceSize customBufferSize;
    };

} // namespace SceneryEditorX

/// -------------------------------------------------------

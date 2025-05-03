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
#include <SceneryEditorX/renderer/vk_device.h>
#include <vulkan/vulkan.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct SoftwareStats;

    struct MemoryPool
    {
        VkDeviceSize blockSize;
        VkDeviceSize minAlignment;
        std::vector<VmaPool> pools;
        VmaPoolCreateInfo createInfo;

        MemoryPool() = default;

        MemoryPool(const VkDeviceSize size, const VmaMemoryUsage usage) : minAlignment(0)
        {
            blockSize = size;
            createInfo = {};
            createInfo.blockSize = size;
            createInfo.memoryTypeIndex = static_cast<uint32_t>(usage);
        }
    };

    // ---------------------------------------------------------

    /// Constants for common sizes
    constexpr VkDeviceSize SMALL_BUFFER_SIZE = 1024 * 256;       // 256KB
    constexpr VkDeviceSize MEDIUM_BUFFER_SIZE = 1024 * 1 * 1024; // 1MB
    constexpr VkDeviceSize LARGE_BUFFER_SIZE = 1024 * 16 * 1024; // 16MB

    // ---------------------------------------------------------

    class MemoryAllocator
    {
    public:
        MemoryAllocator() = default;
        explicit MemoryAllocator(std::string tag);
        ~MemoryAllocator();

        /// Defragmentation methods
        void BeginDefragmentation(VmaDefragmentationFlags flags = 0);
        void EndDefragmentation();

        /// Method to mark an allocation as defragmentable
        void MarkForDefragmentation(VmaAllocation allocation);

		// ---------------------------------------------------------

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

        enum class AllocationStrategy : uint8_t
        {
            Default,        /// Let VMA decide
            SpeedOptimized, /// Optimize for fast allocation
            MemoryOptimized /// Optimize for minimal memory usage
        };

        void SetAllocationStrategy(AllocationStrategy strategy);
        //void ApplyAllocationStrategy(VmaAllocationCreateInfo &createInfo) const;
        void ApplyAllocationStrategy(VmaAllocationCreateInfo &createInfo);
        bool ContainsAllocation(VmaAllocation allocation) const;
        void DestroyBuffer(VkBuffer buffer, VmaAllocation allocation);

        // ---------------------------------------------------------

		struct MemoryBudget
        {
            uint64_t totalBytes;
            uint64_t usedBytes;
            float usagePercentage;
            bool isOverBudget;
        };

        [[nodiscard]] MemoryBudget GetMemoryBudget() const;
        void SetMemoryUsageWarningThreshold(float percentage);

		// ---------------------------------------------------------

        void SetBufferAlignment(VkDeviceSize alignment);

		// ---------------------------------------------------------

        struct BatchBufferAllocation
        {
            VkBuffer buffer;
            VmaAllocation allocation;
            VkDeviceSize size;
        };

        /// Batch allocation methods
        [[nodiscard]] std::vector<BatchBufferAllocation> AllocateBufferBatch(const std::vector<VkDeviceSize> &sizes, BufferUsageFlags usage, VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO);

        void FreeBufferBatch(const std::vector<BatchBufferAllocation> &allocations);

		// ---------------------------------------------------------

        VmaAllocation AllocateBuffer(const VkBufferCreateInfo &bufferCreateInfo, VmaMemoryUsage usage, VkBuffer &outBuffer);
        VmaAllocation AllocateImage(const VkImageCreateInfo &imageCreateInfo, VmaMemoryUsage usage, VkImage &outImage, VkDeviceSize* allocatedSize = nullptr);

        void Free(VmaAllocation allocation);
        void DestroyImage(VkImage image, VmaAllocation allocation);

		// ---------------------------------------------------------

		template<typename T>
		T* MapMemory(const VmaAllocation allocation)
		{
			T* mappedMemory;
            vmaMapMemory(GetMemAllocator(), allocation, static_cast<void **>(&mappedMemory));
			return mappedMemory;
		}

		// ---------------------------------------------------------

        void UnmapMemory(VmaAllocation allocation);
        GLOBAL VmaAllocator GetMemAllocator();

		GLOBAL void Init(Ref<VulkanDevice> device);
		GLOBAL void Shutdown();

		// ---------------------------------------------------------

    private:
        std::string Tag_;
        std::vector<VmaAllocation> defragmentationCandidates;
        VmaDefragmentationContext defragmentationContext;
        AllocationStrategy currentStrategy;

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

    };

} // namespace SceneryEditorX

// -------------------------------------------------------

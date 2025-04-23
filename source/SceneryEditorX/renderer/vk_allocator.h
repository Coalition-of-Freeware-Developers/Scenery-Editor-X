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

    struct VulkanAllocatorData
    {
        struct MemoryTypeStats
        {
            uint64_t bytesAllocated = 0;
            uint64_t bytesFreed = 0;
            uint32_t currentAllocations = 0;
            uint32_t totalAllocations = 0;
        };

        VmaAllocator Allocator;
        uint64_t TotalAllocatedBytes = 0;
        uint64_t MemoryUsage = 0; // all heaps
        std::array<MemoryTypeStats, VK_MAX_MEMORY_TYPES> memoryTypeStats;
        uint64_t totalAllocations = 0;
        uint64_t activeAllocations = 0;
        uint64_t peakMemoryUsage = 0;
    };

    // ---------------------------------------------------------

    enum class AllocationType : uint8_t
    {
        None = 0,
        Buffer = 1,
        Image = 2
    };

    struct AllocInfo
    {
        uint64_t AllocatedSize = 0;
        AllocationType Type = AllocationType::None;
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

        // Defragmentation methods
        void BeginDefragmentation(VmaDefragmentationFlags flags = 0);
        void EndDefragmentation();

        // Method to mark an allocation as defragmentable
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
            Default,        // Let VMA decide
            SpeedOptimized, // Optimize for fast allocation
            MemoryOptimized // Optimize for minimal memory usage
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

        // Batch allocation methods
        [[nodiscard]] std::vector<BatchBufferAllocation> AllocateBufferBatch(const std::vector<VkDeviceSize> &sizes, BufferUsageFlags usage, VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO);

        void FreeBufferBatch(const std::vector<BatchBufferAllocation> &allocations);

		// ---------------------------------------------------------

        VulkanAllocatorData *memAllocatorData = nullptr;
        mutable std::map<VmaAllocation, AllocInfo> allocationMap;
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

        void UnmapMemory(VmaAllocation allocation) const;
        VmaAllocator GetMemAllocator();

		void Init(VkDevice device, VkPhysicalDevice physicalDevice, VkInstance instance);
		void Shutdown();

		// ---------------------------------------------------------

    private:
        std::string Tag_;
        std::vector<VmaAllocation> defragmentationCandidates;
        VmaDefragmentationContext defragmentationContext;
        AllocationStrategy currentStrategy;

        // Fixed-size pools for common allocation sizes
        std::unordered_map<VkDeviceSize, MemoryPool> bufferPools;
        std::unordered_map<VkDeviceSize, MemoryPool> imagePools;

        // Helper methods for pool creation and retrieval
        VmaPool GetOrCreateBufferPool(VkDeviceSize size, VmaMemoryUsage usage);
        VmaPool GetOrCreateImagePool(VkDeviceSize size, VmaMemoryUsage usage);

        // For thread safety
        std::mutex allocationMutex;
        std::mutex poolMutex;

        float memoryWarningThreshold = 0.9f; // 90% usage generates warnings
        [[nodiscard]] bool CheckMemoryBudget() const;

        VkDeviceSize customBufferAlignment = 0;

        // Helper function to align buffer sizes for better caching
        [[nodiscard]] VkDeviceSize AlignBufferSize(VkDeviceSize size) const;

    };

} // namespace SceneryEditorX

// -------------------------------------------------------

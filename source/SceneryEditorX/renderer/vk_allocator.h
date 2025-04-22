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
#include <vma/vk_mem_alloc.h>
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
            createInfo.usage = usage;
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

    class MemoryAllocator
    {
    public:
        MemoryAllocator() = default;
        explicit MemoryAllocator(std::string tag);
        ~MemoryAllocator();

        // Defragmentation methods
        GLOBAL void BeginDefragmentation(VmaDefragmentationFlags flags = 0);
        GLOBAL void EndDefragmentation();

        // Method to mark an allocation as defragmentable
        GLOBAL void MarkForDefragmentation(VmaAllocation allocation);

		// ---------------------------------------------------------

        struct AllocationStats
        {
            uint64_t totalBytes;
            uint64_t usedBytes;
            uint64_t allocationCount;
            float fragmentationRatio;
        };

        [[nodiscard]] GLOBAL AllocationStats GetStats();
        void PrintDetailedStats() const;
        GLOBAL void ResetStats();

        enum class AllocationStrategy : uint8_t
        {
            Default,        // Let VMA decide
            SpeedOptimized, // Optimize for fast allocation
            MemoryOptimized // Optimize for minimal memory usage
        };

        GLOBAL void SetAllocationStrategy(AllocationStrategy strategy);
        GLOBAL void ApplyAllocationStrategy(const VmaAllocationCreateInfo &createInfo);

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
        [[nodiscard]] std::vector<BatchBufferAllocation> AllocateBufferBatch(const std::vector<VkDeviceSize> &sizes, BufferUsageFlags usage, VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO) const;

        INTERNAL void FreeBufferBatch(const std::vector<BatchBufferAllocation> &allocations);

		// ---------------------------------------------------------

        LOCAL inline VulkanAllocatorData *memAllocatorData = nullptr;
        LOCAL std::map<VmaAllocation, AllocInfo> allocationMap;
        LOCAL VmaAllocation AllocateBuffer(const VkBufferCreateInfo &bufferCreateInfo, VmaMemoryUsage usage, VkBuffer &outBuffer);
        LOCAL VmaAllocation AllocateImage(const VkImageCreateInfo &imageCreateInfo, VmaMemoryUsage usage, VkImage &outImage, VkDeviceSize* allocatedSize = nullptr);

        INTERNAL void Free(VmaAllocation allocation);
        INTERNAL void DestroyImage(VkImage image, VmaAllocation allocation);
        INTERNAL void DestroyBuffer(VkBuffer buffer, VmaAllocation allocation);

		// ---------------------------------------------------------

		template<typename T>
		T* MapMemory(const VmaAllocation allocation)
		{
			T* mappedMemory;
            vmaMapMemory(GetMemAllocator(), allocation, static_cast<void **>(&mappedMemory));
			return mappedMemory;
		}

		// ---------------------------------------------------------

        INTERNAL void UnmapMemory(VmaAllocation allocation);
        INTERNAL VmaAllocator& GetMemAllocator();

		GLOBAL void Init(VkDevice device, VkPhysicalDevice physicalDevice, VkInstance instance);
		GLOBAL void Shutdown();

		// ---------------------------------------------------------

    private:
        std::string Tag_;
        INTERNAL std::vector<VmaAllocation> defragmentationCandidates;
        INTERNAL VmaDefragmentationContext defragmentationContext;
        INTERNAL AllocationStrategy currentStrategy;
        INTERNAL void ApplyAllocationStrategy(VmaAllocationCreateInfo &createInfo);

        // Fixed-size pools for common allocation sizes
        INTERNAL std::unordered_map<VkDeviceSize, MemoryPool> bufferPools;
        INTERNAL std::unordered_map<VkDeviceSize, MemoryPool> imagePools;

        // Helper methods for pool creation and retrieval
        INTERNAL VmaPool GetOrCreateBufferPool(VkDeviceSize size, VmaMemoryUsage usage);
        INTERNAL VmaPool GetOrCreateImagePool(VkDeviceSize size, VmaMemoryUsage usage);

        // Constants for common sizes
        INTERNAL constexpr VkDeviceSize SMALL_BUFFER_SIZE  = 1024 * 256;       // 256KB
        INTERNAL constexpr VkDeviceSize MEDIUM_BUFFER_SIZE = 1024 * 1 * 1024;  // 1MB
        INTERNAL constexpr VkDeviceSize LARGE_BUFFER_SIZE  = 1024 * 16 * 1024; // 16MB

        // For thread safety
        INTERNAL std::mutex allocationMutex;
        INTERNAL std::mutex poolMutex;

        float memoryWarningThreshold = 0.9f; // 90% usage generates warnings
        [[nodiscard]] bool CheckMemoryBudget() const;

        VkDeviceSize customBufferAlignment = 0;

        // Helper function to align buffer sizes for better caching
        [[nodiscard]] VkDeviceSize AlignBufferSize(VkDeviceSize size) const;

    };

} // namespace SceneryEditorX

// -------------------------------------------------------

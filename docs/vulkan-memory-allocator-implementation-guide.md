# Scenery Editor X - Vulkan Memory Allocator Implementation Guide

---

## Introduction

This implementation guide provides detailed information about the internal workings of the Vulkan Memory Allocator system in Scenery Editor X. It covers the architecture, implementation details, and advanced usage patterns for developers working with or extending the memory allocation system.

---

## Architecture Deep Dive

### Class Hierarchy and Relationships

```
RefCounted
    └── MemoryAllocator
            ├── VulkanAllocatorData (global singleton)
            ├── MemoryPool (per-size pools)
            ├── AllocationStats (statistics tracking)
            └── BatchBufferAllocation (batch operations)
```

### Memory Management Flow

```
Application Startup
    └── MemoryAllocator::Init()
        ├── Create VmaAllocator instance
        ├── Setup Vulkan function pointers
        ├── Initialize global allocator data
        └── Configure memory pools

Memory Allocation Request
    └── AllocateBuffer()/AllocateImage()
        ├── Apply allocation strategy
        ├── Determine appropriate memory pool
        ├── Create VMA allocation
        ├── Update statistics
        └── Track allocation in global map

Memory Deallocation
    └── DestroyBuffer()/DestroyImage()
        ├── Update statistics
        ├── Remove from tracking map
        └── Free VMA allocation

Application Shutdown
    └── MemoryAllocator::Shutdown()
        ├── Destroy all memory pools
        ├── Clean up tracking structures
        └── Destroy VmaAllocator instance
```

---

## Core Data Structures

### VulkanAllocatorData

```cpp
/**
 * @brief Global singleton containing the VMA allocator and statistics
 */
struct VulkanAllocatorData
{
    VmaAllocator Allocator = nullptr;        // Main VMA allocator instance
    uint64_t BytesAllocated = 0;             // Total bytes allocated
    uint64_t BytesFreed = 0;                 // Total bytes freed
    uint64_t CurrentAllocations = 0;         // Active allocation count
    uint64_t PeakMemoryUsage = 0;            // Highest recorded usage
};
```

The global allocator data is a singleton that maintains:

- The primary VMA allocator instance used by all MemoryAllocator objects
- Global statistics tracking across all allocators
- Peak memory usage for budget monitoring

### MemoryPool Structure

```cpp
/**
 * @brief Represents a memory pool for efficient allocation
 */
struct MemoryPool
{
    VkDeviceSize blockSize = 0;              // Size of allocations this pool handles
    VkDeviceSize minAlignment = 0;           // Minimum alignment requirement
    std::vector<VmaPool> pools;              // VMA pool instances
    VmaPoolCreateInfo createInfo = {};       // Configuration for pool creation
  
    MemoryPool() : createInfo({}) {}
  
    MemoryPool(VkDeviceSize size, VmaMemoryUsage usage) 
        : blockSize(size), createInfo({})
    {
        createInfo.memoryTypeIndex = static_cast<uint32_t>(usage);
    }
};
```

Memory pools optimize allocation patterns by:

- Grouping similar-sized allocations together
- Reducing fragmentation through size-based pooling
- Enabling batch allocation and deallocation operations
- Improving cache locality for related allocations

### AllocationInfo Tracking

```cpp
/**
 * @brief Information tracked for each allocation
 */
struct AllocInfo
{
    uint64_t AllocatedSize = 0;                 // Size in bytes
    AllocationType Type = AllocationType::None; // Buffer or Image
};

/**
 * @brief Maps VMA allocations to metadata
 */
GLOBAL std::map<VmaAllocation, AllocInfo> AllocationMap;
```

This tracking system enables:

- Accurate statistics reporting
- Memory leak detection
- Defragmentation candidate identification
- Debug information for troubleshooting

---

## Implementation Details

### Initialization Process

The initialization process sets up the VMA allocator with proper Vulkan integration:

```cpp
void MemoryAllocator::Init(const Ref<VulkanDevice>& device, const uint32_t& apiVersion)
{
    // Create global allocator data
    memAllocatorData = new VulkanAllocatorData();
  
    // Configure VMA allocator
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.vulkanApiVersion = apiVersion;
    allocatorInfo.physicalDevice = device->GetPhysicalDevice()->GetGPUDevices();
    allocatorInfo.device = device->GetDevice();
    allocatorInfo.instance = RenderContext::GetInstance();
  
    // Set up Vulkan function pointers for VMA
    VmaVulkanFunctions vulkanFunctions = {};
    vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;
    allocatorInfo.pVulkanFunctions = &vulkanFunctions;
  
    // Create the VMA allocator
    VkResult result = vmaCreateAllocator(&allocatorInfo, &memAllocatorData->Allocator);
    SEDX_CORE_ASSERT(result == VK_SUCCESS, "Failed to create VMA allocator");
}
```

### Buffer Allocation Strategy

The buffer allocation process uses intelligent pool selection:

```cpp
VmaAllocation MemoryAllocator::AllocateBuffer(
    const VkBufferCreateInfo& bufferCreateInfo, 
    VmaMemoryUsage usage, 
    VkBuffer& outBuffer)
{
    std::lock_guard<std::mutex> lock(allocationMutex);
  
    // Set up allocation create info
    VmaAllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.usage = usage;
    ApplyAllocationStrategy(allocCreateInfo);
  
    // Select appropriate memory pool based on size
    const VkDeviceSize size = bufferCreateInfo.size;
    VmaPool pool = nullptr;
  
    if (size <= SMALL_BUFFER_SIZE) {
        pool = GetOrCreateBufferPool(SMALL_BUFFER_SIZE, usage);
    } else if (size <= MEDIUM_BUFFER_SIZE) {
        pool = GetOrCreateBufferPool(MEDIUM_BUFFER_SIZE, usage);
    } else if (size <= LARGE_BUFFER_SIZE) {
        pool = GetOrCreateBufferPool(LARGE_BUFFER_SIZE, usage);
    }
    // else: Use default allocator for very large buffers
  
    if (pool) {
        allocCreateInfo.pool = pool;
    }
  
    // Create the allocation
    VmaAllocation allocation;
    VmaAllocationInfo allocInfo;
    VkResult result = vmaCreateBuffer(
        memAllocatorData->Allocator, 
        &bufferCreateInfo, 
        &allocCreateInfo, 
        &outBuffer, 
        &allocation, 
        &allocInfo
    );
  
    if (result != VK_SUCCESS) {
        SEDX_CORE_ERROR("Buffer allocation failed: {}", static_cast<int>(result));
        return nullptr;
    }
  
    // Update statistics and tracking
    UpdateAllocationStatistics(allocation, allocInfo, AllocationType::Buffer);
  
    return allocation;
}
```

### Memory Pool Management

Memory pools are created on-demand and cached for reuse:

```cpp
VmaPool MemoryAllocator::GetOrCreateBufferPool(VkDeviceSize size, VmaMemoryUsage usage)
{
    std::lock_guard<std::mutex> lock(poolMutex);
  
    // Check if pool already exists
    auto it = bufferPools.find(size);
    if (it != bufferPools.end() && !it->second.pools.empty()) {
        return it->second.pools[0];
    }
  
    // Create new pool entry
    if (it == bufferPools.end()) {
        bufferPools[size] = MemoryPool(size, usage);
    }
  
    MemoryPool& pool = bufferPools[size];
  
    // Configure VMA pool
    VmaPoolCreateInfo poolInfo = pool.createInfo;
    poolInfo.blockSize = size;
    poolInfo.maxBlockCount = 0; // No limit
    poolInfo.minBlockCount = 1; // Ensure at least one block
  
    // Determine appropriate memory type based on usage
    poolInfo.memoryTypeIndex = FindMemoryTypeIndex(usage);
  
    // Create VMA pool
    VmaPool newPool;
    VkResult result = vmaCreatePool(memAllocatorData->Allocator, &poolInfo, &newPool);
  
    if (result != VK_SUCCESS) {
        SEDX_CORE_ERROR("Failed to create memory pool of size {}: {}", 
                        size, static_cast<int>(result));
        return nullptr;
    }
  
    pool.pools.push_back(newPool);
  
    SEDX_CORE_INFO_TAG("VulkanAllocator", "Created memory pool for size: {} KB", 
                       size / 1024);
  
    return newPool;
}
```

### Allocation Strategy Implementation

Different allocation strategies modify VMA flags to optimize for specific use cases:

```cpp
void MemoryAllocator::ApplyAllocationStrategy(VmaAllocationCreateInfo& createInfo) const
{
    switch (currentStrategy) {
    case AllocationStrategy::SpeedOptimized:
        // Prioritize allocation speed over memory efficiency
        createInfo.flags |= VMA_ALLOCATION_CREATE_STRATEGY_MIN_TIME_BIT;
        createInfo.flags &= ~VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT;
    
        // Use dedicated allocations for large objects to avoid searching
        if (createInfo.usage == VMA_MEMORY_USAGE_GPU_ONLY) {
            createInfo.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        }
        break;
    
    case AllocationStrategy::MemoryOptimized:
        // Prioritize memory efficiency over allocation speed
        createInfo.flags |= VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT;
        createInfo.flags &= ~VMA_ALLOCATION_CREATE_STRATEGY_MIN_TIME_BIT;
    
        // Enable defragmentation for better memory usage
        createInfo.flags |= VMA_ALLOCATION_CREATE_CAN_BECOME_LOST_BIT;
        break;
    
    case AllocationStrategy::Default:
    default:
        // Use VMA's default behavior
        break;
    }
}
```

### Defragmentation System

The defragmentation system operates in phases to minimize performance impact:

```cpp
void MemoryAllocator::BeginDefragmentation(VmaDefragmentationFlags flags)
{
    std::lock_guard<std::mutex> lock(allocationMutex);
  
    if (defragmentationContext != nullptr) {
        SEDX_CORE_WARN("Defragmentation already in progress");
        EndDefragmentation();
    }
  
    // Clear previous candidates
    defragmentationCandidates.clear();
  
    // Store flags for later use in EndDefragmentation
    defragmentationFlags = flags;
  
    SEDX_CORE_INFO("Defragmentation preparation started");
}

void MemoryAllocator::MarkForDefragmentation(VmaAllocation allocation)
{
    std::lock_guard<std::mutex> lock(allocationMutex);
  
    if (!AllocationMap.contains(allocation)) {
        SEDX_CORE_WARN("Cannot mark unknown allocation for defragmentation");
        return;
    }
  
    // Check if already marked
    auto it = std::find(defragmentationCandidates.begin(), 
                       defragmentationCandidates.end(), 
                       allocation);
    if (it != defragmentationCandidates.end()) {
        return; // Already marked
    }
  
    defragmentationCandidates.push_back(allocation);
  
    // Log periodically to avoid spam
    if (defragmentationCandidates.size() % 50 == 1) {
        const auto& allocInfo = AllocationMap[allocation];
        SEDX_CORE_INFO("Marked {} allocation for defragmentation ({} total)", 
                       allocInfo.Type == AllocationType::Buffer ? "buffer" : "image",
                       defragmentationCandidates.size());
    }
}

void MemoryAllocator::EndDefragmentation()
{
    std::lock_guard<std::mutex> lock(allocationMutex);
  
    if (defragmentationCandidates.empty()) {
        SEDX_CORE_WARN("No allocations marked for defragmentation");
        return;
    }
  
    // Configure defragmentation
    VmaDefragmentationInfo defragInfo = {};
    defragInfo.flags = defragmentationFlags;
    defragInfo.allocations = defragmentationCandidates.data();
    defragInfo.allocationCount = static_cast<uint32_t>(defragmentationCandidates.size());
  
    // Begin defragmentation
    VmaDefragmentationStats stats;
    VkResult result = vmaBeginDefragmentation(
        memAllocatorData->Allocator, 
        &defragInfo, 
        &defragmentationContext
    );
  
    if (result != VK_SUCCESS) {
        SEDX_CORE_ERROR("Failed to begin defragmentation: {}", static_cast<int>(result));
        return;
    }
  
    // Process defragmentation passes
    do {
        VmaDefragmentationPassMoveInfo passInfo;
        result = vmaBeginDefragmentationPass(defragmentationContext, &passInfo);
    
        if (result == VK_SUCCESS) {
            // Process moves (this would typically involve command buffer recording)
            ProcessDefragmentationMoves(passInfo);
            vmaEndDefragmentationPass(defragmentationContext, &passInfo);
        }
    } while (result == VK_SUCCESS);
  
    // Finalize defragmentation
    vmaEndDefragmentation(defragmentationContext, &stats);
  
    // Log results
    SEDX_CORE_INFO("Defragmentation completed:");
    SEDX_CORE_INFO("  Bytes moved: {} MB", stats.bytesMoved / (1024 * 1024));
    SEDX_CORE_INFO("  Bytes freed: {} MB", stats.bytesFreed / (1024 * 1024));
    SEDX_CORE_INFO("  Allocations moved: {}", stats.allocationsMoved);
  
    // Reset state
    defragmentationContext = nullptr;
    defragmentationCandidates.clear();
}
```

### Statistics Collection and Reporting

The statistics system provides comprehensive memory usage information:

```cpp
MemoryAllocator::AllocationStats MemoryAllocator::GetStats()
{
    std::lock_guard<std::mutex> lock(allocationMutex);
  
    AllocationStats stats{};
  
    if (!memAllocatorData || !memAllocatorData->Allocator) {
        return stats;
    }
  
    // Get VMA statistics
    VmaTotalStatistics vmaStats;
    vmaCalculateStatistics(memAllocatorData->Allocator, &vmaStats);
  
    // Fill custom stats structure
    stats.totalBytes = vmaStats.total.statistics.blockBytes;
    stats.usedBytes = vmaStats.total.statistics.allocationBytes;
    stats.allocationCount = vmaStats.total.statistics.allocationCount;
  
    // Calculate fragmentation ratio
    if (vmaStats.total.statistics.blockBytes > 0) {
        stats.fragmentationRatio = 1.0f - 
            static_cast<float>(vmaStats.total.statistics.allocationBytes) / 
            static_cast<float>(vmaStats.total.statistics.blockBytes);
    } else {
        stats.fragmentationRatio = 0.0f;
    }
  
    return stats;
}

void MemoryAllocator::PrintDetailedStats() const
{
    std::lock_guard<std::mutex> lock(allocationMutex);
  
    // Get comprehensive VMA statistics
    VmaTotalStatistics vmaStats;
    vmaCalculateStatistics(memAllocatorData->Allocator, &vmaStats);
  
    // Get memory budget information
    VmaBudget budgets[VK_MAX_MEMORY_HEAPS];
    vmaGetHeapBudgets(memAllocatorData->Allocator, budgets);
  
    // Get physical device properties
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(
        RenderContext::GetCurrentDevice()->GetPhysicalDevice()->GetGPUDevices(), 
        &memProps
    );
  
    SEDX_CORE_INFO("=== VULKAN MEMORY ALLOCATOR STATISTICS ===");
    SEDX_CORE_INFO("Allocator Tag: {}", Tag_);
  
    // Overall statistics
    SEDX_CORE_INFO("Total allocated: {} MB", 
                   vmaStats.total.statistics.blockBytes / (1024 * 1024));
    SEDX_CORE_INFO("Actually used: {} MB", 
                   vmaStats.total.statistics.allocationBytes / (1024 * 1024));
    SEDX_CORE_INFO("Wasted (fragmentation): {} MB", 
                   (vmaStats.total.statistics.blockBytes - 
                    vmaStats.total.statistics.allocationBytes) / (1024 * 1024));
  
    float fragmentation = 0.0f;
    if (vmaStats.total.statistics.blockBytes > 0) {
        fragmentation = (1.0f - 
            static_cast<float>(vmaStats.total.statistics.allocationBytes) / 
            static_cast<float>(vmaStats.total.statistics.blockBytes)) * 100.0f;
    }
    SEDX_CORE_INFO("Fragmentation ratio: {:.2f}%", fragmentation);
  
    SEDX_CORE_INFO("Active allocations: {}", vmaStats.total.statistics.allocationCount);
    SEDX_CORE_INFO("Memory blocks: {}", vmaStats.total.statistics.blockCount);
  
    // Per-heap budget information
    SEDX_CORE_INFO("=== MEMORY HEAP BUDGETS ===");
    for (uint32_t i = 0; i < memProps.memoryHeapCount; i++) {
        const char* heapType = (memProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) 
            ? "DEVICE" : "HOST";
        float usagePercent = static_cast<float>(budgets[i].usage) / 
                            static_cast<float>(budgets[i].budget) * 100.0f;
    
        SEDX_CORE_INFO("Heap {}: {} - {} MB / {} MB ({:.1f}%)", 
                       i, heapType,
                       budgets[i].usage / (1024 * 1024),
                       budgets[i].budget / (1024 * 1024),
                       usagePercent);
    }
  
    // Per-memory-type allocation details
    SEDX_CORE_INFO("=== MEMORY TYPE DETAILS ===");
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        const auto& typeStats = memoryTypeStats[i];
        if (typeStats.CurrentAllocations == 0) continue;
    
        std::string properties;
        VkMemoryPropertyFlags flags = memProps.memoryTypes[i].propertyFlags;
        if (flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) properties += "DEVICE_LOCAL ";
        if (flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) properties += "HOST_VISIBLE ";
        if (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) properties += "HOST_COHERENT ";
        if (flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) properties += "HOST_CACHED ";
    
        SEDX_CORE_INFO("Type {}: Heap {}, Properties: {}", 
                       i, memProps.memoryTypes[i].heapIndex, properties);
        SEDX_CORE_INFO("  Active: {} MB ({} allocations)",
                       (typeStats.BytesAllocated - typeStats.BytesFreed) / (1024 * 1024),
                       typeStats.CurrentAllocations);
    }
  
    SEDX_CORE_INFO("Peak memory usage: {} MB", 
                   memAllocatorData->PeakMemoryUsage / (1024 * 1024));
    SEDX_CORE_INFO("===========================================");
}
```

---

## Advanced Features

### Batch Allocation Optimization

Batch allocation reduces overhead by processing multiple allocations together:

```cpp
std::vector<MemoryAllocator::BatchBufferAllocation> 
MemoryAllocator::AllocateBufferBatch(
    const std::vector<VkDeviceSize>& sizes, 
    BufferUsageFlags usage, 
    VmaMemoryUsage memoryUsage) const
{
    std::vector<BatchBufferAllocation> allocations;
    allocations.reserve(sizes.size());
  
    // Pre-sort by size for better pool utilization
    std::vector<std::pair<VkDeviceSize, size_t>> sortedSizes;
    for (size_t i = 0; i < sizes.size(); ++i) {
        sortedSizes.emplace_back(sizes[i], i);
    }
    std::sort(sortedSizes.begin(), sortedSizes.end());
  
    // Group allocations by size categories
    struct SizeGroup {
        VmaPool pool;
        std::vector<std::pair<VkDeviceSize, size_t>> allocations;
    };
  
    std::map<VkDeviceSize, SizeGroup> sizeGroups;
  
    for (const auto& [size, index] : sortedSizes) {
        VkDeviceSize poolSize;
        if (size <= SMALL_BUFFER_SIZE) poolSize = SMALL_BUFFER_SIZE;
        else if (size <= MEDIUM_BUFFER_SIZE) poolSize = MEDIUM_BUFFER_SIZE;
        else if (size <= LARGE_BUFFER_SIZE) poolSize = LARGE_BUFFER_SIZE;
        else poolSize = 0; // Use default allocator
    
        if (poolSize > 0) {
            if (sizeGroups.find(poolSize) == sizeGroups.end()) {
                sizeGroups[poolSize].pool = GetOrCreateBufferPool(poolSize, memoryUsage);
            }
            sizeGroups[poolSize].allocations.emplace_back(size, index);
        }
    }
  
    // Allocate each group
    for (auto& [poolSize, group] : sizeGroups) {
        AllocateGroupBuffers(group, usage, memoryUsage, allocations);
    }
  
    return allocations;
}
```

### Memory Budget Enforcement

The budget system can enforce memory limits and trigger cleanup:

```cpp
bool MemoryAllocator::CheckMemoryBudget() const
{
    if (!memAllocatorData || !memAllocatorData->Allocator) {
        return false;
    }
  
    VmaBudget budgets[VK_MAX_MEMORY_HEAPS];
    vmaGetHeapBudgets(memAllocatorData->Allocator, budgets);
  
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(
        RenderContext::GetCurrentDevice()->GetPhysicalDevice()->GetGPUDevices(), 
        &memProps
    );
  
    uint64_t totalUsage = 0;
    uint64_t totalBudget = 0;
    bool anyHeapOverBudget = false;
  
    for (uint32_t i = 0; i < memProps.memoryHeapCount; i++) {
        totalUsage += budgets[i].usage;
        totalBudget += budgets[i].budget;
    
        float heapUsage = static_cast<float>(budgets[i].usage) / 
                         static_cast<float>(budgets[i].budget);
    
        if (heapUsage > memoryWarningThreshold) {
            SEDX_CORE_WARN_TAG("Memory", "Heap {} usage: {:.1f}% ({} MB / {} MB)",
                               i, heapUsage * 100.0f,
                               budgets[i].usage / (1024 * 1024),
                               budgets[i].budget / (1024 * 1024));
            anyHeapOverBudget = true;
        }
    }
  
    float totalUsage_pct = static_cast<float>(totalUsage) / static_cast<float>(totalBudget);
    bool overBudget = totalUsage_pct > memoryWarningThreshold;
  
    if (overBudget) {
        SEDX_CORE_WARN_TAG("Memory", "Total memory usage: {:.1f}% ({} MB / {} MB)",
                           totalUsage_pct * 100.0f,
                           totalUsage / (1024 * 1024),
                           totalBudget / (1024 * 1024));
    
        // Trigger automatic cleanup if enabled
        if (m_AutoCleanupEnabled) {
            TriggerMemoryCleanup();
        }
    }
  
    return overBudget || anyHeapOverBudget;
}

void MemoryAllocator::TriggerMemoryCleanup()
{
    SEDX_CORE_INFO_TAG("Memory", "Triggering automatic memory cleanup");
  
    // Begin aggressive defragmentation
    BeginDefragmentation(VMA_DEFRAGMENTATION_FLAG_ALGORITHM_FULL_BIT);
  
    // Mark old allocations for defragmentation
    for (const auto& [allocation, allocInfo] : AllocationMap) {
        // Add heuristics to identify good defragmentation candidates
        if (ShouldDefragmentAllocation(allocation, allocInfo)) {
            MarkForDefragmentation(allocation);
        }
    }
  
    EndDefragmentation();
  
    // Notify application of memory pressure
    if (m_MemoryPressureCallback) {
        m_MemoryPressureCallback(GetMemoryBudget());
    }
}
```

---

## Performance Optimizations

### Cache-Friendly Allocation Patterns

```cpp
VkDeviceSize MemoryAllocator::AlignBufferSize(VkDeviceSize size) const
{
    if (size == 0) return 0;
  
    // Apply custom alignment if specified
    if (customBufferAlignment > 0) {
        return ((size + customBufferAlignment - 1) / customBufferAlignment) * customBufferAlignment;
    }
  
    // Smart alignment based on size
    if (size < SMALL_BUFFER_SIZE) {
        // For small buffers, align to cache line size (64 bytes) for better cache efficiency
        constexpr VkDeviceSize cacheLineSize = 64;
        return ((size + cacheLineSize - 1) / cacheLineSize) * cacheLineSize;
    }
  
    if (size < MEDIUM_BUFFER_SIZE) {
        // For medium buffers, align to page size (4KB) for better memory management
        constexpr VkDeviceSize pageSize = 4 * 1024;
        return ((size + pageSize - 1) / pageSize) * pageSize;
    }
  
    // For large buffers, align to large page size (2MB) where supported
    constexpr VkDeviceSize largePageSize = 2 * 1024 * 1024;
    return ((size + largePageSize - 1) / largePageSize) * largePageSize;
}
```

### Memory Type Selection Optimization

```cpp
uint32_t MemoryAllocator::FindOptimalMemoryTypeIndex(
    VmaMemoryUsage usage, 
    VkMemoryPropertyFlags requiredFlags,
    VkMemoryPropertyFlags preferredFlags) const
{
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(
        RenderContext::GetCurrentDevice()->GetPhysicalDevice()->GetGPUDevices(), 
        &memProps
    );
  
    // First pass: find exact match
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if ((memProps.memoryTypes[i].propertyFlags & requiredFlags) == requiredFlags &&
            (memProps.memoryTypes[i].propertyFlags & preferredFlags) == preferredFlags) {
            return i;
        }
    }
  
    // Second pass: find compatible match
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if ((memProps.memoryTypes[i].propertyFlags & requiredFlags) == requiredFlags) {
            return i;
        }
    }
  
    SEDX_CORE_ERROR("No suitable memory type found");
    return 0;
}
```

---

## Integration Points

### Vulkan Device Integration

```cpp
// In VulkanDevice class
class VulkanDevice : public RefCounted
{
public:
    void InitializeMemoryAllocator()
    {
        // Initialize after device creation
        MemoryAllocator::Init(this, m_VulkanApiVersion);
    
        // Create device-specific allocator
        m_DeviceAllocator = CreateRef<MemoryAllocator>("VulkanDevice");
        m_DeviceAllocator->SetAllocationStrategy(
            MemoryAllocator::AllocationStrategy::SpeedOptimized
        );
    
        // Configure for this device's capabilities
        auto deviceLimits = m_PhysicalDevice->GetLimits();
        m_DeviceAllocator->SetBufferAlignment(deviceLimits.nonCoherentAtomSize);
    
        if (SupportsLargeBuffers()) {
            MemoryAllocator::SetCustomBufferSize(64 * 1024 * 1024, *this);
        }
    }
  
private:
    Ref<MemoryAllocator> m_DeviceAllocator;
};
```

### Renderer Integration

```cpp
// In Renderer class
class Renderer : public Module
{
public:
    void OnAttach() override
    {
        SEDX_CORE_INFO("=== Initializing Renderer ===");
    
        // Create allocator for renderer resources
        m_Allocator = CreateRef<MemoryAllocator>("Renderer");
        m_Allocator->SetAllocationStrategy(MemoryAllocator::AllocationStrategy::Default);
        m_Allocator->SetMemoryUsageWarningThreshold(0.85f);
    
        // Set up memory pressure callback
        m_Allocator->SetMemoryPressureCallback([this](const auto& budget) {
            HandleMemoryPressure(budget);
        });
    
        // Initialize render resources
        InitializeRenderResources();
    }
  
    void OnUpdate() override
    {
        SEDX_PROFILE_SCOPE("Renderer::OnUpdate");
    
        // Check memory budget periodically
        static int frameCount = 0;
        if (++frameCount % 120 == 0) { // Every 2 seconds at 60 FPS
            auto budget = m_Allocator->GetMemoryBudget();
            if (budget.isOverBudget) {
                SEDX_CORE_WARN_TAG("RENDERER", "Memory budget exceeded: {:.1f}%", 
                                   budget.usagePercentage * 100.0f);
                OptimizeMemoryUsage();
            }
        }
    }
  
private:
    void HandleMemoryPressure(const MemoryAllocator::MemoryBudget& budget)
    {
        SEDX_CORE_WARN_TAG("RENDERER", "Memory pressure detected: {:.1f}% usage", 
                           budget.usagePercentage * 100.0f);
    
        // Implement LOD reduction, texture streaming, etc.
        ReduceDetailLevel();
        FlushUnusedResources();
    
        // Trigger defragmentation
        m_Allocator->BeginDefragmentation();
        MarkOldResourcesForDefragmentation();
        m_Allocator->EndDefragmentation();
    }
  
    Ref<MemoryAllocator> m_Allocator;
};
```

---

## Debugging and Troubleshooting

### Memory Leak Detection

```cpp
class MemoryLeakDetector
{
public:
    static void CheckForLeaks()
    {
        auto stats = MemoryAllocator::GetStats();
    
        if (stats.allocationCount > 0) {
            SEDX_CORE_ERROR("Memory leak detected: {} allocations still active", 
                            stats.allocationCount);
        
            // Print details of remaining allocations
            for (const auto& [allocation, info] : AllocationMap) {
                const char* typeStr = info.Type == AllocationType::Buffer ? "Buffer" : "Image";
                SEDX_CORE_ERROR("  Leaked {}: {} bytes", typeStr, info.AllocatedSize);
            }
        }
    }
  
    static void EnableLeakDetection()
    {
        // Set up allocation tracking for leak detection
        s_EnableLeakDetection = true;
    }
  
private:
    static bool s_EnableLeakDetection;
};
```

### Performance Profiling

```cpp
class MemoryProfiler
{
public:
    struct AllocationEvent
    {
        std::chrono::high_resolution_clock::time_point timestamp;
        VkDeviceSize size;
        AllocationType type;
        bool isAllocation; // true for allocation, false for deallocation
    };
  
    static void RecordAllocation(VkDeviceSize size, AllocationType type)
    {
        if (!s_ProfilingEnabled) return;
    
        AllocationEvent event;
        event.timestamp = std::chrono::high_resolution_clock::now();
        event.size = size;
        event.type = type;
        event.isAllocation = true;
    
        std::lock_guard<std::mutex> lock(s_EventsMutex);
        s_AllocationEvents.push_back(event);
    }
  
    static void GenerateReport()
    {
        std::lock_guard<std::mutex> lock(s_EventsMutex);
    
        SEDX_CORE_INFO("=== MEMORY ALLOCATION PROFILE ===");
    
        uint64_t totalAllocated = 0;
        uint64_t totalDeallocated = 0;
        size_t allocationCount = 0;
        size_t deallocationCount = 0;
    
        for (const auto& event : s_AllocationEvents) {
            if (event.isAllocation) {
                totalAllocated += event.size;
                allocationCount++;
            } else {
                totalDeallocated += event.size;
                deallocationCount++;
            }
        }
    
        SEDX_CORE_INFO("Total allocated: {} MB ({} operations)", 
                       totalAllocated / (1024 * 1024), allocationCount);
        SEDX_CORE_INFO("Total deallocated: {} MB ({} operations)", 
                       totalDeallocated / (1024 * 1024), deallocationCount);
        SEDX_CORE_INFO("Net allocation: {} MB", 
                       (totalAllocated - totalDeallocated) / (1024 * 1024));
    
        // Calculate allocation rate
        if (!s_AllocationEvents.empty()) {
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(
                s_AllocationEvents.back().timestamp - s_AllocationEvents.front().timestamp
            ).count();
        
            if (duration > 0) {
                SEDX_CORE_INFO("Allocation rate: {:.2f} ops/sec", 
                               static_cast<double>(allocationCount) / duration);
            }
        }
    }
  
private:
    static std::vector<AllocationEvent> s_AllocationEvents;
    static std::mutex s_EventsMutex;
    static bool s_ProfilingEnabled;
};
```

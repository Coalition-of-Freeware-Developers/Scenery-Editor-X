# Vulkan Memory Allocator System Instructions

## Overview

This document provides GitHub Copilot with specific instructions for working with the Vulkan Memory Allocator system in Scenery Editor X. These guidelines ensure consistent implementation patterns, optimal performance, and adherence to the project's architectural principles.

## Core Principles

### 1. Always Use the MemoryAllocator Class

**DO:**

```cpp
// Create allocator instance with identifying tag
auto allocator = CreateRef<MemoryAllocator>("ModuleName");

// Use allocator for all memory operations
VmaAllocation allocation = allocator->AllocateBuffer(bufferInfo, usage, buffer);
allocator->DestroyBuffer(buffer, allocation);
```

**DON'T:**

```cpp
// Never use raw VMA calls directly
vmaCreateBuffer(vmaAllocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr);
vmaDestroyBuffer(vmaAllocator, buffer, allocation);
```

### 2. Integrate with Module System

**Always inherit from Module and follow RAII principles:**

```cpp
class RenderModule : public Module
{
public:
    void OnAttach() override
    {
        SEDX_CORE_INFO("=== Initializing {} ===", GetName());
      
        // Create module-specific allocator
        m_Allocator = CreateRef<MemoryAllocator>(GetName());
        m_Allocator->SetAllocationStrategy(MemoryAllocator::AllocationStrategy::SpeedOptimized);
      
        // Initialize resources
        InitializeResources();
    }
  
    void OnDetach() override
    {
        SEDX_CORE_INFO("=== Cleaning up {} ===", GetName());
      
        // Print final statistics
        m_Allocator->PrintDetailedStats();
      
        // Automatic cleanup through smart pointers
        m_Allocator.Reset();
    }
  
private:
    Ref<MemoryAllocator> m_Allocator;
};
```

### 3. Use Smart Pointers for Resource Management

**Always use the custom smart pointer system:**

```cpp
// Create resources with smart pointers
auto texture = CreateRef<Texture2D>();
auto mesh = CreateRef<Mesh>();

// Store in containers using Ref<T>
std::vector<Ref<Texture2D>> m_Textures;
std::unordered_map<std::string, Ref<Mesh>> m_Meshes;

// Use WeakRef<T> for non-owning references
WeakRef<MemoryAllocator> m_AllocatorRef;
```

### 4. Implement Comprehensive Error Handling

**Always validate allocations and use proper logging:**

```cpp
VmaAllocation allocation = allocator->AllocateBuffer(bufferInfo, usage, buffer);

// Validate allocation success
SEDX_CORE_ASSERT(allocation != nullptr, "Failed to allocate buffer memory");
SEDX_CORE_VERIFY(allocator->ContainsAllocation(allocation), "Allocation not tracked");

// Log allocation details
SEDX_CORE_INFO_TAG("RENDERER", "Allocated buffer: {} KB", bufferSize / 1024);
```

## Implementation Patterns

### 1. Buffer Allocation Pattern

```cpp
VkBuffer CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
{
    // Set up buffer creation info
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  
    // Allocate using the module's allocator
    VkBuffer buffer;
    VmaAllocation allocation = m_Allocator->AllocateBuffer(bufferInfo, memoryUsage, buffer);
  
    // Validate and log
    SEDX_CORE_ASSERT(allocation != nullptr, "Buffer allocation failed");
    SEDX_CORE_INFO_TAG(GetName(), "Allocated buffer: {} KB", size / 1024);
  
    // Store for cleanup (use RAII container)
    m_BufferAllocations[buffer] = allocation;
  
    return buffer;
}
```

### 2. Image Allocation Pattern

```cpp
VkImage CreateImage(const VkImageCreateInfo& imageInfo, VmaMemoryUsage memoryUsage)
{
    VkImage image;
    VkDeviceSize allocatedSize;
    VmaAllocation allocation = m_Allocator->AllocateImage(
        imageInfo, memoryUsage, image, &allocatedSize);
  
    SEDX_CORE_ASSERT(allocation != nullptr, "Image allocation failed");
    SEDX_CORE_INFO_TAG(GetName(), "Allocated image: {} MB", allocatedSize / (1024 * 1024));
  
    // Store allocation info
    m_ImageAllocations[image] = allocation;
  
    return image;
}
```

### 3. Memory Mapping Pattern

```cpp
template<typename T>
void UpdateBufferData(VkBuffer buffer, const std::vector<T>& data)
{
    // Create staging buffer for CPU-to-GPU transfer
    VkDeviceSize dataSize = sizeof(T) * data.size();
  
    VkBufferCreateInfo stagingInfo{};
    stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingInfo.size = dataSize;
    stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  
    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation = m_Allocator->AllocateBuffer(
        stagingInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, stagingBuffer);
  
    // Map and copy data
    T* mappedData = m_Allocator->MapMemory<T>(stagingAllocation);
    memcpy(mappedData, data.data(), dataSize);
    MemoryAllocator::UnmapMemory(stagingAllocation);
  
    // Copy to GPU buffer
    CopyBuffer(stagingBuffer, buffer, dataSize);
  
    // Cleanup staging buffer
    m_Allocator->DestroyBuffer(stagingBuffer, stagingAllocation);
}
```

### 4. Batch Allocation Pattern

```cpp
void LoadScene(const SceneData& sceneData)
{
    SEDX_PROFILE_SCOPE("SceneLoader::LoadScene");
  
    // Prepare buffer sizes for batch allocation
    std::vector<VkDeviceSize> bufferSizes;
    for (const auto& mesh : sceneData.meshes) {
        bufferSizes.push_back(mesh.vertexDataSize);
        bufferSizes.push_back(mesh.indexDataSize);
    }
  
    // Allocate all buffers in batch
    auto batchAllocations = m_Allocator->AllocateBufferBatch(
        bufferSizes,
        BufferUsage::Vertex | BufferUsage::Index,
        VMA_MEMORY_USAGE_GPU_ONLY
    );
  
    // Process allocated buffers
    size_t allocIndex = 0;
    for (const auto& mesh : sceneData.meshes) {
        ProcessMeshBuffers(mesh, batchAllocations[allocIndex], batchAllocations[allocIndex + 1]);
        allocIndex += 2;
    }
}
```

## Configuration Guidelines

### 1. Allocation Strategy Selection

```cpp
// For rendering modules (prioritize speed)
allocator->SetAllocationStrategy(MemoryAllocator::AllocationStrategy::SpeedOptimized);

// For asset loading (prioritize memory efficiency)
allocator->SetAllocationStrategy(MemoryAllocator::AllocationStrategy::MemoryOptimized);

// For general purpose (use default)
allocator->SetAllocationStrategy(MemoryAllocator::AllocationStrategy::Default);
```

### 2. Memory Budget Configuration

```cpp
// Set warning threshold based on module requirements
allocator->SetMemoryUsageWarningThreshold(0.85f); // 85% for critical modules
allocator->SetMemoryUsageWarningThreshold(0.90f); // 90% for general modules

// Set custom buffer alignment for performance
allocator->SetBufferAlignment(256); // 256-byte alignment for cache efficiency
```

### 3. Device-Specific Configuration

```cpp
void ConfigureForDevice(const Ref<VulkanDevice>& device)
{
    // Set custom buffer size for large allocations
    if (MemoryAllocator::SetCustomBufferSize(64 * 1024 * 1024, *device)) {
        SEDX_CORE_INFO("Enabled large buffer support (64MB)");
    }
  
    // Adjust alignment based on device properties
    auto limits = device->GetPhysicalDevice()->GetLimits();
    allocator->SetBufferAlignment(limits.nonCoherentAtomSize);
}
```

## Memory Monitoring and Optimization

### 1. Statistics Monitoring

```cpp
void OnUpdate() override
{
    SEDX_PROFILE_SCOPE("Module::OnUpdate");
  
    // Check memory budget periodically
    static int frameCount = 0;
    if (++frameCount % 120 == 0) { // Every 2 seconds at 60 FPS
        auto budget = m_Allocator->GetMemoryBudget();
        if (budget.isOverBudget) {
            SEDX_CORE_WARN_TAG(GetName(), "Memory budget exceeded: {:.1f}%", 
                               budget.usagePercentage * 100.0f);
            HandleMemoryPressure();
        }
    }
}
```

### 2. Defragmentation Implementation

```cpp
void PerformDefragmentation()
{
    SEDX_PROFILE_SCOPE("Module::PerformDefragmentation");
  
    SEDX_CORE_INFO_TAG(GetName(), "Starting memory defragmentation");
  
    // Begin defragmentation process
    m_Allocator->BeginDefragmentation(VMA_DEFRAGMENTATION_FLAG_ALGORITHM_BALANCED_BIT);
  
    // Mark old/unused allocations for defragmentation
    for (const auto& [buffer, allocation] : m_BufferAllocations) {
        if (ShouldDefragmentBuffer(buffer)) {
            m_Allocator->MarkForDefragmentation(allocation);
        }
    }
  
    // Execute defragmentation
    m_Allocator->EndDefragmentation();
}
```

### 3. Memory Pressure Handling

```cpp
void HandleMemoryPressure()
{
    SEDX_CORE_WARN_TAG(GetName(), "Handling memory pressure");
  
    // Free unused resources
    CleanupUnusedResources();
  
    // Reduce detail levels if possible
    ReduceLODLevels();
  
    // Trigger defragmentation
    PerformDefragmentation();
  
    // Report new memory usage
    auto stats = m_Allocator->GetStats();
    SEDX_CORE_INFO_TAG(GetName(), "Post-cleanup memory usage: {} MB", 
                       stats.usedBytes / (1024 * 1024));
}
```

## Debugging and Profiling

### 1. Tagged Logging

```cpp
// Use consistent tagging for memory-related logs
SEDX_CORE_INFO_TAG("VulkanAllocator", "Allocated buffer: {} KB", size / 1024);
SEDX_CORE_WARN_TAG("VulkanAllocator", "Memory usage warning: {}%", usagePercent);
SEDX_CORE_ERROR_TAG("VulkanAllocator", "Allocation failed: {}", errorCode);

// Module-specific memory logs
SEDX_CORE_INFO_TAG(GetName(), "Module memory usage: {} MB", usage / (1024 * 1024));
```

### 2. Profiling Integration

```cpp
void OnUpdate() override
{
    SEDX_PROFILE_SCOPE("Module::OnUpdate");
  
    // Memory-intensive operations should be profiled
    {
        SEDX_PROFILE_SCOPE("Memory Operations");
        ProcessMemoryOperations();
    }
}
```

### 3. Debug Information

```cpp
#ifdef SEDX_DEBUG
void PrintDebugInfo()
{
    SEDX_CORE_INFO("=== {} Memory Debug Info ===", GetName());
  
    auto stats = m_Allocator->GetStats();
    SEDX_CORE_INFO("Active allocations: {}", stats.allocationCount);
    SEDX_CORE_INFO("Memory usage: {} MB", stats.usedBytes / (1024 * 1024));
    SEDX_CORE_INFO("Fragmentation: {:.2f}%", stats.fragmentationRatio * 100.0f);
  
    // Print detailed statistics
    m_Allocator->PrintDetailedStats();
}
#endif
```

## Common Patterns to Follow

### 1. Resource Lifecycle Management

```cpp
class ResourceManager : public Module
{
public:
    void OnAttach() override
    {
        m_Allocator = CreateRef<MemoryAllocator>(GetName());
        // Initialize resources
    }
  
    void OnDetach() override
    {
        // Resources automatically cleaned up by smart pointers
        m_Resources.clear();
        m_Allocator.Reset();
    }
  
private:
    struct Resource
    {
        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation allocation = nullptr;
      
        ~Resource()
        {
            // Cleanup handled by allocator's destructor
        }
    };
  
    std::vector<std::unique_ptr<Resource>> m_Resources;
    Ref<MemoryAllocator> m_Allocator;
};
```

### 2. Thread-Safe Operations

```cpp
// The allocator is thread-safe, but still use proper synchronization
class ThreadSafeRenderer
{
public:
    VkBuffer AllocateBuffer(VkDeviceSize size)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
      
        // Safe to call from multiple threads
        return m_Allocator->AllocateBuffer(bufferInfo, usage, buffer);
    }
  
private:
    std::mutex m_Mutex;
    Ref<MemoryAllocator> m_Allocator;
};
```

### 3. Error Recovery Patterns

```cpp
bool SafeAllocateResource(ResourceType type, VkDeviceSize size)
{
    try
    {
        auto allocation = m_Allocator->AllocateBuffer(bufferInfo, usage, buffer);
        if (allocation == nullptr) {
            SEDX_CORE_ERROR_TAG(GetName(), "Primary allocation failed, attempting fallback");
          
            // Try with different memory usage
            allocation = m_Allocator->AllocateBuffer(bufferInfo, fallbackUsage, buffer);
            if (allocation == nullptr) {
                SEDX_CORE_ERROR_TAG(GetName(), "All allocation attempts failed");
                return false;
            }
        }
      
        // Store successful allocation
        StoreAllocation(buffer, allocation);
        return true;
    }
    catch (const std::exception& e)
    {
        SEDX_CORE_ERROR_TAG(GetName(), "Exception during allocation: {}", e.what());
        return false;
    }
}
```

## Anti-Patterns to Avoid

### 1. DON'T Mix Allocation Systems

```cpp
// WRONG: Don't mix VMA calls with MemoryAllocator
VmaAllocation directAllocation;
vmaCreateBuffer(vmaAllocator, &bufferInfo, &allocInfo, &buffer, &directAllocation, nullptr);

// CORRECT: Use MemoryAllocator consistently
VmaAllocation allocation = m_Allocator->AllocateBuffer(bufferInfo, usage, buffer);
```

### 2. DON'T Ignore Memory Budget

```cpp
// WRONG: Allocate without checking budget
for (int i = 0; i < 1000; ++i) {
    AllocateLargeBuffer(); // Could exhaust memory
}

// CORRECT: Check budget and handle pressure
auto budget = m_Allocator->GetMemoryBudget();
if (budget.usagePercentage > 0.8f) {
    SEDX_CORE_WARN("High memory usage, reducing allocations");
    return;
}
```

### 3. DON'T Skip Statistics Monitoring

```cpp
// WRONG: No monitoring
void OnDetach() override
{
    m_Allocator.Reset(); // No insight into memory usage
}

// CORRECT: Print statistics before cleanup
void OnDetach() override
{
    SEDX_CORE_INFO("=== Final Memory Statistics ===");
    m_Allocator->PrintDetailedStats();
    m_Allocator.Reset();
}
```

## Performance Optimization Guidelines

### 1. Use Batch Allocation for Multiple Resources

```cpp
// GOOD: Batch allocate related resources
auto allocations = m_Allocator->AllocateBufferBatch(sizes, usage, memoryUsage);

// AVOID: Individual allocations for related resources
for (auto size : sizes) {
    auto allocation = m_Allocator->AllocateBuffer(bufferInfo, usage, buffer);
}
```

### 2. Configure Allocation Strategy Based on Use Case

```cpp
// For real-time rendering
allocator->SetAllocationStrategy(MemoryAllocator::AllocationStrategy::SpeedOptimized);

// For memory-constrained scenarios
allocator->SetAllocationStrategy(MemoryAllocator::AllocationStrategy::MemoryOptimized);
```

### 3. Implement Smart Defragmentation

```cpp
void SmartDefragmentation()
{
    auto stats = m_Allocator->GetStats();
  
    // Only defragment if fragmentation is significant
    if (stats.fragmentationRatio > 0.3f) {
        PerformDefragmentation();
    }
}
```

## Integration with Framework Components

### 1. With Smart Pointers

```cpp
class Texture2D : public RefCounted
{
public:
    void Initialize(const TextureCreateInfo& info)
    {
        VkImageCreateInfo imageInfo = BuildImageCreateInfo(info);
      
        // Use module's allocator
        m_Image = m_Allocator->AllocateImage(imageInfo, VMA_MEMORY_USAGE_GPU_ONLY, image);
      
        SEDX_CORE_ASSERT(m_Image != nullptr, "Failed to allocate texture memory");
    }
  
private:
    VkImage m_Image = VK_NULL_HANDLE;
    VmaAllocation m_Allocation = nullptr;
    Ref<MemoryAllocator> m_Allocator;
};
```

### 2. With Logging System

```cpp
// Always use tagged logging for memory operations
SEDX_CORE_INFO_TAG("MEMORY", "System initialized with {} MB budget", 
                   budget / (1024 * 1024));
SEDX_CORE_WARN_TAG("MEMORY", "High fragmentation: {:.1f}%", fragmentation * 100.0f);
SEDX_CORE_ERROR_TAG("MEMORY", "Allocation failed: insufficient memory");
```

### 3. With Module System Lifecycle

```cpp
// Follow module lifecycle patterns
void OnAttach() override
{
    SEDX_CORE_INFO("=== Initializing {} ===", GetName());
    m_Allocator = CreateRef<MemoryAllocator>(GetName());
    ConfigureAllocator();
    InitializeResources();
}

void OnDetach() override
{
    SEDX_CORE_INFO("=== Shutting down {} ===", GetName());
    m_Allocator->PrintDetailedStats();
    CleanupResources();
    m_Allocator.Reset();
}
```

## Summary

These instructions ensure that GitHub Copilot will generate code that:

1. **Follows architectural patterns** - Proper module integration and RAII principles
2. **Uses framework components** - Smart pointers, logging, and error handling
3. **Implements best practices** - Memory monitoring, defragmentation, and optimization
4. **Maintains consistency** - Standardized patterns and naming conventions
5. **Provides robustness** - Error handling, validation, and recovery mechanisms

Always prioritize **memory efficiency**, **performance optimization**, and **framework integration** when generating code that uses the Vulkan Memory Allocator system.

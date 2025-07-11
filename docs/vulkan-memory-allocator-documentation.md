# Scenery Editor X - Vulkan Memory Allocator System Documentation

---

## Overview

The Vulkan Memory Allocator (VMA) system in Scenery Editor X provides a sophisticated, high-performance memory management solution for Vulkan applications. Built on top of the VulkanMemoryAllocator (VMA) library, it offers automatic memory allocation, defragmentation, memory budget tracking, and optimization features specifically designed for 3D graphics applications.

---

## Architecture

### Core Components

The memory allocator system consists of several key components:

1. **MemoryAllocator Class** - Main interface for memory operations
2. **VulkanAllocatorData** - Global allocator state and statistics
3. **Memory Pools** - Optimized pools for different allocation sizes
4. **Defragmentation System** - Automated memory optimization
5. **Statistics and Monitoring** - Comprehensive memory usage tracking

### Design Principles

- **Performance First**: Optimized for real-time 3D rendering workloads
- **Memory Efficiency**: Automatic defragmentation and pool management
- **Thread Safety**: Full thread-safe operations with mutex protection
- **Monitoring**: Detailed statistics and budget tracking
- **Flexibility**: Multiple allocation strategies and customizable parameters

---

## Key Features

### 1. Intelligent Memory Pools

The system automatically manages memory pools for different allocation sizes:

```cpp
// Predefined pool sizes for optimal allocation patterns
constexpr VkDeviceSize SMALL_BUFFER_SIZE = 1024 * 256;       // 256KB
constexpr VkDeviceSize MEDIUM_BUFFER_SIZE = 1024 * 1024;     // 1MB
constexpr VkDeviceSize LARGE_BUFFER_SIZE = 1024 * 16 * 1024; // 16MB
```

### 2. Allocation Strategies

Three distinct allocation strategies optimize for different use cases:

- **Default**: Balanced approach letting VMA decide
- **SpeedOptimized**: Prioritizes allocation speed for real-time applications
- **MemoryOptimized**: Minimizes memory usage and fragmentation

### 3. Defragmentation System

Automated memory defragmentation reduces fragmentation and improves performance:

```cpp
// Begin defragmentation with specific algorithm
allocator.BeginDefragmentation(VMA_DEFRAGMENTATION_FLAG_ALGORITHM_BALANCED_BIT);

// Mark allocations for defragmentation
allocator.MarkForDefragmentation(allocation);

// Execute defragmentation
allocator.EndDefragmentation();
```

### 4. Memory Budget Monitoring

Real-time memory budget tracking with configurable warning thresholds:

```cpp
// Set warning threshold to 85% of available memory
allocator.SetMemoryUsageWarningThreshold(0.85f);

// Get current memory budget information
auto budget = allocator.GetMemoryBudget();
if (budget.isOverBudget) {
    // Handle over-budget situation
}
```

---

## Usage Examples

### Basic Buffer Allocation

```cpp
// Initialize the memory allocator system
MemoryAllocator::Init(vulkanDevice, VK_API_VERSION_1_3);

// Create allocator instance with identifying tag
auto allocator = CreateRef<MemoryAllocator>("Mesh Renderer");

// Set up buffer creation info
VkBufferCreateInfo bufferInfo{};
bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
bufferInfo.size = 1024 * 1024; // 1MB vertex buffer
bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

// Allocate the buffer
VkBuffer buffer;
VmaAllocation allocation = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY, buffer);

// Use the buffer...

// Clean up
allocator->DestroyBuffer(buffer, allocation);
```

### Image Allocation

```cpp
// Set up image creation info
VkImageCreateInfo imageInfo{};
imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
imageInfo.imageType = VK_IMAGE_TYPE_2D;
imageInfo.extent.width = 1024;
imageInfo.extent.height = 1024;
imageInfo.extent.depth = 1;
imageInfo.mipLevels = 1;
imageInfo.arrayLayers = 1;
imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
imageInfo.usage = VK_IMAGE_USAGE_TEXTURE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

// Allocate the image
VkImage image;
VkDeviceSize allocatedSize;
VmaAllocation allocation = allocator->AllocateImage(imageInfo, VMA_MEMORY_USAGE_GPU_ONLY, image, &allocatedSize);

SEDX_CORE_INFO("Allocated texture: {} MB", allocatedSize / (1024 * 1024));

// Clean up
allocator->DestroyImage(image, allocation);
```

### Batch Buffer Allocation

```cpp
// Prepare sizes for multiple buffers
std::vector<VkDeviceSize> bufferSizes = {
    64 * 1024,    // 64KB index buffer
    256 * 1024,   // 256KB vertex buffer
    16 * 1024,    // 16KB uniform buffer
    128 * 1024    // 128KB storage buffer
};

// Allocate buffers in batch for better performance
auto batchAllocations = allocator->AllocateBufferBatch(
    bufferSizes, 
    BufferUsage::Vertex | BufferUsage::Index | BufferUsage::Uniform | BufferUsage::Storage,
    VMA_MEMORY_USAGE_GPU_ONLY
);

// Process allocated buffers
for (const auto& allocation : batchAllocations) {
    SEDX_CORE_INFO("Allocated buffer: {} bytes", allocation.size);
    // Use allocation.buffer and allocation.allocation
}

// Clean up batch
allocator->FreeBufferBatch(batchAllocations);
```

### Memory Mapping for CPU Access

```cpp
// Allocate staging buffer for CPU-GPU data transfer
VkBufferCreateInfo stagingInfo{};
stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
stagingInfo.size = dataSize;
stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
stagingInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

VkBuffer stagingBuffer;
VmaAllocation stagingAllocation = allocator->AllocateBuffer(
    stagingInfo, 
    VMA_MEMORY_USAGE_CPU_TO_GPU, 
    stagingBuffer
);

// Map memory for CPU access
uint8_t* mappedData = allocator->MapMemory<uint8_t>(stagingAllocation);
memcpy(mappedData, sourceData, dataSize);
MemoryAllocator::UnmapMemory(stagingAllocation);

// Transfer to GPU buffer...
```

### Advanced Configuration

```cpp
// Configure memory allocator for specific workload
allocator->SetAllocationStrategy(MemoryAllocator::AllocationStrategy::SpeedOptimized);
allocator->SetMemoryUsageWarningThreshold(0.90f); // 90% warning threshold
allocator->SetBufferAlignment(256); // 256-byte alignment for cache efficiency

// Set custom buffer size for large allocations
if (MemoryAllocator::SetCustomBufferSize(32 * 1024 * 1024, *vulkanDevice)) {
    SEDX_CORE_INFO("Custom buffer size set to 32MB");
}

// Enable automated defragmentation
allocator->BeginDefragmentation(VMA_DEFRAGMENTATION_FLAG_ALGORITHM_BALANCED_BIT);

// Mark frequently used allocations for defragmentation
for (auto& meshAllocation : sceneMeshAllocations) {
    allocator->MarkForDefragmentation(meshAllocation);
}

// Execute defragmentation during a quiet period
allocator->EndDefragmentation();
```

---

## Memory Statistics and Monitoring

### Getting Allocation Statistics

```cpp
// Get current allocation statistics
auto stats = allocator->GetStats();

SEDX_CORE_INFO("Memory Statistics:");
SEDX_CORE_INFO("  Total Bytes: {} MB", stats.totalBytes / (1024 * 1024));
SEDX_CORE_INFO("  Used Bytes: {} MB", stats.usedBytes / (1024 * 1024));
SEDX_CORE_INFO("  Allocation Count: {}", stats.allocationCount);
SEDX_CORE_INFO("  Fragmentation: {:.2f}%", stats.fragmentationRatio * 100.0f);
```

### Detailed Statistics Reporting

```cpp
// Print comprehensive memory statistics
allocator->PrintDetailedStats();

// Output includes:
// - Total memory allocated/used
// - Per-heap memory usage
// - Per-memory-type allocation details
// - Fragmentation ratios
// - Peak memory usage
```

### Memory Budget Monitoring

```cpp
// Monitor memory budget in real-time
auto budget = allocator->GetMemoryBudget();

if (budget.isOverBudget) {
    SEDX_CORE_WARN("Memory usage: {:.1f}% ({} MB / {} MB)", 
                   budget.usagePercentage * 100.0f,
                   budget.usedBytes / (1024 * 1024),
                   budget.totalBytes / (1024 * 1024));
  
    // Trigger cleanup or LOD reduction
    TriggerMemoryCleanup();
}
```

---

## Integration with Scenery Editor X Framework

### Module Integration

The memory allocator integrates seamlessly with the Module system:

```cpp
class RenderModule : public Module
{
public:
    void OnAttach() override
    {
        SEDX_CORE_INFO("=== Initializing {} ===", GetName());
    
        // Initialize memory allocator for this module
        m_Allocator = CreateRef<MemoryAllocator>(GetName());
        m_Allocator->SetAllocationStrategy(MemoryAllocator::AllocationStrategy::SpeedOptimized);
    
        // Load resources using the allocator
        InitializeRenderResources();
    }
  
    void OnDetach() override
    {
        SEDX_CORE_INFO("=== Cleaning up {} ===", GetName());
    
        // Resources are automatically cleaned up by smart pointers
        // But we can print final statistics
        m_Allocator->PrintDetailedStats();
        m_Allocator.Reset();
    }
  
    void OnUpdate() override
    {
        SEDX_PROFILE_SCOPE("RenderModule::OnUpdate");
    
        // Check memory budget periodically
        static int frameCount = 0;
        if (++frameCount % 60 == 0) { // Check every 60 frames
            auto budget = m_Allocator->GetMemoryBudget();
            if (budget.isOverBudget) {
                SEDX_CORE_WARN_TAG("RENDER", "Memory budget exceeded: {:.1f}%", 
                                   budget.usagePercentage * 100.0f);
            }
        }
    }
  
private:
    Ref<MemoryAllocator> m_Allocator;
};
```

### Smart Pointer Integration

```cpp
// Use with custom smart pointer system
class TextureManager : public RefCounted
{
public:
    Ref<Texture2D> LoadTexture(const std::string& path)
    {
        auto texture = CreateRef<Texture2D>();
    
        // Allocate GPU memory using the allocator
        VkImageCreateInfo imageInfo = texture->GetImageCreateInfo();
        VkImage image;
        VkDeviceSize allocatedSize;
    
        VmaAllocation allocation = m_Allocator->AllocateImage(
            imageInfo, 
            VMA_MEMORY_USAGE_GPU_ONLY, 
            image, 
            &allocatedSize
        );
    
        texture->SetImage(image, allocation);
    
        SEDX_CORE_INFO_TAG("TEXTURE", "Loaded texture '{}': {} MB", 
                           path, allocatedSize / (1024 * 1024));
    
        return texture;
    }
  
private:
    Ref<MemoryAllocator> m_Allocator = CreateRef<MemoryAllocator>("TextureManager");
};
```

---

## Error Handling and Debugging

### Assertion and Validation

```cpp
// The allocator includes comprehensive validation
VmaAllocation allocation = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY, buffer);

// Validate allocation success
SEDX_CORE_ASSERT(allocation != nullptr, "Failed to allocate buffer memory");
SEDX_CORE_VERIFY(allocator->ContainsAllocation(allocation), "Allocation not tracked");
```

### Tagged Logging

```cpp
// All allocator operations use tagged logging for easy filtering
SEDX_CORE_INFO_TAG("VulkanAllocator", "Allocated buffer: {} bytes", bufferSize);
SEDX_CORE_WARN_TAG("VulkanAllocator", "Memory usage warning: {}% of budget used", usagePercent);
SEDX_CORE_ERROR_TAG("VulkanAllocator", "Failed to allocate memory: {}", errorCode);
```

### Debug Information

```cpp
// Enable detailed debug information
#ifdef SEDX_DEBUG
    allocator->SetMemoryUsageWarningThreshold(0.8f); // Lower threshold in debug
  
    // Print statistics more frequently in debug builds
    static int debugFrameCount = 0;
    if (++debugFrameCount % 300 == 0) { // Every 5 seconds at 60 FPS
        allocator->PrintDetailedStats();
    }
#endif
```

---

## Performance Optimization

### Memory Pool Optimization

```cpp
// Pre-warm memory pools for expected allocation patterns
std::vector<VkDeviceSize> commonSizes = {
    SMALL_BUFFER_SIZE,   // For uniform buffers
    MEDIUM_BUFFER_SIZE,  // For vertex/index buffers
    LARGE_BUFFER_SIZE    // For large assets
};

// Allocate and immediately free to create pools
for (auto size : commonSizes) {
    VkBufferCreateInfo warmupInfo{};
    warmupInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    warmupInfo.size = size;
    warmupInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    warmupInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  
    VkBuffer warmupBuffer;
    VmaAllocation warmupAllocation = allocator->AllocateBuffer(
        warmupInfo, VMA_MEMORY_USAGE_GPU_ONLY, warmupBuffer);
  
    allocator->DestroyBuffer(warmupBuffer, warmupAllocation);
}
```

### Defragmentation Strategy

```cpp
// Implement smart defragmentation timing
class DefragmentationManager
{
public:
    void Update()
    {
        // Only defragment during low activity periods
        if (ShouldDefragment()) {
            SEDX_CORE_INFO("Starting memory defragmentation");
        
            m_Allocator->BeginDefragmentation();
        
            // Mark old/unused allocations for defragmentation
            MarkCandidatesForDefragmentation();
        
            m_Allocator->EndDefragmentation();
        
            m_LastDefragTime = std::chrono::steady_clock::now();
        }
    }
  
private:
    bool ShouldDefragment() const
    {
        auto stats = m_Allocator->GetStats();
        auto now = std::chrono::steady_clock::now();
        auto timeSinceLastDefrag = std::chrono::duration_cast<std::chrono::minutes>(
            now - m_LastDefragTime).count();
    
        // Defragment if fragmentation is high and enough time has passed
        return stats.fragmentationRatio > 0.3f && timeSinceLastDefrag > 5;
    }
  
    Ref<MemoryAllocator> m_Allocator;
    std::chrono::steady_clock::time_point m_LastDefragTime;
};
```

---

## Best Practices

### 1. Initialization and Shutdown

```cpp
// Application startup
void Application::OnStartup()
{
    // Initialize VMA after Vulkan device creation
    MemoryAllocator::Init(m_VulkanDevice, VK_API_VERSION_1_3);
  
    // Configure global settings
    auto globalAllocator = CreateRef<MemoryAllocator>("Global");
    globalAllocator->SetMemoryUsageWarningThreshold(0.85f);
    globalAllocator->SetAllocationStrategy(MemoryAllocator::AllocationStrategy::Default);
}

// Application shutdown
void Application::OnShutdown()
{
    // Clean up all allocators before VMA shutdown
    m_ModuleAllocators.clear();
  
    // Shutdown VMA system
    MemoryAllocator::Shutdown();
}
```

### 2. Resource Lifecycle Management

```cpp
class SceneRenderer : public Module
{
private:
    struct MeshData
    {
        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VmaAllocation vertexAllocation = nullptr;
        VkBuffer indexBuffer = VK_NULL_HANDLE;
        VmaAllocation indexAllocation = nullptr;
    
        ~MeshData()
        {
            // RAII cleanup
            if (vertexBuffer != VK_NULL_HANDLE) {
                allocator->DestroyBuffer(vertexBuffer, vertexAllocation);
            }
            if (indexBuffer != VK_NULL_HANDLE) {
                allocator->DestroyBuffer(indexBuffer, indexAllocation);
            }
        }
    };
  
    std::vector<std::unique_ptr<MeshData>> m_SceneMeshes;
};
```

### 3. Error Recovery

```cpp
// Implement robust error handling
bool SafeAllocateBuffer(VkBufferCreateInfo& bufferInfo, VkBuffer& outBuffer, VmaAllocation& outAllocation)
{
    // Try with requested memory usage first
    outAllocation = m_Allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY, outBuffer);
  
    if (outAllocation == nullptr) {
        SEDX_CORE_WARN("GPU memory allocation failed, trying CPU memory");
    
        // Fallback to CPU memory if GPU memory is exhausted
        outAllocation = m_Allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, outBuffer);
    
        if (outAllocation == nullptr) {
            SEDX_CORE_ERROR("All memory allocation attempts failed");
            return false;
        }
    }
  
    return true;
}
```

---

## Thread Safety

The memory allocator is fully thread-safe and can be used from multiple threads simultaneously:

```cpp
// Safe to call from multiple threads
std::vector<std::thread> allocationThreads;

for (int i = 0; i < std::thread::hardware_concurrency(); ++i) {
    allocationThreads.emplace_back([&allocator]() {
        // Each thread can safely allocate memory
        VkBufferCreateInfo bufferInfo{};
        // ... setup buffer info ...
    
        VkBuffer buffer;
        VmaAllocation allocation = allocator->AllocateBuffer(
            bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY, buffer);
    
        // ... use buffer ...
    
        allocator->DestroyBuffer(buffer, allocation);
    });
}

for (auto& thread : allocationThreads) {
    thread.join();
}
```

# Scenery Editor X - Memory System - Inline Code Examples and Use Cases

## Quick Reference Examples

This document provides immediate, copy-paste examples for common memory management scenarios in SceneryEditorX.

### Basic Allocation Patterns

```cpp
// Basic tracked allocation with automatic source location
MyClass* obj = hnew MyClass(arg1, arg2);
uint8_t* buffer = hnew uint8_t[1024];

// Corresponding deallocation  
hdelete obj;
hdelete[] buffer;

// Category-based allocation for subsystem tracking
Texture* tex = new("Renderer::Textures") Texture(width, height);
AudioBuffer* audio = new("Audio::Buffers") AudioBuffer(sampleCount);
PhysicsBody* body = new("Physics::Bodies") RigidBody(mass);

hdelete tex;    // Category is preserved in tracking
hdelete audio;
hdelete body;
```

### Application Initialization Template

```cpp
#include <SceneryEditorX/core/memory.h>

int main() {
    // REQUIRED: Initialize memory tracking system
    SceneryEditorX::Allocator::Init();
  
    try {
        // Your application code
        Application app;
        int result = app.Run();
      
        // Optional: Debug memory leak checking
        #ifndef SEDX_DIST
        const auto& stats = SceneryEditorX::Memory::GetAllocationStats();
        size_t leaked = stats.TotalAllocated - stats.TotalFreed;
        if (leaked > 0) {
            SEDX_CORE_WARN("Memory leak detected: {} bytes", leaked);
        }
        #endif
      
        return result;
    }
    catch (const std::exception& e) {
        SEDX_CORE_ERROR("Application error: {}", e.what());
        return -1;
    }
}
```

### RAII Resource Management Template

```cpp
// Template for memory-managed resources
template<typename T>
class TrackedResource {
private:
    T* resource_;
    const char* category_;
  
public:
    template<typename... Args>
    TrackedResource(const char* category, Args&&... args) 
        : category_(category), resource_(new(category) T(std::forward<Args>(args)...)) {
    }
  
    ~TrackedResource() {
        if (resource_) {
            hdelete resource_;
        }
    }
  
    // Move-only semantics (prevent accidental copies)
    TrackedResource(const TrackedResource&) = delete;
    TrackedResource& operator=(const TrackedResource&) = delete;
  
    TrackedResource(TrackedResource&& other) noexcept 
        : resource_(other.resource_), category_(other.category_) {
        other.resource_ = nullptr;
    }
  
    TrackedResource& operator=(TrackedResource&& other) noexcept {
        if (this != &other) {
            if (resource_) hdelete resource_;
            resource_ = other.resource_;
            category_ = other.category_;
            other.resource_ = nullptr;
        }
        return *this;
    }
  
    T* Get() const { return resource_; }
    T& operator*() const { return *resource_; }
    T* operator->() const { return resource_; }
    explicit operator bool() const { return resource_ != nullptr; }
};

// Usage example
TrackedResource<Texture2D> texture("Renderer::Textures", 1024, 1024);
TrackedResource<VertexBuffer> vertices("Renderer::Buffers::Vertex", vertexCount);
```

### Smart Pointer Integration

```cpp
// Custom deleter for tracked allocations
template<typename T>
struct TrackedDeleter {
    void operator()(T* ptr) {
        hdelete ptr;
    }
};

// Type aliases for convenience
template<typename T>
using TrackedUniquePtr = std::unique_ptr<T, TrackedDeleter<T>>;

template<typename T>
using TrackedSharedPtr = std::shared_ptr<T>;

// Factory functions
template<typename T, typename... Args>
TrackedUniquePtr<T> MakeTrackedUnique(const char* category, Args&&... args) {
    T* ptr = new(category) T(std::forward<Args>(args)...);
    return TrackedUniquePtr<T>(ptr);
}

template<typename T, typename... Args>
TrackedSharedPtr<T> MakeTrackedShared(const char* category, Args&&... args) {
    return TrackedSharedPtr<T>(new(category) T(std::forward<Args>(args)...), TrackedDeleter<T>{});
}

// Usage examples
auto texture = MakeTrackedUnique<Texture2D>("Renderer::Textures", width, height);
auto sharedBuffer = MakeTrackedShared<DataBuffer>("Processing::Buffers", size);
```

### Memory Statistics and Debugging

```cpp
// Memory usage reporting utility
class MemoryReporter {
public:
    // Report total memory usage
    static void ReportGlobalUsage() {
        const auto& stats = SceneryEditorX::Memory::GetAllocationStats();
        size_t currentUsage = stats.TotalAllocated - stats.TotalFreed;
      
        SEDX_CORE_INFO("Global Memory Usage:");
        SEDX_CORE_INFO("  Allocated: {} bytes", stats.TotalAllocated);
        SEDX_CORE_INFO("  Freed: {} bytes", stats.TotalFreed);
        SEDX_CORE_INFO("  Current: {} bytes", currentUsage);
    }
  
    // Report memory usage by category/subsystem
    static void ReportCategoryUsage(const std::string& prefix = "") {
        const auto& stats = SceneryEditorX::Allocator::GetAllocationStats();
      
        SEDX_CORE_INFO("Memory Usage by Category{}:", 
                       prefix.empty() ? "" : (" (filter: " + prefix + ")"));
      
        size_t totalFiltered = 0;
        for (const auto& [category, allocation] : stats) {
            if (prefix.empty() || std::string(category).find(prefix) == 0) {
                size_t current = allocation.TotalAllocated - allocation.TotalFreed;
                if (current > 0) {
                    SEDX_CORE_INFO("  {}: {} bytes", category, current);
                    totalFiltered += current;
                }
            }
        }
      
        if (!prefix.empty()) {
            SEDX_CORE_INFO("Total for '{}': {} bytes", prefix, totalFiltered);
        }
    }
  
    // Check for memory leaks in specific categories
    static bool CheckForLeaks(const std::string& categoryPrefix = "") {
        const auto& stats = SceneryEditorX::Allocator::GetAllocationStats();
        bool leaksFound = false;
      
        for (const auto& [category, allocation] : stats) {
            if (categoryPrefix.empty() || 
                std::string(category).find(categoryPrefix) == 0) {
              
                size_t leaked = allocation.TotalAllocated - allocation.TotalFreed;
                if (leaked > 0) {
                    SEDX_CORE_WARN("Memory leak in '{}': {} bytes", category, leaked);
                    leaksFound = true;
                }
            }
        }
      
        return leaksFound;
    }
};

// Usage in application code
void CheckRendererMemory() {
    MemoryReporter::ReportCategoryUsage("Renderer::");
  
    if (MemoryReporter::CheckForLeaks("Renderer::")) {
        SEDX_CORE_ERROR("Renderer has memory leaks!");
    }
}
```

### Component-Based Memory Management

```cpp
// Base class for components that need memory tracking
class MemoryTrackedComponent {
protected:
    const char* componentCategory_;
  
    template<typename T, typename... Args>
    T* AllocateComponentMemory(Args&&... args) {
        return new(componentCategory_) T(std::forward<Args>(args)...);
    }
  
    template<typename T>
    void FreeComponentMemory(T* ptr) {
        hdelete ptr;
    }
  
public:
    explicit MemoryTrackedComponent(const char* category) 
        : componentCategory_(category) {}
      
    virtual ~MemoryTrackedComponent() = default;
  
    // Get memory usage for this component category
    size_t GetCategoryMemoryUsage() const {
        const auto& stats = SceneryEditorX::Allocator::GetAllocationStats();
        auto it = stats.find(componentCategory_);
        if (it != stats.end()) {
            return it->second.TotalAllocated - it->second.TotalFreed;
        }
        return 0;
    }
};

// Example renderer component
class VulkanRenderer : public MemoryTrackedComponent {
private:
    VkBuffer* vertexBuffers_;
    VkImage* textures_;
    uint32_t bufferCount_;
    uint32_t textureCount_;
  
public:
    VulkanRenderer() : MemoryTrackedComponent("Renderer::Vulkan") {
        // Allocate renderer resources with category tracking
        vertexBuffers_ = AllocateComponentMemory<VkBuffer[]>(MAX_VERTEX_BUFFERS);
        textures_ = AllocateComponentMemory<VkImage[]>(MAX_TEXTURES);
      
        SEDX_CORE_INFO("VulkanRenderer initialized with category tracking");
    }
  
    ~VulkanRenderer() {
        // Cleanup tracked resources
        FreeComponentMemory(vertexBuffers_);
        FreeComponentMemory(textures_);
      
        // Report any remaining memory in this category
        size_t remaining = GetCategoryMemoryUsage();
        if (remaining > 0) {
            SEDX_CORE_WARN("VulkanRenderer destroyed with {} bytes still allocated", remaining);
        }
    }
  
    void CreateTexture(uint32_t width, uint32_t height) {
        // Use component category for texture allocation
        auto* textureData = AllocateComponentMemory<uint8_t[]>(width * height * 4);
        // ... texture creation logic
    }
};
```

### Memory Pool Implementation

```cpp
// Memory pool with category tracking
class TrackedMemoryPool {
private:
    void* poolMemory_;
    size_t poolSize_;
    size_t currentOffset_;
    const char* poolCategory_;
    std::vector<size_t> allocatedSizes_; // Track individual allocations
  
public:
    TrackedMemoryPool(size_t poolSize, const char* category) 
        : poolSize_(poolSize), currentOffset_(0), poolCategory_(category) {
      
        // Single large allocation for the entire pool
        poolMemory_ = new(poolCategory_) uint8_t[poolSize_];
      
        SEDX_CORE_INFO("Memory pool '{}' created with {} bytes", category, poolSize);
    }
  
    ~TrackedMemoryPool() {
        hdelete[] static_cast<uint8_t*>(poolMemory_);
      
        SEDX_CORE_INFO("Memory pool '{}' destroyed. {} allocations were made.", 
                       poolCategory_, allocatedSizes_.size());
    }
  
    void* Allocate(size_t size) {
        // Align to 8-byte boundary
        size_t alignedSize = (size + 7) & ~7;
      
        if (currentOffset_ + alignedSize > poolSize_) {
            SEDX_CORE_ERROR("Pool '{}' allocation failed: {} bytes requested, {} available", 
                           poolCategory_, alignedSize, poolSize_ - currentOffset_);
            return nullptr;
        }
      
        void* result = static_cast<uint8_t*>(poolMemory_) + currentOffset_;
        currentOffset_ += alignedSize;
        allocatedSizes_.push_back(alignedSize);
      
        SEDX_CORE_TRACE("Pool '{}' allocated {} bytes (total used: {})", 
                        poolCategory_, alignedSize, currentOffset_);
      
        return result;
    }
  
    void Reset() {
        currentOffset_ = 0;
        allocatedSizes_.clear();
        SEDX_CORE_INFO("Memory pool '{}' reset", poolCategory_);
    }
  
    size_t GetUsedBytes() const { return currentOffset_; }
    size_t GetAvailableBytes() const { return poolSize_ - currentOffset_; }
    float GetUsagePercentage() const { 
        return static_cast<float>(currentOffset_) / static_cast<float>(poolSize_) * 100.0f; 
    }
};

// Usage example
void ExamplePoolUsage() {
    TrackedMemoryPool tempPool(1024 * 1024, "Memory::Pools::Temporary"); // 1MB pool
  
    // Allocate from pool
    void* buffer1 = tempPool.Allocate(1024);
    void* buffer2 = tempPool.Allocate(2048);
  
    SEDX_CORE_INFO("Pool usage: {:.1f}%", tempPool.GetUsagePercentage());
  
    // Reset pool when done with temporary allocations
    tempPool.Reset();
}
```

### Error Handling and Exception Safety

```cpp
// Exception-safe allocation wrapper
template<typename T>
class SafeTrackedAllocation {
private:
    T* ptr_;
    const char* category_;
  
public:
    template<typename... Args>
    SafeTrackedAllocation(const char* category, Args&&... args) 
        : ptr_(nullptr), category_(category) {
      
        try {
            ptr_ = new(category) T(std::forward<Args>(args)...);
        }
        catch (const std::bad_alloc& e) {
            SEDX_CORE_ERROR("Memory allocation failed for category '{}': {}", category, e.what());
          
            // Report current memory state to help debug
            const auto& stats = SceneryEditorX::Memory::GetAllocationStats();
            size_t currentUsage = stats.TotalAllocated - stats.TotalFreed;
            SEDX_CORE_ERROR("Current total memory usage: {} bytes", currentUsage);
          
            throw; // Re-throw for caller to handle
        }
        catch (...) {
            SEDX_CORE_ERROR("Unknown error during allocation for category '{}'", category);
            throw;
        }
    }
  
    ~SafeTrackedAllocation() {
        if (ptr_) {
            hdelete ptr_;
        }
    }
  
    // Move semantics only
    SafeTrackedAllocation(const SafeTrackedAllocation&) = delete;
    SafeTrackedAllocation& operator=(const SafeTrackedAllocation&) = delete;
  
    SafeTrackedAllocation(SafeTrackedAllocation&& other) noexcept 
        : ptr_(other.ptr_), category_(other.category_) {
        other.ptr_ = nullptr;
    }
  
    SafeTrackedAllocation& operator=(SafeTrackedAllocation&& other) noexcept {
        if (this != &other) {
            if (ptr_) hdelete ptr_;
            ptr_ = other.ptr_;
            category_ = other.category_;
            other.ptr_ = nullptr;
        }
        return *this;
    }
  
    T* Get() const { return ptr_; }
    T& operator*() const { return *ptr_; }
    T* operator->() const { return ptr_; }
    explicit operator bool() const { return ptr_ != nullptr; }
  
    // Release ownership (caller becomes responsible for deletion)
    T* Release() {
        T* temp = ptr_;
        ptr_ = nullptr;
        return temp;
    }
};

// Usage with exception handling
void ExampleExceptionSafeUsage() {
    try {
        SafeTrackedAllocation<LargeObject> obj("Examples::LargeObjects", constructorArgs);
      
        // Use obj->Method() or (*obj).Method()
        obj->ProcessData();
      
        // Automatic cleanup when obj goes out of scope
    }
    catch (const std::bad_alloc& e) {
        SEDX_CORE_ERROR("Failed to create LargeObject: insufficient memory");
        // Handle allocation failure appropriately
    }
}
```

### Conditional Memory Tracking (Debug vs Release)

```cpp
// Conditional allocation based on build configuration
template<typename T, typename... Args>
T* ConditionalTrackedNew(const char* category, Args&&... args) {
    #ifdef SEDX_TRACK_MEMORY
        return new(category) T(std::forward<Args>(args)...);
    #else
        return new T(std::forward<Args>(args)...);
    #endif
}

template<typename T>
void ConditionalTrackedDelete(T* ptr) {
    #ifdef SEDX_TRACK_MEMORY
        hdelete ptr;
    #else
        delete ptr;
    #endif
}

// Macro for easy conditional tracking
#ifdef SEDX_TRACK_MEMORY
    #define TRACKED_NEW(category, type, ...) new(category) type(__VA_ARGS__)
    #define TRACKED_DELETE(ptr) hdelete ptr
#else
    #define TRACKED_NEW(category, type, ...) new type(__VA_ARGS__)
    #define TRACKED_DELETE(ptr) delete ptr
#endif

// Usage example
void ExampleConditionalUsage() {
    // This will use tracking in debug builds, standard allocation in release
    auto* obj = TRACKED_NEW("Examples::Objects", MyClass, arg1, arg2);
  
    // Process with obj...
  
    TRACKED_DELETE(obj);
}
```

### Integration with Existing C++ Patterns

```cpp
// Integration with std::vector using custom allocator
template<typename T>
using TrackedVector = std::vector<T>; // Uses overridden global operators

// Integration with std::map
template<typename K, typename V>
using TrackedMap = std::map<K, V>; // Uses overridden global operators

// Factory pattern with memory tracking
template<typename Base>
class TrackedFactory {
private:
    const char* factoryCategory_;
  
public:
    explicit TrackedFactory(const char* category) : factoryCategory_(category) {}
  
    template<typename Derived, typename... Args>
    Base* Create(Args&&... args) {
        static_assert(std::is_base_of_v<Base, Derived>, "Derived must inherit from Base");
      
        std::string fullCategory = std::string(factoryCategory_) + "::" + typeid(Derived).name();
        return new(fullCategory.c_str()) Derived(std::forward<Args>(args)...);
    }
  
    void Destroy(Base* obj) {
        hdelete obj;
    }
};

// Usage example
void ExampleFactoryUsage() {
    TrackedFactory<GameObject> gameObjectFactory("Game::Objects");
  
    auto* player = gameObjectFactory.Create<Player>("PlayerName");
    auto* enemy = gameObjectFactory.Create<Enemy>(enemyType);
  
    // Objects are tracked under "Game::Objects::Player" and "Game::Objects::Enemy"
  
    gameObjectFactory.Destroy(player);
    gameObjectFactory.Destroy(enemy);
}
```

### Performance Monitoring Integration

```cpp
// Performance monitoring with memory tracking
class PerformanceMonitor {
private:
    std::chrono::high_resolution_clock::time_point startTime_;
    SceneryEditorX::AllocationStats initialStats_;
    const char* operationName_;
  
public:
    explicit PerformanceMonitor(const char* operationName) 
        : operationName_(operationName), startTime_(std::chrono::high_resolution_clock::now()) {
        initialStats_ = SceneryEditorX::Memory::GetAllocationStats();
    }
  
    ~PerformanceMonitor() {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime_);
      
        const auto& finalStats = SceneryEditorX::Memory::GetAllocationStats();
      
        size_t allocatedDuring = finalStats.TotalAllocated - initialStats_.TotalAllocated;
        size_t freedDuring = finalStats.TotalFreed - initialStats_.TotalFreed;
        size_t netChange = allocatedDuring - freedDuring;
      
        SEDX_CORE_INFO("Performance Report for '{}':", operationName_);
        SEDX_CORE_INFO("  Duration: {} μs", duration.count());
        SEDX_CORE_INFO("  Memory allocated: {} bytes", allocatedDuring);
        SEDX_CORE_INFO("  Memory freed: {} bytes", freedDuring);
        SEDX_CORE_INFO("  Net memory change: {} bytes", netChange);
    }
};

// Usage with RAII
void ExamplePerformanceMonitoring() {
    {
        PerformanceMonitor monitor("Complex Algorithm");
      
        // Perform memory-intensive operations
        auto* data = hnew uint8_t[1024 * 1024]; // 1MB allocation
        ProcessData(data);
        hdelete[] data;
      
        // Monitor reports performance and memory usage automatically
    }
}

// Macro for easy performance monitoring
#define MONITOR_PERFORMANCE(name) PerformanceMonitor _monitor(name)

void ExampleMacroUsage() {
    MONITOR_PERFORMANCE("Texture Loading");
  
    // Load and process textures...
    auto* texture = hnew TextureData(width, height);
    // ... processing
    hdelete texture;
  
    // Performance report generated automatically at scope exit
}
```

This collection of examples and patterns provides comprehensive guidance for integrating with the SceneryEditorX memory management system. Use these patterns as templates for new code development and as references when working with the existing codebase.

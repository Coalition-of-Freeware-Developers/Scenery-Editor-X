# Scenery Editor X - Memory Management System Documentation

---

## Overview

The SceneryEditorX Memory Management System provides comprehensive memory allocation tracking, statistics collection, and debugging capabilities for the Scenery Editor X application. This system is designed to help developers monitor memory usage, detect memory leaks, and optimize memory allocation patterns throughout the application lifecycle.

## Table of Contents

1. [Core Components](#core-components)
2. [Architecture](#architecture)
3. [Memory Tracking Features](#memory-tracking-features)
4. [Usage Examples](#usage-examples)
5. [Best Practices](#best-practices)
6. [Integration Guidelines](#integration-guidelines)
7. [Performance Considerations](#performance-considerations)
8. [Debugging and Profiling](#debugging-and-profiling)

## Core Components

### 1. Allocator Class (`memory.h` / `memory.cpp`)

The `Allocator` class is the central component of the memory management system. It provides static methods for memory allocation and tracking.

**Key Features:**

- Thread-safe memory allocation and deallocation
- Category-based allocation tracking
- Source location capture for debugging
- Integration with Tracy profiler
- Comprehensive statistics collection

**Core Methods:**

```cpp
// Initialize the memory system (call once at application startup)
static void Init();

// Raw allocation without tracking (internal use)
static void* AllocateRaw(size_t size);

// Standard tracked allocation
static void* Allocate(size_t size);

// Allocation with category descriptor
static void* Allocate(size_t size, const char* desc);

// Allocation with source location information
static void* Allocate(size_t size, const char* file, int line);

// Deallocate tracked memory
static void Free(void* memory);

// Get allocation statistics by category
static const AllocatorData::AllocationStatsMap& GetAllocationStats();
```

### 2. Data Structures

#### AllocationStats

Tracks cumulative memory allocation statistics:

```cpp
struct AllocationStats {
    size_t TotalAllocated = 0;  // Total bytes allocated
    size_t TotalFreed = 0;      // Total bytes freed
};
```

#### Allocation

Represents a single tracked memory allocation:

```cpp
struct Allocation {
    void* Memory = nullptr;        // Pointer to allocated memory
    size_t Size = 0;              // Size in bytes
    const char* Category = nullptr; // Category/descriptor string
};
```

#### AllocatorData

Core data structure managing all allocation tracking:

```cpp
struct AllocatorData {
    std::map<const void*, Allocation> AllocationMap;        // Active allocations
    AllocationStatsMap AllocStatsMap;                       // Statistics by category
    std::mutex Mutex_;                                      // Thread safety for allocations
    std::mutex MutexStats_;                                 // Thread safety for statistics
};
```

### 3. Custom Allocator (Mallocator)

The `Mallocator` template provides STL-compatible allocation using malloc/free, specifically designed to avoid recursive allocation issues in the tracking system itself.

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    Application Layer                             │
├─────────────────────────────────────────────────────────────────┤
│  new/delete    │  new("category")  │  operator new/delete     │
├─────────────────────────────────────────────────────────────────┤
│                    Memory Tracking Layer                         │
│  ┌─────────────┐  ┌─────────────────┐  ┌─────────────────────┐ │
│  │  Allocator  │  │ AllocationStats │  │ AllocationData      │ │
│  │   Methods   │  │   Management    │  │   Storage           │ │
│  └─────────────┘  └─────────────────┘  └─────────────────────┘ │
├─────────────────────────────────────────────────────────────────┤
│                    System Layer                                  │
│              malloc/free + tracking metadata                     │
└─────────────────────────────────────────────────────────────────┘
```

## Memory Tracking Features

### 1. Automatic Memory Tracking

When `SEDX_TRACK_MEMORY` is enabled, all dynamic allocations are automatically tracked:

```cpp
// These are automatically tracked when memory tracking is enabled
MyClass* obj = new MyClass();        // Tracked with source location
int* array = new int[100];           // Tracked array allocation
delete obj;                          // Tracked deallocation
delete[] array;                      // Tracked array deallocation
```

### 2. Category-Based Tracking

Allocations can be categorized for detailed analysis:

```cpp
// Allocate with category for granular tracking
void* buffer = new("Renderer::Textures") char[textureSize];
void* vertexData = new("Renderer::Geometry") Vertex[vertexCount];

// Later analysis can show memory usage by category
auto stats = Allocator::GetAllocationStats();
for (const auto& [category, allocation] : stats) {
    size_t currentUsage = allocation.TotalAllocated - allocation.TotalFreed;
    SEDX_CORE_INFO("Category: {} - Current Usage: {} bytes", category, currentUsage);
}
```

### 3. Source Location Tracking

The `new` macro automatically captures file and line information:

```cpp
// Automatically captures __FILE__ and __LINE__
MyClass* obj = new MyClass();
int* data = new int[1000];

// This helps identify exactly where allocations occur for leak detection
```

### 4. Statistics Collection

Comprehensive statistics are maintained both globally and per-category:

```cpp
// Get global memory statistics
const auto& globalStats = Memory::GetAllocationStats();
size_t totalMemoryInUse = globalStats.TotalAllocated - globalStats.TotalFreed;

// Get category-specific statistics
const auto& categoryStats = Allocator::GetAllocationStats();
for (const auto& [category, stats] : categoryStats) {
    size_t categoryUsage = stats.TotalAllocated - stats.TotalFreed;
    // Analyze per-category usage
}
```

## Usage Examples

### 1. Basic Application Integration

```cpp
// At application startup
int main() {
    // Initialize the memory tracking system
    SceneryEditorX::Allocator::Init();
  
    // Your application code here
    RunApplication();
  
    // Optional: Check for memory leaks before exit
    const auto& stats = SceneryEditorX::Memory::GetAllocationStats();
    size_t leakedBytes = stats.TotalAllocated - stats.TotalFreed;
    if (leakedBytes > 0) {
        SEDX_CORE_WARN("Memory leak detected: {} bytes", leakedBytes);
    }
  
    return 0;
}
```

### 2. Renderer Memory Management

```cpp
class VulkanRenderer {
private:
    void* CreateBuffer(size_t size, const char* usage) {
        // Use category-based allocation for renderer resources
        return new(std::format("Renderer::{}", usage).c_str()) char[size];
    }
  
    void CreateTexture() {
        // Specific categorization for different resource types
        textureData = new uint8_t[textureSize];  // Captures source location
        // Allocation is automatically tracked with file:line information
    }
  
public:
    void Initialize() {
        // Allocate various renderer resources with categories
        vertexBuffer = CreateBuffer(vertexBufferSize, "VertexBuffers");
        indexBuffer = CreateBuffer(indexBufferSize, "IndexBuffers");
        uniformBuffer = CreateBuffer(uniformBufferSize, "UniformBuffers");
    }
  
    void ReportMemoryUsage() {
        const auto& stats = Allocator::GetAllocationStats();
  
        for (const auto& [category, allocation] : stats) {
            if (strstr(category, "Renderer::") == category) {
                size_t usage = allocation.TotalAllocated - allocation.TotalFreed;
                SEDX_CORE_INFO("Renderer Category: {} - Usage: {} bytes", 
                              category, usage);
            }
        }
    }
};
```

### 3. Resource Management Pattern

```cpp
template<typename T>
class TrackedResource {
private:
    T* resource;
    const char* category;
  
public:
    TrackedResource(const char* resourceCategory) 
        : category(resourceCategory), resource(nullptr) {}
  
    void Allocate(size_t count = 1) {
        if (resource) {
            SEDX_CORE_WARN("Resource already allocated");
            return;
        }
  
        // Use category-based allocation
        resource = new(category) T[count];
        SEDX_CORE_TRACE("Allocated {} {} objects in category {}", 
                        count, typeid(T).name(), category);
    }
  
    void Deallocate() {
        if (resource) {
            delete[] resource;
            resource = nullptr;
            SEDX_CORE_TRACE("Deallocated resource in category {}", category);
        }
    }
  
    ~TrackedResource() {
        Deallocate();
    }
  
    T* Get() const { return resource; }
};

// Usage example
void ExampleUsage() {
    TrackedResource<Vertex> vertices("Geometry::Vertices");
    TrackedResource<uint32_t> indices("Geometry::Indices");
  
    vertices.Allocate(1000);
    indices.Allocate(3000);
  
    // Resources are automatically freed when destructors are called
    // Memory usage is tracked by category
}
```

### 4. Memory Pool Integration

```cpp
class CategoryMemoryPool {
private:
    const char* poolCategory;
    std::vector<void*> allocatedBlocks;
  
public:
    CategoryMemoryPool(const char* category) : poolCategory(category) {}
  
    void* Allocate(size_t size) {
        // All allocations from this pool use the same category
        void* memory = new(poolCategory) char[size];
        allocatedBlocks.push_back(memory);
        return memory;
    }
  
    void FreeAll() {
        for (void* block : allocatedBlocks) {
            delete[] static_cast<char*>(block);
        }
        allocatedBlocks.clear();
  
        SEDX_CORE_INFO("Freed all memory in pool category: {}", poolCategory);
    }
  
    size_t GetCategoryUsage() const {
        const auto& stats = Allocator::GetAllocationStats();
        auto it = stats.find(poolCategory);
        if (it != stats.end()) {
            return it->second.TotalAllocated - it->second.TotalFreed;
        }
        return 0;
    }
};
```

### 5. Debugging Memory Issues

```cpp
class MemoryLeakDetector {
public:
    static void TakeSnapshot(const std::string& label) {
        snapshots[label] = Memory::GetAllocationStats();
        SEDX_CORE_INFO("Memory snapshot taken: {}", label);
    }
  
    static void CompareSnapshots(const std::string& before, const std::string& after) {
        auto beforeIt = snapshots.find(before);
        auto afterIt = snapshots.find(after);
  
        if (beforeIt == snapshots.end() || afterIt == snapshots.end()) {
            SEDX_CORE_ERROR("Snapshot not found for comparison");
            return;
        }
  
        const auto& beforeStats = beforeIt->second;
        const auto& afterStats = afterIt->second;
  
        size_t beforeUsage = beforeStats.TotalAllocated - beforeStats.TotalFreed;
        size_t afterUsage = afterStats.TotalAllocated - afterStats.TotalFreed;
  
        if (afterUsage > beforeUsage) {
            SEDX_CORE_WARN("Memory usage increased by {} bytes between '{}' and '{}'",
                           afterUsage - beforeUsage, before, after);
        }
    }
  
    static void DetectCategoryLeaks() {
        const auto& stats = Allocator::GetAllocationStats();
  
        for (const auto& [category, allocation] : stats) {
            size_t currentUsage = allocation.TotalAllocated - allocation.TotalFreed;
            if (currentUsage > 0) {
                SEDX_CORE_WARN("Potential leak in category '{}': {} bytes", 
                              category, currentUsage);
            }
        }
    }
  
private:
    static std::map<std::string, AllocationStats> snapshots;
};
```

## Best Practices

### 1. Memory System Integration

- **Initialize Early**: Call `Allocator::Init()` at the very beginning of your application
- **Use new/delete**: Prefer `new` and `delete` for automatic source location tracking
- **Categorize Allocations**: Use descriptive category strings for better analysis

### 2. Category Naming Conventions

- Use hierarchical naming: `"Component::Subsystem::Resource"`
- Examples:
  - `"Renderer::Textures::Diffuse"`
  - `"Audio::Buffers::Music"`
  - `"Physics::Bodies::Static"`
  - `"UI::Widgets::Text"`

### 3. Resource Management

- Always pair allocations with deallocations
- Use RAII principles with smart pointers when possible
- Consider using the provided `TrackedResource` pattern for automatic cleanup

### 4. Performance Optimization

- Use `AllocateRaw()` for internal memory system operations to avoid recursion
- Be mindful of category string lifetimes (use string literals or ensure persistence)
- Consider disabling tracking in release builds if performance is critical

### 5. Debugging Workflow

```cpp
// Development phase - frequent memory checking
void DevelopmentMemoryCheck() {
    #ifndef SEDX_DIST
    MemoryLeakDetector::TakeSnapshot("before_operation");
  
    // Perform operations that might leak
    PerformComplexOperation();
  
    MemoryLeakDetector::TakeSnapshot("after_operation");
    MemoryLeakDetector::CompareSnapshots("before_operation", "after_operation");
    #endif
}
```

## Integration Guidelines

### 1. Existing Codebase Integration

When integrating into existing code:

```cpp
// Step 1: Add memory system initialization
int main() {
    SceneryEditorX::Allocator::Init();  // Add this line
  
    // Existing code...
    return 0;
}

// Step 2: Gradually replace new/delete with new/delete
// Old code:
MyClass* obj = new MyClass();
delete obj;

// New code:
MyClass* obj = new MyClass();
delete obj;

// Step 3: Add categories for major subsystems
// Renderer allocations
textureData = new("Renderer::Textures") uint8_t[size];

// Audio allocations  
audioBuffer = new("Audio::Buffers") float[samples];
```

### 2. Smart Pointer Integration

The memory system works seamlessly with smart pointers:

```cpp
// Custom deleter for tracked allocations
struct TrackedDeleter {
    void operator()(void* ptr) {
        SceneryEditorX::Allocator::Free(ptr);
    }
};

// Use with unique_ptr
std::unique_ptr<MyClass, TrackedDeleter> obj(
    static_cast<MyClass*>(SceneryEditorX::Allocator::Allocate(sizeof(MyClass), "SmartPointers"))
);
```

### 3. Container Integration

For STL containers that need tracking:

```cpp
// Create a tracked allocator for STL containers
template<typename T>
using TrackedAllocator = std::allocator<T>;  // Automatically uses overridden new/delete

// Use with containers
std::vector<int, TrackedAllocator<int>> trackedVector;
std::map<std::string, int, std::less<std::string>, 
         TrackedAllocator<std::pair<const std::string, int>>> trackedMap;
```

## Performance Considerations

### 1. Tracking Overhead

- **Memory overhead**: Approximately 40-64 bytes per allocation for tracking data
- **CPU overhead**: Mutex locking for each allocation/deallocation
- **Recommendation**: Disable in release builds if performance is critical

### 2. Optimization Strategies

```cpp
// Use SEDX_DIST macro for conditional tracking
#ifndef SEDX_DIST
    #define ENABLE_MEMORY_TRACKING 1
#else
    #define ENABLE_MEMORY_TRACKING 0
#endif

// Conditional allocation
inline void* AllocateOptimal(size_t size, const char* category = nullptr) {
    #if ENABLE_MEMORY_TRACKING
        return category ? 
            SceneryEditorX::Allocator::Allocate(size, category) :
            SceneryEditorX::Allocator::Allocate(size);
    #else
        return malloc(size);
    #endif
}
```

### 3. Large Allocation Handling

For very large allocations, consider:

```cpp
void* AllocateLarge(size_t size, const char* category) {
    if (size > LARGE_ALLOCATION_THRESHOLD) {
        // Use raw allocation for very large blocks to reduce tracking overhead
        SEDX_CORE_WARN("Large allocation detected: {} bytes in category {}", size, category);
        return SceneryEditorX::Allocator::AllocateRaw(size);
    }
  
    return SceneryEditorX::Allocator::Allocate(size, category);
}
```

## Debugging and Profiling

### 1. Memory Leak Detection

```cpp
class MemoryLeakChecker {
public:
    static void CheckForLeaks() {
        const auto& stats = SceneryEditorX::Memory::GetAllocationStats();
        size_t totalLeaked = stats.TotalAllocated - stats.TotalFreed;
  
        if (totalLeaked > 0) {
            SEDX_CORE_ERROR("Memory leak detected: {} bytes", totalLeaked);
      
            // Report category-specific leaks
            const auto& categoryStats = SceneryEditorX::Allocator::GetAllocationStats();
            for (const auto& [category, catStats] : categoryStats) {
                size_t categoryLeak = catStats.TotalAllocated - catStats.TotalFreed;
                if (categoryLeak > 0) {
                    SEDX_CORE_ERROR("  Category '{}': {} bytes leaked", category, categoryLeak);
                }
            }
        } else {
            SEDX_CORE_INFO("No memory leaks detected");
        }
    }
};
```

### 2. Tracy Profiler Integration

When `SEDX_ENABLE_PROFILING` is defined, the system automatically reports to Tracy:

```cpp
// Automatic Tracy integration in Allocator::Allocate
#if SEDX_ENABLE_PROFILING
    TracyAlloc(memory, size);  // Automatically called
#endif

// Custom Tracy zones for memory-intensive operations
void ProcessLargeDataset() {
    ZoneScoped;  // Tracy zone
  
    // Memory allocations here are automatically tracked in both
    // the SceneryEditorX system and Tracy profiler
    auto data = new DataPoint[LARGE_DATASET_SIZE];
  
    // Process data...
  
    delete[] data;
}
```

### 3. Memory Usage Reporting

```cpp
class MemoryReporter {
public:
    static void GenerateReport() {
        const auto& globalStats = SceneryEditorX::Memory::GetAllocationStats();
        const auto& categoryStats = SceneryEditorX::Allocator::GetAllocationStats();
  
        SEDX_CORE_INFO("=== Memory Usage Report ===");
        SEDX_CORE_INFO("Global - Allocated: {} bytes, Freed: {} bytes, Current: {} bytes",
                       globalStats.TotalAllocated,
                       globalStats.TotalFreed,
                       globalStats.TotalAllocated - globalStats.TotalFreed);
  
        SEDX_CORE_INFO("=== Category Breakdown ===");
        for (const auto& [category, stats] : categoryStats) {
            size_t current = stats.TotalAllocated - stats.TotalFreed;
            if (current > 0) {
                SEDX_CORE_INFO("  {}: {} bytes (allocated: {}, freed: {})",
                              category, current, stats.TotalAllocated, stats.TotalFreed);
            }
        }
    }
  
    static void GenerateJSONReport(const std::string& filename) {
        json report;
        const auto& globalStats = SceneryEditorX::Memory::GetAllocationStats();
        const auto& categoryStats = SceneryEditorX::Allocator::GetAllocationStats();
  
        report["global"]["allocated"] = globalStats.TotalAllocated;
        report["global"]["freed"] = globalStats.TotalFreed;
        report["global"]["current"] = globalStats.TotalAllocated - globalStats.TotalFreed;
  
        for (const auto& [category, stats] : categoryStats) {
            report["categories"][category]["allocated"] = stats.TotalAllocated;
            report["categories"][category]["freed"] = stats.TotalFreed;
            report["categories"][category]["current"] = stats.TotalAllocated - stats.TotalFreed;
        }
  
        std::ofstream file(filename);
        file << report.dump(4);
    }
};
```

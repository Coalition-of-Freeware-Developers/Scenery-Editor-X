# Scenery Editor X - Memory System - Development Instructions

---

## Overview

The memory system contains comprehensive memory allocation tracking, statistics collection, and debugging capabilities for the Scenery Editor X application. This system is designed to help developers monitor memory usage, detect memory leaks, and optimize memory allocation patterns throughout the application lifecycle.

## Core Architecture Understanding

### Memory System Hierarchy

```
Application Code (uses new/delete or new("category"))
    ↓
Global operator new/delete overrides (when SEDX_TRACK_MEMORY enabled)
    ↓
SceneryEditorX::Allocator static methods
    ↓
Internal tracking (AllocatorData with thread-safe maps)
    ↓
System malloc/free with comprehensive metadata
```

### Key Files and Their Roles

- `source/SceneryEditorX/core/memory.h` - Interface definitions, structs, and operator overrides
- `source/SceneryEditorX/core/memory.cpp` - Implementation of tracking logic and statistics

## Implementation Patterns for AI Assistance

### 1. Standard Memory Allocation Patterns

**ALWAYS use these patterns when generating new code:**

```cpp
// ✅ CORRECT - Automatic source location tracking
MyClass* obj = new MyClass(constructor_args);
auto* buffer = new uint8_t[bufferSize];

// ✅ CORRECT - Category-based allocation for subsystem tracking  
Texture* texture = new("Renderer::Textures") Texture(width, height);
AudioBuffer* audio = new("Audio::Buffers::Music") AudioBuffer(samples);

// ✅ CORRECT - Corresponding deallocation
delete obj;
delete[] buffer;
delete texture;
delete audio;
```

**AVOID these patterns:**

```cpp
// ❌ INCORRECT - Missing tracking in tracked builds
MyClass* obj = new MyClass();  // Use new instead
auto* buffer = malloc(size);   // Use new instead (unless specifically needed)

// ❌ INCORRECT - Not using delete  
delete obj;     // Use delete instead
free(buffer);   // Use delete[] instead
```

### 2. Category Naming Conventions for AI Code Generation

When generating code with memory allocations, use these category patterns:

```cpp
// Hierarchical naming: "System::Subsystem::ResourceType"
new("Renderer::Buffers::Vertex") VertexBuffer(size);
new("Renderer::Textures::Diffuse") Texture2D(width, height);
new("Renderer::Shaders::Fragment") ShaderProgram(source);

new("Audio::Buffers::Music") MusicBuffer(samples);
new("Audio::Buffers::Effects") SoundEffect(data);
new("Audio::Streaming::Decoder") AudioDecoder(format);

new("Physics::Bodies::Static") StaticBody(mesh);
new("Physics::Bodies::Dynamic") RigidBody(mass);
new("Physics::Constraints::Joint") HingeJoint(bodyA, bodyB);

new("UI::Widgets::Text") TextWidget(content);
new("UI::Widgets::Button") ButtonWidget(label);
new("UI::Rendering::Canvas") UICanvas(dimensions);

new("Assets::Meshes::Static") StaticMesh(vertices);
new("Assets::Textures::HDR") HDRTexture(data);
new("Assets::Materials::PBR") PBRMaterial(properties);

new("Memory::Pools::Temporary") TempAllocator(poolSize);
new("Memory::Buffers::Circular") CircularBuffer(capacity);
```

### 3. Resource Management Patterns for AI Code Generation

**RAII Pattern with Memory Tracking:**

```cpp
class TrackedResource {
private:
    void* data;
    const char* category;
    size_t size;

public:
    TrackedResource(size_t allocSize, const char* resourceCategory) 
        : category(resourceCategory), size(allocSize) {
        data = new(category) uint8_t[size];
    }
  
    ~TrackedResource() {
        delete[] static_cast<uint8_t*>(data);
    }
  
    // Non-copyable but movable
    TrackedResource(const TrackedResource&) = delete;
    TrackedResource& operator=(const TrackedResource&) = delete;
  
    TrackedResource(TrackedResource&& other) noexcept 
        : data(other.data), category(other.category), size(other.size) {
        other.data = nullptr;
    }
  
    TrackedResource& operator=(TrackedResource&& other) noexcept {
        if (this != &other) {
            if (data) delete[] static_cast<uint8_t*>(data);
            data = other.data;
            category = other.category;
            size = other.size;
            other.data = nullptr;
        }
        return *this;
    }
  
    void* Get() const { return data; }
    size_t Size() const { return size; }
};
```

**Smart Pointer Integration:**

```cpp
// Custom deleter for tracked allocations
template<typename T>
struct TrackedDeleter {
    void operator()(T* ptr) {
        delete ptr;
    }
};

// Usage in generated code
template<typename T>
using TrackedUniquePtr = std::unique_ptr<T, TrackedDeleter<T>>;

template<typename T, typename... Args>
TrackedUniquePtr<T> MakeTrackedUnique(const char* category, Args&&... args) {
    T* ptr = new(category) T(std::forward<Args>(args)...);
    return TrackedUniquePtr<T>(ptr);
}

// Example usage in AI-generated code
auto texture = MakeTrackedUnique<Texture2D>("Renderer::Textures", width, height);
auto buffer = MakeTrackedUnique<VertexBuffer>("Renderer::Buffers::Vertex", vertexCount);
```

### 4. Memory Statistics and Debugging Integration

**When generating debugging/profiling code:**

```cpp
// Memory usage reporting functions
void ReportSubsystemMemoryUsage(const std::string& subsystemPrefix) {
    const auto& stats = SceneryEditorX::Allocator::GetAllocationStats();
  
    size_t totalSubsystemUsage = 0;
    SEDX_CORE_INFO("=== {} Subsystem Memory Usage ===", subsystemPrefix);
  
    for (const auto& [category, allocation] : stats) {
        if (std::string(category).find(subsystemPrefix) == 0) {
            size_t currentUsage = allocation.TotalAllocated - allocation.TotalFreed;
            if (currentUsage > 0) {
                SEDX_CORE_INFO("  {}: {} bytes", category, currentUsage);
                totalSubsystemUsage += currentUsage;
            }
        }
    }
  
    SEDX_CORE_INFO("Total {} usage: {} bytes", subsystemPrefix, totalSubsystemUsage);
}

// Memory leak detection for specific categories
bool CheckCategoryForLeaks(const char* categoryPrefix) {
    const auto& stats = SceneryEditorX::Allocator::GetAllocationStats();
    bool leaksFound = false;
  
    for (const auto& [category, allocation] : stats) {
        if (strstr(category, categoryPrefix) == category) {
            size_t leaked = allocation.TotalAllocated - allocation.TotalFreed;
            if (leaked > 0) {
                SEDX_CORE_WARN("Memory leak in category '{}': {} bytes", category, leaked);
                leaksFound = true;
            }
        }
    }
  
    return leaksFound;
}
```

### 5. Class Design Patterns for Memory-Tracked Components

**When generating class definitions that allocate memory:**

```cpp
class RendererComponent {
private:
    void* bufferData;
    size_t bufferSize;
    static constexpr const char* MEMORY_CATEGORY = "Renderer::Components";
  
public:
    explicit RendererComponent(size_t size) : bufferSize(size) {
        bufferData = new(MEMORY_CATEGORY) uint8_t[bufferSize];
        SEDX_CORE_TRACE("RendererComponent allocated {} bytes", bufferSize);
    }
  
    ~RendererComponent() {
        if (bufferData) {
            delete[] static_cast<uint8_t*>(bufferData);
            SEDX_CORE_TRACE("RendererComponent deallocated {} bytes", bufferSize);
            bufferData = nullptr;
        }
    }
  
    // Ensure proper move semantics
    RendererComponent(RendererComponent&& other) noexcept 
        : bufferData(other.bufferData), bufferSize(other.bufferSize) {
        other.bufferData = nullptr;
        other.bufferSize = 0;
    }
  
    RendererComponent& operator=(RendererComponent&& other) noexcept {
        if (this != &other) {
            if (bufferData) {
                delete[] static_cast<uint8_t*>(bufferData);
            }
            bufferData = other.bufferData;
            bufferSize = other.bufferSize;
            other.bufferData = nullptr;
            other.bufferSize = 0;
        }
        return *this;
    }
  
    // Delete copy operations to prevent double-free
    RendererComponent(const RendererComponent&) = delete;
    RendererComponent& operator=(const RendererComponent&) = delete;
};
```

### 6. Container Usage Patterns

**When generating code that uses STL containers with memory tracking:**

```cpp
// Standard containers automatically use tracked allocation when global operators are overridden
std::vector<Vertex> vertices;           // Automatically tracked
std::map<std::string, Texture*> textures; // Automatically tracked

// For explicit category tracking with containers:
template<typename T>
class TrackedVector {
private:
    std::vector<T> impl;
    const char* category;
  
public:
    explicit TrackedVector(const char* cat) : category(cat) {
        // Reserve with tracked allocation
        impl.reserve(0); // Will use overridden operators
    }
  
    void push_back(const T& value) {
        impl.push_back(value);
    }
  
    void push_back(T&& value) {
        impl.push_back(std::move(value));
    }
  
    // Delegate other operations...
    size_t size() const { return impl.size(); }
    T& operator[](size_t idx) { return impl[idx]; }
    const T& operator[](size_t idx) const { return impl[idx]; }
  
    auto begin() { return impl.begin(); }
    auto end() { return impl.end(); }
    auto begin() const { return impl.begin(); }
    auto end() const { return impl.end(); }
};

// Usage in generated code
TrackedVector<Vertex> geometryVertices("Renderer::Geometry::Vertices");
TrackedVector<uint32_t> geometryIndices("Renderer::Geometry::Indices");
```

### 7. Initialization Patterns for AI-Generated Applications

**When generating main functions or application initialization:**

```cpp
int main() {
    // ALWAYS initialize memory system first
    SceneryEditorX::Allocator::Init();
  
    try {
        // Application code here
        Application app;
        return app.Run();
    }
    catch (const std::exception& e) {
        SEDX_CORE_ERROR("Application error: {}", e.what());
  
        // Optional: Check for memory leaks before exit
        #ifndef SEDX_DIST
        const auto& stats = SceneryEditorX::Memory::GetAllocationStats();
        size_t leaked = stats.TotalAllocated - stats.TotalFreed;
        if (leaked > 0) {
            SEDX_CORE_WARN("Memory leak detected at exit: {} bytes", leaked);
        }
        #endif
  
        return -1;
    }
}

// For subsystem initialization
class SubsystemManager {
public:
    static void InitializeRenderer() {
        // Use category-based allocation for renderer initialization
        rendererData = new("Renderer::Core") RendererData();
  
        SEDX_CORE_INFO("Renderer initialized with memory tracking");
    }
  
    static void ShutdownRenderer() {
        if (rendererData) {
            delete rendererData;
            rendererData = nullptr;
        }
  
        // Check for renderer-specific leaks
        if (CheckCategoryForLeaks("Renderer::")) {
            SEDX_CORE_WARN("Renderer memory leaks detected during shutdown");
        }
    }
};
```

### 8. Macro Usage Guidelines for AI Code Generation

**When generating code, use these memory-related macros correctly:**

```cpp
// ✅ CORRECT usage
#ifdef SEDX_TRACK_MEMORY
    auto* trackedBuffer = new uint8_t[size];
    // Memory tracking active
#else
    auto* trackedBuffer = new uint8_t[size];
    // Fallback to standard allocation
#endif

// ✅ CORRECT conditional memory reporting
#ifndef SEDX_DIST
    ReportSubsystemMemoryUsage("Renderer");
    CheckCategoryForLeaks("Audio::");
#endif

// ✅ CORRECT profiling integration
#if SEDX_ENABLE_PROFILING
    ZoneScoped; // Tracy zone
    auto* data = new LargeDataStructure();
    // Both Tracy and SceneryEditorX tracking active
#endif
```

### 9. Error Handling Patterns

**When generating error-prone memory operations:**

```cpp
template<typename T, typename... Args>
T* SafeTrackedNew(const char* category, Args&&... args) {
    try {
        return new(category) T(std::forward<Args>(args)...);
    }
    catch (const std::bad_alloc& e) {
        SEDX_CORE_ERROR("Memory allocation failed for category '{}': {}", category, e.what());
  
        // Report current memory usage to help diagnose the issue
        const auto& stats = SceneryEditorX::Memory::GetAllocationStats();
        size_t currentUsage = stats.TotalAllocated - stats.TotalFreed;
        SEDX_CORE_ERROR("Current memory usage: {} bytes", currentUsage);
  
        throw; // Re-throw for caller to handle
    }
}

// Exception-safe resource management
template<typename T>
class ExceptionSafeResource {
private:
    T* resource;
  
public:
    template<typename... Args>
    ExceptionSafeResource(const char* category, Args&&... args) 
        : resource(nullptr) {
        try {
            resource = new(category) T(std::forward<Args>(args)...);
        }
        catch (...) {
            // Resource is null, destructor will handle safely
            throw;
        }
    }
  
    ~ExceptionSafeResource() {
        if (resource) {
            delete resource;
        }
    }
  
    T* Get() const { return resource; }
    T& operator*() const { return *resource; }
    T* operator->() const { return resource; }
  
    // Move-only semantics
    ExceptionSafeResource(const ExceptionSafeResource&) = delete;
    ExceptionSafeResource& operator=(const ExceptionSafeResource&) = delete;
  
    ExceptionSafeResource(ExceptionSafeResource&& other) noexcept 
        : resource(other.resource) {
        other.resource = nullptr;
    }
  
    ExceptionSafeResource& operator=(ExceptionSafeResource&& other) noexcept {
        if (this != &other) {
            if (resource) delete resource;
            resource = other.resource;
            other.resource = nullptr;
        }
        return *this;
    }
};
```

### 10. Performance-Conscious Patterns

**When generating performance-critical code:**

```cpp
// Use raw allocation for temporary/internal operations
class PerformanceCriticalAllocator {
public:
    // For high-frequency, short-lived allocations in hot paths
    static void* FastAlloc(size_t size) {
        #ifdef SEDX_DIST
            return malloc(size); // Skip tracking in release builds
        #else
            return SceneryEditorX::Allocator::AllocateRaw(size); // Skip tracking but keep consistent interface
        #endif
    }
  
    static void FastFree(void* ptr) {
        #ifdef SEDX_DIST
            free(ptr);
        #else
            free(ptr); // Raw allocation requires raw free
        #endif
    }
};

// Memory pool pattern for high-frequency allocations
class TrackedMemoryPool {
private:
    void* poolMemory;
    size_t poolSize;
    size_t currentOffset;
    const char* category;
  
public:
    TrackedMemoryPool(size_t size, const char* poolCategory) 
        : poolSize(size), currentOffset(0), category(poolCategory) {
        // Single large allocation for the entire pool
        poolMemory = new(category) uint8_t[poolSize];
    }
  
    ~TrackedMemoryPool() {
        delete[] static_cast<uint8_t*>(poolMemory);
    }
  
    void* Allocate(size_t size) {
        if (currentOffset + size > poolSize) {
            SEDX_CORE_ERROR("Pool allocation failed: insufficient space");
            return nullptr;
        }
  
        void* result = static_cast<uint8_t*>(poolMemory) + currentOffset;
        currentOffset += size;
        return result;
    }
  
    void Reset() {
        currentOffset = 0; // Reset pool without deallocating
    }
};
```

## Code Rules Summary

### MUST DO:

1. Always use `new`/`delete` instead of `new`/`delete`
2. Initialize memory system with `SceneryEditorX::Allocator::Init()` in main functions
3. Use descriptive, hierarchical category names for tracked allocations
4. Implement proper RAII patterns with move semantics
5. Add memory leak checking in debug/development code paths
6. Use Tracy profiling macros when available

### MUST NOT DO:

1. Mix tracked and untracked allocation/deallocation
2. Use raw `malloc`/`free` unless specifically needed (use `AllocateRaw` instead)
3. Forget to handle memory tracking in exception paths
4. Use category strings with dynamic lifetime (prefer string literals)
5. Generate code that bypasses the tracking system without good reason

### WHEN IN DOUBT:

- Default to using `new`/`delete` for all dynamic allocations
- Use appropriate category strings that match the component hierarchy
- Add debug-only memory usage reporting for new subsystems
- Follow the existing patterns in the codebase for consistency

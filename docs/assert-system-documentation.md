# Scenery Editor X - Assert System Documentation

---

## Overview

The Scenery Editor X assert system provides a robust framework for runtime validation and debugging. Built on top of the logging system, it offers conditional compilation support, cross-platform debugging capabilities, and comprehensive error reporting. The system includes both traditional assertions (debug-only) and verification macros (always enabled) to support different validation requirements throughout the application lifecycle.

## Architecture

The assert system is implemented in `SceneryEditorX/logging/asserts.h` and integrates seamlessly with the logging framework defined in `logging.hpp`. It provides platform-specific debug break functionality and leverages the existing log categorization system.

### Key Components

1. **Debug Break Support**: Platform-specific debugging hooks for development
2. **Conditional Compilation**: Debug-only assertions with release-mode optimization
3. **Verification System**: Always-enabled validation for critical checks
4. **Integration with Logging**: Utilizes the existing log categorization (Core/Editor)
5. **Message Formatting**: Support for formatted error messages with variable arguments

## Available Macros

### Debug Break Macros

```cpp
#ifdef SEDX_PLATFORM_WINDOWS
    #define SEDX_DEBUG_BREAK __debugbreak()
#elif defined(SEDX_COMPILER_CLANG)
    #define SEDX_DEBUG_BREAK __builtin_debugtrap()
#else
    #define SEDX_DEBUG_BREAK
#endif
```

Platform-specific debugging hooks that trigger debugger breaks when assertions fail.

### Assertion Macros

#### SEDX_CORE_ASSERT(condition, ...)

**Purpose**: Core system assertions for engine/framework-level validation.
**Behavior**:

- **Debug builds**: Evaluates condition, logs error message with file/line info, triggers debug break on failure
- **Release builds**: Completely optimized out (condition not evaluated)

**Usage Guidelines**:

- Use for critical engine subsystems (renderer, memory, core modules)
- Validate function parameters and preconditions
- Check resource initialization and validity
- Verify internal state consistency

**Examples**:

```cpp
// Parameter validation
void CreateBuffer(uint64_t size, BufferUsageFlags usage, MemoryFlags memory)
{
    SEDX_CORE_ASSERT(size > 0, "Buffer size must be greater than zero");
    SEDX_CORE_ASSERT(usage != 0, "Buffer usage flags cannot be empty");
  
    // Buffer creation logic...
}

// Resource validation
void InitializeRenderer()
{
    SEDX_CORE_ASSERT(m_Device != nullptr, "Vulkan device not initialized");
    SEDX_CORE_ASSERT(m_SwapChain.IsValid(), "SwapChain initialization failed");
  
    // Renderer initialization...
}

// State validation
void RenderFrame()
{
    SEDX_CORE_ASSERT(m_CommandBuffers.size() > 0, "No command buffers available");
    SEDX_CORE_ASSERT(m_CurrentFrame < MAX_FRAMES_IN_FLIGHT, 
                     "Frame index {} exceeds maximum {}", m_CurrentFrame, MAX_FRAMES_IN_FLIGHT);
}

// Array bounds checking
template<typename T>
T& DynamicArray<T>::operator[](size_t index)
{
    SEDX_CORE_ASSERT(index < m_Size, "Index {} out of bounds (size: {})", index, m_Size);
    return m_Data[index];
}
```

#### SEDX_ASSERT(condition, ...)

**Purpose**: Application-level assertions for editor and user-facing functionality.
**Behavior**: Same as SEDX_CORE_ASSERT but uses Editor logging category.

**Usage Guidelines**:

- Use for editor-specific validation
- UI state validation
- User input validation
- Application logic verification

**Examples**:

```cpp
// UI state validation
void ScenePanel::SelectObject(uint32_t objectId)
{
    SEDX_ASSERT(objectId < m_SceneObjects.size(), 
               "Object ID {} does not exist in scene", objectId);
    SEDX_ASSERT(m_SceneObjects[objectId].IsValid(), 
               "Cannot select invalid object {}", objectId);
  
    m_SelectedObject = objectId;
}

// Editor operation validation
void AssetBrowser::LoadAsset(const std::string& filePath)
{
    SEDX_ASSERT(!filePath.empty(), "Asset path cannot be empty");
    SEDX_ASSERT(std::filesystem::exists(filePath), 
               "Asset file does not exist: {}", filePath);
  
    // Asset loading logic...
}

// Module state validation
class CustomModule : public Module
{
    void OnUpdate() override
    {
        SEDX_ASSERT(m_IsInitialized, "Module {} not properly initialized", GetName());
  
        // Update logic...
    }
};
```

### Verification Macros

#### SEDX_CORE_VERIFY(condition, ...)

**Purpose**: Core system verification that runs in both debug and release builds.
**Behavior**: Always evaluates condition and logs errors, but continues execution.

**Usage Guidelines**:

- Use for non-fatal but important validations
- Performance monitoring and diagnostics
- Resource availability checks
- Cross-platform compatibility checks

**Examples**:

```cpp
// Resource loading verification
Ref<Texture2D> LoadTexture(const std::string& path)
{
    auto texture = CreateRef<Texture2D>(path);
    SEDX_CORE_VERIFY(texture->IsValid(), "Failed to load texture: {}", path);
  
    return texture; // Return even if invalid for graceful degradation
}

// Performance verification
void UpdatePhysics(float deltaTime)
{
    auto start = std::chrono::high_resolution_clock::now();
  
    // Physics update logic...
  
    auto duration = std::chrono::high_resolution_clock::now() - start;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
  
    SEDX_CORE_VERIFY(ms < 16, "Physics update took {}ms (target: <16ms)", ms);
}

// Configuration verification
void LoadShaderPack(const std::string& packPath)
{
    SEDX_CORE_VERIFY(std::filesystem::exists(packPath), 
                     "Shader pack not found: {}", packPath);
  
    // Continue with default shaders if pack not found
    LoadDefaultShaders();
}
```

#### SEDX_VERIFY(condition, ...)

**Purpose**: Application-level verification for editor functionality.
**Behavior**: Same as SEDX_CORE_VERIFY but uses Editor logging category.

**Examples**:

```cpp
// Editor preference verification
void ApplyEditorSettings(const EditorConfig& config)
{
    SEDX_VERIFY(config.windowWidth > 0 && config.windowHeight > 0,
               "Invalid window dimensions: {}x{}", config.windowWidth, config.windowHeight);
  
    // Apply settings with fallbacks for invalid values
    SetWindowSize(std::max(config.windowWidth, 800), std::max(config.windowHeight, 600));
}

// Asset management verification
void ImportModel(const std::string& modelPath)
{
    SEDX_VERIFY(HasValidFormat(modelPath), "Unsupported model format: {}", modelPath);
  
    // Continue with import, may use default converter
    ProcessModelImport(modelPath);
}
```

## Integration Patterns

### Module System Integration

```cpp
class RendererModule : public Module
{
public:
    void OnAttach() override
    {
        SEDX_CORE_INFO("=== Initializing {} ===", GetName());
  
        // Critical initialization - use ASSERT
        SEDX_CORE_ASSERT(InitializeVulkan(), "Failed to initialize Vulkan");
        SEDX_CORE_ASSERT(CreateSwapChain(), "Failed to create swap chain");
  
        // Non-critical features - use VERIFY
        SEDX_CORE_VERIFY(LoadOptionalExtensions(), "Some optional extensions unavailable");
  
        m_IsInitialized = true;
        SEDX_CORE_INFO_TAG("INIT", "Renderer module initialized successfully");
    }
  
    void OnUpdate() override
    {
        if (!m_IsInitialized) return;
  
        SEDX_PROFILE_SCOPE("RendererModule::OnUpdate");
  
        // Runtime validation
        SEDX_CORE_ASSERT(m_Device->IsValid(), "Vulkan device became invalid");
  
        RenderFrame();
    }
  
private:
    bool m_IsInitialized = false;
};
```

### Memory Management Integration

```cpp
template<typename T>
class CustomAllocator
{
public:
    T* Allocate(size_t count)
    {
        SEDX_CORE_ASSERT(count > 0, "Cannot allocate zero elements");
        SEDX_CORE_ASSERT(count <= MAX_ALLOCATION_SIZE, 
                         "Allocation size {} exceeds maximum {}", count, MAX_ALLOCATION_SIZE);
  
        T* ptr = static_cast<T*>(std::malloc(count * sizeof(T)));
        SEDX_CORE_VERIFY(ptr != nullptr, "Memory allocation failed for {} elements", count);
  
        return ptr;
    }
  
    void Deallocate(T* ptr, size_t count)
    {
        SEDX_CORE_ASSERT(ptr != nullptr, "Cannot deallocate null pointer");
  
        std::free(ptr);
    }
};
```

### Resource Management Integration

```cpp
class BufferManager
{
public:
    Buffer CreateBuffer(uint64_t size, BufferUsageFlags usage, MemoryFlags memory)
    {
        // Input validation with assertions
        SEDX_CORE_ASSERT(size > 0, "Buffer size must be greater than zero");
        SEDX_CORE_ASSERT(usage != 0, "Buffer usage flags cannot be empty");
  
        // Adjust usage flags based on buffer type
        if (usage & BufferUsage::Vertex)
        {
            usage |= BufferUsage::TransferDst;
            SEDX_CORE_VERIFY(size % sizeof(Vertex) == 0, 
                           "Vertex buffer size {} not aligned to vertex size", size);
        }
  
        // Create buffer with validation
        const auto device = RenderContext::Get()->GetLogicDevice();
        SEDX_CORE_ASSERT(device != nullptr, "No valid logical device available");
  
        VkBuffer bufferHandle;
        VkResult result = CreateVulkanBuffer(device, size, usage, &bufferHandle);
  
        SEDX_CORE_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan buffer: {}", 
                         VkResultToString(result));
  
        return Buffer{bufferHandle, size, usage, memory};
    }
};
```

## Best Practices

### When to Use Each Macro Type

#### Use SEDX_CORE_ASSERT when:

- Validating critical engine preconditions
- Checking resource initialization success
- Verifying memory allocations in debug builds
- Validating function parameters that could cause crashes
- Checking array bounds and buffer overflows
- Verifying thread safety assumptions

#### Use SEDX_ASSERT when:

- Validating editor state consistency
- Checking user input boundaries
- Verifying UI component states
- Validating application-level logic
- Checking file operations and asset loading

#### Use SEDX_CORE_VERIFY when:

- Checking optional feature availability
- Performance monitoring and diagnostics
- Validating configuration files
- Checking network connectivity
- Monitoring resource usage

#### Use SEDX_VERIFY when:

- Validating user preferences
- Checking plugin compatibility
- Monitoring editor performance
- Validating workspace states

### Message Formatting Guidelines

```cpp
// Good: Descriptive messages with context
SEDX_CORE_ASSERT(texture != nullptr, "Failed to load texture from path: {}", texturePath);
SEDX_CORE_ASSERT(index < size, "Array index {} out of bounds (size: {})", index, size);

// Good: State information
SEDX_ASSERT(m_IsInitialized, "Module '{}' operation attempted before initialization", GetName());

// Avoid: Vague messages
SEDX_CORE_ASSERT(ptr != nullptr, "Null pointer"); // Too vague

// Avoid: No context
SEDX_CORE_ASSERT(result == VK_SUCCESS, "Vulkan operation failed"); // What operation?
```

### Performance Considerations

```cpp
// Good: Expensive checks only in debug
#ifdef SEDX_DEBUG
    bool ValidateComplexState() const
    {
        // Expensive validation logic
        return true;
    }
#endif

void CriticalFunction()
{
    SEDX_CORE_ASSERT(ValidateComplexState(), "Complex state validation failed");
  
    // Function implementation...
}

// Good: Simple checks that compile out
SEDX_CORE_ASSERT(m_InitFlag, "Not initialized"); // Simple flag check

// Avoid: Side effects in assertions
SEDX_CORE_ASSERT(++counter < MAX_COUNT, "Counter overflow"); // BAD: Modifies counter
```

### Error Recovery Patterns

```cpp
// Pattern 1: Graceful degradation with VERIFY
Ref<Shader> LoadShader(const std::string& path)
{
    auto shader = CreateRef<Shader>(path);
  
    if (!SEDX_CORE_VERIFY(shader->Compile(), "Shader compilation failed: {}", path))
    {
        // Fallback to default shader
        return GetDefaultShader();
    }
  
    return shader;
}

// Pattern 2: Early return with validation
bool InitializeSubsystem()
{
    if (!SEDX_CORE_VERIFY(CheckDependencies(), "Subsystem dependencies not met"))
    {
        return false;
    }
  
    // Continue initialization...
    return true;
}

// Pattern 3: Resource cleanup on assertion failure  
class ResourceManager
{
    ~ResourceManager()
    {
        // Verify all resources cleaned up
        SEDX_CORE_VERIFY(m_ActiveResources.empty(), 
                         "{} resources leaked during shutdown", m_ActiveResources.size());
    }
};
```

## Conditional Compilation

The assert system supports different build configurations:

### Debug Builds (SEDX_DEBUG defined)

- `SEDX_ENABLE_ASSERTS` is automatically defined
- All assertions are active and will break on failure
- Full message formatting and logging
- Performance impact from condition evaluation

### Release Builds (SEDX_DEBUG not defined)

- Assertions compile to `((void)(condition))` - no-op
- Verification macros remain active
- Optimized performance with no assertion overhead
- Production-ready error handling

### Force Enable Features

```cpp
// Force enable verification in all builds
#define SEDX_ENABLE_VERIFY

// Force enable assertions (not recommended for release)
#define SEDX_ENABLE_ASSERTS
```

## Logging Integration

The assert system integrates with the logging framework:

- **Core assertions**: Use `Log::Type::Core` logger
- **Editor assertions**: Use `Log::Type::Editor` logger
- **File/line information**: Automatically included in messages
- **Message formatting**: Supports std::format-style arguments
- **Platform dialogs**: Optional message boxes on Windows (debug builds)

Example log output:

```
[2025-07-10 14:32:15.123] [core] [error] Assertion Failed (vk_buffer.cpp:45) Buffer size must be greater than zero
[2025-07-10 14:32:15.124] [editor] [error] Verify Failed (scene_panel.cpp:123) Object ID 42 does not exist in scene
```

## Thread Safety

The assert system is thread-safe through the underlying logging system:

```cpp
// Safe to use from multiple threads
void WorkerThread()
{
    SEDX_CORE_ASSERT(m_ThreadSafeResource != nullptr, "Resource not available in worker thread");
  
    // Thread work...
}

// Logger handles concurrent access safely
std::mutex g_AssertMutex; // Not needed - handled internally
```

## Testing and Validation

### Unit Testing with Asserts

```cpp
// Test that asserts fire correctly
TEST_CASE("Buffer Creation Validation")
{
    SECTION("Zero size should assert")
    {
        // In test builds, you might want to catch assertion failures
        REQUIRE_THROWS([]{
            CreateBuffer(0, BufferUsage::Vertex, MemoryFlags::GPU);
        });
    }
  
    SECTION("Valid parameters should succeed")
    {
        auto buffer = CreateBuffer(1024, BufferUsage::Vertex, MemoryFlags::GPU);
        REQUIRE(buffer.IsValid());
    }
}
```

### Integration Testing

```cpp
class ModuleTestFixture
{
public:
    void SetUp()
    {
        // Ensure clean state for assertions
        m_Module = CreateRef<TestModule>();
        SEDX_CORE_ASSERT(m_Module != nullptr, "Failed to create test module");
    }
  
    void ValidateModuleState()
    {
        SEDX_ASSERT(m_Module->IsInitialized(), "Module should be initialized after setup");
        SEDX_VERIFY(m_Module->GetResourceCount() > 0, "Module should have loaded resources");
    }
};
```

## Platform-Specific Considerations

### Windows

- Debug breaks use `__debugbreak()`
- Optional message box display for assertions
- Full debugging symbol support

### Clang/GCC

- Debug breaks use `__builtin_debugtrap()`
- Standard stderr output for messages
- Compatible with GDB and LLDB debuggers

### Cross-Platform Development

```cpp
// Platform-agnostic validation
void ValidatePlatformFeature()
{
    #ifdef SEDX_PLATFORM_WINDOWS
        SEDX_CORE_VERIFY(CheckWindowsFeature(), "Windows-specific feature unavailable");
    #elif defined(SEDX_PLATFORM_LINUX)
        SEDX_CORE_VERIFY(CheckLinuxFeature(), "Linux-specific feature unavailable");
    #elif defined(SEDX_PLATFORM_MAC)  
        SEDX_CORE_VERIFY(CheckMacFeature(), "macOS-specific feature unavailable");
    #endif
}
```

## Migration and Adoption

### Converting from Standard Assertions

```cpp
// Old style
assert(ptr != nullptr);
assert(size > 0 && "Size must be positive");

// New style  
SEDX_CORE_ASSERT(ptr != nullptr, "Null pointer encountered");
SEDX_CORE_ASSERT(size > 0, "Size must be positive, got: {}", size);
```

### Converting from Manual Checks

```cpp
// Old style
if (!condition)
{
    SEDX_CORE_ERROR("Validation failed");
    // Manual error handling...
}

// New style
SEDX_CORE_ASSERT(condition, "Validation failed: {}", GetContextInfo());
```

### Gradual Adoption Strategy

1. **Start with critical paths**: Add assertions to core systems first
2. **Use VERIFY for existing checks**: Convert existing error logging to verification
3. **Add parameter validation**: Systematically add input validation
4. **Expand to edge cases**: Add assertions for boundary conditions
5. **Performance tune**: Review and optimize assertion placement

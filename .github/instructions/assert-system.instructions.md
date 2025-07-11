# Assert System Instructions for GitHub Copilot

## Overview

These instructions guide GitHub Copilot on the proper usage of the Scenery Editor X custom assert system. The assert system provides debug-time validation with optimized release builds and comprehensive error reporting through the logging framework.

## Assert System Location

- **Header File**: `source/SceneryEditorX/logging/asserts.h`
- **Integration**: Built on top of the logging system in `source/SceneryEditorX/logging/logging.hpp`
- **Documentation**: See `docs/assert-system-documentation.md` for comprehensive details

## Available Assert Macros

### Core System Assertions (Engine/Framework Level)

#### SEDX_CORE_ASSERT(condition, ...)
- **Purpose**: Critical engine subsystem validation
- **Behavior**: Debug-only, optimized out in release builds
- **Usage**: Parameter validation, resource initialization, state consistency
- **Format**: `SEDX_CORE_ASSERT(condition, "message format", args...)`

```cpp
// Examples of proper usage:
SEDX_CORE_ASSERT(size > 0, "Buffer size must be greater than zero");
SEDX_CORE_ASSERT(m_Device != nullptr, "Vulkan device not initialized");
SEDX_CORE_ASSERT(index < m_Size, "Index {} out of bounds (size: {})", index, m_Size);
```

#### SEDX_CORE_VERIFY(condition, ...)
- **Purpose**: Non-fatal core system validation
- **Behavior**: Always enabled, logs errors but continues execution
- **Usage**: Optional features, performance monitoring, graceful degradation
- **Format**: `SEDX_CORE_VERIFY(condition, "message format", args...)`

```cpp
// Examples of proper usage:
SEDX_CORE_VERIFY(texture->IsValid(), "Failed to load texture: {}", path);
SEDX_CORE_VERIFY(ms < 16, "Physics update took {}ms (target: <16ms)", ms);
```

### Application/Editor Level Assertions

#### SEDX_ASSERT(condition, ...)
- **Purpose**: Editor and application-level validation
- **Behavior**: Debug-only, optimized out in release builds
- **Usage**: UI state validation, user input validation, application logic
- **Format**: `SEDX_ASSERT(condition, "message format", args...)`

```cpp
// Examples of proper usage:
SEDX_ASSERT(objectId < m_SceneObjects.size(), "Object ID {} does not exist", objectId);
SEDX_ASSERT(m_IsInitialized, "Module {} not properly initialized", GetName());
```

#### SEDX_VERIFY(condition, ...)
- **Purpose**: Non-fatal application validation
- **Behavior**: Always enabled, logs errors but continues execution
- **Usage**: User preferences, plugin compatibility, workspace validation
- **Format**: `SEDX_VERIFY(condition, "message format", args...)`

```cpp
// Examples of proper usage:
SEDX_VERIFY(config.windowWidth > 0, "Invalid window width: {}", config.windowWidth);
SEDX_VERIFY(HasValidFormat(path), "Unsupported model format: {}", path);
```

## When to Use Each Macro

### Use SEDX_CORE_ASSERT for:
- ✅ Critical engine preconditions that must never fail
- ✅ Resource initialization validation
- ✅ Memory allocation success checks
- ✅ Function parameter validation that could cause crashes
- ✅ Array bounds and buffer overflow prevention
- ✅ Thread safety assumption verification

### Use SEDX_ASSERT for:
- ✅ Editor state consistency checks
- ✅ User input boundary validation
- ✅ UI component state verification
- ✅ Application-level logic validation
- ✅ File operation and asset loading checks

### Use SEDX_CORE_VERIFY for:
- ✅ Optional feature availability checks
- ✅ Performance monitoring and diagnostics
- ✅ Configuration file validation
- ✅ Network connectivity checks
- ✅ Resource usage monitoring

### Use SEDX_VERIFY for:
- ✅ User preference validation
- ✅ Plugin compatibility checks
- ✅ Editor performance monitoring
- ✅ Workspace state validation

## Code Generation Patterns

### Module System Integration
When generating modules that inherit from the Module base class:

```cpp
class CustomModule : public Module
{
public:
    void OnAttach() override
    {
        SEDX_CORE_INFO("=== Initializing {} ===", GetName());
        
        // Critical initialization - use ASSERT
        SEDX_CORE_ASSERT(InitializeResources(), "Failed to initialize module resources");
        
        // Non-critical features - use VERIFY
        SEDX_CORE_VERIFY(LoadOptionalFeatures(), "Some optional features unavailable");
        
        m_IsInitialized = true;
    }
    
    void OnUpdate() override
    {
        if (!m_IsEnabled) return;
        
        SEDX_CORE_ASSERT(m_IsInitialized, "Module {} not initialized", GetName());
        SEDX_PROFILE_SCOPE("CustomModule::OnUpdate");
        
        // Update logic here
    }
    
private:
    bool m_IsInitialized = false;
    bool m_IsEnabled = true;
};
```

### Resource Management Patterns
When generating resource management code:

```cpp
class ResourceManager
{
public:
    Ref<Resource> LoadResource(const std::string& path)
    {
        SEDX_CORE_ASSERT(!path.empty(), "Resource path cannot be empty");
        SEDX_CORE_ASSERT(std::filesystem::exists(path), "Resource file not found: {}", path);
        
        auto resource = CreateRef<Resource>();
        SEDX_CORE_VERIFY(resource->LoadFromFile(path), "Failed to load resource: {}", path);
        
        return resource;
    }
    
    void ReleaseResource(Ref<Resource>& resource)
    {
        SEDX_CORE_ASSERT(resource != nullptr, "Cannot release null resource");
        resource.Reset();
    }
};
```

### Vulkan/Graphics Code Patterns
When generating Vulkan or graphics-related code:

```cpp
Buffer CreateBuffer(uint64_t size, BufferUsageFlags usage, MemoryFlags memory)
{
    // Input validation
    SEDX_CORE_ASSERT(size > 0, "Buffer size must be greater than zero");
    SEDX_CORE_ASSERT(usage != 0, "Buffer usage flags cannot be empty");
    
    const auto device = RenderContext::Get()->GetLogicDevice();
    SEDX_CORE_ASSERT(device != nullptr, "No valid logical device available");
    
    // Create buffer
    VkBuffer bufferHandle;
    VkResult result = CreateVulkanBuffer(device, size, usage, &bufferHandle);
    SEDX_CORE_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan buffer: {}", 
                     VkResultToString(result));
    
    return Buffer{bufferHandle, size, usage, memory};
}
```

### Array/Container Access Patterns
When generating array or container access code:

```cpp
template<typename T>
class DynamicArray
{
public:
    T& operator[](size_t index)
    {
        SEDX_CORE_ASSERT(index < m_Size, "Index {} out of bounds (size: {})", index, m_Size);
        return m_Data[index];
    }
    
    void Resize(size_t newSize)
    {
        SEDX_CORE_ASSERT(newSize > 0, "Array size must be greater than zero");
        SEDX_CORE_VERIFY(newSize <= MAX_ARRAY_SIZE, "Requested size {} exceeds maximum {}", 
                         newSize, MAX_ARRAY_SIZE);
        
        // Resize implementation
    }
};
```

## Message Formatting Guidelines

### Descriptive Messages with Context
```cpp
// ✅ Good: Provides context and values
SEDX_CORE_ASSERT(texture != nullptr, "Failed to load texture from path: {}", texturePath);
SEDX_CORE_ASSERT(index < size, "Array index {} out of bounds (size: {})", index, size);

// ❌ Avoid: Vague or generic messages
SEDX_CORE_ASSERT(ptr != nullptr, "Null pointer");
SEDX_CORE_ASSERT(result == VK_SUCCESS, "Vulkan operation failed");
```

### State Information
```cpp
// ✅ Good: Includes object/module state
SEDX_ASSERT(m_IsInitialized, "Module '{}' operation attempted before initialization", GetName());
SEDX_CORE_ASSERT(m_RefCount > 0, "Reference count is {} but should be positive", m_RefCount);
```

## Integration with Existing Systems

### With Logging System
- Assertions automatically use the appropriate logger (Core/Editor)
- File and line information is included automatically
- Messages support std::format-style formatting
- No need to manually add SEDX_CORE_ERROR calls alongside assertions

### With Smart Pointers
```cpp
// Validate smart pointer operations
auto resource = CreateRef<Resource>();
SEDX_CORE_ASSERT(resource != nullptr, "Failed to create resource");

// Validate before dereferencing
SEDX_CORE_ASSERT(m_Texture != nullptr, "Texture not loaded");
m_Texture->Bind();
```

### With Module Lifecycle
```cpp
// In OnAttach()
SEDX_CORE_ASSERT(InitializeCriticalSystems(), "Critical system initialization failed");
SEDX_CORE_VERIFY(InitializeOptionalSystems(), "Optional systems partially available");

// In OnUpdate()
SEDX_CORE_ASSERT(m_IsInitialized, "Update called on uninitialized module");

// In OnDetach()
SEDX_CORE_VERIFY(m_ActiveResources.empty(), "{} resources leaked", m_ActiveResources.size());
```

## Performance Considerations

### Debug vs Release Behavior
- `SEDX_CORE_ASSERT` and `SEDX_ASSERT` compile to no-ops in release builds
- `SEDX_CORE_VERIFY` and `SEDX_VERIFY` always execute but continue on failure
- Expensive validation should be wrapped in `#ifdef SEDX_DEBUG`

### Avoiding Side Effects
```cpp
// ❌ Bad: Side effects in assertions
SEDX_CORE_ASSERT(++counter < MAX_COUNT, "Counter overflow");

// ✅ Good: No side effects
counter++;
SEDX_CORE_ASSERT(counter < MAX_COUNT, "Counter {} exceeds maximum {}", counter, MAX_COUNT);
```

## Don't Forget

1. **Always include descriptive messages** with format arguments
2. **Use appropriate macro type** (Core vs Editor, Assert vs Verify)
3. **Validate parameters** at function entry points
4. **Check resource validity** before use
5. **Avoid side effects** in assertion conditions
6. **Include context information** in error messages
7. **Use VERIFY for graceful degradation** scenarios
8. **Combine with proper error handling** patterns

## Quick Reference

| Scenario | Macro to Use | Example |
|----------|-------------|---------|
| Critical engine validation | `SEDX_CORE_ASSERT` | `SEDX_CORE_ASSERT(ptr != nullptr, "Null pointer")` |
| Editor/UI validation | `SEDX_ASSERT` | `SEDX_ASSERT(index < size, "Index out of bounds")` |
| Non-fatal engine checks | `SEDX_CORE_VERIFY` | `SEDX_CORE_VERIFY(LoadOptional(), "Optional failed")` |
| Non-fatal editor checks | `SEDX_VERIFY` | `SEDX_VERIFY(config.valid, "Invalid config")` |

Follow these guidelines to generate robust, maintainable code that integrates seamlessly with the Scenery Editor X assert system.

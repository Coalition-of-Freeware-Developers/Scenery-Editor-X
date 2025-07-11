# GitHub Copilot Instructions: Scenery Editor X Logging System

## Quick Reference for AI Agents

This document provides specific instructions for GitHub Copilot and other AI coding assistants when working with the Scenery Editor X logging system. Follow these guidelines to ensure consistent, proper logging implementation.

## Core Principles

### 1. Always Use Tagged Logging
**✅ PREFERRED - Tagged logging macros:**
```cpp
SEDX_CORE_INFO_TAG("Renderer", "Vulkan device initialized: {}", deviceName);
EDITOR_WARN_TAG("AssetManager", "Asset not found: {}", assetPath);
```

**❌ AVOID - Untagged logging:**
```cpp
SEDX_CORE_INFO("Vulkan device initialized: {}", deviceName);  // No tag context
```

### 2. Choose Appropriate Logger Types
- **SEDX_CORE_*_TAG()**: Engine internals, system operations, Vulkan/graphics, memory management
- **EDITOR_*_TAG()**: UI operations, user interactions, editor tools, project management
- **LAUNCHER_*_TAG()**: Application startup, launcher operations, pre-initialization

### 3. Use Meaningful Tag Names
Create descriptive, hierarchical tags that clearly indicate the subsystem:

**✅ GOOD:**
```cpp
SEDX_CORE_INFO_TAG("Renderer::Vulkan", "Creating command buffer pool");
SEDX_CORE_WARN_TAG("AssetLoader::GLTF", "Missing texture reference: {}", textureName);
EDITOR_INFO_TAG("UI::Viewport", "Camera position updated: {}", cameraPos);
```

**❌ AVOID:**
```cpp
SEDX_CORE_INFO_TAG("System", "Creating command buffer pool");  // Too generic
SEDX_CORE_WARN_TAG("Loader", "Missing texture");              // Insufficient detail
```

## Log Level Guidelines

### Trace Level (`TRACE_TAG`)
Use for very detailed debugging information:
```cpp
SEDX_CORE_TRACE_TAG("Memory", "Allocating {} bytes at address {}", size, ptr);
SEDX_CORE_TRACE_TAG("Renderer", "Binding descriptor set {} to pipeline {}", setIndex, pipelineId);
EDITOR_TRACE_TAG("UI", "Mouse moved to ({}, {})", mouseX, mouseY);
```

### Info Level (`INFO_TAG`)
Use for important state changes and successful operations:
```cpp
SEDX_CORE_INFO_TAG("AssetManager", "Loaded texture: {} ({}x{})", path, width, height);
SEDX_CORE_INFO_TAG("Vulkan", "Created logical device: {}", deviceName);
EDITOR_INFO_TAG("Project", "Project saved: {}", projectPath);
```

### Warning Level (`WARN_TAG`)
Use for recoverable issues, deprecated usage, fallback behavior:
```cpp
SEDX_CORE_WARN_TAG("Renderer", "Extension {} not supported, using fallback", extensionName);
SEDX_CORE_WARN_TAG("AssetManager", "Asset {} not in cache, loading from disk", assetId);
EDITOR_WARN_TAG("Project", "Project has unsaved changes");
```

### Error Level (`ERROR_TAG`)
Use for failed operations that don't crash the application:
```cpp
SEDX_CORE_ERROR_TAG("FileSystem", "Failed to open file: {} (error: {})", filename, GetLastError());
SEDX_CORE_ERROR_TAG("Vulkan", "Buffer creation failed: {}", VkResultToString(result));
EDITOR_ERROR_TAG("Serialization", "Failed to save scene: {}", errorMessage);
```

### Fatal Level (`FATAL_TAG`)
Use for critical errors that may cause application termination:
```cpp
SEDX_CORE_FATAL_TAG("Memory", "Out of memory: failed to allocate {} bytes", size);
SEDX_CORE_FATAL_TAG("Vulkan", "Device lost: cannot continue rendering");
EDITOR_FATAL_TAG("Application", "Critical configuration error: {}", errorDetails);
```

## Standard Tag Names

Use these established tag names for consistency:

### Core Engine Tags
- `"Renderer"` - General rendering operations
- `"Renderer::Vulkan"` - Vulkan-specific operations
- `"Memory"` - Memory allocation/deallocation
- `"AssetManager"` - Asset loading and management
- `"AssetLoader::GLTF"` - GLTF asset loading
- `"AssetLoader::OBJ"` - OBJ asset loading
- `"FileSystem"` - File operations
- `"Core"` - Core engine operations
- `"Physics"` - Physics system
- `"Animation"` - Animation system
- `"Audio"` - Audio system

### Editor Tags
- `"UI"` - General UI operations
- `"UI::Viewport"` - 3D viewport operations
- `"UI::Menu"` - Menu interactions
- `"UI::Toolbar"` - Toolbar operations
- `"Project"` - Project management
- `"Scene"` - Scene operations
- `"Tools"` - Editor tools
- `"Serialization"` - Save/load operations

### System Tags
- `"Application"` - Application lifecycle
- `"Config"` - Configuration management
- `"Performance"` - Performance monitoring
- `"Network"` - Network operations
- `"Security"` - Security-related operations

## Formatting Guidelines

### 1. Provide Context in Messages
Always include relevant information:

**✅ GOOD:**
```cpp
SEDX_CORE_ERROR_TAG("Vulkan", "Failed to create buffer: size={}, usage={}, result={}",
                   bufferSize, usageFlags, VkResultToString(result));
```

**❌ INSUFFICIENT:**
```cpp
SEDX_CORE_ERROR_TAG("Vulkan", "Buffer creation failed");
```

### 2. Use Consistent Formatting Patterns

**For resource operations:**
```cpp
SEDX_CORE_INFO_TAG("AssetManager", "Loading asset: {}", assetPath);
SEDX_CORE_INFO_TAG("AssetManager", "Loaded asset: {} ({} KB in {} ms)", assetPath, sizeKB, timeMs);
SEDX_CORE_ERROR_TAG("AssetManager", "Failed to load asset: {} - {}", assetPath, errorMessage);
```

**For system initialization:**
```cpp
SEDX_CORE_INFO_TAG("Renderer", "Initializing Vulkan renderer");
SEDX_CORE_INFO_TAG("Renderer", "Vulkan renderer initialized: {} ({})", deviceName, driverVersion);
```

**For state changes:**
```cpp
EDITOR_INFO_TAG("Application", "State transition: {} -> {}", oldState, newState);
```

### 3. Include Units and Scale
Be explicit about measurements:

**✅ GOOD:**
```cpp
SEDX_CORE_INFO_TAG("Memory", "Allocated {} bytes ({} MB)", size, size / (1024 * 1024));
SEDX_CORE_TRACE_TAG("Performance", "Frame rendered in {} microseconds", duration);
```

## Code Patterns

### 1. Error Checking with Logging
```cpp
VkResult result = vkCreateBuffer(device, &createInfo, nullptr, &buffer);
if (result != VK_SUCCESS) {
    SEDX_CORE_ERROR_TAG("Vulkan", "Failed to create buffer: size={}, result={}",
                       createInfo.size, VkResultToString(result));
    return false;
}
SEDX_CORE_TRACE_TAG("Vulkan", "Created buffer: {} bytes", createInfo.size);
```

### 2. Resource Loading Pattern
```cpp
bool LoadTexture(const std::string& path) {
    SEDX_CORE_TRACE_TAG("AssetManager", "Loading texture: {}", path);
    
    if (!std::filesystem::exists(path)) {
        SEDX_CORE_ERROR_TAG("AssetManager", "Texture file not found: {}", path);
        return false;
    }
    
    auto result = LoadTextureData(path);
    if (!result.success) {
        SEDX_CORE_ERROR_TAG("AssetManager", "Failed to load texture: {} - {}", 
                           path, result.errorMessage);
        return false;
    }
    
    SEDX_CORE_INFO_TAG("AssetManager", "Loaded texture: {} ({}x{}, {} bytes)",
                       path, result.width, result.height, result.dataSize);
    return true;
}
```

### 3. Performance Monitoring Pattern
```cpp
void RenderFrame() {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    SEDX_CORE_TRACE_TAG("Renderer", "Beginning frame render");
    
    // Render operations...
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    
    if (Log::HasTag("Performance")) {
        SEDX_CORE_TRACE_TAG("Performance", "Frame rendered in {} microseconds", duration.count());
    }
}
```

### 4. Conditional Expensive Logging
```cpp
void ProcessComplexData(const DataSet& data) {
    SEDX_CORE_TRACE_TAG("DataProcessor", "Processing dataset with {} elements", data.size());
    
    // Only generate expensive debug info if detailed logging is enabled
    if (Log::HasTag("DataProcessorDetails") && 
        Log::EnabledTags()["DataProcessorDetails"].LevelFilter <= Log::Level::Trace) {
        
        std::string detailedReport = GenerateDetailedReport(data);
        SEDX_CORE_TRACE_TAG("DataProcessorDetails", "Dataset analysis: {}", detailedReport);
    }
    
    // Continue processing...
}
```

## Assertion Integration

When using assertions, they automatically integrate with the logging system:

```cpp
// Basic assertions
SEDX_CORE_ASSERT(ptr != nullptr, "Pointer cannot be null");
SEDX_ASSERT(index < vector.size(), "Index {} out of bounds (size: {})", index, vector.size());

// Vulkan error checking
SEDX_CORE_ASSERT(result == VK_SUCCESS, "Vulkan operation failed: {}", VkResultToString(result));

// Resource validation
SEDX_VERIFY(texture.IsValid(), "Texture validation failed: {}", texture.GetPath());
```

## Anti-Patterns to Avoid

### 1. Don't Log in Tight Loops Without Conditions
**❌ AVOID:**
```cpp
for (const auto& vertex : vertices) {
    SEDX_CORE_TRACE_TAG("Mesh", "Processing vertex: {}", vertex.position);  // Too verbose!
    ProcessVertex(vertex);
}
```

**✅ BETTER:**
```cpp
SEDX_CORE_TRACE_TAG("Mesh", "Processing {} vertices", vertices.size());
for (const auto& vertex : vertices) {
    ProcessVertex(vertex);
}
```

### 2. Don't Use Generic Tags
**❌ AVOID:**
```cpp
SEDX_CORE_INFO_TAG("System", "Something happened");
SEDX_CORE_ERROR_TAG("Error", "Something failed");
```

**✅ BETTER:**
```cpp
SEDX_CORE_INFO_TAG("Renderer", "Swap chain created successfully");
SEDX_CORE_ERROR_TAG("AssetManager", "Failed to load model file");
```

### 3. Don't Mix Log Levels Inappropriately
**❌ AVOID:**
```cpp
SEDX_CORE_FATAL_TAG("AssetManager", "Asset not found in cache");  // Not fatal!
SEDX_CORE_TRACE_TAG("Application", "Application crashed");        // Should be fatal!
```

### 4. Don't Forget Error Context
**❌ AVOID:**
```cpp
SEDX_CORE_ERROR_TAG("FileSystem", "File operation failed");
```

**✅ BETTER:**
```cpp
SEDX_CORE_ERROR_TAG("FileSystem", "Failed to open file: {} (error: {}, path: {})",
                   filename, std::strerror(errno), std::filesystem::current_path().string());
```

## Special Considerations

### 1. Vulkan Logging
For Vulkan operations, always include the VkResult and operation context:
```cpp
VkResult result = vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);
if (result != VK_SUCCESS) {
    SEDX_CORE_ERROR_TAG("Vulkan", "Command pool creation failed: family={}, result={}",
                       poolInfo.queueFamilyIndex, VkResultToString(result));
}
```

### 2. Memory Operations
Include size and address information where relevant:
```cpp
SEDX_CORE_TRACE_TAG("Memory", "Allocating {} bytes for {}", size, category);
SEDX_CORE_TRACE_TAG("Memory", "Allocated {} bytes at {} for {}", size, ptr, category);
SEDX_CORE_TRACE_TAG("Memory", "Deallocating {} bytes at {} from {}", size, ptr, category);
```

### 3. Performance-Critical Code
Use conditional logging in performance-critical sections:
```cpp
if (Log::HasTag("Performance")) {
    auto start = std::chrono::high_resolution_clock::now();
    // ... operation ...
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    SEDX_CORE_TRACE_TAG("Performance", "Operation completed in {} μs", duration.count());
}
```

## Integration Checklist

When adding logging to new code, ensure:

- [ ] Used tagged logging macros (`*_TAG`)
- [ ] Chosen appropriate log level (Trace/Info/Warn/Error/Fatal)
- [ ] Selected correct logger type (SEDX_CORE/EDITOR/LAUNCHER)
- [ ] Used descriptive, hierarchical tag names
- [ ] Included sufficient context in messages
- [ ] Added error logging for failure paths
- [ ] Added success logging for important operations
- [ ] Used conditional logging for expensive operations
- [ ] Followed consistent formatting patterns
- [ ] Included units and scale where appropriate

## Quick Template

Use this template for new functions:

```cpp
ReturnType FunctionName(parameters) {
    LOGGER_TRACE_TAG("Subsystem", "Starting operation: {}", operationDetails);
    
    // Validate inputs
    if (!ValidateInputs()) {
        LOGGER_ERROR_TAG("Subsystem", "Invalid input parameters: {}", inputDetails);
        return ErrorValue;
    }
    
    // Perform operation
    auto result = PerformOperation();
    if (!result.success) {
        LOGGER_ERROR_TAG("Subsystem", "Operation failed: {} - {}", 
                        operationDetails, result.errorMessage);
        return ErrorValue;
    }
    
    LOGGER_INFO_TAG("Subsystem", "Operation completed successfully: {}", result.details);
    return result.value;
}
```

Replace `LOGGER` with appropriate logger type (SEDX_CORE/EDITOR/LAUNCHER) and `Subsystem` with appropriate tag name.

Following these guidelines ensures consistent, useful logging throughout the Scenery Editor X codebase that aids in debugging, monitoring, and maintenance.

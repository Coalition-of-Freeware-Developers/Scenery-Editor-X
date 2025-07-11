# Scenery Editor X - Logging System Documentation

---

## Overview

The Scenery Editor X logging system provides a comprehensive, multi-channel logging solution built on top of the `spdlog` library. This system is designed to handle logging for different components of the application (Core, Editor, Launcher) with configurable tag-based filtering, multiple output sinks, and integration with assertion systems.

## Architecture

### Core Components

1. **Log Class** (`SceneryEditorX::Log`): Static interface for logging operations
2. **Multiple Logger Types**: Core, Editor, EditorConsole, and Launcher loggers
3. **Tag-based Filtering**: Dynamic filtering system with per-tag configuration
4. **Multi-sink Output**: Console and file output with customizable patterns
5. **Integration with Assertions**: Seamless integration with the assertion system

### Logger Types

#### Core Logger (`CoreLogger`)

- **Purpose**: Core application and engine logging
- **Usage**: Internal engine operations, system initialization, critical errors
- **File Output**: `../logs/SceneryEditorX.log`
- **Console Output**: Colored console output with timestamps

#### Editor Logger (`EditorLogger`)

- **Purpose**: Editor-specific functionality and user interactions
- **Usage**: UI operations, editor tools, user-facing messages
- **File Output**: `../logs/SceneryEditorX.log` (shared with Core)
- **Console Output**: Colored console output with timestamps

#### Editor Console Logger (`EditorConsoleLogger`)

- **Purpose**: In-editor debug console output
- **Usage**: Debug information displayed within the editor's UI console
- **File Output**: `../logs/EditorConsoleOut.log`
- **Console Output**: Raw output without timestamps for UI display

#### Launcher Logger (`LauncherLogger`)

- **Purpose**: Application launcher and startup logging
- **Usage**: Launcher operations, application startup, pre-initialization
- **File Output**: `../logs/Launcher.log`
- **Console Output**: Colored console output with timestamps

## Logging Levels

The system supports five logging levels in order of severity:

1. **Trace**: Detailed debugging information
2. **Info**: General informational messages
3. **Warn**: Warning messages for potential issues
4. **Error**: Error conditions that don't halt execution
5. **Fatal**: Critical errors that may cause application termination

## Tag-based Filtering System

### Overview

The tag system allows fine-grained control over what gets logged from different subsystems. Each tag can be individually enabled/disabled and have its own minimum log level.

### Default Tags Configuration

```cpp
// Example of default tag configuration
std::map<std::string, Log::TagDetails> DefaultTagDetails_ = {
    {"Animation",       TagDetails{true, Level::Warn}},   // Only warnings and above
    {"AssetManager",    TagDetails{true, Level::Info}},   // Info level and above
    {"Memory",          TagDetails{true, Level::Error}},  // Only errors and critical
    {"Timer",           TagDetails{false, Level::Trace}}, // Disabled entirely
    {"Renderer",        TagDetails{true, Level::Info}},   // Info level and above
};
```

### TagDetails Structure

```cpp
struct TagDetails {
    bool Enabled = true;        // Whether the tag is enabled for logging
    Level LevelFilter = Level::Trace;  // Minimum log level for this tag
};
```

## Usage Examples

### Basic Logging (Without Tags)

```cpp
// Core logging - for engine internals
SEDX_CORE_TRACE("Initializing subsystem with {} parameters", paramCount);
SEDX_CORE_INFO("Vulkan device created successfully");
SEDX_CORE_WARN("Deprecated function called: {}", functionName);
SEDX_CORE_ERROR("Failed to load texture: {}", texturePath);
SEDX_CORE_FATAL("Critical memory allocation failed");

// Editor logging - for editor-specific operations
EDITOR_TRACE("Loading editor preferences");
EDITOR_INFO("Scene loaded: {}", sceneName);
EDITOR_WARN("Unsaved changes detected");
EDITOR_ERROR("Failed to save project: {}", errorMsg);
EDITOR_FATAL("Editor configuration corrupted");

// Launcher logging - for application startup
LAUNCHER_CORE_TRACE("Checking system requirements");
LAUNCHER_CORE_INFO("Application launched successfully");
LAUNCHER_CORE_WARN("Legacy configuration detected");
LAUNCHER_CORE_ERROR("Failed to initialize launcher: {}", error);
```

### Tagged Logging (Recommended)

```cpp
// Renderer subsystem logging
SEDX_CORE_TRACE_TAG("Renderer", "Creating render pass with {} attachments", attachmentCount);
SEDX_CORE_INFO_TAG("Renderer", "Vulkan renderer initialized");
SEDX_CORE_WARN_TAG("Renderer", "Falling back to software rendering");
SEDX_CORE_ERROR_TAG("Renderer", "Failed to create buffer: {}", VkResultToString(result));

// Asset management logging
SEDX_CORE_INFO_TAG("AssetManager", "Loading asset: {}", assetPath);
SEDX_CORE_WARN_TAG("AssetManager", "Asset not found in cache: {}", assetId);
SEDX_CORE_ERROR_TAG("AssetManager", "Failed to load asset: {}", errorDetails);

// Memory management logging
SEDX_CORE_TRACE_TAG("Memory", "Allocating {} bytes", size);
SEDX_CORE_ERROR_TAG("Memory", "Memory leak detected: {} bytes", leakedBytes);

// Scene management
EDITOR_INFO_TAG("Scene", "Creating new scene: {}", sceneName);
EDITOR_WARN_TAG("Scene", "Scene has unsaved changes");
EDITOR_ERROR_TAG("Scene", "Failed to serialize scene: {}", error);
```

### Advanced Usage Patterns

#### Conditional Logging

```cpp
// Check if a tag is enabled before expensive operations
if (Log::HasTag("Performance")) {
    auto startTime = std::chrono::high_resolution_clock::now();
    // ... perform operation ...
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    SEDX_CORE_TRACE_TAG("Performance", "Operation completed in {} microseconds", duration.count());
}
```

#### Dynamic Tag Configuration

```cpp
// Enable/disable tags at runtime
auto& tags = Log::EnabledTags();
tags["Debug"] = {true, Log::Level::Trace};  // Enable debug tag with trace level
tags["Network"] = {false, Log::Level::Info}; // Disable network tag

// Modify existing tag settings
if (Log::HasTag("Renderer")) {
    auto& rendererTag = tags["Renderer"];
    rendererTag.LevelFilter = Log::Level::Error; // Only show errors and above
}
```

#### Vulkan-specific Logging

```cpp
// The system provides special handling for Vulkan debug output
void vulkanDebugCallback(/* vulkan debug parameters */) {
    std::string message = formatVulkanMessage(/* parameters */);
    Log::LogVulkanDebug(message); // Automatically categorizes by severity
}

// Vulkan result logging
VkResult result = vkCreateBuffer(/* parameters */);
Log::LogVulkanResult(result, "vkCreateBuffer"); // Logs success/failure appropriately
```

#### Editor Console Integration

```cpp
// Direct logging to the in-editor console (bypasses tag filtering)
EDITOR_CONSOLE_LOG_INFO("Build completed successfully");
EDITOR_CONSOLE_LOG_WARN("Build completed with {} warnings", warningCount);
EDITOR_CONSOLE_LOG_ERROR("Build failed: {}", errorMessage);
```

## Integration with Assertion System

The logging system is tightly integrated with the assertion system defined in `asserts.h`:

### Assert Macros

```cpp
// Core assertions (for engine code)
SEDX_CORE_ASSERT(ptr != nullptr, "Pointer cannot be null");
SEDX_CORE_VERIFY(result == VK_SUCCESS, "Vulkan operation failed: {}", VkResultToString(result));

// Editor assertions (for editor code)
SEDX_ASSERT(index < vector.size(), "Index {} out of bounds (size: {})", index, vector.size());
SEDX_VERIFY(file.is_open(), "Failed to open file: {}", filename);
```

### Assert vs Verify

- **Assert**: Only active in debug builds (`SEDX_DEBUG`)
- **Verify**: Always active (can be disabled with `SEDX_ENABLE_VERIFY`)

Both use the logging system to output formatted error messages before triggering a debug break.

## File Organization and Output

### Log File Structure

```
logs/
├── SceneryEditorX.log      # Core and Editor output
├── EditorConsoleOut.log    # Editor console output
└── Launcher.log            # Launcher output
```

### Log File Patterns

- **File Pattern**: `[timestamp] [level] logger_name: message`
- **Console Pattern**: `[timestamp] logger_name: message` (with colors)
- **Editor Console Pattern**: `message` (raw output for UI display)

### Example Log Output

```
[14:32:15] [info] Core: Vulkan renderer initialized
[14:32:15] [warn] Core: [Renderer] Falling back to software rendering
[14:32:16] [error] SceneryEditorX: [AssetManager] Failed to load texture: file_not_found.png
[14:32:16] [critical] Core: Critical memory allocation failed
```

## Best Practices

### 1. Use Tagged Logging

Always prefer tagged logging macros over untagged ones:

```cpp
// ✅ Good - tagged logging
SEDX_CORE_INFO_TAG("Renderer", "Initializing Vulkan with {} extensions", extensionCount);

// ❌ Avoid - untagged logging
SEDX_CORE_INFO("Initializing Vulkan with {} extensions", extensionCount);
```

### 2. Choose Appropriate Log Levels

- **Trace**: Very detailed debugging information, performance metrics
- **Info**: Important state changes, successful operations
- **Warn**: Recoverable issues, deprecated usage, fallback behavior
- **Error**: Failed operations that don't crash the application
- **Fatal**: Critical errors that may cause application termination

### 3. Use Meaningful Tags

Create descriptive, hierarchical tags:

```cpp
// ✅ Good - specific, descriptive tags
SEDX_CORE_INFO_TAG("AssetLoader::GLTF", "Loading GLTF model: {}", path);
SEDX_CORE_WARN_TAG("Renderer::Vulkan", "Extension not supported: {}", extension);

// ❌ Avoid - vague tags
SEDX_CORE_INFO_TAG("System", "Loading model: {}", path);
```

### 4. Format Messages Consistently

Use consistent formatting patterns:

```cpp
// ✅ Good - consistent formatting
SEDX_CORE_INFO_TAG("AssetManager", "Loading asset: {} (type: {})", assetPath, assetType);
SEDX_CORE_ERROR_TAG("AssetManager", "Failed to load asset: {} - {}", assetPath, errorMessage);

// ❌ Inconsistent formatting
SEDX_CORE_INFO_TAG("AssetManager", "Loading: {}", assetPath);
SEDX_CORE_ERROR_TAG("AssetManager", "Asset load failure - {} ({})", errorMessage, assetPath);
```

### 5. Performance Considerations

- Avoid expensive operations in trace-level logs in release builds
- Use conditional logging for performance-critical sections
- Prefer compile-time format strings when possible

```cpp
// ✅ Good - conditional expensive logging
if (Log::HasTag("Performance")) {
    std::string expensiveDebugInfo = generateDetailedReport();
    SEDX_CORE_TRACE_TAG("Performance", "Debug report: {}", expensiveDebugInfo);
}

// ❌ Avoid - always computing expensive data
std::string expensiveDebugInfo = generateDetailedReport(); // Always computed!
SEDX_CORE_TRACE_TAG("Performance", "Debug report: {}", expensiveDebugInfo);
```

### 6. Error Context

Provide sufficient context in error messages:

```cpp
// ✅ Good - detailed error context
SEDX_CORE_ERROR_TAG("FileSystem", "Failed to open file: {} (error: {}, path: {})", 
                    filename, GetLastError(), std::filesystem::current_path().string());

// ❌ Insufficient context
SEDX_CORE_ERROR_TAG("FileSystem", "File open failed");
```

## Initialization and Shutdown

### System Initialization

```cpp
int main() {
    // Initialize logging system early in application startup
    SceneryEditorX::Log::Init();
  
    // Log system information
    SceneryEditorX::Log::LogHeader();
  
    // Your application code here...
  
    // Shutdown logging system before exit
    SceneryEditorX::Log::ShutDown();
    return 0;
}
```

### Custom Tag Configuration

```cpp
void configureLoggingTags() {
    auto& tags = SceneryEditorX::Log::EnabledTags();
  
    // Enable detailed renderer logging in debug builds
    #ifdef SEDX_DEBUG
    tags["Renderer"] = {true, SceneryEditorX::Log::Level::Trace};
    tags["Memory"] = {true, SceneryEditorX::Log::Level::Trace};
    #else
    tags["Renderer"] = {true, SceneryEditorX::Log::Level::Info};
    tags["Memory"] = {true, SceneryEditorX::Log::Level::Error};
    #endif
  
    // Disable verbose subsystems in release
    #ifdef SEDX_RELEASE
    tags["Animation"] = {false, SceneryEditorX::Log::Level::Trace};
    tags["Timer"] = {false, SceneryEditorX::Log::Level::Trace};
    #endif
}
```

## Thread Safety

The logging system is thread-safe thanks to the underlying `spdlog` library. All logger instances use multi-threaded sinks (`_mt` suffix) and can be safely called from multiple threads concurrently.

```cpp
// Safe to call from any thread
void workerThreadFunction() {
    SEDX_CORE_INFO_TAG("Worker", "Thread {} starting work", std::this_thread::get_id());
    // ... work ...
    SEDX_CORE_INFO_TAG("Worker", "Thread {} completed work", std::this_thread::get_id());
}
```

## Memory Management

The logging system uses smart pointers (`std::shared_ptr`) for logger management and automatically handles resource cleanup during shutdown. The system is designed to be memory-efficient and avoid leaks.

## Error Handling

The system includes robust error handling for initialization failures and runtime issues:

```cpp
void Log::Init() {
    try {
        // Logger initialization...
    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Log initialization failed: " << ex.what() << '\n';
    }
}
```

## Platform Considerations

The system is designed to work across multiple platforms:

- **Windows**: Supports Windows-specific debugging features and message boxes for assertions
- **Linux/Mac**: Uses appropriate debug break mechanisms for each platform
- **Multi-platform**: File paths and system information adapt to the target platform

## Future Extensions

The logging system is designed to be extensible:

- Additional logger types can be easily added
- New output sinks (network, database, etc.) can be integrated
- Custom formatters can be implemented
- Additional metadata (thread ID, process ID, etc.) can be added to log patterns

# Scenery Editor X - Logging System API Reference

---

## Class: `SceneryEditorX::Log`

The `Log` class provides the primary interface for all logging operations in the Scenery Editor X application. This static class manages multiple logger instances and provides formatted logging with tag-based filtering.

### Public Types

#### `enum class Type : uint8_t`

Defines the different logger types available in the system.

**Values:**

- `Core = 0`: Core engine and system-level logging
- `Editor = 1`: Editor-specific user interface and tool logging
- `Launcher = 2`: Application launcher and initialization logging

**Usage Example:**

```cpp
// Used internally by logging macros
Log::PrintMessage(Log::Type::Core, Log::Level::Info, "System initialized");
Log::PrintMessage(Log::Type::Editor, Log::Level::Warn, "Unsaved changes detected");
Log::PrintMessage(Log::Type::Launcher, Log::Level::Error, "Failed to start application");
```

#### `enum class Level : uint8_t`

Defines the severity levels for log messages, from most verbose to most critical.

**Values:**

- `Trace = 0`: Detailed debugging information, function entry/exit, variable values
- `Info = 1`: General informational messages about program flow and state
- `Warn = 2`: Warning messages for recoverable issues or deprecated usage
- `Error = 3`: Error conditions that don't prevent continued execution
- `Fatal = 4`: Critical errors that may cause application termination

**Usage Example:**

```cpp
// Setting tag level filters
TagDetails details{true, Log::Level::Warn}; // Only warnings and above
EnabledTags()["MySubsystem"] = details;

// Converting between string and enum
Log::Level level = Log::LevelFromString("Error"); // Returns Level::Error
const char* levelStr = Log::LevelToString(Level::Info); // Returns "Info"
```

#### `struct TagDetails`

Configuration structure for individual logging tags.

**Members:**

- `bool Enabled = true`: Whether logging is enabled for this tag
- `Level LevelFilter = Level::Trace`: Minimum log level that will be output

**Usage Example:**

```cpp
// Configure renderer logging to only show warnings and errors
TagDetails rendererConfig;
rendererConfig.Enabled = true;
rendererConfig.LevelFilter = Log::Level::Warn;
Log::EnabledTags()["Renderer"] = rendererConfig;

// Disable timer logging entirely
TagDetails timerConfig;
timerConfig.Enabled = false;
Log::EnabledTags()["Timer"] = timerConfig;
```

### Public Static Methods

#### `static void Init()`

Initializes the logging system with all logger instances and their configured sinks.

**Behavior:**

- Creates console and file sinks for each logger type
- Sets up appropriate formatting patterns for each sink type
- Registers all loggers with spdlog
- Configures default tag settings
- Handles initialization errors gracefully

**File Outputs Created:**

- `../logs/SceneryEditorX.log`: Core and Editor logging
- `../logs/EditorConsoleOut.log`: Editor console output
- `../logs/Launcher.log`: Launcher logging

**Usage Example:**

```cpp
int main() {
    // Initialize logging system before any other operations
    SceneryEditorX::Log::Init();
  
    // Now safe to use logging macros
    SEDX_CORE_INFO("Application starting...");
  
    // ... rest of application ...
  
    return 0;
}
```

**Error Handling:**

```cpp
// Init() handles spdlog exceptions internally
try {
    Log::Init();
} catch (...) {
    // Init() already caught and logged spdlog exceptions
    // Additional error handling if needed
}
```

#### `static void LogHeader()`

Outputs comprehensive system information to the core logger.

**Information Logged:**

- Operating system name and version
- Current date and time with timezone
- Processor architecture and core count
- Memory page size and addressing limits
- Application version and build information
- Copyright and company information

**Usage Example:**

```cpp
int main() {
    Log::Init();
    Log::LogHeader(); // Log system information at startup
  
    // Typical output:
    // ============================================
    // System Information
    // Operating System: Windows 64-bit
    // 14:32:15 10/07/2025
    // Time Zone: Eastern Standard Time
    // Processor Architecture: 9
    // Processor Cores: 8
    // ============================================
  
    return 0;
}
```

#### `static void ShutDown()`

Properly shuts down the logging system and flushes all pending log messages.

**Behavior:**

- Flushes all pending messages to files and console
- Drops all registered loggers from spdlog
- Resets all shared_ptr logger instances
- Calls spdlog::shutdown() for final cleanup

**Usage Example:**

```cpp
int main() {
    Log::Init();
  
    // ... application code ...
  
    // Ensure all logs are written before exit
    Log::ShutDown();
    return 0;
}
```

**Critical Notes:**

- Must be called before application exit to ensure log integrity
- Failure to call may result in lost log messages
- Safe to call multiple times

#### `static void LogVulkanDebug(const std::string& message)`

Specialized logging function for Vulkan debug layer output.

**Parameters:**

- `message`: Pre-formatted Vulkan debug message from validation layers

**Behavior:**

- Automatically parses message severity markers ([ERROR], [WARNING], etc.)
- Routes messages to appropriate log levels based on content
- Provides immediate flushing for critical Vulkan errors
- Handles performance-related messages with special formatting

**Usage Example:**

```cpp
// Vulkan debug callback implementation
VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
  
    // Format message with severity and type information
    std::string formattedMessage = fmt::format("[{}] [{}] {}", 
        severityToString(messageSeverity),
        typeToString(messageType),
        pCallbackData->pMessage);
  
    // Let the logging system handle severity routing
    SceneryEditorX::Log::LogVulkanDebug(formattedMessage);
  
    return VK_FALSE;
}
```

**Severity Parsing:**

```cpp
// Message content determines log level:
Log::LogVulkanDebug("[ERROR] Validation error found");     // -> error level
Log::LogVulkanDebug("[WARNING] Performance warning");      // -> warn level  
Log::LogVulkanDebug("[INFO] Device capability info");      // -> info level
Log::LogVulkanDebug("[VERBOSE] Detailed state info");      // -> trace level
Log::LogVulkanDebug("General Vulkan message");             // -> trace level (default)
```

#### `static void LogVulkanResult(VkResult result, const std::string& operation)`

Logs Vulkan API function results with appropriate severity levels.

**Parameters:**

- `result`: VkResult return value from Vulkan API call
- `operation`: Description of the operation that was performed

**Behavior:**

- Maps VkResult values to appropriate log levels
- Negative results (errors) logged as errors
- Positive results (warnings) logged as warnings
- VK_SUCCESS optionally logged at trace level
- Includes human-readable VkResult string conversion

**Usage Example:**

```cpp
// Manual result checking and logging
VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);
Log::LogVulkanResult(result, "vkCreateBuffer");

// Integrated with error checking macros
#define VK_CHECK_RESULT(call, operation) \
    do { \
        VkResult result = call; \
        Log::LogVulkanResult(result, operation); \
        if (result != VK_SUCCESS) { \
            SEDX_CORE_ERROR("Vulkan operation failed: {}", operation); \
            return false; \
        } \
    } while(0)

// Usage with macro
VK_CHECK_RESULT(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool), 
                "vkCreateCommandPool");
```

**Result Mapping:**

```cpp
// Error results (negative values) -> error level
VK_ERROR_OUT_OF_HOST_MEMORY  // -> Log error
VK_ERROR_DEVICE_LOST         // -> Log error

// Warning results (positive values) -> warn level  
VK_SUBOPTIMAL_KHR           // -> Log warning
VK_INCOMPLETE               // -> Log warning

// Success -> trace level (optional)
VK_SUCCESS                  // -> Log trace (if enabled)
```

### Logger Access Methods

#### `static std::shared_ptr<spdlog::logger>& GetCoreLogger()`

Returns reference to the core system logger instance.

**Usage Example:**

```cpp
// Direct logger access (not recommended - use macros instead)
auto& logger = Log::GetCoreLogger();
logger->info("Direct logger usage: {}", value);
logger->flush();

// Checking logger availability
if (Log::GetCoreLogger()) {
    // Logger is initialized and ready
    SEDX_CORE_INFO("Logger is available");
}
```

#### `static std::shared_ptr<spdlog::logger>& GetEditorLogger()`

Returns reference to the editor-specific logger instance.

#### `static std::shared_ptr<spdlog::logger>& GetEditorConsoleLogger()`

Returns reference to the editor console logger instance for UI display.

#### `static std::shared_ptr<spdlog::logger>& GetLauncherLogger()`

Returns reference to the launcher logger instance.

### Tag Management Methods

#### `static bool HasTag(const std::string& tag)`

Checks if a specific logging tag exists in the configuration.

**Parameters:**

- `tag`: Tag name to check for existence

**Returns:**

- `true` if tag exists in EnabledTags_ map
- `false` if tag does not exist

**Usage Example:**

```cpp
// Conditional logging based on tag existence
if (Log::HasTag("Performance")) {
    auto start = std::chrono::high_resolution_clock::now();
    performExpensiveOperation();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    SEDX_CORE_TRACE_TAG("Performance", "Operation took {} microseconds", duration.count());
}

// Tag-based feature enablement
bool shouldLogMemoryDetails = Log::HasTag("Memory") && 
                             Log::EnabledTags()["Memory"].LevelFilter <= Log::Level::Trace;
```

#### `static std::map<std::string, TagDetails>& EnabledTags()`

Provides access to the mutable tag configuration map.

**Returns:**

- Reference to the internal tag configuration map
- Allows runtime modification of tag settings

**Usage Example:**

```cpp
// Runtime tag configuration
auto& tags = Log::EnabledTags();

// Enable debug logging for specific subsystem
tags["Renderer"] = {true, Log::Level::Trace};

// Disable verbose subsystem in release builds
#ifdef SEDX_RELEASE
tags["Animation"] = {false, Log::Level::Trace};
tags["Timer"] = {false, Log::Level::Trace};
#endif

// Modify existing tag settings
if (tags.contains("AssetManager")) {
    tags["AssetManager"].LevelFilter = Log::Level::Warn;
}

// Add new custom tags
tags["CustomModule"] = {true, Log::Level::Info};
```

#### `static void SetDefaultTagSettings()`

Resets all tag configurations to their default values.

**Behavior:**

- Copies DefaultTagDetails_ to EnabledTags_
- Overwrites any runtime modifications
- Useful for resetting to known configuration state

**Usage Example:**

```cpp
// Reset to defaults after runtime modifications
Log::EnabledTags()["Debug"] = {true, Log::Level::Trace}; // Custom setting
Log::SetDefaultTagSettings(); // Resets to defaults
// "Debug" tag is now removed, all defaults restored
```

### Utility Methods

#### `static const char* LevelToString(Level level)`

Converts log level enum to string representation.

**Parameters:**

- `level`: Log level enum value to convert

**Returns:**

- String representation of the log level
- Returns empty string for invalid level values

**Usage Example:**

```cpp
// Converting levels to strings for UI display
Log::Level currentLevel = Log::Level::Warn;
const char* levelName = Log::LevelToString(currentLevel); // "Warn"

// Logging current configuration
for (const auto& [tag, details] : Log::EnabledTags()) {
    SEDX_CORE_INFO("Tag '{}': enabled={}, level={}", 
                   tag, details.Enabled, Log::LevelToString(details.LevelFilter));
}
```

#### `static Level LevelFromString(std::string_view string)`

Converts string to log level enum value.

**Parameters:**

- `string`: String representation of log level (case-sensitive)

**Returns:**

- Corresponding Level enum value
- Returns Level::Trace for unrecognized strings

**Valid Strings:**

- "Trace", "Info", "Warn", "Error", "Fatal"

**Usage Example:**

```cpp
// Configuration file parsing
std::string configLevel = "Error";
Log::Level level = Log::LevelFromString(configLevel); // Level::Error

// Command line argument processing
if (argc > 1) {
    Log::Level cmdLevel = Log::LevelFromString(argv[1]);
    auto& tags = Log::EnabledTags();
    tags["Runtime"] = {true, cmdLevel};
}

// Default handling for invalid input
Log::Level invalid = Log::LevelFromString("InvalidLevel"); // Returns Level::Trace
```

#### `static void FlushAll()`

Forces immediate flush of all logger instances to their outputs.

**Behavior:**

- Calls flush() on all non-null logger instances
- Ensures all pending messages are written to files/console
- Useful before critical operations or shutdown

**Usage Example:**

```cpp
// Flush before critical operations
SEDX_CORE_WARN("About to perform critical operation");
Log::FlushAll(); // Ensure warning is written immediately

// Periodic flushing in long-running operations
for (int i = 0; i < largeDataSet.size(); ++i) {
    processItem(largeDataSet[i]);
  
    if (i % 1000 == 0) {
        SEDX_CORE_INFO("Processed {} items", i);
        Log::FlushAll(); // Periodic flush to ensure log availability
    }
}
```

### Template Methods

#### `template<typename... Args> static void PrintMessage(...)`

Core message printing function used by logging macros.

**Template Parameters:**

- `Args...`: Variadic template for format arguments

**Parameters:**

- `type`: Logger type (Core, Editor, Launcher)
- `level`: Message severity level
- `format`: Format string (platform-specific format_string or string_view)
- `args...`: Arguments for format string

**Usage Example:**

```cpp
// Used internally by macros - direct usage not recommended
Log::PrintMessage(Log::Type::Core, Log::Level::Info, "Processing {} items", itemCount);

// Macro expansion of SEDX_CORE_INFO("Processing {} items", itemCount):
// ::SceneryEditorX::Log::PrintMessage(::SceneryEditorX::Log::Type::Core, 
//                                    ::SceneryEditorX::Log::Level::Info, 
//                                    "Processing {} items", itemCount)
```

#### `template<typename... Args> static void PrintMessageTag(...)`

Tagged message printing function used by tagged logging macros.

**Template Parameters:**

- `Args...`: Variadic template for format arguments

**Parameters:**

- `type`: Logger type (Core, Editor, Launcher)
- `level`: Message severity level
- `tag`: Tag name for filtering and message prefixing
- `format`: Format string template
- `args...`: Arguments for format string

**Usage Example:**

```cpp
// Used internally by tagged macros
Log::PrintMessageTag(Log::Type::Core, Log::Level::Info, "Renderer", 
                    "Created buffer with {} bytes", bufferSize);

// Macro expansion of SEDX_CORE_INFO_TAG("Renderer", "Created buffer with {} bytes", bufferSize):
// ::SceneryEditorX::Log::PrintMessageTag(::SceneryEditorX::Log::Type::Core,
//                                       ::SceneryEditorX::Log::Level::Info,
//                                       "Renderer",
//                                       "Created buffer with {} bytes", bufferSize)
```

#### `template<typename... Args> static void PrintAssertMessage(...)`

Specialized message printing for assertion failures.

**Template Parameters:**

- `Args...`: Variadic template for format arguments

**Parameters:**

- `type`: Logger type for categorizing the assertion
- `prefix`: Standard assertion prefix with file/line information
- `message`: Format string for assertion details
- `args...`: Arguments for format string

**Behavior:**

- Always logs at error level regardless of tag settings
- Shows message box on Windows debug builds
- Includes file and line information in the prefix

**Usage Example:**

```cpp
// Used internally by assertion macros
Log::PrintAssertMessage(Log::Type::Core, 
                       "Assertion Failed (file.cpp:123) ", 
                       "Buffer size {} exceeds maximum {}", 
                       actualSize, maxSize);

// SEDX_CORE_ASSERT macro expansion:
// if (!(condition)) {
//     Log::PrintAssertMessage(Log::Type::Core, 
//                           "Assertion Failed (" __FILE__ ":" STRINGIFY(__LINE__) ") ",
//                           __VA_ARGS__);
//     SEDX_DEBUG_BREAK;
// }
```

## Logging Macros Reference

### Core Logging Macros (Untagged)

#### Basic Core Macros

```cpp
SEDX_CORE_TRACE(format, ...);   // Trace level core logging
SEDX_CORE_INFO(format, ...);    // Info level core logging  
SEDX_CORE_WARN(format, ...);    // Warning level core logging
SEDX_CORE_ERROR(format, ...);   // Error level core logging
SEDX_CORE_FATAL(format, ...);   // Fatal level core logging
```

**Usage Examples:**

```cpp
SEDX_CORE_TRACE("Function entry: processData() with {} items", itemCount);
SEDX_CORE_INFO("Vulkan renderer initialized successfully");
SEDX_CORE_WARN("Using fallback rendering path due to missing extension");
SEDX_CORE_ERROR("Failed to allocate buffer: {}", GetLastError());
SEDX_CORE_FATAL("Critical system failure: cannot continue execution");
```

#### Basic Editor Macros

```cpp
EDITOR_TRACE(format, ...);      // Trace level editor logging
EDITOR_INFO(format, ...);       // Info level editor logging
EDITOR_WARN(format, ...);       // Warning level editor logging  
EDITOR_ERROR(format, ...);      // Error level editor logging
EDITOR_FATAL(format, ...);      // Fatal level editor logging
```

**Usage Examples:**

```cpp
EDITOR_TRACE("User clicked button: {}", buttonName);
EDITOR_INFO("Scene '{}' loaded successfully", sceneName);
EDITOR_WARN("Unsaved changes detected in current project");
EDITOR_ERROR("Failed to save project: {}", errorMessage);
EDITOR_FATAL("Editor configuration corrupted: cannot start");
```

#### Basic Launcher Macros

```cpp
LAUNCHER_CORE_TRACE(format, ...);   // Trace level launcher logging
LAUNCHER_CORE_INFO(format, ...);    // Info level launcher logging
LAUNCHER_CORE_WARN(format, ...);    // Warning level launcher logging
LAUNCHER_CORE_ERROR(format, ...);   // Error level launcher logging
LAUNCHER_CORE_FATAL(format, ...);   // Fatal level launcher logging
```

### Tagged Logging Macros (Recommended)

#### Core Tagged Macros

```cpp
SEDX_CORE_TRACE_TAG(tag, format, ...);  // Tagged trace core logging
SEDX_CORE_INFO_TAG(tag, format, ...);   // Tagged info core logging
SEDX_CORE_WARN_TAG(tag, format, ...);   // Tagged warning core logging
SEDX_CORE_ERROR_TAG(tag, format, ...);  // Tagged error core logging
SEDX_CORE_FATAL_TAG(tag, format, ...);  // Tagged fatal core logging
```

**Usage Examples:**

```cpp
SEDX_CORE_TRACE_TAG("Memory", "Allocated {} bytes at address {}", size, ptr);
SEDX_CORE_INFO_TAG("Renderer", "Vulkan device created: {}", deviceName);
SEDX_CORE_WARN_TAG("AssetManager", "Asset not found in cache: {}", assetId);
SEDX_CORE_ERROR_TAG("FileSystem", "Cannot open file: {} ({})", filename, errno);
SEDX_CORE_FATAL_TAG("Core", "Out of memory: failed to allocate {} bytes", size);
```

#### Editor Tagged Macros

```cpp
EDITOR_TRACE_TAG(tag, format, ...);     // Tagged trace editor logging
EDITOR_INFO_TAG(tag, format, ...);      // Tagged info editor logging
EDITOR_WARN_TAG(tag, format, ...);      // Tagged warning editor logging
EDITOR_ERROR_TAG(tag, format, ...);     // Tagged error editor logging
EDITOR_FATAL_TAG(tag, format, ...);     // Tagged fatal editor logging
```

**Usage Examples:**

```cpp
EDITOR_TRACE_TAG("UI", "Widget {} created with parent {}", widgetId, parentId);
EDITOR_INFO_TAG("Project", "Project '{}' opened successfully", projectName);
EDITOR_WARN_TAG("Tools", "Tool '{}' deprecated, use '{}' instead", oldTool, newTool);
EDITOR_ERROR_TAG("Serialization", "Failed to serialize object: {}", objName);
EDITOR_FATAL_TAG("UI", "Critical UI system failure: {}", errorDetails);
```

#### Launcher Tagged Macros

```cpp
LAUNCHER_TRACE_TAG(tag, format, ...);   // Tagged trace launcher logging
LAUNCHER_INFO_TAG(tag, format, ...);    // Tagged info launcher logging
LAUNCHER_WARN_TAG(tag, format, ...);    // Tagged warning launcher logging
LAUNCHER_ERROR_TAG(tag, format, ...);   // Tagged error launcher logging
LAUNCHER_FATAL_TAG(tag, format, ...);   // Tagged fatal launcher logging
```

### Direct Logger Access Macros

#### Editor Console Macros (UI Display)

```cpp
EDITOR_CONSOLE_LOG_TRACE(format, ...);  // Direct editor console trace
EDITOR_CONSOLE_LOG_INFO(format, ...);   // Direct editor console info
EDITOR_CONSOLE_LOG_WARN(format, ...);   // Direct editor console warning
EDITOR_CONSOLE_LOG_ERROR(format, ...);  // Direct editor console error
EDITOR_CONSOLE_LOG_FATAL(format, ...);  // Direct editor console fatal
```

**Usage Examples:**

```cpp
// These bypass tag filtering and go directly to the editor console
EDITOR_CONSOLE_LOG_INFO("Build completed successfully in {} seconds", buildTime);
EDITOR_CONSOLE_LOG_WARN("Build completed with {} warnings", warningCount);
EDITOR_CONSOLE_LOG_ERROR("Build failed: {}", errorMessage);
```

#### Direct Logger Macros

```cpp
EDITOR_LOG_TRACE(format, ...);      // Direct editor logger access
EDITOR_LOG_INFO(format, ...);       // Direct editor logger access
EDITOR_LOG_WARN(format, ...);       // Direct editor logger access
EDITOR_LOG_ERROR(format, ...);      // Direct editor logger access
EDITOR_LOG_CRITICAL(format, ...);   // Direct editor logger access

LAUNCHER_LOG_TRACE(format, ...);    // Direct launcher logger access
LAUNCHER_LOG_INFO(format, ...);     // Direct launcher logger access
LAUNCHER_LOG_WARN(format, ...);     // Direct launcher logger access
LAUNCHER_LOG_ERROR(format, ...);    // Direct launcher logger access
LAUNCHER_LOG_CRITICAL(format, ...); // Direct launcher logger access
```

## Integration with Assertion System

The logging system provides the backend for the assertion macros defined in `asserts.h`:

### Assert Macros

```cpp
SEDX_CORE_ASSERT(condition, message, ...);  // Core assertion with optional message
SEDX_ASSERT(condition, message, ...);       // Editor assertion with optional message
```

**Behavior:**

- Only active in debug builds (when `SEDX_DEBUG` is defined)
- Uses `Log::PrintAssertMessage()` for formatted output
- Triggers debug break after logging
- Shows message box on Windows debug builds

**Usage Examples:**

```cpp
// Basic assertions
SEDX_CORE_ASSERT(ptr != nullptr);
SEDX_CORE_ASSERT(index < vector.size(), "Index {} out of bounds", index);

// Complex assertions with context
SEDX_CORE_ASSERT(result == VK_SUCCESS, 
                 "Vulkan operation failed: {} (result: {})", 
                 operation, VkResultToString(result));

SEDX_ASSERT(file.is_open(), 
            "Failed to open file: {} (error: {})", 
            filename, std::strerror(errno));
```

### Verify Macros

```cpp
SEDX_CORE_VERIFY(condition, message, ...);  // Core verification with optional message
SEDX_VERIFY(condition, message, ...);       // Editor verification with optional message
```

**Behavior:**

- Always active (unless `SEDX_ENABLE_VERIFY` is disabled)
- Uses `Log::PrintAssertMessage()` for formatted output
- Triggers debug break after logging
- Useful for release build error checking

**Usage Examples:**

```cpp
// Critical checks that should always run
SEDX_CORE_VERIFY(allocatedMemory != nullptr, "Critical memory allocation failed");
SEDX_VERIFY(configurationValid, "Invalid configuration detected");

// Resource validation
SEDX_CORE_VERIFY(buffer.size > 0, "Buffer size must be greater than zero");
SEDX_VERIFY(textureLoaded, "Required texture '{}' failed to load", textureName);
```

## Best Practices Summary

### 1. Macro Selection Priority

1. **First Choice**: Tagged macros (`SEDX_CORE_INFO_TAG`, `EDITOR_WARN_TAG`, etc.)
2. **Second Choice**: Basic macros (`SEDX_CORE_INFO`, `EDITOR_WARN`, etc.)
3. **Avoid**: Direct logger access unless bypassing tag system is required

### 2. Tag Naming Conventions

- Use descriptive, hierarchical names: `"AssetLoader::GLTF"`, `"Renderer::Vulkan"`
- Be consistent within subsystems: `"UI::Menu"`, `"UI::Toolbar"`, `"UI::Viewport"`
- Use existing default tags when appropriate

### 3. Performance Considerations

- Check tag existence before expensive operations: `if (Log::HasTag("Performance"))`
- Avoid complex formatting in high-frequency trace logs
- Use conditional compilation for development-only logging

### 4. Error Reporting

- Provide sufficient context in error messages
- Include relevant values, file paths, error codes
- Use appropriate severity levels consistently

### 5. Thread Safety

- All logging macros are thread-safe
- Logger instances use multi-threaded sinks
- No additional synchronization required

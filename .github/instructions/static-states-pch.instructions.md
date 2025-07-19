# GitHub Copilot Instructions: Static States and Pre-Compiled Headers

## Overview

This instruction file provides guidance for GitHub Copilot on the proper usage of the Static States system (`static_states.h`) and Pre-Compiled Header system (`app_pch.h`) in the Scenery Editor X project. These systems provide semantic clarity for static declarations and efficient compilation performance.

## Static States System Instructions

### Core Principles

**Always use semantic static macros instead of bare `static` keyword:**

```cpp
// Correct Usage
INTERNAL void ProcessModuleData();     // Internal module function
LOCAL int callCounter = 0;             // Function-local persistent variable
GLOBAL const char* g_ModuleName = "Renderer"; // Global module state

// Incorrect Usage - Avoid bare static
static void ProcessModuleData();       // Ambiguous intent
static int callCounter = 0;           // Unclear scope
static const char* g_ModuleName = "Renderer"; // Unclear purpose
```

### Macro Selection Guidelines

#### Use INTERNAL for:
- Module-private helper functions
- Implementation-specific constants
- Internal configuration templates
- Module-specific utilities not exposed in headers

```cpp
// Internal helper functions
INTERNAL void ValidateInputData(const InputParams& params)
{
    // Validation logic
}

// Internal constants and templates
INTERNAL constexpr int MAX_BUFFER_SIZE = 1024;
INTERNAL constexpr const char* CONFIG_TEMPLATE = R"(
    enabled = true;
    version = "1.0";
)";

// Internal module state
INTERNAL std::unique_ptr<ModuleData> g_ModuleData;
```

#### Use LOCAL for:
- Function-persistent counters
- One-time initialization flags
- Function-scoped caches
- Function-specific state that persists between calls

```cpp
// Function call counters
int GetUniqueId()
{
    LOCAL int counter = 0;
    return ++counter;
}

// One-time initialization
void InitializeSystemOnce()
{
    LOCAL bool initialized = false;
    if (!initialized)
    {
        PerformInitialization();
        initialized = true;
    }
}

// Function-scoped caches
const ResourceData& GetResourceData(int resourceId)
{
    LOCAL std::unordered_map<int, ResourceData> cache;
    
    if (cache.find(resourceId) == cache.end())
    {
        cache[resourceId] = LoadResourceData(resourceId);
    }
    
    return cache[resourceId];
}
```

#### Use GLOBAL for:
- Module-level configuration
- Translation unit-wide state
- Global constants with internal linkage
- Module-wide utility functions

```cpp
// Module-level configuration
GLOBAL const AppConfig g_DefaultConfig = {
    .windowWidth = 1280,
    .windowHeight = 720,
    .enableVSync = true
};

// Global module state
GLOBAL std::unique_ptr<Logger> g_ModuleLogger;
GLOBAL bool g_IsSystemInitialized = false;

// Global utility functions
GLOBAL std::string FormatLogMessage(const std::string& message)
{
    return fmt::format("[{}]: {}", GetModuleName(), message);
}
```

### Naming Conventions

#### INTERNAL Items
- **Functions**: PascalCase (e.g., `ProcessInternalData`)
- **Variables**: camelCase or UPPER_SNAKE_CASE for constants
- **Constants**: Descriptive names with context

#### LOCAL Items
- **Variables**: camelCase with descriptive names
- **Avoid**: Generic names like `temp`, `var`, `data`
- **Prefer**: Specific names like `initialized`, `counter`, `cache`

#### GLOBAL Items
- **Variables**: `g_` prefix with camelCase (e.g., `g_ModuleState`)
- **Constants**: UPPER_SNAKE_CASE (e.g., `g_MAX_CONNECTIONS`)
- **Functions**: PascalCase (e.g., `GetGlobalConfig`)

### Documentation Requirements

**Always document static variables with their purpose and lifetime:**

```cpp
/**
 * @brief Internal cache for expensive shader compilation results
 * @note Cache is cleared during system shutdown
 */
INTERNAL std::unordered_map<std::string, CompiledShader> g_ShaderCache;

/**
 * @brief Tracks initialization state for this module
 * @note Reset to false on program restart
 */
LOCAL bool isModuleInitialized = false;

/**
 * @brief Global configuration for the rendering system
 * @note Loaded from config file during application startup
 */
GLOBAL RenderConfig g_RenderConfig;
```

### Integration with Project Architecture

#### Module System Integration

```cpp
class CustomModule : public Module
{
public:
    void OnAttach() override
    {
        SEDX_CORE_INFO("=== Initializing {} ===", GetName());
        InitializeModuleResources();
    }
    
private:
    void InitializeModuleResources()
    {
        LOCAL bool initialized = false;
        if (!initialized)
        {
            // Initialize module-specific resources
            g_ModuleResources = CreateRef<ModuleResources>();
            initialized = true;
        }
    }
};

// Module-level configuration
GLOBAL const ModuleConfig g_CustomModuleConfig = {
    .updateFrequency = 60.0f,
    .enableDebugOutput = true
};
```

#### Memory Management Integration

```cpp
// Global resource management using project smart pointers
GLOBAL Ref<ResourceManager> g_ResourceManager = nullptr;

INTERNAL void InitializeResourceManager()
{
    if (!g_ResourceManager)
    {
        g_ResourceManager = CreateRef<ResourceManager>();
        SEDX_CORE_INFO_TAG("RESOURCE", "Resource manager initialized");
    }
}

// Function-local resource caching
Ref<Texture2D> GetCachedTexture(const std::string& path)
{
    LOCAL std::unordered_map<std::string, Ref<Texture2D>> textureCache;
    
    if (textureCache.find(path) == textureCache.end())
    {
        textureCache[path] = CreateRef<Texture2D>(path);
    }
    
    return textureCache[path];
}
```

#### Logging Integration

```cpp
// Use tagged logging with static states
INTERNAL void LogModuleError(const std::string& message)
{
    LOCAL int errorCount = 0;
    SEDX_CORE_ERROR_TAG("MODULE", "Error #{}: {}", ++errorCount, message);
}

GLOBAL void LogGlobalEvent(const std::string& event)
{
    SEDX_CORE_INFO_TAG("GLOBAL", "Global event: {}", event);
}
```

### Thread Safety Considerations

**Always consider thread safety for static variables:**

```cpp
// Thread-safe counter
int GetThreadSafeUniqueId()
{
    LOCAL std::atomic<int> counter{0};
    return ++counter;
}

// Thread-safe global state with mutex
GLOBAL std::mutex g_ConfigMutex;
GLOBAL AppConfig g_Config;

void UpdateGlobalConfig(const AppConfig& newConfig)
{
    std::lock_guard<std::mutex> lock(g_ConfigMutex);
    g_Config = newConfig;
}
```

## Pre-Compiled Header System Instructions

### Core Principles

**Never manually include `app_pch.h` in source files - it's automatically included by the build system.**

### Available Headers

The following headers are automatically available in all source files:

#### Standard Library (Always Available)
```cpp
// These are automatically included - don't add explicit includes
// Container headers: <vector>, <array>, <map>, <unordered_map>, etc.
// Utility headers: <memory>, <functional>, <algorithm>, etc.
// String headers: <string>, <string_view>
// System headers: <filesystem>, <chrono>, <thread>, <mutex>
```

#### Mathematics Library (Always Available)
```cpp
// GLM is automatically configured and available
// Use directly without explicit includes:
glm::vec3 position{0.0f, 0.0f, 0.0f};
glm::mat4 transform = glm::translate(glm::mat4(1.0f), position);
```

#### Project Core (Always Available)
```cpp
// Core project headers are automatically included:
// - SceneryEditorX/core/pointers.h (Ref<T>, CreateRef<T>())
// - SceneryEditorX/logging/logging.hpp (SEDX_CORE_INFO, etc.)
// - SceneryEditorX/logging/asserts.h (SEDX_ASSERT, etc.)
// - SceneryEditorX/utils/static_states.h (INTERNAL, LOCAL, GLOBAL)
```

### What to Include in Source Files

```cpp
// Always include your specific module headers
#include "my_module.h"
#include "specific_functionality.h"

// Include specialized third-party libraries not in PCH
#include <imgui.h>              // UI-specific
#include <vulkan/vulkan.h>      // Graphics-specific
#include <catch2/catch.hpp>     // Test-specific

// Never include these - they're in PCH:
// #include <vector>            // Already available
// #include <memory>            // Already available
// #include <SceneryEditorX/core/pointers.h>  // Already available
```

### Platform-Specific Code

**Use the platform detection macros that are automatically available:**

```cpp
void PlatformSpecificFunction()
{
#ifdef SEDX_PLATFORM_WINDOWS
    // Windows-specific implementation
    ErrMsg("Windows error occurred");
#endif

#ifdef SEDX_PLATFORM_LINUX
    // Linux-specific implementation
    throw std::runtime_error("Linux error occurred");
#endif

#ifdef SEDX_PLATFORM_APPLE
    // macOS-specific implementation
    ErrMsg("macOS error occurred");
#endif
}
```

### Error Handling

**Use the cross-platform `ErrMsg` function that's automatically available:**

```cpp
void HandleError(const std::string& errorMessage)
{
    // ErrMsg is automatically available from PCH
    ErrMsg(fmt::format("Module error: {}", errorMessage));
}

// Template version also works
template<typename T>
void HandleTypedError(const T& error)
{
    ErrMsg(error);  // Automatically formats using fmt::format
}
```

### Development and Debug Features

**Use debug-specific features when available:**

```cpp
void DebugFunction()
{
#ifdef SEDX_DEBUG
    // Debug-specific code
    SEDX_CORE_DEBUG("Debug information available");
    
    // Use profiling if enabled
    #ifdef SEDX_PROFILING_ENABLED
        SEDX_PROFILE_SCOPE("DebugFunction");
    #endif
#endif
}
```

### Best Practices for PCH Usage

#### 1. Don't Include PCH Headers Explicitly

```cpp
// Wrong - these are already in PCH
#include <vector>
#include <memory>
#include <fmt/format.h>

// Correct - only include what's not in PCH
#include "module_specific.h"
#include <specialized_library.h>
```

#### 2. Use Available Functionality

```cpp
// Leverage what's already available
void ModuleFunction()
{
    // Smart pointers from PCH
    auto resource = CreateRef<ResourceType>();
    
    // Logging from PCH
    SEDX_CORE_INFO("Module initialized");
    
    // Containers from PCH
    std::vector<int> data;
    std::unordered_map<std::string, int> lookup;
    
    // Filesystem from PCH
    std::filesystem::path configPath = "config.json";
    
    // JSON from PCH
    nlohmann::json config;
    
    // Formatting from PCH
    std::string formatted = fmt::format("Value: {}", 42);
}
```

#### 3. Platform-Aware Development

```cpp
class PlatformManager
{
public:
    void Initialize()
    {
        SEDX_CORE_INFO("Initializing platform manager");
        
        // Use platform detection from PCH
        #ifdef SEDX_PLATFORM_WINDOWS
            InitializeWindows();
        #elif defined(SEDX_PLATFORM_LINUX)
            InitializeLinux();
        #elif defined(SEDX_PLATFORM_APPLE)
            InitializeMacOS();
        #endif
    }
    
private:
    // Platform-specific implementations
    void InitializeWindows() { /* Windows code */ }
    void InitializeLinux() { /* Linux code */ }
    void InitializeMacOS() { /* macOS code */ }
};
```

## Common Patterns and Examples

### Module Implementation Pattern

```cpp
// MyModule.cpp
#include "MyModule.h"  // Only include module-specific headers

// Module-level configuration using static states
GLOBAL const ModuleConfig g_MyModuleConfig = {
    .enabled = true,
    .updateRate = 60.0f
};

// Internal module state
INTERNAL std::unique_ptr<ModuleData> g_ModuleData;

// Internal helper functions
INTERNAL void ValidateModuleState()
{
    SEDX_CORE_ASSERT(g_ModuleData != nullptr, "Module data not initialized");
}

class MyModule : public Module
{
public:
    MyModule() : Module("MyModule") {}
    
    void OnAttach() override
    {
        SEDX_CORE_INFO("=== Initializing {} ===", GetName());
        
        // Use LOCAL for one-time initialization
        LOCAL bool initialized = false;
        if (!initialized)
        {
            InitializeModuleData();
            initialized = true;
        }
    }
    
    void OnUpdate() override
    {
        SEDX_PROFILE_SCOPE("MyModule::OnUpdate");
        
        // Use LOCAL for persistent state
        LOCAL float accumulator = 0.0f;
        accumulator += GetDeltaTime();
        
        if (accumulator >= g_MyModuleConfig.updateRate)
        {
            UpdateModuleLogic();
            accumulator = 0.0f;
        }
    }
    
private:
    void InitializeModuleData()
    {
        g_ModuleData = CreateRef<ModuleData>();
        SEDX_CORE_INFO_TAG("MODULE", "Module data initialized");
    }
    
    void UpdateModuleLogic()
    {
        ValidateModuleState();
        // Module update logic here
    }
};
```

### Settings Management Pattern

```cpp
// Settings.cpp
#include "Settings.h"

// Internal configuration templates
INTERNAL constexpr const char* CONFIG_TEMPLATE = R"(
    # Application Configuration
    version = "1.0.0";
    window = {
        width = 1280;
        height = 720;
        fullscreen = false;
    };
)";

// Global settings state
GLOBAL std::unique_ptr<libconfig::Config> g_Config;
GLOBAL bool g_IsConfigLoaded = false;

// Internal helper functions
INTERNAL bool ValidateConfig(const libconfig::Config& config)
{
    try
    {
        // Validation logic using libconfig
        config.lookup("version");
        config.lookup("window.width");
        config.lookup("window.height");
        return true;
    }
    catch (const libconfig::SettingNotFoundException& e)
    {
        SEDX_CORE_ERROR_TAG("CONFIG", "Missing setting: {}", e.what());
        return false;
    }
}

INTERNAL void CreateDefaultConfig(const std::string& configPath)
{
    std::ofstream file(configPath);
    file << CONFIG_TEMPLATE;
    file.close();
    
    SEDX_CORE_INFO_TAG("CONFIG", "Created default config at: {}", configPath);
}

// Public API
bool LoadSettings(const std::string& configPath)
{
    LOCAL std::string lastLoadedPath;
    
    // Check if we're reloading the same file
    if (g_IsConfigLoaded && lastLoadedPath == configPath)
    {
        return true;
    }
    
    // Initialize global config
    g_Config = std::make_unique<libconfig::Config>();
    
    try
    {
        // Check if config file exists
        if (!std::filesystem::exists(configPath))
        {
            CreateDefaultConfig(configPath);
        }
        
        // Load configuration
        g_Config->readFile(configPath.c_str());
        
        if (!ValidateConfig(*g_Config))
        {
            SEDX_CORE_ERROR_TAG("CONFIG", "Invalid configuration");
            return false;
        }
        
        lastLoadedPath = configPath;
        g_IsConfigLoaded = true;
        
        SEDX_CORE_INFO_TAG("CONFIG", "Configuration loaded successfully");
        return true;
    }
    catch (const libconfig::Exception& e)
    {
        SEDX_CORE_ERROR_TAG("CONFIG", "Config error: {}", e.what());
        return false;
    }
}
```

## Key Reminders for GitHub Copilot

1. **Always use semantic static macros** (INTERNAL, LOCAL, GLOBAL) instead of bare `static`
2. **Never manually include `app_pch.h`** - it's automatically included
3. **Leverage PCH includes** - don't re-include standard library headers
4. **Use platform detection macros** from PCH for cross-platform code
5. **Document static variables** with their purpose and lifetime
6. **Consider thread safety** for static variables in multi-threaded contexts
7. **Use tagged logging** with static states for better debugging
8. **Follow naming conventions** for different static scopes
9. **Use `ErrMsg` function** for cross-platform error dialogs
10. **Profile performance-critical code** using available profiling macros

## Integration Checklist

When writing code in the Scenery Editor X project:

- [ ] Use INTERNAL/LOCAL/GLOBAL instead of static
- [ ] Only include headers not available in PCH
- [ ] Use CreateRef<T>() for smart pointers
- [ ] Use SEDX_CORE_* macros for logging
- [ ] Use SEDX_ASSERT for debugging
- [ ] Use platform detection macros for cross-platform code
- [ ] Document static variables with purpose and lifetime
- [ ] Consider thread safety for static variables
- [ ] Use ErrMsg() for error dialogs
- [ ] Profile performance-critical functions

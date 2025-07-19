# Static States System Documentation

## Overview

The Static States system (`static_states.h`) provides semantic clarity for static variable declarations throughout the Scenery Editor X codebase. This system introduces three semantic macros that replace the traditional `static` keyword with more descriptive alternatives, improving code readability and maintainability.

## Table of Contents

1. [Purpose and Design Philosophy](#purpose-and-design-philosophy)
2. [Macro Definitions](#macro-definitions)
3. [Usage Guidelines](#usage-guidelines)
4. [Implementation Examples](#implementation-examples)
5. [Best Practices](#best-practices)
6. [Integration with Project Architecture](#integration-with-project-architecture)
7. [Migration Guide](#migration-guide)
8. [Troubleshooting](#troubleshooting)

## Purpose and Design Philosophy

### Design Goals

The Static States system was designed with the following principles:

1. **Semantic Clarity**: Replace ambiguous `static` keywords with descriptive macros that clearly indicate the intended scope and purpose
2. **Code Maintainability**: Improve code readability for developers by making static variable intentions explicit
3. **Architectural Consistency**: Provide consistent patterns for static declarations across the entire codebase
4. **Documentation Through Code**: Make the code self-documenting by using descriptive macro names

### Problems Solved

The traditional `static` keyword in C++ serves multiple purposes:
- Internal linkage for functions and global variables
- Local persistence for function-scope variables
- Class-level variables (static members)

This overloading of the `static` keyword can lead to confusion. The Static States system addresses this by providing specific macros for each use case.

## Macro Definitions

### INTERNAL

```cpp
#define INTERNAL static
```

**Purpose**: Declares functions and variables that are internal to a specific module or compilation unit.

**Semantic Meaning**: 
- The function or variable is not intended for use outside its defining module
- Provides internal linkage (file scope)
- Typically used for helper functions, implementation details, and module-private data

**Use Cases**:
- Helper functions within a module
- Implementation-specific global variables
- Private module constants
- Utility functions not exposed in headers

### LOCAL

```cpp
#define LOCAL static
```

**Purpose**: Declares static variables that are local to a specific function scope.

**Semantic Meaning**:
- The variable maintains its value between function calls
- The variable is not accessible outside the function
- Provides persistent storage within function scope
- Initialized only once on first function call

**Use Cases**:
- Function-local counters
- One-time initialization flags
- Cached computation results
- Function-specific state maintenance

### GLOBAL

```cpp
#define GLOBAL static
```

**Purpose**: Declares static variables and functions that have global scope within their compilation unit.

**Semantic Meaning**:
- The variable or function has global scope within the source file
- Provides internal linkage (not externally visible)
- Typically used for module-level state or configuration
- Accessible throughout the entire source file

**Use Cases**:
- Module-level configuration
- Shared state within a compilation unit
- Translation unit-wide constants
- Module initialization state

## Usage Guidelines

### When to Use Each Macro

#### INTERNAL Usage

```cpp
// Internal helper function
INTERNAL void ProcessInternalData(const std::vector<int>& data)
{
    // Implementation details
}

// Internal module constants
INTERNAL constexpr const char* MODULE_NAME = "RenderEngine";
INTERNAL constexpr int MAX_BUFFER_SIZE = 1024;

// Internal configuration templates
INTERNAL constexpr const char* CONFIG_TEMPLATE = R"(
    version = "1.0";
    enabled = true;
)";
```

#### LOCAL Usage

```cpp
// Function-local counter
int GetNextId()
{
    LOCAL int counter = 0;
    return ++counter;
}

// One-time initialization
void InitializeSystem()
{
    LOCAL bool initialized = false;
    if (!initialized)
    {
        // Perform initialization
        initialized = true;
    }
}

// Cached computation result
float ComputeExpensiveValue(int input)
{
    LOCAL std::unordered_map<int, float> cache;
    
    if (cache.find(input) == cache.end())
    {
        cache[input] = PerformExpensiveComputation(input);
    }
    
    return cache[input];
}
```

#### GLOBAL Usage

```cpp
// Module-level state
GLOBAL bool g_IsSystemInitialized = false;
GLOBAL std::unique_ptr<Logger> g_ModuleLogger;

// Module-level configuration
GLOBAL const AppConfig g_DefaultConfig = {
    .windowWidth = 1280,
    .windowHeight = 720,
    .enableVSync = true
};

// Global utility functions
GLOBAL std::string FormatMessage(const std::string& message)
{
    return fmt::format("[{}]: {}", MODULE_NAME, message);
}
```

### Naming Conventions

When using these macros, follow these naming conventions:

#### INTERNAL Items
- Functions: Use PascalCase (e.g., `ProcessInternalData`)
- Variables: Use camelCase or UPPER_SNAKE_CASE for constants
- Prefix with descriptive module context when appropriate

#### LOCAL Items
- Use camelCase for variables
- Use descriptive names that indicate purpose (e.g., `initialized`, `counter`, `cache`)
- Avoid generic names like `temp` or `var`

#### GLOBAL Items
- Use `g_` prefix for global variables
- Use UPPER_SNAKE_CASE for constants
- Use PascalCase for functions

## Implementation Examples

### Real-World Example: Settings Module

```cpp
// settings.cpp
#include "settings.h"

// Internal configuration templates
INTERNAL constexpr const char* APPLICATION_SECTION_TEMPLATE = R"(
    # Application settings
    version = "${APP_VERSION}";
    no_titlebar = false;
    window_width = 1280;
    window_height = 720;
)";

INTERNAL constexpr const char* XPLANE_SECTION_TEMPLATE = R"(
    # X-Plane integration settings
    xplane_path = "";
    auto_detect = true;
    plugins_enabled = true;
)";

// Global module state
GLOBAL std::unique_ptr<libconfig::Config> g_Config;
GLOBAL bool g_IsConfigLoaded = false;

// Internal helper functions
INTERNAL bool ValidateConfigSection(const libconfig::Setting& section)
{
    // Validation logic
    return true;
}

INTERNAL void ApplyDefaults(libconfig::Config& config)
{
    // Apply default values
}

// Public API implementation
bool LoadSettings(const std::string& configPath)
{
    LOCAL std::string lastConfigPath;
    
    // Check if we're reloading the same file
    if (g_IsConfigLoaded && lastConfigPath == configPath)
    {
        return true;
    }
    
    // Load configuration
    g_Config = std::make_unique<libconfig::Config>();
    
    try
    {
        g_Config->readFile(configPath.c_str());
        ApplyDefaults(*g_Config);
        
        lastConfigPath = configPath;
        g_IsConfigLoaded = true;
        
        SEDX_CORE_INFO_TAG("SETTINGS", "Configuration loaded from: {}", configPath);
        return true;
    }
    catch (const libconfig::Exception& e)
    {
        SEDX_CORE_ERROR_TAG("SETTINGS", "Failed to load config: {}", e.what());
        return false;
    }
}
```

### Real-World Example: UI Modal System

```cpp
// modals.cpp
#include "modals.h"

// Local state for modal dialogs
INTERNAL char projectName[128];
INTERNAL char projectLocation[2048];
INTERNAL bool showCreateProjectModal = false;

// Internal helper functions
INTERNAL void ResetModalState()
{
    memset(projectName, 0, sizeof(projectName));
    memset(projectLocation, 0, sizeof(projectLocation));
    showCreateProjectModal = false;
}

INTERNAL bool ValidateProjectInput()
{
    return strlen(projectName) > 0 && strlen(projectLocation) > 0;
}

// Public API
void ShowCreateProjectModal()
{
    showCreateProjectModal = true;
}

void RenderCreateProjectModal()
{
    if (!showCreateProjectModal)
        return;
        
    LOCAL bool firstShow = true;
    if (firstShow)
    {
        ResetModalState();
        firstShow = false;
    }
    
    // ImGui modal rendering logic
    if (ImGui::BeginPopupModal("Create Project"))
    {
        ImGui::InputText("Project Name", projectName, sizeof(projectName));
        ImGui::InputText("Location", projectLocation, sizeof(projectLocation));
        
        if (ImGui::Button("Create"))
        {
            if (ValidateProjectInput())
            {
                CreateProject(projectName, projectLocation);
                showCreateProjectModal = false;
                firstShow = true;
            }
        }
        
        ImGui::EndPopup();
    }
}
```

## Best Practices

### 1. Choose the Right Macro

- Use `INTERNAL` for module-private functions and variables
- Use `LOCAL` for function-persistent state
- Use `GLOBAL` for module-wide state and configuration

### 2. Documentation

Always document the purpose and lifetime of static variables:

```cpp
/**
 * @brief Internal cache for expensive computations
 * @note This cache is cleared on system shutdown
 */
INTERNAL std::unordered_map<int, ComputationResult> g_ComputationCache;

/**
 * @brief Tracks the number of times this function has been called
 * @note Resets to 0 on program restart
 */
LOCAL int callCount = 0;
```

### 3. Initialization

Be explicit about initialization, especially for complex types:

```cpp
// Good: Explicit initialization
GLOBAL std::unique_ptr<Logger> g_Logger = nullptr;

// Better: Use function-level initialization for complex setup
GLOBAL Logger& GetLogger()
{
    LOCAL std::unique_ptr<Logger> logger = std::make_unique<Logger>();
    return *logger;
}
```

### 4. Thread Safety

Consider thread safety for GLOBAL and LOCAL variables:

```cpp
// Thread-safe counter
int GetNextThreadSafeId()
{
    LOCAL std::atomic<int> counter{0};
    return ++counter;
}

// Thread-safe global state
GLOBAL std::mutex g_ConfigMutex;
GLOBAL AppConfig g_Config;
```

### 5. Avoid Overuse

Don't use these macros for every static declaration. Use them where semantic clarity provides value:

```cpp
// Don't overuse for simple cases
static const int BUFFER_SIZE = 1024;  // This is fine

// Use for complex or ambiguous cases
INTERNAL constexpr const char* ERROR_MESSAGE_TEMPLATE = "Error in {}: {}";
```

## Integration with Project Architecture

### Module System Integration

The Static States system integrates seamlessly with the project's module system:

```cpp
class CustomModule : public Module
{
public:
    void OnAttach() override
    {
        // Initialize module using static states
        InitializeModuleState();
    }
    
private:
    void InitializeModuleState()
    {
        LOCAL bool initialized = false;
        if (!initialized)
        {
            // One-time initialization
            initialized = true;
        }
    }
};

// Module-level configuration
GLOBAL const ModuleConfig g_CustomModuleConfig = {
    .updateFrequency = 60.0f,
    .enableDebug = true
};
```

### Logging System Integration

The macros work well with the project's logging system:

```cpp
INTERNAL void LogInternalError(const std::string& message)
{
    SEDX_CORE_ERROR_TAG("INTERNAL", "Internal error: {}", message);
}

GLOBAL void LogGlobalEvent(const std::string& event)
{
    LOCAL int eventCounter = 0;
    SEDX_CORE_INFO_TAG("GLOBAL", "Event #{}: {}", ++eventCounter, event);
}
```

### Memory Management Integration

Use with the project's smart pointer system:

```cpp
// Global resource management
GLOBAL Ref<ResourceManager> g_ResourceManager = nullptr;

INTERNAL void InitializeResourceManager()
{
    if (!g_ResourceManager)
    {
        g_ResourceManager = CreateRef<ResourceManager>();
    }
}
```

## Migration Guide

### From Traditional Static

When migrating existing code, follow these steps:

1. **Identify the purpose** of each static declaration
2. **Choose the appropriate macro** based on scope and intent
3. **Update naming conventions** to match the new semantic meaning
4. **Add documentation** explaining the purpose and lifetime

#### Example Migration

Before:
```cpp
// Ambiguous static usage
static int counter = 0;
static void helperFunction();
static const char* CONFIG_PATH = "config.txt";
```

After:
```cpp
// Clear semantic meaning
LOCAL int functionCallCounter = 0;           // Function-local counter
INTERNAL void ProcessHelperFunction();       // Internal helper
GLOBAL const char* g_ConfigPath = "config.txt"; // Global constant
```

### Gradual Migration Strategy

1. **Phase 1**: Add static_states.h to new files
2. **Phase 2**: Update critical modules during maintenance
3. **Phase 3**: Gradually convert remaining files
4. **Phase 4**: Establish coding standards requiring these macros

## Troubleshooting

### Common Issues

#### 1. Linker Errors

**Problem**: Multiple definition errors when using GLOBAL in headers.

**Solution**: Use GLOBAL only in source files, not headers.

```cpp
// Wrong: In header file
GLOBAL int g_SharedValue = 42;

// Correct: In source file
GLOBAL int g_SharedValue = 42;
```

#### 2. Initialization Order

**Problem**: Global static variables with initialization dependencies.

**Solution**: Use function-local static initialization.

```cpp
// Problematic
GLOBAL Logger g_Logger;         // May not be initialized
GLOBAL Config g_Config(&g_Logger); // Depends on g_Logger

// Better
GLOBAL Config& GetConfig()
{
    LOCAL Config config(&GetLogger());
    return config;
}
```

#### 3. Thread Safety

**Problem**: Race conditions with LOCAL variables in multi-threaded code.

**Solution**: Use proper synchronization or atomic types.

```cpp
// Thread-unsafe
int GetNextId()
{
    LOCAL int counter = 0;
    return ++counter;  // Race condition
}

// Thread-safe
int GetNextId()
{
    LOCAL std::atomic<int> counter{0};
    return ++counter;
}
```

### Debugging Tips

1. **Use descriptive names** to make debugging easier
2. **Add logging** to track static variable lifecycle
3. **Use static analysis tools** to detect potential issues
4. **Test initialization order** carefully

## Conclusion

The Static States system provides a simple but powerful way to improve code clarity and maintainability in the Scenery Editor X project. By replacing ambiguous `static` keywords with semantic macros, developers can quickly understand the purpose and scope of static declarations, leading to more maintainable and readable code.

Remember to use these macros judiciously and always prioritize code clarity over brevity. The system is designed to enhance understanding, not to complicate simple declarations.

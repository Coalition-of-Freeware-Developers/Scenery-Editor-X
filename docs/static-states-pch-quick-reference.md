# Static States & PCH Quick Reference

## Static States Macros

### Quick Decision Tree
```
Need static variable/function?
├── Internal to module only?
│   └── Use INTERNAL
├── Persists within function?
│   └── Use LOCAL  
└── Global to compilation unit?
    └── Use GLOBAL
```

### Macro Reference

| Macro | Purpose | Example |
|-------|---------|---------|
| `INTERNAL` | Module-private functions/variables | `INTERNAL void HelperFunction();` |
| `LOCAL` | Function-persistent variables | `LOCAL int counter = 0;` |
| `GLOBAL` | Translation unit-wide state | `GLOBAL Config g_Config;` |

### Naming Conventions

| Scope | Functions | Variables | Constants |
|-------|-----------|-----------|-----------|
| **INTERNAL** | `PascalCase` | `camelCase` | `UPPER_SNAKE_CASE` |
| **LOCAL** | N/A | `camelCase` | `camelCase` |
| **GLOBAL** | `PascalCase` | `g_camelCase` | `g_UPPER_SNAKE_CASE` |

## PCH Quick Reference

### ✅ Automatically Available (Don't Include)
```cpp
// Standard Library
std::vector, std::string, std::filesystem, std::chrono, std::thread

// Mathematics
Vec3, Mat4, Quat

// Project Core
Ref<T>, CreateRef<T>(), SEDX_CORE_INFO, SEDX_ASSERT

// Utilities
fmt::format, nlohmann::json, static states macros

// Platform
SEDX_PLATFORM_WINDOWS, SEDX_PLATFORM_LINUX, ErrMsg()
```

### ❌ Still Need to Include
```cpp
#include "module_specific.h"
#include <imgui.h>
#include <vulkan/vulkan.h>
#include <catch2/catch.hpp>
```

## Common Patterns

### Module Implementation
```cpp
// Module.cpp - No PCH include needed
#include "Module.h"

GLOBAL const Config g_ModuleConfig = { /* config */ };
INTERNAL std::unique_ptr<Data> g_ModuleData;

INTERNAL void ValidateState() { /* validation */ }

class Module : public BaseModule 
{
    void OnAttach() override 
    {
        LOCAL bool initialized = false;
        if (!initialized) {
            InitializeModule();
            initialized = true;
        }
    }
};
```

### Settings Management
```cpp
// Settings.cpp
#include "Settings.h"

INTERNAL constexpr const char* CONFIG_TEMPLATE = R"(
    version = "1.0";
    enabled = true;
)";

GLOBAL std::unique_ptr<libconfig::Config> g_Config;

bool LoadSettings(const std::string& path) 
{
    LOCAL std::string lastPath;
    if (lastPath == path) return true;
    
    g_Config = std::make_unique<libconfig::Config>();
    // Load logic here
    lastPath = path;
    return true;
}
```

### Error Handling
```cpp
void HandleError(const std::string& message) 
{
    // ErrMsg available from PCH
    ErrMsg(fmt::format("Error: {}", message));
}

#ifdef SEDX_PLATFORM_WINDOWS
    // Windows-specific code
#endif
```

## Thread Safety

### Safe Patterns
```cpp
// Atomic counters
int GetNextId() {
    LOCAL std::atomic<int> counter{0};
    return ++counter;
}

// Mutex protection
GLOBAL std::mutex g_StateMutex;
GLOBAL AppState g_State;
```

### Documentation Template
```cpp
/**
 * @brief [Brief description]
 * @note [Lifetime and threading notes]
 */
INTERNAL/LOCAL/GLOBAL [type] [name];
```

## Debugging Tips

### Logging Integration
```cpp
SEDX_CORE_INFO_TAG("MODULE", "Message: {}", value);
SEDX_CORE_ERROR_TAG("ERROR", "Error: {}", error);
```

### Profiling
```cpp
#ifdef SEDX_PROFILING_ENABLED
    SEDX_PROFILE_SCOPE("FunctionName");
#endif
```

### Assertions
```cpp
SEDX_CORE_ASSERT(condition, "Error message");
SEDX_ASSERT(condition, "App assertion");
```

## Migration Checklist

- [ ] Replace `static` with INTERNAL/LOCAL/GLOBAL
- [ ] Update naming conventions
- [ ] Remove PCH headers from includes
- [ ] Add documentation for static variables
- [ ] Test cross-platform compatibility
- [ ] Verify thread safety
- [ ] Update error handling to use ErrMsg()

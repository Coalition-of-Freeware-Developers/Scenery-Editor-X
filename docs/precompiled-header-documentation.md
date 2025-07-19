# Pre-Compiled Header System Documentation

## Overview

The Pre-Compiled Header (PCH) system in Scenery Editor X, implemented through `app_pch.h`, provides a centralized mechanism for including frequently used headers and defining common macros. This system significantly improves compilation times by pre-compiling stable headers and provides a consistent foundation for all application modules.

## Table of Contents

1. [Purpose and Design Philosophy](#purpose-and-design-philosophy)
2. [Architecture Overview](#architecture-overview)
3. [Header Organization](#header-organization)
4. [Platform-Specific Components](#platform-specific-components)
5. [Integration Guidelines](#integration-guidelines)
6. [Performance Considerations](#performance-considerations)
7. [Maintenance and Best Practices](#maintenance-and-best-practices)
8. [Troubleshooting](#troubleshooting)

## Purpose and Design Philosophy

### Design Goals

The PCH system is designed with the following principles:

1. **Compilation Performance**: Dramatically reduce compilation times by pre-compiling stable headers
2. **Consistency**: Provide a unified set of includes and definitions across all modules
3. **Platform Abstraction**: Handle platform-specific includes and definitions in one place
4. **Dependency Management**: Centralize management of third-party library includes
5. **Development Efficiency**: Reduce boilerplate includes in source files

### Benefits

- **Faster Build Times**: Pre-compilation of stable headers reduces repetitive parsing
- **Reduced Include Bloat**: Common headers are included once in the PCH
- **Platform Consistency**: Unified platform detection and macro definitions
- **Easier Maintenance**: Single point of control for common dependencies

## Architecture Overview

### File Structure

```
app_pch.h
├── Copyright Header
├── System Detection
├── C++20 Standard Library Includes
├── GLFW Configuration
├── GLM Mathematics Library
├── Third-Party Libraries
├── Project-Specific Includes
├── Development Macros
└── Utility Functions
```

### CMake Integration

The PCH is integrated into the build system through CMake:

```cmake
TARGET_PRECOMPILE_HEADERS(AppCore PRIVATE ${CMAKE_SOURCE_DIR}/Source/SceneryEditorX/app_pch.h)
```

This automatically includes the PCH in all source files that link against the `AppCore` target.

## Header Organization

### 1. System Detection

```cpp
#include <SceneryEditorX/platform/system_detection.h>
```

Provides platform-specific macros and definitions:
- `SEDX_PLATFORM_WINDOWS`
- `SEDX_PLATFORM_LINUX`
- `SEDX_PLATFORM_APPLE`
- Architecture detection
- Compiler-specific settings

### 2. C++20 Standard Library Includes

The PCH includes commonly used standard library headers:

#### Container Headers
```cpp
#include <vector>
#include <array>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <set>
```

#### Utility Headers
```cpp
#include <memory>
#include <functional>
#include <utility>
#include <algorithm>
#include <limits>
```

#### String and I/O Headers
```cpp
#include <string>
#include <string_view>
#include <iostream>
#include <fstream>
#include <sstream>
```

#### System Headers
```cpp
#include <filesystem>
#include <chrono>
#include <thread>
#include <mutex>
#include <random>
```

#### Type System Headers
```cpp
#include <type_traits>
#include <cstddef>
#include <cstdlib>
#include <cstdarg>
```

### 3. GLFW Configuration

```cpp
#define GLFW_INCLUDE_VULKAN
```

Configures GLFW to include Vulkan headers automatically, ensuring proper graphics API integration.

### 4. GLM Mathematics Library

```cpp
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
```

**Configuration Details**:
- `GLM_FORCE_RADIANS`: Ensures all angle parameters are in radians
- `GLM_ENABLE_EXPERIMENTAL`: Enables experimental GLM features
- `GLM_FORCE_DEPTH_ZERO_TO_ONE`: Configures depth range for Vulkan (0.0 to 1.0)

### 5. Third-Party Libraries

#### Image Processing
```cpp
#include <stb_image.h>
```

#### File Dialogs
```cpp
#include <portable-file-dialogs.h>
```

#### JSON Processing
```cpp
#include <nlohmann/json.hpp>
```

#### Formatting
```cpp
#include <fmt/core.h>
#include <fmt/format.h>
```

### 6. Project-Specific Includes

```cpp
#include <SceneryEditorX/resource.h>
#include <SceneryEditorX/core/pointers.h>
#include <SceneryEditorX/core/base.hpp>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/logging/asserts.h>
#include <SceneryEditorX/utils/static_states.h>
```

These provide core project functionality that's used across all modules.

## Platform-Specific Components

### Development Macros

```cpp
#ifdef SEDX_DEBUG
    extern std::filesystem::path workingDir;
#endif

#if defined(SEDX_DEBUG) && defined(SEDX_PROFILING_ENABLED)
    #include <SceneryEditorX/logging/profiler.hpp>
    #define TRACY_ENABLE
#endif
```

### Error Message Function

The PCH includes a cross-platform error message function:

```cpp
template <typename T>
void ErrMsg(const T &errorMessage)
{
    std::string errorStr = fmt::format("{}", errorMessage);

#ifdef SEDX_PLATFORM_WINDOWS
    const std::wstring errorWStr(errorStr.begin(), errorStr.end());
    MessageBoxW(nullptr, errorWStr.c_str(), L"Error", MB_OK | MB_ICONERROR);
#endif

#ifdef SEDX_PLATFORM_APPLE
    @autoreleasepool {
        NSString *errorStr = [NSString stringWithUTF8String:errorMessage.c_str()];
        NSString *nsTitle = [NSString stringWithUTF8String:"Error"];
        
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:nsTitle];
        [alert setInformativeText:errorStr];
        [alert setAlertStyle:NSAlertStyleCritical];
        [alert runModal];
    }
#endif

#ifdef SEDX_PLATFORM_LINUX
    throw std::runtime_error(errorStr);
#endif
}
```

## Integration Guidelines

### Using the PCH in Source Files

When the PCH is properly configured, you **do not** need to manually include it in source files. The build system automatically includes it.

#### What You Get Automatically

```cpp
// These are automatically available in all source files:
// - All standard library headers
// - GLM mathematics library
// - Project core headers
// - Logging macros
// - Smart pointer types
// - Platform detection macros
```

#### What You Still Need to Include

```cpp
// Module-specific headers
#include "my_module.h"
#include "specific_functionality.h"

// Specialized third-party libraries not in PCH
#include <specialized_library.h>
```

### Best Practices for PCH Usage

#### 1. Keep PCH Stable

Only include headers that are:
- Rarely changed
- Used across multiple modules
- Stable third-party libraries

#### 2. Avoid Including in PCH

- Headers that change frequently
- Large headers used by only a few modules
- Platform-specific headers (handle through system_detection.h)

#### 3. Module-Specific Includes

```cpp
// Good: Module-specific functionality
#include "renderer/vulkan_renderer.h"
#include "ui/imgui_integration.h"

// Bad: These are already in PCH
#include <vector>
#include <memory>
#include <SceneryEditorX/core/pointers.h>
```

## Performance Considerations

### Compilation Time Benefits

The PCH system provides significant compilation time improvements:

- **Initial Compilation**: PCH is compiled once and reused
- **Incremental Builds**: Only changed source files are recompiled
- **Dependency Reduction**: Reduces repetitive header parsing

### Memory Usage

- **Build Time**: Increased memory usage during compilation
- **Runtime**: No impact on runtime memory usage
- **Storage**: Additional disk space for PCH files

### Optimization Strategies

1. **Selective Inclusion**: Only include frequently used headers
2. **Platform Guards**: Use conditional compilation for platform-specific includes
3. **Regular Cleanup**: Remove unused includes from PCH

## Maintenance and Best Practices

### Regular Maintenance Tasks

#### 1. Audit PCH Contents

Periodically review PCH includes:

```cpp
// Check usage frequency
grep -r "include.*vector" source/
grep -r "include.*memory" source/

// Remove unused includes
// Update third-party library versions
// Optimize platform-specific sections
```

#### 2. Performance Monitoring

Monitor compilation times:

```bash
# Measure build times with and without PCH
cmake --build build --target clean
time cmake --build build

# Profile compilation
cmake --build build --verbose
```

### Adding New Headers to PCH

When adding new headers, consider:

1. **Usage Frequency**: Is it used in >50% of source files?
2. **Stability**: Does it change frequently?
3. **Size**: Is it a large header that significantly impacts build time?
4. **Dependencies**: Does it have complex dependencies?

Example addition process:

```cpp
// 1. Add to appropriate section
#include <new_stable_library.h>

// 2. Test build times
// 3. Verify no conflicts
// 4. Update documentation
```

### Removing Headers from PCH

When removing headers:

1. **Identify affected files** that will need explicit includes
2. **Update source files** to include the header explicitly
3. **Test the build** to ensure no missing includes
4. **Update documentation**

### Cross-Platform Considerations

#### Platform-Specific Headers

```cpp
// Don't add platform-specific headers directly
// Instead, use system_detection.h patterns

#ifdef SEDX_PLATFORM_WINDOWS
    // Windows-specific includes in system_detection.h
#endif

#ifdef SEDX_PLATFORM_LINUX
    // Linux-specific includes in system_detection.h
#endif
```

#### Compiler Compatibility

Test PCH across different compilers:
- **MSVC**: Windows development
- **GCC**: Linux development
- **Clang**: macOS development

## Troubleshooting

### Common Issues

#### 1. PCH Compilation Errors

**Problem**: PCH fails to compile due to conflicting definitions.

**Solution**: 
- Check for conflicting macros
- Ensure proper include order
- Use include guards

```cpp
// Check for conflicts
#ifdef CONFLICTING_MACRO
#undef CONFLICTING_MACRO
#endif
```

#### 2. Missing Includes in Source Files

**Problem**: Source files fail to compile after PCH changes.

**Solution**: 
- Add explicit includes for removed PCH headers
- Update source files systematically

```cpp
// Add missing includes
#include <vector>  // If removed from PCH
#include <memory>  // If removed from PCH
```

#### 3. Build Time Regression

**Problem**: Build times increase after PCH changes.

**Solution**: 
- Profile compilation times
- Remove heavy or rarely used headers
- Optimize include order

#### 4. Platform-Specific Compilation Issues

**Problem**: PCH works on one platform but fails on another.

**Solution**: 
- Use conditional compilation
- Test on all target platforms
- Separate platform-specific code

### Debugging PCH Issues

#### 1. Verbose Compilation

```bash
# Enable verbose output
cmake --build build --verbose

# Check PCH generation
find build -name "*.pch" -o -name "*.gch"
```

#### 2. Manual PCH Testing

```bash
# Test PCH compilation separately
g++ -x c++-header app_pch.h -o app_pch.h.gch

# Test source compilation with PCH
g++ -include app_pch.h source.cpp
```

#### 3. Dependency Analysis

```bash
# Analyze header dependencies
cpp -M source.cpp | grep -v "^#"

# Check for circular dependencies
graphviz dependency_graph.dot
```

## Advanced Usage

### Conditional PCH Content

```cpp
// Feature-specific includes
#ifdef SEDX_ENABLE_NETWORKING
#include <networking_library.h>
#endif

#ifdef SEDX_ENABLE_AUDIO
#include <audio_library.h>
#endif

// Debug-specific includes
#ifdef SEDX_DEBUG
#include <debug_utilities.h>
#endif
```

### PCH Versioning

```cpp
// Version tracking for PCH compatibility
#define SEDX_PCH_VERSION 1
#define SEDX_PCH_COMPATIBLE_VERSION 1

// Ensure compatibility
#if SEDX_PCH_VERSION != SEDX_PCH_COMPATIBLE_VERSION
#error "PCH version mismatch - rebuild required"
#endif
```

### Module-Specific PCH

For large projects, consider multiple PCH files:

```cmake
# Core PCH for all modules
TARGET_PRECOMPILE_HEADERS(AppCore PRIVATE app_pch.h)

# Renderer-specific PCH
TARGET_PRECOMPILE_HEADERS(RendererCore PRIVATE renderer_pch.h)

# UI-specific PCH
TARGET_PRECOMPILE_HEADERS(UICore PRIVATE ui_pch.h)
```

## Conclusion

The Pre-Compiled Header system is a critical component of the Scenery Editor X build system, providing significant performance benefits while maintaining code consistency. Proper maintenance and thoughtful header selection are essential for maximizing these benefits.

Key takeaways:
- PCH should contain stable, frequently used headers
- Regular maintenance prevents build time regression
- Platform-specific code should be properly guarded
- Monitor build performance and adjust PCH contents accordingly

By following these guidelines, the PCH system will continue to provide excellent build performance while supporting the project's growth and evolution.

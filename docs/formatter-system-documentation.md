# Formatter System Documentation

## Overview

The `formatter.h` file provides custom formatter specializations for Scenery Editor X's vector types (`Vec2`, `Vec3`, `Vec4`) and standard library types (`std::filesystem::path`) to enable seamless string formatting using both the `fmt` library and C++20's `std::format`. This system ensures consistent and readable output formatting across the entire application.

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Supported Types](#supported-types)
3. [Format Specifications](#format-specifications)
4. [Usage Examples](#usage-examples)
5. [Best Practices](#best-practices)
6. [Integration with Logging System](#integration-with-logging-system)
7. [Performance Considerations](#performance-considerations)
8. [Error Handling](#error-handling)
9. [Extending the System](#extending-the-system)

## Architecture Overview

The formatter system is built on template specializations that extend the formatting capabilities of both the `fmt` library and C++20's `std::format`. The implementation follows a dual-namespace approach:

1. **fmt namespace**: Provides formatters for the `fmt` library (external dependency)
2. **std namespace**: Provides formatters for C++20's standard `std::format`

This dual approach ensures compatibility with both modern C++ standards and the widely-used fmt library, allowing for gradual migration or mixed usage as needed.

### Design Principles

- **Consistency**: All vector types use the same formatting pattern and precision
- **Readability**: Output is human-readable with parentheses and comma separation
- **Flexibility**: Support for both fixed-point and scientific notation
- **Performance**: Template-based implementation with compile-time optimization
- **Error Safety**: Proper error handling for invalid format specifications

## Supported Types

### Vector Types

All vector types in Scenery Editor X are supported through GLM type aliases:

- **Vec2** (`glm::vec2`): 2D floating-point vector
- **Vec3** (`glm::vec3`): 3D floating-point vector  
- **Vec4** (`glm::vec4`): 4D floating-point vector

### Standard Library Types

- **std::filesystem::path**: File system path objects

## Format Specifications

### Vector Format Specifications

Vector types support two presentation formats:

| Format | Description | Example Output |
|--------|-------------|----------------|
| `f` (default) | Fixed-point notation with 3 decimal places | `(1.235, 6.789)` |
| `e` | Scientific notation with 3 decimal places | `(1.235e+00, 6.789e+00)` |

### Path Format Specifications

Filesystem paths inherit all format specifications from `std::formatter<std::string>`, providing full string formatting capabilities.

## Usage Examples

### Basic Vector Formatting

```cpp
#include <SceneryEditorX/utils/formatter.h>
#include <format>
#include <iostream>

// Vec2 formatting
Vec2 position(1.2345f, 6.7890f);
std::cout << std::format("Player position: {}", position) << std::endl;
// Output: Player position: (1.235, 6.789)

// Vec3 formatting with scientific notation
Vec3 velocity(12.5f, -8.3f, 15.7f);
std::cout << std::format("Velocity: {:e}", velocity) << std::endl;
// Output: Velocity: (1.250e+01, -8.300e+00, 1.570e+01)

// Vec4 formatting (commonly used for colors)
Vec4 color(1.0f, 0.5f, 0.2f, 1.0f);
std::cout << std::format("RGBA Color: {}", color) << std::endl;
// Output: RGBA Color: (1.000, 0.500, 0.200, 1.000)
```

### Path Formatting

```cpp
#include <SceneryEditorX/utils/formatter.h>
#include <filesystem>
#include <format>

std::filesystem::path assetPath = "assets/models/aircraft.obj";
std::cout << std::format("Loading model from: {}", assetPath) << std::endl;
// Output: Loading model from: assets/models/aircraft.obj

// With additional string formatting
std::cout << std::format("Asset: '{:>30}'", assetPath) << std::endl;
// Output: Asset: '       assets/models/aircraft.obj'
```

### Integration with fmt Library

```cpp
#include <SceneryEditorX/utils/formatter.h>
#include <fmt/format.h>

Vec3 cameraPosition(10.5f, 20.3f, 5.8f);

// Using fmt::format (if you prefer fmt over std::format)
std::string message = fmt::format("Camera at: {}", cameraPosition);
// Output: Camera at: (10.500, 20.300, 5.800)

// Scientific notation with fmt
std::string scientific = fmt::format("Precise position: {:e}", cameraPosition);
// Output: Precise position: (1.050e+01, 2.030e+01, 5.800e+00)
```

### Real-world Module Integration Examples

```cpp
class TerrainModule : public Module
{
public:
    void OnUpdate() override
    {
        SEDX_PROFILE_SCOPE("TerrainModule::OnUpdate");
        
        // Update terrain chunks based on camera position
        Vec3 cameraPos = GetCameraPosition();
        
        // Log camera position for debugging
        SEDX_CORE_INFO_TAG("TERRAIN", "Camera position: {}", cameraPos);
        
        // Check if we need to update visible chunks
        if (ShouldUpdateChunks(cameraPos))
        {
            SEDX_CORE_INFO_TAG("TERRAIN", "Updating terrain chunks around: {:e}", cameraPos);
            UpdateVisibleChunks(cameraPos);
        }
    }
    
private:
    void LoadHeightmapFromFile(const std::filesystem::path& heightmapPath)
    {
        SEDX_CORE_INFO_TAG("TERRAIN", "Loading heightmap from: {}", heightmapPath);
        
        if (!std::filesystem::exists(heightmapPath))
        {
            SEDX_CORE_ERROR_TAG("TERRAIN", "Heightmap not found: {}", heightmapPath);
            return;
        }
        
        // Load heightmap...
    }
};
```

### UI Debug Panels

```cpp
class DebugOverlay : public Module
{
public:
    void OnUIRender() override
    {
        if (ImGui::Begin("Debug Information"))
        {
            // Display formatted vector information
            Vec3 playerPos = GetPlayerPosition();
            Vec3 playerVel = GetPlayerVelocity();
            
            ImGui::Text("Position: %s", std::format("{}", playerPos).c_str());
            ImGui::Text("Velocity: %s", std::format("{:e}", playerVel).c_str());
            
            // Display path information
            auto currentPath = std::filesystem::current_path();
            ImGui::Text("Working Directory: %s", std::format("{}", currentPath).c_str());
        }
        ImGui::End();
    }
};
```

## Best Practices

### 1. Consistent Format Usage

```cpp
// ✅ Good: Consistent formatting across related data
SEDX_CORE_INFO_TAG("PHYSICS", "Object position: {}, velocity: {}", position, velocity);

// ❌ Avoid: Mixing format styles without reason
SEDX_CORE_INFO_TAG("PHYSICS", "Object position: {:e}, velocity: {}", position, velocity);
```

### 2. Appropriate Precision Context

```cpp
// ✅ Good: Use scientific notation for very large/small values
Vec3 astronomicalDistance(1.234e12f, 5.678e11f, 9.012e10f);
SEDX_CORE_INFO_TAG("SPACE", "Distance to star: {:e}", astronomicalDistance);

// ✅ Good: Use fixed-point for typical game coordinates
Vec3 playerPosition(10.5f, 0.0f, -5.2f);
SEDX_CORE_INFO_TAG("PLAYER", "Player at: {}", playerPosition);
```

### 3. Path Formatting Standards

```cpp
// ✅ Good: Use the formatter for consistent path display
SEDX_CORE_INFO_TAG("ASSETS", "Loading texture: {}", texturePath);

// ❌ Avoid: Manual path string conversion
SEDX_CORE_INFO_TAG("ASSETS", "Loading texture: {}", texturePath.string());
```

### 4. Error Context Enhancement

```cpp
// ✅ Good: Include formatted data in error messages
try
{
    LoadModel(modelPath);
}
catch (const std::exception& e)
{
    SEDX_CORE_ERROR_TAG("MODEL", "Failed to load model from {}: {}", modelPath, e.what());
    throw;
}
```

## Integration with Logging System

The formatter system integrates seamlessly with Scenery Editor X's logging system:

### Tagged Logging Examples

```cpp
// Module initialization with formatted output
void ModelLoaderModule::OnAttach() override
{
    SEDX_CORE_INFO("=== Initializing {} ===", GetName());
    
    auto assetsPath = std::filesystem::current_path() / "assets" / "models";
    SEDX_CORE_INFO_TAG("INIT", "Models directory: {}", assetsPath);
    
    // Load default models
    for (const auto& modelFile : std::filesystem::directory_iterator(assetsPath))
    {
        if (modelFile.path().extension() == ".obj")
        {
            SEDX_CORE_INFO_TAG("MODEL", "Found model: {}", modelFile.path());
        }
    }
}

// Performance critical sections with position data
void PhysicsModule::OnUpdate() override
{
    SEDX_PROFILE_SCOPE("PhysicsModule::OnUpdate");
    
    for (auto& entity : m_PhysicsEntities)
    {
        Vec3 oldPos = entity.position;
        entity.UpdatePhysics(GetDeltaTime());
        
        // Log significant position changes
        Vec3 deltaPos = entity.position - oldPos;
        if (glm::length(deltaPos) > SIGNIFICANT_MOVEMENT_THRESHOLD)
        {
            SEDX_CORE_INFO_TAG("PHYSICS", "Entity {} moved from {} to {}", 
                               entity.id, oldPos, entity.position);
        }
    }
}
```

### Performance Monitoring

```cpp
class PerformanceMonitor : public Module
{
public:
    void OnUpdate() override
    {
        SEDX_PROFILE_SCOPE("PerformanceMonitor::OnUpdate");
        
        // Monitor memory usage
        auto memUsage = GetMemoryUsage();
        if (memUsage.physicalUsed > HIGH_MEMORY_THRESHOLD)
        {
            SEDX_CORE_WARN_TAG("MEMORY", "High memory usage: {} MB", memUsage.physicalUsed / 1024 / 1024);
        }
        
        // Monitor render performance
        Vec3 cameraPos = GetMainCamera().GetPosition();
        float frameTime = GetFrameTime();
        
        if (frameTime > TARGET_FRAME_TIME * 1.5f)
        {
            SEDX_CORE_WARN_TAG("RENDER", "Frame time spike: {:.3f}ms at camera position: {}", 
                               frameTime * 1000.0f, cameraPos);
        }
    }
};
```

## Performance Considerations

### 1. Compile-Time Optimization

The formatter implementations are designed for optimal compile-time performance:

```cpp
// Template specializations are resolved at compile time
constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
{
    // Parse logic is executed during compilation when possible
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && (*it == 'f' || *it == 'e'))
        presentation = *it++;
    
    if (it != end && *it != '}')
        throw format_error("invalid format");
        
    return it;
}
```

### 2. Runtime Performance

- **String allocation**: The formatters use efficient `format_to` calls to minimize temporary string allocations
- **Precision**: Fixed 3-decimal precision strikes a balance between readability and performance
- **Branch prediction**: Simple conditional formatting improves CPU branch prediction

### 3. Memory Usage

```cpp
// ✅ Good: Direct formatting to output iterator
auto format(const Vec3 &vec, FormatContext &ctx) const -> decltype(ctx.out())
{
    return presentation == 'f' 
        ? format_to(ctx.out(), "({:.3f}, {:.3f}, {:.3f})", vec.x, vec.y, vec.z)
        : format_to(ctx.out(), "({:.3e}, {:.3e}, {:.3e})", vec.x, vec.y, vec.z);
}

// ❌ Avoid: Creating temporary strings
std::string format_vector_bad(const Vec3& vec)
{
    std::string temp = std::format("({:.3f}, {:.3f}, {:.3f})", vec.x, vec.y, vec.z);
    return temp; // Unnecessary temporary allocation
}
```

## Error Handling

### Format Error Detection

The formatters include comprehensive error detection:

```cpp
constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
{
    auto it = ctx.begin(), end = ctx.end();
    
    // Validate format specification
    if (it != end && (*it == 'f' || *it == 'e'))
        presentation = *it++;
    
    // Ensure format string is properly terminated
    if (it != end && *it != '}')
        throw format_error("invalid format");
    
    return it;
}
```

### Safe Usage Patterns

```cpp
// ✅ Safe: Validate format strings in debug builds
#ifdef SEDX_DEBUG
    try
    {
        auto result = std::format("Position: {}", playerPosition);
        SEDX_CORE_INFO_TAG("PLAYER", "{}", result);
    }
    catch (const std::format_error& e)
    {
        SEDX_CORE_ERROR_TAG("FORMAT", "Format error: {}", e.what());
    }
#else
    // Release builds: direct formatting for performance
    SEDX_CORE_INFO_TAG("PLAYER", "Position: {}", playerPosition);
#endif
```

## Extending the System

### Adding New Vector Types

To add support for new vector types (e.g., integer vectors):

```cpp
// Example: Adding IVec2 formatter for integer vectors
template <>
struct std::formatter<IVec2>
{
    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
    {
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && *it != '}')
            throw format_error("IVec2 formatter doesn't support format specifications");
        return it;
    }

    template <typename FormatContext>
    auto format(const IVec2 &vec, FormatContext &ctx) const -> decltype(ctx.out())
    {
        return format_to(ctx.out(), "({}, {})", vec.x, vec.y);
    }
};
```

### Adding Matrix Support

```cpp
// Example: Adding Mat4 formatter
template <>
struct std::formatter<Mat4>
{
    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
    {
        // Matrix formatting could support 'c' for compact, 'f' for full
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && (*it == 'c' || *it == 'f'))
            presentation = *it++;
        
        if (it != end && *it != '}')
            throw format_error("invalid format");
            
        return it;
    }

    template <typename FormatContext>
    auto format(const Mat4 &mat, FormatContext &ctx) const -> decltype(ctx.out())
    {
        if (presentation == 'c')
        {
            // Compact: single line
            return format_to(ctx.out(), "Mat4[{:.2f}, {:.2f}, {:.2f}, {:.2f}; ...]", 
                           mat[0][0], mat[0][1], mat[0][2], mat[0][3]);
        }
        else
        {
            // Full: multi-line matrix
            return format_to(ctx.out(), 
                "Mat4[\n  [{:.3f}, {:.3f}, {:.3f}, {:.3f}]\n"
                "     [{:.3f}, {:.3f}, {:.3f}, {:.3f}]\n"
                "     [{:.3f}, {:.3f}, {:.3f}, {:.3f}]\n"
                "     [{:.3f}, {:.3f}, {:.3f}, {:.3f}]\n]",
                mat[0][0], mat[0][1], mat[0][2], mat[0][3],
                mat[1][0], mat[1][1], mat[1][2], mat[1][3],
                mat[2][0], mat[2][1], mat[2][2], mat[2][3],
                mat[3][0], mat[3][1], mat[3][2], mat[3][3]);
        }
    }
    
private:
    char presentation = 'f';
};
```

### Custom Application Types

For application-specific types:

```cpp
// Example: Formatting a custom Transform component
struct Transform
{
    Vec3 position;
    Vec3 rotation;
    Vec3 scale;
};

template <>
struct std::formatter<Transform>
{
    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
    {
        return ctx.begin(); // No format options for Transform
    }

    template <typename FormatContext>
    auto format(const Transform &transform, FormatContext &ctx) const -> decltype(ctx.out())
    {
        return format_to(ctx.out(), "Transform[pos: {}, rot: {}, scale: {}]",
                        transform.position, transform.rotation, transform.scale);
    }
};
```

## Integration Guidelines

When using the formatter system in your modules:

1. **Include the header**: Always include `<SceneryEditorX/utils/formatter.h>` when you need to format vectors or paths
2. **Use tagged logging**: Combine formatting with the logging system for better categorization
3. **Choose appropriate precision**: Use scientific notation for extreme values, fixed-point for typical game data
4. **Profile when necessary**: Use `SEDX_PROFILE_SCOPE` in performance-critical formatting sections
5. **Handle errors gracefully**: Wrap formatting in try-catch blocks during development and testing

## Conclusion

The formatter system provides a robust, extensible foundation for string formatting in Scenery Editor X. By leveraging both standard library and fmt library capabilities, it ensures consistent, readable output across the entire application while maintaining high performance and type safety.

The system integrates seamlessly with the module architecture, logging system, and overall coding standards, making it an essential tool for debugging, logging, and user interface development.

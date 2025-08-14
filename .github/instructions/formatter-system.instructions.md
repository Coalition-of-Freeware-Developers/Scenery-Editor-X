# Formatter System Instructions for GitHub Copilot

## Overview

The Scenery Editor X project uses a sophisticated custom formatter system that provides string formatting capabilities for vector types (`Vec2`, `Vec3`, `Vec4`) and standard library types (`std::filesystem::path`). This system integrates with both `std::format` (C++20) and the `fmt` library to ensure consistent, readable output across the application.

## Key Components

### Location
- **Primary header**: `source/SceneryEditorX/utils/formatter.h`
- **Documentation**: `docs/formatter-system-documentation.md`

### Supported Types
- `Vec2` - 2D floating-point vector
- `Vec3` - 3D floating-point vector  
- `Vec4` - 4D floating-point vector
- `std::filesystem::path` - File system paths

## Mandatory Usage Patterns

### 1. Always Include the Formatter Header

```cpp
#include <SceneryEditorX/utils/formatter.h>
```

When writing any code that formats vectors or paths, ALWAYS include this header to enable custom formatting.

### 2. Use Consistent Vector Formatting

```cpp
// ✅ REQUIRED: Use the custom formatters for all vector output
Vec3 position(10.5f, 20.3f, 5.8f);
SEDX_CORE_INFO_TAG("MODULE", "Position: {}", position);
// Output: Position: (10.500, 20.300, 5.800)

// ❌ NEVER: Manual component formatting
SEDX_CORE_INFO_TAG("MODULE", "Position: ({}, {}, {})", position.x, position.y, position.z);
```

### 3. Standard Format Specifications

Use these format specifiers for vector types:

```cpp
// Default fixed-point notation (3 decimal places)
std::format("Position: {}", vec3Position);

// Scientific notation for extreme values
std::format("Large distance: {:e}", astronomicalVector);
```

### 4. Integration with Logging System

**ALWAYS** use the formatter system with tagged logging:

```cpp
// ✅ REQUIRED pattern for module logging
void CustomModule::OnAttach() override
{
    SEDX_CORE_INFO("=== Initializing {} ===", GetName());
    
    Vec3 initialPosition = GetInitialPosition();
    SEDX_CORE_INFO_TAG("INIT", "Starting position: {}", initialPosition);
    
    auto resourcePath = GetResourceDirectory();
    SEDX_CORE_INFO_TAG("INIT", "Resource directory: {}", resourcePath);
}

void CustomModule::OnUpdate() override
{
    if (!m_IsEnabled) return;
    SEDX_PROFILE_SCOPE("CustomModule::OnUpdate");
    
    // Use formatting in performance monitoring
    Vec3 currentPos = GetCurrentPosition();
    if (HasSignificantMovement(currentPos))
    {
        SEDX_CORE_INFO_TAG("MOVEMENT", "Entity moved to: {}", currentPos);
    }
}
```

### 5. Error Context Enhancement

**ALWAYS** include formatted data in error messages:

```cpp
// ✅ REQUIRED: Enhanced error context with formatting
try
{
    LoadAssetFromPath(assetPath);
}
catch (const std::exception& e)
{
    SEDX_CORE_ERROR_TAG("ASSET", "Failed to load asset from {}: {}", assetPath, e.what());
    throw;
}

// ✅ REQUIRED: Position-based error reporting
if (!IsValidPosition(position))
{
    SEDX_CORE_ERROR_TAG("PHYSICS", "Invalid position detected: {}", position);
    return false;
}
```

### 6. UI Debug Integration

Use formatters in ImGui debug panels:

```cpp
void DebugModule::OnUIRender() override
{
    if (ImGui::Begin("Debug Information"))
    {
        // ✅ REQUIRED: Use formatters for debug display
        Vec3 cameraPos = GetCameraPosition();
        Vec3 cameraDir = GetCameraDirection();
        
        ImGui::Text("Camera Position: %s", std::format("{}", cameraPos).c_str());
        ImGui::Text("Camera Direction: %s", std::format("{}", cameraDir).c_str());
        
        auto workingDir = std::filesystem::current_path();
        ImGui::Text("Working Directory: %s", std::format("{}", workingDir).c_str());
    }
    ImGui::End();
}
```

## Code Generation Guidelines

### When to Use Scientific Notation

```cpp
// ✅ Use scientific notation for:
// - Astronomical distances
Vec3 starPosition(1.234e12f, 5.678e11f, 9.012e10f);
SEDX_CORE_INFO_TAG("SPACE", "Star position: {:e}", starPosition);

// - Very small physics values
Vec3 particleAcceleration(1.23e-6f, 4.56e-7f, 7.89e-8f);
SEDX_CORE_INFO_TAG("PHYSICS", "Acceleration: {:e}", particleAcceleration);

// - High-precision scientific calculations
SEDX_CORE_INFO_TAG("CALC", "Precise result: {:e}", calculationResult);
```

### When to Use Fixed-Point Notation

```cpp
// ✅ Use fixed-point notation (default) for:
// - Game world coordinates
Vec3 playerPosition(10.5f, 0.0f, -5.2f);
SEDX_CORE_INFO_TAG("PLAYER", "Player at: {}", playerPosition);

// - UI positions and sizes
Vec2 buttonSize(120.0f, 30.0f);
SEDX_CORE_INFO_TAG("UI", "Button size: {}", buttonSize);

// - Color values
Vec4 backgroundColor(0.1f, 0.105f, 0.11f, 1.0f);
SEDX_CORE_INFO_TAG("UI", "Background color: {}", backgroundColor);
```

## Module Integration Requirements

### 1. Module Lifecycle Integration

```cpp
class ExampleModule : public Module
{
public:
    void OnAttach() override
    {
        SEDX_CORE_INFO("=== Initializing {} ===", GetName());
        
        // ✅ REQUIRED: Format paths and positions during initialization
        auto configPath = GetConfigPath();
        SEDX_CORE_INFO_TAG("CONFIG", "Loading config from: {}", configPath);
        
        m_DefaultPosition = Vec3(0.0f, 0.0f, 0.0f);
        SEDX_CORE_INFO_TAG("INIT", "Default position set to: {}", m_DefaultPosition);
    }
    
    void OnDetach() override
    {
        SEDX_CORE_INFO("Cleaning up {}", GetName());
        
        // ✅ REQUIRED: Format final state information
        SEDX_CORE_INFO_TAG("CLEANUP", "Final position: {}", m_CurrentPosition);
    }
    
    void OnUpdate() override
    {
        if (!m_IsEnabled) return;
        SEDX_PROFILE_SCOPE("ExampleModule::OnUpdate");
        
        // ✅ REQUIRED: Use formatting for performance monitoring
        UpdateLogic();
        
        if (m_LogPositionUpdates)
        {
            SEDX_CORE_INFO_TAG("UPDATE", "Position updated to: {}", m_CurrentPosition);
        }
    }
    
private:
    Vec3 m_CurrentPosition;
    Vec3 m_DefaultPosition;
    bool m_LogPositionUpdates = false;
};
```

### 2. Resource Management Integration

```cpp
class ResourceManager : public Module
{
private:
    void LoadResource(const std::filesystem::path& resourcePath)
    {
        // ✅ REQUIRED: Always format paths in resource operations
        SEDX_CORE_INFO_TAG("RESOURCE", "Loading resource: {}", resourcePath);
        
        if (!std::filesystem::exists(resourcePath))
        {
            SEDX_CORE_ERROR_TAG("RESOURCE", "Resource not found: {}", resourcePath);
            return;
        }
        
        try
        {
            // Load resource logic...
            SEDX_CORE_INFO_TAG("RESOURCE", "Successfully loaded: {}", resourcePath);
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("RESOURCE", "Failed to load {}: {}", resourcePath, e.what());
            throw;
        }
    }
};
```

## Performance Guidelines

### 1. Efficient Formatting Patterns

```cpp
// ✅ EFFICIENT: Direct formatting in logging calls
SEDX_CORE_INFO_TAG("TAG", "Vector data: {}", vectorData);

// ❌ INEFFICIENT: Creating temporary strings
std::string temp = std::format("Vector data: {}", vectorData);
SEDX_CORE_INFO_TAG("TAG", "{}", temp);
```

### 2. Conditional Formatting for Performance

```cpp
// ✅ EFFICIENT: Conditional detailed logging
if (m_DetailedLoggingEnabled)
{
    SEDX_CORE_INFO_TAG("DETAIL", "Detailed position: {:e}", precisePosition);
}

// ✅ EFFICIENT: Debug-only formatting
#ifdef SEDX_DEBUG
    SEDX_CORE_INFO_TAG("DEBUG", "Debug position: {}", debugPosition);
#endif
```

## Error Handling Requirements

### 1. Format Error Safety

```cpp
// ✅ REQUIRED: Safe formatting with error handling in debug builds
#ifdef SEDX_DEBUG
template<typename... Args>
void SafeLogFormat(const std::string& tag, const std::string& format, Args&&... args)
{
    try
    {
        auto message = std::format(format, std::forward<Args>(args)...);
        SEDX_CORE_INFO_TAG(tag, "{}", message);
    }
    catch (const std::format_error& e)
    {
        SEDX_CORE_ERROR_TAG("FORMAT", "Format error in {}: {}", tag, e.what());
    }
}
#endif
```

### 2. Validation Integration

```cpp
// ✅ REQUIRED: Validate data before formatting operations
bool ValidateAndLogPosition(const Vec3& position, const std::string& context)
{
    if (!IsFinite(position))
    {
        SEDX_CORE_ERROR_TAG("VALIDATION", "Invalid position in {}: {}", context, position);
        return false;
    }
    
    SEDX_CORE_INFO_TAG("VALIDATION", "Valid position in {}: {}", context, position);
    return true;
}
```

## Code Review Checklist

When generating or reviewing code, ensure:

1. **✅ Formatter header included** when using vector or path formatting
2. **✅ Tagged logging used** with formatted output
3. **✅ Appropriate format specifiers** (`:f` for normal, `:e` for scientific)
4. **✅ Error context enhanced** with formatted data
5. **✅ Performance considerations** applied (direct formatting, conditional logging)
6. **✅ Module lifecycle integration** with formatted logging
7. **✅ Path operations use** formatter for consistent display
8. **✅ Debug UI integration** uses formatters for display

## Common Anti-Patterns to Avoid

```cpp
// ❌ NEVER: Manual vector component formatting
std::format("Position: ({}, {}, {})", vec.x, vec.y, vec.z);

// ❌ NEVER: Path string conversion without formatter
SEDX_CORE_INFO_TAG("PATH", "Loading: {}", path.string());

// ❌ NEVER: Mixed format styles without reason
SEDX_CORE_INFO_TAG("DATA", "Pos: {:e}, Vel: {}", position, velocity);

// ❌ NEVER: Temporary string creation for simple logging
auto temp = std::format("Data: {}", vectorData);
SEDX_CORE_INFO_TAG("TAG", temp);
```

## Integration with Module System

Always follow the module pattern when using formatters:

```cpp
// ✅ REQUIRED: Module pattern with formatter integration
class PhysicsModule : public Module
{
public:
    explicit PhysicsModule(const std::string& name = "PhysicsModule")
        : Module(name) {}
    
    void OnAttach() override
    {
        SEDX_CORE_INFO("=== Initializing {} ===", GetName());
        
        m_Gravity = Vec3(0.0f, -9.81f, 0.0f);
        SEDX_CORE_INFO_TAG("PHYSICS", "Gravity set to: {}", m_Gravity);
        
        auto configPath = GetPhysicsConfigPath();
        SEDX_CORE_INFO_TAG("CONFIG", "Physics config: {}", configPath);
    }
    
    void OnUpdate() override
    {
        if (!m_IsEnabled) return;
        SEDX_PROFILE_SCOPE("PhysicsModule::OnUpdate");
        
        // Update physics with formatted logging
        for (auto& entity : m_PhysicsEntities)
        {
            Vec3 oldPos = entity.position;
            UpdateEntityPhysics(entity);
            
            if (m_LogMovement && HasMoved(oldPos, entity.position))
            {
                SEDX_CORE_INFO_TAG("MOVEMENT", "Entity {} moved from {} to {}", 
                                   entity.id, oldPos, entity.position);
            }
        }
    }
    
private:
    Vec3 m_Gravity;
    std::vector<PhysicsEntity> m_PhysicsEntities;
    bool m_LogMovement = false;
};
```

This formatter system is a core component of Scenery Editor X's architecture. Always use it consistently to maintain code quality, debugging capability, and user experience standards.

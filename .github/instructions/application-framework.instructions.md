# Application Framework GitHub Copilot Instructions

## Overview

These instructions guide GitHub Copilot in understanding and implementing the Application framework in Scenery Editor X. The Application framework provides the foundation for all applications in the project and follows specific architectural patterns and best practices.

## Core Application Framework Architecture

### Application Class Pattern

When creating or modifying Application classes, always follow this pattern:

```cpp
class MyApplication : public SceneryEditorX::Application
{
public:
    MyApplication(const AppData& appData) : Application(appData) 
    {
        // Initialize application-specific data
        // Do NOT initialize graphics resources here
    }
  
    virtual ~MyApplication() override
    {
        // Cleanup application-specific resources
        // Graphics cleanup is handled by base class
    }
  
    void OnInit() override
    {
        SEDX_CORE_INFO("=== Initializing {} ===", GetAppData().appName);
      
        try
        {
            // Initialize resources using CreateRef<T>()
            InitializeApplicationResources();
          
            // If using modules, initialize them here
            if (m_UseModuleSystem)
            {
                InitializeModules();
            }
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR("Failed to initialize application: {}", e.what());
            throw;
        }
    }
  
    void OnUpdate() override
    {
        if (!m_IsInitialized) return;
      
        SEDX_PROFILE_SCOPE("MyApplication::OnUpdate");
      
        try
        {
            // Update application logic
            UpdateApplicationLogic();
          
            // Update modules if using module system
            if (m_UseModuleSystem)
            {
                UpdateModules();
            }
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_WARN("Update error: {}", e.what());
        }
    }
  
    void OnShutdown() override
    {
        SEDX_CORE_INFO("=== Shutting Down {} ===", GetAppData().appName);
      
        // Cleanup in reverse order of initialization
        if (m_UseModuleSystem)
        {
            ShutdownModules();
        }
      
        CleanupApplicationResources();
      
        // Call base class shutdown
        Application::OnShutdown();
    }
  
private:
    bool m_IsInitialized = false;
    bool m_UseModuleSystem = false;
};
```

### AppData Configuration Pattern

Always configure AppData properly for different application types:

```cpp
// Editor Application Configuration
AppData CreateEditorConfig()
{
    AppData config;
    config.appName = "Scenery Editor X";
    config.WinWidth = 1280;
    config.WinHeight = 720;
    config.Resizable = true;
    config.StartMaximized = true;
    config.VSync = true;
    config.EnableImGui = true;
  
    return config;
}

// Runtime Application Configuration  
AppData CreateRuntimeConfig()
{
    AppData config;
    config.appName = "Scenery Viewer";
    config.Fullscreen = true;
    config.VSync = true;
    config.EnableImGui = false; // No UI in runtime
  
    return config;
}

// Launcher Configuration
AppData CreateLauncherConfig()
{
    AppData config;
    config.appName = "Scenery Editor X Launcher";
    config.WinWidth = 978;
    config.WinHeight = 526;
    config.NoTitlebar = true;
    config.Resizable = false;
    config.StartMaximized = false;
    config.VSync = true;
  
    return config;
}
```

### CreateApplication Function Pattern

Every application must implement the CreateApplication function:

```cpp
SceneryEditorX::Application* SceneryEditorX::CreateApplication(int argc, char** argv)
{
    // Parse command line arguments
    std::string_view projectPath;
    if (argc > 1)
        projectPath = argv[1];
  
    // Create appropriate configuration
    AppData config = CreateEditorConfig(); // or CreateRuntimeConfig(), etc.
  
    // Validate configuration
    if (!ValidateAppData(config))
    {
        SEDX_CORE_ERROR("Invalid application configuration");
        return nullptr;
    }
  
    // Return application instance
    return new MyApplication(config, projectPath);
}
```

## Module System Integration

When integrating with the Module System:

```cpp
class ModularApplication : public Application
{
public:
    void OnInit() override
    {
        SEDX_CORE_INFO("=== Initializing Modular Application ===");
      
        // Initialize core modules first
        auto rendererModule = CreateRef<RendererModule>();
        m_ModuleStage.PushModule(rendererModule.get());
        m_Modules.push_back(rendererModule);
      
        auto sceneModule = CreateRef<SceneModule>();
        m_ModuleStage.PushModule(sceneModule.get());
        m_Modules.push_back(sceneModule);
      
        // Initialize UI overlays last
        auto debugOverlay = CreateRef<DebugOverlay>();
        m_ModuleStage.PushOverlay(debugOverlay.get());
        m_Modules.push_back(debugOverlay);
      
        // Attach all modules
        for (auto* module : m_ModuleStage)
        {
            SEDX_CORE_INFO_TAG("MODULE", "Attaching {}", module->GetName());
            module->OnAttach();
        }
    }
  
    void OnUpdate() override
    {
        SEDX_PROFILE_SCOPE("ModularApplication::OnUpdate");
      
        for (auto* module : m_ModuleStage)
        {
            if (module->IsEnabled())
            {
                module->OnUpdate();
            }
        }
    }
  
    void OnShutdown() override
    {
        // Detach modules in reverse order
        for (auto it = m_ModuleStage.rbegin(); it != m_ModuleStage.rend(); ++it)
        {
            (*it)->OnDetach();
        }
      
        m_Modules.clear();
        Application::OnShutdown();
    }
  
private:
    ModuleStage m_ModuleStage;
    std::vector<Ref<Module>> m_Modules;
};
```

## Memory Management Rules

### Smart Pointer Usage

- **ALWAYS** use `CreateRef<T>()` for shared ownership
- **ALWAYS** use `CreateScope<T>()` for unique ownership
- **NEVER** use raw `new`/`delete`
- **NEVER** use `std::shared_ptr` or `std::unique_ptr` directly

```cpp
// Correct
auto texture = CreateRef<Texture2D>("path/to/texture.png");
auto buffer = CreateScope<VertexBuffer>(bufferSize);

// Incorrect
auto texture = std::make_shared<Texture2D>("path/to/texture.png");
auto buffer = new VertexBuffer(bufferSize);
```

### Resource Cleanup

Resources are automatically cleaned up by smart pointers, but for explicit cleanup:

```cpp
void CleanupResources()
{
    // Reset smart pointers to release resources
    m_Texture.Reset();
    m_Buffer.Reset();
  
    // Clear containers
    m_Resources.clear();
}
```

## Logging Integration

### Standard Logging Pattern

```cpp
// Core system logging
SEDX_CORE_INFO("General information");
SEDX_CORE_WARN("Warning message");
SEDX_CORE_ERROR("Error occurred");

// Tagged logging (preferred)
SEDX_CORE_INFO_TAG("APPLICATION", "Application '{}' initialized", appName);
SEDX_CORE_ERROR_TAG("GRAPHICS", "Graphics error: {}", errorMessage);
SEDX_CORE_WARN_TAG("MEMORY", "High memory usage: {} MB", memUsage);
```

### Lifecycle Logging

```cpp
void OnAttach() override
{
    SEDX_CORE_INFO("=== Initializing {} ===", GetName());
  
    try
    {
        InitializeResources();
        SEDX_CORE_INFO_TAG("INIT", "Resources loaded for {}", GetName());
    }
    catch (const std::exception& e)
    {
        SEDX_CORE_ERROR_TAG("INIT", "Failed to load resources: {}", e.what());
        throw;
    }
}
```

## Error Handling Patterns

### Application-Level Error Handling

```cpp
void OnInit() override
{
    try
    {
        SEDX_CORE_INFO("Initializing application");
      
        if (!ValidateDependencies())
        {
            SEDX_CORE_ERROR("Dependencies not met");
            m_InitializationFailed = true;
            return;
        }
      
        InitializeResources();
        m_IsInitialized = true;
        SEDX_CORE_INFO("Application initialized successfully");
    }
    catch (const std::exception& e)
    {
        SEDX_CORE_ERROR("Exception during initialization: {}", e.what());
        m_InitializationFailed = true;
    }
}

void OnUpdate() override
{
    if (m_InitializationFailed || !m_IsInitialized)
        return;
      
    try
    {
        PerformUpdate();
    }
    catch (const std::exception& e)
    {
        SEDX_CORE_ERROR("Exception during update: {}", e.what());
    }
}
```

### Assertion Usage

```cpp
// Core assertions - use in system/engine code
SEDX_CORE_ASSERT(condition, "Error message with {}", formatArgs);
SEDX_CORE_ASSERT(pointer != nullptr, "Null pointer detected");

// Application assertions - use in application/editor code
SEDX_ASSERT(condition, "Application assertion failed");

// Verification - continues in release builds
SEDX_CORE_VERIFY(resource->IsValid(), "Resource validation failed");
```

## Performance Integration

### Profiling Scopes

Always include profiling scopes in performance-critical code:

```cpp
void ExpensiveFunction()
{
    SEDX_PROFILE_SCOPE("ExpensiveFunction");
  
    {
        SEDX_PROFILE_SCOPE("InitializationSection");
        InitializeData();
    }
  
    ProcessData();
}

// Application lifecycle methods should be profiled
void OnUpdate() override
{
    if (!m_IsEnabled) return;
    SEDX_PROFILE_SCOPE("MyApplication::OnUpdate");
  
    // Update logic here
}
```

## Configuration Management

### Settings Integration

```cpp
class ConfigurableApplication : public Application
{
public:
    void OnInit() override
    {
        // Load user settings
        m_UserSettings = CreateRef<ApplicationSettings>("settings.cfg");
        if (!m_UserSettings->ReadSettings())
        {
            SEDX_CORE_WARN("Failed to load settings, using defaults");
            m_UserSettings->InitMinConfig();
        }
      
        ApplyUserSettings();
    }
  
    void ApplyUserSettings()
    {
        // Apply settings to application components
        if (m_UserSettings->GetBool("graphics.vsync", true))
        {
            EnableVSync();
        }
      
        auto windowWidth = m_UserSettings->GetInt("window.width", 1280);
        auto windowHeight = m_UserSettings->GetInt("window.height", 720);
        SetWindowSize(windowWidth, windowHeight);
    }
  
private:
    Ref<ApplicationSettings> m_UserSettings;
};
```

## File Organization Rules

### Header Structure

```cpp
/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* filename.h
* -------------------------------------------------------
* Created: DD/MM/YYYY
* -------------------------------------------------------
*/
#pragma once

// System includes
#include <vector>
#include <memory>
#include <string>

// Third-party includes
#include <vulkan/vulkan.h>
#include <imgui.h>

// Project includes
#include <SceneryEditorX/core/application.h>
#include <SceneryEditorX/core/pointers.h>
#include <SceneryEditorX/logging/logging.hpp>

namespace SceneryEditorX
{
    class MyApplication : public Application
    {
        // Implementation
    };
}
```

## Common Application Types

### Editor Application

```cpp
class EditorApplication : public Application
{
public:
    EditorApplication(const AppData& appData, std::string_view projectPath)
        : Application(appData), m_ProjectPath(projectPath) {}
  
    void OnInit() override
    {
        // Initialize editor systems
        InitializeRenderer();
        InitializeUI();
        InitializeAssetManager();
        LoadProject(m_ProjectPath);
    }
  
private:
    std::string m_ProjectPath;
    Ref<UI::UIContext> m_UIContext;
    Ref<AssetManager> m_AssetManager;
};
```

### Launcher Application

```cpp
class LauncherApplication : public Application
{
public:
    LauncherApplication(const AppData& appData) : Application(appData) {}
  
    void OnInit() override
    {
        // Initialize launcher-specific systems
        InitializeLauncherUI();
        CheckForUpdates();
        LoadProjectList();
    }
  
    void OnUpdate() override
    {
        UpdateLauncherUI();
        ProcessLauncherEvents();
    }
};
```

### Runtime Application

```cpp
class RuntimeApplication : public Application
{
public:
    RuntimeApplication(const AppData& appData, const std::string& scenePath)
        : Application(appData), m_ScenePath(scenePath) {}
  
    void OnInit() override
    {
        // Initialize minimal runtime systems
        InitializeRenderer();
        LoadScene(m_ScenePath);
        // No UI initialization for runtime
    }
  
private:
    std::string m_ScenePath;
    Ref<Scene> m_Scene;
};
```

## Key Reminders for GitHub Copilot

1. **Always inherit from Application** for new application types
2. **Use AppData configuration** for window and application settings
3. **Follow the lifecycle pattern** with OnInit, OnUpdate, OnShutdown
4. **Use CreateRef`<T>`() and CreateScope`<T>`()** for memory management
5. **Include tagged logging** for better debugging and tracking
6. **Use profiling scopes** in performance-critical methods
7. **Handle errors gracefully** with try-catch and proper logging
8. **Integrate with Module System** for complex applications
9. **Follow file organization rules** with proper headers and includes
10. **Validate all configuration data** before use

## Integration Guidelines

- Applications should integrate with the existing Window, Renderer, and Module systems
- Use the settings system for persistent user preferences
- Follow the project's error handling and logging conventions
- Implement proper resource management using RAII principles
- Consider thread safety when implementing multi-threaded features
- Use the event system for loose coupling between components

This instruction set ensures consistent, maintainable, and well-architected applications within the Scenery Editor X framework.

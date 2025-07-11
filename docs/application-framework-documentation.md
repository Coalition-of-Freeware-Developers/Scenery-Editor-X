# Scenery Editor X - Application Framework Documentation

---

## Overview

The Application Framework in Scenery Editor X provides a robust foundation for creating cross-platform 3D applications built on modern C++ architecture. This documentation covers the core application system, its lifecycle management, and integration patterns.

---

## Architecture Components

### Core Classes

The application framework consists of three primary components:

1. **`Application`** - The main application class that manages the application lifecycle
2. **`AppData`** - Configuration structure containing application and window settings
3. **`XPlaneStats`** - X-Plane simulator integration configuration

---

## Application Class (`application.h`)

### Purpose

The `Application` class serves as the central hub for all application operations, providing:

- Window management and lifecycle control
- Event loop processing
- Settings management
- Frame timing and synchronization
- Platform abstraction layer

### Class Definition

```cpp
class Application
{
public:
    Application(const AppData &appData);
    virtual ~Application();

    void Run();
    void Stop();

    virtual void OnInit() {}
    virtual void OnUpdate() {}
    virtual void OnShutdown();

    // Accessors
    inline Window GetWindow() const { return *m_Window; }
    uint32_t GetCurrentFrameIndex() const { return currentFrameIndex; }
    const AppData &GetAppData() const { return m_AppData; }
    GLOBAL Application &Get() { return *appInstance; }
    ApplicationSettings &GetSettings() { return settings; }
  
    // Platform information
    GLOBAL const char* GetConfigurationName();
    GLOBAL const char* GetPlatformName();
  
    void RenderUI();
    float GetTime();

private:
    Scope<Window> m_Window;
    WindowData m_WindowData;
    AppData m_AppData;
    std::deque<std::pair<bool, std::function<void()>>> m_EventQueue;
    bool isRunning = true;
    bool isMinimized = false;
    ApplicationSettings settings;
    uint32_t currentFrameIndex = 0;
  
    INTERNAL Application *appInstance;
  
protected:
    inline LOCAL bool isRuntime = false;
};
```

### Key Features

#### Singleton Pattern

The Application class implements a singleton pattern through the static `appInstance` pointer, ensuring only one application instance exists at runtime.

#### Virtual Lifecycle Methods

Applications can override these virtual methods to customize behavior:

- `OnInit()` - Called once during application initialization
- `OnUpdate()` - Called every frame during the main loop
- `OnShutdown()` - Called during application cleanup

#### Window Management

The application automatically creates and manages a `Window` instance, handling:

- Window creation and initialization
- Event polling and processing
- Window state management (minimized, resized, etc.)

### Usage Patterns

#### Basic Application Implementation

```cpp
class MyApplication : public SceneryEditorX::Application
{
public:
    MyApplication(const AppData& appData) : Application(appData) {}
  
    void OnInit() override
    {
        SEDX_CORE_INFO("Initializing application");
        // Initialize your resources here
        InitializeRenderer();
        LoadAssets();
    }
  
    void OnUpdate() override
    {
        // Per-frame logic
        UpdateScene();
        RenderFrame();
    }
  
    void OnShutdown() override
    {
        SEDX_CORE_INFO("Shutting down application");
        // Cleanup resources
        Application::OnShutdown();
    }
};
```

#### Application Creation Function

Every application must implement the `CreateApplication` function:

```cpp
SceneryEditorX::Application* SceneryEditorX::CreateApplication(int argc, char** argv)
{
    AppData windowData;
    windowData.appName = "My Application";
    windowData.WinWidth = 1280;
    windowData.WinHeight = 720;
    windowData.Resizable = true;
    windowData.VSync = true;
  
    return new MyApplication(windowData);
}
```

### Application Lifecycle

#### Initialization Phase

1. **Constructor**: Creates window, applies configuration from AppData
2. **OnInit()**: User-defined initialization code
3. **Window Setup**: Initializes GLFW window and applies changes

#### Runtime Phase

1. **Event Polling**: Processes window and input events
2. **OnUpdate()**: Executes per-frame application logic
3. **Frame Management**: Tracks frame indices and timing

#### Shutdown Phase

1. **OnShutdown()**: User-defined cleanup code
2. **Resource Cleanup**: Destroys window and releases resources
3. **Application Termination**: Sets global running state to false

### Error Handling

The Application class provides robust error handling through:

- Assertion macros for debugging (`SEDX_CORE_ASSERT`)
- Logging integration with tagged messages
- Exception safety in critical code paths

```cpp
void Application::Run()
{
    OnInit();
  
    // Main application loop with error checking
    while (isRunning && !m_Window->GetShouldClose())
    {
        try
        {
            m_Window->Update();
          
            if (isMinimized)
                continue;

            OnUpdate();
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR("Application update failed: {}", e.what());
            Stop();
        }
    }

    OnShutdown();
}
```

### Integration with Module System

The Application class works seamlessly with the Module System:

```cpp
class ModularApplication : public Application
{
public:
    ModularApplication(const AppData& appData) : Application(appData) {}
  
    void OnInit() override
    {
        // Initialize core modules
        auto terrainModule = CreateRef<TerrainModule>();
        m_ModuleStage.PushModule(terrainModule.get());
      
        // Initialize UI overlays
        auto debugOverlay = CreateRef<DebugOverlay>();
        m_ModuleStage.PushOverlay(debugOverlay.get());
      
        // Attach all modules
        for (auto* module : m_ModuleStage)
        {
            module->OnAttach();
        }
    }
  
    void OnUpdate() override
    {
        // Update all modules
        for (auto* module : m_ModuleStage)
        {
            module->OnUpdate();
        }
    }
  
private:
    ModuleStage m_ModuleStage;
    std::vector<Ref<Module>> m_Modules; // Keep references alive
};
```

### Platform Considerations

#### Cross-Platform Support

The Application class abstracts platform differences:

- Windows: Supports both console and WinMain entry points
- macOS: Handles Cocoa application lifecycle
- Linux: Manages X11/Wayland windowing systems

#### Build Configuration Detection

Platform and build information is available at runtime:

```cpp
void Application::LogPlatformInfo()
{
    SEDX_CORE_INFO("Platform: {}", GetPlatformName());
    SEDX_CORE_INFO("Configuration: {}", GetConfigurationName());
}
```

### Best Practices

#### Resource Management

- Use smart pointers (`Ref<T>`, `Scope<T>`) for automatic cleanup
- Initialize resources in `OnInit()`, cleanup in `OnShutdown()`
- Handle window resize events gracefully

#### Performance Optimization

- Use the frame index for temporal effects
- Implement frame skipping when minimized
- Profile critical update paths

#### Debugging Support

- Leverage the logging system with appropriate tags
- Use assertions for development-time validation
- Implement debug UI for runtime inspection

### Error Recovery

```cpp
class RobustApplication : public Application
{
public:
    void OnInit() override
    {
        try
        {
            InitializeRenderer();
            LoadRequiredAssets();
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR("Critical initialization failure: {}", e.what());
            ShowErrorDialog("Failed to initialize application");
            Stop();
        }
    }
  
    void OnUpdate() override
    {
        try
        {
            UpdateApplicationLogic();
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_WARN("Non-critical update error: {}", e.what());
            // Continue running but log the issue
        }
    }
};
```

## Summary

The Application class provides a solid foundation for building complex 3D applications with:

- Clean lifecycle management
- Cross-platform windowing support
- Integration with the Module System
- Robust error handling and logging
- Performance optimization features

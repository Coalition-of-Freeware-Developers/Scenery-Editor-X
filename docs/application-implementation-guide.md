# Scenery Editor X - Application Implementation Guide

---

## Overview

This guide provides comprehensive implementation details for the Application framework in Scenery Editor X, covering the core application lifecycle, event handling, window management, and integration patterns with the broader system architecture.

---

## Implementation Architecture

### Core Implementation (`application.cpp`)

The Application class implementation demonstrates several key patterns and best practices used throughout the Scenery Editor X codebase.

#### Constructor Implementation

```cpp
Application::Application(const AppData &appData)
{
    SEDX_CORE_INFO("Creating application with window: {}x{}", appData.WinWidth, appData.WinHeight);

    // Establish singleton instance
    appInstance = this;

    // Create window using smart pointer system
    m_Window = CreateScope<Window>();

    // Apply configuration from AppData
    if (appData.WinWidth > 0 && appData.WinHeight > 0)
    {
        // Window configuration is applied through the Window class API
        // Due to encapsulation, direct member access is not available
        // Configuration is applied via ApplyChanges() after initialization
    }

    if (!appData.appName.empty())
    {
        m_Window->SetTitle(appData.appName);
        SEDX_CORE_INFO_TAG("Application", "Window title changed to {}", appData.appName);  
    }

    SEDX_CORE_INFO("Initializing Window");
    // Initialize the window system
    m_Window->Init();

    SEDX_CORE_INFO("Window Initialized");
    // Apply any pending configuration changes
    m_Window->ApplyChanges();
    SEDX_CORE_INFO("Window changes applied");

    // Set initial application state
    isRunning = true;
    isMinimized = false;
}
```

#### Key Implementation Details

1. **Singleton Pattern**: The static `appInstance` pointer ensures global access while maintaining object-oriented design
2. **Smart Pointer Usage**: `CreateScope<Window>()` follows the project's memory management guidelines
3. **Logging Integration**: Uses the tagged logging system for detailed tracking
4. **Error Handling**: Defensive programming with state validation

### Main Loop Implementation

The `Run()` method implements the core application lifecycle:

```cpp
void Application::Run()
{
    OnInit();
  
    // Main application loop
    while (isRunning && !m_Window->GetShouldClose())
    {
        // Update the window (poll events)
        m_Window->Update();
      
        // Skip frame if window is minimized
        if (isMinimized)
            continue;

        // Call user-defined update function
        OnUpdate();
    }

    OnShutdown();
}
```

#### Loop Architecture Benefits

1. **Event-Driven Design**: Window events are processed before application logic
2. **Performance Optimization**: Minimized windows skip rendering logic
3. **Clean Lifecycle**: Guaranteed initialization and cleanup phases
4. **User Extensibility**: Virtual methods allow customization without modifying core logic

### Window Integration

The Application class demonstrates proper integration with the Window system:

#### Window State Management

```cpp
void Application::HandleWindowEvents()
{
    // The Window class handles GLFW events internally
    // Application receives state changes through callbacks or polling
  
    if (m_Window->IsMinimized())
    {
        isMinimized = true;
        SEDX_CORE_INFO_TAG("Window", "Application minimized");
    }
    else if (isMinimized && !m_Window->IsMinimized())
    {
        isMinimized = false;
        SEDX_CORE_INFO_TAG("Window", "Application restored");
    }
}
```

#### Window Configuration Application

The implementation shows how to properly apply configuration:

```cpp
void Application::ApplyWindowConfiguration(const AppData& config)
{
    // Window size configuration
    if (config.WinWidth > 0 && config.WinHeight > 0)
    {
        // Note: Direct access to window internals is not available
        // Configuration must be applied through public APIs
        // This demonstrates the importance of proper API design
    }
  
    // Window properties
    if (!config.appName.empty())
    {
        m_Window->SetTitle(config.appName);
    }
  
    // Apply changes after all configuration is set
    m_Window->ApplyChanges();
}
```

### Settings Integration

The Application class integrates with the settings system:

```cpp
class Application
{
private:
    ApplicationSettings settings = ApplicationSettings(std::filesystem::path("settings.cfg"));
  
public:
    ApplicationSettings& GetSettings() { return settings; }
    const ApplicationSettings& GetSettings() const { return settings; }
};
```

#### Settings Usage Pattern

```cpp
void Application::LoadUserPreferences()
{
    try
    {
        if (settings.ReadSettings())
        {
            // Apply user preferences
            ApplyDisplaySettings();
            ApplyInputSettings();
            ApplyPerformanceSettings();
        }
        else
        {
            SEDX_CORE_WARN("Failed to load user settings, using defaults");
            settings.InitMinConfig();
        }
    }
    catch (const std::exception& e)
    {
        SEDX_CORE_ERROR("Settings error: {}", e.what());
    }
}
```

### Platform Abstraction

The implementation provides platform information access:

```cpp
const char* Application::GetConfigurationName()
{
    return SEDX_BUILD_TYPE;
}

const char* Application::GetPlatformName()
{
    return SEDX_PLATFORM_NAME;
}
```

This allows applications to adapt behavior based on build configuration and target platform.

### Error Handling Patterns

#### Graceful Degradation

```cpp
void Application::HandleCriticalError(const std::exception& e)
{
    SEDX_CORE_ERROR("Critical application error: {}", e.what());
  
    // Attempt graceful shutdown
    try
    {
        OnShutdown();
    }
    catch (...)
    {
        SEDX_CORE_ERROR("Error during emergency shutdown");
    }
  
    // Force application termination
    Stop();
}
```

#### Resource Cleanup

```cpp
Application::~Application()
{
    // Manual window cleanup (temporary)
    m_Window->~Window();
  
    // TODO: Re-enable Renderer::Shutdown() once header dependencies are resolved
    // Renderer::Shutdown();
  
    // Note: Smart pointers handle automatic cleanup
    // Manual cleanup is only needed for legacy systems
}
```



---

## Advanced Implementation Patterns

### Event Queue System

The Application class includes an event queue for deferred processing:

```cpp
class Application
{
private:
    std::deque<std::pair<bool, std::function<void()>>> m_EventQueue;
  
public:
    void QueueEvent(std::function<void()> event, bool highPriority = false)
    {
        m_EventQueue.emplace_back(highPriority, std::move(event));
    }
  
    void ProcessEventQueue()
    {
        // Process high-priority events first
        auto it = m_EventQueue.begin();
        while (it != m_EventQueue.end())
        {
            if (it->first) // High priority
            {
                it->second(); // Execute event
                it = m_EventQueue.erase(it);
            }
            else
            {
                ++it;
            }
        }
      
        // Process remaining events
        for (auto& [priority, event] : m_EventQueue)
        {
            event();
        }
        m_EventQueue.clear();
    }
};
```

### Frame Management

The Application tracks frame information for temporal effects:

```cpp
class Application
{
private:
    uint32_t currentFrameIndex = 0;
  
public:
    uint32_t GetCurrentFrameIndex() const { return currentFrameIndex; }
  
    void AdvanceFrame()
    {
        currentFrameIndex++;
        // Handle overflow for long-running applications
        if (currentFrameIndex == UINT32_MAX)
        {
            currentFrameIndex = 0;
            SEDX_CORE_INFO("Frame counter reset after overflow");
        }
    }
};
```

### Integration with Module System

Applications can integrate with the Module System for modular architecture:

```cpp
class ModularApplication : public Application
{
public:
    ModularApplication(const AppData& appData) : Application(appData) {}
  
    void OnInit() override
    {
        SEDX_CORE_INFO("=== Initializing Modular Application ===");
      
        try
        {
            InitializeModules();
            AttachModules();
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR("Module initialization failed: {}", e.what());
            throw;
        }
    }
  
    void OnUpdate() override
    {
        SEDX_PROFILE_SCOPE("ModularApplication::OnUpdate");
      
        // Update all modules
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
        SEDX_CORE_INFO("=== Shutting Down Modular Application ===");
      
        // Detach modules in reverse order
        for (auto it = m_ModuleStage.rbegin(); it != m_ModuleStage.rend(); ++it)
        {
            (*it)->OnDetach();
        }
      
        Application::OnShutdown();
    }
  
private:
    void InitializeModules()
    {
        // Create core modules
        auto rendererModule = CreateRef<RendererModule>();
        auto sceneModule = CreateRef<SceneModule>();
        auto inputModule = CreateRef<InputModule>();
      
        // Store references for lifetime management
        m_Modules.push_back(rendererModule);
        m_Modules.push_back(sceneModule);
        m_Modules.push_back(inputModule);
      
        // Add to module stage
        m_ModuleStage.PushModule(rendererModule.get());
        m_ModuleStage.PushModule(sceneModule.get());
        m_ModuleStage.PushModule(inputModule.get());
      
        // Create UI overlays
        auto debugOverlay = CreateRef<DebugOverlay>();
        m_Modules.push_back(debugOverlay);
        m_ModuleStage.PushOverlay(debugOverlay.get());
    }
  
    void AttachModules()
    {
        for (auto* module : m_ModuleStage)
        {
            SEDX_CORE_INFO_TAG("MODULE", "Attaching module: {}", module->GetName());
            module->OnAttach();
        }
    }
  
    ModuleStage m_ModuleStage;
    std::vector<Ref<Module>> m_Modules;
};
```

### Performance Profiling Integration

The implementation can include performance monitoring:

```cpp
class ProfiledApplication : public Application
{
public:
    void OnUpdate() override
    {
        SEDX_PROFILE_SCOPE("Application::OnUpdate");
      
        auto frameStart = std::chrono::high_resolution_clock::now();
      
        // Application logic
        UpdateApplicationState();
      
        auto frameEnd = std::chrono::high_resolution_clock::now();
        auto frameDuration = std::chrono::duration_cast<std::chrono::microseconds>(frameEnd - frameStart);
      
        // Track frame times for performance analysis
        m_FrameTimes.push_back(frameDuration.count());
        if (m_FrameTimes.size() > 120) // Keep last 2 seconds at 60 FPS
        {
            m_FrameTimes.erase(m_FrameTimes.begin());
        }
      
        // Log performance warnings
        if (frameDuration.count() > 16666) // > 16.666ms (60 FPS threshold)
        {
            SEDX_CORE_WARN_TAG("PERFORMANCE", "Slow frame: {} μs", frameDuration.count());
        }
    }
  
private:
    std::vector<uint64_t> m_FrameTimes;
};
```

### Memory Management Implementation

The Application demonstrates proper memory management patterns:

```cpp
class Application
{
private:
    // Smart pointer usage for automatic cleanup
    Scope<Window> m_Window;           // Unique ownership
  
    // Settings management
    ApplicationSettings settings;      // Value semantics for configuration
  
    // State management
    bool isRunning = true;            // Simple value types
    bool isMinimized = false;
  
    // Singleton management
    INTERNAL Application *appInstance; // Raw pointer for singleton access
  
public:
    // Factory function for application creation
    GLOBAL Application& Get() { return *appInstance; }
  
    // RAII cleanup in destructor
    ~Application()
    {
        // Explicit cleanup for resources not handled by smart pointers
        if (m_Window)
        {
            m_Window->~Window();
        }
      
        // Reset singleton pointer
        appInstance = nullptr;
    }
};
```

### Thread Safety Considerations

For multi-threaded applications:

```cpp
class ThreadSafeApplication : public Application
{
public:
    void QueueThreadSafeEvent(std::function<void()> event)
    {
        std::lock_guard<std::mutex> lock(m_EventMutex);
        m_ThreadSafeEventQueue.push(std::move(event));
    }
  
    void OnUpdate() override
    {
        // Process thread-safe events on main thread
        ProcessThreadSafeEvents();
      
        // Continue with normal update
        Application::OnUpdate();
    }
  
private:
    void ProcessThreadSafeEvents()
    {
        std::lock_guard<std::mutex> lock(m_EventMutex);
      
        while (!m_ThreadSafeEventQueue.empty())
        {
            auto event = std::move(m_ThreadSafeEventQueue.front());
            m_ThreadSafeEventQueue.pop();
          
            // Unlock before executing event
            m_EventMutex.unlock();
            event();
            m_EventMutex.lock();
        }
    }
  
    std::mutex m_EventMutex;
    std::queue<std::function<void()>> m_ThreadSafeEventQueue;
};
```



---

## Best Practices Summary

### Implementation Guidelines

1. **Use Smart Pointers**: Follow the project's memory management system with `CreateRef<T>()` and `CreateScope<T>()`
2. **Leverage Logging**: Use tagged logging for detailed tracking and debugging
3. **Handle Errors Gracefully**: Implement defensive programming with proper exception handling
4. **Follow RAII**: Ensure resources are properly managed through constructors and destructors
5. **Validate Configuration**: Check all input data before use
6. **Use Profiling**: Include profiling scopes in performance-critical code

### Performance Considerations

1. **Minimize Allocations**: Reuse objects where possible
2. **Handle Minimization**: Skip expensive operations when the window is minimized
3. **Frame Pacing**: Implement proper frame timing for consistent performance
4. **Event Batching**: Process events efficiently to avoid stalls

### Integration Patterns

1. **Module System**: Use the Module System for complex applications
2. **Settings System**: Integrate with ApplicationSettings for user preferences
3. **Logging System**: Use the custom logging implementation for debugging
4. **Window System**: Properly integrate with the Window class for platform abstraction

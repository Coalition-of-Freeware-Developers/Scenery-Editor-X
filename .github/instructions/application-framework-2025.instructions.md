# Updated Application Framework GitHub Copilot Instructions

## Overview

These instructions guide GitHub Copilot in understanding and implementing the Application framework in Scenery Editor X. The Application framework provides the foundation for all applications in the project and follows specific architectural patterns and best practices with the latest 2025 updates.

## Core Application Framework Architecture

### Application Class Pattern

When creating or modifying Application classes, always follow this pattern:

```cpp
class MyApplication : public SceneryEditorX::Application
{
public:
    MyApplication(const AppData& appData) : Application(appData) {}
    
    void OnInit() override 
    {
        SEDX_CORE_INFO_TAG("Application", "Initializing {}", GetAppData().appName);
        // Initialize application resources
    }
    
    void OnUpdate() override 
    {
        // Per-frame application logic
    }
    
    void OnShutdown() override 
    {
        SEDX_CORE_INFO_TAG("Application", "Shutting down application");
        Application::OnShutdown(); // Call base class cleanup
    }
    
private:
    // Application-specific members
};
```

### Modern Event System Integration

The Application now includes advanced event handling capabilities:

```cpp
class EventAwareApplication : public Application
{
public:
    void OnInit() override 
    {
        // Register for events using callback system
        AddEventCallback([this](Event& e) {
            EventDispatcher dispatcher(e);
            dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& event) {
                OnWindowResize(event);
                return true;
            });
        });
    }
    
    // Use modern event dispatch system
    void NotifyAssetLoaded(const std::string& assetPath) 
    {
        DispatchEvent<AssetReloadedEvent>(assetPath);
    }
    
    // Queue events for thread-safe processing
    void QueueBackgroundTask(std::function<void()> task) 
    {
        QueueEvent(std::move(task));
    }
    
    void OnUpdate() override 
    {
        // Process events first
        ProcessEvents();
        
        // Then update application logic
        UpdateApplicationLogic();
    }
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
    config.WinWidth = 1920;
    config.WinHeight = 1080;
    config.Resizable = true;
    config.VSync = true;
    return config;
}

// Runtime Application Configuration  
AppData CreateRuntimeConfig()
{
    AppData config;
    config.appName = "X-Plane Scenery Runtime";
    config.WinWidth = 1280;
    config.WinHeight = 720;
    config.Resizable = false;
    config.VSync = false;
    return config;
}
```

### CreateApplication Function Pattern

Every application must implement the CreateApplication function:

```cpp
SceneryEditorX::Application* SceneryEditorX::CreateApplication(int argc, char** argv)
{
    // Configure application based on command line arguments
    AppData config = CreateEditorConfig();
    
    // Parse command line for additional configuration
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--windowed") {
            config.Fullscreen = false;
        }
    }
    
    return new MyApplication(config);
}
```

## Module System Integration

When integrating with the Module System (latest implementation):

```cpp
class ModularApplication : public Application
{
public:
    ModularApplication(const AppData& appData) : Application(appData) {}
    
    void OnInit() override 
    {
        SEDX_CORE_INFO_TAG("Application", "Initializing modular application");
        InitializeModules();
    }
    
    void OnUpdate() override 
    {
        // Update all modules with delta time
        for (auto* module : m_ModuleStage) {
            module->OnUpdate(DeltaTime::FromSeconds(GetTime()));
        }
    }
    
    void OnEvent(Event& event) override 
    {
        // Process events through module system
        for (auto it = m_ModuleStage.end(); it != m_ModuleStage.begin(); ) {
            (*--it)->OnEvent(event);
            if (event.Handled) break;
        }
        
        // Call base class event handling
        Application::OnEvent(event);
    }
    
private:
    void InitializeModules()
    {
        // Create and register modules using smart pointers
        auto rendererModule = CreateRef<RendererModule>();
        auto assetModule = CreateRef<AssetModule>();
        auto uiModule = CreateRef<UIModule>();
        
        // Store references for lifetime management
        m_Modules.push_back(rendererModule);
        m_Modules.push_back(assetModule);
        m_Modules.push_back(uiModule);
        
        // Add to module stage (raw pointers)
        m_ModuleStage.PushModule(rendererModule.get());
        m_ModuleStage.PushModule(assetModule.get());
        m_ModuleStage.PushOverlay(uiModule.get()); // UI as overlay
        
        // Attach all modules
        for (auto* module : m_ModuleStage) {
            module->OnAttach();
        }
    }
    
    std::vector<Ref<Module>> m_Modules; // Lifetime management
};
```

## Memory Management Rules (2025 Update)

### Smart Pointer Usage

- **ALWAYS** use `CreateRef<T>()` for shared ownership with automatic reference counting
- **ALWAYS** use `CreateScope<T>()` for unique ownership
- **ALWAYS** inherit from `RefCounted` for shared objects
- **NEVER** use raw `new`/`delete`
- **NEVER** use `std::shared_ptr` or `std::unique_ptr` directly

```cpp
// Correct - Reference counted shared ownership
auto texture = CreateRef<Texture2D>("path/to/texture.png");
auto material = CreateRef<Material>(texture);

// Correct - Unique ownership for temporary objects
auto buffer = CreateScope<VertexBuffer>(bufferSize);
auto commandList = CreateScope<CommandList>();

// Correct - Weak references for observation
WeakRef<Texture2D> weakTexture = texture;
if (auto lockedTexture = weakTexture.Lock()) {
    lockedTexture->Bind();
}

// Incorrect - Direct STL smart pointer usage
auto texture = std::make_shared<Texture2D>("path/to/texture.png");
auto buffer = std::make_unique<VertexBuffer>(bufferSize);
```

### Resource Cleanup Patterns

```cpp
class ResourceManager : public RefCounted
{
public:
    void LoadResources() 
    {
        // Resources are automatically cleaned up via smart pointers
        m_Texture = CreateRef<Texture2D>("diffuse.png");
        m_Mesh = CreateRef<Mesh>("model.obj");
        
        SEDX_CORE_INFO_TAG("ResourceManager", "Loaded {} resources", 2);
    }
    
    void CleanupResources() 
    {
        // Explicit cleanup if needed
        m_Texture.Reset();
        m_Mesh.Reset();
        
        SEDX_CORE_INFO_TAG("ResourceManager", "Resources cleaned up");
    }
    
private:
    Ref<Texture2D> m_Texture;
    Ref<Mesh> m_Mesh;
};
```

## Logging Integration (2025 Tagged System)

### Tagged Logging Pattern (Always Preferred)

**ALWAYS** use tagged logging for better categorization and filtering:

```cpp
class ApplicationLogger 
{
public:
    void InitializeApplication() 
    {
        SEDX_CORE_INFO_TAG("Application", "Starting application initialization");
        SEDX_CORE_TRACE_TAG("Application", "Loading configuration from: {}", configPath);
        
        try {
            LoadConfiguration();
            SEDX_CORE_INFO_TAG("Application", "Configuration loaded successfully");
        } catch (const std::exception& e) {
            SEDX_CORE_ERROR_TAG("Application", "Failed to load configuration: {}", e.what());
            throw;
        }
    }
    
    void OnGraphicsError(VkResult result) 
    {
        SEDX_CORE_ERROR_TAG("Graphics", "Vulkan operation failed: {} ({})", 
                           VkResultToString(result), static_cast<int32_t>(result));
    }
    
    void LogPerformanceMetrics(float frameTime) 
    {
        SEDX_CORE_TRACE_TAG("Performance", "Frame time: {:.2f}ms ({:.1f} FPS)", 
                           frameTime, 1000.0f / frameTime);
    }
};
```

### Module Lifecycle Logging

```cpp
class ApplicationModule : public Module
{
public:
    explicit ApplicationModule(const std::string& name = "ApplicationModule") 
        : Module(name) {}
    
    void OnAttach() override 
    {
        SEDX_CORE_INFO_TAG("MODULE", "Attaching module: {}", GetName());
        // Module initialization
    }
    
    void OnDetach() override 
    {
        SEDX_CORE_INFO_TAG("MODULE", "Detaching module: {}", GetName());
        // Module cleanup
    }
    
    void OnUpdate(DeltaTime dt) override 
    {
        SEDX_CORE_TRACE_TAG("MODULE", "Updating module: {} (dt: {:.2f}ms)", 
                           GetName(), dt.GetMilliseconds());
        // Module logic
    }
};
```

## Error Handling Patterns

### Application-Level Error Handling

```cpp
class RobustApplication : public Application
{
public:
    void OnInit() override 
    {
        SEDX_CORE_INFO_TAG("Application", "=== Application Initialization ===");
        
        try {
            InitializeCriticalSystems();
            SEDX_CORE_INFO_TAG("Application", "Critical systems initialized");
        } catch (const std::exception& e) {
            SEDX_CORE_FATAL_TAG("Application", "Critical initialization failed: {}", e.what());
            Stop();
            throw;
        }
    }
    
    void OnUpdate() override 
    {
        try {
            UpdateApplicationLogic();
        } catch (const std::exception& e) {
            SEDX_CORE_WARN_TAG("Application", "Non-critical update error: {}", e.what());
            // Continue running but log the issue
        }
    }
    
private:
    void InitializeCriticalSystems() 
    {
        SEDX_CORE_ASSERT(InitializeRenderer(), "Renderer initialization failed");
        SEDX_CORE_ASSERT(InitializeAssetManager(), "Asset manager initialization failed");
        
        if (!SEDX_CORE_VERIFY(LoadCriticalAssets(), "Failed to load some critical assets")) {
            SEDX_CORE_WARN_TAG("Application", "Continuing with fallback assets");
        }
    }
};
```

### Assertion Usage (Updated)

```cpp
void ValidatedOperation(const Ref<Texture>& texture, int width, int height)
{
    // Debug-time validation (removed in release builds)
    SEDX_CORE_ASSERT(texture, "Texture cannot be null");
    SEDX_CORE_ASSERT(width > 0 && height > 0, "Invalid dimensions: {}x{}", width, height);
    
    // Runtime validation for production (always enabled)
    if (!SEDX_CORE_VERIFY(texture->IsValid(), "Texture is invalid: {}", texture->GetPath())) {
        return; // Graceful failure
    }
    
    // Proceed with operation
    texture->Resize(width, height);
    SEDX_CORE_TRACE_TAG("Graphics", "Resized texture {} to {}x{}", texture->GetPath(), width, height);
}
```

## Thread Safety and Event Queue (2025 Update)

### Thread-Safe Event Dispatch

```cpp
class ThreadAwareApplication : public Application
{
public:
    // From background threads - queue events for main thread
    void LoadAssetAsync(const std::string& path) 
    {
        // Queue event for main thread processing
        QueueEvent([this, path]() {
            auto asset = LoadAssetFromDisk(path);
            DispatchEvent<AssetReloadedEvent>(path, asset);
        });
    }
    
    // Synchronize events before main thread processing
    void SynchronizeThreadEvents() 
    {
        SyncEvents(); // Mark queued events as ready for processing
    }
    
    void OnUpdate() override 
    {
        // Process queued events first
        ProcessEvents();
        
        // Then update application logic
        UpdateApplicationLogic();
    }
};
```

## Platform Information Access

```cpp
class PlatformAwareApplication : public Application
{
public:
    void LogPlatformInfo() 
    {
        SEDX_CORE_INFO_TAG("Platform", "Running on: {}", GetPlatformName());
        SEDX_CORE_INFO_TAG("Platform", "Build configuration: {}", GetConfigurationName());
        SEDX_CORE_INFO_TAG("Platform", "Main thread ID: {}", GetMainThreadID());
    }
    
    bool IsMainThread() 
    {
        return Application::IsMainThread();
    }
};
```

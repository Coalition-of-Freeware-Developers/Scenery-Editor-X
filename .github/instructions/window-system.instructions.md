# Window System GitHub Copilot Instructions

This instruction file provides GitHub Copilot with comprehensive guidance for working with the Scenery Editor X window management system, including the Window class, MonitorData class, and related components.

---

## Core Principles

### Always Follow These Patterns

1. **Use the Module System**: Integrate window management as modules inheriting from the Module base class
2. **Smart Pointer Management**: Use `CreateRef<T>()` and `CreateScope<T>()` for all window and monitor objects
3. **Exception Safety**: Implement comprehensive try-catch blocks for all monitor and window operations
4. **Logging Integration**: Use tagged logging (`SEDX_CORE_INFO_TAG`, `SEDX_CORE_ERROR_TAG`) for all window system operations
5. **Error Handling**: Always validate window/monitor state before operations and provide graceful fallbacks

### Memory Management Patterns

```cpp
// ✅ CORRECT - Use factory method with smart pointers
WindowData windowConfig;
windowConfig.title = "Application Window";
windowConfig.width = 1280;
windowConfig.height = 720;

auto window = CreateRef<Window>(Window::Create(windowConfig));
window->Init();

// ✅ CORRECT - Automatic cleanup with smart pointers
window.Reset(); // Explicit cleanup (optional)

// ❌ INCORRECT - Never use raw pointers
Window* window = new Window(windowConfig); // DON'T DO THIS
```

### Module Integration Pattern

```cpp
class WindowModule : public Module
{
public:
    explicit WindowModule() : Module("WindowModule") {}
    
    void OnAttach() override
    {
        SEDX_CORE_INFO_TAG("WINDOW", "Initializing window module");
        
        // Configure window
        WindowData windowData;
        windowData.title = "Scenery Editor X";
        windowData.width = 1280;
        windowData.height = 720;
        windowData.resizable = true;
        windowData.vsync = true;
        
        // Create using factory method
        m_Window = CreateRef<Window>(Window::Create(windowData));
        m_Window->Init();
        
        SEDX_CORE_INFO_TAG("WINDOW", "Window module initialized successfully");
    }
    
    void OnDetach() override
    {
        SEDX_CORE_INFO_TAG("WINDOW", "Cleaning up window module");
        m_Window.Reset();
    }
    
    void OnUpdate() override
    {
        if (!m_Window) return;
        
        SEDX_PROFILE_SCOPE("WindowModule::OnUpdate");
        m_Window->Update();
        
        // Handle window close event
        if (m_Window->GetShouldClose()) {
            Application::Get().Stop();
        }
        
        // Handle framebuffer resize
        if (m_Window->GetFramebufferResized()) {
            HandleFramebufferResize();
        }
    }
    
    Ref<Window> GetWindow() const { return m_Window; }
    
private:
    Ref<Window> m_Window;
    
    void HandleFramebufferResize()
    {
        SEDX_CORE_INFO_TAG("WINDOW", "Handling framebuffer resize");
        m_Window->UpdateFramebufferSize();
        // Recreate render resources as needed
        m_Window->SetFramebufferResized(false);
    }
};
```

---

## Window Creation Guidelines

### Standard Window Creation Pattern

```cpp
class ApplicationWindow
{
public:
    void CreateMainWindow()
    {
        // Configure window properties
        WindowData windowConfig;
        windowConfig.title = "Application Title";
        windowConfig.width = 1280;
        windowConfig.height = 720;
        windowConfig.mode = WindowMode::Windowed;
        windowConfig.resizable = true;
        windowConfig.decorated = true;
        windowConfig.vsync = true;
        
        // Create and initialize
        m_Window = CreateRef<Window>(Window::Create(windowConfig));
        
        // Always call Init() after creation
        m_Window->Init();
        
        // Verify creation success
        if (!m_Window->GetWindow()) {
            SEDX_CORE_ERROR_TAG("WINDOW", "Failed to create main window");
            throw std::runtime_error("Window creation failed");
        }
        
        SEDX_CORE_INFO_TAG("WINDOW", "Main window created successfully");
    }
    
private:
    Ref<Window> m_Window;
};
```

### Multi-Monitor Window Setup

```cpp
class MultiMonitorSetup
{
public:
    void CreateMultiMonitorWindows()
    {
        try {
            // Get monitor information
            MonitorData monitorData;
            auto monitors = monitorData.GetMonitorStats();
            
            if (monitors.empty()) {
                SEDX_CORE_WARN_TAG("MONITOR", "No monitors detected - using fallback configuration");
                CreateFallbackWindow();
                return;
            }
            
            SEDX_CORE_INFO_TAG("MONITOR", "Detected {} monitor(s)", monitors.size());
            
            // Create windows for each monitor
            for (size_t i = 0; i < monitors.size(); ++i) {
                CreateWindowForMonitor(static_cast<int>(i), monitors[i]);
            }
            
        } catch (const std::exception& e) {
            SEDX_CORE_ERROR_TAG("MONITOR", "Exception in multi-monitor setup: {}", e.what());
            CreateFallbackWindow();
        }
    }
    
private:
    void CreateWindowForMonitor(int monitorIndex, const Monitor& monitor)
    {
        WindowData windowConfig;
        windowConfig.title = fmt::format("Window on {}", monitor.monitorID);
        
        if (monitor.isPrimary) {
            // Main window on primary monitor
            windowConfig.width = static_cast<uint32_t>(monitor.resolution.x * 0.8f);
            windowConfig.height = static_cast<uint32_t>(monitor.resolution.y * 0.8f);
            windowConfig.mode = WindowMode::Windowed;
            windowConfig.maximized = true;
        } else {
            // Secondary windows in borderless fullscreen
            windowConfig.width = static_cast<uint32_t>(monitor.resolution.x);
            windowConfig.height = static_cast<uint32_t>(monitor.resolution.y);
            windowConfig.mode = WindowMode::WindowedFullScreen;
            windowConfig.decorated = false;
        }
        
        auto window = CreateRef<Window>(Window::Create(windowConfig));
        window->Init();
        
        m_Windows.push_back(window);
        
        SEDX_CORE_INFO_TAG("WINDOW", "Created window for monitor {} ({}x{})",
                          monitor.monitorID, windowConfig.width, windowConfig.height);
    }
    
    std::vector<Ref<Window>> m_Windows;
};
```

---

## Input Handling Best Practices

### Proper Input Processing

```cpp
class InputHandler
{
public:
    void ProcessInput(Window* window)
    {
        if (!window) {
            SEDX_CORE_WARN_TAG("INPUT", "Cannot process input - window is null");
            return;
        }
        
        // Handle keyboard input
        ProcessKeyboardInput(window);
        
        // Handle mouse input
        ProcessMouseInput(window);
        
        // Handle window-specific events
        ProcessWindowEvents(window);
    }
    
private:
    void ProcessKeyboardInput(Window* window)
    {
        // Continuous input (held keys)
        if (window->IsKeyDown(GLFW_KEY_W)) MoveForward();
        if (window->IsKeyDown(GLFW_KEY_S)) MoveBackward();
        if (window->IsKeyDown(GLFW_KEY_A)) MoveLeft();
        if (window->IsKeyDown(GLFW_KEY_D)) MoveRight();
        
        // Single press events
        if (window->IsKeyPressed(GLFW_KEY_F11)) {
            ToggleFullscreen(window);
        }
        
        if (window->IsKeyPressed(GLFW_KEY_ESCAPE)) {
            SEDX_CORE_INFO_TAG("INPUT", "Escape key pressed - requesting window close");
            // Don't directly close - let application handle
            RequestApplicationExit();
        }
    }
    
    void ProcessMouseInput(Window* window)
    {
        Vec2 mouseDelta = window->GetDeltaMouse();
        
        if (window->IsMouseDown(GLFW_MOUSE_BUTTON_RIGHT)) {
            // Camera rotation with sensitivity scaling
            float sensitivity = 0.1f;
            RotateCamera(mouseDelta.x * sensitivity, mouseDelta.y * sensitivity);
        }
        
        if (window->IsMouseDown(GLFW_MOUSE_BUTTON_MIDDLE)) {
            // Camera panning
            float panSpeed = 0.01f;
            PanCamera(mouseDelta.x * panSpeed, mouseDelta.y * panSpeed);
        }
    }
    
    void ToggleFullscreen(Window* window)
    {
        static WindowMode previousMode = WindowMode::Windowed;
        
        if (WindowData::mode == WindowMode::FullScreen) {
            SEDX_CORE_INFO_TAG("WINDOW", "Switching from fullscreen to windowed mode");
            WindowData::mode = previousMode;
        } else {
            SEDX_CORE_INFO_TAG("WINDOW", "Switching to fullscreen mode");
            previousMode = WindowData::mode;
            WindowData::mode = WindowMode::FullScreen;
        }
        
        window->ChangeWindowMode();
    }
};
```

---

## Monitor Management Guidelines

### Safe Monitor Operations

```cpp
class MonitorManager
{
public:
    bool ValidateMonitorConfiguration()
    {
        try {
            MonitorData monitorData;
            
            // Check basic monitor availability
            if (monitorData.GetMonitorCount() <= 0) {
                SEDX_CORE_ERROR_TAG("MONITOR", "No monitors detected");
                return false;
            }
            
            // Validate primary monitor
            if (!monitorData.GetPrimaryMonitor()) {
                SEDX_CORE_ERROR_TAG("MONITOR", "No primary monitor available");
                return false;
            }
            
            // Get detailed monitor stats
            auto monitors = monitorData.GetMonitorStats();
            if (monitors.empty()) {
                SEDX_CORE_ERROR_TAG("MONITOR", "Failed to retrieve monitor statistics");
                return false;
            }
            
            // Validate each monitor
            for (const auto& monitor : monitors) {
                if (!ValidateMonitor(monitor)) {
                    SEDX_CORE_WARN_TAG("MONITOR", "Invalid monitor detected: {}", monitor.monitorID);
                }
            }
            
            SEDX_CORE_INFO_TAG("MONITOR", "Monitor configuration validated successfully");
            return true;
            
        } catch (const std::exception& e) {
            SEDX_CORE_ERROR_TAG("MONITOR", "Exception during monitor validation: {}", e.what());
            return false;
        }
    }
    
private:
    bool ValidateMonitor(const Monitor& monitor)
    {
        if (monitor.resolution.x <= 0 || monitor.resolution.y <= 0) {
            SEDX_CORE_ERROR_TAG("MONITOR", "Invalid resolution: {}x{}", 
                               monitor.resolution.x, monitor.resolution.y);
            return false;
        }
        
        if (monitor.refreshRate <= 0) {
            SEDX_CORE_WARN_TAG("MONITOR", "Invalid refresh rate: {}Hz", monitor.refreshRate);
            // Don't fail validation - some systems report 0
        }
        
        if (!monitor.handle) {
            SEDX_CORE_ERROR_TAG("MONITOR", "Invalid monitor handle");
            return false;
        }
        
        return true;
    }
};
```

### Video Mode Selection

```cpp
class VideoModeSelector
{
public:
    struct VideoModePreference
    {
        int preferredWidth = 1920;
        int preferredHeight = 1080;
        int preferredRefreshRate = 60;
        bool requireExactMatch = false;
    };
    
    bool SelectOptimalVideoMode(int monitorIndex, const VideoModePreference& pref)
    {
        try {
            MonitorData monitorData;
            
            // Validate monitor index
            if (monitorIndex < 0 || monitorIndex >= monitorData.GetMonitorCount()) {
                SEDX_CORE_ERROR_TAG("MONITOR", "Invalid monitor index: {}", monitorIndex);
                return false;
            }
            
            // Get available video modes
            int modeCount = 0;
            const GLFWvidmode* modes = monitorData.GetVideoModes(monitorIndex, &modeCount);
            
            if (!modes || modeCount == 0) {
                SEDX_CORE_ERROR_TAG("MONITOR", "No video modes available for monitor {}", monitorIndex);
                return false;
            }
            
            // Find best matching mode
            int bestModeIndex = FindBestVideoMode(modes, modeCount, pref);
            
            if (bestModeIndex >= 0) {
                const GLFWvidmode& selectedMode = modes[bestModeIndex];
                
                SEDX_CORE_INFO_TAG("MONITOR", "Selected video mode: {}x{} @ {}Hz",
                                  selectedMode.width, selectedMode.height, selectedMode.refreshRate);
                
                monitorData.SetVideoModeIndex(bestModeIndex);
                return true;
            }
            
            SEDX_CORE_ERROR_TAG("MONITOR", "No suitable video mode found");
            return false;
            
        } catch (const std::exception& e) {
            SEDX_CORE_ERROR_TAG("MONITOR", "Exception in video mode selection: {}", e.what());
            return false;
        }
    }
    
private:
    int FindBestVideoMode(const GLFWvidmode* modes, int count, const VideoModePreference& pref)
    {
        int bestIndex = -1;
        int bestScore = INT_MAX;
        
        for (int i = 0; i < count; ++i) {
            const GLFWvidmode& mode = modes[i];
            
            if (pref.requireExactMatch) {
                if (mode.width == pref.preferredWidth && 
                    mode.height == pref.preferredHeight &&
                    mode.refreshRate == pref.preferredRefreshRate) {
                    return i; // Exact match found
                }
                continue;
            }
            
            // Calculate similarity score
            int widthDiff = abs(mode.width - pref.preferredWidth);
            int heightDiff = abs(mode.height - pref.preferredHeight);
            int refreshDiff = abs(mode.refreshRate - pref.preferredRefreshRate);
            
            int score = widthDiff + heightDiff + (refreshDiff * 10); // Weight refresh rate
            
            if (score < bestScore) {
                bestScore = score;
                bestIndex = i;
            }
        }
        
        return bestIndex;
    }
};
```

---

## Error Handling Patterns

### Window Creation Error Handling

```cpp
class RobustWindowCreation
{
public:
    Ref<Window> CreateWindowWithFallback(const WindowData& preferredConfig)
    {
        // Try with preferred configuration
        try {
            auto window = CreateRef<Window>(Window::Create(preferredConfig));
            window->Init();
            
            if (window->GetWindow()) {
                SEDX_CORE_INFO_TAG("WINDOW", "Window created with preferred configuration");
                return window;
            }
        } catch (const std::exception& e) {
            SEDX_CORE_WARN_TAG("WINDOW", "Failed to create window with preferred config: {}", e.what());
        }
        
        // Try with reduced configuration
        WindowData fallbackConfig = preferredConfig;
        fallbackConfig.width = 1024;
        fallbackConfig.height = 768;
        fallbackConfig.mode = WindowMode::Windowed;
        fallbackConfig.decorated = true;
        fallbackConfig.resizable = true;
        
        try {
            auto window = CreateRef<Window>(Window::Create(fallbackConfig));
            window->Init();
            
            if (window->GetWindow()) {
                SEDX_CORE_INFO_TAG("WINDOW", "Window created with fallback configuration");
                return window;
            }
        } catch (const std::exception& e) {
            SEDX_CORE_WARN_TAG("WINDOW", "Failed to create window with fallback config: {}", e.what());
        }
        
        // Final attempt with minimal configuration
        WindowData minimalConfig;
        minimalConfig.title = "Scenery Editor X (Safe Mode)";
        minimalConfig.width = 800;
        minimalConfig.height = 600;
        minimalConfig.mode = WindowMode::Windowed;
        minimalConfig.decorated = true;
        minimalConfig.resizable = true;
        
        try {
            auto window = CreateRef<Window>(Window::Create(minimalConfig));
            window->Init();
            
            if (window->GetWindow()) {
                SEDX_CORE_INFO_TAG("WINDOW", "Window created with minimal configuration");
                return window;
            }
        } catch (const std::exception& e) {
            SEDX_CORE_ERROR_TAG("WINDOW", "All window creation attempts failed: {}", e.what());
        }
        
        SEDX_CORE_ERROR_TAG("WINDOW", "Unable to create any window configuration");
        throw std::runtime_error("Window creation completely failed");
    }
};
```

---

## Performance Optimization Guidelines

### Efficient Update Patterns

```cpp
class OptimizedWindowManager
{
public:
    void UpdateWindows()
    {
        SEDX_PROFILE_SCOPE("WindowManager::UpdateWindows");
        
        for (auto& window : m_Windows) {
            if (!window) continue;
            
            // Update window state
            window->Update();
            
            // Handle resize only when needed
            if (window->GetFramebufferResized()) {
                HandleWindowResize(window);
            }
            
            // Process input efficiently
            if (window->IsDirty()) {
                ProcessWindowInput(window);
                // Window will clear dirty flag automatically
            }
        }
    }
    
private:
    void HandleWindowResize(Ref<Window> window)
    {
        SEDX_PROFILE_SCOPE("WindowManager::HandleResize");
        
        window->UpdateFramebufferSize();
        
        // Recreate render resources only if necessary
        RecreateRenderResources(window);
        
        window->SetFramebufferResized(false);
        
        SEDX_CORE_INFO_TAG("WINDOW", "Window resize handled: {}x{}", 
                          window->GetWidth(), window->GetHeight());
    }
    
    std::vector<Ref<Window>> m_Windows;
};
```

---

## Threading Considerations

### Thread-Safe Window Operations

```cpp
class ThreadSafeWindowOperations
{
public:
    void SafeWindowOperationFromWorkerThread()
    {
        // ❌ NEVER do window operations directly from worker threads
        // window->Update(); // DON'T DO THIS
        
        // ✅ Queue operations for main thread
        Application::Get().QueueMainThreadOperation([this]() {
            if (m_Window) {
                m_Window->Update();
            }
        });
    }
    
    void SafeRenderContextAccess()
    {
        // ✅ Render context access is thread-safe
        if (m_Window) {
            auto renderContext = m_Window->GetRenderContext(); // SAFE
            auto& swapChain = m_Window->GetSwapChain();        // SAFE
            
            // Perform rendering operations...
        }
    }
    
private:
    Ref<Window> m_Window;
};
```

---

## Common Patterns to Avoid

### ❌ INCORRECT Patterns

```cpp
// DON'T create windows without smart pointers
Window* window = new Window(windowData); // BAD

// DON'T forget to call Init()
auto window = CreateRef<Window>(Window::Create(windowData));
// Missing window->Init(); // BAD

// DON'T ignore error states
auto window = CreateRef<Window>(Window::Create(windowData));
window->Init();
// Should check if creation succeeded // BAD

// DON'T perform window operations from worker threads
std::thread([window]() {
    window->Update(); // BAD - not thread safe
}).detach();
```

### ✅ CORRECT Patterns

```cpp
// DO use smart pointers with factory method
auto window = CreateRef<Window>(Window::Create(windowData));

// DO always call Init() and check success
window->Init();
if (!window->GetWindow()) {
    throw std::runtime_error("Window creation failed");
}

// DO use module pattern for integration
class WindowModule : public Module { /* ... */ };

// DO queue operations for main thread
Application::Get().QueueMainThreadOperation([window]() {
    window->Update();
});
```

---

## Key Reminders for Copilot

1. **Always use Window::Create() factory method** - never construct Window directly
2. **Always call Init() after window creation** and verify success
3. **Use smart pointers** (`CreateRef<Window>()`) for all window objects
4. **Implement as modules** inheriting from Module base class
5. **Handle exceptions** around all monitor and window operations
6. **Use tagged logging** for all window system operations
7. **Validate state** before performing window operations
8. **Queue window operations** for main thread when in worker threads
9. **Handle framebuffer resize** events properly in update loops
10. **Provide fallback configurations** for robust window creation

These patterns ensure robust, maintainable, and efficient window management code that follows Scenery Editor X architectural principles.

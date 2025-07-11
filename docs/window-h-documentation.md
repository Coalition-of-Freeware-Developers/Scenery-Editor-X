# Window.h - Core Window Management API Documentation

---

## Overview

The `Window.h` header file defines the core window management system for Scenery Editor X, providing a comprehensive interface for creating and managing application windows across multiple platforms. This system is built on top of GLFW and integrates seamlessly with the Vulkan rendering pipeline, offering full control over window properties, monitor configuration, and input handling.

The window system follows the modular architecture principles of Scenery Editor X, using the custom smart pointer system and logging infrastructure for robust, maintainable code.

---

## Core Components

### WindowMode Enumeration

```cpp
enum class WindowMode : uint8_t
{
    Windowed,           ///< Standard windowed mode with decorations
    WindowedFullScreen, ///< Borderless fullscreen that matches desktop resolution
    FullScreen          ///< True fullscreen with monitor mode switching
};
```

**Usage Guidelines:**

- `Windowed`: Use for normal application operation with window controls
- `WindowedFullScreen`: Use for seamless fullscreen without resolution changes
- `FullScreen`: Use for performance-critical fullscreen applications

### WindowData Structure

The `WindowData` structure contains all configuration parameters for window creation and runtime state management:

```cpp
struct WindowData
{
    GLOBAL inline WindowMode mode = WindowMode::Windowed;
    GLOBAL GLFWwindow *window;
    std::string title = "Scenery Editor X";
    uint32_t width = 1280;
    uint32_t height = 720;
    int posX = 0;
    int posY = 30;
    bool framebufferResized = false;
    bool dirty = true;
    bool resizable = true;
    bool decorated = true;
    bool maximized = true;
    bool focused = true;
    bool vsync = false;
    bool startMaximized = false;
    float scroll = .0f;
    float deltaScroll = .0f;
    Vec2 mousePos = Vec2(.0f, .0f);
    Vec2 deltaMousePos = Vec2(.0f, .0f);
};
```

**Key Properties:**

- **Window Geometry**: `width`, `height`, `posX`, `posY` control window size and position
- **State Flags**: `resizable`, `decorated`, `maximized` control window behavior
- **Input Tracking**: `mousePos`, `deltaMousePos`, `scroll`, `deltaScroll` for input handling
- **Render State**: `framebufferResized`, `dirty`, `vsync` for rendering coordination

### WindowCallbacks Structure

Comprehensive callback system for window and input events:

```cpp
struct WindowCallbacks
{
    void (*scrollCallback)(GLFWwindow *window, double x, double y) = nullptr;
    void (*keyCallback)(GLFWwindow *window, int key, int scancode, int action, int mods) = nullptr;
    void (*mouseButtonCallback)(GLFWwindow *window, int button, int action, int mods) = nullptr;
    void (*cursorPosCallback)(GLFWwindow *window, double x, double y) = nullptr;
    void (*frameBufferSizeCallback)(GLFWwindow *window, int width, int height) = nullptr;
    void (*framebufferResizeCallback)(GLFWwindow *window, int width, int height) = nullptr;
    void (*windowMaximizeCallback)(GLFWwindow *window, int maximize) = nullptr;
    void (*windowChangePosCallback)(GLFWwindow *window, int x, int y) = nullptr;
    void (*mousePositionCallback)(GLFWwindow *window, double x, double y) = nullptr;
    void (*windowDropCallback)(GLFWwindow *window, int count, const char *paths[]) = nullptr;
    void (*windowCloseCallback)(GLFWwindow *window) = nullptr;
    void (*windowFocusCallback)(GLFWwindow *window, int focused) = nullptr;
    void (*windowIconifyCallback)(GLFWwindow *window, int iconified) = nullptr;
    void (*windowSizeCallback)(GLFWwindow *window, int width, int height) = nullptr;
    void (*charCallback)(GLFWwindow *window, unsigned int codepoint) = nullptr;
};
```

---

## Window Class API

### Construction and Lifecycle

#### Constructors

```cpp
Window();                        // Default constructor with default WindowData
Window(WindowData winData);      // Constructor with custom configuration
```

#### Factory Method

```cpp
GLOBAL Window* Create(const WindowData &windowSpecs = WindowData());
```

**Best Practice Example:**

```cpp
// Create window with custom specifications
WindowData windowSpecs;
windowSpecs.title = "My Application";
windowSpecs.width = 1920;
windowSpecs.height = 1080;
windowSpecs.mode = WindowMode::Windowed;
windowSpecs.resizable = true;
windowSpecs.decorated = true;

// Use factory method for consistent creation
auto window = CreateRef<Window>(Window::Create(windowSpecs));
window->Init();
```

### Core Window Operations

#### Initialization

```cpp
virtual void Init();
```

**Description:** Initializes GLFW, creates the window, sets up monitor configuration, and establishes render context.

**Implementation Details:**

- Initializes GLFW if not already done
- Configures window hints based on WindowData settings
- Creates window in specified mode (windowed, borderless fullscreen, or fullscreen)
- Sets up render context and Vulkan integration
- Registers all window and input callbacks
- Configures ImGui cursor support

#### Update Cycle

```cpp
virtual void Update();
```

**Description:** Per-frame update method that handles input polling, timing, and state management.

**Responsibilities:**

- Updates keyboard state tracking for all keys
- Calculates frame delta time
- Updates mouse position and movement deltas
- Polls GLFW events
- Resets per-frame values (like deltaScroll)

### Window State Management

#### Display Mode Control

```cpp
virtual void ChangeWindowMode();
virtual void ApplyChanges();
virtual void Maximize();
virtual void CenterWindow();
```

**ChangeWindowMode() Usage:**

```cpp
// Switch to fullscreen mode
WindowData::mode = WindowMode::FullScreen;
window->ChangeWindowMode();

// Switch back to windowed
WindowData::mode = WindowMode::Windowed;
window->ChangeWindowMode();
```

#### Property Accessors

```cpp
virtual const std::string &GetTitle() const;
virtual void SetTitle(const std::string &title);
virtual void SetResizable(bool resizable) const;
virtual VkExtent2D GetSize() const;

uint32_t GetWidth();
uint32_t GetHeight();
Vec2 GetDeltaMouse();
bool GetFramebufferResized();
bool IsDirty();
bool GetShouldClose();
GLOBAL float GetDeltaTime();
```

### Input Handling

#### Keyboard Input

```cpp
bool IsKeyDown(uint16_t keyCode);
bool IsKeyPressed(uint16_t keyCode) const;
```

**Usage Example:**

```cpp
// Check if key is currently held down
if (window->IsKeyDown(GLFW_KEY_W)) {
    // Handle continuous movement
    ProcessForwardMovement();
}

// Check for single key press event
if (window->IsKeyPressed(GLFW_KEY_SPACE)) {
    // Handle one-time action
    ProcessJumpAction();
}
```

#### Mouse Input

```cpp
bool IsMouseDown(uint16_t buttonCode);
Vec2 GetDeltaMouse();
```

**Mouse Tracking Example:**

```cpp
// Get mouse movement delta for camera control
Vec2 mouseDelta = window->GetDeltaMouse();
if (window->IsMouseDown(GLFW_MOUSE_BUTTON_RIGHT)) {
    camera.Rotate(mouseDelta.x * sensitivity, mouseDelta.y * sensitivity);
}
```

### Rendering Integration

#### Render Context Access

```cpp
virtual Ref<RenderContext> GetRenderContext();
virtual SwapChain &GetSwapChain();
RenderData GetRenderData();
```

#### Framebuffer Management

```cpp
void UpdateFramebufferSize();
void SetFramebufferResized(const bool resized);
bool GetFramebufferResized();
```

**Framebuffer Resize Handling:**

```cpp
// In your render loop
if (window->GetFramebufferResized()) {
    window->UpdateFramebufferSize();
    swapChain.Recreate();
    window->SetFramebufferResized(false);
}
```

---

## Integration Examples

### Basic Application Setup

```cpp
class MyApplication : public Application
{
public:
    void Initialize() override
    {
        // Configure window
        WindowData windowData;
        windowData.title = "My Scenery Editor Application";
        windowData.width = 1280;
        windowData.height = 720;
        windowData.resizable = true;
        windowData.vsync = true;
      
        // Create and initialize window
        m_Window = CreateScope<Window>(windowData);
        m_Window->Init();
      
        // Setup callbacks
        SetupWindowCallbacks();
    }
  
private:
    Scope<Window> m_Window;
  
    void SetupWindowCallbacks()
    {
        // Register custom callbacks for application-specific handling
        // Callbacks are managed internally by the Window class
    }
};
```

### Multi-Monitor Application

```cpp
class MultiMonitorApp
{
public:
    void InitializeWindows()
    {
        // Get monitor information
        MonitorData monitorData;
        auto monitors = monitorData.GetMonitorStats();
      
        for (const auto& monitor : monitors) {
            WindowData windowData;
            windowData.title = fmt::format("Window on {}", monitor.monitorID);
            windowData.width = static_cast<uint32_t>(monitor.resolution.x);
            windowData.height = static_cast<uint32_t>(monitor.resolution.y);
          
            if (monitor.isPrimary) {
                windowData.mode = WindowMode::Windowed;
            } else {
                windowData.mode = WindowMode::WindowedFullScreen;
            }
          
            auto window = CreateRef<Window>(windowData);
            window->Init();
            m_Windows.push_back(window);
        }
    }
  
private:
    std::vector<Ref<Window>> m_Windows;
};
```

### Input Event Processing

```cpp
class InputHandler
{
public:
    void ProcessInput(Window* window)
    {
        // Handle keyboard input
        ProcessKeyboardInput(window);
      
        // Handle mouse input
        ProcessMouseInput(window);
      
        // Handle scroll input
        ProcessScrollInput(window);
    }
  
private:
    void ProcessKeyboardInput(Window* window)
    {
        // Movement keys (continuous)
        if (window->IsKeyDown(GLFW_KEY_W)) MoveForward();
        if (window->IsKeyDown(GLFW_KEY_S)) MoveBackward();
        if (window->IsKeyDown(GLFW_KEY_A)) MoveLeft();
        if (window->IsKeyDown(GLFW_KEY_D)) MoveRight();
      
        // Action keys (single press)
        if (window->IsKeyPressed(GLFW_KEY_F11)) {
            ToggleFullscreen(window);
        }
      
        if (window->IsKeyPressed(GLFW_KEY_ESCAPE)) {
            window->SetShouldClose(true);
        }
    }
  
    void ProcessMouseInput(Window* window)
    {
        Vec2 mouseDelta = window->GetDeltaMouse();
      
        if (window->IsMouseDown(GLFW_MOUSE_BUTTON_RIGHT)) {
            // Camera rotation
            m_Camera.Rotate(mouseDelta.x * m_Sensitivity, mouseDelta.y * m_Sensitivity);
        }
      
        if (window->IsMouseDown(GLFW_MOUSE_BUTTON_MIDDLE)) {
            // Camera panning
            m_Camera.Pan(mouseDelta.x * m_PanSpeed, mouseDelta.y * m_PanSpeed);
        }
    }
  
    void ToggleFullscreen(Window* window)
    {
        static WindowMode previousMode = WindowMode::Windowed;
      
        if (WindowData::mode == WindowMode::FullScreen) {
            WindowData::mode = previousMode;
        } else {
            previousMode = WindowData::mode;
            WindowData::mode = WindowMode::FullScreen;
        }
      
        window->ChangeWindowMode();
    }
  
private:
    Camera m_Camera;
    float m_Sensitivity = 0.1f;
    float m_PanSpeed = 0.01f;
};
```

---

## Best Practices and Guidelines

### Window Creation

1. **Always use the factory method** `Window::Create()` for consistent initialization
2. **Initialize immediately after creation** to ensure proper resource setup
3. **Configure WindowData before creation** rather than modifying after initialization
4. **Use smart pointers** (`CreateRef<Window>()` or `CreateScope<Window>()`) for memory management

### Event Handling

1. **Register callbacks early** in the initialization process
2. **Handle window resize events** to maintain proper rendering state
3. **Check for window closure** in your main loop
4. **Use IsKeyPressed() for single actions** and IsKeyDown() for continuous actions

### Performance Considerations

1. **Enable VSync** for consistent frame pacing unless benchmarking
2. **Handle framebuffer resize efficiently** by recreating only necessary resources
3. **Monitor window dirty state** to avoid unnecessary rendering updates
4. **Use appropriate window modes** based on application requirements

### Error Handling

1. **Check window creation success** before proceeding with initialization
2. **Handle monitor configuration changes** gracefully
3. **Validate input parameters** before window operations
4. **Use logging macros** for debugging and error reporting

### Multi-Platform Considerations

1. **Test window behavior** on all target platforms
2. **Handle platform-specific window decorations** appropriately
3. **Consider DPI scaling** for high-resolution displays
4. **Validate monitor enumeration** on multi-monitor setups

---

## Integration with Other Systems

### Module System Integration

```cpp
class WindowModule : public Module
{
public:
    explicit WindowModule() : Module("WindowModule") {}
  
    void OnAttach() override
    {
        SEDX_CORE_INFO_TAG("WINDOW", "Initializing window module");
      
        // Setup window configuration
        WindowData windowData;
        windowData.title = "Scenery Editor X";
        windowData.width = 1280;
        windowData.height = 720;
      
        // Create window using factory
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
      
        // Handle window events
        if (m_Window->GetShouldClose()) {
            Application::Get().Stop();
        }
    }
  
    Ref<Window> GetWindow() const { return m_Window; }
  
private:
    Ref<Window> m_Window;
};
```

### Renderer Integration

```cpp
class RenderModule : public Module
{
public:
    void OnAttach() override
    {
        // Get window from window module
        auto windowModule = Application::Get().GetModule<WindowModule>();
        m_Window = windowModule->GetWindow();
      
        // Initialize renderer with window context
        m_RenderContext = m_Window->GetRenderContext();
        InitializeRenderer();
    }
  
    void OnUpdate() override
    {
        // Handle framebuffer resize
        if (m_Window->GetFramebufferResized()) {
            RecreateSwapChain();
            m_Window->SetFramebufferResized(false);
        }
      
        // Render frame
        RenderFrame();
    }
  
private:
    Ref<Window> m_Window;
    Ref<RenderContext> m_RenderContext;
};
```

---

## Common Use Cases

### Game-Style Applications

```cpp
// Setup for game-style applications with fullscreen support
WindowData gameWindowData;
gameWindowData.title = "Game Application";
gameWindowData.width = 1920;
gameWindowData.height = 1080;
gameWindowData.mode = WindowMode::FullScreen;  // Start in fullscreen
gameWindowData.vsync = true;                   // Enable VSync for smooth gameplay
gameWindowData.resizable = false;             // Lock resolution for consistency
```

### Editor Applications

```cpp
// Setup for editor-style applications with flexible windowing
WindowData editorWindowData;
editorWindowData.title = "Scenery Editor X";
editorWindowData.width = 1280;
editorWindowData.height = 720;
editorWindowData.mode = WindowMode::Windowed;  // Start windowed for flexibility
editorWindowData.resizable = true;            // Allow resizing for workflow
editorWindowData.maximized = true;            // Start maximized
editorWindowData.decorated = true;            // Keep window decorations
```

### Presentation Applications

```cpp
// Setup for presentation/demo applications
WindowData presentationWindowData;
presentationWindowData.title = "Presentation";
presentationWindowData.mode = WindowMode::WindowedFullScreen; // Borderless fullscreen
presentationWindowData.vsync = true;                         // Smooth presentation
presentationWindowData.decorated = false;                   // No window decorations
```

---

## Threading Considerations

The Window class is designed to be used from the main thread only, as GLFW requires all window operations to occur on the main thread. However, the render context and associated resources can be accessed from render threads with proper synchronization:

```cpp
class ThreadSafeWindowOperations
{
public:
    void SafeUpdateFromRenderThread()
    {
        // DON'T do this - window operations must be on main thread
        // window->Update(); // ❌ NOT SAFE
      
        // DO this - queue operations for main thread
        Application::Get().QueueMainThreadOperation([this]() {
            m_Window->Update();
        });
    }
  
    void SafeRenderOperations()
    {
        // This is safe from render thread
        auto renderContext = m_Window->GetRenderContext(); // ✅ SAFE
        auto& swapChain = m_Window->GetSwapChain();        // ✅ SAFE
      
        // Perform rendering operations...
    }
  
private:
    Ref<Window> m_Window;
};
```

---

## Memory Management

The Window class integrates with the Scenery Editor X smart pointer system for automatic memory management:

```cpp
// Correct memory management patterns
class WindowManager
{
public:
    void CreateWindows()
    {
        // Use CreateRef for shared ownership
        m_MainWindow = CreateRef<Window>(Window::Create(mainWindowData));
      
        // Use CreateScope for exclusive ownership
        m_DialogWindow = CreateScope<Window>(Window::Create(dialogWindowData));
      
        // Initialize windows
        m_MainWindow->Init();
        m_DialogWindow->Init();
    }
  
    void Cleanup()
    {
        // Explicit cleanup (optional - automatic with smart pointers)
        m_DialogWindow.Reset();
        m_MainWindow.Reset();
      
        // Smart pointers handle actual memory deallocation
    }
  
private:
    Ref<Window> m_MainWindow;      // Shared ownership
    Scope<Window> m_DialogWindow;  // Exclusive ownership
};
```

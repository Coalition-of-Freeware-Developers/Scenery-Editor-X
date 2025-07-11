# Window.cpp - Window Implementation Documentation

---

## Overview

The `Window.cpp` file contains the complete implementation of the Window class, providing cross-platform window management functionality for Scenery Editor X. This implementation is built on GLFW and integrates seamlessly with Vulkan rendering, following the project's modular architecture principles and using the custom smart pointer system for memory management.

The implementation handles complex scenarios including multi-monitor setups, window mode transitions, input event processing, and robust error handling across different operating systems.

---

## Implementation Architecture

### Static Variables and Global State

```cpp
// Global GLFW window handle - accessible across the application
GLFWwindow* WindowData::window = nullptr;

// Global initialization flag - prevents multiple GLFW initializations
GLOBAL bool windowInit = false;

// Static timing and input tracking
INTERNAL inline std::chrono::high_resolution_clock::time_point lastTime;
INTERNAL inline std::vector<std::string> pathsDrop;
INTERNAL inline float deltaTime = .0f;
INTERNAL inline char lastKeyState[GLFW_KEY_LAST + 1];
INTERNAL inline WindowMode mode = WindowMode::Windowed;
```

**Design Rationale:**

- GLFW requires a single global context per application
- Static state enables consistent behavior across window instances
- Thread-safe access patterns ensure stability in multi-threaded scenarios

### Error Handling Strategy

```cpp
GLOBAL void WindowErrorCallback(int error, const char *description)
{
    /// Filter out joystick-related errors (codes around 65539 GLFW_INVALID_ENUM)
    if (error == 0x10003 && strstr(description, "joystick"))
    {
        /// Silently ignore joystick-related GLFW_INVALID_ENUM errors
        return;
    }
    SEDX_CORE_ERROR_TAG("Window", "GLFW Error ({0}): {1}", error, description);
}
```

**Special Considerations:**

- Filters joystick errors to prevent log spam with flight simulator hardware
- Uses tagged logging for better error categorization
- Maintains application stability despite GLFW errors

---

## Constructor Implementation

### Default Constructor

```cpp
Window::Window() : winData(WindowData{}) {}
```

**Characteristics:**

- Lightweight initialization with default parameters
- No resource allocation until `Init()` is called
- Follows RAII principles for safe construction

### Parameterized Constructor

```cpp
Window::Window(WindowData winData) : winData(std::move(winData))
{
}
```

**Benefits:**

- Uses move semantics for efficient parameter passing
- Stores configuration without immediate resource allocation
- Allows configuration validation before window creation

### Factory Method Implementation

```cpp
Window *Window::Create(const WindowData &windowSpecs)
{
    return new Window(windowSpecs);
}
```

**Usage Pattern:**

```cpp
// Recommended usage with smart pointers
auto window = CreateRef<Window>(Window::Create(windowSpecs));
window->Init();
```

---

## Window Initialization Process

### GLFW Initialization

The `Init()` method implements a comprehensive initialization sequence:

```cpp
void Window::Init()
{
    m_winSpecs.title = winData.title;
    m_winSpecs.width = winData.width;
    m_winSpecs.height = winData.height;
  
    /// Initialize GLFW if not already initialized
    if (!windowInit)
    {
        SEDX_CORE_INFO("Initializing GLFW");
        int success = glfwInit();
        if (!success)
        {
            SEDX_CORE_ERROR("Failed to initialize GLFW!");
            return;
        }
        glfwSetErrorCallback(WindowErrorCallback);
        windowInit = true;
        SEDX_CORE_INFO("GLFW initialized successfully");
    }
}
```

**Key Implementation Details:**

- Single GLFW initialization per application instance
- Error callback registration for debugging
- Graceful failure handling with logging

### Window Hints Configuration

```cpp
/// Set window hints
glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
glfwWindowHint(GLFW_RESIZABLE, winData.resizable ? GLFW_TRUE : GLFW_FALSE);

if (!winData.decorated)
{
    SEDX_CORE_INFO("Creating window without decorations");
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
}
```

**Configuration Strategy:**

- Sets `GLFW_NO_API` for Vulkan compatibility
- Conditional decoration based on window data
- Proper boolean to GLFW constant conversion

### Monitor Integration

```cpp
/// Initialize monitor data
MonitorData monitorData;
bool monitorInitSuccess = false;

try
{
    monitorData.RefreshMonitorList();
    monitorInitSuccess = true;
    SEDX_CORE_INFO("Monitor data initialized successfully");
}
catch (const std::exception& e)
{
    SEDX_CORE_ERROR("Failed to initialize monitor data: {}", e.what());
    monitorInitSuccess = false;
}
```

**Robust Monitor Handling:**

- Exception-safe monitor initialization
- Graceful degradation if monitor data unavailable
- Comprehensive logging for troubleshooting

### Window Creation Logic

The implementation includes a sophisticated fallback system for window creation:

```cpp
bool windowCreated = false;

/// First attempt - create with specified settings
if (WindowData::mode == WindowMode::FullScreen && monitorInitSuccess)
{
    if (GLFWmonitor* primaryMonitor = monitorData.GetPrimaryMonitor())
    {
        if (const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor))
        {
            SEDX_CORE_INFO("Creating fullscreen window on primary monitor: {}x{} @ {}Hz", 
                          mode->width, mode->height, mode->refreshRate);
            glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
            glfwWindowHint(GLFW_RED_BITS, mode->redBits);
            glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
            glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
            glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
            WindowData::window = glfwCreateWindow(mode->width, mode->height, 
                                                 winData.title.c_str(), primaryMonitor, nullptr);
            windowCreated = (WindowData::window != nullptr);
        }
    }
}

/// If fullscreen creation failed or not in fullscreen mode, create windowed
if (!windowCreated) {
    SEDX_CORE_INFO("Creating window in windowed mode: {}x{}", 
                   static_cast<int>(winData.width), static_cast<int>(winData.height));
    WindowData::window = glfwCreateWindow(static_cast<int>(winData.width), 
                                         static_cast<int>(winData.height), 
                                         winData.title.c_str(), nullptr, nullptr);
    windowCreated = (WindowData::window != nullptr);
}

// Final fallback - try creating a minimal window
if (!windowCreated) {
    SEDX_CORE_WARN("Window creation failed with specified parameters, trying fallback settings");
    // Reset window hints to defaults
    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  
    // Try creating a basic window
    WindowData::window = glfwCreateWindow(800, 600, "Scenery Editor X (Fallback)", nullptr, nullptr);
    windowCreated = (WindowData::window != nullptr);
  
    if (windowCreated) {
        SEDX_CORE_INFO("Created fallback window successfully");
        m_winSpecs.width = 800;
        m_winSpecs.height = 600;
        winData.width = 800;
        winData.height = 600;
    } else {
        SEDX_CORE_ERROR("All window creation attempts failed!");
        return;
    }
}
```

**Fallback Strategy Benefits:**

1. **Primary Attempt**: Use specified mode and monitor configuration
2. **Secondary Attempt**: Fall back to windowed mode with specified dimensions
3. **Final Attempt**: Create minimal window with safe defaults
4. **Graceful Failure**: Log errors and maintain application stability

---

## Callback System Implementation

### Callback Registration

The implementation registers comprehensive callbacks for all window and input events:

```cpp
// Set all the callbacks
glfwSetWindowCloseCallback(WindowData::window, windowCallbacks.windowCloseCallback);
glfwSetFramebufferSizeCallback(WindowData::window, windowCallbacks.framebufferResizeCallback);
glfwSetWindowPos(WindowData::window, winData.posX, winData.posY);
glfwSetCharCallback(WindowData::window, windowCallbacks.charCallback);
glfwSetCursorPosCallback(WindowData::window, windowCallbacks.cursorPosCallback);
glfwSetKeyCallback(WindowData::window, windowCallbacks.keyCallback);
glfwSetMouseButtonCallback(WindowData::window, windowCallbacks.mouseButtonCallback);
glfwSetScrollCallback(WindowData::window, windowCallbacks.scrollCallback);
glfwSetWindowMaximizeCallback(WindowData::window, windowCallbacks.windowMaximizeCallback);
glfwSetWindowPosCallback(WindowData::window, windowCallbacks.windowChangePosCallback);
glfwSetDropCallback(WindowData::window, windowCallbacks.windowDropCallback);
glfwSetWindowIconifyCallback(WindowData::window, windowCallbacks.windowIconifyCallback);
```

### Specialized Callback Implementations

#### Framebuffer Resize Callback

```cpp
void Window::FramebufferResizeCallback(GLFWwindow *window, int width, int height)
{
    // Retrieve the Window instance from the GLFW user pointer
    if (auto windowInstance = static_cast<Window *>(glfwGetWindowUserPointer(window))) {
        windowInstance->winData.width = width;
        windowInstance->winData.height = height;
        windowInstance->winData.framebufferResized = true;
        SEDX_CORE_INFO("Window framebuffer resized to: {}x{}", width, height);
    }
}
```

**Key Features:**

- Safe pointer retrieval from GLFW user data
- Automatic window dimension updates
- Framebuffer resize flag for render pipeline notification

#### Mouse Position Callback

```cpp
void Window::MousePositionCallback(GLFWwindow* window, double x, double y)
{
    auto windowInstance = static_cast<Window *>(glfwGetWindowUserPointer(window));
    if (windowInstance->captureMovement)
    {
        windowInstance->winData.mousePos.x = static_cast<float>(x);
        windowInstance->winData.mousePos.y = static_cast<float>(y);

        const auto pointerX = static_cast<float>(x);
        const auto pointerY = static_cast<float>(y);
        if (windowInstance->initState)
        {
            windowInstance->winData.deltaMousePos.x = pointerX;
            windowInstance->winData.deltaMousePos.y = pointerY;
            windowInstance->initState = false;
        }

        float xOffset = x - pointerX - windowInstance->winData.deltaMousePos.x;
        float yOffset = windowInstance->winData.deltaMousePos.y - pointerY; /// Invert the sign here

        windowInstance->winData.deltaMousePos.x = pointerX;
        windowInstance->winData.deltaMousePos.y = pointerY;

        xOffset *= 0.01;
        yOffset *= 0.01;

        //TODO: Finish rest of mouse position callback when camera is implemented
    }
}
```

**Implementation Features:**

- Conditional movement capture for different interaction modes
- Delta calculation with sensitivity scaling
- Y-axis inversion for proper camera control
- Initialization state handling

---

## Window State Management

### Mode Switching Implementation

```cpp
void Window::ChangeWindowMode()
{
    /// Apply changes to the window based on the current mode
    ApplyChanges();
  
    /// Update the swap chain if it exists
    if (swapChain)
    {
        //swapChain->Present();
    }
  
    SEDX_CORE_INFO("Window mode changed to: {}", static_cast<int>(mode));
}
```

### Complex Mode Application

The `ApplyChanges()` method implements sophisticated window mode transitions:

```cpp
void Window::ApplyChanges()
{
    if (!WindowData::window) {
        SEDX_CORE_WARN("Cannot apply window changes - window not created yet");
        return;
    }
  
    /// Create a MonitorData instance to access monitor information
    MonitorData monitorData;
  
    try {
        monitorData.RefreshDisplayCount();
        monitorData.RefreshMonitorList();
    } catch (const std::exception& e) {
        SEDX_CORE_ERROR("Failed to refresh monitor data: {}", e.what());
        return;
    }
  
    /// Get the current monitor
    GLFWmonitor* currentMonitor = monitorData.GetCurrentMonitor();
    if (!currentMonitor)
    {
        SEDX_CORE_ERROR("Failed to get current monitor");
        return;
    }
  
    // ... validation logic ...
  
    /// Apply window configuration based on current mode
    switch (mode)
    {
        case WindowMode::Windowed:
            winData.posY = std::max(winData.posY, 31);
            glfwSetWindowMonitor(WindowData::window, nullptr, winData.posX, winData.posY, 
                               m_winSpecs.width, m_winSpecs.height, GLFW_DONT_CARE);
          
            if (winData.maximized)
                glfwMaximizeWindow(WindowData::window);

            glfwSetWindowAttrib(WindowData::window, GLFW_MAXIMIZED, winData.maximized ? GLFW_TRUE : GLFW_FALSE);
            glfwSetWindowAttrib(WindowData::window, GLFW_RESIZABLE, winData.resizable ? GLFW_TRUE : GLFW_FALSE);
            glfwSetWindowAttrib(WindowData::window, GLFW_DECORATED, winData.decorated ? GLFW_TRUE : GLFW_FALSE);
            break;
          
        case WindowMode::WindowedFullScreen:
            glfwSetWindowMonitor(WindowData::window, currentMonitor, 0, 0, 
                               monitorMode->width, monitorMode->height, monitorMode->refreshRate);
            break;
          
        case WindowMode::FullScreen:
            if (videoModes && videoModeIndex < modesCount)
            {
                const GLFWvidmode videoMode = videoModes[videoModeIndex];
                glfwSetWindowMonitor(WindowData::window, currentMonitor, 0, 0, 
                                   videoMode.width, videoMode.height, videoMode.refreshRate);
            }
            else
                SEDX_CORE_ERROR("Invalid video mode index or no video modes available");
            break;
    }
  
    winData.framebufferResized = false;
    winData.dirty = false;
}
```

**Key Implementation Features:**

- Exception-safe monitor data access
- Comprehensive error checking and validation
- Mode-specific window configuration
- Proper state flag management

---

## Update Loop Implementation

### Per-Frame Update Logic

```cpp
void Window::Update()
{
    if (!WindowData::window) {
        SEDX_CORE_WARN("Cannot update window - window not created yet");
        return;
    }
  
    for (auto i = 0; i < GLFW_KEY_LAST + 1; i++)
        lastKeyState[i] = static_cast<char>(glfwGetKey(WindowData::window, i));

    winData.deltaScroll = 0;
    auto newTime = std::chrono::high_resolution_clock::now();
    deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(newTime - lastTime).count();
    deltaTime /= 1000.0f;
    lastTime = newTime;

    double x, y;
    glfwGetCursorPos(WindowData::window, &x, &y);

    float prevX = winData.mousePos.x;
    float prevY = winData.mousePos.y;
    winData.deltaMousePos.x = prevX - static_cast<float>(x);
    winData.deltaMousePos.y = prevY - static_cast<float>(y);
  
    winData.mousePos.x = static_cast<float>(x);
    winData.mousePos.y = static_cast<float>(y);

    glfwPollEvents();
}
```

**Update Responsibilities:**

1. **Input State Tracking**: Update keyboard state for all keys
2. **Timing Calculation**: High-precision delta time calculation
3. **Mouse Tracking**: Position and movement delta calculation
4. **Event Processing**: Poll GLFW events for callback triggers
5. **State Reset**: Clear per-frame values like scroll delta

---

## Input Processing Implementation

### Keyboard Input Implementation

```cpp
bool Window::IsKeyPressed(const uint16_t keyCode) const
{
    if (!WindowData::window) {
        return false;
    }
    return lastKeyState[keyCode] && !glfwGetKey(WindowData::window, keyCode);
}
```

**Input State Logic:**

- `IsKeyDown()`: Current frame key state from GLFW
- `IsKeyPressed()`: Transition from pressed to released (single-frame event)
- State tracking array maintains previous frame state

### Mouse Input Processing

```cpp
void Window::MouseClickCallback(GLFWwindow *window, int button, int action, int mods)
{
    auto windowInstance = static_cast<Window *>(glfwGetWindowUserPointer(window));
    if (windowInstance->captureMovement)
    {
        GLFWcursor *hand = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
        GLFWcursor *cursor = glfwCreateStandardCursor(GLFW_CURSOR_NORMAL);

        if (button == GLFW_MOUSE_BUTTON_RIGHT)
        {
            if (action == GLFW_PRESS)
            {
                windowInstance->mousePressed = true;
                glfwSetCursor(window, hand);
            }
            else if (action == GLFW_RELEASE)
            {
                windowInstance->mousePressed = false;
                glfwSetCursor(window, cursor);
            }
        }
    }
}
```

**Mouse Interaction Features:**

- Cursor style changes based on interaction state
- Right-click capture for camera movement
- Visual feedback through cursor modification

---

## Resource Management Implementation

### Window Icon Implementation

```cpp
void Window::SetWindowIcon(GLFWwindow *window)
{
    if (!window) {
        SEDX_CORE_WARN("Cannot set window icon - window handle is null");
        return;
    }
  
    IconData iconData;

    std::ifstream file(iconData.path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        SEDX_CORE_ERROR("Failed to open icon file: {}", iconData.path);
        return;
    }

    /// Get file size and read the data
    const std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    iconData.buffer.resize(size);
    if (!file.read(reinterpret_cast<char *>(iconData.buffer.data()), size))
    {
        SEDX_CORE_ERROR("Failed to read icon file data!");
        return;
    }

    iconData.pixels = stbi_load_from_memory(iconData.buffer.data(),
                                          static_cast<int>(size),
                                          &iconData.width,
                                          &iconData.height,
                                          &iconData.channels,
                                          4);

    if (iconData.pixels)
    {
        const GLFWimage icon = {iconData.width, iconData.height, iconData.pixels};
        glfwSetWindowIcon(window, 1, &icon);
        stbi_image_free(iconData.pixels);
        iconData.pixels = nullptr;
        SEDX_CORE_INFO("Window icon set successfully");
    }
    else
        SEDX_CORE_ERROR("Failed to load window icon!");
}
```

**Icon Loading Process:**

1. **File Reading**: Binary file reading with error checking
2. **Image Decoding**: STB_image for PNG decoding
3. **GLFW Integration**: Proper GLFWimage structure creation
4. **Memory Management**: Automatic cleanup with stbi_image_free

### Shutdown Implementation

```cpp
void Window::Shutdown()
{
    if (WindowData::window) {
        glfwDestroyWindow(WindowData::window);
        WindowData::window = nullptr;
    }
  
    if (renderContext) {
        renderContext.As<RenderContext>()->GetLogicDevice()->Destroy();
    }
  
    if (windowInit) {
        glfwTerminate();
        windowInit = false;
    }
}
```

**Cleanup Sequence:**

1. **Window Destruction**: Safe GLFW window cleanup
2. **Render Context**: Vulkan device cleanup
3. **GLFW Termination**: Global library cleanup
4. **State Reset**: Flag reset for reinitialization

---

## Specialized Features Implementation

### Joystick Handling Disabled

```cpp
void Window::DisableJoystickHandling()
{
    /// Detach any registered joystick callback
    glfwSetJoystickCallback(nullptr);
    SEDX_CORE_INFO_TAG("Window", "Joystick handling disabled to prevent conflicts with flight simulator hardware");
}
```

**Rationale:**

- Flight simulator hardware often causes GLFW joystick enumeration errors
- Disabling prevents error spam in logs
- Specialized hardware should use dedicated APIs

### ImGui Cursor Integration

```cpp
// Create mouse cursors for ImGui
ImGuiMouseCursors[ImGuiMouseCursor_Arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
ImGuiMouseCursors[ImGuiMouseCursor_TextInput] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
ImGuiMouseCursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);   // FIXME: GLFW doesn't have this.
ImGuiMouseCursors[ImGuiMouseCursor_ResizeNS] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
ImGuiMouseCursors[ImGuiMouseCursor_ResizeEW] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
ImGuiMouseCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);  // FIXME: GLFW doesn't have this.
ImGuiMouseCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);  // FIXME: GLFW doesn't have this.
ImGuiMouseCursors[ImGuiMouseCursor_Hand] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
```

**Integration Benefits:**

- Seamless ImGui integration for UI applications
- Proper cursor style feedback for UI interactions
- Platform-appropriate cursor styles

---

## Performance Considerations

### Efficient State Updates

```cpp
// Update window dimensions efficiently
int width, height;
glfwGetWindowSize(WindowData::window, &width, &height);
m_winSpecs.width = width;
m_winSpecs.height = height;
```

### Delta Time Calculation

```cpp
auto newTime = std::chrono::high_resolution_clock::now();
deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(newTime - lastTime).count();
deltaTime /= 1000.0f;  // Convert to milliseconds
lastTime = newTime;
```

**High-Precision Timing:**

- Uses `std::chrono::high_resolution_clock` for accurate timing
- Microsecond precision converted to milliseconds
- Consistent frame timing across platforms

---

## Error Handling Patterns

### Defensive Programming

```cpp
if (!WindowData::window) {
    SEDX_CORE_WARN("Cannot apply window changes - window not created yet");
    return;
}
```

### Exception Safety

```cpp
try {
    monitorData.RefreshDisplayCount();
    monitorData.RefreshMonitorList();
} catch (const std::exception& e) {
    SEDX_CORE_ERROR("Failed to refresh monitor data: {}", e.what());
    return;
}
```

**Error Handling Strategy:**

- Null pointer checks before operations
- Exception handling for external library calls
- Graceful degradation with appropriate logging
- State validation before critical operations

---

## Integration Examples

### Application Integration

```cpp
class WindowApplication
{
public:
    void Initialize()
    {
        // Create window with custom configuration
        WindowData config;
        config.title = "Advanced Application";
        config.width = 1920;
        config.height = 1080;
      
        m_Window = CreateRef<Window>(Window::Create(config));
        m_Window->Init();
      
        // Setup application-specific callbacks
        SetupCallbacks();
    }
  
    void RunMainLoop()
    {
        while (!m_Window->GetShouldClose())
        {
            // Update window state
            m_Window->Update();
          
            // Handle framebuffer resize
            if (m_Window->GetFramebufferResized()) {
                HandleResize();
            }
          
            // Process input
            ProcessInput();
          
            // Render frame
            Render();
        }
    }
  
private:
    Ref<Window> m_Window;
  
    void HandleResize()
    {
        m_Window->UpdateFramebufferSize();
        RecreateRenderResources();
        m_Window->SetFramebufferResized(false);
    }
  
    void ProcessInput()
    {
        // Handle application-specific input
        if (m_Window->IsKeyPressed(GLFW_KEY_F11)) {
            ToggleFullscreen();
        }
      
        if (m_Window->IsKeyPressed(GLFW_KEY_ESCAPE)) {
            m_Window->SetShouldClose(true);
        }
    }
};
```

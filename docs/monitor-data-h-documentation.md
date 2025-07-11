# Monitor_Data.h - Monitor Management API Documentation

---

## Overview

The `Monitor_Data.h` header file defines the comprehensive monitor management system for Scenery Editor X, providing robust multi-monitor support and display configuration capabilities. This system enables applications to query monitor properties, manage video modes, and handle complex multi-monitor setups with proper error handling and cross-platform compatibility.

The monitor system integrates seamlessly with the Window management system and follows Scenery Editor X architectural principles, using the custom logging system and memory management patterns.

---

## Core Data Structures

### Monitor Structure

```cpp
struct Monitor
{
    std::string monitorID;  ///< Monitor identifier/name from the system
    Vec2 resolution;        ///< Resolution in pixels (width, height)
    Vec2 dimensions;        ///< Physical size in inches (width, height)
    Vec2 pixDensity;        ///< Pixel density in points per inch (PPI)
    int refreshRate;        ///< Monitor refresh rate in Hz
    bool isPrimary;         ///< True if this is the primary/main monitor
    GLFWmonitor* handle;    ///< Direct handle to the GLFW monitor
};
```

**Property Details:**

- **monitorID**: System-provided monitor name/identifier for display purposes
- **resolution**: Pixel dimensions representing the monitor's native resolution
- **dimensions**: Physical size in inches, calculated from GLFW's millimeter values
- **pixDensity**: Pixels per inch (PPI) for proper DPI scaling calculations
- **refreshRate**: Native refresh rate for optimal display synchronization
- **isPrimary**: Indicates the system's primary display for window placement
- **handle**: Direct GLFW monitor handle for low-level operations

**Usage Example:**

```cpp
MonitorData monitorData;
auto monitors = monitorData.GetMonitorStats();

for (const auto& monitor : monitors) {
    SEDX_CORE_INFO("Monitor: {} ({}x{} @ {}Hz, {:.1f}\" diagonal, {:.0f} PPI)",
                   monitor.monitorID,
                   monitor.resolution.x, monitor.resolution.y,
                   monitor.refreshRate,
                   sqrt(monitor.dimensions.x * monitor.dimensions.x + 
                        monitor.dimensions.y * monitor.dimensions.y),
                   monitor.pixDensity.x);
}
```

---

## MonitorData Class API

### Constructor and Destructor

```cpp
MonitorData();
~MonitorData();
```

**Constructor Behavior:**

- Automatically initializes monitor discovery
- Calls `RefreshDisplayCount()` and `RefreshMonitorList()`
- Handles initialization failures gracefully with warning logs
- Continues with default values if monitor enumeration fails

**Destructor Behavior:**

- Automatic cleanup (GLFW manages monitor handle lifetime)
- No explicit resource deallocation required
- Exception-safe destruction

### Monitor Discovery and Enumeration

#### Core Discovery Methods

```cpp
void RefreshDisplayCount();
void RefreshMonitorList();
std::vector<Monitor> GetMonitorStats();
void PresentMonitorStats() const;
```

**RefreshDisplayCount() Implementation:**

```cpp
void RefreshDisplayCount();
```

**Responsibilities:**

- Validates GLFW initialization state
- Queries GLFW for current monitor list
- Updates internal monitor count and handle array
- Identifies primary monitor
- Resets monitor index if current selection becomes invalid

**Usage Pattern:**

```cpp
MonitorData monitorData;
monitorData.RefreshDisplayCount();
int count = monitorData.GetMonitorCount();
SEDX_CORE_INFO("Detected {} monitors", count);
```

**RefreshMonitorList() Implementation:**

```cpp
void RefreshMonitorList();
```

**Responsibilities:**

- Rebuilds the complete monitor list from GLFW
- Updates all monitor handles and state
- Identifies primary monitor
- Logs discovery results

#### Comprehensive Monitor Statistics

```cpp
std::vector<Monitor> GetMonitorStats();
```

**Functionality:**

- Returns complete monitor information for all connected displays
- Automatically refreshes monitor data if needed
- Calculates physical dimensions and pixel density
- Identifies primary monitor status
- Provides exception-safe operation

**Implementation Example:**

```cpp
class MultiMonitorApplication
{
public:
    void DiscoverMonitors()
    {
        MonitorData monitorData;
        auto monitors = monitorData.GetMonitorStats();
      
        if (monitors.empty()) {
            SEDX_CORE_WARN("No monitors detected - using fallback configuration");
            return;
        }
      
        for (const auto& monitor : monitors) {
            ConfigureWindowForMonitor(monitor);
        }
    }
  
private:
    void ConfigureWindowForMonitor(const Monitor& monitor)
    {
        WindowData windowConfig;
        windowConfig.width = static_cast<uint32_t>(monitor.resolution.x);
        windowConfig.height = static_cast<uint32_t>(monitor.resolution.y);
        windowConfig.title = fmt::format("Window on {}", monitor.monitorID);
      
        if (monitor.isPrimary) {
            windowConfig.mode = WindowMode::Windowed;
        } else {
            windowConfig.mode = WindowMode::WindowedFullScreen;
        }
      
        // Create window for this monitor...
    }
};
```

### Monitor Access and Selection

#### Primary Monitor Access

```cpp
[[nodiscard]] GLFWmonitor* GetPrimaryMonitor() const;
```

**Features:**

- Returns handle to system-designated primary monitor
- Provides null-check with warning logging
- Used for default window placement and fullscreen operations

**Usage:**

```cpp
MonitorData monitorData;
GLFWmonitor* primary = monitorData.GetPrimaryMonitor();
if (primary) {
    const GLFWvidmode* mode = glfwGetVideoMode(primary);
    // Configure window for primary monitor...
}
```

#### Current Monitor Management

```cpp
[[nodiscard]] GLFWmonitor* GetCurrentMonitor() const;
[[nodiscard]] int GetCurrentMonitorIndex() const;
```

**GetCurrentMonitor() Features:**

- Returns handle to currently selected monitor
- Validates monitor index range
- Provides comprehensive error logging
- Safe null return on invalid selection

**Monitor Selection Pattern:**

```cpp
class MonitorSelector
{
public:
    void SelectMonitor(int index)
    {
        if (index >= 0 && index < m_MonitorData.GetMonitorCount()) {
            m_CurrentIndex = index;
            GLFWmonitor* monitor = m_MonitorData.GetCurrentMonitor();
            if (monitor) {
                ConfigureForMonitor(monitor);
            }
        }
    }
  
private:
    MonitorData m_MonitorData;
    int m_CurrentIndex = 0;
};
```

#### Monitor Information Access

```cpp
[[nodiscard]] const std::vector<Monitor>& GetMonitors() const;
[[nodiscard]] int GetMonitorCount() const;
```

**Direct Access Benefits:**

- `GetMonitors()`: Returns reference to avoid copying large data
- `GetMonitorCount()`: Efficient count access without vector allocation
- Thread-safe read access to monitor information

### Video Mode Management

#### Video Mode Enumeration

```cpp
const GLFWvidmode* GetVideoModes(int monitorIndex, int* count);
[[nodiscard]] const GLFWvidmode* GetCurrentVideoMode() const;
```

**GetVideoModes() Implementation:**

- Validates monitor index range
- Returns array of all supported video modes for specified monitor
- Sets count parameter to number of available modes
- Provides comprehensive error handling and logging

**Video Mode Selection Example:**

```cpp
class VideoModeManager
{
public:
    void DisplayAvailableModes(int monitorIndex)
    {
        int modeCount = 0;
        const GLFWvidmode* modes = m_MonitorData.GetVideoModes(monitorIndex, &modeCount);
      
        if (!modes || modeCount == 0) {
            SEDX_CORE_WARN("No video modes available for monitor {}", monitorIndex);
            return;
        }
      
        SEDX_CORE_INFO("Available video modes for monitor {}:", monitorIndex);
        for (int i = 0; i < modeCount; ++i) {
            const GLFWvidmode& mode = modes[i];
            SEDX_CORE_INFO("  {}x{} @ {}Hz ({}-bit color)",
                          mode.width, mode.height, mode.refreshRate,
                          mode.redBits + mode.greenBits + mode.blueBits);
        }
    }
  
    bool SelectOptimalMode(int monitorIndex, int targetWidth, int targetHeight)
    {
        int modeCount = 0;
        const GLFWvidmode* modes = m_MonitorData.GetVideoModes(monitorIndex, &modeCount);
      
        if (!modes) return false;
      
        // Find best matching mode
        int bestMode = -1;
        int bestScore = INT_MAX;
      
        for (int i = 0; i < modeCount; ++i) {
            const GLFWvidmode& mode = modes[i];
            int score = abs(mode.width - targetWidth) + abs(mode.height - targetHeight);
          
            if (score < bestScore) {
                bestScore = score;
                bestMode = i;
            }
        }
      
        if (bestMode >= 0) {
            m_MonitorData.SetVideoModeIndex(bestMode);
            return true;
        }
      
        return false;
    }
  
private:
    MonitorData m_MonitorData;
};
```

#### Video Mode Index Management

```cpp
[[nodiscard]] int GetVideoModeIndex() const;
void SetVideoModeIndex(int index);
```

**Index Management Features:**

- Tracks currently selected video mode for each monitor
- Used by Window system for fullscreen mode switching
- Validates index bounds before application
- Integrates with monitor mode switching operations

### Utility Functions

#### Monitor Center Calculation

```cpp
Vec2 GetMonitorCenter(GLFWmonitor **monitors = nullptr);
```

**Advanced Center Calculation:**

- Determines geometric center of specified monitor
- Falls back to primary monitor if invalid monitor provided
- Handles monitor disconnection scenarios gracefully
- Supports both current monitor and custom monitor array

**Implementation Features:**

- Recursive fallback mechanism for invalid monitors
- Comprehensive error handling and logging
- Default center coordinates for emergency fallback
- Thread-safe operation with proper validation

**Usage Examples:**

```cpp
// Get center of current monitor
MonitorData monitorData;
Vec2 center = monitorData.GetMonitorCenter();
SEDX_CORE_INFO("Monitor center: {}, {}", center.x, center.y);

// Use for window centering
WindowData windowData;
windowData.posX = static_cast<int>(center.x - windowData.width / 2);
windowData.posY = static_cast<int>(center.y - windowData.height / 2);
```

```cpp
// Advanced multi-monitor center calculation
class MultiMonitorCentering
{
public:
    Vec2 GetOptimalWindowPosition(uint32_t windowWidth, uint32_t windowHeight)
    {
        MonitorData monitorData;
        auto monitors = monitorData.GetMonitorStats();
      
        if (monitors.empty()) {
            return {640.0f, 360.0f}; // Fallback
        }
      
        // Find monitor with best fit for window size
        const Monitor* bestMonitor = nullptr;
        float bestRatio = 0.0f;
      
        for (const auto& monitor : monitors) {
            float widthRatio = static_cast<float>(windowWidth) / monitor.resolution.x;
            float heightRatio = static_cast<float>(windowHeight) / monitor.resolution.y;
            float maxRatio = std::max(widthRatio, heightRatio);
          
            if (maxRatio < 1.0f && maxRatio > bestRatio) {
                bestRatio = maxRatio;
                bestMonitor = &monitor;
            }
        }
      
        if (bestMonitor) {
            // Calculate center for best fitting monitor
            return {
                bestMonitor->resolution.x / 2.0f,
                bestMonitor->resolution.y / 2.0f
            };
        }
      
        // Use primary monitor as fallback
        return monitorData.GetMonitorCenter();
    }
};
```

---

## Integration with Window System

### Window Creation Integration

```cpp
class WindowWithMonitorAwareness
{
public:
    void CreateOptimalWindow()
    {
        MonitorData monitorData;
        auto monitors = monitorData.GetMonitorStats();
      
        if (monitors.empty()) {
            CreateFallbackWindow();
            return;
        }
      
        // Select best monitor based on criteria
        const Monitor& targetMonitor = SelectOptimalMonitor(monitors);
      
        // Configure window for selected monitor
        WindowData windowConfig;
        windowConfig.width = static_cast<uint32_t>(targetMonitor.resolution.x * 0.8f);
        windowConfig.height = static_cast<uint32_t>(targetMonitor.resolution.y * 0.8f);
      
        // Center on selected monitor
        Vec2 center = monitorData.GetMonitorCenter();
        windowConfig.posX = static_cast<int>(center.x - windowConfig.width / 2);
        windowConfig.posY = static_cast<int>(center.y - windowConfig.height / 2);
      
        // Create window
        m_Window = CreateRef<Window>(Window::Create(windowConfig));
        m_Window->Init();
    }
  
private:
    const Monitor& SelectOptimalMonitor(const std::vector<Monitor>& monitors)
    {
        // Prefer primary monitor
        for (const auto& monitor : monitors) {
            if (monitor.isPrimary) {
                return monitor;
            }
        }
      
        // Fallback to first monitor
        return monitors[0];
    }
  
    Ref<Window> m_Window;
};
```

### Fullscreen Mode Integration

```cpp
class FullscreenManager
{
public:
    bool SwitchToFullscreen(int monitorIndex = -1)
    {
        MonitorData monitorData;
      
        // Use current monitor if not specified
        if (monitorIndex < 0) {
            monitorIndex = monitorData.GetCurrentMonitorIndex();
        }
      
        // Validate monitor index
        if (monitorIndex >= monitorData.GetMonitorCount()) {
            SEDX_CORE_ERROR("Invalid monitor index for fullscreen: {}", monitorIndex);
            return false;
        }
      
        // Get optimal video mode
        int modeCount = 0;
        const GLFWvidmode* modes = monitorData.GetVideoModes(monitorIndex, &modeCount);
      
        if (!modes || modeCount == 0) {
            SEDX_CORE_ERROR("No video modes available for fullscreen");
            return false;
        }
      
        // Select highest resolution mode
        int bestMode = 0;
        for (int i = 1; i < modeCount; ++i) {
            if (modes[i].width * modes[i].height > modes[bestMode].width * modes[bestMode].height) {
                bestMode = i;
            }
        }
      
        // Apply fullscreen mode
        monitorData.SetVideoModeIndex(bestMode);
        WindowData::mode = WindowMode::FullScreen;
      
        if (m_Window) {
            m_Window->ChangeWindowMode();
            return true;
        }
      
        return false;
    }
  
private:
    Ref<Window> m_Window;
};
```

---

## Error Handling and Robustness

### Exception Safety

The MonitorData class implements comprehensive exception handling:

```cpp
class RobustMonitorOperations
{
public:
    void SafeMonitorOperations()
    {
        try {
            MonitorData monitorData;
          
            // Safe monitor enumeration
            auto monitors = monitorData.GetMonitorStats();
          
            if (monitors.empty()) {
                HandleNoMonitorsAvailable();
                return;
            }
          
            // Process monitors safely
            for (const auto& monitor : monitors) {
                ProcessMonitorSafely(monitor);
            }
          
        } catch (const std::exception& e) {
            SEDX_CORE_ERROR("Exception in monitor operations: {}", e.what());
            HandleMonitorError();
        }
    }
  
private:
    void HandleNoMonitorsAvailable()
    {
        SEDX_CORE_WARN("No monitors available - using fallback configuration");
        // Setup fallback window configuration
        WindowData fallbackConfig;
        fallbackConfig.width = 800;
        fallbackConfig.height = 600;
        // ... continue with fallback setup
    }
  
    void ProcessMonitorSafely(const Monitor& monitor)
    {
        if (monitor.resolution.x <= 0 || monitor.resolution.y <= 0) {
            SEDX_CORE_WARN("Invalid monitor resolution: {}x{}", monitor.resolution.x, monitor.resolution.y);
            return;
        }
      
        // Safe monitor processing...
    }
  
    void HandleMonitorError()
    {
        // Fallback to safe defaults
        CreateSafeDefaultWindow();
    }
};
```

### Validation Patterns

```cpp
class MonitorDataValidator
{
public:
    bool ValidateMonitorData(const MonitorData& monitorData)
    {
        // Validate monitor count
        if (monitorData.GetMonitorCount() <= 0) {
            SEDX_CORE_ERROR("No monitors detected");
            return false;
        }
      
        // Validate primary monitor
        if (!monitorData.GetPrimaryMonitor()) {
            SEDX_CORE_ERROR("No primary monitor available");
            return false;
        }
      
        // Validate current monitor selection
        if (!monitorData.GetCurrentMonitor()) {
            SEDX_CORE_ERROR("Current monitor selection invalid");
            return false;
        }
      
        return true;
    }
  
    bool ValidateMonitor(const Monitor& monitor)
    {
        if (monitor.resolution.x <= 0 || monitor.resolution.y <= 0) {
            SEDX_CORE_ERROR("Invalid monitor resolution: {}x{}", monitor.resolution.x, monitor.resolution.y);
            return false;
        }
      
        if (monitor.refreshRate <= 0) {
            SEDX_CORE_WARN("Invalid refresh rate: {}Hz", monitor.refreshRate);
            // Don't fail validation - some systems report 0 refresh rate
        }
      
        if (!monitor.handle) {
            SEDX_CORE_ERROR("Invalid monitor handle");
            return false;
        }
      
        return true;
    }
};
```

---

## Performance Considerations

### Efficient Monitor Queries

```cpp
class MonitorCaching
{
public:
    void InitializeCache()
    {
        m_MonitorData.RefreshDisplayCount();
        m_MonitorData.RefreshMonitorList();
        m_CachedMonitors = m_MonitorData.GetMonitorStats();
        m_CacheValid = true;
      
        SEDX_CORE_INFO("Monitor cache initialized with {} monitors", m_CachedMonitors.size());
    }
  
    const std::vector<Monitor>& GetCachedMonitors()
    {
        if (!m_CacheValid) {
            InitializeCache();
        }
        return m_CachedMonitors;
    }
  
    void InvalidateCache()
    {
        m_CacheValid = false;
        SEDX_CORE_INFO("Monitor cache invalidated");
    }
  
private:
    MonitorData m_MonitorData;
    std::vector<Monitor> m_CachedMonitors;
    bool m_CacheValid = false;
};
```

### Monitor Change Detection

```cpp
class MonitorChangeDetector
{
public:
    bool DetectMonitorChanges()
    {
        MonitorData currentData;
        currentData.RefreshDisplayCount();
      
        int currentCount = currentData.GetMonitorCount();
        if (currentCount != m_LastMonitorCount) {
            SEDX_CORE_INFO("Monitor count changed: {} -> {}", m_LastMonitorCount, currentCount);
            m_LastMonitorCount = currentCount;
            OnMonitorConfigurationChanged();
            return true;
        }
      
        return false;
    }
  
private:
    int m_LastMonitorCount = 0;
  
    void OnMonitorConfigurationChanged()
    {
        // Handle monitor configuration changes
        // Notify window system, update cached data, etc.
    }
};
```

---

## Advanced Usage Patterns

### Multi-Monitor Application Setup

```cpp
class MultiMonitorApplication
{
public:
    void InitializeMultiMonitorSetup()
    {
        MonitorData monitorData;
        auto monitors = monitorData.GetMonitorStats();
      
        if (monitors.size() < 2) {
            SEDX_CORE_INFO("Single monitor setup detected");
            InitializeSingleMonitorSetup();
            return;
        }
      
        SEDX_CORE_INFO("Multi-monitor setup detected: {} monitors", monitors.size());
      
        // Create main window on primary monitor
        CreateMainWindow(GetPrimaryMonitor(monitors));
      
        // Create secondary windows on other monitors
        for (const auto& monitor : monitors) {
            if (!monitor.isPrimary) {
                CreateSecondaryWindow(monitor);
            }
        }
    }
  
private:
    const Monitor& GetPrimaryMonitor(const std::vector<Monitor>& monitors)
    {
        for (const auto& monitor : monitors) {
            if (monitor.isPrimary) {
                return monitor;
            }
        }
        return monitors[0]; // Fallback
    }
  
    void CreateMainWindow(const Monitor& monitor)
    {
        WindowData mainConfig;
        mainConfig.title = "Main Application Window";
        mainConfig.width = static_cast<uint32_t>(monitor.resolution.x * 0.8f);
        mainConfig.height = static_cast<uint32_t>(monitor.resolution.y * 0.8f);
        mainConfig.mode = WindowMode::Windowed;
      
        // Center on monitor
        mainConfig.posX = static_cast<int>((monitor.resolution.x - mainConfig.width) / 2);
        mainConfig.posY = static_cast<int>((monitor.resolution.y - mainConfig.height) / 2);
      
        auto mainWindow = CreateRef<Window>(Window::Create(mainConfig));
        mainWindow->Init();
        m_Windows.push_back(mainWindow);
    }
  
    void CreateSecondaryWindow(const Monitor& monitor)
    {
        WindowData secondaryConfig;
        secondaryConfig.title = fmt::format("Secondary - {}", monitor.monitorID);
        secondaryConfig.width = static_cast<uint32_t>(monitor.resolution.x);
        secondaryConfig.height = static_cast<uint32_t>(monitor.resolution.y);
        secondaryConfig.mode = WindowMode::WindowedFullScreen;
      
        auto secondaryWindow = CreateRef<Window>(Window::Create(secondaryConfig));
        secondaryWindow->Init();
        m_Windows.push_back(secondaryWindow);
    }
  
    std::vector<Ref<Window>> m_Windows;
};
```

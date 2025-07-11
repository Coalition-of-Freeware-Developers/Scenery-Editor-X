# Monitor_Data.cpp - Monitor Implementation Documentation

---

## Overview

The `Monitor_Data.cpp` file contains the complete implementation of the MonitorData class, providing comprehensive monitor discovery, enumeration, and management functionality for Scenery Editor X. This implementation handles complex multi-monitor scenarios, DPI calculations, video mode management, and robust error handling across different operating systems.

The implementation follows Scenery Editor X architectural principles, utilizing the custom logging system, exception-safe programming patterns, and efficient resource management for high-performance applications.

---

## Implementation Architecture

### Constructor Implementation

```cpp
MonitorData::MonitorData() : monitorHandles(nullptr), primaryMonitor(nullptr), monitorCount(0), monitorIndex(0), videoModeIndex(0)
{
    try
    {
        RefreshDisplayCount();
        RefreshMonitorList();
    }
    catch (const std::exception& e)
    {
        SEDX_CORE_WARN("Exception during MonitorData initialization: {}", e.what()); /// Continue with default values rather than crashing
    }
}
```

**Initialization Strategy:**

- **Member Initialization**: All pointers initialized to safe null values
- **Automatic Discovery**: Immediate monitor enumeration upon construction
- **Exception Safety**: Graceful handling of initialization failures
- **Fallback Behavior**: Continues with default values if monitor discovery fails
- **Logging Integration**: Comprehensive status reporting through custom logging system

**Benefits:**

- Zero-invalid-state construction - object is always in a usable state
- Immediate availability of monitor data after construction
- Crash-resistant initialization for robust application startup
- Clear logging for debugging monitor configuration issues

### Destructor Implementation

```cpp
MonitorData::~MonitorData() = default;
```

**Resource Management:**

- **GLFW Managed**: Monitor handles are managed by GLFW, not requiring explicit cleanup
- **RAII Compliance**: Automatic cleanup through default destructor
- **Exception Safe**: No throwing operations in destructor
- **Minimal Overhead**: No unnecessary cleanup code

---

## Monitor Discovery Implementation

### Display Count Refresh

```cpp
void MonitorData::RefreshDisplayCount()
{
    /// Check if GLFW is initialized
    if (int initialized = glfwInit(); !initialized)
    {
        SEDX_CORE_WARN("Cannot refresh display count - GLFW not initialized");
        monitorCount = 0;
        monitorHandles = nullptr;
        primaryMonitor = nullptr;
        return;
    }
  
    /// Get monitors from GLFW
    monitorHandles = glfwGetMonitors(&monitorCount);
    primaryMonitor = glfwGetPrimaryMonitor();
  
    /// Safety check to ensure we have at least one monitor
    if (monitorCount <= 0)
    {
        SEDX_CORE_WARN("No monitors detected during RefreshDisplayCount");
        monitorCount = 0;
        monitorHandles = nullptr;
        primaryMonitor = nullptr;
    }
    else
        SEDX_CORE_INFO("Detected {} monitor(s)", monitorCount);

    /// Make sure the current monitor index is valid
    if (monitorIndex >= monitorCount)
        monitorIndex = 0;
}
```

**Implementation Features:**

1. **GLFW Initialization Check**: Validates GLFW state before proceeding
2. **Atomic Update**: Updates all monitor-related state consistently
3. **Error Recovery**: Handles zero-monitor scenarios gracefully
4. **Index Validation**: Ensures current monitor selection remains valid
5. **Comprehensive Logging**: Status reporting for all scenarios

**Error Handling Strategy:**

- **Graceful Degradation**: Sets safe default values on failure
- **State Consistency**: Ensures all related variables are updated together
- **User Notification**: Clear warning messages for debugging
- **Recovery Mechanism**: Automatic fallback to valid monitor indices

### Monitor List Refresh

```cpp
void MonitorData::RefreshMonitorList()
{
    if (glfwInit() == GLFW_FALSE)
    {
        SEDX_CORE_WARN("GLFW not initialized - cannot refresh monitor list");
        return;
    }

    int count;
    monitorHandles = glfwGetMonitors(&count);

    monitorCount = count;

    if (monitorCount > 0)
    {
        SEDX_CORE_INFO("Successfully refreshed monitor list: {} monitor(s) detected", monitorCount);
        primaryMonitor = glfwGetPrimaryMonitor();
    }
    else
    {
        SEDX_CORE_WARN("No monitors detected");
        primaryMonitor = nullptr;
    }
}
```

**Refresh Strategy:**

- **Validation First**: Checks GLFW initialization before operations
- **Atomic State Update**: Updates count and handles together
- **Primary Monitor Identification**: Locates system-designated primary display
- **Status Reporting**: Detailed logging of discovery results

---

## Monitor Statistics Implementation

### Comprehensive Monitor Data Collection

```cpp
std::vector<Monitor> MonitorData::GetMonitorStats()
{
    try
    {
        /// Update monitor data if needed
        RefreshDisplayCount();
      
        /// If we already have monitors, return them
        if (!monitors.empty())
            return monitors;

        /// Otherwise, populate the monitor data
        RefreshMonitorList();
        for (const auto &displayName : monitors)
            SEDX_CORE_INFO("Monitor: {} = {}", monitorCount, displayName.monitorID);

        return monitors;

    }
    catch (const std::exception& e)
    {
        SEDX_CORE_WARN("Exception in GetMonitorStats: {}", e.what());
        return {}; /// Return empty vector on error
    }
}
```

**Data Collection Strategy:**

- **Cache Optimization**: Returns existing data if available
- **Lazy Loading**: Refreshes data only when needed
- **Exception Safety**: Comprehensive error handling with safe return values
- **Performance Consideration**: Avoids unnecessary monitor enumeration

**Implementation Benefits:**

- Efficient caching mechanism reduces GLFW API calls
- Exception-safe operation prevents application crashes
- Detailed logging for debugging monitor configuration issues
- Graceful failure with empty vector return on errors

### Monitor Statistics Presentation

```cpp
void MonitorData::PresentMonitorStats() const
{
    try
    {
        for (const auto& data : GetMonitors())
        {
            SEDX_CORE_TRACE("{}{}: {} x {} pixels, {:0.1f} x {:0.1f} inches, {:0.2f} Pixels",
                       data.monitorID,
                       data.isPrimary ? " (Primary)" : "",
                       data.resolution.x,
                       data.resolution.y,
                       data.dimensions.x,
                       data.dimensions.y,
                       data.pixDensity.x);
        }
    }
    catch (const std::exception& e)
    {
        SEDX_CORE_WARN("Exception in PresentMonitorStats: {}", e.what());
    }
}
```

**Presentation Features:**

- **Formatted Output**: Professional display of monitor information
- **Primary Monitor Indication**: Clear identification of primary display
- **Comprehensive Metrics**: Resolution, physical size, and pixel density
- **Exception Safety**: Error handling prevents crashes during output

**Usage in Applications:**

```cpp
class MonitorDiagnostics
{
public:
    void GenerateMonitorReport()
    {
        MonitorData monitorData;
      
        SEDX_CORE_INFO("=== Monitor Configuration Report ===");
        monitorData.PresentMonitorStats();
      
        // Additional diagnostic information
        auto monitors = monitorData.GetMonitorStats();
        AnalyzeMonitorConfiguration(monitors);
    }
  
private:
    void AnalyzeMonitorConfiguration(const std::vector<Monitor>& monitors)
    {
        if (monitors.empty()) {
            SEDX_CORE_WARN("No monitors detected - system may have display issues");
            return;
        }
      
        // Analyze DPI consistency
        if (monitors.size() > 1) {
            AnalyzeDPIConsistency(monitors);
        }
      
        // Check for common resolutions
        AnalyzeResolutions(monitors);
    }
};
```

---

## Monitor Center Calculation Implementation

### Advanced Center Point Calculation

```cpp
Vec2 MonitorData::GetMonitorCenter(GLFWmonitor **monitors)
{
    try
    {
        GLFWmonitor **monitorsToUse = (monitors != nullptr) ? monitors : monitorHandles;
      
        /// Check if monitors are available and monitorIndex is valid
        if (monitorIndex < 0 || monitorIndex >= monitorCount || monitorsToUse == nullptr)
        {
            /// Get primary monitor and reset monitor index
            RefreshDisplayCount();
            monitorIndex = 0;
          
            /// Safety check to prevent infinite recursion
            if (monitorHandles && monitorCount > 0)
                return GetMonitorCenter(monitorHandles);  /// Recursive call with valid monitor

            SEDX_CORE_WARN("No valid monitors available for GetMonitorCenter");
            return {640.0f, 360.0f};  /// Default fallback center
        }

        const GLFWvidmode *mode = glfwGetVideoMode(monitorsToUse[monitorIndex]);
        if (!mode)
        {
            /// Fallback if mode cannot be retrieved
            SEDX_CORE_WARN("Failed to get video mode for monitor {}", monitorIndex);
            return {640.0f, 360.0f};  /// Default fallback center
        }

        const int screenCenterX = mode->width / 2;
        const int screenCenterY = mode->height / 2;

        return {static_cast<float>(screenCenterX), static_cast<float>(screenCenterY)};
    }
    catch (const std::exception& e)
    {
        SEDX_CORE_WARN("Exception in GetMonitorCenter: {}", e.what());
        return {640.0f, 360.0f};  /// Default fallback center
    }
}
```

**Advanced Features:**

1. **Flexible Monitor Source**: Accepts custom monitor array or uses internal handles
2. **Recursive Fallback**: Automatic retry with refreshed monitor data
3. **Index Validation**: Comprehensive bounds checking with automatic correction
4. **Infinite Recursion Prevention**: Safety checks to prevent stack overflow
5. **Exception Safety**: Complete error handling with safe fallback values
6. **Detailed Logging**: Status reporting for debugging positioning issues

**Center Calculation Logic:**

- **Geometric Center**: Calculates exact pixel center of monitor
- **Type Safety**: Proper integer to float conversion
- **Fallback Strategy**: Progressive fallback from specific monitor to primary to default

**Advanced Usage Patterns:**

```cpp
class SmartWindowPositioning
{
public:
    Vec2 CalculateOptimalPosition(uint32_t windowWidth, uint32_t windowHeight)
    {
        MonitorData monitorData;
      
        // Get center of current monitor
        Vec2 center = monitorData.GetMonitorCenter();
      
        // Adjust for window size
        Vec2 position;
        position.x = center.x - static_cast<float>(windowWidth) / 2.0f;
        position.y = center.y - static_cast<float>(windowHeight) / 2.0f;
      
        // Ensure window stays within monitor bounds
        return ClampToMonitorBounds(position, windowWidth, windowHeight);
    }
  
private:
    Vec2 ClampToMonitorBounds(Vec2 position, uint32_t width, uint32_t height)
    {
        MonitorData monitorData;
        auto monitors = monitorData.GetMonitorStats();
      
        if (monitors.empty()) {
            return position; // No clamping possible
        }
      
        // Find monitor containing the position
        const Monitor* targetMonitor = FindMonitorContaining(position, monitors);
        if (!targetMonitor) {
            targetMonitor = &monitors[0]; // Use first monitor as fallback
        }
      
        // Clamp to monitor bounds
        position.x = std::max(0.0f, std::min(position.x, targetMonitor->resolution.x - width));
        position.y = std::max(0.0f, std::min(position.y, targetMonitor->resolution.y - height));
      
        return position;
    }
};
```

---

## Video Mode Management Implementation

### Video Mode Enumeration

```cpp
const GLFWvidmode* MonitorData::GetVideoModes(const int monitorIndex, int* count)
{
    /// Set default return value for error cases
    *count = 0;
  
    try
    {
        /// Validate index and monitor handles
        if (monitorIndex < 0 || monitorIndex >= monitorCount || monitorHandles == nullptr)
        {
            SEDX_CORE_WARN("Invalid monitor index {} for GetVideoModes (total: {})", monitorIndex, monitorCount);
            return nullptr;
        }
      
        /// Get video modes from GLFW
        const GLFWvidmode* modes = glfwGetVideoModes(monitorHandles[monitorIndex], count);
      
        /// Log result
        if (modes && *count > 0)
            SEDX_CORE_INFO("Retrieved {} video modes for monitor {}", *count, monitorIndex);
        else
        {
            SEDX_CORE_WARN("No video modes available for monitor {}", monitorIndex);
            *count = 0;
        }
      
        return modes;
    }
    catch (const std::exception& e)
    {
        SEDX_CORE_WARN("Exception in GetVideoModes: {}", e.what());
        return nullptr;
    }
}
```

**Video Mode Features:**

1. **Comprehensive Validation**: Index bounds checking with detailed error reporting
2. **Safe Parameter Handling**: Count parameter always set to valid value
3. **Exception Safety**: Complete error handling with null return on failure
4. **Resource Management**: GLFW manages video mode array lifetime
5. **Detailed Logging**: Status reporting for debugging video mode issues

**Video Mode Selection Patterns:**

```cpp
class VideoModeManager
{
public:
    struct VideoModeSelection
    {
        int monitorIndex;
        int modeIndex;
        GLFWvidmode mode;
        bool isValid;
    };
  
    VideoModeSelection FindBestVideoMode(int monitorIndex, int targetWidth, int targetHeight, int targetRefreshRate = 60)
    {
        VideoModeSelection selection{};
        selection.monitorIndex = monitorIndex;
        selection.isValid = false;
      
        MonitorData monitorData;
        int modeCount = 0;
        const GLFWvidmode* modes = monitorData.GetVideoModes(monitorIndex, &modeCount);
      
        if (!modes || modeCount == 0) {
            SEDX_CORE_WARN("No video modes available for mode selection");
            return selection;
        }
      
        int bestModeIndex = -1;
        int bestScore = INT_MAX;
      
        for (int i = 0; i < modeCount; ++i) {
            const GLFWvidmode& mode = modes[i];
          
            // Calculate matching score
            int widthDiff = abs(mode.width - targetWidth);
            int heightDiff = abs(mode.height - targetHeight);
            int refreshDiff = abs(mode.refreshRate - targetRefreshRate);
          
            int score = widthDiff + heightDiff + refreshDiff * 10; // Weight refresh rate more
          
            if (score < bestScore) {
                bestScore = score;
                bestModeIndex = i;
            }
        }
      
        if (bestModeIndex >= 0) {
            selection.modeIndex = bestModeIndex;
            selection.mode = modes[bestModeIndex];
            selection.isValid = true;
          
            SEDX_CORE_INFO("Selected video mode: {}x{} @ {}Hz (score: {})",
                          selection.mode.width, selection.mode.height,
                          selection.mode.refreshRate, bestScore);
        }
      
        return selection;
    }
  
    std::vector<GLFWvidmode> GetHighResolutionModes(int monitorIndex, int minWidth = 1920, int minHeight = 1080)
    {
        std::vector<GLFWvidmode> highResModes;
      
        MonitorData monitorData;
        int modeCount = 0;
        const GLFWvidmode* modes = monitorData.GetVideoModes(monitorIndex, &modeCount);
      
        if (!modes) return highResModes;
      
        for (int i = 0; i < modeCount; ++i) {
            const GLFWvidmode& mode = modes[i];
            if (mode.width >= minWidth && mode.height >= minHeight) {
                highResModes.push_back(mode);
            }
        }
      
        // Sort by resolution (highest first)
        std::sort(highResModes.begin(), highResModes.end(),
                 [](const GLFWvidmode& a, const GLFWvidmode& b) {
                     return (a.width * a.height) > (b.width * b.height);
                 });
      
        return highResModes;
    }
};
```

### Current Video Mode Access

```cpp
const GLFWvidmode* MonitorData::GetCurrentVideoMode() const
{
    if (GLFWmonitor* monitor = GetCurrentMonitor())
    {
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        if (!mode)
            SEDX_CORE_WARN("Failed to get video mode for current monitor");

        return mode;
    }
  
    SEDX_CORE_WARN("Cannot get current video mode - no monitor available");
    return nullptr;
}
```

**Current Mode Features:**

- **Safe Monitor Access**: Uses validated monitor handle
- **Error Reporting**: Detailed logging for failure scenarios
- **Null Safety**: Safe return value on error conditions

---

## Monitor Access Implementation

### Primary Monitor Access

```cpp
GLFWmonitor* MonitorData::GetPrimaryMonitor() const
{
    if (!primaryMonitor)
        SEDX_CORE_WARN("Primary monitor not available");

    return primaryMonitor;
}
```

### Current Monitor Access

```cpp
GLFWmonitor* MonitorData::GetCurrentMonitor() const
{
    if (monitorIndex >= 0 && monitorIndex < monitorCount && monitorHandles != nullptr)
        return monitorHandles[monitorIndex];

    SEDX_CORE_WARN("Current monitor not available (index: {}, count: {})", monitorIndex, monitorCount);
    return nullptr;
}
```

**Access Pattern Benefits:**

- **Bounds Checking**: Comprehensive validation before access
- **Error Logging**: Detailed diagnostic information
- **Safe Failure**: Null return on invalid state
- **State Validation**: Multiple condition checking for robustness

---

## Integration Examples

### Window System Integration

```cpp
class MonitorAwareWindowManager
{
public:
    void CreateWindowOnSpecificMonitor(int monitorIndex)
    {
        MonitorData monitorData;
      
        // Validate monitor index
        if (monitorIndex < 0 || monitorIndex >= monitorData.GetMonitorCount()) {
            SEDX_CORE_ERROR("Invalid monitor index: {}", monitorIndex);
            return;
        }
      
        // Get monitor information
        auto monitors = monitorData.GetMonitorStats();
        if (static_cast<size_t>(monitorIndex) >= monitors.size()) {
            SEDX_CORE_ERROR("Monitor index out of range");
            return;
        }
      
        const Monitor& targetMonitor = monitors[monitorIndex];
      
        // Configure window for specific monitor
        WindowData windowConfig;
        windowConfig.title = fmt::format("Window on {}", targetMonitor.monitorID);
      
        // Use 80% of monitor resolution
        windowConfig.width = static_cast<uint32_t>(targetMonitor.resolution.x * 0.8f);
        windowConfig.height = static_cast<uint32_t>(targetMonitor.resolution.y * 0.8f);
      
        // Center on target monitor
        Vec2 center = monitorData.GetMonitorCenter();
        windowConfig.posX = static_cast<int>(center.x - windowConfig.width / 2);
        windowConfig.posY = static_cast<int>(center.y - windowConfig.height / 2);
      
        // Create and initialize window
        auto window = CreateRef<Window>(Window::Create(windowConfig));
        window->Init();
      
        m_Windows.push_back(window);
      
        SEDX_CORE_INFO("Created window on monitor {} ({}x{})",
                      targetMonitor.monitorID,
                      windowConfig.width, windowConfig.height);
    }
  
    void CreateWindowsOnAllMonitors()
    {
        MonitorData monitorData;
        auto monitors = monitorData.GetMonitorStats();
      
        for (size_t i = 0; i < monitors.size(); ++i) {
            CreateWindowOnSpecificMonitor(static_cast<int>(i));
        }
    }
  
private:
    std::vector<Ref<Window>> m_Windows;
};
```

### Multi-Monitor Application Setup

```cpp
class MultiMonitorApplication
{
public:
    void InitializeMultiMonitorLayout()
    {
        MonitorData monitorData;
        auto monitors = monitorData.GetMonitorStats();
      
        if (monitors.empty()) {
            HandleNoMonitorsDetected();
            return;
        }
      
        SEDX_CORE_INFO("Initializing multi-monitor layout with {} monitors", monitors.size());
      
        // Analyze monitor configuration
        AnalyzeMonitorLayout(monitors);
      
        // Create optimized window layout
        CreateOptimizedLayout(monitors);
    }
  
private:
    void AnalyzeMonitorLayout(const std::vector<Monitor>& monitors)
    {
        // Find primary monitor
        const Monitor* primary = nullptr;
        for (const auto& monitor : monitors) {
            if (monitor.isPrimary) {
                primary = &monitor;
                break;
            }
        }
      
        if (primary) {
            SEDX_CORE_INFO("Primary monitor: {} ({}x{} @ {}Hz)",
                          primary->monitorID,
                          primary->resolution.x, primary->resolution.y,
                          primary->refreshRate);
        }
      
        // Analyze DPI consistency
        if (monitors.size() > 1) {
            float minDPI = monitors[0].pixDensity.x;
            float maxDPI = monitors[0].pixDensity.x;
          
            for (const auto& monitor : monitors) {
                minDPI = std::min(minDPI, monitor.pixDensity.x);
                maxDPI = std::max(maxDPI, monitor.pixDensity.x);
            }
          
            if (abs(maxDPI - minDPI) > 20.0f) {
                SEDX_CORE_WARN("Significant DPI variation detected: {:.1f} - {:.1f} PPI", minDPI, maxDPI);
                SEDX_CORE_WARN("Consider DPI scaling for consistent UI appearance");
            }
        }
    }
  
    void CreateOptimizedLayout(const std::vector<Monitor>& monitors)
    {
        if (monitors.size() == 1) {
            CreateSingleMonitorLayout(monitors[0]);
        } else if (monitors.size() == 2) {
            CreateDualMonitorLayout(monitors);
        } else {
            CreateMultiMonitorLayout(monitors);
        }
    }
  
    void CreateDualMonitorLayout(const std::vector<Monitor>& monitors)
    {
        // Find primary and secondary monitors
        const Monitor* primary = nullptr;
        const Monitor* secondary = nullptr;
      
        for (const auto& monitor : monitors) {
            if (monitor.isPrimary) {
                primary = &monitor;
            } else if (!secondary) {
                secondary = &monitor;
            }
        }
      
        if (primary && secondary) {
            // Create main application window on primary
            CreateMainWindow(*primary);
          
            // Create tool windows on secondary
            CreateToolWindows(*secondary);
        }
    }
};
```

### Monitor Change Detection

```cpp
class MonitorChangeHandler
{
public:
    void StartMonitoring()
    {
        // Initial snapshot
        UpdateMonitorSnapshot();
      
        // Set up monitoring (pseudo-code for monitoring thread)
        m_MonitoringActive = true;
        m_MonitorThread = std::thread([this]() {
            MonitoringLoop();
        });
    }
  
    void StopMonitoring()
    {
        m_MonitoringActive = false;
        if (m_MonitorThread.joinable()) {
            m_MonitorThread.join();
        }
    }
  
private:
    void MonitoringLoop()
    {
        while (m_MonitoringActive) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
          
            if (CheckForMonitorChanges()) {
                HandleMonitorConfigurationChange();
            }
        }
    }
  
    bool CheckForMonitorChanges()
    {
        MonitorData currentData;
        currentData.RefreshDisplayCount();
      
        int currentCount = currentData.GetMonitorCount();
        if (currentCount != m_LastMonitorCount) {
            SEDX_CORE_INFO("Monitor count changed: {} -> {}", m_LastMonitorCount, currentCount);
            m_LastMonitorCount = currentCount;
            return true;
        }
      
        return false;
    }
  
    void HandleMonitorConfigurationChange()
    {
        SEDX_CORE_INFO("Monitor configuration changed - updating application layout");
      
        // Update monitor snapshot
        UpdateMonitorSnapshot();
      
        // Notify application components
        NotifyMonitorChange();
    }
  
    void UpdateMonitorSnapshot()
    {
        MonitorData monitorData;
        m_CurrentMonitors = monitorData.GetMonitorStats();
        m_LastMonitorCount = static_cast<int>(m_CurrentMonitors.size());
    }
  
    void NotifyMonitorChange()
    {
        // Notify window manager to adjust layouts
        // Notify renderer to update contexts
        // Update UI scaling factors
    }
  
    std::vector<Monitor> m_CurrentMonitors;
    int m_LastMonitorCount = 0;
    bool m_MonitoringActive = false;
    std::thread m_MonitorThread;
};
```

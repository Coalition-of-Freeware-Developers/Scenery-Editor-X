# Scenery Editor X - Application Data Structures Documentation

---

## Overview

The Application Data system in Scenery Editor X provides structured configuration management for application settings, window properties, and X-Plane integration. This documentation covers the data structures used to configure and manage application behavior.

## Core Data Structures

### AppData Structure (`application_data.h`)

The `AppData` structure serves as the primary configuration container for application initialization and window management.

```cpp
struct AppData
{
    std::string appName = "Scenery Editor X";
    uint32_t WinWidth = 1280;
    uint32_t WinHeight = 720;
  
    GLOBAL inline std::string renderName = "X-Plane 12 Graphics Emulator";
    GLOBAL inline std::string versionString = SEDX_VERSION_STRING;
    GLOBAL inline uint32_t version = SEDX_VERSION;

    bool Fullscreen = false;
    bool NoTitlebar = false;
    bool VSync = false;
    bool StartMaximized = true;
    bool Resizable = true;
    bool EnableImGui = true;
    std::string WorkingDirectory;
    std::filesystem::path IconPath;
};
```

### XPlaneStats Structure

The `XPlaneStats` structure manages X-Plane flight simulator integration configuration.

```cpp
struct XPlaneStats
{
    std::string xPlaneVersion;
    std::string xPlanePath;
    std::string xPlaneBinPath;
    std::string xPlaneResourcesPath;
    bool isSteam = false;
};
```

---

## AppData Configuration Guide

### Window Configuration

#### Basic Window Setup

```cpp
AppData CreateBasicWindow()
{
    AppData config;
    config.appName = "My Scenery Editor";
    config.WinWidth = 1920;
    config.WinHeight = 1080;
    config.Resizable = true;
    config.VSync = true;
    return config;
}
```

#### Fullscreen Application

```cpp
AppData CreateFullscreenApp()
{
    AppData config;
    config.appName = "Fullscreen Viewer";
    config.Fullscreen = true;
    config.VSync = true;
    config.StartMaximized = false; // Ignored in fullscreen
    return config;
}
```

#### Borderless Window (Launcher Style)

```cpp
AppData CreateLauncherWindow()
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

### Application Identification

#### Version Management

The version system uses preprocessor macros for consistent versioning:

```cpp
// Automatic version configuration
AppData config;
// config.versionString is automatically set to SEDX_VERSION_STRING
// config.version is automatically set to SEDX_VERSION

// Manual version override (not recommended)
config.versionString = "1.0.0-custom";
config.version = 0x010000; // Major.Minor.Patch format
```

#### Render Context Naming

```cpp
AppData config;
config.renderName = "Custom Graphics Engine";
// This appears in debugging tools and graphics driver logs
```

### Advanced Configuration

#### Working Directory Management

```cpp
AppData ConfigureWorkingDirectory()
{
    AppData config;
    config.WorkingDirectory = std::filesystem::current_path().string();
  
    // Or set specific project directory
    config.WorkingDirectory = "C:/Projects/MyScenery";
  
    return config;
}
```

#### Custom Application Icon

```cpp
AppData ConfigureCustomIcon()
{
    AppData config;
    config.IconPath = "assets/icons/custom_icon.ico";
  
    // Verify icon exists
    if (!std::filesystem::exists(config.IconPath))
    {
        SEDX_CORE_WARN("Custom icon not found: {}", config.IconPath.string());
        config.IconPath.clear(); // Use default icon
    }
  
    return config;
}
```

#### ImGui Integration Control

```cpp
AppData ConfigureUISystem()
{
    AppData config;
    config.EnableImGui = true; // Enable for editor applications
  
    // For runtime applications, you might disable UI
    if (IsRuntimeMode())
    {
        config.EnableImGui = false;
    }
  
    return config;
}
```

---

## XPlaneStats Configuration

### Automatic X-Plane Detection

```cpp
XPlaneStats DetectXPlaneInstallation()
{
    XPlaneStats stats;
  
    // Check common installation paths
    std::vector<std::string> commonPaths = {
        "C:/X-Plane 12/",
        "C:/Program Files/X-Plane 12/",
        "D:/X-Plane 12/",
        "/Applications/X-Plane 12/",
        std::filesystem::path(std::getenv("HOME")) / "X-Plane 12"
    };
  
    for (const auto& path : commonPaths)
    {
        if (std::filesystem::exists(path + "X-Plane.exe") || 
            std::filesystem::exists(path + "X-Plane"))
        {
            stats.xPlanePath = path;
            stats.xPlaneBinPath = path + "bin/";
            stats.xPlaneResourcesPath = path + "Resources/";
            stats.xPlaneVersion = DetectXPlaneVersion(path);
            break;
        }
    }
  
    return stats;
}
```

### Steam Installation Handling

```cpp
XPlaneStats ConfigureSteamXPlane()
{
    XPlaneStats stats;
    stats.isSteam = true;
  
    // Steam typically installs to:
    stats.xPlanePath = "C:/Program Files (x86)/Steam/steamapps/common/X-Plane 12/";
    stats.xPlaneBinPath = stats.xPlanePath + "bin/";
    stats.xPlaneResourcesPath = stats.xPlanePath + "Resources/";
  
    // Verify Steam installation
    if (!std::filesystem::exists(stats.xPlanePath))
    {
        SEDX_CORE_ERROR("Steam X-Plane installation not found");
        stats = {}; // Reset to empty
    }
  
    return stats;
}
```

### Version Detection

```cpp
std::string DetectXPlaneVersion(const std::string& xplanePath)
{
    std::string versionFile = xplanePath + "version.txt";
  
    if (std::filesystem::exists(versionFile))
    {
        std::ifstream file(versionFile);
        std::string version;
        if (std::getline(file, version))
        {
            return version;
        }
    }
  
    // Fallback: check executable version
    return GetExecutableVersion(xplanePath + "X-Plane.exe");
}
```

---

## Configuration Validation

### AppData Validation

```cpp
bool ValidateAppData(const AppData& config)
{
    // Validate window dimensions
    if (config.WinWidth < 800 || config.WinHeight < 600)
    {
        SEDX_CORE_WARN("Window size too small: {}x{}", config.WinWidth, config.WinHeight);
        return false;
    }
  
    // Validate application name
    if (config.appName.empty())
    {
        SEDX_CORE_ERROR("Application name cannot be empty");
        return false;
    }
  
    // Validate working directory
    if (!config.WorkingDirectory.empty() && 
        !std::filesystem::exists(config.WorkingDirectory))
    {
        SEDX_CORE_ERROR("Working directory does not exist: {}", config.WorkingDirectory);
        return false;
    }
  
    // Validate icon path
    if (!config.IconPath.empty() && !std::filesystem::exists(config.IconPath))
    {
        SEDX_CORE_WARN("Icon file not found: {}", config.IconPath.string());
        // Non-fatal - will use default icon
    }
  
    return true;
}
```

### XPlaneStats Validation

```cpp
bool ValidateXPlaneStats(const XPlaneStats& stats)
{
    if (stats.xPlanePath.empty())
    {
        SEDX_CORE_INFO("X-Plane path not configured");
        return true; // Not required for all applications
    }
  
    // Validate main installation path
    if (!std::filesystem::exists(stats.xPlanePath))
    {
        SEDX_CORE_ERROR("X-Plane installation path not found: {}", stats.xPlanePath);
        return false;
    }
  
    // Validate executable
    std::string exePath = stats.xPlanePath + (stats.isSteam ? "X-Plane.exe" : "X-Plane");
    if (!std::filesystem::exists(exePath))
    {
        SEDX_CORE_ERROR("X-Plane executable not found: {}", exePath);
        return false;
    }
  
    // Validate resources path
    if (!std::filesystem::exists(stats.xPlaneResourcesPath))
    {
        SEDX_CORE_WARN("X-Plane resources path not found: {}", stats.xPlaneResourcesPath);
    }
  
    return true;
}
```

---

## Configuration Serialization

### Saving Configuration

```cpp
void SaveAppDataToFile(const AppData& config, const std::string& filename)
{
    nlohmann::json j;
  
    j["application"] = {
        {"name", config.appName},
        {"width", config.WinWidth},
        {"height", config.WinHeight},
        {"fullscreen", config.Fullscreen},
        {"no_titlebar", config.NoTitlebar},
        {"vsync", config.VSync},
        {"start_maximized", config.StartMaximized},
        {"resizable", config.Resizable},
        {"enable_imgui", config.EnableImGui},
        {"working_directory", config.WorkingDirectory},
        {"icon_path", config.IconPath.string()}
    };
  
    std::ofstream file(filename);
    file << j.dump(4);
}
```

### Loading Configuration

```cpp
AppData LoadAppDataFromFile(const std::string& filename)
{
    AppData config; // Start with defaults
  
    if (!std::filesystem::exists(filename))
    {
        SEDX_CORE_INFO("Configuration file not found, using defaults: {}", filename);
        return config;
    }
  
    try
    {
        std::ifstream file(filename);
        nlohmann::json j;
        file >> j;
    
        if (j.contains("application"))
        {
            auto& app = j["application"];
            config.appName = app.value("name", config.appName);
            config.WinWidth = app.value("width", config.WinWidth);
            config.WinHeight = app.value("height", config.WinHeight);
            config.Fullscreen = app.value("fullscreen", config.Fullscreen);
            config.NoTitlebar = app.value("no_titlebar", config.NoTitlebar);
            config.VSync = app.value("vsync", config.VSync);
            config.StartMaximized = app.value("start_maximized", config.StartMaximized);
            config.Resizable = app.value("resizable", config.Resizable);
            config.EnableImGui = app.value("enable_imgui", config.EnableImGui);
            config.WorkingDirectory = app.value("working_directory", config.WorkingDirectory);
            config.IconPath = app.value("icon_path", std::string{});
        }
    }
    catch (const std::exception& e)
    {
        SEDX_CORE_ERROR("Failed to load configuration: {}", e.what());
    }
  
    return config;
}
```

---

## Usage Examples

### Editor Application Configuration

```cpp
SceneryEditorX::Application* SceneryEditorX::CreateApplication(int argc, char** argv)
{
    AppData config;
    config.appName = "Scenery Editor X";
    config.WinWidth = 1280;
    config.WinHeight = 720;
    config.Resizable = true;
    config.StartMaximized = true;
    config.VSync = true;
    config.EnableImGui = true;
  
    // Validate configuration
    if (!ValidateAppData(config))
    {
        SEDX_CORE_ERROR("Invalid application configuration");
        return nullptr;
    }
  
    return new EditorApplication(config);
}
```

### Runtime Application Configuration

```cpp
SceneryEditorX::Application* CreateRuntimeApplication(int argc, char** argv)
{
    AppData config;
    config.appName = "Scenery Viewer";
    config.Fullscreen = true;
    config.VSync = true;
    config.EnableImGui = false; // No UI in runtime mode
  
    return new ViewerApplication(config);
}
```

### Launcher Application Configuration

```cpp
SceneryEditorX::Application* CreateLauncherApplication(int argc, char** argv)
{
    AppData config;
    config.appName = "Scenery Editor X Launcher";
    config.WinWidth = 978;
    config.WinHeight = 526;
    config.NoTitlebar = true;
    config.Resizable = false;
    config.StartMaximized = false;
    config.VSync = true;
    config.IconPath = "assets/launcher_icon.ico";
  
    return new LauncherApplication(config);
}
```

---

## Best Practices

### Configuration Management

- Always validate configuration data before use
- Provide sensible defaults for all settings
- Use the settings system for persistent configuration
- Handle missing or invalid configuration gracefully

### Version Management

- Use the automatic version system when possible
- Keep version information consistent across all components
- Update version strings for custom builds appropriately

### Resource Path Management

- Use absolute paths for critical resources
- Validate all file paths before use
- Provide fallbacks for missing resources
- Use the working directory appropriately

### Integration Guidelines

- Configure X-Plane integration only when needed
- Handle both Steam and standalone X-Plane installations
- Gracefully degrade when X-Plane is not available
- Cache X-Plane configuration for performance

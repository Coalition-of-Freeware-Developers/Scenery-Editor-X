# Settings System Instructions for GitHub Copilot

## Overview

These instructions guide the proper implementation and usage of the Settings System in Scenery Editor X. The Settings System provides comprehensive configuration management using libconfig++ with automatic X-Plane path detection, hierarchical settings organization, and robust error handling.

## Core Principles

### 1. Always Use the ApplicationSettings Class
**Never implement custom configuration handling.** Use the existing `ApplicationSettings` class for all configuration needs.

```cpp
// ✅ CORRECT - Use the ApplicationSettings class
auto settings = CreateRef<ApplicationSettings>("config/module.cfg");
settings->AddStringOption("module.enabled", "true");

// ❌ WRONG - Don't create custom config handling
// std::ofstream configFile("config.txt");
// configFile << "enabled=true\n";
```

### 2. Follow Smart Pointer Usage
**Always use the custom smart pointer system** for ApplicationSettings instances.

```cpp
// ✅ CORRECT - Use CreateRef for new instances
Ref<ApplicationSettings> m_Settings = CreateRef<ApplicationSettings>("config/settings.cfg");

// ✅ CORRECT - Store as member variable with proper type
class MyModule : public Module
{
private:
    Ref<ApplicationSettings> m_Settings;
};

// ❌ WRONG - Don't use raw pointers or standard smart pointers
// std::unique_ptr<ApplicationSettings> settings;
// ApplicationSettings* settings = new ApplicationSettings("config.cfg");
```

### 3. Use Hierarchical Path Structure
**Organize settings using dot notation** for logical grouping and easy maintenance.

```cpp
// ✅ CORRECT - Use hierarchical paths
settings->AddStringOption("ui.theme", "dark");
settings->AddIntOption("ui.font_size", 12);
settings->AddBoolOption("renderer.vsync", true);
settings->AddFloatOption("renderer.scale", 1.0f);

// ❌ WRONG - Don't use flat key names
// settings->AddStringOption("ui_theme", "dark");
// settings->AddStringOption("theme", "dark");
```

### 4. Always Provide Default Values
**Every getter should include a sensible default value** to ensure graceful degradation.

```cpp
// ✅ CORRECT - Always provide defaults
bool vsync = settings->GetBoolOption("renderer.vsync", true);
int fontSize = settings->GetIntOption("ui.font_size", 12);
std::string theme = settings->GetStringOption("ui.theme", "dark");

// ❌ WRONG - Don't rely on hardcoded fallbacks
// bool vsync = settings->GetBoolOption("renderer.vsync"); // No default
// if (vsync == false) vsync = true; // Manual fallback
```

## Implementation Patterns

### Module Integration Pattern

**Always inherit from SettingsAwareModule** for modules that need configuration.

```cpp
class MyModule : public SettingsAwareModule
{
public:
    MyModule() : SettingsAwareModule("MyModule", "my_module") {}

protected:
    void LoadModuleConfiguration() override
    {
        auto settings = GetModuleSettings();
        
        // Load configuration with defaults
        m_Config.enabled = settings->GetBoolOption("enabled", true);
        m_Config.updateRate = settings->GetIntOption("update_rate", 60);
        m_Config.quality = settings->GetStringOption("quality", "high");
        
        SEDX_CORE_INFO_TAG("MODULE", "{} configuration loaded", GetName());
    }
    
    void SaveModuleConfiguration() override
    {
        auto settings = GetModuleSettings();
        
        // Save current configuration
        settings->AddBoolOption("enabled", m_Config.enabled);
        settings->AddIntOption("update_rate", m_Config.updateRate);
        settings->AddStringOption("quality", m_Config.quality);
        
        settings->WriteSettings();
        SEDX_CORE_INFO_TAG("MODULE", "{} configuration saved", GetName());
    }
    
    void InitializeWithSettings() override
    {
        // Apply loaded settings to module initialization
        ValidateConfiguration();
        ApplySettings();
    }

private:
    struct Configuration
    {
        bool enabled = true;
        int updateRate = 60;
        std::string quality = "high";
    } m_Config;
};
```

### X-Plane Integration Pattern

**Always use the built-in X-Plane detection and validation** for X-Plane path handling.

```cpp
void SetupXPlaneIntegration()
{
    auto settings = CreateRef<ApplicationSettings>("config/xplane.cfg");
    
    // Check existing configuration first
    if (settings->ValidateXPlanePaths())
    {
        SEDX_CORE_INFO_TAG("XPLANE", "X-Plane paths validated");
        return;
    }
    
    // Attempt automatic detection
    if (settings->DetectXPlanePath())
    {
        SEDX_CORE_INFO_TAG("XPLANE", "X-Plane detected automatically");
        settings->WriteSettings();
        return;
    }
    
    // Handle manual configuration case
    SEDX_CORE_WARN_TAG("XPLANE", "X-Plane requires manual configuration");
    // Show user dialog or provide guidance
}
```

### Error Handling Pattern

**Always use the robust error handling approach** with logging and fallbacks.

```cpp
Ref<ApplicationSettings> LoadSettingsWithFallback(const std::string& configPath)
{
    try
    {
        auto settings = CreateRef<ApplicationSettings>(configPath);
        
        // Validate critical settings exist
        if (!settings->HasOption("application.version"))
        {
            SEDX_CORE_WARN_TAG("CONFIG", "Missing application version, using defaults");
            settings->AddStringOption("application.version", AppData::versionString);
        }
        
        return settings;
    }
    catch (const std::exception& e)
    {
        SEDX_CORE_ERROR_TAG("CONFIG", "Failed to load settings: {}", e.what());
        
        // Create default configuration
        return CreateDefaultSettings(configPath);
    }
}

Ref<ApplicationSettings> CreateDefaultSettings(const std::string& configPath)
{
    auto settings = CreateRef<ApplicationSettings>(configPath);
    
    // Set essential defaults
    settings->AddStringOption("application.version", AppData::versionString);
    settings->AddStringOption("ui.theme", "dark");
    settings->AddBoolOption("project.auto_save", true);
    
    settings->WriteSettings();
    return settings;
}
```

## Required Usage Guidelines

### 1. Configuration File Naming
**Use descriptive, module-specific configuration file names.**

```cpp
// ✅ CORRECT - Descriptive names
auto appSettings = CreateRef<ApplicationSettings>("config/application.cfg");
auto renderSettings = CreateRef<ApplicationSettings>("config/renderer.cfg");
auto uiSettings = CreateRef<ApplicationSettings>("config/ui.cfg");

// ❌ WRONG - Generic or unclear names
// auto settings = CreateRef<ApplicationSettings>("config.cfg");
// auto settings = CreateRef<ApplicationSettings>("settings.dat");
```

### 2. Lifecycle Management
**Follow proper initialization and cleanup patterns.**

```cpp
class Module : public Module
{
public:
    void OnAttach() override
    {
        SEDX_CORE_INFO("=== Initializing {} ===", GetName());
        
        // Initialize settings early
        m_Settings = CreateRef<ApplicationSettings>("config/module.cfg");
        
        // Load and validate configuration
        LoadConfiguration();
        ValidateConfiguration();
        ApplyConfiguration();
    }
    
    void OnDetach() override
    {
        // Save any pending changes
        if (m_Settings)
        {
            SaveConfiguration();
            SEDX_CORE_INFO_TAG("MODULE", "{} configuration saved", GetName());
        }
        
        // Automatic cleanup via smart pointers
        m_Settings.Reset();
        
        SEDX_CORE_INFO("=== Cleaning up {} ===", GetName());
    }
};
```

### 3. Setting Validation
**Always validate settings after loading and before use.**

```cpp
void ValidateRenderSettings()
{
    auto settings = GetModuleSettings();
    
    // Validate ranges
    int maxFPS = settings->GetIntOption("max_fps", 60);
    if (maxFPS < 30 || maxFPS > 300)
    {
        SEDX_CORE_WARN_TAG("RENDER", "Invalid max FPS {}, using 60", maxFPS);
        settings->AddIntOption("max_fps", 60);
    }
    
    // Validate enum values
    std::string quality = settings->GetStringOption("quality", "high");
    if (quality != "low" && quality != "medium" && quality != "high" && quality != "ultra")
    {
        SEDX_CORE_WARN_TAG("RENDER", "Invalid quality '{}', using 'high'", quality);
        settings->AddStringOption("quality", "high");
    }
    
    // Save corrected values
    settings->WriteSettings();
}
```

## Logging Integration

### Tagged Logging Requirements
**Always use tagged logging** for settings operations with appropriate tags.

```cpp
// ✅ CORRECT - Use appropriate tags
SEDX_CORE_INFO_TAG("SETTINGS", "Configuration file loaded: {}", configPath);
SEDX_CORE_WARN_TAG("CONFIG", "Invalid setting detected: {}", settingName);
SEDX_CORE_ERROR_TAG("XPLANE", "X-Plane path validation failed: {}", xplanePath);
SEDX_CORE_TRACE_TAG("MODULE", "Setting cached: {} = {}", key, value);

// ❌ WRONG - Don't use untagged logging
// SEDX_CORE_INFO("Configuration loaded");
// std::cout << "Setting value: " << value << std::endl;
```

### Log Level Guidelines
- **TRACE:** Setting cache operations, detailed path operations
- **INFO:** Successful operations, configuration loaded/saved
- **WARN:** Invalid values corrected, fallback operations
- **ERROR:** Critical failures, corrupted configuration

## Performance Considerations

### 1. Cache Frequently Accessed Settings
**For performance-critical code, cache settings locally.**

```cpp
class PerformanceModule : public Module
{
private:
    struct CachedSettings
    {
        bool enableOptimizations;
        int updateFrequency;
        float performanceScale;
        std::chrono::steady_clock::time_point lastRefresh;
    } m_Cache;
    
    void RefreshCache()
    {
        auto settings = GetModuleSettings();
        m_Cache.enableOptimizations = settings->GetBoolOption("optimizations", true);
        m_Cache.updateFrequency = settings->GetIntOption("update_frequency", 60);
        m_Cache.performanceScale = settings->GetFloatOption("scale", 1.0f);
        m_Cache.lastRefresh = std::chrono::steady_clock::now();
    }
    
public:
    void OnUpdate() override
    {
        // Use cached values in performance-critical code
        if (m_Cache.enableOptimizations)
        {
            PerformOptimizedUpdate();
        }
        
        // Refresh cache periodically
        auto now = std::chrono::steady_clock::now();
        if (now - m_Cache.lastRefresh > std::chrono::seconds(5))
        {
            RefreshCache();
        }
    }
};
```

### 2. Batch Settings Updates
**Batch multiple setting changes before writing to disk.**

```cpp
void UpdateMultipleSettings()
{
    auto settings = GetModuleSettings();
    
    // Make all changes in memory first
    settings->AddStringOption("ui.theme", newTheme);
    settings->AddIntOption("ui.font_size", newFontSize);
    settings->AddBoolOption("ui.animations", enableAnimations);
    settings->AddFloatOption("ui.scale", uiScale);
    
    // Single write operation
    settings->WriteSettings();
    
    SEDX_CORE_INFO_TAG("UI", "Batch settings update completed");
}
```

## Common Anti-Patterns to Avoid

### 1. Don't Query Settings in Tight Loops
```cpp
// ❌ WRONG - Settings query in loop
for (int i = 0; i < 1000; ++i)
{
    bool enabled = settings->GetBoolOption("feature.enabled", true); // Expensive!
    if (enabled) ProcessItem(i);
}

// ✅ CORRECT - Cache before loop
bool featureEnabled = settings->GetBoolOption("feature.enabled", true);
for (int i = 0; i < 1000; ++i)
{
    if (featureEnabled) ProcessItem(i);
}
```

### 2. Don't Create Multiple Settings Instances for Same File
```cpp
// ❌ WRONG - Multiple instances
auto settings1 = CreateRef<ApplicationSettings>("config/app.cfg");
auto settings2 = CreateRef<ApplicationSettings>("config/app.cfg"); // Wasteful!

// ✅ CORRECT - Single instance, pass references
auto settings = CreateRef<ApplicationSettings>("config/app.cfg");
ConfigureModule1(settings);
ConfigureModule2(settings);
```

### 3. Don't Ignore Return Values
```cpp
// ❌ WRONG - Ignoring validation results
settings->SetXPlanePath(userPath); // Could fail!

// ✅ CORRECT - Check results and handle failures
if (!settings->SetXPlanePath(userPath))
{
    SEDX_CORE_ERROR_TAG("XPLANE", "Failed to set X-Plane path: {}", userPath);
    ShowErrorDialog("Invalid X-Plane path selected");
    return false;
}
```

## Testing Requirements

### Unit Testing Pattern
**Always test settings functionality** with temporary configuration files.

```cpp
TEST_CASE("Module Settings", "[module][settings]")
{
    // Use temporary path for testing
    std::filesystem::path tempPath = std::filesystem::temp_directory_path() / "test_module.cfg";
    
    SECTION("Configuration loading")
    {
        auto settings = CreateRef<ApplicationSettings>(tempPath);
        
        // Test default values
        REQUIRE(settings->GetBoolOption("enabled", true) == true);
        REQUIRE(settings->GetIntOption("update_rate", 60) == 60);
        
        // Test setting and retrieval
        settings->AddStringOption("test_value", "test_result");
        REQUIRE(settings->GetStringOption("test_value") == "test_result");
    }
    
    // Cleanup
    if (std::filesystem::exists(tempPath))
    {
        std::filesystem::remove(tempPath);
    }
}
```

## Integration with Other Systems

### 1. Module System Integration
**Always use the Module base class** for components that need settings.

```cpp
class ConfigurableModule : public Module
{
public:
    void OnAttach() override
    {
        // Initialize settings during module attachment
        InitializeSettings();
        LoadConfiguration();
        ApplyConfiguration();
    }
    
    void OnUIRender() override
    {
        // Provide UI for runtime configuration changes
        if (m_ShowConfigWindow)
        {
            RenderConfigurationUI();
        }
    }
};
```

### 2. Smart Pointer System Integration
**Follow the smart pointer lifecycle** for all settings instances.

```cpp
class SettingsManager
{
private:
    std::vector<Ref<ApplicationSettings>> m_ConfigFiles;
    
public:
    void AddConfigFile(const std::string& path)
    {
        auto settings = CreateRef<ApplicationSettings>(path);
        m_ConfigFiles.push_back(settings);
    }
    
    void SaveAllConfigurations()
    {
        for (auto& settings : m_ConfigFiles)
        {
            settings->WriteSettings();
        }
    }
    
    // Automatic cleanup when SettingsManager is destroyed
};
```

### 3. Logging System Integration
**Use the custom logging system** for all settings-related operations.

```cpp
void LogSettingsState(Ref<ApplicationSettings> settings)
{
    SEDX_CORE_INFO_TAG("SETTINGS", "Current configuration state:");
    
    if (settings->HasOption("ui.theme"))
    {
        std::string theme = settings->GetStringOption("ui.theme");
        SEDX_CORE_INFO_TAG("SETTINGS", "  UI Theme: {}", theme);
    }
    
    if (settings->HasOption("renderer.vsync"))
    {
        bool vsync = settings->GetBoolOption("renderer.vsync");
        SEDX_CORE_INFO_TAG("SETTINGS", "  VSync: {}", vsync ? "enabled" : "disabled");
    }
}
```

## Security and Validation

### 1. Path Validation
**Always validate file paths** before using them in settings.

```cpp
bool ValidateConfigPath(const std::string& configPath)
{
    try
    {
        std::filesystem::path path(configPath);
        
        // Ensure path is within allowed directories
        std::filesystem::path configDir = "config/";
        std::filesystem::path relativePath = std::filesystem::relative(path, configDir);
        
        if (relativePath.string().starts_with(".."))
        {
            SEDX_CORE_ERROR_TAG("SECURITY", "Config path outside allowed directory: {}", configPath);
            return false;
        }
        
        return true;
    }
    catch (const std::exception& e)
    {
        SEDX_CORE_ERROR_TAG("SECURITY", "Invalid config path: {}", e.what());
        return false;
    }
}
```

### 2. Input Sanitization
**Sanitize user input** before storing in settings.

```cpp
void SetUserConfigValue(const std::string& key, const std::string& userInput)
{
    // Sanitize input
    std::string sanitized = SanitizeConfigValue(userInput);
    
    if (sanitized != userInput)
    {
        SEDX_CORE_WARN_TAG("CONFIG", "User input sanitized for key: {}", key);
    }
    
    settings->AddStringOption(key, sanitized);
}

std::string SanitizeConfigValue(const std::string& input)
{
    std::string result = input;
    
    // Remove control characters
    result.erase(std::remove_if(result.begin(), result.end(), 
                               [](char c) { return std::iscntrl(c); }), 
                result.end());
    
    // Limit length
    if (result.length() > 1024)
    {
        result = result.substr(0, 1024);
    }
    
    return result;
}
```

## Summary

These instructions ensure proper usage of the Settings System in Scenery Editor X:

1. **Always use ApplicationSettings class** - Never implement custom configuration
2. **Follow smart pointer patterns** - Use CreateRef and Ref<T> properly
3. **Use hierarchical organization** - Organize settings with dot notation
4. **Provide default values** - Every getter needs sensible defaults
5. **Integrate with Module system** - Use SettingsAwareModule base class
6. **Handle X-Plane integration** - Use built-in detection and validation
7. **Implement robust error handling** - Log errors and provide fallbacks
8. **Cache for performance** - Cache frequently accessed settings
9. **Test thoroughly** - Use temporary files for unit testing
10. **Follow security practices** - Validate paths and sanitize input

Following these instructions ensures reliable, maintainable, and secure configuration management throughout the application.

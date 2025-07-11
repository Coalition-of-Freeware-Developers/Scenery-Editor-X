# Scenery Editor X - Settings System Examples and Use Cases

---

## Table of Contents

1. [Basic Usage Examples](#basic-usage-examples)
2. [Advanced Configuration Scenarios](#advanced-configuration-scenarios)
3. [X-Plane Integration Examples](#x-plane-integration-examples)
4. [Module Integration Patterns](#module-integration-patterns)
5. [Error Handling Examples](#error-handling-examples)
6. [Performance Optimization Examples](#performance-optimization-examples)
7. [Testing and Validation Examples](#testing-and-validation-examples)

---

## Basic Usage Examples

### Creating and Initializing Settings

```cpp
#include <SceneryEditorX/platform/settings.h>
#include <SceneryEditorX/core/pointers.h>
#include <SceneryEditorX/logging/logging.hpp>

namespace SceneryEditorX
{
    class ApplicationModule : public Module
    {
    public:
        void OnAttach() override
        {
            SEDX_CORE_INFO("=== Initializing Settings System ===");
          
            // Create settings with default configuration file path
            std::filesystem::path configPath = "config/settings.cfg";
            m_Settings = CreateRef<ApplicationSettings>(configPath);
          
            // Settings are automatically loaded during construction
            SEDX_CORE_INFO_TAG("MODULE", "Settings system initialized");
        }
      
        void OnDetach() override
        {
            // Save any pending changes before shutdown
            if (m_Settings)
            {
                m_Settings->WriteSettings();
                SEDX_CORE_INFO_TAG("MODULE", "Settings saved on shutdown");
            }
          
            // Automatic cleanup via smart pointers
            m_Settings.Reset();
        }
      
    private:
        Ref<ApplicationSettings> m_Settings;
    };
}
```

### Reading and Writing Basic Settings

```cpp
void ConfigureUserInterface()
{
    auto settings = CreateRef<ApplicationSettings>("config/ui.cfg");
  
    // Reading settings with defaults
    std::string theme = settings->GetStringOption("ui.theme", "dark");
    int fontSize = settings->GetIntOption("ui.font_size", 12);
    bool enableAnimations = settings->GetBoolOption("ui.animations", true);
  
    SEDX_CORE_INFO_TAG("UI", "Loading UI configuration:");
    SEDX_CORE_INFO_TAG("UI", "  Theme: {}", theme);
    SEDX_CORE_INFO_TAG("UI", "  Font Size: {}", fontSize);
    SEDX_CORE_INFO_TAG("UI", "  Animations: {}", enableAnimations ? "enabled" : "disabled");
  
    // Writing new settings
    settings->AddStringOption("ui.theme", "dark");
    settings->AddIntOption("ui.font_size", 14);
    settings->AddBoolOption("ui.show_tooltips", true);
    settings->AddFloatOption("ui.scale_factor", 1.25);
  
    // Persist changes
    settings->WriteSettings();
    SEDX_CORE_INFO_TAG("UI", "UI configuration saved");
}
```

### Working with Key-Value Settings

```cpp
void ManageUserPreferences()
{
    auto settings = CreateRef<ApplicationSettings>("config/preferences.cfg");
  
    // Check if a setting exists before reading
    if (settings->HasOption("user.last_project"))
    {
        std::string lastProject;
        settings->GetOption("user.last_project", lastProject);
        SEDX_CORE_INFO_TAG("USER", "Last project: {}", lastProject);
    }
    else
    {
        SEDX_CORE_INFO_TAG("USER", "No previous project found");
    }
  
    // Set user preferences
    settings->SetOption("user.name", "John Developer");
    settings->SetOption("user.email", "john@example.com");
    settings->SetOption("user.workspace", "C:\\Dev\\SceneryProjects");
  
    // Save and verify
    settings->WriteSettings();
  
    // Verify settings were saved
    std::string userName;
    settings->GetOption("user.name", userName);
    SEDX_CORE_VERIFY(!userName.empty(), "User name should be set");
}
```



---

## Advanced Configuration Scenarios

### Dynamic Section Creation

```cpp
void CreateDynamicConfiguration()
{
    auto settings = CreateRef<ApplicationSettings>("config/dynamic.cfg");
  
    // Create a new plugin configuration section
    const std::string pluginName = "TerrainGenerator";
    const std::string sectionPath = "plugins." + pluginName;
  
    // Add plugin-specific settings
    settings->AddBoolOption(sectionPath + ".enabled", true);
    settings->AddStringOption(sectionPath + ".version", "1.2.3");
    settings->AddIntOption(sectionPath + ".priority", 100);
    settings->AddFloatOption(sectionPath + ".quality_factor", 0.85);
  
    // Create nested configuration
    settings->AddStringOption(sectionPath + ".paths.textures", "textures/terrain/");
    settings->AddStringOption(sectionPath + ".paths.models", "models/terrain/");
    settings->AddIntOption(sectionPath + ".cache.size_mb", 512);
    settings->AddBoolOption(sectionPath + ".cache.compress", true);
  
    // Save configuration
    settings->WriteSettings();
  
    SEDX_CORE_INFO_TAG("PLUGIN", "Dynamic configuration created for {}", pluginName);
}
```

### Configuration Validation and Migration

```cpp
class ConfigurationValidator
{
public:
    static bool ValidateAndMigrate(Ref<ApplicationSettings> settings)
    {
        bool needsUpdate = false;
      
        // Check application version for migration needs
        std::string configVersion = settings->GetStringOption("application.version", "0.0.0");
        std::string currentVersion = AppData::versionString;
      
        if (configVersion != currentVersion)
        {
            SEDX_CORE_INFO_TAG("CONFIG", "Migrating configuration from {} to {}", 
                              configVersion, currentVersion);
            needsUpdate = MigrateConfiguration(settings, configVersion, currentVersion);
        }
      
        // Validate required settings exist
        needsUpdate |= EnsureRequiredSettings(settings);
      
        // Validate value ranges
        needsUpdate |= ValidateSettingRanges(settings);
      
        if (needsUpdate)
        {
            settings->WriteSettings();
            SEDX_CORE_INFO_TAG("CONFIG", "Configuration updated and saved");
        }
      
        return true;
    }
  
private:
    static bool MigrateConfiguration(Ref<ApplicationSettings> settings, 
                                   const std::string& fromVersion, 
                                   const std::string& toVersion)
    {
        bool updated = false;
      
        // Example: Migrate old setting names
        if (settings->HasOption("renderer.old_setting"))
        {
            std::string value;
            settings->GetOption("renderer.old_setting", value);
            settings->AddStringOption("renderer.new_setting", value);
            settings->RemoveOption("renderer.old_setting");
            updated = true;
          
            SEDX_CORE_INFO_TAG("CONFIG", "Migrated renderer.old_setting to renderer.new_setting");
        }
      
        // Update version
        settings->AddStringOption("application.version", toVersion);
        updated = true;
      
        return updated;
    }
  
    static bool EnsureRequiredSettings(Ref<ApplicationSettings> settings)
    {
        bool updated = false;
      
        // Ensure UI settings exist
        if (!settings->HasOption("ui.theme"))
        {
            settings->AddStringOption("ui.theme", "dark");
            updated = true;
        }
      
        if (!settings->HasOption("project.auto_save_interval"))
        {
            settings->AddIntOption("project.auto_save_interval", 5);
            updated = true;
        }
      
        return updated;
    }
  
    static bool ValidateSettingRanges(Ref<ApplicationSettings> settings)
    {
        bool updated = false;
      
        // Validate font size range
        int fontSize = settings->GetIntOption("ui.font_size", 12);
        if (fontSize < 8 || fontSize > 32)
        {
            settings->AddIntOption("ui.font_size", 12);
            updated = true;
            SEDX_CORE_WARN_TAG("CONFIG", "Font size {} out of range, reset to 12", fontSize);
        }
      
        // Validate auto-save interval
        int interval = settings->GetIntOption("project.auto_save_interval", 5);
        if (interval < 1 || interval > 60)
        {
            settings->AddIntOption("project.auto_save_interval", 5);
            updated = true;
            SEDX_CORE_WARN_TAG("CONFIG", "Auto-save interval {} out of range, reset to 5", interval);
        }
      
        return updated;
    }
};
```



---

## X-Plane Integration Examples

### Automatic X-Plane Detection and Setup

```cpp
class XPlaneIntegrationModule : public Module
{
public:
    void OnAttach() override
    {
        SEDX_CORE_INFO("=== Initializing X-Plane Integration ===");
      
        m_Settings = CreateRef<ApplicationSettings>("config/xplane.cfg");
      
        if (SetupXPlaneIntegration())
        {
            SEDX_CORE_INFO_TAG("XPLANE", "X-Plane integration ready");
            m_IsXPlaneAvailable = true;
        }
        else
        {
            SEDX_CORE_WARN_TAG("XPLANE", "X-Plane integration not available");
            m_IsXPlaneAvailable = false;
        }
    }
  
    bool SetupXPlaneIntegration()
    {
        // Check if X-Plane paths are already configured and valid
        if (m_Settings->ValidateXPlanePaths())
        {
            LogXPlaneConfiguration();
            return true;
        }
      
        SEDX_CORE_INFO_TAG("XPLANE", "X-Plane not configured, attempting detection...");
      
        // Attempt automatic detection
        if (m_Settings->DetectXPlanePath())
        {
            LogXPlaneConfiguration();
          
            // Save the detected configuration
            m_Settings->WriteSettings();
            return true;
        }
      
        // Manual configuration required
        SEDX_CORE_WARN_TAG("XPLANE", "X-Plane detection failed");
        ShowXPlaneSetupDialog();
        return false;
    }
  
    void LogXPlaneConfiguration()
    {
        const auto& xpStats = m_Settings->GetXPlaneStats();
      
        SEDX_CORE_INFO_TAG("XPLANE", "X-Plane Configuration:");
        SEDX_CORE_INFO_TAG("XPLANE", "  Version: {}", xpStats.xPlaneVersion);
        SEDX_CORE_INFO_TAG("XPLANE", "  Path: {}", xpStats.xPlanePath);
        SEDX_CORE_INFO_TAG("XPLANE", "  Bin Path: {}", xpStats.xPlaneBinPath);
        SEDX_CORE_INFO_TAG("XPLANE", "  Resources: {}", xpStats.xPlaneResourcesPath);
        SEDX_CORE_INFO_TAG("XPLANE", "  Steam Install: {}", xpStats.isSteam ? "Yes" : "No");
    }
  
    void ShowXPlaneSetupDialog()
    {
        // Implementation would show a file dialog for manual X-Plane path selection
        SEDX_CORE_INFO_TAG("XPLANE", "Please configure X-Plane path manually");
    }
  
    [[nodiscard]] bool IsXPlaneAvailable() const { return m_IsXPlaneAvailable; }
  
private:
    Ref<ApplicationSettings> m_Settings;
    bool m_IsXPlaneAvailable = false;
};
```

### Manual X-Plane Path Configuration

```cpp
void ConfigureXPlaneManually(const std::string& userSelectedPath)
{
    auto settings = CreateRef<ApplicationSettings>("config/xplane.cfg");
  
    try
    {
        SEDX_CORE_INFO_TAG("XPLANE", "Configuring X-Plane path: {}", userSelectedPath);
      
        if (settings->SetXPlanePath(userSelectedPath))
        {
            // Verify the configuration
            if (settings->ValidateXPlanePaths())
            {
                const auto& xpStats = settings->GetXPlaneStats();
              
                SEDX_CORE_INFO_TAG("XPLANE", "X-Plane configuration successful:");
                SEDX_CORE_INFO_TAG("XPLANE", "  Main Path: {}", xpStats.xPlanePath);
                SEDX_CORE_INFO_TAG("XPLANE", "  Bin Path: {}", xpStats.xPlaneBinPath);
                SEDX_CORE_INFO_TAG("XPLANE", "  Resources: {}", xpStats.xPlaneResourcesPath);
              
                // Save configuration
                settings->WriteSettings();
              
                // Test resource access
                TestXPlaneResourceAccess(settings);
            }
            else
            {
                SEDX_CORE_ERROR_TAG("XPLANE", "X-Plane path validation failed after setting");
            }
        }
        else
        {
            SEDX_CORE_ERROR_TAG("XPLANE", "Failed to set X-Plane path: {}", userSelectedPath);
        }
    }
    catch (const std::exception& e)
    {
        SEDX_CORE_ERROR_TAG("XPLANE", "Exception configuring X-Plane: {}", e.what());
    }
}

void TestXPlaneResourceAccess(Ref<ApplicationSettings> settings)
{
    const auto& xpStats = settings->GetXPlaneStats();
  
    // Test access to critical X-Plane files
    std::vector<std::string> requiredFiles = {
        xpStats.xPlaneBinPath + "/X-Plane.exe",  // Windows
        xpStats.xPlaneResourcesPath + "/default scenery",
        xpStats.xPlaneResourcesPath + "/plugins"
    };
  
    for (const auto& file : requiredFiles)
    {
        if (std::filesystem::exists(file))
        {
            SEDX_CORE_TRACE_TAG("XPLANE", "Found required file: {}", file);
        }
        else
        {
            SEDX_CORE_WARN_TAG("XPLANE", "Missing file: {}", file);
        }
    }
}
```



---

## Module Integration Patterns

### Settings-Aware Module Base Class

```cpp
class SettingsAwareModule : public Module
{
public:
    explicit SettingsAwareModule(const std::string& name, const std::string& configSection)
        : Module(name), m_ConfigSection(configSection)
    {
    }
  
    void OnAttach() override
    {
        SEDX_CORE_INFO("=== Initializing {} ===", GetName());
      
        // Load shared settings
        m_GlobalSettings = CreateRef<ApplicationSettings>("config/settings.cfg");
      
        // Load module-specific settings
        std::string configPath = fmt::format("config/{}.cfg", m_ConfigSection);
        m_ModuleSettings = CreateRef<ApplicationSettings>(configPath);
      
        // Load module configuration
        LoadModuleConfiguration();
      
        // Initialize module with loaded settings
        InitializeWithSettings();
    }
  
    void OnDetach() override
    {
        // Save module-specific settings
        SaveModuleConfiguration();
      
        SEDX_CORE_INFO("=== Cleaning up {} ===", GetName());
    }
  
protected:
    virtual void LoadModuleConfiguration() = 0;
    virtual void SaveModuleConfiguration() = 0;
    virtual void InitializeWithSettings() = 0;
  
    Ref<ApplicationSettings> GetGlobalSettings() const { return m_GlobalSettings; }
    Ref<ApplicationSettings> GetModuleSettings() const { return m_ModuleSettings; }
  
private:
    std::string m_ConfigSection;
    Ref<ApplicationSettings> m_GlobalSettings;
    Ref<ApplicationSettings> m_ModuleSettings;
};
```

### Render Module Settings Example

```cpp
class RenderModule : public SettingsAwareModule
{
public:
    RenderModule() : SettingsAwareModule("RenderModule", "renderer") {}
  
protected:
    void LoadModuleConfiguration() override
    {
        auto settings = GetModuleSettings();
      
        // Load render settings with reasonable defaults
        m_Config.vsync = settings->GetBoolOption("vsync", true);
        m_Config.multisampling = settings->GetIntOption("multisampling", 4);
        m_Config.maxFPS = settings->GetIntOption("max_fps", 60);
        m_Config.shaderQuality = settings->GetStringOption("shader_quality", "high");
        m_Config.textureQuality = settings->GetStringOption("texture_quality", "high");
        m_Config.shadowQuality = settings->GetStringOption("shadow_quality", "medium");
      
        SEDX_CORE_INFO_TAG("RENDER", "Render configuration loaded:");
        SEDX_CORE_INFO_TAG("RENDER", "  VSync: {}", m_Config.vsync);
        SEDX_CORE_INFO_TAG("RENDER", "  Multisampling: {}x", m_Config.multisampling);
        SEDX_CORE_INFO_TAG("RENDER", "  Max FPS: {}", m_Config.maxFPS);
        SEDX_CORE_INFO_TAG("RENDER", "  Shader Quality: {}", m_Config.shaderQuality);
    }
  
    void SaveModuleConfiguration() override
    {
        auto settings = GetModuleSettings();
      
        // Save current render settings
        settings->AddBoolOption("vsync", m_Config.vsync);
        settings->AddIntOption("multisampling", m_Config.multisampling);
        settings->AddIntOption("max_fps", m_Config.maxFPS);
        settings->AddStringOption("shader_quality", m_Config.shaderQuality);
        settings->AddStringOption("texture_quality", m_Config.textureQuality);
        settings->AddStringOption("shadow_quality", m_Config.shadowQuality);
      
        settings->WriteSettings();
        SEDX_CORE_INFO_TAG("RENDER", "Render configuration saved");
    }
  
    void InitializeWithSettings() override
    {
        // Initialize renderer with loaded settings
        ValidateConfiguration();
        ApplyRenderSettings();
    }
  
    void OnUIRender() override
    {
        if (m_ShowSettingsWindow)
        {
            RenderSettingsUI();
        }
    }
  
private:
    struct RenderConfig
    {
        bool vsync = true;
        int multisampling = 4;
        int maxFPS = 60;
        std::string shaderQuality = "high";
        std::string textureQuality = "high";
        std::string shadowQuality = "medium";
    } m_Config;
  
    bool m_ShowSettingsWindow = false;
  
    void ValidateConfiguration()
    {
        // Validate multisampling values
        if (m_Config.multisampling != 1 && m_Config.multisampling != 2 && 
            m_Config.multisampling != 4 && m_Config.multisampling != 8)
        {
            SEDX_CORE_WARN_TAG("RENDER", "Invalid multisampling value {}, using 4x", 
                              m_Config.multisampling);
            m_Config.multisampling = 4;
        }
      
        // Validate FPS range
        if (m_Config.maxFPS < 30 || m_Config.maxFPS > 300)
        {
            SEDX_CORE_WARN_TAG("RENDER", "Invalid max FPS {}, using 60", m_Config.maxFPS);
            m_Config.maxFPS = 60;
        }
    }
  
    void ApplyRenderSettings()
    {
        // Apply settings to render context
        SEDX_CORE_INFO_TAG("RENDER", "Applying render settings...");
      
        // Implementation would configure actual renderer
        // RenderContext::SetVSync(m_Config.vsync);
        // RenderContext::SetMultisampling(m_Config.multisampling);
    }
  
    void RenderSettingsUI()
    {
        ImGui::Begin("Render Settings", &m_ShowSettingsWindow);
      
        if (ImGui::Checkbox("VSync", &m_Config.vsync))
        {
            // Apply immediately and save
            ApplyRenderSettings();
            SaveModuleConfiguration();
        }
      
        if (ImGui::SliderInt("Multisampling", &m_Config.multisampling, 1, 8))
        {
            ValidateConfiguration();
            ApplyRenderSettings();
            SaveModuleConfiguration();
        }
      
        const char* qualityLevels[] = {"low", "medium", "high", "ultra"};
        if (ImGui::Combo("Shader Quality", &GetQualityIndex(m_Config.shaderQuality), 
                        qualityLevels, IM_ARRAYSIZE(qualityLevels)))
        {
            m_Config.shaderQuality = qualityLevels[GetQualityIndex(m_Config.shaderQuality)];
            ApplyRenderSettings();
            SaveModuleConfiguration();
        }
      
        ImGui::End();
    }
  
    int GetQualityIndex(const std::string& quality)
    {
        if (quality == "low") return 0;
        if (quality == "medium") return 1;
        if (quality == "high") return 2;
        if (quality == "ultra") return 3;
        return 2; // Default to high
    }
};
```



---

## Error Handling Examples

### Robust Settings Loading with Fallbacks

```cpp
class RobustSettingsLoader
{
public:
    static Ref<ApplicationSettings> LoadWithFallbacks(const std::string& primaryPath)
    {
        // Try primary configuration path
        try
        {
            auto settings = CreateRef<ApplicationSettings>(primaryPath);
            if (ValidateSettings(settings))
            {
                SEDX_CORE_INFO_TAG("CONFIG", "Primary configuration loaded: {}", primaryPath);
                return settings;
            }
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_WARN_TAG("CONFIG", "Primary config failed: {}", e.what());
        }
      
        // Try backup configuration
        std::string backupPath = primaryPath + ".backup";
        try
        {
            if (std::filesystem::exists(backupPath))
            {
                auto settings = CreateRef<ApplicationSettings>(backupPath);
                if (ValidateSettings(settings))
                {
                    SEDX_CORE_INFO_TAG("CONFIG", "Backup configuration loaded: {}", backupPath);
                  
                    // Restore primary from backup
                    settings->WriteSettings();
                    return settings;
                }
            }
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_WARN_TAG("CONFIG", "Backup config failed: {}", e.what());
        }
      
        // Create default configuration
        SEDX_CORE_INFO_TAG("CONFIG", "Creating default configuration");
        return CreateDefaultConfiguration(primaryPath);
    }
  
private:
    static bool ValidateSettings(Ref<ApplicationSettings> settings)
    {
        // Check required sections exist
        return settings->HasOption("application.version") &&
               settings->HasOption("ui.theme") &&
               settings->HasOption("project.auto_save");
    }
  
    static Ref<ApplicationSettings> CreateDefaultConfiguration(const std::string& path)
    {
        auto settings = CreateRef<ApplicationSettings>(path);
      
        // Set default values
        settings->AddStringOption("application.version", AppData::versionString);
        settings->AddStringOption("ui.theme", "dark");
        settings->AddIntOption("ui.font_size", 12);
        settings->AddBoolOption("project.auto_save", true);
        settings->AddIntOption("project.auto_save_interval", 5);
      
        // Try to detect X-Plane
        if (!settings->DetectXPlanePath())
        {
            SEDX_CORE_WARN_TAG("CONFIG", "X-Plane not detected in default config");
        }
      
        settings->WriteSettings();
        return settings;
    }
};
```

### Settings Corruption Recovery

```cpp
class SettingsRecoveryManager
{
public:
    static bool RecoverCorruptedSettings(const std::string& configPath)
    {
        SEDX_CORE_WARN_TAG("RECOVERY", "Attempting settings recovery for: {}", configPath);
      
        std::string corruptedPath = configPath + ".corrupted";
        std::string recoveredPath = configPath + ".recovered";
      
        try
        {
            // Move corrupted file for analysis
            if (std::filesystem::exists(configPath))
            {
                std::filesystem::rename(configPath, corruptedPath);
                SEDX_CORE_INFO_TAG("RECOVERY", "Corrupted config moved to: {}", corruptedPath);
            }
          
            // Attempt partial recovery
            auto partialData = ExtractPartialData(corruptedPath);
          
            // Create new configuration with recovered data
            auto settings = CreateRef<ApplicationSettings>(configPath);
            ApplyRecoveredData(settings, partialData);
          
            settings->WriteSettings();
            SEDX_CORE_INFO_TAG("RECOVERY", "Settings recovery completed");
          
            return true;
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("RECOVERY", "Settings recovery failed: {}", e.what());
            return false;
        }
    }
  
private:
    struct PartialData
    {
        std::map<std::string, std::string> recoveredValues;
    };
  
    static PartialData ExtractPartialData(const std::string& corruptedPath)
    {
        PartialData data;
      
        if (!std::filesystem::exists(corruptedPath))
            return data;
      
        // Simple text-based recovery for basic key-value pairs
        std::ifstream file(corruptedPath);
        std::string line;
      
        while (std::getline(file, line))
        {
            // Look for simple assignments: key = "value";
            if (auto eqPos = line.find(" = "); eqPos != std::string::npos)
            {
                std::string key = line.substr(0, eqPos);
                std::string value = line.substr(eqPos + 3);
              
                // Clean up key and value
                key = TrimWhitespace(key);
                value = TrimQuotes(TrimWhitespace(value));
              
                if (!key.empty() && !value.empty())
                {
                    data.recoveredValues[key] = value;
                    SEDX_CORE_TRACE_TAG("RECOVERY", "Recovered: {} = {}", key, value);
                }
            }
        }
      
        SEDX_CORE_INFO_TAG("RECOVERY", "Recovered {} values from corrupted config", 
                          data.recoveredValues.size());
        return data;
    }
  
    static void ApplyRecoveredData(Ref<ApplicationSettings> settings, const PartialData& data)
    {
        for (const auto& [key, value] : data.recoveredValues)
        {
            try
            {
                // Try to determine type and apply appropriately
                if (value == "true" || value == "false")
                {
                    settings->AddBoolOption(key, value == "true");
                }
                else if (IsNumeric(value))
                {
                    if (value.find('.') != std::string::npos)
                    {
                        settings->AddFloatOption(key, std::stod(value));
                    }
                    else
                    {
                        settings->AddIntOption(key, std::stoi(value));
                    }
                }
                else
                {
                    settings->AddStringOption(key, value);
                }
              
                SEDX_CORE_TRACE_TAG("RECOVERY", "Applied recovered setting: {}", key);
            }
            catch (const std::exception& e)
            {
                SEDX_CORE_WARN_TAG("RECOVERY", "Failed to apply {}: {}", key, e.what());
            }
        }
    }
  
    static std::string TrimWhitespace(const std::string& str)
    {
        size_t first = str.find_first_not_of(" \t\n\r");
        if (first == std::string::npos) return "";
      
        size_t last = str.find_last_not_of(" \t\n\r");
        return str.substr(first, (last - first + 1));
    }
  
    static std::string TrimQuotes(const std::string& str)
    {
        if (str.length() >= 2 && str.front() == '"' && str.back() == '"')
        {
            return str.substr(1, str.length() - 2);
        }
        return str;
    }
  
    static bool IsNumeric(const std::string& str)
    {
        char* end;
        std::strtod(str.c_str(), &end);
        return end != str.c_str() && *end == '\0';
    }
};
```



---

## Performance Optimization Examples

### Settings Caching Strategy

```cpp
class PerformanceOptimizedModule : public Module
{
public:
    void OnAttach() override
    {
        SEDX_CORE_INFO("=== Initializing Performance Module ===");
      
        m_Settings = CreateRef<ApplicationSettings>("config/performance.cfg");
      
        // Cache frequently accessed settings
        CacheFrequentSettings();
      
        // Set up periodic cache refresh
        SetupCacheRefresh();
    }
  
    void OnUpdate() override
    {
        SEDX_PROFILE_SCOPE("PerformanceOptimizedModule::OnUpdate");
      
        // Use cached values instead of querying settings directly
        if (m_CachedSettings.enableOptimizations)
        {
            PerformOptimizedUpdate();
        }
        else
        {
            PerformStandardUpdate();
        }
      
        // Refresh cache periodically
        RefreshCacheIfNeeded();
    }
  
private:
    struct CachedSettings
    {
        bool enableOptimizations = true;
        int updateFrequency = 60;
        float performanceScale = 1.0f;
        bool enableProfiling = false;
      
        // Cache metadata
        std::chrono::steady_clock::time_point lastRefresh;
        std::chrono::seconds refreshInterval{5}; // Refresh every 5 seconds
    } m_CachedSettings;
  
    Ref<ApplicationSettings> m_Settings;
  
    void CacheFrequentSettings()
    {
        SEDX_PROFILE_SCOPE("CacheFrequentSettings");
      
        m_CachedSettings.enableOptimizations = 
            m_Settings->GetBoolOption("performance.enable_optimizations", true);
        m_CachedSettings.updateFrequency = 
            m_Settings->GetIntOption("performance.update_frequency", 60);
        m_CachedSettings.performanceScale = 
            m_Settings->GetFloatOption("performance.scale", 1.0f);
        m_CachedSettings.enableProfiling = 
            m_Settings->GetBoolOption("performance.enable_profiling", false);
      
        m_CachedSettings.lastRefresh = std::chrono::steady_clock::now();
      
        SEDX_CORE_TRACE_TAG("PERF", "Settings cached: optimizations={}, frequency={}, scale={}", 
                           m_CachedSettings.enableOptimizations,
                           m_CachedSettings.updateFrequency,
                           m_CachedSettings.performanceScale);
    }
  
    void SetupCacheRefresh()
    {
        // Configure cache refresh interval based on setting
        int refreshSeconds = m_Settings->GetIntOption("performance.cache_refresh_seconds", 5);
        m_CachedSettings.refreshInterval = std::chrono::seconds(refreshSeconds);
    }
  
    void RefreshCacheIfNeeded()
    {
        auto now = std::chrono::steady_clock::now();
        if (now - m_CachedSettings.lastRefresh > m_CachedSettings.refreshInterval)
        {
            CacheFrequentSettings();
        }
    }
  
    void PerformOptimizedUpdate()
    {
        // Use cached values for performance-critical operations
        // No settings queries in tight loops
    }
  
    void PerformStandardUpdate()
    {
        // Standard update path
    }
};
```

### Batch Settings Operations

```cpp
void BatchUpdateSettings()
{
    auto settings = CreateRef<ApplicationSettings>("config/batch.cfg");
  
    SEDX_PROFILE_SCOPE("BatchUpdateSettings");
  
    // Collect all changes before writing
    std::vector<std::function<void()>> settingUpdates;
  
    // Queue UI settings changes
    settingUpdates.emplace_back([&]() {
        settings->AddStringOption("ui.theme", "dark");
        settings->AddIntOption("ui.font_size", 14);
        settings->AddBoolOption("ui.animations", true);
    });
  
    // Queue performance settings changes  
    settingUpdates.emplace_back([&]() {
        settings->AddBoolOption("performance.vsync", true);
        settings->AddIntOption("performance.max_fps", 60);
        settings->AddFloatOption("performance.scale", 1.0f);
    });
  
    // Queue project settings changes
    settingUpdates.emplace_back([&]() {
        settings->AddBoolOption("project.auto_save", true);
        settings->AddIntOption("project.backup_count", 5);
        settings->AddStringOption("project.default_dir", "C:\\Projects");
    });
  
    // Apply all changes
    for (auto& update : settingUpdates)
    {
        update();
    }
  
    // Single write operation
    settings->WriteSettings();
  
    SEDX_CORE_INFO_TAG("BATCH", "Batch settings update completed with {} changes", 
                      settingUpdates.size());
}
```



---

## Testing and Validation Examples

### Unit Testing Settings System

```cpp
#include <catch2/catch_test_macros.hpp>

TEST_CASE("ApplicationSettings - Basic Operations", "[settings]")
{
    // Create temporary config file for testing
    std::filesystem::path tempPath = std::filesystem::temp_directory_path() / "test_settings.cfg";
  
    SECTION("Create and initialize settings")
    {
        auto settings = CreateRef<ApplicationSettings>(tempPath);
        REQUIRE(settings != nullptr);
      
        // Verify default sections exist
        REQUIRE(settings->HasOption("application.version"));
        REQUIRE(settings->HasOption("ui.theme"));
    }
  
    SECTION("String option operations")
    {
        auto settings = CreateRef<ApplicationSettings>(tempPath);
      
        // Test setting and getting string options
        settings->SetOption("test.string", "test_value");
      
        std::string retrieved;
        settings->GetOption("test.string", retrieved);
        REQUIRE(retrieved == "test_value");
      
        // Test option existence
        REQUIRE(settings->HasOption("test.string"));
      
        // Test option removal
        settings->RemoveOption("test.string");
        REQUIRE_FALSE(settings->HasOption("test.string"));
    }
  
    SECTION("Typed option operations")
    {
        auto settings = CreateRef<ApplicationSettings>(tempPath);
      
        // Test integer options
        settings->AddIntOption("test.int", 42);
        REQUIRE(settings->GetIntOption("test.int") == 42);
        REQUIRE(settings->GetIntOption("test.missing", 99) == 99);
      
        // Test boolean options
        settings->AddBoolOption("test.bool", true);
        REQUIRE(settings->GetBoolOption("test.bool") == true);
        REQUIRE(settings->GetBoolOption("test.missing", false) == false);
      
        // Test float options
        settings->AddFloatOption("test.float", 3.14);
        REQUIRE(settings->GetFloatOption("test.float") == Approx(3.14));
      
        // Test string options
        settings->AddStringOption("test.string", "hello");
        REQUIRE(settings->GetStringOption("test.string") == "hello");
    }
  
    SECTION("Hierarchical paths")
    {
        auto settings = CreateRef<ApplicationSettings>(tempPath);
      
        // Test nested path creation
        settings->AddStringOption("level1.level2.level3.value", "deep_value");
        REQUIRE(settings->GetStringOption("level1.level2.level3.value") == "deep_value");
    }
  
    SECTION("File persistence")
    {
        {
            auto settings = CreateRef<ApplicationSettings>(tempPath);
            settings->AddStringOption("persist.test", "persistent_value");
            settings->WriteSettings();
        }
      
        // Create new instance and verify persistence
        {
            auto settings = CreateRef<ApplicationSettings>(tempPath);
            REQUIRE(settings->GetStringOption("persist.test") == "persistent_value");
        }
    }
  
    // Cleanup
    if (std::filesystem::exists(tempPath))
    {
        std::filesystem::remove(tempPath);
    }
}

TEST_CASE("ApplicationSettings - X-Plane Integration", "[settings][xplane]")
{
    std::filesystem::path tempPath = std::filesystem::temp_directory_path() / "test_xplane.cfg";
  
    SECTION("X-Plane path validation")
    {
        auto settings = CreateRef<ApplicationSettings>(tempPath);
      
        // Test with invalid path
        REQUIRE_FALSE(settings->SetXPlanePath(""));
        REQUIRE_FALSE(settings->SetXPlanePath("/invalid/path"));
      
        // Note: Valid path testing would require actual X-Plane installation
        // or mocked SteamGameFinder::validateXPlanePath
    }
  
    SECTION("X-Plane stats access")
    {
        auto settings = CreateRef<ApplicationSettings>(tempPath);
      
        const auto& xpStats = settings->GetXPlaneStats();
        REQUIRE(xpStats.xPlaneVersion.find("X-Plane") != std::string::npos);
      
        // Test modifiable access
        auto& mutableStats = settings->GetXPlaneStats();
        mutableStats.xPlaneVersion = "Test Version";
        REQUIRE(settings->GetXPlaneStats().xPlaneVersion == "Test Version");
    }
  
    // Cleanup
    if (std::filesystem::exists(tempPath))
    {
        std::filesystem::remove(tempPath);
    }
}
```

### Integration Testing Example

```cpp
class SettingsIntegrationTest
{
public:
    static void RunFullIntegrationTest()
    {
        SEDX_CORE_INFO("=== Starting Settings Integration Test ===");
      
        try
        {
            TestBasicOperations();
            TestXPlaneIntegration();
            TestModuleIntegration();
            TestErrorRecovery();
            TestPerformance();
          
            SEDX_CORE_INFO_TAG("TEST", "All integration tests passed");
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("TEST", "Integration test failed: {}", e.what());
        }
    }
  
private:
    static void TestBasicOperations()
    {
        SEDX_CORE_INFO_TAG("TEST", "Testing basic operations...");
      
        std::filesystem::path testPath = "test_configs/integration.cfg";
        auto settings = CreateRef<ApplicationSettings>(testPath);
      
        // Test all data types
        settings->AddStringOption("test.string", "integration_test");
        settings->AddIntOption("test.integer", 12345);
        settings->AddFloatOption("test.float", 98.76);
        settings->AddBoolOption("test.boolean", true);
      
        // Verify immediate access
        SEDX_ASSERT(settings->GetStringOption("test.string") == "integration_test", 
                   "String option mismatch");
        SEDX_ASSERT(settings->GetIntOption("test.integer") == 12345, 
                   "Integer option mismatch");
        SEDX_ASSERT(std::abs(settings->GetFloatOption("test.float") - 98.76) < 0.001, 
                   "Float option mismatch");
        SEDX_ASSERT(settings->GetBoolOption("test.boolean") == true, 
                   "Boolean option mismatch");
      
        // Test persistence
        settings->WriteSettings();
      
        // Reload and verify
        auto reloadedSettings = CreateRef<ApplicationSettings>(testPath);
        SEDX_ASSERT(reloadedSettings->GetStringOption("test.string") == "integration_test", 
                   "Persistence test failed");
      
        SEDX_CORE_INFO_TAG("TEST", "Basic operations test passed");
    }
  
    static void TestXPlaneIntegration()
    {
        SEDX_CORE_INFO_TAG("TEST", "Testing X-Plane integration...");
      
        std::filesystem::path testPath = "test_configs/xplane_integration.cfg";
        auto settings = CreateRef<ApplicationSettings>(testPath);
      
        // Test detection (may fail if X-Plane not installed)
        bool detected = settings->DetectXPlanePath();
        if (detected)
        {
            SEDX_ASSERT(settings->ValidateXPlanePaths(), "X-Plane path validation failed");
            SEDX_CORE_INFO_TAG("TEST", "X-Plane detected and validated");
        }
        else
        {
            SEDX_CORE_INFO_TAG("TEST", "X-Plane not detected (expected on test system)");
        }
      
        // Test manual configuration with mock path
        const auto& xpStats = settings->GetXPlaneStats();
        SEDX_ASSERT(!xpStats.xPlaneVersion.empty(), "X-Plane version should be set");
      
        SEDX_CORE_INFO_TAG("TEST", "X-Plane integration test passed");
    }
  
    static void TestModuleIntegration()
    {
        SEDX_CORE_INFO_TAG("TEST", "Testing module integration...");
      
        // Create test module
        class TestModule : public SettingsAwareModule
        {
        public:
            TestModule() : SettingsAwareModule("TestModule", "test_module") {}
          
        protected:
            void LoadModuleConfiguration() override
            {
                auto settings = GetModuleSettings();
                testValue = settings->GetStringOption("test_value", "default");
                loaded = true;
            }
          
            void SaveModuleConfiguration() override
            {
                auto settings = GetModuleSettings();
                settings->AddStringOption("test_value", testValue);
                settings->WriteSettings();
                saved = true;
            }
          
            void InitializeWithSettings() override
            {
                initialized = true;
            }
          
        public:
            std::string testValue = "default";
            bool loaded = false;
            bool saved = false;
            bool initialized = false;
        };
      
        // Test module lifecycle
        auto testModule = CreateRef<TestModule>();
        testModule->OnAttach();
      
        SEDX_ASSERT(testModule->loaded, "Module settings should be loaded");
        SEDX_ASSERT(testModule->initialized, "Module should be initialized");
      
        // Modify and save
        testModule->testValue = "modified_value";
        testModule->OnDetach();
      
        SEDX_ASSERT(testModule->saved, "Module settings should be saved");
      
        SEDX_CORE_INFO_TAG("TEST", "Module integration test passed");
    }
  
    static void TestErrorRecovery()
    {
        SEDX_CORE_INFO_TAG("TEST", "Testing error recovery...");
      
        // Create corrupted config file
        std::filesystem::path corruptedPath = "test_configs/corrupted.cfg";
        {
            std::ofstream file(corruptedPath);
            file << "corrupted config content {{{{\n";
            file << "invalid syntax !!@#$\n";
            file << "more invalid content\n";
        }
      
        // Test recovery
        bool recovered = SettingsRecoveryManager::RecoverCorruptedSettings(corruptedPath.string());
      
        if (recovered)
        {
            // Verify recovery created valid config
            auto settings = CreateRef<ApplicationSettings>(corruptedPath);
            SEDX_ASSERT(settings->HasOption("application.version"), 
                       "Recovered config should have application version");
            SEDX_CORE_INFO_TAG("TEST", "Error recovery test passed");
        }
        else
        {
            SEDX_CORE_WARN_TAG("TEST", "Error recovery test failed (expected in some cases)");
        }
    }
  
    static void TestPerformance()
    {
        SEDX_CORE_INFO_TAG("TEST", "Testing performance...");
      
        std::filesystem::path perfPath = "test_configs/performance.cfg";
        auto settings = CreateRef<ApplicationSettings>(perfPath);
      
        const int iterations = 10000;
      
        // Test read performance
        auto startTime = std::chrono::high_resolution_clock::now();
      
        for (int i = 0; i < iterations; ++i)
        {
            [[maybe_unused]] int value = settings->GetIntOption("perf.test", 42);
        }
      
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
      
        double avgMicroseconds = static_cast<double>(duration.count()) / iterations;
        SEDX_CORE_INFO_TAG("TEST", "Average read time: {:.2f} microseconds", avgMicroseconds);
      
        SEDX_ASSERT(avgMicroseconds < 10.0, "Read performance should be under 10 microseconds");
      
        SEDX_CORE_INFO_TAG("TEST", "Performance test passed");
    }
};
```



---

## Summary

These examples demonstrate the comprehensive capabilities of the Settings System in Scenery Editor X:

1. **Basic Usage** - Simple configuration management for application settings
2. **Advanced Configuration** - Dynamic section creation and configuration migration
3. **X-Plane Integration** - Automatic detection and manual configuration of X-Plane paths
4. **Module Integration** - Settings-aware modules with proper lifecycle management
5. **Error Handling** - Robust error recovery and fallback mechanisms
6. **Performance Optimization** - Caching strategies and batch operations
7. **Testing and Validation** - Comprehensive testing approaches for reliability

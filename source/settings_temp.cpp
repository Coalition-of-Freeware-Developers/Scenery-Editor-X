/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* settings.cpp
* -------------------------------------------------------
* Created: 23/4/2025
* -------------------------------------------------------
*/
#include <filesystem>
#include <libconfig.h++>
#include <regex>
#include <SceneryEditorX/platform/settings.h>
#include <SceneryEditorX/logging/logging.hpp>

// -------------------------------------------------------

namespace SceneryEditorX
{
	using namespace libconfig;
	namespace fs = std::filesystem;

    const char SteamGameFinder::dirSeparator =
	#ifdef _WIN32
	        '\\';
	#else
	        '/';
	#endif

    // -------------------------------------------------------

    /// Default section templates for use when creating config sections
    INTERNAL constexpr const char *APPLICATION_SECTION_TEMPLATE = R"(
  # Application settings
  version = "${APP_VERSION}";
  no_titlebar = false;
)";

    INTERNAL constexpr const char *XPLANE_SECTION_TEMPLATE = R"(
  # X-Plane settings
  version = "X-Plane 12.06b1";
  path = "";
  bin_path = "";
  resources_path = "";
  is_steam = false;
)";

    INTERNAL constexpr const char *UI_SECTION_TEMPLATE = R"(
  # User interface settings
  theme = "dark";
  font_size = 12;
  language = "english";
)";

    INTERNAL constexpr const char *PROJECT_SECTION_TEMPLATE = R"(
  # Project settings
  auto_save = true;
  auto_save_interval = 5; # minutes
  backup_count = 3;
  default_project_dir = "~/Documents/SceneryEditorX";
)";


    // ----------------------------------------------------------

    ApplicationSettings::ApplicationSettings(std::filesystem::path filepath) : filePath(std::move(filepath))
    {
        cfg.setOptions(
            Config::OptionAutoConvert |
            Config::OptionOpenBraceOnSeparateLine |
            Config::OptionFsync |
            Config::OptionAllowOverrides);
        cfg.setTabWidth(2);

        if (!ReadSettings())
        {
            /// If we couldn't read existing settings, create a minimal configuration
            InitMinConfig();
            configInitialized = true;

            /// Try to detect X-Plane installation
            if (DetectXPlanePath())
                SEDX_CORE_TRACE_TAG("SETTINGS", "X-Plane 12 detected and paths configured");
            else
                SEDX_CORE_WARN_TAG("SETTINGS", "X-Plane 12 installation not found");

            /// Save the initial configuration
            WriteSettings();
        }
        else
        {
            /// Validate X-Plane paths from loaded configuration
            if (!ValidateXPlanePaths())
            {
                SEDX_CORE_WARN_TAG("SETTINGS", "X-Plane paths in configuration are invalid, attempting detection");
                if (DetectXPlanePath())
                {
                    SEDX_CORE_TRACE_TAG("SETTINGS", "X-Plane 12 detected and paths updated");
                    WriteSettings();
                }
            }
        }
    }

    bool ApplicationSettings::ReadSettings()
    {
        try
        {
            // Check if file exists
            if (!fs::exists(filePath))
            {
                SEDX_CORE_WARN_TAG("SETTINGS", "Config file not found: {}", filePath.string());
                return false;
            }

            // Read the configuration file
            cfg.readFile(filePath.string().c_str());
            SEDX_CORE_INFO_TAG("SETTINGS", "Settings loaded from {}", filePath.string());

            // Read specific settings from the configuration
            try
            {
                const Setting& root = cfg.getRoot();

                // Read application settings
                if (root.exists("application"))
                {
                    const Setting& app = root["application"];
                    if (app.exists("version"))
                        app.lookupValue("version", appVersion);
                    if (app.exists("no_titlebar"))
                        app.lookupValue("no_titlebar", noTitlebar);
                }

                // Read X-Plane settings
                if (root.exists("xplane"))
                {
                    const Setting& xp = root["xplane"];
                    if (xp.exists("version"))
                        xp.lookupValue("version", xplaneVersion);
                    if (xp.exists("path"))
                        xp.lookupValue("path", xplanePath);
                    if (xp.exists("bin_path"))
                        xp.lookupValue("bin_path", xplaneBinPath);
                    if (xp.exists("resources_path"))
                        xp.lookupValue("resources_path", xplaneResourcesPath);
                    if (xp.exists("is_steam"))
                        xp.lookupValue("is_steam", xplaneIsSteam);
                }

                // Read UI settings
                if (root.exists("ui"))
                {
                    const Setting& ui = root["ui"];
                    if (ui.exists("theme"))
                        ui.lookupValue("theme", uiTheme);
                    if (ui.exists("font_size"))
                        ui.lookupValue("font_size", uiFontSize);
                    if (ui.exists("language"))
                        ui.lookupValue("language", uiLanguage);
                }

                // Read project settings
                if (root.exists("project"))
                {
                    const Setting& proj = root["project"];
                    if (proj.exists("auto_save"))
                        proj.lookupValue("auto_save", autoSave);
                    if (proj.exists("auto_save_interval"))
                        proj.lookupValue("auto_save_interval", autoSaveInterval);
                    if (proj.exists("backup_count"))
                        proj.lookupValue("backup_count", backupCount);
                    if (proj.exists("default_project_dir"))
                        proj.lookupValue("default_project_dir", defaultProjectDir);
                }

                configInitialized = true;
                return true;
            }
            catch (const SettingNotFoundException& nfex)
            {
                SEDX_CORE_WARN_TAG("SETTINGS", "Setting not found: {}", nfex.getPath());
                // Continue with defaults for missing settings
            }
            catch (const SettingTypeException& tex)
            {
                SEDX_CORE_ERROR_TAG("SETTINGS", "Setting type error: {}", tex.getPath());
                // Continue with defaults for incorrectly typed settings
            }

            return configInitialized;
        }
        catch (const FileIOException& fioex)
        {
            SEDX_CORE_ERROR_TAG("SETTINGS", "Error reading config file: {}", fioex.what());
            return false;
        }
        catch (const ParseException& pex)
        {
            SEDX_CORE_ERROR_TAG("SETTINGS", "Parse error in {} at line {}: {}",
                filePath.string(), pex.getLine(), pex.getError());
            return false;
        }
    }

    bool ApplicationSettings::WriteSettings()
    {
        try
        {
            // Create directories if they don't exist
            fs::path dir = filePath.parent_path();
            if (!dir.empty() && !fs::exists(dir))
            {
                fs::create_directories(dir);
            }

            // Update configuration object with current settings
            UpdateConfigFromSettings();

            // Write to file
            cfg.writeFile(filePath.string().c_str());
            SEDX_CORE_INFO_TAG("SETTINGS", "Settings saved to {}", filePath.string());
            return true;
        }
        catch (const FileIOException& fioex)
        {
            SEDX_CORE_ERROR_TAG("SETTINGS", "Error writing config file: {}", fioex.what());
            return false;
        }
        catch (const std::exception& ex)
        {
            SEDX_CORE_ERROR_TAG("SETTINGS", "Error saving settings: {}", ex.what());
            return false;
        }
    }

    void ApplicationSettings::UpdateConfigFromSettings()
    {
        Setting& root = cfg.getRoot();

        // Create or update application section
        if (!root.exists("application"))
            root.add("application", Setting::TypeGroup);
        Setting& app = root["application"];

        app.remove("version");
        app.add("version", Setting::TypeString) = appVersion;

        app.remove("no_titlebar");
        app.add("no_titlebar", Setting::TypeBoolean) = noTitlebar;

        // Create or update X-Plane section
        if (!root.exists("xplane"))
            root.add("xplane", Setting::TypeGroup);
        Setting& xp = root["xplane"];

        xp.remove("version");
        xp.add("version", Setting::TypeString) = xplaneVersion;

        xp.remove("path");
        xp.add("path", Setting::TypeString) = xplanePath;

        xp.remove("bin_path");
        xp.add("bin_path", Setting::TypeString) = xplaneBinPath;

        xp.remove("resources_path");
        xp.add("resources_path", Setting::TypeString) = xplaneResourcesPath;

        xp.remove("is_steam");
        xp.add("is_steam", Setting::TypeBoolean) = xplaneIsSteam;

        // Create or update UI section
        if (!root.exists("ui"))
            root.add("ui", Setting::TypeGroup);
        Setting& ui = root["ui"];

        ui.remove("theme");
        ui.add("theme", Setting::TypeString) = uiTheme;

        ui.remove("font_size");
        ui.add("font_size", Setting::TypeInt) = uiFontSize;

        ui.remove("language");
        ui.add("language", Setting::TypeString) = uiLanguage;

        // Create or update project section
        if (!root.exists("project"))
            root.add("project", Setting::TypeGroup);
        Setting& proj = root["project"];

        proj.remove("auto_save");
        proj.add("auto_save", Setting::TypeBoolean) = autoSave;

        proj.remove("auto_save_interval");
        proj.add("auto_save_interval", Setting::TypeInt) = autoSaveInterval;

        proj.remove("backup_count");
        proj.add("backup_count", Setting::TypeInt) = backupCount;

        proj.remove("default_project_dir");
        proj.add("default_project_dir", Setting::TypeString) = defaultProjectDir;
    }

    void ApplicationSettings::InitMinConfig()
    {
        // Set default values
        appVersion = "0.1.0";
        noTitlebar = false;

        xplaneVersion = "X-Plane 12.06b1";
        xplanePath = "";
        xplaneBinPath = "";
        xplaneResourcesPath = "";
        xplaneIsSteam = false;

        uiTheme = "dark";
        uiFontSize = 12;
        uiLanguage = "english";

        autoSave = true;
        autoSaveInterval = 5;
        backupCount = 3;
        defaultProjectDir = "~/Documents/SceneryEditorX";

        // Create empty config
        cfg.setRoot(Setting::TypeGroup);
    }

    bool ApplicationSettings::DetectXPlanePath()
    {
        // Basic implementation for X-Plane path detection
        // This is a placeholder - implement proper detection based on your needs

        std::vector<fs::path> possiblePaths;

        // Add common X-Plane installation locations
        #ifdef _WIN32
        possiblePaths.push_back("C:/Program Files/X-Plane 12");
        possiblePaths.push_back("D:/Program Files/X-Plane 12");
        possiblePaths.push_back("E:/Program Files/X-Plane 12");

        // Check for Steam installation
        const char* steamPath = getenv("ProgramFiles(x86)");
        if (steamPath)
        {
            possiblePaths.push_back(fs::path(steamPath) / "Steam/steamapps/common/X-Plane 12");
        }
        #elif defined(__APPLE__)
        possiblePaths.push_back("/Applications/X-Plane 12");
        #else
        possiblePaths.push_back("/opt/X-Plane 12");
        possiblePaths.push_back("~/X-Plane 12");
        #endif

        // Check each path
        for (const auto& path : possiblePaths)
        {
            if (fs::exists(path) && fs::is_directory(path))
            {
                // Check for X-Plane.exe or similar
                #ifdef _WIN32
                if (fs::exists(path / "X-Plane.exe"))
                #elif defined(__APPLE__)
                if (fs::exists(path / "X-Plane.app"))
                #else
                if (fs::exists(path / "X-Plane-x86_64"))
                #endif
                {
                    xplanePath = path.string();
                    xplaneBinPath = (path / "Resources/plugins").string();
                    xplaneResourcesPath = (path / "Resources").string();

                    // Check if it's a Steam installation
                    xplaneIsSteam = path.string().find("Steam") != std::string::npos;

                    return true;
                }
            }
        }

        return false;
    }

    bool ApplicationSettings::ValidateXPlanePaths()
    {
        // Check if X-Plane path is set and valid
        if (xplanePath.empty() || !fs::exists(xplanePath))
            return false;

        // Check for X-Plane executable
        #ifdef _WIN32
        if (!fs::exists(fs::path(xplanePath) / "X-Plane.exe"))
        #elif defined(__APPLE__)
        if (!fs::exists(fs::path(xplanePath) / "X-Plane.app"))
        #else
        if (!fs::exists(fs::path(xplanePath) / "X-Plane-x86_64"))
        #endif
            return false;

        // Check for Resources directory
        if (!fs::exists(fs::path(xplaneResourcesPath)))
            return false;

        return true;
    }

    void ApplicationSettings::SetOption(const std::string &key, const std::string &value)
    {
        settings[key] = value;

        /// Try to update the config directly
        try
        {
            /// Split the key by dots to navigate the config hierarchy
            size_t pos = 0;
            std::string section = key;
            std::string name = key;

            pos = key.find_last_of('.');
            if (pos != std::string::npos)
            {
                section = key.substr(0, pos);
                name = key.substr(pos + 1);

                /// Ensure the section exists
                try
				{
                    Setting &setting = cfg.lookup(section);
                    if (setting.exists(name))
                        setting.remove(name);

                    setting.add(name, Setting::TypeString) = value;
                }
                catch (const SettingNotFoundException &)
				{
                    CreateSettingPath(key, value);
                }
            }
            else
            {
                /// It's a root setting
                if (cfg.getRoot().exists(key))
                    cfg.getRoot().remove(key);

                cfg.getRoot().add(key, Setting::TypeString) = value;
            }
        }
        catch (...)
        {
            /// If direct update fails, we'll rely on UpdateConfigFromData() during WriteSettings()
        }
    }

    void ApplicationSettings::GetOption(const std::string &key, std::string &value)
    {
        if (const auto it = settings.find(key); it != settings.end())
            value = it->second;
    }

    bool ApplicationSettings::HasOption(const std::string &key) const
    {
        return settings.contains(key);
    }

    void ApplicationSettings::RemoveOption(const std::string &key)
    {
        settings.erase(key);

        /// Try to remove from the config directly.
        try
        {
            /// Split the key by dots to navigate the config hierarchy.
            std::string section = key;
            std::string name = key;

            if (const size_t pos = key.find_last_of('.'); pos != std::string::npos)
            {
                section = key.substr(0, pos);
                name = key.substr(pos + 1);

                /// Try to look up the setting
                if (Setting &setting = cfg.lookup(section); setting.exists(name))
                    setting.remove(name);
            }
            else
            {
                /// It's a root setting
                if (cfg.getRoot().exists(key))
                    cfg.getRoot().remove(key);
            }
        }
        catch (...)
        {
            /// If direct removal fails, we'll rely on UpdateConfigFromData() during WriteSettings()
        }
    }

    void ApplicationSettings::AddIntOption(const std::string &path, const int value)
    {
        try
        {
            if (const size_t pos = path.find_last_of('.'); pos != std::string::npos)
            {
                const std::string section = path.substr(0, pos);
                const std::string name = path.substr(pos + 1);

                try
				{
                    Setting &setting = cfg.lookup(section);
                    if (setting.exists(name))
                        setting.remove(name);
                    setting.add(name, Setting::TypeInt) = value;
                }
                catch (const SettingNotFoundException &)
                {
                    CreateSettingPath(path, value);
                }
            }
            else
            {
                if (cfg.getRoot().exists(path))
                    cfg.getRoot().remove(path);
                cfg.getRoot().add(path, Setting::TypeInt) = value;
            }

            /// Update settings map
            settings[path] = std::to_string(value);
        }
        catch (const SettingException &e)
        {
            SEDX_CORE_ERROR_TAG("SETTINGS", "Error setting int option {}: {}", path, e.what());
            CreateSettingPath(path, value);
        }
    }

    void ApplicationSettings::AddFloatOption(const std::string &path, const double value)
    {
        try
        {
            if (const size_t pos = path.find_last_of('.'); pos != std::string::npos)
            {
                const std::string section = path.substr(0, pos);
                const std::string name = path.substr(pos + 1);

                try
				{
                    Setting &setting = cfg.lookup(section);
                    if (setting.exists(name))
                        setting.remove(name);
                    setting.add(name, Setting::TypeFloat) = value;
                }
                catch (const SettingNotFoundException &)
				{
                    CreateSettingPath(path, value);
                }
            }
            else
            {
                if (cfg.getRoot().exists(path))
                    cfg.getRoot().remove(path);
                cfg.getRoot().add(path, Setting::TypeFloat) = value;
            }

            /// Update settings map
            settings[path] = std::to_string(value);
        }
        catch (const SettingException &e)
        {
            SEDX_CORE_ERROR_TAG("SETTINGS", "Error setting float option {}: {}", path, e.what());
            CreateSettingPath(path, value);
        }
    }

    void ApplicationSettings::AddBoolOption(const std::string &path, const bool value)
    {
        try
        {
            if (const size_t pos = path.find_last_of('.'); pos != std::string::npos)
            {
                const std::string section = path.substr(0, pos);
                const std::string name = path.substr(pos + 1);

                try
				{
                    Setting &setting = cfg.lookup(section);
                    if (setting.exists(name))
                        setting.remove(name);
                    setting.add(name, Setting::TypeBoolean) = value;
                }
                catch (const SettingNotFoundException &)
				{
                    CreateSettingPath(path, value);
                }
            }
            else
            {
                if (cfg.getRoot().exists(path))
                    cfg.getRoot().remove(path);
                cfg.getRoot().add(path, Setting::TypeBoolean) = value;
            }

            /// Update settings map
            settings[path] = value ? "true" : "false";
        }
        catch (const SettingException &e)
        {
            SEDX_CORE_ERROR("Error setting bool option {}: {}", path, e.what());
            CreateSettingPath(path, value);
        }
    }

    void ApplicationSettings::AddStringOption(const std::string &path, const std::string &value)
    {
        try
        {
            if (const size_t pos = path.find_last_of('.'); pos != std::string::npos)
            {
                const std::string section = path.substr(0, pos);
                const std::string name = path.substr(pos + 1);

                try
				{
                    Setting &setting = cfg.lookup(section);
                    if (setting.exists(name))
                        setting.remove(name);
                    setting.add(name, Setting::TypeString) = value;
                }
                catch (const SettingNotFoundException &)
				{
                    CreateSettingPath(path, value);
                }
            }
            else
            {
                if (cfg.getRoot().exists(path))
                    cfg.getRoot().remove(path);
                cfg.getRoot().add(path, Setting::TypeString) = value;
            }

            /// Update settings map
            settings[path] = value;
        }
        catch (const SettingException &e)
        {
            SEDX_CORE_ERROR("Error setting string option {}: {}", path, e.what());
            CreateSettingPath(path, value);
        }
    }

    bool ApplicationSettings::GetBoolOption(const std::string &path, const bool defaultValue) const
    {
        try
        {
            if (bool value; cfg.lookupValue(path, value))
                return value;
        }
        catch (...)
        {
            /// Fallthrough to default
        }
        return defaultValue;
    }

    int ApplicationSettings::GetIntOption(const std::string &path, const int defaultValue) const
    {
        try
        {
            if (int value; cfg.lookupValue(path, value))
                return value;
        }
        catch (...)
        {
            /// Fallthrough to default
        }
        return defaultValue;
    }

    double ApplicationSettings::GetFloatOption(const std::string &path, const double defaultValue) const
    {
        try
        {
            if (double value; cfg.lookupValue(path, value))
                return value;
        }
        catch (...)
        {
            /// Fallthrough to default
        }
        return defaultValue;
    }

    std::string ApplicationSettings::GetStringOption(const std::string &path, const std::string &defaultValue) const
    {
        try
        {
            std::string value;
            if (cfg.lookupValue(path, value))
                return value;
        }
        catch (...)
        {
            /// Fallthrough to default
        }
        return defaultValue;
    }

    bool ApplicationSettings::DetectXPlanePath()
    {
        /// First try to find X-Plane through Steam
        if (const auto steamPath = SteamGameFinder::findXPlane12())
        {
            SEDX_CORE_INFO("Found X-Plane 12 via Steam: {}", *steamPath);
            xPlaneStats.isSteam = true;
            return SetXPlanePath(*steamPath);
        }

        xPlaneStats.isSteam = false;
        /// If not found via Steam, try some common installation paths
        std::vector<std::string> commonPaths = {"C:/X-Plane 12",
                                                "D:/X-Plane 12",
                                                "C:/Program Files/X-Plane 12",
                                                "D:/Program Files/X-Plane 12"};


        #ifdef SEDX_PLATFORM_MACOS
        const char* homeDir = getenv("HOME");
        if (homeDir)
		{
            commonPaths.push_back(std::string(homeDir) + "/X-Plane 12");
            commonPaths.push_back(std::string(homeDir) + "/Applications/X-Plane 12");
        }
        commonPaths.push_back("/Applications/X-Plane 12");
        #endif

        #ifdef SEDX_PLATFORM_LINUX
        const char* homeDir = getenv("HOME");
        if (homeDir)
		{
            commonPaths.push_back(std::string(homeDir) + "/X-Plane 12");
        }
        #endif

        for (const auto& path : commonPaths)
		{
            if (SteamGameFinder::validateXPlanePath(path))
			{
                SEDX_CORE_TRACE("Found X-Plane 12 at common path: {}", path);
                return SetXPlanePath(path);
            }
        }

        /// Not found
        SEDX_CORE_WARN("Could not automatically detect X-Plane 12 installation");
        return false;
    }

    bool ApplicationSettings::SetXPlanePath(const std::string &path)
    {
        if (!SteamGameFinder::validateXPlanePath(path))
		{
            SEDX_CORE_ERROR_TAG("SETTINGS", "Invalid X-Plane 12 path: {}", path);
            return false;
        }

        /// Set the main path
        xPlaneStats.xPlanePath = path;

        /// Update derived paths
        UpdateDerivedXPlanePaths();

        /// Update the config
        try
		{
            if (!cfg.exists("x_plane"))
			{
                Setting &root = cfg.getRoot();
                root.add("x_plane", Setting::TypeGroup);
            }

            Setting &xp = cfg.lookup("x_plane");

            if (xp.exists("path"))
                xp.remove("path");
            xp.add("path", Setting::TypeString) = path;

            if (xp.exists("bin_path"))
                xp.remove("bin_path");
            xp.add("bin_path", Setting::TypeString) = xPlaneStats.xPlaneBinPath;

            if (xp.exists("resources_path"))
                xp.remove("resources_path");
            xp.add("resources_path", Setting::TypeString) = xPlaneStats.xPlaneResourcesPath;

            /// Update settings map
            settings["x_plane.path"] = path;
            settings["x_plane.bin_path"] = xPlaneStats.xPlaneBinPath;
            settings["x_plane.resources_path"] = xPlaneStats.xPlaneResourcesPath;

            return true;
        }
        catch (const ConfigException &e)
		{
            SEDX_CORE_ERROR_TAG("SETTINGS", "Error setting X-Plane path in config: {}", e.what());
            return false;
        }
    }

    std::string ApplicationSettings::GetXPlanePath() const
    {
        return xPlaneStats.xPlanePath;
    }

    bool ApplicationSettings::ValidateXPlanePaths() const
    {
        if (xPlaneStats.xPlanePath.empty())
            return false;

        return SteamGameFinder::validateXPlanePath(xPlaneStats.xPlanePath);
    }

    void ApplicationSettings::UpdateDerivedXPlanePaths()
    {
        const fs::path basePath = xPlaneStats.xPlanePath;

        /// Set bin path
        xPlaneStats.xPlaneBinPath = (basePath / "bin").string();

        /// Set resources path
        xPlaneStats.xPlaneResourcesPath = (basePath / "Resources").string();

        /// Detect if this is a Steam installation (could be improved with more reliable detection)
        xPlaneStats.isSteam = basePath.string().find("steamapps") != std::string::npos;
    }

    void ApplicationSettings::InitMinConfig()
    {
        try
        {
            /// Create a minimal configuration with empty sections
            std::string minimalConfig = "# Scenery Editor X Configuration\n";

            /// Add application section with correct version
            minimalConfig += "application: {";
            std::string appSection = APPLICATION_SECTION_TEMPLATE;
            if (const size_t versionPos = appSection.find("${APP_VERSION}"); versionPos != std::string::npos)
                appSection.replace(versionPos, 13, AppData::versionString);

            minimalConfig += appSection;
            minimalConfig += "};\n";

            /// Add X-Plane section
            minimalConfig += "x_plane: {";
            minimalConfig += XPLANE_SECTION_TEMPLATE;
            minimalConfig += "};\n";

            /// Add UI section
            minimalConfig += "ui: {";
            minimalConfig += UI_SECTION_TEMPLATE;
            minimalConfig += "};\n";

            /// Add project section
            minimalConfig += "project: {";
            minimalConfig += PROJECT_SECTION_TEMPLATE;
            minimalConfig += "};\n";

            /// Parse the minimal config
            cfg.readString(minimalConfig.c_str());

            /// Load settings into the map
            LoadSettingsToMap();

            SEDX_CORE_TRACE_TAG("SETTINGS", "Minimal configuration initialized");
        }
        catch (const ConfigException &e)
        {
            SEDX_CORE_WARN_TAG("SETTINGS", "Error initializing minimal config: {}", e.what());
        }
    }

    void ApplicationSettings::EnsureRequiredSections()
    {
        // Ensure application section exists
        if (!cfg.exists("application"))
		{
            Setting &root = cfg.getRoot();
            root.add("application", Setting::TypeGroup);
            Setting &app = cfg.lookup("application");
            app.add("version", Setting::TypeString) = AppData::versionString;
            app.add("no_titlebar", Setting::TypeBoolean) = appStats.NoTitlebar;
        }

        // Ensure x_plane section exists
        if (!cfg.exists("x_plane"))
		{
            Setting &root = cfg.getRoot();
            root.add("x_plane", Setting::TypeGroup);
            Setting &xp = cfg.lookup("x_plane");

            xp.add("version", Setting::TypeString) = xPlaneStats.xPlaneVersion;
            xp.add("path", Setting::TypeString) = xPlaneStats.xPlanePath;
            xp.add("bin_path", Setting::TypeString) = xPlaneStats.xPlaneBinPath;
            xp.add("resources_path", Setting::TypeString) = xPlaneStats.xPlaneResourcesPath;
            xp.add("is_steam", Setting::TypeBoolean) = xPlaneStats.isSteam;
        }

        // Ensure ui section exists
        if (!cfg.exists("ui"))
		{
            Setting &root = cfg.getRoot();
            root.add("ui", Setting::TypeGroup);

            // Only add default values if not already set
            if (!HasOption("ui.theme"))
                AddStringOption("ui.theme", "dark");
            if (!HasOption("ui.font_size"))
                AddIntOption("ui.font_size", 12);
            if (!HasOption("ui.language"))
                AddStringOption("ui.language", "english");
        }

        // Ensure project section exists
        if (!cfg.exists("project"))
		{
            Setting &root = cfg.getRoot();
            root.add("project", Setting::TypeGroup);

            // Only add default values if not already set
            if (!HasOption("project.auto_save"))
                AddBoolOption("project.auto_save", true);
            if (!HasOption("project.auto_save_interval"))
                AddIntOption("project.auto_save_interval", 5);
            if (!HasOption("project.backup_count"))
                AddIntOption("project.backup_count", 3);
            if (!HasOption("project.default_project_dir"))
			{
                /// Set default project directory
                std::string defaultDir = "~/Documents/SceneryEditorX";

                /// Replace ~ with actual home directory
                if (defaultDir.starts_with("~"))
				{
                    const char* homeDir = nullptr;
                    #ifdef _WIN32
                    homeDir = getenv("USERPROFILE");
                    #else
                    homeDir = getenv("HOME");
                    #endif

                    if (homeDir)
					{
                        defaultDir.replace(0, 1, homeDir);
                    }
                }

                AddStringOption("project.default_project_dir", defaultDir);
            }
        }
    }

    void ApplicationSettings::UpdateConfigFromData()
    {
        /// Update X-Plane data
        try
        {
            Setting &xp = cfg.lookup("x_plane");

            if (xp.exists("version"))
                xp.remove("version");
            xp.add("version", Setting::TypeString) = xPlaneStats.xPlaneVersion;

            if (xp.exists("path"))
                xp.remove("path");
            xp.add("path", Setting::TypeString) = xPlaneStats.xPlanePath;

            if (xp.exists("bin_path"))
                xp.remove("bin_path");
            xp.add("bin_path", Setting::TypeString) = xPlaneStats.xPlaneBinPath;

            if (xp.exists("resources_path"))
                xp.remove("resources_path");
            xp.add("resources_path", Setting::TypeString) = xPlaneStats.xPlaneResourcesPath;

            if (xp.exists("is_steam"))
                xp.remove("is_steam");
            xp.add("is_steam", Setting::TypeBoolean) = xPlaneStats.isSteam;
        }
        catch (const SettingNotFoundException &e)
        {
            SEDX_CORE_ERROR_TAG("SETTINGS", "X-Plane section not found: {}", e.what());
            EnsureRequiredSections();
        }

        /// Update application data
        try
        {
            Setting &app = cfg.lookup("application");

            if (app.exists("no_titlebar"))
                app.remove("no_titlebar");
            app.add("no_titlebar", Setting::TypeBoolean) = appStats.NoTitlebar;

            if (app.exists("version"))
                app.remove("version");
            app.add("version", Setting::TypeString) = AppData::versionString;
        }
        catch (const SettingNotFoundException &e)
        {
            SEDX_CORE_WARN_TAG("SETTINGS", "Application section not found: {}", e.what());
            EnsureRequiredSections();
        }
    }

    void ApplicationSettings::LoadSettingsToMap()
    {
        settings.clear();

        /// Recursive function to traverse config settings
        std::function<void(const Setting &, const std::string &)> traverseSettings;
        traverseSettings = [&](const Setting &setting, const std::string &prefix)
        {
            for (int i = 0; i < setting.getLength(); ++i)
            {
                const Setting &child = setting[i];
                std::string name = prefix.empty() ? child.getName() : prefix + "." + child.getName();

                if (child.isGroup())
                    traverseSettings(child, name);
                else
                {
                    /// Store the value as string in our map
                    switch (child.getType())
                    {
                    case Setting::TypeInt:
                        settings[name] = std::to_string(static_cast<int>(child));
                        break;
                    case Setting::TypeInt64:
                        settings[name] = std::to_string(static_cast<long long>(child));
                        break;
                    case Setting::TypeFloat:
                        settings[name] = std::to_string(static_cast<double>(child));
                        break;
                    case Setting::TypeString:
                        settings[name] = static_cast<const char *>(child);
                        break;
                    case Setting::TypeBoolean:
                        settings[name] = static_cast<bool>(child) ? "true" : "false";
                        break;
                    default:
                        /// For arrays, lists, and groups, we skip adding to settings map
                        break;
                    }
                }
            }
        };

        traverseSettings(cfg.getRoot(), "");
    }

    template <typename T>
    void ApplicationSettings::CreateSettingPath(const std::string &path, T value)
    {
        /// Split the path by dots
        std::vector<std::string> parts;
        std::string temp = path;
        size_t pos = 0;

        while ((pos = temp.find('.')) != std::string::npos)
        {
            parts.push_back(temp.substr(0, pos));
            temp.erase(0, pos + 1);
        }
        parts.push_back(temp); /// Add the last part (name)

        /// Build the path
        Setting *current = &cfg.getRoot();
        std::string currentPath;

        for (size_t i = 0; i < parts.size() - 1; ++i)
        {
            if (!currentPath.empty())
                currentPath += ".";
            currentPath += parts[i];

            try
            {
                current = &(current->lookup(parts[i]));
            }
            catch (const SettingNotFoundException &)
            {
                /// Create the group if it doesn't exist
                current = &(current->add(parts[i], Setting::TypeGroup));
            }
        }

        /// Add the final value
        const std::string name = parts.back();
        if (current->exists(name))
            current->remove(name);

        if constexpr (std::is_same_v<T, int>)
        {
            current->add(name, Setting::TypeInt) = value;
            settings[path] = std::to_string(value);
        }
        else if constexpr (std::is_same_v<T, double>)
        {
            current->add(name, Setting::TypeFloat) = value;
            settings[path] = std::to_string(value);
        }
        else if constexpr (std::is_same_v<T, bool>)
        {
            current->add(name, Setting::TypeBoolean) = value;
            settings[path] = value ? "true" : "false";
        }
        else if constexpr (std::is_same_v<T, std::string>)
        {
            current->add(name, Setting::TypeString) = value;
            settings[path] = value;
        }
    }

    std::optional<std::string> SteamGameFinder::findXPlane12()
    {
        /// Get Steam directory
        const std::string steamDir = getSteamDirectory();
        if (steamDir.empty())
        {
            SEDX_CORE_WARN_TAG("SETTINGS", "Steam directory not found!");
            return std::nullopt;
        }

        /// Get all Steam library folders
        std::vector<std::string> libraryFolders = getSteamLibraryFolders(steamDir);

        /// Add default Steam apps directory
        libraryFolders.push_back(steamDir + dirSeparator + "steamapps");

        /// Search for X-Plane 12 in each library folder
        for (const auto &library : libraryFolders)
            if (auto xplanePath = checkForXPlane12(library))
                return xplanePath;

        return std::nullopt;
    }

    bool SteamGameFinder::validateXPlanePath(const std::string &path)
    {
        if (path.empty())
            return false;

        fs::path basePath = path;

        /// Check if the directory exists
        if (!fs::exists(basePath) || !fs::is_directory(basePath))
            return false;

        /// Check for essential subdirectories
        if (!fs::exists(basePath / "Resources") || !fs::is_directory(basePath / "Resources"))
            return false;

        if (!fs::exists(basePath / "bin") || !fs::is_directory(basePath / "bin"))
            return false;

        /// Check for X-Plane executable
        #ifdef SEDX_PLATFORM_WINDOWS
        if (!fs::exists(basePath / "bin" / "X-Plane.exe"))
            return false;

        #elif defined(SEDX_PLATFORM_MACOS)
        if (!fs::exists(basePath / "X-Plane.app"))
            return false;

        #else
        if (!fs::exists(basePath / "bin" / "X-Plane-x86_64"))
            return false;

        #endif

        /// Check for Resources/default data directory
        if (!fs::exists(basePath / "Resources" / "default data") || !fs::is_directory(basePath / "Resources" / "default data"))
            return false;

        // -------------------------------------------------------

        return true;
    }

    std::string SteamGameFinder::getSteamDirectory()
    {
        std::string steamPath;

	#ifdef SEDX_PLATFORM_WINDOWS
	    /// Windows: Check registry or default locations
	    /// Default is usually C:\Program Files (x86)\Steam
	    char programFiles[MAX_PATH];
	    if (SHGetFolderPathA(nullptr, CSIDL_PROGRAM_FILESX86, nullptr, 0, programFiles) == S_OK)
		{
	        steamPath = std::string(programFiles) + "\\Steam";
	        if (!fs::exists(steamPath))
			{
	            /// Try alternative locations (common alternative installations)
	            steamPath = "C:\\Steam";
	            if (!fs::exists(steamPath))
				{
	                steamPath = "D:\\Steam";
	                if (!fs::exists(steamPath))
					{
	                    return "";
	                }
	            }
	        }
	    }
    #elif defined(SEDX_PLATFORM_MACOS)
	    /// macOS: Check default location
	    const char* homeDir = getenv("HOME");
	    if (homeDir)
		{
	        steamPath = std::string(homeDir) + "/Library/Application Support/Steam";
	        if (!fs::exists(steamPath))
			{
	            return "";
	        }
	    }
	#elif defined(SEDX_PLATFORM_LINUX)
	    /// Linux: Check default location
	    const char* homeDir = getenv("HOME");
	    if (homeDir)
		{
	        steamPath = std::string(homeDir) + "/.steam/steam";
	        if (!fs::exists(steamPath))
			{
	            steamPath = std::string(homeDir) + "/.local/share/Steam";
	            if (!fs::exists(steamPath))
				{
	                return "";
	            }
	        }
	    }
	#endif

	    return steamPath;
    }

    std::vector<std::string> SteamGameFinder::getSteamLibraryFolders(const std::string &steamPath)
    {
        std::vector<std::string> libraries;

        /// Path to the Steam library config file
        std::string configPath = steamPath + dirSeparator + "steamapps" + dirSeparator + "libraryfolders.vdf";

        if (!fs::exists(configPath))
        {
            return libraries;
        }

        /// Read the libraryfolders.vdf file
        std::ifstream file(configPath);
        if (!file.is_open())
        {
            return libraries;
        }

        std::string line;
        std::regex pathRegex("\"path\"\\s+\"(.+?)\"");

        while (std::getline(file, line))
        {
            if (std::smatch match; std::regex_search(line, match, pathRegex) && match.size() > 1)
            {
                std::string libraryPath = match[1].str();

                /// Fix path separators for Windows paths in the VDF file
                if (dirSeparator == '\\')
                {
                    std::ranges::replace(libraryPath, '/', '\\');
                }

                /// Add steamapps subdirectory
                libraryPath += dirSeparator;
                libraryPath += "steamapps";

                if (fs::exists(libraryPath))
                {
                    libraries.push_back(libraryPath);
                }
            }
        }

        return libraries;
    }

    std::optional<std::string> SteamGameFinder::checkForXPlane12(const std::string &libraryPath)
    {
        /// Check common folder paths for X-Plane 12
        if (std::string commonPath = libraryPath + dirSeparator + "common"; fs::exists(commonPath))
        {
            /// Check for X-Plane 12 directory
            std::string xplanePath = commonPath + dirSeparator + "X-Plane 12";
            if (fs::exists(xplanePath))
            {
                return xplanePath;
            }

            /// Also try other possible folder names
            xplanePath = commonPath + dirSeparator + "X-Plane12";
            if (fs::exists(xplanePath))
            {
                return xplanePath;
            }

            xplanePath = commonPath + dirSeparator + "XPlane12";
            if (fs::exists(xplanePath))
            {
                return xplanePath;
            }

            xplanePath = commonPath + dirSeparator + "X-Plane-12";
            if (fs::exists(xplanePath))
            {
                return xplanePath;
            }
        }

        /// Check for manifest file for X-Plane 12
        for (const auto &entry : fs::directory_iterator(libraryPath))
        {
            if (std::string path = entry.path().string(); path.find("appmanifest") != std::string::npos && path.find(".acf") != std::string::npos)
            {
                std::ifstream manifestFile(path);
                std::string line;
                while (std::getline(manifestFile, line))
                {
                    /// Look for app ID for X-Plane 12 or name in the manifest
                    if ((line.find("\"appid\"") != std::string::npos && line.find("2014780") != std::string::npos) ||
                        (line.find("\"name\"") != std::string::npos && line.find("X-Plane 12") != std::string::npos))
                    {
                        /// Found X-Plane 12 manifest, extract install dir
                        std::regex installDirRegex("\"installdir\"\\s+\"(.+?)\"");
                        std::string installDir;

                        while (std::getline(manifestFile, line))
                        {
                            if (std::smatch match; std::regex_search(line, match, installDirRegex) && match.size() > 1)
                            {
                                installDir = match[1].str();
                                if (std::string fullPath = libraryPath + dirSeparator + "common" + dirSeparator + installDir; fs::exists(fullPath))
                                {
                                    return fullPath;
                                }
                                break;
                            }
                        }
                    }
                }
            }
        }

        return std::nullopt;
    }

    bool SteamGameFinder::savePathToConfig(const std::string &path, const std::string &configFile)
    {
        Config cfg;

        /// Try to read existing config
        try
        {
            cfg.readFile(configFile.c_str());
        }
        catch (const FileIOException &ex)
        {
            SEDX_CORE_ERROR_TAG("SETTINGS", "Error reading config file: {}", ex.what());
            /// File doesn't exist, we'll create a new one
        }
        catch (const ParseException &ex)
        {
            SEDX_CORE_ERROR_TAG("SETTINGS", "Parse error in config file at line: {} : {}", ex.getLine(), ex.getError());
            return false;
        }

        /// Set the X-Plane 12 path in the config
        try
        {
            /// Check if paths section exists, create if not
            if (!cfg.exists("paths"))
            {
                cfg.getRoot().add("paths", Setting::TypeGroup);
            }

            Setting &paths = cfg.lookup("paths");

            /// Remove existing setting if it exists
            if (paths.exists("xplane12"))
            {
                paths.remove("xplane12");
            }

            /// Add the new path
            paths.add("xplane12", Setting::TypeString) = path;

            /// Write to file
            cfg.writeFile(configFile.c_str());
            return true;
        }
        catch (const SettingException &ex)
        {
            SEDX_CORE_ERROR_TAG("SETTINGS", "Error in config setting: {}", ex.what());
            return false;
        }
        catch (const FileIOException &ex)
        {
            SEDX_CORE_ERROR_TAG("SETTINGS", "Error writing config file: {}", ex.what());
            return false;
        }

        return false;
    }

    /*
	VkDeviceSize ApplicationSettings::GetCustomBufferSize() const
	{
	    // If not specified, return the default value
	    return GetIntOption("vulkan.custom_buffer_size", static_cast<int>(CUSTOM_BUFFER_SIZE));
	}
	*/

	bool ApplicationSettings::SetCustomBufferSize(VkDeviceSize size)
	{
	    // Store in settings
	    AddIntOption("vulkan.custom_buffer_size", static_cast<int>(size));
	    return true;
	}

	bool ApplicationSettings::ValidateBufferSize(VkDeviceSize size, const VkPhysicalDeviceLimits& deviceLimits)
	{
	    // Check that buffer size is not larger than the maximum allowed by the device
	    if (size > deviceLimits.maxStorageBufferRange)
	    {
	        SEDX_CORE_ERROR_TAG("SETTINGS", "Requested buffer size ({} bytes) exceeds device maximum ({} bytes)",
	            size, deviceLimits.maxStorageBufferRange);
	        return false;
	    }

	    // Ensure the size is a multiple of minStorageBufferOffsetAlignment
	    if (size % deviceLimits.minStorageBufferOffsetAlignment != 0)
	    {
	        SEDX_CORE_WARN_TAG("SETTINGS", "Buffer size {} is not aligned to device requirements ({})",
	            size, deviceLimits.minStorageBufferOffsetAlignment);
	        return false;
	    }

	    return true;
	}
} // namespace SceneryEditorX

// -------------------------------------------------------

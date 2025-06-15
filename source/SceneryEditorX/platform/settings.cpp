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
#include <SceneryEditorX/core/steam_parser.h>
#include <SceneryEditorX/platform/settings.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	using namespace libconfig;
	namespace fs = std::filesystem;

    /// -------------------------------------------------------

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

    /// ----------------------------------------------------------


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
            if (!fs::exists(filePath))
            {
                SEDX_CORE_TRACE_TAG("SETTINGS", "Config file not found: {}", filePath.string());
                return false;
            }
            
            cfg.readFile(filePath.string().c_str());
            SEDX_CORE_TRACE_TAG("SETTINGS", "Reading settings from: {}", filePath.string());

			/*
			try
            {
                if (const Setting &root = cfg.getRoot(); root.exists("application"))
				{
                    const Setting &app = root["application"];
                    if (app.exists("version"))
                        app.lookup("version", appStats.version);
				}

            }
			*/

            /// Load X-Plane stats
            if (cfg.exists("x_plane"))
            {
                const Setting &xp = cfg.lookup("x_plane");

                if (xp.exists("version"))
                    xPlaneStats.xPlaneVersion = static_cast<const char *>(xp["version"]);

                if (xp.exists("path"))
                    xPlaneStats.xPlanePath = static_cast<const char *>(xp["path"]);

                if (xp.exists("bin_path"))
                    xPlaneStats.xPlaneBinPath = static_cast<const char *>(xp["bin_path"]);

                if (xp.exists("resources_path"))
                    xPlaneStats.xPlaneResourcesPath = static_cast<const char *>(xp["resources_path"]);

                if (xp.exists("is_steam"))
                    xp.lookupValue("is_steam", xPlaneStats.isSteam);
            }

            /// Load Application stats
            if (cfg.exists("application"))
            {
                SEDX_CORE_INFO_TAG("SETTINGS", "Loading SceneryEditorX settings");
                if (const Setting &app = cfg.lookup("application");
                    app.exists("no_titlebar"))
                    app.lookupValue("no_titlebar", appStats.NoTitlebar);
            }

            /// Populate the settings map for quick access
            LoadSettingsToMap();
            configInitialized = true;

            return true;
        }
        catch (const FileIOException &fioex)
        {
            SEDX_CORE_ERROR_TAG("SETTINGS", "Error trying to read application settings: {}", filePath.string());
            return false;
        }
        catch (const ParseException &pex)
        {
            SEDX_CORE_ERROR_TAG("SETTINGS","Parse error at {}:{} - {}", pex.getFile(), pex.getLine(), pex.getError());
            return false;
        }
        catch (const ConfigException &confex)
        {
            SEDX_CORE_ERROR_TAG("SETTINGS", "Config error while reading file: {}", confex.what());
            return false;
        }
    }

    void ApplicationSettings::WriteSettings()
    {
        try
        {
            /// Ensure all required sections exist before writing.
            EnsureRequiredSections();
            
            /// Update the config from our data structures.
            UpdateConfigFromData();

            /// Write config to file.
            cfg.writeFile(filePath.string().c_str());
            SEDX_CORE_INFO_TAG("SETTINGS", "Settings successfully written to: {}", filePath.string());
            
            /// Update the settings map
            LoadSettingsToMap();
        }
        catch (const FileIOException &fioex)
        {
            SEDX_CORE_ERROR_TAG("SETTINGS", "Error writing settings to file: {}", filePath.string());
        }
        catch (const ConfigException &confex)
        {
            SEDX_CORE_ERROR_TAG("SETTINGS", "Config error while writing file: {}", confex.what());
        }
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
        /// Ensure application section exists
        if (!cfg.exists("application"))
		{
            Setting &root = cfg.getRoot();
            root.add("application", Setting::TypeGroup);
            Setting &app = cfg.lookup("application");
            app.add("version", Setting::TypeString) = AppData::versionString;
            app.add("no_titlebar", Setting::TypeBoolean) = appStats.NoTitlebar;
        }
        
        /// Ensure x_plane section exists
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
    void ApplicationSettings::CreateSettingPath(const std::string &path, const T &value)
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

    /*
	VkDeviceSize ApplicationSettings::GetCustomBufferSize() const
	{
	    // If not specified, return the default value
	    return GetIntOption("vulkan.custom_buffer_size", static_cast<int>(CUSTOM_BUFFER_SIZE));
	}
	*/

	/*
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
	*/

} // namespace SceneryEditorX

/// -------------------------------------------------------

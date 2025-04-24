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

#include <SceneryEditorX/platform/settings.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
    using namespace libconfig;

	// -------------------------------------------------------

    /// Default configuration structure
    INTERNAL const char *DEFAULT_CONFIG = R"(
# Scenery Editor X Configuration
# Auto-generated default configuration

application:
{
  # Application settings
  version = "1.0";
  no_titlebar = false;
};

x_plane:
{
  # X-Plane settings
  version = "X-Plane 12.06b1";
  path = "C:/X-Plane 12";
  bin_path = "C:/X-Plane 12/bin";
  resources_path = "C:/X-Plane 12/Resources";
  is_steam = false;
};

# User interface settings
ui:
{
  theme = "dark";
  font_size = 12;
  language = "english";
};

# Project settings
project:
{
  auto_save = true;
  auto_save_interval = 5; # minutes
  backup_count = 3;
  default_project_dir = "~/Documents/SceneryEditorX";
};
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
            // If we failed to read settings, initialize with defaults
            InitializeDefaultConfig();
            WriteSettings();
        }
	}

    bool ApplicationSettings::ReadSettings()
    {
        try
        {
            cfg.readFile(filePath.string().c_str());

            // Load X-Plane stats
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

            // Load Application stats
            if (cfg.exists("application"))
            {

                if (const Setting &app = cfg.lookup("application");
					app.exists("no_titlebar"))
                    app.lookupValue("no_titlebar", appStats.NoTitlebar);
            }

            // Populate the settings map for quick access
            LoadSettingsToMap();

            return true;
        }
        catch (const FileIOException &fioex)
        {
            SEDX_CORE_ERROR("Error trying to read user settings: {}", filePath.string());
            return false;
        }
        catch (const ParseException &pex)
        {
            SEDX_CORE_ERROR("Parse error at {}:{} - {}", pex.getFile(), pex.getLine(), pex.getError());
            return false;
        }
        catch (const ConfigException &confex)
        {
            SEDX_CORE_ERROR("Config error while reading file: {}", confex.what());
            return false;
        }
    }

    void ApplicationSettings::WriteSettings()
    {
        try
        {
            // Update the config from our data structures before writing
            UpdateConfigFromData();

            // Write the config to file
            cfg.writeFile(filePath.string().c_str());
            SEDX_CORE_INFO("Settings successfully written to: {}", filePath.string());
        }
        catch (const FileIOException &fioex)
        {
            SEDX_CORE_ERROR("Error writing settings to file: {}", filePath.string());
        }
        catch (const ConfigException &confex)
        {
            SEDX_CORE_ERROR("Config error while writing file: {}", confex.what());
        }
    }

    void ApplicationSettings::SetOption(const std::string &key, const std::string &value)
    {
        settings[key] = value;

        // Try to update the config directly
        try
        {
            // Split the key by dots to navigate the config hierarchy
            size_t pos = 0;
            std::string section = key;
            std::string name = key;

            pos = key.find_last_of('.');
            if (pos != std::string::npos)
            {
                section = key.substr(0, pos);
                name = key.substr(pos + 1);

                // Try to look up the setting
                Setting &setting = cfg.lookup(section);
                if (setting.exists(name))
                {
                    setting.remove(name);
                }
                setting.add(name, Setting::TypeString) = value;
            }
            else
            {
                // It's a root setting
                if (cfg.getRoot().exists(key))
                {
                    cfg.getRoot().remove(key);
                }
                cfg.getRoot().add(key, Setting::TypeString) = value;
            }
        }
        catch (...)
        {
            // If direct update fails, we'll rely on UpdateConfigFromData() during WriteSettings()
        }
    }

    void ApplicationSettings::GetOption(const std::string &key, std::string &value)
    {
        if (const auto it = settings.find(key); it != settings.end())
        {
            value = it->second;
        }
    }

    bool ApplicationSettings::HasOption(const std::string &key) const
    {
        return settings.contains(key);
    }

    void ApplicationSettings::RemoveOption(const std::string &key)
    {
        settings.erase(key);

        // Try to remove from the config directly
        try
        {
            // Split the key by dots to navigate the config hierarchy
            std::string section = key;
            std::string name = key;

            if (const size_t pos = key.find_last_of('.'); pos != std::string::npos)
            {
                section = key.substr(0, pos);
                name = key.substr(pos + 1);

                // Try to look up the setting
                if (Setting &setting = cfg.lookup(section); setting.exists(name))
                {
                    setting.remove(name);
                }
            }
            else
            {
                // It's a root setting
                if (cfg.getRoot().exists(key))
                {
                    cfg.getRoot().remove(key);
                }
            }
        }
        catch (...)
        {
            // If direct removal fails, we'll rely on UpdateConfigFromData() during WriteSettings()
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

                Setting &setting = cfg.lookup(section);
                setting.remove(name);
                setting.add(name, Setting::TypeInt) = value;
            }
            else
            {
                cfg.getRoot().remove(path);
                cfg.getRoot().add(path, Setting::TypeInt) = value;
            }

            // Update settings map
            settings[path] = std::to_string(value);
        }
        catch (const SettingNotFoundException &e)
        {
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

                Setting &setting = cfg.lookup(section);
                setting.remove(name);
                setting.add(name, Setting::TypeFloat) = value;
            }
            else
            {
                cfg.getRoot().remove(path);
                cfg.getRoot().add(path, Setting::TypeFloat) = value;
            }

            // Update settings map
            settings[path] = std::to_string(value);
        }
        catch (const SettingNotFoundException &e)
        {
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

                Setting &setting = cfg.lookup(section);
                setting.remove(name);
                setting.add(name, Setting::TypeBoolean) = value;
            }
            else
            {
                cfg.getRoot().remove(path);
                cfg.getRoot().add(path, Setting::TypeBoolean) = value;
            }

            // Update settings map
            settings[path] = value ? "true" : "false";
        }
        catch (const SettingNotFoundException &e)
        {
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

                Setting &setting = cfg.lookup(section);
                setting.remove(name);
                setting.add(name, Setting::TypeString) = value;
            }
            else
            {
                cfg.getRoot().remove(path);
                cfg.getRoot().add(path, Setting::TypeString) = value;
            }

            // Update settings map
            settings[path] = value;
        }
        catch (const SettingNotFoundException &e)
        {
            CreateSettingPath(path, value);
        }
    }

    bool ApplicationSettings::GetBoolOption(const std::string &path, const bool defaultValue)
    {
        try
        {
            if (bool value; cfg.lookupValue(path, value))
                return value;
        }
        catch (...)
        {
            // Fallthrough to default
        }
        return defaultValue;
    }

    int ApplicationSettings::GetIntOption(const std::string &path, const int defaultValue)
    {
        try
        {
            if (int value; cfg.lookupValue(path, value))
                return value;
        }
        catch (...)
        {
            // Fallthrough to default
        }
        return defaultValue;
    }

    double ApplicationSettings::GetFloatOption(const std::string &path, const double defaultValue)
    {
        try
        {
            if (double value; cfg.lookupValue(path, value))
                return value;
        }
        catch (...)
        {
            // Fallthrough to default
        }
        return defaultValue;
    }

    std::string ApplicationSettings::GetStringOption(const std::string &path, const std::string &defaultValue)
    {
        try
        {
            std::string value;
            if (cfg.lookupValue(path, value))
                return value;
        }
        catch (...)
        {
            // Fallthrough to default
        }
        return defaultValue;
    }

    void ApplicationSettings::InitializeDefaultConfig()
    {
        try
        {
            // Parse the default config string
            cfg.readString(DEFAULT_CONFIG);

            // Set application metadata
            const Setting &appSetting = cfg.lookup("application");
            appSetting["version"] = SoftwareStats::versionString;

            // Update X-Plane paths if they exist already
            const Setting &xpSetting = cfg.lookup("x_plane");
            xpSetting["version"] = xPlaneStats.xPlaneVersion;
            xpSetting["path"] = xPlaneStats.xPlanePath;
            xpSetting["bin_path"] = xPlaneStats.xPlaneBinPath;
            xpSetting["resources_path"] = xPlaneStats.xPlaneResourcesPath;
            xpSetting["is_steam"] = xPlaneStats.isSteam;

            // Load settings into the map
            LoadSettingsToMap();

            SEDX_CORE_INFO("Default configuration initialized");
        }
        catch (const ConfigException &e)
        {
            SEDX_CORE_ERROR("Error initializing default config: {}", e.what());
        }
    }

    void ApplicationSettings::UpdateConfigFromData()
    {
        // Update X-Plane data
        try
        {
            const Setting &xp = cfg.lookup("x_plane");
            xp["version"] = xPlaneStats.xPlaneVersion;
            xp["path"] = xPlaneStats.xPlanePath;
            xp["bin_path"] = xPlaneStats.xPlaneBinPath;
            xp["resources_path"] = xPlaneStats.xPlaneResourcesPath;
            xp["is_steam"] = xPlaneStats.isSteam;
        }
        catch (const SettingNotFoundException &e)
        {
            // If x_plane section doesn't exist, create it
            Setting &root = cfg.getRoot();
            Setting &xp = root.add("x_plane", Setting::TypeGroup);
            xp.add("version", Setting::TypeString) = xPlaneStats.xPlaneVersion;
            xp.add("path", Setting::TypeString) = xPlaneStats.xPlanePath;
            xp.add("bin_path", Setting::TypeString) = xPlaneStats.xPlaneBinPath;
            xp.add("resources_path", Setting::TypeString) = xPlaneStats.xPlaneResourcesPath;
            xp.add("is_steam", Setting::TypeBoolean) = xPlaneStats.isSteam;
        }

        // Update application data
        try
        {
            const Setting &app = cfg.lookup("application");
            app["no_titlebar"] = appStats.NoTitlebar;
            app["version"] = SoftwareStats::versionString;
        }
        catch (const SettingNotFoundException &e)
        {
            // If application section doesn't exist, create it
            Setting &root = cfg.getRoot();
            Setting &app = root.add("application", Setting::TypeGroup);
            app.add("no_titlebar", Setting::TypeBoolean) = appStats.NoTitlebar;
            app.add("version", Setting::TypeString) = SoftwareStats::versionString;
        }
    }

    void ApplicationSettings::LoadSettingsToMap()
    {
        settings.clear();

        // Recursive function to traverse config settings
        std::function<void(const Setting &, const std::string &)> traverseSettings;
        traverseSettings = [&](const Setting &setting, const std::string &prefix) {
            for (int i = 0; i < setting.getLength(); ++i)
            {
                const Setting &child = setting[i];
                std::string name = prefix.empty() ? child.getName() : prefix + "." + child.getName();

                if (child.isGroup())
                {
                    traverseSettings(child, name);
                }
                else
                {
                    // Store the value as string in our map
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
                    case Setting::TypeNone:
                        break;
                    case Setting::TypeGroup:
                        break;
                    case Setting::TypeArray:
                        break;
                    case Setting::TypeList:
                        break;
                    default:
                        // For arrays, lists, and groups, we skip adding to settings map
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
        // Split the path by dots
        std::vector<std::string> parts;
        std::string temp = path;
        size_t pos = 0;

        while ((pos = temp.find('.')) != std::string::npos)
        {
            parts.push_back(temp.substr(0, pos));
            temp.erase(0, pos + 1);
        }
        parts.push_back(temp); // Add the last part (name)

        // Build the path
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
            catch (const SettingNotFoundException &e)
            {
                // Create the group if it doesn't exist
                current = &(current->add(parts[i], Setting::TypeGroup));
            }
        }

        // Add the final value
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

} // namespace SceneryEditorX

// -------------------------------------------------------

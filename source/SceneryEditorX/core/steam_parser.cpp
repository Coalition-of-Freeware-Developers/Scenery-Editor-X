/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* steam_parser.cpp
* -------------------------------------------------------
* Created: 13/6/2025
* -------------------------------------------------------
*/
#include <filesystem>
#include <libconfig.h++>
#include <regex>
#include <SceneryEditorX/core/steam_parser.h>

/// -------------------------------------------------------

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

    /// -------------------------------------------------------

	#ifdef SEDX_PLATFORM_WINDOWS
        INTERNAL std::wstring GetRegValue(HKEY hKey, const std::wstring &subKey, const std::wstring &valueName)
		{
	        HKEY openedKey = nullptr;
	        if (RegOpenKeyExW(hKey, subKey.c_str(), 0, KEY_READ, &openedKey) != ERROR_SUCCESS)
				return L"";
	
			DWORD dataSize = 0;
	        RegQueryValueExW(openedKey, valueName.c_str(), nullptr, nullptr, nullptr, &dataSize);
			if (dataSize == 0)
			{
	            RegCloseKey(openedKey);
				return L"";
			}
	
			std::wstring value(dataSize / sizeof(wchar_t), L'\0');
			RegQueryValueExW(openedKey, valueName.c_str(), nullptr, nullptr, reinterpret_cast<LPBYTE>(value.data()), &dataSize);
	        RegCloseKey(openedKey);
			return value;
	    }
	#endif

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
        if (!fs::exists(basePath / "Resources" / "default data") ||
            !fs::is_directory(basePath / "Resources" / "default data"))
            return false;

        /// -------------------------------------------------------

        return true;
    }

    std::string SteamGameFinder::getSteamDirectory()
    {
        std::string steamPath;

    #ifdef SEDX_PLATFORM_WINDOWS
        /// Windows: Check registry or default locations
        /// Default is usually C:\Program Files (x86)\Steam
        const std::wstring steamRegPath = L"HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Valve\\Steam";
        std::wstring steamInstallPath = GetRegValue(HKEY_LOCAL_MACHINE, steamRegPath, L"InstallPath");
        if (!steamInstallPath.empty())
            SEDX_CORE_WARN_TAG("Settings", "Could not find the Steam Library install registry key.");
        else
        {
            /// Convert to UTF-8 string
			steamPath = std::string(steamInstallPath.begin(), steamInstallPath.end());
			if (fs::exists(steamPath))
				return steamPath;
        }
        /// Convert to UTF-8 string
        steamPath = std::string(steamInstallPath.begin(), steamInstallPath.end());

	#elif defined(SEDX_PLATFORM_MACOS)
	
	    #error Not implemented yet
	
	#elif defined(SEDX_PLATFORM_LINUX)
	
	    #error Not implemented yet
	
	#endif

        return steamPath;
    }

    std::vector<std::string> SteamGameFinder::getSteamLibraryFolders(const std::string &steamPath)
    {
        std::vector<std::string> libraries;

        /// Path to the Steam library config file
        std::string configPath = steamPath + dirSeparator + "steamapps" + dirSeparator + "libraryfolders.vdf";

        if (!fs::exists(configPath))
            return libraries;

        /// Read the libraryfolders.vdf file
        std::ifstream file(configPath);
        if (!file.is_open())
            return libraries;

        std::string line;
        std::regex pathRegex("\"path\"\\s+\"(.+?)\"");

        while (std::getline(file, line))
        {
            if (std::smatch match; std::regex_search(line, match, pathRegex) && match.size() > 1)
            {
                std::string libraryPath = match[1].str();

                /// Add steamapps subdirectory
                libraryPath += dirSeparator;
                libraryPath += "steamapps";

                if (fs::exists(libraryPath))
                    libraries.push_back(libraryPath);
            }
        }

        return libraries;
    }

    std::optional<std::string> SteamGameFinder::findXPlane12()
    {
        std::string steamPath = getSteamDirectory();
        if (steamPath.empty())
            return std::nullopt;

        std::vector<std::string> libraries = getSteamLibraryFolders(steamPath);
        
        /// Add the main Steam library to the list if not already present
        if (std::string mainLibrary = steamPath + dirSeparator + "steamapps"; fs::exists(mainLibrary))
        {
            if (std::ranges::find(libraries, mainLibrary) == libraries.end())
                libraries.push_back(mainLibrary);
        }

        /// Check for manifest file for X-Plane 12
        for (const auto &library : libraries)
        {
            for (std::string manifestsPath = library; const auto &entry : fs::directory_iterator(manifestsPath))
                if (std::string path = entry.path().string(); path.find("appmanifest_") != std::string::npos && path.find(".acf") != std::string::npos)
                {
                    std::ifstream manifestFile(path);
                    std::string manifestLine;
                    while (std::getline(manifestFile, manifestLine))
                    {
                        /// Look for app ID for X-Plane 12 or name in the manifest
                        if (manifestLine.find("\"appid\"") != std::string::npos && manifestLine.find("2014780") != std::string::npos)
                        {
                            /// Found X-Plane 12 manifest, extract install dir
                            std::regex installDirRegex("\"installdir\"\\s+\"(.+?)\"");
                            std::string installDir;

                            while (std::getline(manifestFile, manifestLine))
                            {
                                if (std::smatch match; std::regex_search(manifestLine, match, installDirRegex) && match.size() > 1)
                                {
                                    installDir = match[1].str();
                                    if (std::string fullPath = library + dirSeparator + "common" + dirSeparator += installDir; fs::exists(fullPath) && validateXPlanePath(fullPath))
                                    {
                                        return fullPath;
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }

            /// Check common folder paths for X-Plane 12
            if (std::string commonPath = library + dirSeparator + "common"; fs::exists(commonPath))
            {
                /// Check for X-Plane 12 directory with various possible names
                std::vector<std::string> possibleNames = {
                    "X-Plane 12",
                    "X-Plane12",
                    "XPlane12",
                    "X-Plane-12"
                };

                for (const auto &name : possibleNames)
                {
                    if (std::string xplanePath = commonPath + dirSeparator += name; fs::exists(xplanePath) && validateXPlanePath(xplanePath))
                        return xplanePath;
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
                cfg.getRoot().add("paths", Setting::TypeGroup);

            Setting &paths = cfg.lookup("paths");

            /// Remove existing setting if it exists
            if (paths.exists("xplane12"))
                paths.remove("xplane12");

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

    }

	/// -------------------------------------------------------
}

/// -------------------------------------------------------

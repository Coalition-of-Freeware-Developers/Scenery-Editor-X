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
#include <fstream>
#include <libconfig.h++>
#include <regex>
#include <SceneryEditorX/platform/settings/steam_parser.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    using namespace libconfig;
	namespace fs = std::filesystem;

	const char SteamGameFinder::dirSeparator =
	#ifdef _WIN32S
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

    bool SteamGameFinder::ValidateXPlanePath(const std::string &path)
    {
        if (path.empty())
		{
            SEDX_CORE_TRACE_TAG("SETTINGS", "validateXPlanePath: Path is empty");
            return false;
        }

        const fs::path basePath = path;
        SEDX_CORE_TRACE_TAG("SETTINGS", "Validating X-Plane path: {}", basePath.string());

        /// Check if the directory exists
        if (!fs::exists(basePath))
		{
            SEDX_CORE_TRACE_TAG("SETTINGS", "validateXPlanePath: Directory does not exist: {}", basePath.string());
            return false;
        }

        if (!fs::is_directory(basePath))
		{
            SEDX_CORE_TRACE_TAG("SETTINGS", "validateXPlanePath: Path is not a directory: {}", basePath.string());
            return false;
        }

        /// Check for essential subdirectories
        if (!fs::exists(basePath / "Resources"))
		{
            SEDX_CORE_TRACE_TAG("SETTINGS", "validateXPlanePath: Missing Resources directory");
            return false;
        }

        if (!fs::is_directory(basePath / "Resources"))
		{
            SEDX_CORE_TRACE_TAG("SETTINGS", "validateXPlanePath: Resources is not a directory");
            return false;
        }

        if (!fs::exists(basePath / "bin"))
		{
            SEDX_CORE_TRACE_TAG("SETTINGS", "validateXPlanePath: Missing bin directory");
            return false;
        }

        if (!fs::is_directory(basePath / "bin"))
		{
            SEDX_CORE_TRACE_TAG("SETTINGS", "validateXPlanePath: bin is not a directory");
            return false;
        }

    /// Check for X-Plane executable
#ifdef SEDX_PLATFORM_WINDOWS
        if (!fs::exists(basePath / "bin" / "X-Plane.exe"))
		{
            SEDX_CORE_TRACE_TAG("SETTINGS", "validateXPlanePath: X-Plane.exe not found");
            return false;
        }
#elif defined(SEDX_PLATFORM_MACOS)
        if (!fs::exists(basePath / "X-Plane.app"))
		{
            SEDX_CORE_TRACE_TAG("SETTINGS", "validateXPlanePath: X-Plane.app not found");
            return false;
        }
#else
        if (!fs::exists(basePath / "bin" / "X-Plane-x86_64"))
		{
            SEDX_CORE_TRACE_TAG("SETTINGS", "validateXPlanePath: X-Plane-x86_64 not found");
            return false;
        }
#endif

        /// Check for Resources/default data directory
        if (!fs::exists(basePath / "Resources" / "default data"))
		{
            SEDX_CORE_TRACE_TAG("SETTINGS", "validateXPlanePath: Missing Resources/default data directory");
            return false;
        }

        if (!fs::is_directory(basePath / "Resources" / "default data"))
		{
            SEDX_CORE_TRACE_TAG("SETTINGS", "validateXPlanePath: Resources/default data is not a directory");
            return false;
        }

        SEDX_CORE_TRACE_TAG("SETTINGS", "Path is a valid X-Plane 12 installation: {}", basePath.string());
        return true;
    }

    std::string SteamGameFinder::GetSteamDirectory()
    {
        std::string steamPath;

    #ifdef SEDX_PLATFORM_WINDOWS
        /// Windows: Check registry or default locations
        /// Try WOW6432Node first (Steam in 32-bit registry view on 64-bit Windows)
        const std::wstring steamRegPath = L"SOFTWARE\\WOW6432Node\\Valve\\Steam";
        std::wstring steamInstallPath = GetRegValue(HKEY_LOCAL_MACHINE, steamRegPath, L"InstallPath");

        /// If not found, try without WOW6432Node
        if (steamInstallPath.empty())
        {
            const std::wstring steamRegPathAlt = L"SOFTWARE\\Valve\\Steam";
            steamInstallPath = GetRegValue(HKEY_LOCAL_MACHINE, steamRegPathAlt, L"InstallPath");
        }

        /// If still not found, try current user registry
        if (steamInstallPath.empty())
		{
            const std::wstring steamRegPathUser = L"SOFTWARE\\Valve\\Steam";
            steamInstallPath = GetRegValue(HKEY_CURRENT_USER, steamRegPathUser, L"SteamPath");
        }

        if (!steamInstallPath.empty())
		{
            /// Convert to UTF-8 string
            steamPath = std::string(steamInstallPath.begin(), steamInstallPath.end());

            /// Replace forward slashes with backslashes for consistent path format
            std::ranges::replace(steamPath, '/', '\\');

            if (fs::exists(steamPath))
                return steamPath;
        }
        else
            SEDX_CORE_WARN_TAG("Settings", "Could not find the Steam Library install registry key");

        /// Try default installation paths
        const std::vector<std::string> defaultPaths = {
            "C:\\Program Files (x86)\\Steam",
            "C:\\Program Files\\Steam",
            "D:\\Program Files (x86)\\Steam",
            "D:\\Program Files\\Steam",
            "E:\\Program Files (x86)\\Steam",
            "E:\\Program Files\\Steam",
            "F:\\Program Files (x86)\\Steam",
            "F:\\Program Files\\Steam",
            "G:\\Program Files (x86)\\Steam",
            "G:\\Program Files\\Steam"
        };

        for (const auto& path : defaultPaths)
            if (fs::exists(path))
            {
                SEDX_CORE_TRACE_TAG("Settings", "Found Steam at default path: {}", path);
                return path;
            }

    #elif defined(SEDX_PLATFORM_MACOS)

	    #error Not implemented yet

	#elif defined(SEDX_PLATFORM_LINUX)

	    #error Not implemented yet

	#endif

        return steamPath;
    }

    std::vector<std::string> SteamGameFinder::GetSteamLibraryFolders(const std::string &steamPath)
    {
        std::vector<std::string> libraries;

        /// Add the main Steam library
        if (std::string mainLibrary = steamPath + dirSeparator + "steamapps"; fs::exists(mainLibrary))
		{
            libraries.push_back(mainLibrary);
            SEDX_CORE_TRACE_TAG("Settings", "Added main Steam library: {}", mainLibrary);
        }

        /// Path to the Steam library config file
        std::string configPath = steamPath + dirSeparator + "steamapps" + dirSeparator + "libraryfolders.vdf";

        SEDX_CORE_TRACE_TAG("Settings", "Looking for Steam library config at: {}", configPath);

        if (!fs::exists(configPath))
		{
            SEDX_CORE_WARN_TAG("Settings", "Steam library config file not found: {}", configPath);
            return libraries;
        }

        /// Read the libraryfolders.vdf file
        std::ifstream file(configPath);
        if (!file.is_open())
		{
            SEDX_CORE_WARN_TAG("Settings", "Could not open Steam library config file: {}", configPath);
            return libraries;
        }

        std::string line;
        std::regex pathRegex("\"path\"\\s+\"(.+?)\"");
        std::regex directPathRegex("\"\\d+\"\\s+\"(.+?)\"");  /// For older format like "1" "D:\\SteamLibrary"

        bool inLibraryFolders = false;

        while (std::getline(file, line))
        {
            /// Keep track if we're in the libraryfolders section
            if (line.find("\"libraryfolders\"") != std::string::npos)
			{
                inLibraryFolders = true;
                continue;
            }

            if (inLibraryFolders)
			{
                /// Check for path field
                if (std::smatch match; std::regex_search(line, match, pathRegex) && match.size() > 1)
				{
                    std::string libraryPath = match[1].str();

                    /// Replace escaped backslashes (if any)
                    size_t pos = 0;
                    while ((pos = libraryPath.find("\\\\", pos)) != std::string::npos)
					{
                        libraryPath.replace(pos, 2, "\\");
                        pos += 1;
                    }

                    /// Add steamapps subdirectory
                    if (std::string steamappsPath = libraryPath + dirSeparator + "steamapps"; fs::exists(steamappsPath))
					{
                        libraries.push_back(steamappsPath);
                        SEDX_CORE_TRACE_TAG("Settings", "Found Steam library from path format: {}", steamappsPath);
                    }
                }
                /// Check for direct path (older format)
                else if (std::regex_search(line, match, directPathRegex) && match.size() > 1)
				{
                    std::string libraryPath = match[1].str();

                    /// Replace escaped backslashes (if any)
                    size_t pos = 0;
                    while ((pos = libraryPath.find("\\\\", pos)) != std::string::npos)
					{
                        libraryPath.replace(pos, 2, "\\");
                        pos += 1;
                    }

                    /// Add steamapps subdirectory if not already there
                    std::string steamappsPath;
                    if (libraryPath.find("steamapps") == std::string::npos)
                        steamappsPath = libraryPath + dirSeparator + "steamapps";
                    else
                        steamappsPath = libraryPath;

                    if (fs::exists(steamappsPath))
					{
                        libraries.push_back(steamappsPath);
                        SEDX_CORE_TRACE_TAG("Settings", "Found Steam library from numeric format: {}", steamappsPath);
                    }
                }
            }
        }

        /// If no libraries were found using the VDF file, try to find them manually
        if (libraries.size() <= 1)
		{
            SEDX_CORE_TRACE_TAG("Settings", "Searching for additional Steam libraries by checking common paths");

            std::vector<std::string> commonDrives = {"C:", "D:", "E:", "F:", "G:"};
            std::vector<std::string> commonPaths = {
                "\\SteamLibrary\\steamapps",
                "\\Steam\\steamapps",
                "\\Steam Library\\steamapps",
                R"(\Games\Steam\steamapps)",
                R"(\Games\SteamLibrary\steamapps)",
                R"(\Program Files\Steam\steamapps)",
                R"(\Program Files (x86)\Steam\steamapps)"
            };

            for (const auto& drive : commonDrives)
			{
                for (const auto& path : commonPaths)
				{
                    if (std::string fullPath = drive + path; fs::exists(fullPath) && std::ranges::find(libraries, fullPath) == libraries.end())
                    {
                        /// Check if this path is already in our libraries list
                        libraries.push_back(fullPath);
                        SEDX_CORE_TRACE_TAG("Settings", "Found additional Steam library: {}", fullPath);
                    }
                }
            }
        }

        return libraries;
    }

    std::optional<std::string> SteamGameFinder::FindXPlane12()
    {
        std::string steamPath = GetSteamDirectory();
        if (steamPath.empty())
		{
            SEDX_CORE_WARN_TAG("Settings", "Could not locate Steam installation directory");
            return std::nullopt;
        }

        SEDX_CORE_TRACE_TAG("Settings", "Found Steam installation at: {}", steamPath);

        std::vector<std::string> libraries = GetSteamLibraryFolders(steamPath);

        if (libraries.empty())
		{
            SEDX_CORE_WARN_TAG("Settings", "No Steam library folders found");
            return std::nullopt;
        }

        SEDX_CORE_TRACE_TAG("Settings", "Found {} Steam libraries", libraries.size());

        /// Check for manifest file for X-Plane 12
        for (const auto &library : libraries)
        {
            SEDX_CORE_TRACE_TAG("Settings", "Scanning library: {}", library);

            /// First, check common folder directly - it's faster
            if (std::string commonPath = library.substr(0, library.find("steamapps")) + dirSeparator + "steamapps" + dirSeparator + "common"; fs::exists(commonPath))
            {
                SEDX_CORE_TRACE_TAG("Settings", "Checking common folder: {}", commonPath);

                /// Check for X-Plane 12 directory with various possible names
                std::vector<std::string> possibleNames = {
                    "X-Plane 12",
                    "X-Plane12",
                    "XPlane12",
                    "X-Plane-12"
                };

                for (const auto &name : possibleNames)
                {
                    std::string xplanePath = commonPath + dirSeparator + name;
                    SEDX_CORE_TRACE_TAG("Settings", "Checking potential X-Plane path: {}", xplanePath);

                    if (fs::exists(xplanePath))
					{
                        if (ValidateXPlanePath(xplanePath))
						{
                            SEDX_CORE_INFO_TAG("Settings", "Found X-Plane 12 using common name pattern: {}", xplanePath);
                            return xplanePath;
                        }

                        SEDX_CORE_WARN_TAG("Settings", "Found X-Plane directory but validation failed: {}", xplanePath);
                    }
                }
            }

            /// If not found by direct check, try checking manifest files
            try
			{
                for (fs::path libraryPath(library); const auto &entry : fs::directory_iterator(libraryPath))
                {
                    /// Look for app manifest files
                    if (std::string path = entry.path().string(); path.find("appmanifest_") != std::string::npos && path.find(".acf") != std::string::npos)
                    {
                        SEDX_CORE_TRACE_TAG("Settings", "Checking manifest file: {}", path);

                        std::ifstream manifestFile(path);
                        if (!manifestFile.is_open())
                            continue;

                        std::string manifestLine;
                        bool isXPlane12 = false;
                        while (std::getline(manifestFile, manifestLine))
                        {
                            /// Look for app ID for X-Plane 12 (2014780)
                            if (manifestLine.find("\"appid\"") != std::string::npos && manifestLine.find("2014780") != std::string::npos)
                            {
                                SEDX_CORE_TRACE_TAG("Settings", "Found X-Plane 12 manifest file: {}", path);
                                isXPlane12 = true;
                                break;
                            }
                        }

                        if (isXPlane12)
						{
                            /// Reopen the file to search for installdir
                            manifestFile.clear();
                            manifestFile.seekg(0);

                            std::regex installDirRegex("\"installdir\"\\s+\"(.+?)\"");
                            std::string installDir;

                            while (std::getline(manifestFile, manifestLine))
                            {
                                if (std::smatch match; std::regex_search(manifestLine, match, installDirRegex) && match.size() > 1)
                                {
                                    installDir = match[1].str();

                                    /// Construct the full path to the X-Plane installation
                                    fs::path libBasePath = libraryPath.parent_path(); // Go up one level from steamapps
                                    std::string fullPath = (libBasePath / "steamapps" / "common" / installDir).string();

                                    SEDX_CORE_TRACE_TAG("Settings", "Found X-Plane 12 install dir: {} -> {}", installDir, fullPath);

                                    if (fs::exists(fullPath))
									{
                                        if (ValidateXPlanePath(fullPath))
										{
                                            SEDX_CORE_INFO_TAG("Settings", "Found valid X-Plane 12 installation: {}", fullPath);
                                            return fullPath;
                                        }

                                        SEDX_CORE_WARN_TAG("Settings", "X-Plane 12 directory exists but validation failed: {}", fullPath);
                                    }
                                    else
                                        SEDX_CORE_WARN_TAG("Settings", "X-Plane 12 directory from manifest doesn't exist: {}", fullPath);

                                    break;
                                }
                            }
                        }
                    }
                }
            }
            catch (const std::exception& e)
			{
                SEDX_CORE_WARN_TAG("Settings", "Error scanning Steam library: {}", e.what());
            }
        }

        SEDX_CORE_WARN_TAG("Settings", "Could not find X-Plane 12 in any Steam library");
        return std::nullopt;
    }

    bool SteamGameFinder::SavePathToConfig(const std::string &path, const std::string &configFile)
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

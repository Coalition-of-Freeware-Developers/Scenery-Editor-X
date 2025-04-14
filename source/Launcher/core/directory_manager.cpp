/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* directory_manager.cpp
* -------------------------------------------------------
* Created: 16/3/2025
* -------------------------------------------------------
*/

#include <Launcher/core/directory_manager.hpp>

// -------------------------------------------------------

namespace fs = std::filesystem;

// Define the static members
// TODO: When registry is implemented, remove the static members and replace with registry HKEY Values

std::string DirectoryInit::absolutePath;
std::string DirectoryInit::relativePath;

void DirectoryInit::ensureDirectoriesExist(const std::vector<std::string> &directories)
{
    fs::path exeDir; // The directory containing the executable

#ifdef SEDX_DEBUG
    // Get the executable's directory in debug mode
    exeDir = fs::path(absolutePath).parent_path();
#else
    HKEY hKey;
    const char *subKey = "SOFTWARE\\Scenery Editor X";
    const char *absPathValue = "AbsolutePath";
    char absPathBuffer[MAX_PATH];
    DWORD bufferSize = MAX_PATH;

    // Open the registry key
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, subKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        // Read the absolute path
        if (RegQueryValueExA(hKey, absPathValue, NULL, NULL, (LPBYTE)absPathBuffer, &bufferSize) == ERROR_SUCCESS)
        {
            exeDir = fs::path(std::string(absPathBuffer)).parent_path();
            LAUNCHER_LOG_INFO("Using registry Absolute Path: {}", exeDir.string());
            //std::cerr << "Using registry Absolute Path: " << exeDir.string() << std::endl;
        }
        else
        {
            LAUNCHER_LOG_ERROR("Failed to read AbsolutePath from registry.");
			//ErrMsg("Failed to read AbsolutePath from registry.");
            //std::cerr << "Failed to read AbsolutePath from registry." << std::endl;

            RegCloseKey(hKey);
            return;
        }

        // Close the registry key
        RegCloseKey(hKey);
    }
    else
    {
        LAUNCHER_LOG_ERROR("Failed to open registry key.");
		//ErrMsg("Failed to open registry key.");
        //std::cerr << "Failed to open registry key." << std::endl;

        return;
    }
#endif

    for (const auto &dir : directories)
    {
        // Construct the full path for each directory
        // Check if the directory exists, if not create it
        if (fs::path fullPath = exeDir / dir; !fs::exists(fullPath))
        {
            fs::create_directories(fullPath);

            LAUNCHER_LOG_INFO("Created directory: {}", fullPath.string());
            //std::cerr << "Created directory: " << fullPath.string() << std::endl;
        }
        else
        {
            LAUNCHER_LOG_INFO("Directory already exists: {}", fullPath.string());
            //std::cerr << "Directory already exists: " << fullPath.string() << std::endl;
        }
    }
}

int DirectoryInit::DirectoryCheck(const int argc, char *argv[])
{
#ifdef SEDX_DEBUG
    if (argc > 0) // Used to be (argc > 1) but in debug mode it will not find the executable and fail)
    {
        // Set the absolute path to the executable's path
        absolutePath = fs::absolute(fs::path(argv[0])).string();
        LAUNCHER_LOG_INFO("============================================");
        LAUNCHER_LOG_INFO("Absolute Path: {}", absolutePath);

        // Set the relative path to the current working directory relative to the executable
        relativePath = fs::relative(fs::current_path(), absolutePath).string();
        LAUNCHER_LOG_INFO("Relative Path: {}", relativePath);
        LAUNCHER_LOG_INFO("============================================");
    }
    else
    {
        LAUNCHER_LOG_CRITICAL("No executable path found.");
		//ErrMsg("No executable path found.");

        return -1;
    }
#else
    HKEY hKey;
    const char *subKey = "SOFTWARE\\Scenery Editor X";
    const char *absPathValue = "AbsolutePath";
    const char *relPathValue = "RelativePath";
    char absPathBuffer[MAX_PATH];
    char relPathBuffer[MAX_PATH];
    DWORD bufferSize = MAX_PATH;

    // Open the registry key
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, subKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        // Read the absolute path
        if (RegQueryValueExA(hKey, absPathValue, NULL, NULL, (LPBYTE)absPathBuffer, &bufferSize) == ERROR_SUCCESS)
        {
            absolutePath = std::string(absPathBuffer);
            LAUNCHER_LOG_INFO("============================================");
            LAUNCHER_LOG_INFO("Absolute Path: {}", absolutePath);
            //std::cerr << "Absolute Path: " << absolutePath << std::endl;
        }
        else
        {
            LAUNCHER_LOG_CRITICAL("Failed to read AbsolutePath from registry.");
			//ErrMsgBox("Failed to read AbsolutePath from registry.");
            //std::cerr << "Failed to read AbsolutePath from registry." << std::endl;
            RegCloseKey(hKey);
            return -1;
        }

        // Read the relative path
        bufferSize = MAX_PATH; // Reset buffer size
        if (RegQueryValueExA(hKey, relPathValue, NULL, NULL, (LPBYTE)relPathBuffer, &bufferSize) == ERROR_SUCCESS)
        {
            relativePath = std::string(relPathBuffer);
            LAUNCHER_LOG_INFO("Relative Path: {}", relativePath);
            //std::cerr << "Relative Path: " << relativePath << std::endl;
            LAUNCHER_LOG_INFO("============================================");
        }
        else
        {
            LAUNCHER_LOG_CRITICAL("Failed to read RelativePath from registry.");
			//ErrMsgBox("Failed to read RelativePath from registry.");
            //std::cerr << "Failed to read RelativePath from registry." << std::endl;

            RegCloseKey(hKey);
            return -1;
        }

        // Close the registry key
        RegCloseKey(hKey);
    }
    else
    {
        LAUNCHER_LOG_CRITICAL("Failed to open registry key.");
		//ErrMsg("Failed to open registry key.");
        //std::cerr << "Failed to open registry key." << std::endl;

        return -1;
    }
#endif

    // Define the required directory structure
    const std::vector<std::string> requiredDirectories = {"assets",
                                                    "assets/models",
                                                    "assets/textures",
                                                    "docs",
                                                    "config",
                                                    "export",
                                                    "librarys",
                                                    "librarys/custom",
                                                    "logs",
                                                    "plugins",
                                                    "projects",
                                                    "resources",
                                                    "resources/cache",
                                                    "resources/cache/shaders",
                                                    "resources/cache/thumbnail"};

    // Ensure that the required directories exist
    ensureDirectoriesExist(requiredDirectories);

    return 0;
}

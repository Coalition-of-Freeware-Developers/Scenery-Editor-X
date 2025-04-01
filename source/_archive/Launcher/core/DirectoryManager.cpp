#include <filesystem>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

#include "DirectoryManager.hpp"
#include <iostream>

namespace fs = std::filesystem;

// Define the static members
// TODO: When Registery is implemented, remove the static members and replace with registry HKEY Values

std::string DirectoryInit::absolutePath = "";
std::string DirectoryInit::relativePath = "";

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
            spdlog::info("Using registry Absolute Path: {}", exeDir.string());
            std::cerr << "Using registry Absolute Path: " << exeDir.string() << std::endl;
        }
        else
        {
            spdlog::critical("Failed to read AbsolutePath from registry.");
            std::cerr << "Failed to read AbsolutePath from registry." << std::endl;

            RegCloseKey(hKey);
            return;
        }

        // Close the registry key
        RegCloseKey(hKey);
    }
    else
    {
        spdlog::critical("Failed to open registry key.");
        std::cerr << "Failed to open registry key." << std::endl;

        return;
    }
#endif

    for (const auto &dir : directories)
    {
        // Construct the full path for each directory
        fs::path fullPath = exeDir / dir;

        // Check if the directory exists, if not create it
        if (!fs::exists(fullPath))
        {
            fs::create_directories(fullPath);

            spdlog::info("Created directory: {}", fullPath.string());
            std::cerr << "Created directory: " << fullPath.string() << std::endl;
        }
        else
        {
            spdlog::info("Directory already exists: {}", fullPath.string());
            std::cerr << "Directory already exists: " << fullPath.string() << std::endl;
        }
    }
}

int DirectoryInit::DirectoryCheck(int argc, char *argv[])
{
#ifdef SEDX_DEBUG
    if (argc > 0) // Used to be (argc > 1) but in debug mode it will not find the executable and fail)
    {
        // Set the absolute path to the executable's path
        absolutePath = fs::absolute(fs::path(argv[0])).string();
        spdlog::info("============================================");
        std::cerr << "============================================" << std::endl;
        spdlog::info("Absolute Path: {}", absolutePath);
        std::cerr << "Absolute Path: " << absolutePath << std::endl;

        // Set the relative path to the current working directory relative to the executable
        relativePath = fs::relative(fs::current_path(), absolutePath).string();
        spdlog::info("Relative Path: {}", relativePath);
        std::cerr << "Relative Path: " << relativePath << std::endl;
        spdlog::info("============================================");
        std::cerr << "============================================" << std::endl;
    }
    else
    {
        spdlog::critical("No executable path found.");
        std::cerr << "No executable path found." << std::endl;

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
            spdlog::info("============================================");
            std::cerr << "============================================" << std::endl;
            spdlog::info("Absolute Path: {}", absolutePath);
            std::cerr << "Absolute Path: " << absolutePath << std::endl;
        }
        else
        {
            spdlog::critical("Failed to read AbsolutePath from registry.");
            std::cerr << "Failed to read AbsolutePath from registry." << std::endl;
            RegCloseKey(hKey);
            return -1;
        }

        // Read the relative path
        bufferSize = MAX_PATH; // Reset buffer size
        if (RegQueryValueExA(hKey, relPathValue, NULL, NULL, (LPBYTE)relPathBuffer, &bufferSize) == ERROR_SUCCESS)
        {
            relativePath = std::string(relPathBuffer);
            spdlog::info("Relative Path: {}", relativePath);
            std::cerr << "Relative Path: " << relativePath << std::endl;
            spdlog::info("============================================");
            std::cerr << "============================================" << std::endl;
        }
        else
        {
            spdlog::critical("Failed to read RelativePath from registry.");
            std::cerr << "Failed to read RelativePath from registry." << std::endl;

            RegCloseKey(hKey);
            return -1;
        }

        // Close the registry key
        RegCloseKey(hKey);
    }
    else
    {
        spdlog::critical("Failed to open registry key.");
        std::cerr << "Failed to open registry key." << std::endl;

        return -1;
    }
#endif

    // Define the required directory structure
    std::vector<std::string> requiredDirectories = {"assets",
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

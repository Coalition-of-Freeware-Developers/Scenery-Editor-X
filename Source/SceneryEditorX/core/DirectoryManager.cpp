#include "DirectoryManager.hpp"
#include <spdlog.h>

namespace fs = std::filesystem;

/*
DirectoryInit::DirectoryInit()
{
    spdlog::info("Program folder directory checking");
}

DirectoryInit::~DirectoryInit()
{
    spdlog::info("Program folder directory checking complete");
}
*/

#include <filesystem>

// Define the static members
std::string DirectoryInit::absolutePath = "";
std::string DirectoryInit::relativePath = "";

void DirectoryInit::ensureDirectoriesExist(const std::vector<std::string> &directories)
{
    // Get the executable's directory
    fs::path exeDir = fs::path(absolutePath).parent_path();

    for (const auto &dir : directories)
    {
        // Construct the full path for each directory
        fs::path fullPath = exeDir / dir;

        // Check if the directory exists, if not create it
        if (!fs::exists(fullPath))
        {
            fs::create_directories(fullPath);
            spdlog::info("Created directory: {}", fullPath.string());
        }
        else
        {
            spdlog::info("Directory already exists: {}", fullPath.string());
        }
    }
}

int DirectoryInit::DirectoryCheck(int argc, char *argv[])
{
    // TODO: Add a debug mode and a release mode for the directory checking
    if (argc > 0) // Used to be (argc > 1) but in debug mode it will not find the executable and fail)
    {
        // Set the absolute path to the executable's path
        //char buffer[MAX_PATH];
        //GetModuleFileNameA(NULL, buffer, MAX_PATH);
        //size_t DirEndPos = std::string(buffer).find_last_of("\\/");
        //absolutePath = std::string(buffer).substr(0, DirEndPos);
        absolutePath = fs::absolute(fs::path(argv[0])).string();
        spdlog::info("============================================");
        spdlog::info("Absolute Path: {}", absolutePath);

        // Set the relative path to the current working directory relative to the executable
        relativePath = fs::relative(fs::current_path(), absolutePath).string();
        spdlog::info("Relative Path: {}", relativePath);
        spdlog::info("============================================");
    }
    else
    {
        spdlog::critical("No executable path found.");
        return -1;
    }

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
                                                    "resources",
                                                    "resources/cache",
                                                    "resources/cache/shaders",
                                                    "resources/cache/thumbnail"};

    // Ensure that the required directories exist
    ensureDirectoriesExist(requiredDirectories);

    return 0;
}

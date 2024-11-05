#include "DirectoryManager.hpp"
#include <boost/filesystem.hpp>s
#include <spdlog.h>

namespace fs = boost::filesystem;

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

void DirectoryInit::ensureDirectoriesExist(const std::vector<std::string> &directories)
{
    // Get the executable's directory
    fs::path exeDir = fs::path(*absolutePath).parent_path();

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
    if (argc > 1)
    {
        // Set the absolute path pointer to the executable's path
        absolutePath = std::make_unique<std::string>(fs::absolute(fs::path(argv[0])).string());
        //spdlog::info("Absolute Path: {}", *absolutePath);

        // Set the relative path pointer to the current working directory relative to the executable
        relativePath =
            std::make_unique<std::string>(fs::relative(fs::current_path(), fs::path(argv[0]).parent_path()).string());
        //spdlog::info("Relative Path: {}", *relativePath);
    }
    else
    {
        spdlog::error("No executable path provided.");
        return -1;
    }

    // Define the required directory structure
    std::vector<std::string> requiredDirectories = {"assets",
                                                    "assets/shaders",
                                                    "assets/models",
                                                    "assets/textures",
                                                    "config",
                                                    "export",
                                                    "librarys",
                                                    "librarys/custom",
                                                    "logs",
                                                    "plugins",
                                                    "resources",
                                                    "resources/cache",
                                                    "resources/cache/thumbnail"};

    // Ensure that the required directories exist
    ensureDirectoriesExist(requiredDirectories);

    return 0;
}

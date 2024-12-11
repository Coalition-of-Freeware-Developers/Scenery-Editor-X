#pragma once

#include <filesystem>
#include <string>
#include <vector>

class DirectoryInit
{
public:
    static std::string absolutePath; // The absolute path to the executable
    static std::string relativePath; // The relative path to the executable

    /**
    * Checks the directories based on the provided command-line arguments.
    *
    * @param argc The number of command-line arguments.
    * @param argv An array of command-line argument strings.
    * @return An integer indicating the result of the directory check.
    */
    int DirectoryCheck(int argc, char *argv[]);

    /**
    * Ensures that all directories in the provided list exist.
    * If a directory does not exist, it will be created.
    *
    * @param directories A vector of directory paths to check and create if necessary.
    */
    void ensureDirectoriesExist(const std::vector<std::string> &directories);
};

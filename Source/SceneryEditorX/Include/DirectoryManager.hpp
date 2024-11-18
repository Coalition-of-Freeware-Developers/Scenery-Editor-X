#pragma once

#include <boost/filesystem.hpp>

#include <filesystem>
#include <string>
#include <vector>

class DirectoryInit
{
public:
    static std::string absolutePath;
    static std::string relativePath;

    int DirectoryCheck(int argc, char *argv[]);

    /**
    * Ensures that all directories in the provided list exist.
    * If a directory does not exist, it will be created.
    *
    * @param directories A vector of directory paths to check and create if necessary.
    */
    void ensureDirectoriesExist(const std::vector<std::string> &directories);
};


#pragma once

#include <boost/filesystem.hpp>

#include <filesystem>
#include <string>
#include <vector>

class DirectoryInit
{
public:
    std::string absolutePath;
    std::string relativePath;

   // DirectoryInit();
   // ~DirectoryInit();

    int DirectoryCheck(int argc, char *argv[]);

    /**
    * Ensures that all directories in the provided list exist.
    * If a directory does not exist, it will be created.
    *
    * @param directories A vector of directory paths to check and create if necessary.
    */
    void ensureDirectoriesExist(const std::vector<std::string> &directories);

    //std::string absolutePath;// Pointer for the absolute path

private:
    std::string absolutePath; // Absolute path
    std::string relativePath; // Relative path
};


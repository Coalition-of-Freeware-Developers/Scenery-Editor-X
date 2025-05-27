/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* file_manager.cpp
* -------------------------------------------------------
* Created: 27/5/2025
* -------------------------------------------------------
*/

#include <SceneryEditorX/utils/file_manager.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	std::filesystem::path FileSystem::GetWorkingDir() { return std::filesystem::current_path(); }

    void FileSystem::SetWorkingDir(std::filesystem::path path)
    {
    }

    bool FileSystem::CreateDir(const std::filesystem::path &directory)
    {
        return false;
    }

    bool FileSystem::CreateDir(const std::string &directory)
    {
        return false;
    }

    bool FileSystem::DirExists(const std::filesystem::path &directory)
    {
        return false;
    }

    bool FileSystem::DirExists(const std::string &directory)
    {
        return false;
    }


}

/// -------------------------------------------------------

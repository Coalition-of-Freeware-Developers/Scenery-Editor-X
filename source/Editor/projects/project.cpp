/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* project.cpp
* -------------------------------------------------------
* Created: 16/10/2025
* -------------------------------------------------------
*/
#include "project.h"

#include <fstream>

namespace Project
{
	
	DefaultProject::DefaultProject()
	{
	}

	DefaultProject::~DefaultProject()
	{
	}

    /*
    std::filesystem::path DefaultProject::GetAssetDirectory()
    {
    }

    std::filesystem::path DefaultProject::GetAssetRegistryPath()
    {
    }
    */

    void DefaultProject::ReadProjCache()
    {
        if (std::ifstream cacheFile("data.sedXcache", std::ios::binary); cacheFile.is_open())
        {
            cacheFile.read(reinterpret_cast<char *>(&cacheData), sizeof(cacheData));
            cacheFile.close();
        }
        if (!std::filesystem::exists(cacheData.projectPath) || !std::filesystem::exists(cacheData.binPath))
        {
            SEDX_CORE_ERROR("Cache file is corrupted. Resetting to default.");
            cacheData = {};
            WriteProjCache();
        }
    }

    void DefaultProject::WriteProjCache()
    {
        if (std::ofstream cacheFile("data.sedXcache", std::ios::binary); cacheFile.is_open())
        {
            cacheFile.write(reinterpret_cast<char *>(&cacheData), sizeof(cacheData));
            cacheFile.close();
        }
    }


} // namespace Project

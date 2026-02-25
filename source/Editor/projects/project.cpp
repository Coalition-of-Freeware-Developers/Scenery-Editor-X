/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray 
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * project.cpp
 * -------------------------------------------------------
 * Created: 16/10/2025
 * -------------------------------------------------------
 */
#include "project.h"
#include <fstream>

// -------------------------------------------------------------------------
	
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

// -------------------------------------------------------------------------

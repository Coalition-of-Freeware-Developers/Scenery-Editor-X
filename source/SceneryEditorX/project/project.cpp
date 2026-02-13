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
 * Created: 7/5/2025
 * -------------------------------------------------------
 */
#include "project.h"

// -------------------------------------------------------

namespace SceneryEditorX
{
	Project::Project()
	{
	    // TODO: Initialize the project
	}
	Project::~Project()
	{
	    //TODO: Cleanup the project
	}

    void Project::SetActive(Ref<Project> &project)
    {
        if (activeProject)
		{
            // TODO: Add code to handle the previous active project if needed
		}

		activeProject = project;
        if (activeProject)
        {
            //s_AssetManager = CreateRef<EditorAssetManager>();
        }
    }

    /*
    std::filesystem::path Project::GetAssetRegistryPath()
    {
        if (activeProject)
        {
            return activeProject->GetAssetDirectory() / "AssetRegistry.json";
        }
        return {};
    }
    */

    /*
    std::filesystem::path Project::GetActiveAssetDirectory()
    {
        if (activeProject)
            return activeProject->GetAssetDirectory();

        return {};
    }
    */

    /*
    std::filesystem::path Project::GetAssetDirectory()
    {
        // TODO: Implement proper asset directory retrieval from config
        return config.projectPath / "assets";
    }
    */

    void Project::CreateProject(std::string name, std::filesystem::path path)
    {
        // TODO: Implement project creation
    }

    void Project::Load(const std::filesystem::path &InPath)
    {
        // TODO: Implement project loading
    }

    void Project::Save(const std::filesystem::path &InPath)
    {
        // TODO: Implement project saving
    }

}

// -------------------------------------------------------

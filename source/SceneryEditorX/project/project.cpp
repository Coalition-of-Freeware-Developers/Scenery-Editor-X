/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* project.cpp
* -------------------------------------------------------
* Created: 7/5/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/project/project.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	Project::Project()
	{
	    ///< TODO: Initialize the project
	}
	Project::~Project()
	{
	    ///< TODO: Cleanup the project
	}

    void Project::SetActive(Ref<Project> &project)
    {
        if (activeProject)
		{
            ///< TODO: Add code to handle the previous active project if needed
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

/// -------------------------------------------------------

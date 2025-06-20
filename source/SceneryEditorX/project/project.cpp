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
	    // Initialize the project
	}
	Project::~Project()
	{
	    // Cleanup the project
	}

    void Project::SetActive(Ref<Project> project)
    {
        if (activeProject)
		{
            //TODO: Add code to handle the previous active project if needed
		}

		activeProject = project;
        if (activeProject)
        {
            
        }

    }

    void Project::CreateProject(std::string name, std::filesystem::path path)
    {
    }

    void Project::Load(const std::filesystem::path &InPath)
    {
    }

    void Project::Save(const std::filesystem::path &InPath)
    {
    }

} // namespace SceneryEditorX

// -------------------------------------------------------

/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* project.cpp
* -------------------------------------------------------
* Created: 26/5/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/project/project.h>
#include <SceneryEditorX/scene/asset_manager.h>

/// ---------------------------------------------------------

namespace SceneryEditorX
{
    Project::Project()
	{
    }

    Project::~Project() = default;

	void Project::SetActive(const Ref<Project> &project)
    {
        if (currentProject)
        {

        }

		currentProject = project;
		if (currentProject)
		{
            //assetMgr = Ref<AssetManager>::Create(currentProject->GetPath() + "/assets/");
        }
    }

}

/// ---------------------------------------------------------

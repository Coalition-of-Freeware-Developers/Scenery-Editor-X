/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* project.cpp
* -------------------------------------------------------
* Created: 4/4/2025
* -------------------------------------------------------
*/

#include <SceneryEditorX/project/project.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
 	Project::Project()
 	{
 	}

 	Project::~Project()
	{
	}

	void Project::SetActive(Ref<Project> project)
	{
		if (ActiveProject_)
		{
			//AssetManager_->Shutdown();
			//AssetManager_ = nullptr;
		}

		ActiveProject_ = project;
		if (ActiveProject_)
		{
			//AssetManager_ = Ref<EditorAssetManager>::Create();
		}
	}

	void Project::SetActiveRuntime(Ref<Project> project)
	{
		if (ActiveProject_)
		{
			//AssetManager_ = nullptr;
		}

		ActiveProject_ = project;
		if (ActiveProject_)
		{
			//AssetManager_ = Ref<RuntimeAssetManager>::Create();
			//Project::GetRuntimeAssetManager()->SetAssetPack(assetPack);

		}
	}

	void Project::OnSerialized()
    {
    }

    void Project::OnDeserialized()
    {
    }

} // namespace SceneryEditorX

// -------------------------------------------------------


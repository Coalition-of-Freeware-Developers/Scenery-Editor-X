/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* project.h
* -------------------------------------------------------
* Created: 7/5/2025
* -------------------------------------------------------
*/
#pragma once
#include <filesystem>
//#include "SceneryEditorX/asset/managers/asset_manager.h"
//#include "SceneryEditorX/asset/managers/editor_asset_manager.h"
#include "project_settings.h"
#include "SceneryEditorX/logging/asserts.h"
#include "SceneryEditorX/platform/config/editor_config.hpp"

/// -------------------------------------------------------

namespace SceneryEditorX
{
	class Project : public RefCounted
	{
	public:
	    Project();
	    ~Project();

	    /// -------------------------------------------------------

		const ProjectConfig &GetConfig() const { return config; }

        GLOBAL std::filesystem::path GetAssetRegistryPath()
		{
            SEDX_CORE_ASSERT(activeProject);
            return std::filesystem::path(activeProject->GetConfig().projectPath) /
                   activeProject->GetConfig().assetRegistry;
		}

        GLOBAL std::filesystem::path GetActiveAssetDirectory()
		{
            SEDX_CORE_ASSERT(activeProject);
            return activeProject->GetAssetDirectory();
		}

        GLOBAL Ref<Project> GetActive() { return activeProject; }
        GLOBAL void SetActive(Ref<Project> &project);

        void CreateProject(std::string name, std::filesystem::path path);
	    void Load(const std::filesystem::path &InPath);
	    void Save(const std::filesystem::path &InPath);

        //GLOBAL Ref<AssetManager> GetAssetManager() { return s_AssetManager; }
        //GLOBAL Ref<EditorAssetManager> GetEditorAssetManager() { return s_AssetManager.As<EditorAssetManager>(); }

        std::filesystem::path GetAssetDirectory();

	    /// -------------------------------------------------------

		GLOBAL const std::string &GetProjectName()
        {
            SEDX_CORE_ASSERT(activeProject);
            return activeProject->GetConfig().name;
        }

		GLOBAL std::filesystem::path GetProjectDirectory()
		{
            SEDX_CORE_ASSERT(activeProject);
            return activeProject->GetConfig().projectPath;
		}

	    GLOBAL std::filesystem::path GetCacheDirectory()
		{
            SEDX_CORE_ASSERT(activeProject);
            return std::filesystem::path(activeProject->GetConfig().projectPath) / "Cache";
		}

	private:
        ProjectConfig config;
        std::string projectName;
        std::filesystem::path projectPath;
        std::filesystem::path binPath;

	    //LOCAL Ref<AssetManager> s_AssetManager;
        inline LOCAL Ref<Project> activeProject;

	};

    /// -------------------------------------------------------

}

/// -------------------------------------------------------

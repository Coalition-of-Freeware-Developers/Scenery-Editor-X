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

        static std::filesystem::path GetAssetRegistryPath()
		{
            SEDX_CORE_ASSERT(activeProject);
            return std::filesystem::path(activeProject->GetConfig().projectPath) /
                   activeProject->GetConfig().assetRegistry;
		}

        static std::filesystem::path GetActiveAssetDirectory()
		{
            SEDX_CORE_ASSERT(activeProject);
            return activeProject->GetAssetDirectory();
		}

        static Ref<Project> GetActive() { return activeProject; }
        static void SetActive(Ref<Project> &project);

        void CreateProject(std::string name, std::filesystem::path path);
	    void Load(const std::filesystem::path &InPath);
	    void Save(const std::filesystem::path &InPath);

        //static Ref<AssetManager> GetAssetManager() { return s_AssetManager; }
        //static Ref<EditorAssetManager> GetEditorAssetManager() { return s_AssetManager.As<EditorAssetManager>(); }

        std::filesystem::path GetAssetDirectory();

	    /// -------------------------------------------------------

		static const std::string &GetProjectName()
        {
            SEDX_CORE_ASSERT(activeProject);
            return activeProject->GetConfig().name;
        }

		static std::filesystem::path GetProjectDirectory()
		{
            SEDX_CORE_ASSERT(activeProject);
            return activeProject->GetConfig().projectPath;
		}

	    static std::filesystem::path GetCacheDirectory()
		{
            SEDX_CORE_ASSERT(activeProject);
            return std::filesystem::path(activeProject->GetConfig().projectPath) / "Cache";
		}

	private:
        ProjectConfig config;
        std::string projectName;
        std::filesystem::path projectPath;
        std::filesystem::path binPath;

	    //static Ref<AssetManager> s_AssetManager;
        inline static Ref<Project> activeProject;

	};

    /// -------------------------------------------------------

}

/// -------------------------------------------------------

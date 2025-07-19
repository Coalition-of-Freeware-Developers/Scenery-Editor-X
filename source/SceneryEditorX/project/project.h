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
#include <SceneryEditorX/asset/asset_manager.h>
#include <SceneryEditorX/asset/editor_asset_manager.h>
#include <SceneryEditorX/logging/asserts.h>
#include <SceneryEditorX/platform/editor_config.hpp>
#include <SceneryEditorX/project/project_settings.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	class Project : public RefCounted
	{
	public:
	    Project();
        virtual ~Project() override;

	    /// -------------------------------------------------------

		const ProjectConfig &GetConfig() const { return config; }
        int GetActiveAssetDirectory();

        GLOBAL Ref<Project> GetActive() { return activeProject; }
        GLOBAL void SetActive(const Ref<Project> &project);

        void CreateProject(std::string name, std::filesystem::path path);
	    void Load(const std::filesystem::path &InPath);
	    void Save(const std::filesystem::path &InPath);

        GLOBAL Ref<AssetManager> GetAssetManager() { return s_AssetManager; }
        GLOBAL Ref<EditorAssetManager> GetEditorAssetManager() { return s_AssetManager.As<EditorAssetManager>(); }

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

	private:
        ProjectConfig config;
        std::string projectName;
        std::filesystem::path projectPath;
        std::filesystem::path binPath;

	    static Ref<AssetManager> s_AssetManager;
        inline LOCAL Ref<Project> activeProject;

	};

    /// -------------------------------------------------------

}

/// -------------------------------------------------------

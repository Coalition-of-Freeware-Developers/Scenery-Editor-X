/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* project.h
* -------------------------------------------------------
* Created: 26/5/2025
* -------------------------------------------------------
*/
#pragma once
#include <filesystem>
#include <format>
#include <SceneryEditorX/logging/asserts.h>
#include <SceneryEditorX/scene/asset_manager.h>
#include <SceneryEditorX/core/pointers.h>

/// ---------------------------------------------------------

namespace SceneryEditorX
{
	struct ProjectConfig
	{
        std::string projectName;
        std::string assetDir = "assets";

        std::string startScene;

		std::string projectFileName;
        std::string projectDir;

        bool EnableAutoSave = false;
        int AutoSaveTimer = 300;
	};

    class Project : public RefCounted
    {
    public:
		Project();
		~Project();

        [[nodiscard]] const ProjectConfig &GetConfig() const { return config; }

		GLOBAL Ref<Project> GetActive() { return currentProject; }
        GLOBAL void SetActive(const Ref<Project> &project);

		void Load(const std::filesystem::path& path);
		void Save(const std::filesystem::path& path) const;

		GLOBAL const std::string &GetProjectName()
        {
            SEDX_CORE_ASSERT(currentProject);
            return currentProject->GetConfig().projectName;
        }

		GLOBAL const std::string &GetProjectDir()
		{
            SEDX_ASSERT(currentProject);
            return currentProject->GetConfig().projectDir;
		}

		GLOBAL const std::string &GetAssetDir()
		{
			SEDX_CORE_ASSERT(currentProject);
			return currentProject->GetConfig().assetDir;
        }

		GLOBAL std::filesystem::path GetXPLibraryPath()
		{
            SEDX_CORE_ASSERT(currentProject);
            return std::filesystem::path(currentProject->GetConfig().projectDir) / "library.txt";
		}

		GLOBAL std::filesystem::path GetCacheDir()
        {
			SEDX_CORE_ASSERT(currentProject);
            return std::filesystem::path(currentProject->GetConfig().projectDir) / "cache";
        }
    private:
        ProjectConfig config;
		inline GLOBAL Ref<Project> currentProject;
        //inline GLOBAL Ref<AssetManager> assetManager;
    };
}

/// ---------------------------------------------------------

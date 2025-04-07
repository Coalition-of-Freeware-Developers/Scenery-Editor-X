/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* project.h
* -------------------------------------------------------
* Created: 4/4/2025
* -------------------------------------------------------
*/
#pragma once

#include <filesystem>
#include <format>
#include <SceneryEditorX/core/settings/editor_config.hpp>
#include <SceneryEditorX/core/templates/ref_templates.h>
#include <SceneryEditorX/logging/asserts.h>

// ---------------------------------------------------------

namespace SceneryEditorX
{
	struct ProjectConfig;

	class Project : public RefCounted
    {
    public:
        Project();
        ~Project();

		const ProjectConfig& GetConfig() const { return Config_; }

		static Ref<Project> GetActive() { return ActiveProject_; }
		static void SetActive(Ref<Project> project);
        static void SetActiveRuntime(Ref<Project> project);

        //static void SetActiveRuntime(Ref<Project> project, Ref<AssetPack> assetPack);

		//inline static Ref<AssetManagerBase> GetAssetManager() { return AssetManager_; }
		//inline static Ref<EditorAssetManager> GetEditorAssetManager() { return AssetManager_.As<EditorAssetManager>(); }
		//inline static Ref<LibraryManager> GetLibraryManager() { return AssetManager_.As<LibraryManager>(); }
		//inline static Ref<RuntimeAssetManager> GetRuntimeAssetManager() { return AssetManager_.As<RuntimeAssetManager>(); }

		static const std::string &GetProjectName()
        {
            //SEDX_CORE_ASSERT(ActiveProject_);
            return ActiveProject_->GetConfig().Name;
        }

        static std::filesystem::path GetProjectDirectory()
        {
            //SEDX_CORE_ASSERT(ActiveProject_);
            return ActiveProject_->GetConfig().ProjectDirectory;
        }

        static std::filesystem::path GetCacheDirectory()
        {
            //SEDX_CORE_ASSERT(ActiveProject_);
            return std::filesystem::path(ActiveProject_->GetConfig().ProjectDirectory) / "Cache";
        }

        std::filesystem::path GetAssetDirectory() const
        {
            return std::filesystem::path(GetConfig().ProjectDirectory) / GetConfig().AssetDirectory;
        }

		void OnSerialized();
        void OnDeserialized();


	private:
        ProjectConfig Config_;

		friend class ProjectSettingsWindow;
        friend class ProjectSerializer;

        //inline static Ref<AssetManagerBase> AssetManager_;
        inline static Ref<Project> ActiveProject_;
    };



} // namespace SceneryEditorX

// -------------------------------------------------------

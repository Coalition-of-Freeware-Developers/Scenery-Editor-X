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
#include <SceneryEditorX/logging/asserts.h>
#include <SceneryEditorX/platform/editor_config.hpp>
#include "project_settings.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{
	class Project : public RefCounted
	{
	public:
	    Project();
        virtual ~Project() override;

	    /// -------------------------------------------------------

		const ProjectSettings &GetConfig() const { return config; }

	    GLOBAL Ref<Project> GetActive() { return activeProject; }
        GLOBAL void SetActive(Ref<Project> project);

        void CreateProject(std::string name, std::filesystem::path path);
	    void Load(const std::filesystem::path &InPath);
	    void Save(const std::filesystem::path &InPath);

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
        ProjectSettings config;
        std::string projectName;
        std::filesystem::path projectPath;
        std::filesystem::path binPath;

        inline static Ref<Project> activeProject;

        /// -------------------------------------------------------

	};

} // namespace SceneryEditorX

/// -------------------------------------------------------

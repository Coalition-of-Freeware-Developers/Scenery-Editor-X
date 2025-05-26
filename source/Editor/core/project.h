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
#include <SceneryEditorX/platform/editor_config.hpp>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class Project
	{
	public:
	    Project();
	    ~Project();

	    // -------------------------------------------------------

        void CreateProject(std::string name, std::filesystem::path path);
	    void Load(const std::filesystem::path &InPath);
	    void Save(const std::filesystem::path &InPath);

	    // -------------------------------------------------------

	private:

        std::filesystem::path projectPath;
        std::filesystem::path binPath;
        std::string projectName;

        // -------------------------------------------------------
	};

} // namespace SceneryEditorX

// -------------------------------------------------------

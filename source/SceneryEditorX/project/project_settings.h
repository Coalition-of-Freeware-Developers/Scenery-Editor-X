/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* project_settings.h
* -------------------------------------------------------
* Created: 15/6/2025
* -------------------------------------------------------
*/
#pragma once

/// -------------------------------------------------------

namespace SceneryEditorX
{
    struct ProjectSettings
	{
        std::string name;
		std::string projectName;
		std::string projectPath;
		std::string sceneryType;
        std::string sceneryVersion;
        std::string assetDir = "Assets";

		bool enableAutosave = true;
		bool enableLighting = true;
		bool enableWeatherEffects = false;
		int autosaveInterval = 5; /// in minutes
    };

}

/// -------------------------------------------------------

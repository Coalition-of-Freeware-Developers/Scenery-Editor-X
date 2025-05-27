/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* project_settings.h
* -------------------------------------------------------
* Created: 26/5/2025
* -------------------------------------------------------
*/
#pragma once
#include <map>

/// ---------------------------------------------------------

namespace SceneryEditorX
{
	struct RecentProjects
	{
        std::string projectName;
        std::string projectPath;
        time_t LastOpened;
	};

	

}

/// ---------------------------------------------------------

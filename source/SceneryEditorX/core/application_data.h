/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* application_data.h
* -------------------------------------------------------
* Created: 23/4/2025
* -------------------------------------------------------
*/
#pragma once

// -------------------------------------------------------

namespace SceneryEditorX
{
	
	struct SoftwareStats
	{
	    GLOBAL inline uint32_t minVulkanVersion = VK_API_VERSION_1_3;
	    GLOBAL inline uint32_t maxVulkanVersion = VK_API_VERSION_1_4;
	    GLOBAL inline std::string appName = "Scenery Editor X";
	    GLOBAL inline std::string renderName = "X-Plane 12 Graphics Emulator";
	    GLOBAL inline std::string versionString = SEDX_VERSION_STRING;
	    GLOBAL inline uint32_t version = SEDX_VERSION;

        bool NoTitlebar = false;
	};
	
	struct XPlaneStats
	{
	    std::string xPlaneVersion = "X-Plane 12.2.0-b1-0f62ac46";
	    std::string xPlanePath;
	    std::string xPlaneBinPath;
	    std::string xPlaneResourcesPath;
	    bool isSteam = false;
	};
	
} // namespace SceneryEditorX

// -------------------------------------------------------

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
	    std::string xPlaneVersion = "X-Plane 12.06b1";
	    std::string xPlanePath = "C:/X-Plane 12";
	    std::string xPlaneBinPath = "C:/X-Plane 12/bin";
	    std::string xPlaneResourcesPath = "C:/X-Plane 12/Resources";
	    bool isSteam = false;
	};
	
} // namespace SceneryEditorX

// -------------------------------------------------------

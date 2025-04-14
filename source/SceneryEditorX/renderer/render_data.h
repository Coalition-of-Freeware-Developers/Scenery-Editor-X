/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* render_data.h
* -------------------------------------------------------
* Created: 8/4/2025
* -------------------------------------------------------
*/
#pragma once
#include <string>
#include <SceneryEditorX/resource.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

	struct SoftwareStats
	{
        uint32_t minVulkanVersion = VK_API_VERSION_1_3;
        uint32_t maxVulkanVersion = VK_API_VERSION_1_4;
        std::string appName = "Scenery Editor X";
        std::string renderName = "X-Plane 12 Graphics Emulator";
        std::string versionString = SEDX_VERSION_STRING;
        uint32_t version = SEDX_VERSION;

	};

	struct RendererCapabilities
	{
	    std::string Vendor;
	    std::string Device;
	    std::string Version;
	
	    int MaxSamples = 0;
	    int MaxTextureUnits = 0;
	    float MaxAnisotropy = 0.0f;
	};
	
	struct RenderData
	{
        RendererCapabilities capabilities;
        SoftwareStats stats;

	    uint32_t width = 0;
	    uint32_t height = 0;

        uint32_t FramesInFlight = 3;
        uint32_t imageIndex = 0;
        uint32_t frameIndex = 0;
        uint32_t swapChainCurrentFrame = 0;

	    int viewports = 0;
	    int cameras = 0;
        bool VSync = false;
	};

} // namespace SceneryEditorX

// -------------------------------------------------------

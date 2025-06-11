/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* render_data.h
* -------------------------------------------------------
* Created: 2/4/2025
* -------------------------------------------------------
*/
#pragma once
#include <string>

/// -------------------------------------------------------

namespace SceneryEditorX
{
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
        uint32_t width = 0;
        uint32_t height = 0;
        int viewports = 0;
        int cameras = 0;
        bool vsync = false;

    };

} // namespace SceneryEditorX

/// -------------------------------------------------------

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

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct RendererCapabilities
	{
	    std::string Vendor;
	    std::string Device;
	    std::string Version;
	
	    int MaxSamples = 0;
	    float MaxAnisotropy = 0.0f;
	    int MaxTextureUnits = 0;
	};

	struct RenderData
    {

    };

} // namespace SceneryEditorX

// -------------------------------------------------------

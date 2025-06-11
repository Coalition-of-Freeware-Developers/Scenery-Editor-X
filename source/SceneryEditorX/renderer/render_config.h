/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* renderer_config.h
* -------------------------------------------------------
* Created: 26/5/2025
* -------------------------------------------------------
*/
#pragma once
#include <string>

/// --------------------------------------------------------------------------

namespace SceneryEditorX
{
	struct RendererConfig
	{
        uint32_t framesInFlight = 3; ///< Number of frames in flight for rendering
        std::string shaderPath;      ///< Path to the shaders directory
    };

}

/// --------------------------------------------------------------------------

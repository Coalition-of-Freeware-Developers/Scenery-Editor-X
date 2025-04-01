/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* EditorConfig.hpp
* -------------------------------------------------------
* Created: 5/2/2025
* -------------------------------------------------------
*/

#pragma once

#include <SceneryEditorX/platform/system_detection.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

#ifdef SEDX_PLATFORM_WINDOWS

	struct EditorConfig
	{
        std::string shaderFolder	=	"../../assets/shaders";
        std::string textureFolder	=	"../../assets/textures";
        std::string modelFolder		=	"../../assets/models";
        std::string fontFolder		=	"../../assets/fonts";
		
	};

#endif //SEDX_PLATFORM_WINDOWS

#ifdef SEDX_PLATFORM_LINUX

	struct EditorConfig
    {
        std::string shaderFolder	=	"../../assets/shaders";
        std::string textureFolder	=	"../../assets/textures";
        std::string modelFolder		=	"../../assets/models";
        std::string fontFolder		=	"../../assets/fonts";
    };

#endif //SEDX_PLATFORM_LINUX


} // namespace SceneryEditorX

// -------------------------------------------------------

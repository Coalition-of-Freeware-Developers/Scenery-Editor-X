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
	// -------------------------------------------------------

	#ifdef SEDX_PLATFORM_WINDOWS
	
		struct EditorConfig
		{
	        std::string shaderFolder	=	"../../assets/shaders";
	        std::string textureFolder	=	"../../assets/textures";
	        std::string modelFolder		=	"../../assets/models";
	        std::string fontFolder		=	"../../assets/fonts";
            const char *defaultFontPath =	"C:\\Windows\\Fonts\\arial.ttf";
		};

	#elif SEDX_PLATFORM_LINUX
	
		struct EditorConfig
	    {
	        std::string shaderFolder	=	"../../assets/shaders";
	        std::string textureFolder	=	"../../assets/textures";
	        std::string modelFolder		=	"../../assets/models";
	        std::string fontFolder		=	"../../assets/fonts";
            const char *defaultFontPath =	"/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf";
	    };

    #elif SEDX_PLATFORM_MACOS

        struct EditorConfig
        {
            std::string shaderFolder = "../../assets/shaders";
            std::string textureFolder = "../../assets/textures";
            std::string modelFolder = "../../assets/models";
            std::string fontFolder = "../../assets/fonts";
            const char *defaultFontPath = "/Library/Fonts/Arial Unicode.ttf";
        };
    #else
        #error "Unsupported platform!"
    #endif

    // -------------------------------------------------------

} // namespace SceneryEditorX

// -------------------------------------------------------



<<<<<<<< Updated upstream:source/SceneryEditorX/core/settings/windows/editor_config.hpp
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


========
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
#include <string>
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
		std::string projectPath		=	"../../assets/projects";
	};

	struct ProjectConfig
    {
        struct EditorConfig;

        std::string Name;
        std::string AssetDirectory = "Assets";
        std::string DefaultNamespace;
        std::string StartScene;
        std::string ProjectFileName;  // Not serialized
        std::string ProjectDirectory; // Not serialized

		bool AutomaticallyReloadAssembly;
        bool EnableAutoSave = false;

		int AutoSaveIntervalSeconds = 300;

        //AssetHandle StartSceneHandle; // Runtime only
	};


#endif //SEDX_PLATFORM_WINDOWS

#ifdef SEDX_PLATFORM_LINUX

	struct EditorConfig
    {
        std::string shaderFolder	=	"../../assets/shaders";
        std::string textureFolder	=	"../../assets/textures";
        std::string modelFolder		=	"../../assets/models";
        std::string fontFolder		=	"../../assets/fonts";
        std::string projectPath     = "../../assets/projects";
    };

#endif //SEDX_PLATFORM_LINUX

#ifdef SEDX_PLATFORM_MACOS
    struct EditorConfig
    {
        std::string shaderFolder	= "../../assets/shaders";
        std::string textureFolder	= "../../assets/textures";
        std::string modelFolder		= "../../assets/models";
        std::string fontFolder		= "../../assets/fonts";
        std::string projectPath     = "../../assets/projects";
    };
#endif //SEDX_PLATFORM_MACOS

} // namespace SceneryEditorX


>>>>>>>> Stashed changes:source/SceneryEditorX/core/settings/editor_config.hpp

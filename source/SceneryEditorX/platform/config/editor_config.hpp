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
#include <SceneryEditorX/utils/system_detection.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

    /// -------------------------------------------------------

    #ifdef SEDX_PLATFORM_WINDOWS
    
        /**
         * @struct EditorConfig
         *
         * @brief Configuration settings for the Scenery Editor X on Windows platform
         * 
         * Contains paths to essential resources like shaders, textures, models, and fonts.
         * Provides default locations appropriate for the Windows environment.
         * 
         */
        struct EditorConfig : RefCounted
        {
            std::string shaderFolder    = "../../assets/shaders";          ///< Directory containing shader files
            std::string textureFolder   = "../../assets/textures";         ///< Directory containing texture files
            std::string modelFolder     = "../../assets/models";           ///< Directory containing 3D model files
            std::string fontFolder      = "../../assets/fonts";            ///< Directory containing font files
            const char *defaultFontPath = R"(C:\Windows\Fonts\arial.ttf)"; ///< System default font path for Windows
        };

    #elif SEDX_PLATFORM_LINUX
    
        /**
         * @struct EditorConfig
         * @brief Configuration settings for the Scenery Editor X on Linux platform
         * 
         * Contains paths to essential resources like shaders, textures, models, and fonts.
         * Provides default locations appropriate for the Linux environment.
         */
        struct EditorConfig : RefCounted
        {
            std::string shaderFolder    = "../../assets/shaders";   ///< Directory containing shader files
            std::string textureFolder   = "../../assets/textures";  ///< Directory containing texture files
            std::string modelFolder     = "../../assets/models";    ///< Directory containing 3D model files
            std::string fontFolder      = "../../assets/fonts";     ///< Directory containing font files
            const char *defaultFontPath = "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf"; ///< System default font path for Linux
        };

    #elif SEDX_PLATFORM_MACOS

        /**
         * @struct EditorConfig
         * @brief Configuration settings for the Scenery Editor X on macOS platform
         * 
         * Contains paths to essential resources like shaders, textures, models, and fonts.
         * Provides default locations appropriate for the macOS environment.
         */
        struct EditorConfig : RefCounted
        {
            std::string shaderFolder    = "../../assets/shaders";   ///< Directory containing shader files
            std::string textureFolder   = "../../assets/textures";  ///< Directory containing texture files
            std::string modelFolder     = "../../assets/models";    ///< Directory containing 3D model files
            std::string fontFolder      = "../../assets/fonts";     ///< Directory containing font files
            const char *defaultFontPath = "/Library/Fonts/Arial Unicode.ttf"; ///< System default font path for macOS
        };

    #else
        #error "Unsupported platform!"
    #endif

    /// -------------------------------------------------------

}

/// -------------------------------------------------------



/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray 
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * EditorConfig.hpp
 * -------------------------------------------------------
 * Created: 5/2/2025
 * -------------------------------------------------------
 */
#pragma once
#include <SceneryEditorX/utils/system_detection.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

    // -------------------------------------------------------

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
            std::string shaderFolder    = "../../resources/shaders";          ///< Directory containing shader files
            std::string textureFolder   = "../../resources/textures";         ///< Directory containing texture files
            std::string modelFolder     = "../../resources/models";           ///< Directory containing 3D model files
            std::string fontFolder      = "../../resources/fonts";            ///< Directory containing font files
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
            std::string shaderFolder    = "../../resources/shaders";   ///< Directory containing shader files
            std::string textureFolder   = "../../resources/textures";  ///< Directory containing texture files
            std::string modelFolder     = "../../resources/models";    ///< Directory containing 3D model files
            std::string fontFolder      = "../../resources/fonts";     ///< Directory containing font files
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
            std::string shaderFolder    = "../../resources/shaders";   ///< Directory containing shader files
            std::string textureFolder   = "../../resources/textures";  ///< Directory containing texture files
            std::string modelFolder     = "../../resources/models";    ///< Directory containing 3D model files
            std::string fontFolder      = "../../resources/fonts";     ///< Directory containing font files
            const char *defaultFontPath = "/Library/Fonts/Arial Unicode.ttf"; ///< System default font path for macOS
        };

    #else
        #error "Unsupported platform!"
    #endif

    // -------------------------------------------------------

}

// -------------------------------------------------------



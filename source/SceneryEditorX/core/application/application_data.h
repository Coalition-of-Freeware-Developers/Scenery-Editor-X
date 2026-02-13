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
 * application_data.h
 * -------------------------------------------------------
 * Created: 23/4/2025
 * -------------------------------------------------------
 */
#pragma once
#include <filesystem>
#include <string>

// -------------------------------------------------------

namespace SceneryEditorX
{

	/**
	 * @struct AppData
	 * @brief Stores global application configuration and version information.
	 *
	 * Contains version requirements, application identification, and UI configuration
	 * settings used throughout the application.
	 */
	struct AppData
	{
	    /**
		 * @brief Application name displayed in window title and about dialogs.
		 */
		uint32_t WinWidth = 1280;
        uint32_t WinHeight = 720;

	    /**
		 * @brief Human-readable version string of the application.
		 * Defined by SEDX_VERSION_STRING macro
		 */
	    static inline std::string versionString = SEDX_VERSION_STRING;

	    /**
		 * @brief Numeric version identifier.
		 * Used for version comparison and compatibility checks
		 * Defined by SEDX_VERSION macro
		 */
	    static inline uint32_t version = SEDX_VERSION;

        bool Fullscreen = false;
	    bool NoTitlebar = false;
		bool VSync = false;
        bool StartMaximized = true;
        bool Resizable = true;
        bool Decorated = true;
        bool SplashScreen = false;
        std::string WorkingDirectory;
        std::filesystem::path IconPath;
	};

	/**
	 * @struct XPlaneStats
	 * @brief Stores X-Plane simulator configuration and path information.
	 *
	 * Contains version information, installation paths, and configuration
	 * data related to the X-Plane flight simulator that the editor interfaces with.
	 */
    struct XPlaneStats
    {
        /**
	     * @brief Current X-Plane version string.
	     * Identifies the version of X-Plane that the editor is configured to work with.
	     */
        std::string xPlaneVersion;

        /**
	     * @brief Root installation path of X-Plane.
	     * The main directory where X-Plane is installed.
	     */
        std::string xPlanePath;

        /**
	     * @brief Path to X-Plane executables.
	     * Directory containing the X-Plane binary files.
	     */
        std::string xPlaneBinPath;

        /**
	     * @brief Path to X-Plane resources.
	     * Directory containing X-Plane resource files needed by the editor.
	     */
        std::string xPlaneResourcesPath;

        /**
	     * @brief Flag indicating if this is a Steam installation.
	     * When true, X-Plane was installed through the Steam platform,
	     * which may affect certain file paths and integration features.
	     */
        bool isSteam = false;
    };


}

// -------------------------------------------------------

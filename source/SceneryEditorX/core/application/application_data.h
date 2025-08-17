/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* application_data.h
* -------------------------------------------------------
* Created: 23/4/2025
* -------------------------------------------------------
*/
#pragma once
#include <filesystem>
#include <string>
#include "SceneryEditorX/utils/static_states.h"

/// -------------------------------------------------------

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
	    std::string appName = "Scenery Editor X";
		uint32_t WinWidth = 1280;
        uint32_t WinHeight = 720;

	    GLOBAL inline std::string renderName = "X-Plane 12 Graphics Emulator";

	    /**
		 * @brief Human-readable version string of the application.
		 * Defined by SEDX_VERSION_STRING macro
		 */
	    GLOBAL inline std::string versionString = SEDX_VERSION_STRING;

	    /**
		 * @brief Numeric version identifier.
		 * Used for version comparison and compatibility checks
		 * Defined by SEDX_VERSION macro
		 */
	    GLOBAL inline uint32_t version = SEDX_VERSION;

        bool Fullscreen = false;
	    bool NoTitlebar = false;
		bool VSync = false;
        bool StartMaximized = true;
        bool Resizable = true;
        bool EnableImGui = true;
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

/// -------------------------------------------------------

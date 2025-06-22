/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* steam_parser.h
* -------------------------------------------------------
* Created: 13/6/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/platform/platform_states.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

	/**
	 * @class SteamGameFinder
	 * @brief Utility class for locating X-Plane 12 in Steam library folders.
	 *
	 * Provides functionality to search for X-Plane 12 installation across multiple Steam library
	 * folders on different platforms. This helps with automatic detection of X-Plane installation
	 * when the simulator has been installed through Steam rather than as a standalone application.
	 */
	class SteamGameFinder
	{
	public:
	    /**
	     * @brief Attempts to find X-Plane 12 in Steam library folders
	     * 
	     * Searches standard Steam installation directories and all configured Steam library folders
	     * for an X-Plane 12 installation. The search is performed in platform-specific locations.
	     * 
	     * @return An optional string containing the path to X-Plane 12 if found, empty otherwise
	     */
	    GLOBAL std::optional<std::string> findXPlane12();
	
	    /**
	     * @brief Saves the discovered X-Plane path to the config file
	     * 
	     * @param path The X-Plane installation path to save
	     * @param configFile Path to the configuration file
	     * @return true if the path was successfully saved to the config file, false otherwise
	     */
	    GLOBAL bool savePathToConfig(const std::string &path, const std::string &configFile);
	
	    /**
		 * @brief Validates if a path contains a valid X-Plane 12 installation
		 * 
		 * Checks for the presence of critical X-Plane 12 files and directories that
		 * indicate a valid simulator installation.
		 * 
		 * @param path Path to validate as an X-Plane 12 installation directory
		 * @return true if the path contains a valid X-Plane 12 installation, false otherwise
		 */
	    GLOBAL bool validateXPlanePath(const std::string &path);
	
	private:
	    /**
	     * @brief Gets the default Steam installation directory based on current platform
	     * 
	     * Returns the standard Steam installation location for Windows, macOS, or Linux,
	     * depending on the platform the application is running on.
	     * 
	     * @return String containing the default Steam installation directory path
	     */
	    LOCAL std::string getSteamDirectory();
	
	    /**
	     * @brief Retrieves all configured Steam library folders
	     * 
	     * Parses Steam's libraryfolders.vdf file to extract all configured Steam library locations
	     * where games might be installed.
	     * 
	     * @param steamPath Path to the main Steam installation directory
	     * @return Vector of strings containing paths to all Steam library folders
	     */
	    LOCAL std::vector<std::string> getSteamLibraryFolders(const std::string &steamPath);

	    /**
	     * @brief Platform-specific directory separator character
	     * 
	     * Holds the correct directory separator character for the current platform
	     * ('\\' for Windows, '/' for Unix-based systems).
	     */
        INTERNAL const char dirSeparator;
	};

    /// ----------------------------------------------------------

} // namespace SceneryEditorX

/// -------------------------------------------------------

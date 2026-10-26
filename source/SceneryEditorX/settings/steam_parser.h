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
 * steam_parser.h
 * -------------------------------------------------------
 * Created: 13/6/2025
 * -------------------------------------------------------
 */
#pragma once

// -------------------------------------------------------

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
		static std::optional<std::string> FindXPlane12();

		/**
		 * @brief Saves the discovered X-Plane path to the config file
		 *
		 * @param path The X-Plane installation path to save
		 * @param configFile Path to the configuration file
		 * @return true if the path was successfully saved to the config file, false otherwise
		 */
		static bool SavePathToConfig(const std::string &path, const std::string &configFile);

		/**
		 * @brief Validates if a path contains a valid X-Plane 12 installation
		 *
		 * Checks for the presence of critical X-Plane 12 files and directories that
		 * indicate a valid simulator installation.
		 *
		 * @param path Path to validate as an X-Plane 12 installation directory
		 * @return true if the path contains a valid X-Plane 12 installation, false otherwise
		 */
		static bool ValidateXPlanePath(const std::string &path);

	private:
		/**
		 * @brief Gets the default Steam installation directory based on current platform
		 *
		 * Returns the standard Steam installation location for Windows, macOS, or Linux,
		 * depending on the platform the application is running on.
		 *
		 * @return String containing the default Steam installation directory path
		 */
		static std::string GetSteamDirectory();

		/**
		 * @brief Retrieves all configured Steam library folders
		 *
		 * Parses Steam's libraryfolders.vdf file to extract all configured Steam library locations
		 * where games might be installed.
		 *
		 * @param steamPath Path to the main Steam installation directory
		 * @return Vector of strings containing paths to all Steam library folders
		 */
		static std::vector<std::string> GetSteamLibraryFolders(const std::string &steamPath);

		/**
		 * @brief Platform-specific directory separator character
		 *
		 * Holds the correct directory separator character for the current platform
		 * ('\\' for Windows, '/' for Unix-based systems).
		 */
		static const char DIR_SEPARATOR;
	};

}

// -------------------------------------------------------

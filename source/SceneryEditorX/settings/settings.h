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
 * settings.h
 * -------------------------------------------------------
 * Created: 23/4/2025
 * -------------------------------------------------------
 */
#pragma once
#include <filesystem>
#include <string>
#include <SceneryEditorX/core/application/application_data.h>
#include <SceneryEditorX/settings/config_file.h>
#include <SceneryEditorX/utils/pointers.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @class Settings
	 * @brief The application's settings (settings.cfg): the X-Plane install, application, UI and project defaults.
	 *
	 * A ConfigFile, so the file is read and written through the VFS (the same on Windows, macOS and Linux, atomic
	 * writes) and the generic option API (AddIntOption, GetStringOption, ...) works on it. Every instance has its own
	 * document: the user preferences and the editor settings are separate files with their own ConfigFile.
	 *
	 * The editor keeps one instance for its lifetime (Application::GetSettings) at "user:/settings.cfg", the roaming
	 * settings folder each platform has (%APPDATA%, ~/Library/Application Support, $XDG_CONFIG_HOME).
	 */
	class Settings : public RefCounted, public ConfigFile
	{
	public:
		/* @brief Where the editor keeps them. */
		static constexpr const char *DEFAULT_LOCATION = "user:/settings.cfg";

		/* @brief Settings with nothing read yet: Open() names the file. */
		Settings();

		/* @brief Open(location) at once. */
		explicit Settings(std::string location);

		/**
		 * @brief Reads the settings at @p location (a virtual "mount:/path" or a physical path) and makes the file
		 * complete: a missing file is created with the defaults, values a newer version added are filled in, a file
		 * that doesn't parse is kept as "<name>.invalid" before the defaults replace it, and X-Plane is searched for
		 * when the saved folder isn't an X-Plane install.
		 * @return False when the file couldn't be read or written (the defaults are in use).
		 */
		bool Open(std::string location);

		/**
		 * @brief Reads the file into the settings (Open() does this and more).
		 * @return True when the file was there and parsed.
		 */
		virtual bool ReadSettings();

		/**
		 * @brief Writes the settings (atomically) with the X-Plane and application values brought up to date.
		 * @return False (and a log line) when the file couldn't be written.
		 */
		virtual bool WriteSettings();

		// ----------------------------------------------------

		/**
		 * @brief Automatically detects X-Plane installation path.
		 * @return true if X-Plane path was successfully detected, false otherwise
		 */
		bool DetectXPlanePath();

		/**
		 * @brief Sets the X-Plane installation path manually (WriteSettings saves it).
		 * @param path Directory path to X-Plane installation (UTF-8)
		 * @return true if path was valid and successfully set, false otherwise
		 */
		bool SetXPlanePath(const std::string &path);

		/**
		 * @brief Gets the current X-Plane installation path.
		 * @return Path to X-Plane installation directory (UTF-8)
		 */
		[[nodiscard]] std::string GetXPlanePath() const;

		/**
		 * @brief Validates that all required X-Plane paths and files exist.
		 * @return true if all required X-Plane paths are valid, false otherwise
		 */
		[[nodiscard]] bool ValidateXPlanePaths() const;

		/**
		 * @brief Updates derived paths based on main X-Plane path.
		 * Updates resource paths, binary paths, etc. based on the main X-Plane path (as the disk spells them)
		 */
		void UpdateDerivedXPlanePaths();

		/**
		 * @brief The folder new projects go in by default: project.default_project_dir with a leading "~" expanded
		 * to the home folder, else <Documents>/SceneryEditorX.
		 */
		[[nodiscard]] std::filesystem::path GetDefaultProjectFolder() const;

		/**
		 * @brief Gets read-only access to X-Plane stats.
		 * @return Const reference to X-Plane statistics
		 */
		[[nodiscard]] const XPlaneStats& GetXPlaneStats() const { return m_XPlaneStats; }

		/**
		 * @brief Gets modifiable access to X-Plane stats.
		 * @return Reference to X-Plane statistics
		 */
		XPlaneStats& GetXPlaneStats() { return m_XPlaneStats; }

		/**
		 * @brief Gets read-only access to application stats.
		 * @return Const reference to application statistics
		 */
		[[nodiscard]] const AppData& GetAppStats() const { return m_AppStats; }

		/**
		 * @brief Gets modifiable access to application stats.
		 * @return Reference to application statistics
		 */
		AppData& GetAppStats() { return m_AppStats; }

		/**
		 * @brief Gets the custom buffer size to be used for allocations.
		 * @return The custom buffer size in bytes
		 */
		//[[nodiscard]] VkDeviceSize GetCustomBufferSize() const;

		/**
		 * @brief Sets the custom buffer size to be used for allocations.
		 * @param size The desired buffer size in bytes
		 * @return true if the size was set successfully, false if invalid
		 */
		//bool SetCustomBufferSize(VkDeviceSize size);

		/**
		 * @brief Validates if a given buffer size is supported by the device.
		 * @param size The buffer size to validate in bytes
		 * @param deviceLimits
		 * @param device The Vulkan device to check against
		 * @return true if the size is supported, false otherwise
		 */
		//static bool ValidateBufferSize(VkDeviceSize size, const VkPhysicalDeviceLimits& deviceLimits);
	private:
		/**
		 * @brief Adds every section and value the settings have that the file lacks (a new file, or one an older
		 * version wrote). Values that are there are kept.
		 */
		void ApplyDefaults();

		/* @brief Values older versions wrote that no longer work (the Windows-only "~\Documents" project folder). */
		void UpgradeOldValues();

		/* @brief The X-Plane and application values from the document. */
		void LoadDataFromConfig();

		/* @brief The X-Plane and application values into the document. */
		void UpdateConfigFromData();

		XPlaneStats m_XPlaneStats;		// X-Plane statistics and paths
		AppData m_AppStats;				// Application statistics
		LoadResult m_LoadResult = LoadResult::Missing;	// Failed: the file is there but unreadable, so it's never written over
	};

}

// -------------------------------------------------------

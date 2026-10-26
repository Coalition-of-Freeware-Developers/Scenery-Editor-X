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
 * user_settings.h
 * -------------------------------------------------------
 * Created: 11/7/2025
 * -------------------------------------------------------
 */
#pragma once
#include <ctime>
#include <filesystem>
#include <map>
#include <string>
#include <SceneryEditorX/project/project_settings.h>
#include <SceneryEditorX/settings/config_file.h>
#include <SceneryEditorX/utils/pointers.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	// ----------------------------------------------------
	// Date/Time Formatting Utilities
	// ----------------------------------------------------

	/**
	 * @brief Converts a time_t value to a formatted date/time string (local time) for display.
	 *
	 * Formats the time using DD/MM/YYYY HH:MM:SS format by default, or adapts to the user's
	 * operating system regional settings when useSystemFormat is true.
	 *
	 * @param time The time_t value to convert
	 * @param useSystemFormat If true, attempts to use OS regional format (default: true)
	 * @return Formatted date/time string in DD/MM/YYYY HH:MM:SS format or system format
	 */
	std::string TimeToString(time_t time, bool useSystemFormat = true);

	/**
	 * @brief A time as ISO 8601 in UTC ("2026-10-24T13:45:10Z"), which reads the same on every machine whatever its
	 * regional settings or time zone. StringToTime reads it back.
	 * @return The text, or "" for 0 / a time that can't be converted
	 */
	std::string TimeToIso8601(time_t time);

	/**
	 * @brief Converts a stored date/time string back to time_t.
	 *
	 * Reads ISO 8601 ("2026-10-24T13:45:10Z" is UTC) and the formats the preference files hold in local time
	 * (DD/MM/YYYY HH:MM:SS, DD:MM:YYYY HH:MM:SS, YYYY-MM-DD HH:MM:SS), never the regional format, so a day can never
	 * be read as a month ("10/06/2026" is 10 June, wherever the editor runs).
	 *
	 * @param timeString The formatted time string to parse
	 * @return time_t value, or 0 if parsing fails
	 */
	time_t StringToTime(const std::string& timeString);

	/**
	 * @brief Gets the system's regional date/time format pattern.
	 *
	 * Retrieves the user's operating system regional date and time format
	 * preferences. Falls back to DD/MM/YYYY HH:MM:SS if system format
	 * cannot be determined.
	 *
	 * @return System format string compatible with strftime/std::put_time
	 */
	std::string GetSystemDateTimeFormat();

	/**
	 * @brief Checks if the system uses 12-hour time format.
	 * @return true if system uses 12-hour format (AM/PM), false for 24-hour
	 */
	bool IsSystem12HourFormat();

	/**
	 * @class UserPreferences
	 * @brief Manages user-specific preferences and settings for the application.
	 *
	 * Stores user preferences such as welcome screen visibility, startup project,
	 * and recent project history in their own .cfg file (a ConfigFile, so it goes
	 * through the VFS: the same on Windows, macOS and Linux, written atomically).
	 */
	class UserPreferences : public RefCounted
	{
	public:
		/* @brief Where the editor keeps them: the roaming settings folder. */
		static constexpr const char *DEFAULT_LOCATION = "user:/user_preferences.cfg";

		/**
		 * @brief Constructs user preferences kept at DEFAULT_LOCATION (user:/ is mounted at startup).
		 */
		UserPreferences();

		/**
		 * @brief Constructs user preferences with custom configuration file path.
		 * @param configPath Path to the user preferences configuration file (physical, or a virtual "mount:/path")
		 */
		explicit UserPreferences(std::filesystem::path configPath);

		/**
		 * @brief Destructor that saves preferences changed since they were last saved.
		 */
		virtual ~UserPreferences();

		// ----------------------------------------------------
		/// Preference Management
		// ----------------------------------------------------

		/**
		 * @brief Gets whether to show the welcome screen on startup.
		 * @return true if welcome screen should be shown, false otherwise
		 */
		[[nodiscard]] bool GetShowWelcomeScreen() const { return m_ShowWelcomeScreen; }

		/**
		 * @brief Sets whether to show the welcome screen on startup.
		 * @param show true to show welcome screen, false to hide
		 */
		void SetShowWelcomeScreen(bool show);

		/**
		 * @brief Gets the startup project path.
		 * @return Path to the project to load on startup, empty if none
		 */
		[[nodiscard]] const std::string& GetStartupProject() const { return m_StartupProject; }

		/**
		 * @brief Sets the startup project path.
		 * @param projectPath Path to the project to load on startup
		 */
		void SetStartupProject(const std::string& projectPath);

		/**
		 * @brief Gets the map of recent projects ordered by last opened time.
		 * @return Map of recent projects with timestamps as keys
		 */
		[[nodiscard]] std::map<time_t, RecentProject, std::greater<>> GetRecentProjects() const { return m_RecentProjects; }

		/**
		 * @brief Adds a project to the recent projects list (replacing an entry for the same file, however its path is
		 * spelled: "C:\a\b.edX" and "c:/a/b.edX" are one file on Windows and macOS).
		 * @param project Recent project information to add (a lastOpened of 0 means now)
		 */
		void AddRecentProject(const RecentProject& project);

		/**
		 * @brief Removes a project from the recent projects list.
		 * @param projectPath Path of the project to remove
		 */
		void RemoveRecentProject(const std::string& projectPath);

		/**
		 * @brief Clears all recent projects.
		 */
		void ClearRecentProjects();

		// ----------------------------------------------------
		// Persistence
		// ----------------------------------------------------

		/**
		 * @brief Loads user preferences from the configuration file (defaults for anything missing).
		 * @return true if preferences were successfully loaded, false otherwise (the defaults are in use)
		 */
		bool LoadPreferences();

		/**
		 * @brief Saves user preferences to the configuration file (nothing is written when nothing changed).
		 * @return true if preferences were successfully saved, false otherwise
		 */
		bool SavePreferences();

		/**
		 * @brief Gets the configuration file path.
		 * @return Where the configuration file is on disk
		 */
		[[nodiscard]] std::filesystem::path GetConfigPath() const { return m_File.GetPhysicalPath(); }

		/* @brief The configuration file's location as given (virtual "user:/user_preferences.cfg" by default). */
		[[nodiscard]] std::string GetLocation() const { return m_File.GetLocation(); }

		/* @brief True when something changed since the preferences were last loaded or saved. */
		[[nodiscard]] bool IsDirty() const { return m_Dirty; }

	private:
		// User preference data
		bool m_ShowWelcomeScreen = true;
		std::string m_StartupProject;
		std::map<time_t, RecentProject, std::greater<>> m_RecentProjects;

		// Configuration management
		ConfigFile m_File;
		bool m_Dirty = false;			// Changed since the last load / save
		bool m_Writable = true;			// False when the file is there but couldn't be read: it's never written over

		// Maximum number of recent projects to keep
		static constexpr size_t MAX_RECENT_PROJECTS = 10;

		void Init(std::string location);

		/**
		 * @brief Takes out what versions up to 0.2.30 copied into this file from settings.cfg (the application,
		 * x_plane, ui and project sections: settings.cfg keeps them).
		 */
		void RemoveLegacySections();

		/**
		 * @brief Loads recent projects from settings.
		 */
		void LoadRecentProjectsFromSettings();

		/**
		 * @brief Saves recent projects to settings.
		 */
		void SaveRecentProjectsToSettings();

		/**
		 * @brief Ensures recent projects list doesn't exceed maximum size.
		 */
		void TrimRecentProjects();
	};

	// ----------------------------------------------------
	// Convenience Functions
	// ----------------------------------------------------

	/**
	 * @brief Creates and loads user preferences from default location.
	 * @return Shared pointer to loaded user preferences
	 */
	Ref<UserPreferences> CreateUserPreferences();

	/**
	 * @brief Creates and loads user preferences from custom location.
	 * @param configPath Path to configuration file
	 * @return Shared pointer to loaded user preferences
	 */
	Ref<UserPreferences> CreateUserPreferences(const std::filesystem::path& configPath);

}

// -------------------------------------------------------

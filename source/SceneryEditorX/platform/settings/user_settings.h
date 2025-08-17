/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* user_settings.h
* -------------------------------------------------------
* Created: 11/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <filesystem>
#include <map>
#include <string>
#include "SceneryEditorX/project/project_settings.h"
#include "SceneryEditorX/utils/pointers.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{
	/// Forward declarations
	class ApplicationSettings;

	/// ----------------------------------------------------
	/// Date/Time Formatting Utilities
	/// ----------------------------------------------------

	/**
	 * @brief Converts a time_t value to a formatted date/time string.
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
	 * @brief Converts a formatted date/time string back to time_t.
	 *
	 * Parses strings in DD/MM/YYYY HH:MM:SS format or attempts to parse
	 * system-specific formats if detected.
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
	 *
	 * @return true if system uses 12-hour format (AM/PM), false for 24-hour
	 */
	bool IsSystem12HourFormat();

	/**
	 * @class UserPreferences
	 * @brief Manages user-specific preferences and settings for the application.
	 *
	 * Stores user preferences such as welcome screen visibility, startup project,
	 * and recent project history. Uses the ApplicationSettings system for persistent
	 * storage in .cfg format.
	 */
	class UserPreferences : public RefCounted
	{
	public:
		/**
		 * @brief Constructs user preferences with default configuration file path.
		 */
		UserPreferences();

		/**
		 * @brief Constructs user preferences with custom configuration file path.
		 * @param configPath Path to the user preferences configuration file
		 */
		explicit UserPreferences(std::filesystem::path configPath);

		/**
		 * @brief Destructor that automatically saves preferences.
		 */
        virtual ~UserPreferences() override;

		/// ----------------------------------------------------
		/// Preference Management
		/// ----------------------------------------------------

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
		 * @brief Adds a project to the recent projects list.
		 * @param project Recent project information to add
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

		/// ----------------------------------------------------
		/// Persistence
		/// ----------------------------------------------------

		/**
		 * @brief Loads user preferences from the configuration file.
		 * @return true if preferences were successfully loaded, false otherwise
		 */
		bool LoadPreferences();

		/**
		 * @brief Saves user preferences to the configuration file.
		 * @return true if preferences were successfully saved, false otherwise
		 */
		bool SavePreferences();

		/**
		 * @brief Gets the configuration file path.
		 * @return Path to the configuration file being used
		 */
		[[nodiscard]] const std::filesystem::path& GetConfigPath() const { return m_ConfigPath; }

	private:
		/// User preference data
		bool m_ShowWelcomeScreen = true;
		std::string m_StartupProject;
		std::map<time_t, RecentProject, std::greater<>> m_RecentProjects;

		/// Configuration management
		std::filesystem::path m_ConfigPath;
		Ref<ApplicationSettings> m_Settings;

		/// Maximum number of recent projects to keep
		static constexpr size_t MAX_RECENT_PROJECTS = 10;

		/**
		 * @brief Initializes the settings system.
		 */
		void InitializeSettings();

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

	/// ----------------------------------------------------
	/// Convenience Functions
	/// ----------------------------------------------------

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

/// -------------------------------------------------------

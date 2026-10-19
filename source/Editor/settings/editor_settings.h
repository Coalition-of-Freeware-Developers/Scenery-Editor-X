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
 * editor_settings.h
 * -------------------------------------------------------
 * Created: 10/8/2025
 * -------------------------------------------------------
 */
#pragma once
#include <SceneryEditorX/settings/settings.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
/**
	 * @struct EditorSettings
	 * @brief Struct representing the editor settings.
	 */
	struct EditorSettings
	{
		bool enableGridSnapping		= true;
		float translationSnapValue	= 1.0f;		// Snap by 1 unit
		float rotationSnapValue		= 15.0f;    // Snap by 15 degrees
		float scaleSnapValue		= 0.1f;		// Snap by 0.1 units
		bool showColliders			= false;
		bool showLights				= true;
		bool showGrid				= true;
		bool enableVSync			= true;
		int maxFps					= 144;

		static EditorSettings &Get();
		// Add more editor settings as needed
	};

	// -------------------------------------------------------

	/**
	 * @class EditorSettingsSerializer
	 * @brief Handles serialization and deserialization of editor settings.
	 */
	class EditorSettingsSerializer
	{
	public:
		/* @brief Initializes the editor settings serializer. */
		static void Init();

		/* @brief Loads editor settings from the configuration file. */
		static void LoadSettings();

		/* @brief Saves editor settings to the configuration file. */
		static void SaveSettings();
	};

	// -------------------------------------------------------

	/**
	 * @class EditorSettingsManager
	 * @brief Manages persistent editor settings using ApplicationSettings.
	 *
	 * Stores and retrieves editor-specific settings such as grid snapping,
	 * gizmo options, and rendering preferences. Uses the ApplicationSettings
	 * system for persistent storage in .cfg format.
	 */
	class EditorSettingsManager : public Settings
	{
	public:
		/* @brief Constructs editor settings manager with default configuration file path. */
		EditorSettingsManager();

		/**
		 * @brief Constructs editor settings manager with custom configuration file path.
		 * @param configPath Path to the editor settings configuration file
		 */
		explicit EditorSettingsManager(const std::filesystem::path &configPath);

		/* @brief Destructor that automatically saves settings. */
		virtual ~EditorSettingsManager();

		// ----------------------------------------------------
		// Settings Management
		// ----------------------------------------------------

		/**
		 * @brief Gets the path to the editor settings configuration file.
		 * @return Path to the editor settings configuration file
		 */
		static const std::filesystem::path &GetSettingsPath();

		/**
		 * @brief Gets the current editor settings.
		 * @return Current EditorSettings struct
		 */
		[[nodiscard]] const EditorSettings &GetSettings() const { return m_Settings; }

		/**
		 * @brief Updates the editor settings.
		 * @param settings New EditorSettings to apply
		 */
		static void SetSettings(const EditorSettings &settings);

		// ----------------------------------------------------
		// Persistence
		// ----------------------------------------------------

	    /**
		 * @brief Loads editor settings from the configuration file.
		 * @return true if settings were successfully loaded, false otherwise
		 */
		bool ReadSettings() override;

	    /**
		 * @brief Saves editor settings to the configuration file.
		 * @return true if settings were successfully saved, false otherwise
		 */
		void WriteSettings() override;

		/**
		 * @brief Gets the configuration file path.
		 * @return Configuration file path
		 */
		[[nodiscard]] const std::filesystem::path &GetConfigPath() const { return m_ConfigPath; }

	private:
		EditorSettings m_Settings;			// Editor settings data
		std::filesystem::path m_ConfigPath; // Configuration management
		Ref<Settings> m_SettingsStorage;    // Persistent storage for settings

		/* @brief Initializes the settings storage system. */
		void InitializeSettingsStorage();
	};

}

/// -------------------------------------------------------

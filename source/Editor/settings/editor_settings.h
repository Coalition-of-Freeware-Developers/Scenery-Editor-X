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
#include <filesystem>
#include <string>
#include <SceneryEditorX/settings/config_file.h>
#include <SceneryEditorX/utils/pointers.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @struct EditorSettings
	 * @brief Struct representing the editor settings, kept in user:/editor_settings.cfg (the roaming settings folder).
	 *
	 * The snapping and gizmo values are the transform tool's (TransformTool::Settings): EditorSettingsSerializer
	 * applies them when the editor starts and reads them back when it closes. The display and frame-rate values are
	 * stored for the viewport overlays and the frame limiter, which don't read them yet.
	 */
	struct EditorSettings
	{
		bool enableGridSnapping		= false;	// Snap while dragging (Ctrl inverts it)
		float translationSnapValue	= 1.0f;		// Snap by 1 unit
		float rotationSnapValue		= 15.0f;    // Snap by 15 degrees
		float scaleSnapValue		= 0.1f;		// Snap by 0.1 units
		bool gizmoLocalSpace		= false;	// Gizmo axes follow the active object's heading
		bool gizmoPivotActive		= false;	// Turn / scale about the active object instead of the selection's center
		bool showColliders			= false;
		bool showLights				= true;
		bool showGrid				= true;
		bool enableVSync			= true;
		int maxFps					= 144;

		static EditorSettings &Get();
		// Add more editor settings as needed (EditorSettingsManager::ReadSettings / WriteSettings list every field)
	};

	// -------------------------------------------------------

	/**
	 * @class EditorSettingsSerializer
	 * @brief The editor's entry points: EditorSettings::Get() loaded from and saved to user:/editor_settings.cfg,
	 * and kept in step with the tools that use the values.
	 */
	class EditorSettingsSerializer
	{
	public:
		/* @brief Initializes the editor settings serializer (the file at EditorSettingsManager::DEFAULT_LOCATION) and loads it. */
		static void Init();

		/* @brief Loads editor settings from the configuration file and applies them to the tools. */
		static void LoadSettings();

		/* @brief Reads the tools' current values into the editor settings and saves them (when they changed). */
		static void SaveSettings();

		/* @brief Saves what's unsaved and lets the file go (the editor's shutdown). */
		static void Shutdown();
	};

	// -------------------------------------------------------

	/**
	 * @class EditorSettingsManager
	 * @brief Persistent editor settings: an EditorSettings read from and written to its own .cfg file.
	 *
	 * Stores and retrieves editor-specific settings such as grid snapping, gizmo options, and rendering preferences
	 * through a ConfigFile, so the file goes through the VFS (the same on Windows, macOS and Linux, written
	 * atomically). It is separate from the application's settings.cfg.
	 */
	class EditorSettingsManager : public RefCounted
	{
	public:
		/* @brief Where the editor keeps them. */
		static constexpr const char *DEFAULT_LOCATION = "user:/editor_settings.cfg";

		/* @brief Constructs editor settings manager with default configuration file path (and reads it). */
		EditorSettingsManager();

		/**
		 * @brief Constructs editor settings manager with custom configuration file path (and reads it).
		 * @param configPath Path to the editor settings configuration file (physical, or a virtual "mount:/path")
		 */
		explicit EditorSettingsManager(const std::filesystem::path &configPath);

		/* @brief Destructor that saves settings changed since they were last written. */
		virtual ~EditorSettingsManager();

		// ----------------------------------------------------
		// Settings Management
		// ----------------------------------------------------

		/**
		 * @brief Gets the location of the editor settings configuration file.
		 * @return The virtual path the editor uses (DEFAULT_LOCATION)
		 */
		static std::string GetSettingsPath() { return DEFAULT_LOCATION; }

		/**
		 * @brief Gets the current editor settings.
		 * @return Current EditorSettings struct
		 */
		[[nodiscard]] const EditorSettings &GetSettings() const { return m_Settings; }

		/**
		 * @brief Updates the editor settings (WriteSettings saves them).
		 * @param settings New EditorSettings to apply
		 */
		void SetSettings(const EditorSettings &settings);

		// ----------------------------------------------------
		// Persistence
		// ----------------------------------------------------

	    /**
		 * @brief Loads editor settings from the configuration file (defaults for anything missing).
		 * @return true if settings were successfully loaded, false otherwise
		 */
		bool ReadSettings();

	    /**
		 * @brief Saves editor settings to the configuration file (nothing is written when nothing changed).
		 * @return true if settings were successfully saved, false otherwise
		 */
		bool WriteSettings();

		/**
		 * @brief Gets the configuration file path.
		 * @return Where the configuration file is on disk
		 */
		[[nodiscard]] std::filesystem::path GetConfigPath() const { return m_File.GetPhysicalPath(); }

	private:
		EditorSettings m_Settings;			// Editor settings data
		ConfigFile m_File;					// Persistent storage for settings
		bool m_Writable = true;				// False when the file is there but couldn't be read: it's never written over
	};

}

/// -------------------------------------------------------

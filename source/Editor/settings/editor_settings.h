/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* editor_settings.h
* -------------------------------------------------------
* Created: 10/8/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/platform/settings/settings.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	struct EditorSettings
	{
	    bool EnableGridSnapping = true;
	    float TranslationSnapValue = 1.0f; /// Snap by 1 unit
	    float RotationSnapValue = 15.0f;   /// Snap by 15 degrees
	    float ScaleSnapValue = 0.1f;       /// Snap by 0.1 units
	    bool ShowColliders = false;
	    bool ShowLights = true;
	    bool ShowGrid = true;
	    bool EnableVSync = true;
	    int MaxFPS = 144;

		static EditorSettings &Get();
	    /// Add more editor settings as needed
	};

    /// -------------------------------------------------------

	class EditorSettingsSerializer
    {
    public:
        static void Init();

        static void LoadSettings();
        static void SaveSettings();
    };

    /// -------------------------------------------------------

	/**
	 * @class EditorSettingsManager
	 * @brief Manages persistent editor settings using ApplicationSettings.
	 *
	 * Stores and retrieves editor-specific settings such as grid snapping,
	 * gizmo options, and rendering preferences. Uses the ApplicationSettings
	 * system for persistent storage in .cfg format.
	 */
	class EditorSettingsManager : public RefCounted
	{
	public:
	    /**
	     * @brief Constructs editor settings manager with default configuration file path.
	     */
	    EditorSettingsManager();

	    /**
	     * @brief Constructs editor settings manager with custom configuration file path.
	     * @param configPath Path to the editor settings configuration file
	     */
	    explicit EditorSettingsManager(std::filesystem::path configPath);

	    /**
	     * @brief Destructor that automatically saves settings.
	     */
	    virtual ~EditorSettingsManager();

	    /// ----------------------------------------------------
	    /// Settings Management
	    /// ----------------------------------------------------

	    /**
	     * @brief Gets the current editor settings.
	     * @return Current EditorSettings struct
	     */
	    [[nodiscard]] const EditorSettings &GetSettings() const
	    {
	        return m_Settings;
	    }

	    /**
	     * @brief Updates the editor settings.
	     * @param settings New EditorSettings to apply
	     */
	    void SetSettings(const EditorSettings &settings);

	    /// ----------------------------------------------------
	    /// Persistence
	    /// ----------------------------------------------------

	    /**
	     * @brief Loads editor settings from the configuration file.
	     * @return true if settings were successfully loaded, false otherwise
	     */
	    bool LoadSettings();

	    /**
	     * @brief Saves editor settings to the configuration file.
	     * @return true if settings were successfully saved, false otherwise
	     */
	    bool SaveSettings();

	    /**
	     * @brief Gets the configuration file path.
	     * @return
	     */
	    [[nodiscard]] const std::filesystem::path &GetConfigPath() const
	    {
	        return m_ConfigPath;
	    }

	private:
		/// Editor settings data
		EditorSettings m_Settings;
		/// Configuration management
		std::filesystem::path m_ConfigPath;
		Ref<ApplicationSettings> m_SettingsStorage;
		/**
		 * @brief Initializes the settings storage system.
		 */
        void InitializeSettingsStorage();
	};

}

/// -------------------------------------------------------

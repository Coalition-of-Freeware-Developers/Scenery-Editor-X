/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* settings.h
* -------------------------------------------------------
* Created: 23/4/2025
* -------------------------------------------------------
*/
#pragma once
#include <../dependency/libconfig/lib/libconfig.h++>
#include <filesystem>
#include <map>
#include <SceneryEditorX/core/application/application_data.h>
#include <SceneryEditorX/utils/pointers.h>
#include <string>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @class ApplicationSettings
	 * @brief Manages application configuration settings and X-Plane integration.
	 *
	 * Provides functionality for reading and writing application settings, handling X-Plane
	 * path detection and validation, and storing key-value configuration options in various formats.
	 * Settings are stored in a configuration file using libconfig format.
	 */
	class ApplicationSettings : public RefCounted
	{
	public:
	    /**
	     * @brief Constructs settings manager with specified config file path.
	     * @param filepath Path to the configuration file to use
	     */
        explicit ApplicationSettings(std::filesystem::path filepath);
	
	    /**
	     * @brief Reads settings from the configuration file.
	     * @return true if settings were successfully loaded, false otherwise
	     */
	    bool ReadSettings();
	    
	    /**
	     * @brief Writes current settings to the configuration file.
	     */
	    void WriteSettings();
	
	    /// ----------------------------------------------------
	
	    /**
	     * @brief Sets a string option with the specified key.
	     * @param key The setting identifier
	     * @param value The string value to store
	     */
	    void SetOption(const std::string &key, const std::string &value);
	    
	    /**
	     * @brief Gets a string option by key.
	     * @param key The setting identifier
	     * @param value Reference to store the retrieved value
	     */
	    void GetOption(const std::string &key, std::string &value);
	    
	    /**
	     * @brief Checks if a specific option exists.
	     * @param key The setting identifier to check
	     * @return true if the option exists, false otherwise
	     */
	    [[nodiscard]] bool HasOption(const std::string &key) const;
	    
	    /**
	     * @brief Removes an option from settings.
	     * @param key The setting identifier to remove
	     */
	    void RemoveOption(const std::string &key);
	
	    /**
	     * @brief Adds an integer option.
	     * @param path The hierarchical path for the option
	     * @param value Integer value to store
	     */
	    void AddIntOption(const std::string &path, int value);
	    
	    /**
	     * @brief Adds a floating point option.
	     * @param path The hierarchical path for the option
	     * @param value Floating point value to store
	     */
	    void AddFloatOption(const std::string &path, double value);
	    
	    /**
	     * @brief Adds a boolean option.
	     * @param path The hierarchical path for the option
	     * @param value Boolean value to store
	     */
	    void AddBoolOption(const std::string &path, bool value);
	    
	    /**
	     * @brief Adds a string option.
	     * @param path The hierarchical path for the option
	     * @param value String value to store
	     */
	    void AddStringOption(const std::string &path, const std::string &value);
	
	    /**
	     * @brief Retrieves a boolean option.
	     * @param path The hierarchical path for the option
	     * @param defaultValue Value to return if option doesn't exist
	     * @return The boolean value or defaultValue if not found
	     */
        [[nodiscard]] bool GetBoolOption(const std::string &path, bool defaultValue = false) const;
	    
	    /**
	     * @brief Retrieves an integer option.
	     * @param path The hierarchical path for the option
	     * @param defaultValue Value to return if option doesn't exist
	     * @return The integer value or defaultValue if not found
	     */
        [[nodiscard]] int GetIntOption(const std::string &path, int defaultValue = 0) const;
	    
	    /**
	     * @brief Retrieves a floating point option.
	     * @param path The hierarchical path for the option
	     * @param defaultValue Value to return if option doesn't exist
	     * @return The floating point value or defaultValue if not found
	     */
        [[nodiscard]] double GetFloatOption(const std::string &path, double defaultValue = 0.0) const;
	    
	    /**
	     * @brief Retrieves a string option.
	     * @param path The hierarchical path for the option
	     * @param defaultValue Value to return if option doesn't exist
	     * @return The string value or defaultValue if not found
	     */
        [[nodiscard]] std::string GetStringOption(const std::string &path, const std::string &defaultValue = "") const;
	
	    /**
	     * @brief Automatically detects X-Plane installation path.
	     * @return true if X-Plane path was successfully detected, false otherwise
	     */
	    bool DetectXPlanePath();
	    
	    /**
	     * @brief Sets the X-Plane installation path manually.
	     * @param path Directory path to X-Plane installation
	     * @return true if path was valid and successfully set, false otherwise
	     */
	    bool SetXPlanePath(const std::string &path);
	    
	    /**
	     * @brief Gets the current X-Plane installation path.
	     * @return Path to X-Plane installation directory
	     */
        [[nodiscard]] std::string GetXPlanePath() const;
	    
	    /**
	     * @brief Validates that all required X-Plane paths and files exist.
	     * @return true if all required X-Plane paths are valid, false otherwise
	     */
        [[nodiscard]] bool ValidateXPlanePaths() const;
	    
	    /**
	     * @brief Updates derived paths based on main X-Plane path.
	     * Updates resource paths, binary paths, etc. based on the main X-Plane path
	     */
	    void UpdateDerivedXPlanePaths();
	
	    /**
	     * @brief Gets read-only access to X-Plane stats.
	     * @return Const reference to X-Plane statistics
	     */
        [[nodiscard]] const XPlaneStats& GetXPlaneStats() const { return xPlaneStats; }
	    
	    /**
	     * @brief Gets modifiable access to X-Plane stats.
	     * @return Reference to X-Plane statistics
	     */
	    XPlaneStats& GetXPlaneStats() { return xPlaneStats; }
	    
	    /**
	     * @brief Gets read-only access to application stats.
	     * @return Const reference to application statistics
	     */
        [[nodiscard]] const AppData& GetAppStats() const { return appStats; }
	    
	    /**
	     * @brief Gets modifiable access to application stats.
	     * @return Reference to application statistics
	     */
	    AppData& GetAppStats() { return appStats; }
	
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
	     * @brief Initializes config with default minimal settings.
	     * Called when no config file exists or when initialization is needed
	     */
	    void InitMinConfig();
	    
	    /**
	     * @brief Updates libconfig structure from internal data.
	     * Converts internal application and X-Plane stats to configuration format
	     */
	    void UpdateConfigFromData();
	    
	    /**
	     * @brief Loads settings from config to internal map.
	     * Populates the settings map with data from the configuration file
	     */
	    void LoadSettingsToMap();
	    
	    /**
	     * @brief Ensures that required configuration sections exist.
	     * Creates any missing required sections in the configuration
	     */
	    void EnsureRequiredSections();
	
	    /**
	     * @brief Creates a setting path with arbitrary type value.
	     * @tparam T Type of the value to store
	     * @param path The hierarchical path for the setting
	     * @param value Value to store at the specified path
	     */
	    template <typename T>
	    void CreateSettingPath(const std::string &path, const T &value);
	    
	    libconfig::Config cfg;							///< Configuration object
	    XPlaneStats xPlaneStats;						///< X-Plane statistics and paths
        AppData appStats;                               ///< Application statistics
	    std::filesystem::path filePath;					///< Path to configuration file
        std::map<std::string, std::string> settings;    ///< Key-value settings map
        bool configInitialized = false;                 ///< Flag indicating if config is initialized
	};

    /// ----------------------------------------------------------

} // namespace SceneryEditorX

/// -------------------------------------------------------

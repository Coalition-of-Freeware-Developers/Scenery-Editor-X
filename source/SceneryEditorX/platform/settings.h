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
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <iostream>
#include <libconfig.h++>
#include <map>
#include <optional>
#include <SceneryEditorX/core/application_data.h>
#include <string>
#include <vector>

// -------------------------------------------------------

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
	class ApplicationSettings
	{
	public:
	    /**
	    * @brief Constructs settings manager with specified config file path.
	    * @param filepath Path to the configuration file to use
	    */
	    ApplicationSettings(std::filesystem::path filepath);
	
	    /**
	    * @brief Reads settings from the configuration file.
	    * @return true if settings were successfully loaded, false otherwise
	    */
	    bool ReadSettings();
	    
	    /**
	    * @brief Writes current settings to the configuration file.
	    */
	    void WriteSettings();
	
	    // ----------------------------------------------------
	
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
	    bool GetBoolOption(const std::string &path, bool defaultValue = false) const;
	    
	    /**
	    * @brief Retrieves an integer option.
	    * @param path The hierarchical path for the option
	    * @param defaultValue Value to return if option doesn't exist
	    * @return The integer value or defaultValue if not found
	    */
	    int GetIntOption(const std::string &path, int defaultValue = 0) const;
	    
	    /**
	    * @brief Retrieves a floating point option.
	    * @param path The hierarchical path for the option
	    * @param defaultValue Value to return if option doesn't exist
	    * @return The floating point value or defaultValue if not found
	    */
	    double GetFloatOption(const std::string &path, double defaultValue = 0.0) const;
	    
	    /**
	    * @brief Retrieves a string option.
	    * @param path The hierarchical path for the option
	    * @param defaultValue Value to return if option doesn't exist
	    * @return The string value or defaultValue if not found
	    */
	    std::string GetStringOption(const std::string &path, const std::string &defaultValue = "") const;
	
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
	    std::string GetXPlanePath() const;
	    
	    /**
	    * @brief Validates that all required X-Plane paths and files exist.
	    * @return true if all required X-Plane paths are valid, false otherwise
	    */
	    bool ValidateXPlanePaths() const;
	    
	    /**
	    * @brief Updates derived paths based on main X-Plane path.
	    * Updates resource paths, binary paths, etc. based on the main X-Plane path
	    */
	    void UpdateDerivedXPlanePaths();
	
	    /**
	    * @brief Gets read-only access to X-Plane stats.
	    * @return Const reference to X-Plane statistics
	    */
	    const XPlaneStats& GetXPlaneStats() const { return xPlaneStats; }
	    
	    /**
	    * @brief Gets modifiable access to X-Plane stats.
	    * @return Reference to X-Plane statistics
	    */
	    XPlaneStats& GetXPlaneStats() { return xPlaneStats; }
	    
	    /**
	    * @brief Gets read-only access to application stats.
	    * @return Const reference to application statistics
	    */
	    const SoftwareStats& GetAppStats() const { return appStats; }
	    
	    /**
	    * @brief Gets modifiable access to application stats.
	    * @return Reference to application statistics
	    */
	    SoftwareStats& GetAppStats() { return appStats; }
	
	
	private:
	    /**
	    * @brief Initializes config with default minimal settings.
	    * Called when no config file exists or when initialization is needed
	    */
	    void InitializeMinimalConfig();
	    
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
	    void CreateSettingPath(const std::string &path, T value);
	    
	    libconfig::Config cfg;							///< Configuration object
	    XPlaneStats xPlaneStats;						///< X-Plane statistics and paths
	    SoftwareStats appStats;							///< Application statistics
	    std::filesystem::path filePath;					///< Path to configuration file
        std::map<std::string, std::string> settings;    ///< Key-value settings map
        bool configInitialized = false;                 ///< Flag indicating if config is initialized
	};

    /// ----------------------------------------------------------

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
         * @brief Checks if X-Plane 12 exists in the given library path
         * 
         * Searches for X-Plane 12 installation in a specific Steam library folder
         * by checking common installation paths and validating the found directory.
         * 
         * @param libraryPath Path to a Steam library folder to check
         * @return Optional string containing the path if X-Plane 12 is found, empty otherwise
         */
        LOCAL std::optional<std::string> checkForXPlane12(const std::string &libraryPath);

        /**
         * @brief Platform-specific directory separator character
         * 
         * Holds the correct directory separator character for the current platform
         * ('\\' for Windows, '/' for Unix-based systems).
         */
        INTERNAL const char dirSeparator;
    };


} // namespace SceneryEditorX

// -------------------------------------------------------

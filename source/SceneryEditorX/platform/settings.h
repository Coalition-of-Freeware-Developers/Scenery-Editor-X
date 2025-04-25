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
#include <SceneryEditorX/core/application_data.h>
#include <string>
#include <vector>
#include <optional>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class ApplicationSettings
	{
	public:
        ApplicationSettings(std::filesystem::path filepath);
	
		bool ReadSettings();
        void WriteSettings();

		// ----------------------------------------------------

        /// Basic string key-value settings
        void SetOption(const std::string &key, const std::string &value);
        void GetOption(const std::string &key, std::string &value);
        [[nodiscard]] bool HasOption(const std::string &key) const;
        void RemoveOption(const std::string &key);

	    /// Typed settings
        void AddIntOption(const std::string &path, int value);
        void AddFloatOption(const std::string &path, double value);
        void AddBoolOption(const std::string &path, bool value);
        void AddStringOption(const std::string &path, const std::string &value);

        bool GetBoolOption(const std::string &path, bool defaultValue = false) const;
        int GetIntOption(const std::string &path, int defaultValue = 0) const;
        double GetFloatOption(const std::string &path, double defaultValue = 0.0) const;
        std::string GetStringOption(const std::string &path, const std::string &defaultValue = "") const;

		/// X-Plane path management
        bool DetectXPlanePath();
        bool SetXPlanePath(const std::string &path);
        std::string GetXPlanePath() const;
        bool ValidateXPlanePaths() const;
        void UpdateDerivedXPlanePaths();

        /// Access to internal data
        const XPlaneStats& GetXPlaneStats() const { return xPlaneStats; }
        XPlaneStats& GetXPlaneStats() { return xPlaneStats; }
        const SoftwareStats& GetAppStats() const { return appStats; }
        SoftwareStats& GetAppStats() { return appStats; }


	private:
        void InitializeMinimalConfig();
        void UpdateConfigFromData();
        void LoadSettingsToMap();
        void EnsureRequiredSections();

        template <typename T>
        void CreateSettingPath(const std::string &path, T value);
        
        libconfig::Config cfg;
        XPlaneStats xPlaneStats;
        SoftwareStats appStats;
        std::filesystem::path filePath;
        std::map<std::string, std::string> settings;
        bool configInitialized = false;
	};

	class SteamGameFinder
    {
    public:
        /// Find X-Plane 12 in Steam library and return its path if found
        GLOBAL std::optional<std::string> findXPlane12();

        /// Save the found path to config file
        GLOBAL bool savePathToConfig(const std::string &path, const std::string &configFile);

	    /// Validate if path contains X-Plane 12
        GLOBAL bool validateXPlanePath(const std::string &path);
    private:
        /// Get default Steam installation directory based on platform
        LOCAL std::string getSteamDirectory();

        /// Get all Steam library folders
        LOCAL std::vector<std::string> getSteamLibraryFolders(const std::string &steamPath);

        /// Check if X-Plane 12 exists in a given library path
        LOCAL std::optional<std::string> checkForXPlane12(const std::string &libraryPath);

        /// Platform-specific directory separator
        INTERNAL const char dirSeparator;
    };


} // namespace SceneryEditorX

// -------------------------------------------------------

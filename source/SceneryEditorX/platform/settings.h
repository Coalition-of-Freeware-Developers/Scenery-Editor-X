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

        bool GetBoolOption(const std::string &path, bool defaultValue = false);
        int GetIntOption(const std::string &path, int defaultValue = 0);
        double GetFloatOption(const std::string &path, double defaultValue = 0.0);
        std::string GetStringOption(const std::string &path, const std::string &defaultValue = "");



	private:
		void InitializeDefaultConfig();
        void UpdateConfigFromData();
        void LoadSettingsToMap();

        template <typename T>
        void CreateSettingPath(const std::string &path, T value);
        
        libconfig::Config cfg;
        XPlaneStats xPlaneStats;
        SoftwareStats appStats;
	    std::filesystem::path filePath;
	    std::map<std::string, std::string> settings;

	};

} // namespace SceneryEditorX

// -------------------------------------------------------

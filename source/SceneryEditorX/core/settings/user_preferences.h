/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* user_preferences.h
* -------------------------------------------------------
* Created: 4/4/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/core/templates/ref_templates.h>
#include <map>

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct RecentProject
	{
	    std::string Name;
	    std::string FilePath;
	    time_t LastOpened;
	};

	struct UserPreferences : public RefCounted
	{
        std::string FilePath; // Not Serialized
		std::string StartupProject;
		std::map<time_t, RecentProject, std::greater<time_t>> RecentProjects;

		bool ShowWelcomeScreen = true;
	};

	class UserPreferencesSerializer
    {
    public:
        UserPreferencesSerializer(const Ref<UserPreferences> &preferences);
        ~UserPreferencesSerializer();

        void Serialize(const std::filesystem::path &filepath);
        void Deserialize(const std::filesystem::path &filepath);

    private:
        Ref<UserPreferences> m_Preferences;
    };

} // namespace SceneryEditorX

// -------------------------------------------------------

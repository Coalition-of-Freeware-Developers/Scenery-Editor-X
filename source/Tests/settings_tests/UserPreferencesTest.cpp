/**
* -------------------------------------------------------
* Scenery Editor X - Unit Tests
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* UserPreferencesTest.cpp
* -------------------------------------------------------
* Unit tests for the UserPreferences class
* -------------------------------------------------------
*/
#include <catch2/catch_test_macros.hpp>
#include <SceneryEditorX/platform/user_settings.h>
#include <SceneryEditorX/project/project_settings.h>
#include <filesystem>
#include <ctime>

namespace SceneryEditorX
{
    namespace Tests
    {
        // Helper function to create a temporary config file path
        static std::filesystem::path createTempConfigPath()
        {
            static int counter = 0;
            return std::filesystem::temp_directory_path() /
                   ("test_user_prefs_" + std::to_string(++counter) + ".cfg");
        }

        // Helper function to delete a temporary config file
        static void cleanupTempConfig(const std::filesystem::path& path)
        {
            if (std::filesystem::exists(path))
            {
                std::filesystem::remove(path);
            }
        }

        TEST_CASE("UserPreferences Basic Functionality", "[user_preferences][settings]")
        {
            auto tempPath = createTempConfigPath();

            SECTION("Default Construction")
            {
                auto userPrefs = CreateUserPreferences(tempPath);

                REQUIRE(userPrefs != nullptr);
                REQUIRE(userPrefs->GetShowWelcomeScreen() == true);
                REQUIRE(userPrefs->GetStartupProject().empty());
                REQUIRE(userPrefs->GetRecentProjects().empty());
            }

            SECTION("Welcome Screen Setting")
            {
                auto userPrefs = CreateUserPreferences(tempPath);

                // Test setting welcome screen preference
                userPrefs->SetShowWelcomeScreen(false);
                REQUIRE(userPrefs->GetShowWelcomeScreen() == false);

                userPrefs->SetShowWelcomeScreen(true);
                REQUIRE(userPrefs->GetShowWelcomeScreen() == true);
            }

            SECTION("Startup Project Setting")
            {
                auto userPrefs = CreateUserPreferences(tempPath);

                const std::string testProjectPath = "/path/to/test/project.sedx";
                userPrefs->SetStartupProject(testProjectPath);

                REQUIRE(userPrefs->GetStartupProject() == testProjectPath);

                // Test clearing startup project
                userPrefs->SetStartupProject("");
                REQUIRE(userPrefs->GetStartupProject().empty());
            }

            cleanupTempConfig(tempPath);
        }

        TEST_CASE("UserPreferences Recent Projects", "[user_preferences][recent_projects]")
        {
            auto tempPath = createTempConfigPath();
            auto userPrefs = CreateUserPreferences(tempPath);

            SECTION("Adding Recent Projects")
            {
                RecentProject project1;
                project1.name = "Test Project 1";
                project1.filePath = "/path/to/project1.sedx";
                project1.lastOpened = std::time(nullptr);

                RecentProject project2;
                project2.name = "Test Project 2";
                project2.filePath = "/path/to/project2.sedx";
                project2.lastOpened = std::time(nullptr) - 3600; // 1 hour ago

                userPrefs->AddRecentProject(project1);
                userPrefs->AddRecentProject(project2);

                const auto& recentProjects = userPrefs->GetRecentProjects();
                REQUIRE(recentProjects.size() == 2);

                // Should be ordered by most recent first
                auto it = recentProjects.begin();
                REQUIRE(it->second.name == "Test Project 1");
                ++it;
                REQUIRE(it->second.name == "Test Project 2");
            }

            SECTION("Removing Recent Projects")
            {
                RecentProject project;
                project.name = "Test Project";
                project.filePath = "/path/to/project.sedx";
                project.lastOpened = std::time(nullptr);

                userPrefs->AddRecentProject(project);
                REQUIRE(userPrefs->GetRecentProjects().size() == 1);

                userPrefs->RemoveRecentProject(project.filePath);
                REQUIRE(userPrefs->GetRecentProjects().empty());
            }

            SECTION("Clearing Recent Projects")
            {
                // Add multiple projects
                for (int i = 0; i < 5; ++i)
                {
                    RecentProject project;
                    project.name = "Project " + std::to_string(i);
                    project.filePath = "/path/to/project" + std::to_string(i) + ".sedx";
                    project.lastOpened = std::time(nullptr) - (i * 3600);
                    userPrefs->AddRecentProject(project);
                }

                REQUIRE(userPrefs->GetRecentProjects().size() == 5);

                userPrefs->ClearRecentProjects();
                REQUIRE(userPrefs->GetRecentProjects().empty());
            }

            SECTION("Duplicate Project Handling")
            {
                RecentProject project;
                project.name = "Test Project";
                project.filePath = "/path/to/project.sedx";
                project.lastOpened = std::time(nullptr) - 3600;

                userPrefs->AddRecentProject(project);
                REQUIRE(userPrefs->GetRecentProjects().size() == 1);

                // Add the same project with updated timestamp
                project.lastOpened = std::time(nullptr);
                userPrefs->AddRecentProject(project);

                // Should still only have one entry with updated timestamp
                REQUIRE(userPrefs->GetRecentProjects().size() == 1);
                REQUIRE(userPrefs->GetRecentProjects().begin()->first == project.lastOpened);
            }

            cleanupTempConfig(tempPath);
        }

        TEST_CASE("UserPreferences Persistence", "[user_preferences][persistence]")
        {
            auto tempPath = createTempConfigPath();

            SECTION("Save and Load Preferences")
            {
                // Create and configure preferences
                {
                    auto userPrefs = CreateUserPreferences(tempPath);

                    userPrefs->SetShowWelcomeScreen(false);
                    userPrefs->SetStartupProject("/path/to/startup/project.sedx");

                    RecentProject project;
                    project.name = "Persistent Project";
                    project.filePath = "/path/to/persistent/project.sedx";
                    project.lastOpened = 1640995200; // Fixed timestamp for testing

                    userPrefs->AddRecentProject(project);

                    // Force save
                    REQUIRE(userPrefs->SavePreferences() == true);
                }

                // Load preferences from same file
                {
                    auto userPrefs = CreateUserPreferences(tempPath);

                    REQUIRE(userPrefs->GetShowWelcomeScreen() == false);
                    REQUIRE(userPrefs->GetStartupProject() == "/path/to/startup/project.sedx");

                    const auto& recentProjects = userPrefs->GetRecentProjects();
                    REQUIRE(recentProjects.size() == 1);

                    auto project = recentProjects.begin()->second;
                    REQUIRE(project.name == "Persistent Project");
                    REQUIRE(project.filePath == "/path/to/persistent/project.sedx");
                    REQUIRE(project.lastOpened == 1640995200);
                }
            }

            SECTION("Load from Non-existent File")
            {
                auto nonExistentPath = std::filesystem::temp_directory_path() / "non_existent_prefs.cfg";

                // Ensure file doesn't exist
                cleanupTempConfig(nonExistentPath);

                auto userPrefs = CreateUserPreferences(nonExistentPath);

                // Should use defaults
                REQUIRE(userPrefs->GetShowWelcomeScreen() == true);
                REQUIRE(userPrefs->GetStartupProject().empty());
                REQUIRE(userPrefs->GetRecentProjects().empty());

                cleanupTempConfig(nonExistentPath);
            }

            cleanupTempConfig(tempPath);
        }

        TEST_CASE("UserPreferences Recent Projects Limit", "[user_preferences][limits]")
        {
            auto tempPath = createTempConfigPath();
            auto userPrefs = CreateUserPreferences(tempPath);

            SECTION("Maximum Recent Projects Limit")
            {
                // Add more than the maximum allowed projects
                const int numProjects = 15; // More than MAX_RECENT_PROJECTS (10)

                for (int i = 0; i < numProjects; ++i)
                {
                    RecentProject project;
                    project.name = "Project " + std::to_string(i);
                    project.filePath = "/path/to/project" + std::to_string(i) + ".sedx";
                    project.lastOpened = std::time(nullptr) - (i * 60); // 1 minute apart
                    userPrefs->AddRecentProject(project);
                }

                // Should be trimmed to maximum size
                const auto& recentProjects = userPrefs->GetRecentProjects();
                REQUIRE(recentProjects.size() <= 10); // MAX_RECENT_PROJECTS

                // Should keep the most recent projects
                auto it = recentProjects.begin();
                REQUIRE(it->second.name == "Project 0"); // Most recent
            }

            cleanupTempConfig(tempPath);
        }

        TEST_CASE("UserPreferences Configuration File Paths", "[user_preferences][paths]")
        {
            SECTION("Default Configuration Path")
            {
                auto userPrefs = CreateUserPreferences();

                REQUIRE(userPrefs->GetConfigPath().filename() == "user_preferences.cfg");
                REQUIRE(userPrefs->GetConfigPath().parent_path().filename() == "config");
            }

            SECTION("Custom Configuration Path")
            {
                auto customPath = createTempConfigPath();
                auto userPrefs = CreateUserPreferences(customPath);

                REQUIRE(userPrefs->GetConfigPath() == customPath);

                cleanupTempConfig(customPath);
            }
        }

    } // namespace Tests
} // namespace SceneryEditorX

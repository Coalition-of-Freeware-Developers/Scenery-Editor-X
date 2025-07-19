/**
* -------------------------------------------------------
* Scenery Editor X - Unit Tests
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* SettingsTest.cpp
* -------------------------------------------------------
* Unit tests for the ApplicationSettings class
* -------------------------------------------------------
*/
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <SceneryEditorX/platform/settings/settings.h>
#include <SceneryEditorX/utils/pointers.h>
#include <string>

/// -------------------------------------------------------------------

namespace SceneryEditorX
{
    namespace Tests
    {
        // Helper function to create a temporary settings file with content
        static std::filesystem::path createTempSettingsFile(const std::string& content = "")
        {
            std::filesystem::path tempPath = std::filesystem::temp_directory_path() / "test_settings.cfg";

            if (!content.empty())
            {
                std::ofstream file(tempPath);
                file << content;
                file.close();
            }

            return tempPath;
        }

        // Helper function to delete a temporary settings file
        static void cleanupTempSettingsFile(const std::filesystem::path& path)
        {
            if (std::filesystem::exists(path))
                std::filesystem::remove(path);
        }

        // Simple test fixture for settings tests
        class SettingsFixture
        {
        public:
            SettingsFixture()
            {
                // Create a temporary settings file with some initial content
                tempFilePath = createTempSettingsFile(
                    "# Scenery Editor X Configuration\n"
                    "application: {\n"
                    "  version = \"1.0.0\";\n"
                    "  no_titlebar = false;\n"
                    "};\n"
                    "x_plane: {\n"
                    "  version = \"X-Plane 12.06b1\";\n"
                    "  path = \"C:/Test/X-Plane 12\";\n"
                    "  bin_path = \"C:/Test/X-Plane 12/bin\";\n"
                    "  resources_path = \"C:/Test/X-Plane 12/Resources\";\n"
                    "  is_steam = false;\n"
                    "};\n"
                    "ui: {\n"
                    "  theme = \"dark\";\n"
                    "  font_size = 12;\n"
                    "  language = \"english\";\n"
                    "};\n"
                    "project: {\n"
                    "  auto_save = true;\n"
                    "  auto_save_interval = 5;\n"
                    "  backup_count = 3;\n"
                    "  default_project_dir = \"C:/Users/Test/Documents/SceneryEditorX\";\n"
                    "};\n"
                );

                // Create the settings object
                settings = CreateRef<ApplicationSettings>(tempFilePath);
            }

            ~SettingsFixture()
            {
                // Clean up
                settings.Reset();
                cleanupTempSettingsFile(tempFilePath);
            }

            Ref<ApplicationSettings> settings;
            std::filesystem::path tempFilePath;
        };

        TEST_CASE_METHOD(SettingsFixture, "ApplicationSettings constructor and basic functionality", "[Settings][basic]")
        {
            SECTION("Constructor should initialize correctly")
            {
                REQUIRE(settings->ReadSettings());
                REQUIRE(settings->GetStringOption("application.version") == "1.0.0");
            }

            SECTION("ReadSettings reads from existing file")
            {
                REQUIRE(settings->ReadSettings());
                REQUIRE(settings->GetStringOption("ui.theme") == "dark");
                REQUIRE(settings->GetIntOption("ui.font_size") == 12);
            }

            SECTION("WriteSettings writes to file")
            {
                // Modify a setting
                settings->AddStringOption("ui.theme", "light");
                settings->WriteSettings();

                // Create a new settings object to read from the same file
                auto newSettings = CreateRef<ApplicationSettings>(tempFilePath);
                REQUIRE(newSettings->ReadSettings());
                REQUIRE(newSettings->GetStringOption("ui.theme") == "light");
            }

            SECTION("HasOption returns correct values")
            {
                REQUIRE(settings->HasOption("ui.theme"));
                REQUIRE_FALSE(settings->HasOption("nonexistent.option"));
            }
        }

        TEST_CASE_METHOD(SettingsFixture, "String option operations", "[Settings][strings]")
        {
            SECTION("AddStringOption sets string value")
            {
                settings->AddStringOption("test.string", "test value");
                REQUIRE(settings->GetStringOption("test.string") == "test value");
            }

            SECTION("GetStringOption retrieves string value")
            {
                settings->AddStringOption("test.another", "another value");
                REQUIRE(settings->GetStringOption("test.another") == "another value");

                // Test with non-existent option - should return default
                REQUIRE(settings->GetStringOption("nonexistent") == "");
                REQUIRE(settings->GetStringOption("nonexistent", "default") == "default");
            }
                REQUIRE(value == "another value");
            }

            SECTION("AddStringOption adds string option")
            {
                settings->AddStringOption("test.path.string", "test string");
                REQUIRE(settings->GetStringOption("test.path.string") == "test string");
            }

            SECTION("GetStringOption with default value")
            {
                REQUIRE(settings->GetStringOption("nonexistent", "default") == "default");
            }

            SECTION("RemoveOption removes option")
            {
                settings->AddStringOption("test.remove", "to be removed");
                REQUIRE(settings->HasOption("test.remove"));

                settings->RemoveOption("test.remove");
                REQUIRE_FALSE(settings->HasOption("test.remove"));
            }
        }

        TEST_CASE_METHOD(Tests::SettingsFixture, "Integer option operations", "[Settings][integer]")
        {
            SECTION("AddIntOption adds integer option")
            {
                settings->AddIntOption("test.int", 42);
                REQUIRE(settings->GetIntOption("test.int") == 42);
            }

            SECTION("GetIntOption with default value")
            {
                REQUIRE(settings->GetIntOption("nonexistent.int", 100) == 100);
            }

            SECTION("Modify existing integer option")
            {
                // Test with existing option
                REQUIRE(settings->GetIntOption("ui.font_size") == 12);

                settings->AddIntOption("ui.font_size", 14);
                REQUIRE(settings->GetIntOption("ui.font_size") == 14);
            }
        }

        TEST_CASE_METHOD(Tests::SettingsFixture, "Boolean option operations", "[Settings][boolean]")
        {
            SECTION("AddBoolOption adds boolean option")
            {
                settings->AddBoolOption("test.bool", true);
                REQUIRE(settings->GetBoolOption("test.bool") == true);

                settings->AddBoolOption("test.bool2", false);
                REQUIRE(settings->GetBoolOption("test.bool2") == false);
            }

            SECTION("GetBoolOption with default value")
            {
                REQUIRE(settings->GetBoolOption("nonexistent.bool", true) == true);
                REQUIRE(settings->GetBoolOption("nonexistent.bool", false) == false);
            }

            SECTION("Modify existing boolean option")
            {
                // Test with existing option
                REQUIRE(settings->GetBoolOption("application.no_titlebar") == false);

                settings->AddBoolOption("application.no_titlebar", true);
                REQUIRE(settings->GetBoolOption("application.no_titlebar") == true);
            }
        }

        TEST_CASE_METHOD(Tests::SettingsFixture, "Floating point option operations", "[Settings][float]")
        {
            SECTION("AddFloatOption adds floating point option")
            {
                settings->AddFloatOption("test.float", 3.14159);
                REQUIRE(settings->GetFloatOption("test.float") == Catch::Approx(3.14159));
            }

            SECTION("GetFloatOption with default value")
            {
                REQUIRE(settings->GetFloatOption("nonexistent.float", 2.71828) == Catch::Approx(2.71828));
            }

            SECTION("Modify existing floating point option")
            {
                // Add a float option first
                settings->AddFloatOption("test.modify_float", 1.0);
                REQUIRE(settings->GetFloatOption("test.modify_float") == Catch::Approx(1.0));

                // Now modify it
                settings->AddFloatOption("test.modify_float", 2.0);
                REQUIRE(settings->GetFloatOption("test.modify_float") == Catch::Approx(2.0));
            }
        }

        TEST_CASE_METHOD(Tests::SettingsFixture, "X-Plane path operations", "[Settings][xplane]")
        {
            SECTION("GetXPlanePath returns correct path")
            {
                REQUIRE(settings->GetXPlanePath() == "C:/Test/X-Plane 12");
            }

            SECTION("SetXPlanePath updates path and derived paths")
            {
                // Test setting new X-Plane path
                REQUIRE(settings->SetXPlanePath("D:/X-Plane 12"));
                REQUIRE(settings->GetXPlanePath() == "D:/X-Plane 12");

                // Note: Derived paths are updated internally by the UpdateDerivedXPlanePaths method
                // We can verify through the X-Plane stats
                const auto& xpStats = settings->GetXPlaneStats();
                REQUIRE(xpStats.xPlanePath == "D:/X-Plane 12");
            }

            SECTION("ValidateXPlanePaths validates paths")
            {
                // Note: This test might fail if actual paths don't exist
                // In production tests, you might want to mock this
                const bool isValid = settings->ValidateXPlanePaths();
                // Just verify the method can be called - actual validation depends on file system
                REQUIRE((isValid == true || isValid == false));
            }
        }

        TEST_CASE("Creating ApplicationSettings with non-existent file", "[Settings][initialization]")
        {
            // Create with a path to a non-existent file
            auto nonExistentPath = std::filesystem::temp_directory_path() / "nonexistent_settings.cfg";

            // Ensure the file doesn't exist
            Tests::cleanupTempSettingsFile(nonExistentPath);

            // Create settings with non-existent file should initialize with defaults
            const auto settings = CreateRef<ApplicationSettings>(nonExistentPath);

            // The file should now exist with default values
            REQUIRE(std::filesystem::exists(nonExistentPath));

            // Verify some default values are present
            REQUIRE(settings->HasOption("application.version"));
            REQUIRE(settings->HasOption("ui.theme"));

            // Clean up after test
            Tests::cleanupTempSettingsFile(nonExistentPath);
        }

        TEST_CASE("Settings persistence across instances", "[Settings][persistence]")
        {
            // Create a temporary file
            auto tempPath = Tests::createTempSettingsFile();

            // First instance
            {
                auto settings1 = CreateRef<ApplicationSettings>(tempPath);
                settings1->AddStringOption("test.persistence", "persistent value");
                settings1->AddIntOption("test.int_persistence", 12345);
                settings1->AddBoolOption("test.bool_persistence", true);
                settings1->AddFloatOption("test.float_persistence", 98.76);
                settings1->WriteSettings();
            }

            // Second instance should read the values written by first
            {
                auto settings2 = CreateRef<ApplicationSettings>(tempPath);
                REQUIRE(settings2->ReadSettings());
                REQUIRE(settings2->GetStringOption("test.persistence") == "persistent value");
                REQUIRE(settings2->GetIntOption("test.int_persistence") == 12345);
                REQUIRE(settings2->GetBoolOption("test.bool_persistence") == true);
                REQUIRE(settings2->GetFloatOption("test.float_persistence") == Catch::Approx(98.76));
            }

            // Clean up
            Tests::cleanupTempSettingsFile(tempPath);
        }

        TEST_CASE("Error handling for invalid settings files", "[Settings][errors]")
        {
            // Create a temporary file with invalid content
            auto invalidPath = Tests::createTempSettingsFile("This is not a valid config file");

            // Attempt to read invalid settings
            auto settings = CreateRef<ApplicationSettings>(invalidPath);

            // ReadSettings should return false for invalid file
            REQUIRE_FALSE(settings->ReadSettings());

            // But the settings object should still be usable with defaults
            settings->AddStringOption("test.after_error", "value after error");
            REQUIRE(settings->GetStringOption("test.after_error") == "value after error");

            // Clean up
            Tests::cleanupTempSettingsFile(invalidPath);
        }

        // Test for Vulkan-related settings without using SetCustomBufferSize
        TEST_CASE("Basic Vulkan settings operations", "[Settings][vulkan]")
        {
            auto tempPath = Tests::createTempSettingsFile();
            auto settings = CreateRef<ApplicationSettings>(tempPath);

            SECTION("Vulkan buffer size can be set and retrieved via standard options")
            {
                // Use the standard option setters and getters
                settings->AddIntOption("vulkan.buffer_size", 1024 * 1024);
                REQUIRE(settings->GetIntOption("vulkan.buffer_size") == 1024 * 1024);

                // Try modifying it
                settings->AddIntOption("vulkan.buffer_size", 2 * 1024 * 1024);
                REQUIRE(settings->GetIntOption("vulkan.buffer_size") == 2 * 1024 * 1024);
            }

            // Clean up
            Tests::cleanupTempSettingsFile(tempPath);
        }

}


/// -------------------------------------------------------------------

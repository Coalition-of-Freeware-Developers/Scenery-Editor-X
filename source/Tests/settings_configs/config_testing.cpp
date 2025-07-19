/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* settings_test.cpp
* -------------------------------------------------------
* Created: 24/4/2025
* -------------------------------------------------------
*/
#include <../SceneryEditorX/platform/settings.h>
#include <iostream>
#include <filesystem>
#include <cassert>

// Simple assertion helper
#define TEST_ASSERT(condition, message) \
    if (!(condition)) { \
        std::cerr << "ASSERTION FAILED: " << message << " at line " << __LINE__ << std::endl; \
        testsFailCount++; \
    } else { \
        testsPassCount++; \
    }

using namespace SceneryEditorX;

int main() {
    int testsPassCount = 0;
    int testsFailCount = 0;

    // Use a temporary test file
    std::filesystem::path testConfigPath = "settings_test.cfg";

    // Delete the file if it exists already
    if (std::filesystem::exists(testConfigPath)) {
        std::filesystem::remove(testConfigPath);
    }

    std::cout << "== Starting ApplicationSettings Tests ==" << std::endl;

    // Test 1: Create a new configuration with defaults
    {
        std::cout << "Test 1: Initializing with default config..." << std::endl;
        ApplicationSettings settings(testConfigPath);

        // Verify some default values exist
        std::string themeValue = settings.GetStringOption("ui.theme");
        TEST_ASSERT(!themeValue.empty(), "Default theme should be set");
        TEST_ASSERT(settings.GetBoolOption("project.auto_save"), "Default auto_save should be true");

        // Verify writing to file succeeded
        settings.WriteSettings();
        TEST_ASSERT(std::filesystem::exists(testConfigPath), "Config file should be created");
    }

    // Test 2: Read existing configuration
    {
        std::cout << "Test 2: Reading existing config..." << std::endl;
        ApplicationSettings settings(testConfigPath);

        // Verify some values were persisted
        TEST_ASSERT(settings.HasOption("ui.theme"), "ui.theme should exist in loaded config");
        TEST_ASSERT(settings.GetBoolOption("project.auto_save"), "auto_save should be loaded correctly");
    }

    // Test 3: Modify settings
    {
        std::cout << "Test 3: Modifying settings..." << std::endl;
        ApplicationSettings settings(testConfigPath);

        // Modify existing settings
        settings.AddStringOption("ui.theme", "light");
        settings.AddIntOption("ui.font_size", 14);
        settings.AddBoolOption("project.auto_save", false);

        // Add new settings
        settings.AddStringOption("custom.test_string", "test_value");
        settings.AddIntOption("custom.test_int", 42);
        settings.AddFloatOption("custom.test_float", 3.14159);
        settings.AddBoolOption("custom.test_bool", true);

        // Verify the changes in memory
        TEST_ASSERT(settings.GetStringOption("ui.theme") == "light", "Theme should be updated to light");
        TEST_ASSERT(settings.GetIntOption("ui.font_size") == 14, "Font size should be updated to 14");
        TEST_ASSERT(!settings.GetBoolOption("project.auto_save"), "auto_save should be updated to false");
        TEST_ASSERT(settings.GetStringOption("custom.test_string") == "test_value", "Custom string should be set");
        TEST_ASSERT(settings.GetIntOption("custom.test_int") == 42, "Custom int should be set");
        TEST_ASSERT(std::abs(settings.GetFloatOption("custom.test_float") - 3.14159) < 0.0001, "Custom float should be set");
        TEST_ASSERT(settings.GetBoolOption("custom.test_bool"), "Custom bool should be set");

        // Save changes
        settings.WriteSettings();
    }

    // Test 4: Verify persistence
    {
        std::cout << "Test 4: Verifying persistence..." << std::endl;
        ApplicationSettings settings(testConfigPath);

        // Verify modified values were correctly persisted
        TEST_ASSERT(settings.GetStringOption("ui.theme") == "light", "Theme change should persist");
        TEST_ASSERT(settings.GetIntOption("ui.font_size") == 14, "Font size change should persist");
        TEST_ASSERT(!settings.GetBoolOption("project.auto_save"), "auto_save change should persist");

        // Verify new values were correctly persisted
        TEST_ASSERT(settings.GetStringOption("custom.test_string") == "test_value", "Custom string should persist");
        TEST_ASSERT(settings.GetIntOption("custom.test_int") == 42, "Custom int should persist");
        TEST_ASSERT(std::abs(settings.GetFloatOption("custom.test_float") - 3.14159) < 0.0001, "Custom float should persist");
        TEST_ASSERT(settings.GetBoolOption("custom.test_bool"), "Custom bool should persist");
    }

    // Test 5: Test basic string options
    {
        std::cout << "Test 5: Testing basic string options..." << std::endl;
        ApplicationSettings settings(testConfigPath);

        // Use SetOption/GetOption for string values
        settings.SetOption("string_test.key1", "value1");
        std::string result;
        settings.GetOption("string_test.key1", result);
        TEST_ASSERT(result == "value1", "GetOption should retrieve the correct value");

        // Test HasOption
        TEST_ASSERT(settings.HasOption("string_test.key1"), "HasOption should return true for existing option");
        TEST_ASSERT(!settings.HasOption("nonexistent.key"), "HasOption should return false for nonexistent option");

        // Test RemoveOption
        settings.RemoveOption("string_test.key1");
        TEST_ASSERT(!settings.HasOption("string_test.key1"), "RemoveOption should remove the option");

        settings.WriteSettings();
    }

    // Test 6: Edge cases
    {
        std::cout << "Test 6: Testing edge cases..." << std::endl;
        ApplicationSettings settings(testConfigPath);

        // Empty values
        settings.AddStringOption("edge_cases.empty_string", "");
        TEST_ASSERT(settings.GetStringOption("edge_cases.empty_string") == "", "Empty string should be handled correctly");

        // Default values when setting doesn't exist
        TEST_ASSERT(settings.GetStringOption("nonexistent.path", "default") == "default", "Default string should be returned");
        TEST_ASSERT(settings.GetIntOption("nonexistent.path", -1) == -1, "Default int should be returned");
        TEST_ASSERT(settings.GetFloatOption("nonexistent.path", -1.0) == -1.0, "Default float should be returned");
        TEST_ASSERT(settings.GetBoolOption("nonexistent.path", true), "Default bool should be returned");

        settings.WriteSettings();
    }

    // Test 7: Deep hierarchies
    {
        std::cout << "Test 7: Testing deep hierarchies..." << std::endl;
        ApplicationSettings settings(testConfigPath);

        // Create a deep hierarchy
        settings.AddStringOption("level1.level2.level3.level4.level5", "deep_value");
        TEST_ASSERT(settings.GetStringOption("level1.level2.level3.level4.level5") == "deep_value",
                   "Deep hierarchy should be handled correctly");

        settings.WriteSettings();
    }

    // Cleanup
    if (std::filesystem::exists(testConfigPath)) {
        std::filesystem::remove(testConfigPath);
    }

    // Report results
    std::cout << "== Test Results ==" << std::endl;
    std::cout << "Tests passed: " << testsPassCount << std::endl;
    std::cout << "Tests failed: " << testsFailCount << std::endl;

    return testsFailCount > 0 ? 1 : 0;
}

/**
* -------------------------------------------------------
* Scenery Editor X - edX Manager Tests
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* EdxManagerTest.cpp
* -------------------------------------------------------
* Tests for EdxManager high-level API
* -------------------------------------------------------
*/
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <memory>
#include "../../edX/edXConfig.h"
#include "../../edX/edXManager.h"

/// -------------------------------------------------------

using namespace edx;
using Catch::Approx;

namespace EdxTests
{
namespace ManagerTests
{
// Test callback implementations
class TestCallbacks
{
public:
    static void ProgressCallback(float progress, const std::string &status)
    {
        lastProgress = progress;
        lastStatus = status;
        progressCallCount++;
    }

    static void ErrorCallback(const std::string &error)
    {
        lastError = error;
        errorCallCount++;
    }

    static void Reset()
    {
        lastProgress = 0.0f;
        lastStatus.clear();
        lastError.clear();
        progressCallCount = 0;
        errorCallCount = 0;
    }

    static float lastProgress;
    static std::string lastStatus;
    static std::string lastError;
    static int progressCallCount;
    static int errorCallCount;
};

// Static member definitions
float TestCallbacks::lastProgress = 0.0f;
std::string TestCallbacks::lastStatus;
std::string TestCallbacks::lastError;
int TestCallbacks::progressCallCount = 0;
int TestCallbacks::errorCallCount = 0;

} // namespace ManagerTests
} // namespace EdxTests

/// -------------------------------------------------------

TEST_CASE("EdxManager Construction", "[manager][basic]")
{
    using namespace EdxTests::ManagerTests;

    SECTION("Manager can be created and destroyed")
    {
        EdxManager manager;
        // Basic construction test - if this compiles and runs, it's a success
        REQUIRE(true);
    }
}

TEST_CASE("Project Operations via Manager", "[manager][project]")
{
    using namespace EdxTests::ManagerTests;

    // Create test output directory
    auto testDir = std::filesystem::current_path() / "test_output";
    std::filesystem::create_directories(testDir);

    SECTION("Create new project")
    {
        EdxManager manager;

        auto project = manager.create_project("Test Manager Project", "Test Author", "KTST");

        REQUIRE(project != nullptr);
        REQUIRE(project->project.name == "Test Manager Project");
        REQUIRE(project->project.author == "Test Author");
        REQUIRE(project->airport.icao == "KTST");
    }

    SECTION("Save and load project via manager")
    {
        EdxManager manager;
        TestCallbacks::Reset();

        // Create a project
        auto originalProject = manager.create_project("Manager Test Project", "Test Author", "KMGR");
        REQUIRE(originalProject != nullptr);

        // Add some content
        originalProject->project.description = "Created via EdxManager for testing";
        originalProject->airport.name = "Manager Test Airport";
        originalProject->airport.city = "Test City";
        originalProject->airport.datumLat = 35.0;
        originalProject->airport.datumLon = -120.0;

        // Save via manager
        auto saveFilePath = testDir / "manager_test_project.edX";
        bool saveResult =
            manager.save_project(*originalProject, saveFilePath.string(), TestCallbacks::ProgressCallback);

        REQUIRE(saveResult == true);
        REQUIRE(std::filesystem::exists(saveFilePath));

        // Load via manager
        auto loadedProject = manager.load_project(saveFilePath.string(), TestCallbacks::ProgressCallback);

        REQUIRE(loadedProject != nullptr);
        REQUIRE(loadedProject->project.name == originalProject->project.name);
        REQUIRE(loadedProject->project.author == originalProject->project.author);
        REQUIRE(loadedProject->project.description == originalProject->project.description);
        REQUIRE(loadedProject->airport.icao == originalProject->airport.icao);
        REQUIRE(loadedProject->airport.name == originalProject->airport.name);
        REQUIRE(loadedProject->airport.datumLat == Approx(originalProject->airport.datumLat));
        REQUIRE(loadedProject->airport.datumLon == Approx(originalProject->airport.datumLon));
    }

    SECTION("Load non-existent project file")
    {
        EdxManager manager;

        auto nonExistentPath = testDir / "non_existent_manager.edX";
        auto project = manager.load_project(nonExistentPath.string());

        REQUIRE(project == nullptr);
    }
}

TEST_CASE("Library Operations via Manager", "[manager][library]")
{
    using namespace EdxTests::ManagerTests;

    // Create test output directory
    auto testDir = std::filesystem::current_path() / "test_output";
    std::filesystem::create_directories(testDir);

    SECTION("Create new library")
    {
        EdxManager manager;

        auto library = manager.create_library("Test Manager Library", "Test Author", "1.0.0");

        REQUIRE(library != nullptr);
        REQUIRE(library->library.name == "Test Manager Library");
        REQUIRE(library->library.author == "Test Author");
        REQUIRE(library->library.version == "1.0.0");
    }

    SECTION("Save and load library via manager")
    {
        EdxManager manager;
        TestCallbacks::Reset();

        // Create a library
        auto originalLibrary = manager.create_library("Manager Test Library", "Test Author", "2.1.0");
        REQUIRE(originalLibrary != nullptr);

        // Add some content
        originalLibrary->library.description = "Created via EdxManager for testing";
        originalLibrary->library.sizeInMB = 75.5;

        // Add a test object
        LibraryObject obj;
        obj.id = "manager_test_obj";
        obj.uniqueId = "12345678";
        obj.assetType = "building";
        obj.name = "Manager Test Building";
        obj.description = "Test building created via manager";
        obj.category = "test";
        obj.properties["testProp"] = "testValue";
        originalLibrary->add_object(obj);

        // Save via manager
        auto saveFilePath = testDir / "manager_test_library.edxlib";
        bool saveResult =
            manager.save_library(*originalLibrary, saveFilePath.string(), TestCallbacks::ProgressCallback);

        REQUIRE(saveResult == true);
        REQUIRE(std::filesystem::exists(saveFilePath));

        // Load via manager
        auto loadedLibrary = manager.load_library(saveFilePath.string(), TestCallbacks::ProgressCallback);

        REQUIRE(loadedLibrary != nullptr);
        REQUIRE(loadedLibrary->library.name == originalLibrary->library.name);
        REQUIRE(loadedLibrary->library.author == originalLibrary->library.author);
        REQUIRE(loadedLibrary->library.version == originalLibrary->library.version);
        REQUIRE(loadedLibrary->library.description == originalLibrary->library.description);
        REQUIRE(loadedLibrary->library.sizeInMB == Approx(originalLibrary->library.sizeInMB));
        REQUIRE(loadedLibrary->get_object_count() == originalLibrary->get_object_count());

        // Check the loaded object
        auto *loadedObj = loadedLibrary->find_object("manager_test_obj");
        REQUIRE(loadedObj != nullptr);
        REQUIRE(loadedObj->name == "Manager Test Building");
        REQUIRE(loadedObj->assetType == "building");
        REQUIRE(loadedObj->properties["testProp"] == "testValue");
    }
}

TEST_CASE("Manager Validation Functions", "[manager][validation]")
{
    using namespace EdxTests::ManagerTests;

    SECTION("Validate valid project")
    {
        EdxManager manager;

        auto project = manager.create_project("Valid Project", "Test Author", "KVLD");
        project->project.description = "A valid test project";
        project->airport.name = "Valid Test Airport";
        project->airport.datumLat = 40.0;
        project->airport.datumLon = -75.0;

        auto errors = manager.validate_project(*project);
        REQUIRE(errors.empty());
    }

    SECTION("Validate invalid project")
    {
        EdxManager manager;

        EdxProject invalidProject;
        // Leave most fields empty/invalid

        auto errors = manager.validate_project(invalidProject);
        REQUIRE_FALSE(errors.empty());

        // Should have multiple validation errors
        INFO("Validation errors found: " << errors.size());
        for (const auto &error : errors)
        {
            INFO("  Error: " << error);
        }
    }

    SECTION("Validate valid library")
    {
        EdxManager manager;

        auto library = manager.create_library("Valid Library", "Test Author", "1.0.0");
        library->library.description = "A valid test library";

        auto errors = manager.validate_library(*library);
        REQUIRE(errors.empty());
    }

    SECTION("Validate invalid library")
    {
        EdxManager manager;

        LibraryFile invalidLibrary;
        // Leave most fields empty/invalid

        auto errors = manager.validate_library(invalidLibrary);
        REQUIRE_FALSE(errors.empty());

        // Should have validation errors
        INFO("Validation errors found: " << errors.size());
        for (const auto &error : errors)
        {
            INFO("  Error: " << error);
        }
    }
}

TEST_CASE("Manager Error Handling", "[manager][error-handling]")
{
    using namespace EdxTests::ManagerTests;

    // Create test output directory
    auto testDir = std::filesystem::current_path() / "test_output";
    std::filesystem::create_directories(testDir);

    SECTION("Handle invalid save paths")
    {
        EdxManager manager;

        auto project = manager.create_project("Test Project", "Test Author");

        // Try to save to an invalid path
        auto invalidPath = "/invalid/path/that/cannot/exist/test.edX";
        bool result = manager.save_project(*project, invalidPath);

        REQUIRE(result == false);
    }

    SECTION("Handle corrupted project files")
    {
        EdxManager manager;

        // Create a corrupted file
        auto corruptedPath = testDir / "corrupted_manager.edX";
        std::ofstream corruptedFile(corruptedPath);
        corruptedFile << "This is not valid JSON content at all!";
        corruptedFile.close();

        auto loadedProject = manager.load_project(corruptedPath.string());
        REQUIRE(loadedProject == nullptr);
    }

    SECTION("Handle corrupted library files")
    {
        EdxManager manager;

        // Create a corrupted library file
        auto corruptedPath = testDir / "corrupted_manager.edxlib";
        std::ofstream corruptedFile(corruptedPath);
        corruptedFile << "{\"incomplete\": \"json without proper";
        corruptedFile.close();

        auto loadedLibrary = manager.load_library(corruptedPath.string());
        REQUIRE(loadedLibrary == nullptr);
    }
}

TEST_CASE("Manager Comprehensive Integration", "[manager][integration][file-generation]")
{
    using namespace EdxTests::ManagerTests;

    // Create test output directory
    auto testDir = std::filesystem::current_path() / "test_output";
    std::filesystem::create_directories(testDir);

    SECTION("Complete workflow via manager")
    {
        EdxManager manager;
        TestCallbacks::Reset();

        // Create a comprehensive project
        auto project = manager.create_project("Comprehensive Manager Project", "EdX Test Suite", "KCMP");

        // Fill out project details
        project->project.description = "A comprehensive project created and managed entirely "
                                       "through the EdxManager API for testing purposes.";
        project->project.editorVersion = edx::VERSION;
        project->project.xplaneVersion = "12.10";

        // Configure airport
        project->airport.name = "Comprehensive Test Airport";
        project->airport.city = "Manager City";
        project->airport.state = "Test State";
        project->airport.country = "Test Country";
        project->airport.datumLat = 45.0;
        project->airport.datumLon = -100.0;
        project->airport.elevation = 1500;
        project->airport.ctaf = std::make_unique<double>(122.8);
        project->airport.tower = std::make_unique<double>(119.3);

        // Add library references
        LibraryReference lib1;
        lib1.name = "Manager Test Library 1";
        lib1.localPath = "libraries/manager_lib1.edxlib";
        lib1.uuid = "manager-lib-1-uuid-12345";
        lib1.shortId = "ML1";
        lib1.entryCount = 25;
        lib1.version = "1.0.0";
        project->libraries.push_back(lib1);

        // Add assets
        for (int i = 1; i <= 5; ++i)
        {
            SceneAsset asset;
            asset.id = "manager_asset_" + std::to_string(i);
            asset.uniqueId = asset.id + "_unique";
            asset.latitude = 45.0 + (i * 0.001);
            asset.longitude = -100.0 + (i * 0.001);
            asset.altitude = 1500.0 + (i * 2.0);
            asset.heading = i * 72.0; // 72 degrees apart
            asset.associatedLibrary = "ML1";
            asset.layerId = "manager_layer";
            asset.otherProperties["managedBy"] = "EdxManager";
            asset.otherProperties["assetNumber"] = i;
            project->assets.push_back(asset);
        }

        // Add a layer
        SceneLayer layer;
        layer.layerId = "manager_layer";
        layer.name = "Manager Test Layer";
        layer.description = "Layer created via EdxManager";
        layer.opacity = 1.0;
        layer.zOrder = 1;
        for (const auto &asset : project->assets)
        {
            layer.assetIds.push_back(asset.id);
        }
        project->layers.push_back(layer);

        // Validate before saving
        auto projectErrors = manager.validate_project(*project);
        REQUIRE(projectErrors.empty());

        // Save the project
        auto projectPath = testDir / "comprehensive_manager_project.edX";
        bool projectSaveResult = manager.save_project(*project, projectPath.string(), TestCallbacks::ProgressCallback);
        REQUIRE(projectSaveResult == true);
        REQUIRE(std::filesystem::exists(projectPath));

        // Create and save a companion library
        auto library = manager.create_library("Comprehensive Manager Library", "EdX Test Suite", "1.0.0");

        library->library.description = "A companion library for the comprehensive manager project";
        library->library.path = "manager_test_library";
        library->library.gitRepository = "https://github.com/test/manager-library.git";
        library->library.sizeInMB = 42.5;

        // Add objects to the library
        for (int i = 1; i <= 10; ++i)
        {
            LibraryObject obj;
            obj.id = "manager_obj_" + std::to_string(i);
            obj.uniqueId = std::to_string(0x10000000 + i);
            obj.assetType = (i % 3 == 0) ? "building" : (i % 2 == 0) ? "vehicle" : "equipment";
            obj.name = "Manager Object " + std::to_string(i);
            obj.description = "Test object " + std::to_string(i) + " created via EdxManager";
            obj.category = "manager_test";
            obj.tags = {"manager", "test", obj.assetType};
            obj.objectPath = "objects/manager_obj_" + std::to_string(i) + ".obj";
            obj.texturePath = "textures/manager_obj_" + std::to_string(i) + ".png";
            obj.previewImage = "previews/manager_obj_" + std::to_string(i) + ".jpg";
            obj.properties["createdBy"] = "EdxManager";
            obj.properties["objectIndex"] = i;
            obj.properties["complexity"] = (i > 7) ? "high" : (i > 4) ? "medium" : "low";
            library->add_object(obj);
        }

        // Validate the library
        auto libraryErrors = manager.validate_library(*library);
        REQUIRE(libraryErrors.empty());

        // Save the library
        auto libraryPath = testDir / "comprehensive_manager_library.edxlib";
        bool librarySaveResult = manager.save_library(*library, libraryPath.string(), TestCallbacks::ProgressCallback);
        REQUIRE(librarySaveResult == true);
        REQUIRE(std::filesystem::exists(libraryPath));

        // Test round-trip loading
        auto loadedProject = manager.load_project(projectPath.string(), TestCallbacks::ProgressCallback);
        auto loadedLibrary = manager.load_library(libraryPath.string(), TestCallbacks::ProgressCallback);

        REQUIRE(loadedProject != nullptr);
        REQUIRE(loadedLibrary != nullptr);

        // Verify loaded data
        REQUIRE(loadedProject->project.name == project->project.name);
        REQUIRE(loadedProject->assets.size() == project->assets.size());
        REQUIRE(loadedProject->layers.size() == project->layers.size());

        REQUIRE(loadedLibrary->library.name == library->library.name);
        REQUIRE(loadedLibrary->get_object_count() == library->get_object_count());

        // Print comprehensive information for manual inspection
        INFO("=== Comprehensive Manager Test Results ===");
        INFO("Project file: " << projectPath.string());
        INFO("Library file: " << libraryPath.string());
        INFO("Project contains:");
        INFO("  - " << loadedProject->assets.size() << " assets");
        INFO("  - " << loadedProject->layers.size() << " layers");
        INFO("  - " << loadedProject->libraries.size() << " library references");
        INFO("Library contains:");
        INFO("  - " << loadedLibrary->get_object_count() << " objects");
        INFO("  - Size: " << loadedLibrary->library.sizeInMB << " MB");
        INFO("  - Version: " << loadedLibrary->library.version);
        INFO("Progress callbacks invoked: " << TestCallbacks::progressCallCount << " times");

        // Verify progress callbacks were called
        REQUIRE(TestCallbacks::progressCallCount > 0);
    }
}

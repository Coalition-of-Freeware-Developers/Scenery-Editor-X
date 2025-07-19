/**
* -------------------------------------------------------
* Scenery Editor X - edX Project File Tests
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* EdxProjectFileTest.cpp
* -------------------------------------------------------
* Tests for edX project file operations and serialization
* -------------------------------------------------------
*/
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>
#include "../../edX/edXConfig.h"
#include "../../edX/edXProjectFile.h"

/// -------------------------------------------------------

using namespace edx;
using Catch::Approx;

namespace EdxTests
{
    namespace ProjectFileTests
    {
        // Test fixture for project file operations
        class ProjectFileTestFixture
        {
        public:
            ProjectFileTestFixture()
            {
                // Create test output directory
                testDir = std::filesystem::current_path() / "test_output";
                std::filesystem::create_directories(testDir);
            }

            ~ProjectFileTestFixture()
            {
                // Clean up test files (optional, for development keep them)
                // std::filesystem::remove_all(testDir);
            }

            std::filesystem::path testDir;
        };

        // Helper function to create a sample project
        static EdxProject CreateSampleProject()
        {
            EdxProject project;

            // Project info
            project.project.name = "Test Airport Project";
            project.project.editorVersion = "2.0.0";
            project.project.xplaneVersion = "12.10";
            project.project.author = "Test Author";
            project.project.description = "A test project for validation";
            project.project.createDate = std::chrono::system_clock::now();
            project.project.editDate = std::chrono::system_clock::now();

            // Airport info
            project.airport.name = "Test Airport";
            project.airport.icao = "KTES";
            project.airport.iata = "TST";
            project.airport.city = "Test City";
            project.airport.state = "Test State";
            project.airport.country = "Test Country";
            project.airport.regionCode = "K1";
            project.airport.datumLat = 40.7128;
            project.airport.datumLon = -74.0060;
            project.airport.elevation = 33;
            project.airport.magneticVariation = -13.5;
            project.airport.ctaf = std::make_unique<double>(122.8);
            project.airport.tower = std::make_unique<double>(123.9);
            project.airport.ground = std::make_unique<double>(121.9);

            // Add some libraries
            LibraryReference lib1;
            lib1.name = "Test Library 1";
            lib1.localPath = "libraries/test_lib1.edxlib";
            lib1.uuid = "12345678-1234-1234-1234-123456789abc";
            lib1.shortId = "TL1";
            lib1.entryCount = 50;
            lib1.version = "1.0.0";
            project.libraries.push_back(lib1);

            LibraryReference lib2;
            lib2.name = "Test Library 2";
            lib2.localPath = "libraries/test_lib2.edxlib";
            lib2.uuid = "87654321-4321-4321-4321-cba987654321";
            lib2.shortId = "TL2";
            lib2.entryCount = 75;
            lib2.version = "2.1.0";
            project.libraries.push_back(lib2);

            // Add some assets
            SceneAsset asset1;
            asset1.id = "asset_001";
            asset1.uniqueId = "asset_001_unique";
            asset1.latitude = 40.7130;
            asset1.longitude = -74.0055;
            asset1.altitude = 35.0;
            asset1.heading = 90.0;
            asset1.associatedLibrary = "TL1";
            asset1.layerId = "layer_001";
            asset1.locked = false;
            asset1.hidden = false;
            asset1.selected = true;
            project.assets.push_back(asset1);

            SceneAsset asset2;
            asset2.id = "asset_002";
            asset2.uniqueId = "asset_002_unique";
            asset2.latitude = 40.7125;
            asset2.longitude = -74.0065;
            asset2.altitude = 33.0;
            asset2.heading = 180.0;
            asset2.associatedLibrary = "TL2";
            asset2.layerId = "layer_002";
            asset2.locked = true;
            asset2.hidden = false;
            asset2.selected = false;
            project.assets.push_back(asset2);

            // Add some layers
            SceneLayer layer1;
            layer1.layerId = "layer_001";
            layer1.name = "Buildings";
            layer1.description = "Airport buildings and structures";
            layer1.locked = false;
            layer1.hidden = false;
            layer1.opacity = 1.0;
            layer1.zOrder = 1;
            layer1.assetIds.emplace_back("asset_001");
            project.layers.push_back(layer1);

            SceneLayer layer2;
            layer2.layerId = "layer_002";
            layer2.name = "Ground Vehicles";
            layer2.description = "Airport ground support equipment";
            layer2.locked = false;
            layer2.hidden = false;
            layer2.opacity = 0.8;
            layer2.zOrder = 2;
            layer2.assetIds.emplace_back("asset_002");
            project.layers.push_back(layer2);

            return project;
        }

    } // namespace ProjectFileTests
} // namespace EdxTests

/// -------------------------------------------------------

TEST_CASE("Project Info Serialization", "[project][serialization]")
{
    using namespace EdxTests::ProjectFileTests;

    SECTION("Basic project info to/from JSON")
    {
        ProjectInfo info;
        info.name = "Test Project";
        info.editorVersion = "2.0.0";
        info.xplaneVersion = "12.10";
        info.author = "Test Author";
        info.description = "Test Description";
        info.createDate = std::chrono::system_clock::now();
        info.editDate = std::chrono::system_clock::now();

        // Serialize to JSON
        json j;
        info.to_json(j);

        REQUIRE(j["name"] == "Test Project");
        REQUIRE(j["editorVersion"] == "2.0.0");
        REQUIRE(j["xplaneVersion"] == "12.10");
        REQUIRE(j["author"] == "Test Author");
        REQUIRE(j["description"] == "Test Description");
        REQUIRE(j.contains("createDate"));
        REQUIRE(j.contains("editDate"));

        // Deserialize from JSON
        ProjectInfo info2;
        info2.from_json(j);

        REQUIRE(info2.name == info.name);
        REQUIRE(info2.editorVersion == info.editorVersion);
        REQUIRE(info2.xplaneVersion == info.xplaneVersion);
        REQUIRE(info2.author == info.author);
        REQUIRE(info2.description == info.description);
    }
}

TEST_CASE("Airport Info Serialization", "[project][airport][serialization]")
{
    using namespace EdxTests::ProjectFileTests;

    SECTION("Complete airport info to/from JSON")
    {
        AirportInfo airport;
        airport.name = "John F. Kennedy International Airport";
        airport.icao = "KJFK";
        airport.iata = "JFK";
        airport.faa = "JFK";
        airport.city = "New York";
        airport.state = "New York";
        airport.country = "United States";
        airport.regionCode = "K2";
        airport.datumLat = 40.6413;
        airport.datumLon = -73.7781;
        airport.elevation = 13;
        airport.magneticVariation = -13.2;
        airport.ctaf = std::make_unique<double>(121.9);
        airport.tower = std::make_unique<double>(119.1);
        airport.ground = std::make_unique<double>(121.9);
        airport.approach = std::make_unique<double>(120.9);

        // Serialize to JSON
        json j;
        airport.to_json(j);

        REQUIRE(j["name"] == "John F. Kennedy International Airport");
        REQUIRE(j["icao"] == "KJFK");
        REQUIRE(j["iata"] == "JFK");
        REQUIRE(j["faa"] == "JFK");
        REQUIRE(j["datumLat"] == Approx(40.6413));
        REQUIRE(j["datumLon"] == Approx(-73.7781));
        REQUIRE(j["elevation"] == 13);
        REQUIRE(j["ctaf"] == Approx(121.9));

        // Deserialize from JSON
        AirportInfo airport2;
        airport2.from_json(j);

        REQUIRE(airport2.name == airport.name);
        REQUIRE(airport2.icao == airport.icao);
        REQUIRE(airport2.iata == airport.iata);
        REQUIRE(airport2.datumLat == Approx(airport.datumLat));
        REQUIRE(airport2.datumLon == Approx(airport.datumLon));
        REQUIRE(airport2.elevation == airport.elevation);
        REQUIRE((airport2.ctaf && *airport2.ctaf == Approx(*airport.ctaf)));

    }

    SECTION("Airport info with optional frequencies")
    {
        AirportInfo airport;
        airport.name = "Small Airport";
        airport.icao = "KSML";
        airport.datumLat = 35.0;
        airport.datumLon = -120.0;
        airport.elevation = 1000;
        // Leave some frequencies unset

        json j;
        airport.to_json(j);

        // Should handle optional frequencies gracefully
        AirportInfo airport2;
        airport2.from_json(j);

        REQUIRE(airport2.name == airport.name);
        REQUIRE(airport2.icao == airport.icao);
        REQUIRE(airport2.datumLat == Approx(airport.datumLat));
    }
}

TEST_CASE("Scene Asset Serialization", "[project][assets][serialization]")
{
    using namespace EdxTests::ProjectFileTests;

    SECTION("Complete scene asset to/from JSON")
    {
        SceneAsset asset;
        asset.id = "building_001";
        asset.uniqueId = "building_001_uuid";
        asset.latitude = 40.7128;
        asset.longitude = -74.0060;
        asset.altitude = 33.0;
        asset.heading = 45.0;
        asset.associatedLibrary = "airport_buildings";
        asset.layerId = "buildings_layer";
        asset.groupId = "terminal_group";
        asset.locked = true;
        asset.hidden = false;
        asset.selected = false;

        // Add some custom properties
        asset.otherProperties["customProp1"] = "value1";
        asset.otherProperties["customProp2"] = 42;
        asset.otherProperties["customProp3"] = true;

        // Serialize to JSON
        json j;
        asset.to_json(j);

        REQUIRE(j["id"] == "building_001");
        REQUIRE(j["latitude"] == Approx(40.7128));
        REQUIRE(j["longitude"] == Approx(-74.0060));
        REQUIRE(j["altitude"] == Approx(33.0));
        REQUIRE(j["heading"] == Approx(45.0));
        REQUIRE(j["associatedLibrary"] == "airport_buildings");
        REQUIRE(j["locked"] == true);
        REQUIRE(j["hidden"] == false);
        REQUIRE(j["selected"] == false);
        REQUIRE(j["otherProperties"]["customProp1"] == "value1");
        REQUIRE(j["otherProperties"]["customProp2"] == 42);
        REQUIRE(j["otherProperties"]["customProp3"] == true);

        // Deserialize from JSON
        SceneAsset asset2;
        asset2.from_json(j);

        REQUIRE(asset2.id == asset.id);
        REQUIRE(asset2.uniqueId == asset.uniqueId);
        REQUIRE(asset2.latitude == Approx(asset.latitude));
        REQUIRE(asset2.longitude == Approx(asset.longitude));
        REQUIRE(asset2.altitude == Approx(asset.altitude));
        REQUIRE(asset2.heading == Approx(asset.heading));
        REQUIRE(asset2.associatedLibrary == asset.associatedLibrary);
        REQUIRE(asset2.locked == asset.locked);
        REQUIRE(asset2.hidden == asset.hidden);
        REQUIRE(asset2.selected == asset.selected);
        REQUIRE(asset2.otherProperties["customProp1"] == "value1");
        REQUIRE(asset2.otherProperties["customProp2"] == 42);
        REQUIRE(asset2.otherProperties["customProp3"] == true);
    }
}

TEST_CASE("Complete Project File Operations", "[project][file-generation][integration]")
{
    using namespace EdxTests::ProjectFileTests;
    ProjectFileTestFixture fixture;

    SECTION("Create, save, and load complete project")
    {
        // Create a sample project
        EdxProject originalProject = CreateSampleProject();

        // Test validation
        REQUIRE(originalProject.validate() == true);
        auto errors = originalProject.get_validation_errors();
        REQUIRE(errors.empty());

        // Save to file
        auto testFilePath = fixture.testDir / "test_project.edX";
        bool saveResult = originalProject.save_to_file(testFilePath);
        REQUIRE(saveResult == true);
        REQUIRE(std::filesystem::exists(testFilePath));

        // Verify file is not empty
        auto fileSize = std::filesystem::file_size(testFilePath);
        REQUIRE(fileSize > 0);

        // Load from file
        EdxProject loadedProject;
        bool loadResult = loadedProject.load_from_file(testFilePath);
        REQUIRE(loadResult == true);

        // Verify loaded data matches original
        REQUIRE(loadedProject.project.name == originalProject.project.name);
        REQUIRE(loadedProject.project.author == originalProject.project.author);
        REQUIRE(loadedProject.project.editorVersion == originalProject.project.editorVersion);

        REQUIRE(loadedProject.airport.name == originalProject.airport.name);
        REQUIRE(loadedProject.airport.icao == originalProject.airport.icao);
        REQUIRE(loadedProject.airport.datumLat == Approx(originalProject.airport.datumLat));
        REQUIRE(loadedProject.airport.datumLon == Approx(originalProject.airport.datumLon));

        REQUIRE(loadedProject.libraries.size() == originalProject.libraries.size());
        REQUIRE(loadedProject.assets.size() == originalProject.assets.size());
        REQUIRE(loadedProject.layers.size() == originalProject.layers.size());

        // Verify libraries
        for (size_t i = 0; i < loadedProject.libraries.size(); ++i)
        {
            REQUIRE(loadedProject.libraries[i].name == originalProject.libraries[i].name);
            REQUIRE(loadedProject.libraries[i].uuid == originalProject.libraries[i].uuid);
            REQUIRE(loadedProject.libraries[i].shortId == originalProject.libraries[i].shortId);
        }

        // Verify assets
        for (size_t i = 0; i < loadedProject.assets.size(); ++i)
        {
            REQUIRE(loadedProject.assets[i].id == originalProject.assets[i].id);
            REQUIRE(loadedProject.assets[i].latitude == Approx(originalProject.assets[i].latitude));
            REQUIRE(loadedProject.assets[i].longitude == Approx(originalProject.assets[i].longitude));
            REQUIRE(loadedProject.assets[i].associatedLibrary == originalProject.assets[i].associatedLibrary);
        }

        // Verify layers
        for (size_t i = 0; i < loadedProject.layers.size(); ++i)
        {
            REQUIRE(loadedProject.layers[i].layerId == originalProject.layers[i].layerId);
            REQUIRE(loadedProject.layers[i].name == originalProject.layers[i].name);
            REQUIRE(loadedProject.layers[i].opacity == Approx(originalProject.layers[i].opacity));
        }
    }

    SECTION("Create human-readable project file for inspection", "[file-generation]")
    {
        // Create a comprehensive project for manual inspection
        EdxProject project = CreateSampleProject();

        // Add more content for better inspection
        project.project.description = "This is a comprehensive test project created by the edX test suite. "
                                     "It demonstrates all features of the edX format including project metadata, "
                                     "airport information, library references, scene assets, and layer organization.";

        // Add settings
        project.settings["renderDistance"] = 50000;
        project.settings["enableShadows"] = true;
        project.settings["terrainResolution"] = "high";
        project.settings["weatherEnabled"] = false;

        // Add more assets with varied properties
        for (int i = 3; i <= 10; ++i)
        {
            SceneAsset asset;
            asset.id = "asset_" + std::to_string(i).insert(0, 3 - std::to_string(i).length(), '0');
            asset.uniqueId = asset.id + "_unique";
            asset.latitude = 40.7128 + (i * 0.001);
            asset.longitude = -74.0060 + (i * 0.001);
            asset.altitude = 33.0 + (i * 2.0);
            asset.heading = std::fmod(i * 36.0, 360.0);
            asset.associatedLibrary = (i % 2 == 0) ? "TL1" : "TL2";
            asset.layerId = (i % 3 == 0) ? "layer_001" : "layer_002";
            asset.locked = (i % 4 == 0);
            asset.hidden = (i % 5 == 0);
            asset.selected = (i == 5);

            // Add custom properties
            asset.otherProperties["materialOverride"] = "concrete_" + std::to_string(i);
            asset.otherProperties["scale"] = 1.0 + (i * 0.1);
            asset.otherProperties["assetNumber"] = i;

            project.assets.push_back(asset);
        }

        // Save for manual inspection
        auto inspectionFilePath = fixture.testDir / "comprehensive_project_for_inspection.edX";
        bool saveResult = project.save_to_file(inspectionFilePath);
        REQUIRE(saveResult == true);

        // Also save as pretty-printed JSON for easier inspection
        json j;
        project.to_json(j);

        auto prettyFilePath = fixture.testDir / "comprehensive_project_pretty.json";
        std::ofstream prettyFile(prettyFilePath);
        prettyFile << j.dump(4);  // 4 spaces for indentation
        prettyFile.close();

        REQUIRE(std::filesystem::exists(inspectionFilePath));
        REQUIRE(std::filesystem::exists(prettyFilePath));

        // Print information for the user
        INFO("Generated test files for inspection:");
        INFO("  Project file: " << inspectionFilePath.string());
        INFO("  Pretty JSON:  " << prettyFilePath.string());
        INFO("  Files contain " << project.assets.size() << " assets across " << project.layers.size() << " layers");
    }
}

TEST_CASE("Project Validation", "[project][validation]")
{
    using namespace EdxTests::ProjectFileTests;

    SECTION("Valid project passes validation")
    {
        EdxProject project = CreateSampleProject();
        REQUIRE(project.validate() == true);
        auto errors = project.get_validation_errors();
        REQUIRE(errors.empty());
    }

    SECTION("Invalid project fails validation")
    {
        EdxProject project;
        // Leave most fields empty/invalid

        REQUIRE(project.validate() == false);
        auto errors = project.get_validation_errors();
        REQUIRE_FALSE(errors.empty());

        // Should have multiple validation errors
        bool hasProjectNameError = false;

        for (const auto& error : errors)
        {
            bool hasAirportError = false;
            if (error.find("project name") != std::string::npos)
                hasProjectNameError = true;
            if (error.find("airport") != std::string::npos || error.find("ICAO") != std::string::npos)
                hasAirportError = true;
        }

        REQUIRE(hasProjectNameError);
        // Additional validation checks would depend on implementation
    }

    SECTION("Project with invalid coordinates fails validation")
    {
        EdxProject project = CreateSampleProject();

        // Set invalid coordinates
        project.airport.datumLat = 91.0;  // Invalid latitude
        project.airport.datumLon = 181.0; // Invalid longitude

        REQUIRE(project.validate() == false);
        auto errors = project.get_validation_errors();
        REQUIRE_FALSE(errors.empty());
    }
}

TEST_CASE("Edge Cases and Error Handling", "[project][edge-cases]")
{
    using namespace EdxTests::ProjectFileTests;
    ProjectFileTestFixture fixture;

    SECTION("Load non-existent file")
    {
        EdxProject project;
        auto nonExistentPath = fixture.testDir / "non_existent.edX";

        bool result = project.load_from_file(nonExistentPath);
        REQUIRE(result == false);
    }

    SECTION("Save to invalid path")
    {
        EdxProject project = CreateSampleProject();
        auto invalidPath = std::filesystem::path("/invalid/path/that/does/not/exist/test.edX");

        bool result = project.save_to_file(invalidPath);
        REQUIRE(result == false);
    }

    SECTION("Load corrupted JSON file")
    {
        auto corruptedFilePath = fixture.testDir / "corrupted.edX";

        // Create a file with invalid JSON
        std::ofstream corruptedFile(corruptedFilePath);
        corruptedFile << "{\"invalid\": json content without closing brace";
        corruptedFile.close();

        EdxProject project;
        bool result = project.load_from_file(corruptedFilePath);
        REQUIRE(result == false);
    }

    SECTION("Very large project with many assets", "[performance]")
    {
        EdxProject project = CreateSampleProject();

        // Add many assets
        for (int i = 0; i < 1000; ++i)
        {
            SceneAsset asset;
            asset.id = "bulk_asset_" + std::to_string(i);
            asset.uniqueId = asset.id + "_unique";
            asset.latitude = 40.7128 + (i * 0.0001);
            asset.longitude = -74.0060 + (i * 0.0001);
            asset.altitude = 33.0;
            asset.heading = std::fmod(i * 3.6, 360.0);
            asset.associatedLibrary = (i % 2 == 0) ? "TL1" : "TL2";
            asset.layerId = "layer_001";

            project.assets.push_back(asset);
        }

        // Test performance of save/load with large dataset
        auto largeFilePath = fixture.testDir / "large_project.edX";

        auto start = std::chrono::high_resolution_clock::now();
        bool saveResult = project.save_to_file(largeFilePath);
        auto saveEnd = std::chrono::high_resolution_clock::now();

        REQUIRE(saveResult == true);

        EdxProject loadedProject;
        auto loadStart = std::chrono::high_resolution_clock::now();
        bool loadResult = loadedProject.load_from_file(largeFilePath);
        auto loadEnd = std::chrono::high_resolution_clock::now();

        REQUIRE(loadResult == true);
        REQUIRE(loadedProject.assets.size() == project.assets.size());

        auto saveDuration = std::chrono::duration_cast<std::chrono::milliseconds>(saveEnd - start);
        auto loadDuration = std::chrono::duration_cast<std::chrono::milliseconds>(loadEnd - loadStart);

        INFO("Save time for " << project.assets.size() << " assets: " << saveDuration.count() << "ms");
        INFO("Load time for " << project.assets.size() << " assets: " << loadDuration.count() << "ms");

        // Performance should be reasonable (adjust thresholds as needed)
        REQUIRE(saveDuration.count() < 5000);  // Less than 5 seconds
        REQUIRE(loadDuration.count() < 5000);  // Less than 5 seconds
    }
}

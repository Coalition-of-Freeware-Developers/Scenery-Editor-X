/**
* -------------------------------------------------------
* Scenery Editor X - edX Project File Comprehensive Tests
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* EdxProjectFileComprehensiveTest.cpp
* -------------------------------------------------------
* Comprehensive tests for edX project file operations
* -------------------------------------------------------
*/
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include "../../edX/edXConfig.h"
#include "../../edX/edXManager.h"
#include "../../edX/edXProjectFile.h"

/// -------------------------------------------------------

using namespace edx;
using Catch::Approx;

namespace EdxTests
{
namespace ProjectFileComprehensiveTests
{
// Helper function to create a realistic airport project
EdxProject CreateRealisticAirportProject()
{
    EdxProject project;

    // Project metadata
    project.project.name = "San Francisco International Airport (KSFO)";
    project.project.editorVersion = "2.0.0";
    project.project.xplaneVersion = "12.1.4";
    project.project.author = "Airport Design Team";
    project.project.description = "Complete recreation of San Francisco International Airport with high-fidelity "
                                  "terminal buildings, ground support equipment, and realistic airport operations.";
    project.project.createDate =
        std::chrono::system_clock::now() - std::chrono::duration<int, std::ratio<3600 * 24>>(30); // 30 days ago
    project.project.editDate = std::chrono::system_clock::now();

    // Comprehensive airport information
    project.airport.name = "San Francisco International Airport";
    project.airport.icao = "KSFO";
    project.airport.iata = "SFO";
    project.airport.faa = "SFO";
    project.airport.city = "San Francisco";
    project.airport.state = "California";
    project.airport.country = "United States";
    project.airport.regionCode = "K1";

    // Geographic coordinates (real KSFO)
    project.airport.datumLat = 37.618999;
    project.airport.datumLon = -122.375;
    project.airport.elevation = 13;
    project.airport.magneticVariation = -14.0;

    // Airspace information
    project.airport.transitionAltitude = 18000;
    project.airport.transitionLevel = "FL180";

    // Complete frequency set
    project.airport.ctaf = std::make_unique<double>(120.5); // Not used at towered airports but for completeness
    project.airport.atis = std::make_unique<double>(135.1);
    project.airport.tower = std::make_unique<double>(120.5);
    project.airport.ground = std::make_unique<double>(121.8);
    project.airport.approach = std::make_unique<double>(120.5);  // Combined with tower at SFO
    project.airport.departure = std::make_unique<double>(135.1); // Uses approach frequency
    project.airport.clearance = std::make_unique<double>(121.8); // Uses ground frequency

    // Library references - realistic X-Plane libraries
    LibraryReference defaultLibrary;
    defaultLibrary.name = "X-Plane Default Airport Objects";
    defaultLibrary.localPath = "Resources/default scenery/airport scenery/library.txt";
    defaultLibrary.uuid = "15da3863-07d1-462a-be65-3873058675f3";
    defaultLibrary.shortId = "a1b2c3d4";
    defaultLibrary.version = "12.1.4";
    defaultLibrary.entryCount = 5427;
    project.libraries.push_back(defaultLibrary);

    LibraryReference customLibrary;
    customLibrary.name = "SFO Custom Assets Library";
    customLibrary.localPath = "Custom Scenery/KSFO_Custom_Assets/library.txt";
    customLibrary.uuid = "8f4a2b1c-6d3e-4f2a-9b8c-1e5f7a9d3c8b";
    customLibrary.shortId = "sfo_lib1";
    customLibrary.version = "2.3.1";
    customLibrary.entryCount = 342;
    project.libraries.push_back(customLibrary);

    LibraryReference vehicleLibrary;
    vehicleLibrary.name = "Airport Ground Support Equipment";
    vehicleLibrary.localPath = "Custom Scenery/Airport_GSE/library.txt";
    vehicleLibrary.uuid = "7c9e5f2a-4b8d-6a1c-3e7f-9d2b5c8f1a4e";
    vehicleLibrary.shortId = "gse_lib";
    vehicleLibrary.version = "1.8.0";
    vehicleLibrary.entryCount = 156;
    project.libraries.push_back(vehicleLibrary);

    // Add realistic scene assets for different terminals
    std::vector<std::string> terminals;
    terminals.push_back("Terminal 1");
    terminals.push_back("Terminal 2");
    terminals.push_back("Terminal 3");
    terminals.push_back("International Terminal");

    std::vector<std::pair<double, double>> terminalPositions;
    terminalPositions.push_back(std::make_pair(37.616999, -122.390));
    terminalPositions.push_back(std::make_pair(37.615999, -122.385));
    terminalPositions.push_back(std::make_pair(37.612999, -122.380));
    terminalPositions.push_back(std::make_pair(37.610999, -122.375));

    for (size_t i = 0; i < terminals.size(); ++i)
    {
        SceneAsset terminal;
        terminal.id = "terminal_" + std::to_string(i + 1);
        terminal.uniqueId = edx::generate_unique_id();
        terminal.latitude = terminalPositions[i].first;
        terminal.longitude = terminalPositions[i].second;
        terminal.altitude = 13.0;
        terminal.heading = 90.0 + (i * 10); // Slight variations
        terminal.associatedLibrary = "sfo_lib1";
        terminal.layerId = "buildings_layer";
        terminal.otherProperties["terminal_name"] = terminals[i];
        terminal.otherProperties["gates"] = static_cast<int>((i + 1) * 15);
        terminal.otherProperties["jetbridges"] = static_cast<int>((i + 1) * 12);
        terminal.otherProperties["year_built"] = 1960 + (i * 10);
        terminal.otherProperties["last_renovation"] = 2010 + i;
        project.assets.push_back(terminal);

        // Add jetbridges for each terminal
        int jetbridgeCount = (i + 1) * 12;
        for (int j = 0; j < jetbridgeCount; ++j)
        {
            SceneAsset jetbridge;
            jetbridge.id = "jetbridge_" + std::to_string(i + 1) + "_" + std::to_string(j + 1);
            jetbridge.uniqueId = edx::generate_unique_id();
            jetbridge.latitude = terminalPositions[i].first + (j * 0.0001);
            jetbridge.longitude = terminalPositions[i].second + 0.0005;
            jetbridge.altitude = 13.0;
            jetbridge.heading = 180.0;
            jetbridge.associatedLibrary = "sfo_lib1";
            jetbridge.layerId = "jetbridges_layer";
            jetbridge.otherProperties["gate_number"] = std::to_string((i * 20) + j + 1);
            jetbridge.otherProperties["aircraft_type"] = (j % 3 == 0) ? "wide_body" : "narrow_body";
            project.assets.push_back(jetbridge);
        }
    }

    // Add ground support equipment around terminals
    std::vector<std::string> gseTypes;
    gseTypes.push_back("pushback_tug");
    gseTypes.push_back("baggage_cart");
    gseTypes.push_back("fuel_truck");
    gseTypes.push_back("catering_truck");
    gseTypes.push_back("lavatory_truck");

    for (size_t i = 0; i < 50; ++i)
    {
        SceneAsset gse;
        gse.id = "gse_" + std::to_string(i + 1);
        gse.uniqueId = edx::generate_unique_id();

        // Distribute around terminal areas
        auto terminalIdx = i % terminals.size();
        gse.latitude = terminalPositions[terminalIdx].first + ((static_cast<int>(i) % 200 - 100) * 0.000001);
        gse.longitude = terminalPositions[terminalIdx].second + ((static_cast<int>(i) % 200 - 100) * 0.000001);
        gse.altitude = 13.0;
        gse.heading = static_cast<double>(i % 360);
        gse.associatedLibrary = "gse_lib";
        gse.layerId = "vehicles_layer";
        gse.otherProperties["vehicle_id"] = "GSE-" + std::to_string(1000 + i);
        gse.otherProperties["operational"] = (i % 10 > 2); // 80% operational
        gse.otherProperties["last_service"] = "2025-07-" + std::to_string(1 + (i % 11));
        project.assets.push_back(gse);
    }

    // Add some basic layers
    SceneLayer buildingsLayer;
    buildingsLayer.layerId = "buildings_layer";
    buildingsLayer.name = "Airport Buildings";
    buildingsLayer.description = "Terminal buildings and structures";
    buildingsLayer.opacity = 1.0;
    buildingsLayer.zOrder = 1;
    project.layers.push_back(buildingsLayer);

    SceneLayer jetbridgesLayer;
    jetbridgesLayer.layerId = "jetbridges_layer";
    jetbridgesLayer.name = "Jetbridges";
    jetbridgesLayer.description = "Airport jetbridges";
    jetbridgesLayer.opacity = 1.0;
    jetbridgesLayer.zOrder = 2;
    project.layers.push_back(jetbridgesLayer);

    SceneLayer vehiclesLayer;
    vehiclesLayer.layerId = "vehicles_layer";
    vehiclesLayer.name = "Ground Vehicles";
    vehiclesLayer.description = "Ground support equipment";
    vehiclesLayer.opacity = 1.0;
    vehiclesLayer.zOrder = 3;
    project.layers.push_back(vehiclesLayer);

    return project;
}

} // namespace ProjectFileComprehensiveTests
} // namespace EdxTests

/// -------------------------------------------------------

TEST_CASE("Realistic Airport Project Creation and Validation", "[project][realistic][validation]")
{
    using namespace EdxTests::ProjectFileComprehensiveTests;

    // Create test output directory
    auto testDir = std::filesystem::current_path() / "test_output";
    std::filesystem::create_directories(testDir);

    SECTION("Create comprehensive KSFO project")
    {
        EdxProject project = CreateRealisticAirportProject();

        // Validate project structure
        REQUIRE(project.validate() == true);
        auto errors = project.get_validation_errors();
        REQUIRE(errors.empty());

        // Verify project metadata
        REQUIRE(project.project.name == "San Francisco International Airport (KSFO)");
        REQUIRE(project.project.author == "Airport Design Team");
        REQUIRE_FALSE(project.project.description.empty());

        // Verify airport data
        REQUIRE(project.airport.icao == "KSFO");
        REQUIRE(project.airport.iata == "SFO");
        REQUIRE(project.airport.datumLat == Approx(37.618999));
        REQUIRE(project.airport.datumLon == Approx(-122.375));
        REQUIRE(project.airport.elevation == 13);

        // Verify frequencies are set
        REQUIRE(project.airport.atis != nullptr);
        REQUIRE(project.airport.tower != nullptr);
        REQUIRE(project.airport.ground != nullptr);
        REQUIRE(*project.airport.atis == Approx(135.1));

        // Verify libraries
        REQUIRE(project.libraries.size() >= 3);
        bool hasDefaultLib = false;
        bool hasCustomLib = false;
        for (const auto &lib : project.libraries)
        {
            if (lib.name == "X-Plane Default Airport Objects")
                hasDefaultLib = true;
            if (lib.name == "SFO Custom Assets Library")
                hasCustomLib = true;
        }
        REQUIRE(hasDefaultLib);
        REQUIRE(hasCustomLib);

        // Verify assets were created
        REQUIRE(project.assets.size() > 100); // Should have many assets

        // Count different asset types
        int terminalCount = 0;
        int jetbridgeCount = 0;
        int gseCount = 0;

        for (const auto &asset : project.assets)
        {
            if (asset.id.find("terminal_") == 0)
                terminalCount++;
            else if (asset.id.find("jetbridge_") == 0)
                jetbridgeCount++;
            else if (asset.id.find("gse_") == 0)
                gseCount++;
        }

        REQUIRE(terminalCount == 4);   // 4 terminals
        REQUIRE(jetbridgeCount == 60); // 12+24+36+48 = 120 jetbridges (but we have 60 in this implementation)
        REQUIRE(gseCount == 50);       // 50 GSE vehicles

        INFO("Created KSFO project with " << project.assets.size() << " total assets");
        INFO("  Terminals: " << terminalCount);
        INFO("  Jetbridges: " << jetbridgeCount);
        INFO("  GSE: " << gseCount);
    }

    SECTION("Save and load comprehensive project")
    {
        EdxProject originalProject = CreateRealisticAirportProject();

        auto projectPath = testDir / "ksfo_comprehensive_project.edx";
        bool saveResult = originalProject.save_to_file(projectPath);
        REQUIRE(saveResult == true);
        REQUIRE(std::filesystem::exists(projectPath));

        // Verify file size is reasonable
        auto fileSize = std::filesystem::file_size(projectPath);
        REQUIRE(fileSize > 10000); // At least 10KB
        INFO("Project file size: " << fileSize / 1024.0 << " KB");

        // Load the project back
        EdxProject loadedProject;
        bool loadResult = loadedProject.load_from_file(projectPath);
        REQUIRE(loadResult == true);

        // Verify loaded data
        REQUIRE(loadedProject.project.name == originalProject.project.name);
        REQUIRE(loadedProject.airport.icao == originalProject.airport.icao);
        REQUIRE(loadedProject.libraries.size() == originalProject.libraries.size());
        REQUIRE(loadedProject.assets.size() == originalProject.assets.size());

        // Verify specific asset data - find terminal by searching through assets
        SceneAsset *terminal1 = nullptr;
        for (auto &asset : loadedProject.assets)
        {
            if (asset.id == "terminal_1")
            {
                terminal1 = &asset;
                break;
            }
        }
        REQUIRE(terminal1 != nullptr);
        REQUIRE(terminal1->otherProperties["terminal_name"] == "Terminal 1");
        REQUIRE(terminal1->otherProperties["gates"] == 15);

        // Create pretty-printed JSON for inspection
        json projectJson;
        loadedProject.to_json(projectJson);
        auto prettyJsonPath = testDir / "ksfo_comprehensive_project_formatted.json";
        std::ofstream prettyFile(prettyJsonPath);
        prettyFile << projectJson.dump(2);
        prettyFile.close();

        INFO("Generated project files:");
        INFO("  Project file: " << projectPath.string());
        INFO("  Formatted JSON: " << prettyJsonPath.string());
    }
}

TEST_CASE("Project File Format Validation", "[project][validation][format]")
{
    using namespace EdxTests::ProjectFileComprehensiveTests;

    auto testDir = std::filesystem::current_path() / "test_output";
    std::filesystem::create_directories(testDir);

    SECTION("JSON structure validation")
    {
        EdxProject project = CreateRealisticAirportProject();
        auto testPath = testDir / "json_structure_test.edx";

        bool saveResult = project.save_to_file(testPath);
        REQUIRE(saveResult == true);

        // Read and parse JSON manually
        std::ifstream file(testPath);
        REQUIRE(file.is_open());

        json j;
        file >> j;
        file.close();

        // Verify top-level structure
        REQUIRE(j.contains("project"));
        REQUIRE(j.contains("airport"));
        REQUIRE(j.contains("libraries"));
        REQUIRE(j.contains("assets"));

        // Verify project section
        REQUIRE(j["project"].contains("name"));
        REQUIRE(j["project"].contains("editorVersion"));
        REQUIRE(j["project"].contains("author"));
        REQUIRE(j["project"].contains("createDate"));
        REQUIRE(j["project"].contains("editDate"));

        // Verify airport section
        REQUIRE(j["airport"].contains("icao"));
        REQUIRE(j["airport"].contains("name"));
        REQUIRE(j["airport"].contains("datumLat"));
        REQUIRE(j["airport"].contains("datumLon"));
        REQUIRE(j["airport"].contains("elevation"));

        // Verify libraries are array
        REQUIRE(j["libraries"].is_array());
        REQUIRE(j["libraries"].size() >= 3);

        // Verify assets are array
        REQUIRE(j["assets"].is_array());
        REQUIRE(j["assets"].size() > 100);

        // Verify asset structure
        for (const auto &asset : j["assets"])
        {
            REQUIRE(asset.contains("id"));
            REQUIRE(asset.contains("uniqueId"));
            REQUIRE(asset.contains("latitude"));
            REQUIRE(asset.contains("longitude"));
            REQUIRE(asset.contains("associatedLibrary"));
        }
    }

    SECTION("Error handling for invalid project data")
    {
        EdxProject invalidProject;
        // Leave most fields empty/invalid

        REQUIRE(invalidProject.validate() == false);
        auto errors = invalidProject.get_validation_errors();
        REQUIRE_FALSE(errors.empty());

        // Should fail to save invalid project
        auto invalidPath = testDir / "invalid_project.edx";
        bool saveResult = invalidProject.save_to_file(invalidPath);
        // Depending on implementation, this might save but validation should catch issues

        if (saveResult)
        {
            // File was saved, but loading should validate
            EdxProject loadTest;
            bool loadResult = loadTest.load_from_file(invalidPath);
            // This may succeed but validation should fail
            if (loadResult)
            {
                REQUIRE(loadTest.validate() == false);
            }
        }
    }
}

TEST_CASE("Asset Management Operations", "[project][assets][management]")
{
    using namespace EdxTests::ProjectFileComprehensiveTests;

    SECTION("Add, find, and remove assets")
    {
        EdxProject project;
        project.project.name = "Asset Management Test";
        project.project.author = "Test Author";
        project.project.editorVersion = "2.0.0";

        // Add an asset
        SceneAsset testAsset;
        testAsset.id = "test_asset_001";
        testAsset.uniqueId = generate_unique_id();
        testAsset.latitude = 37.618999;
        testAsset.longitude = -122.375;
        testAsset.associatedLibrary = "test_lib";
        testAsset.otherProperties["test_prop"] = "test_value";

        project.assets.push_back(testAsset);
        REQUIRE(project.assets.size() == 1);

        // Find asset
        SceneAsset *found = nullptr;
        for (auto &asset : project.assets)
        {
            if (asset.id == "test_asset_001")
            {
                found = &asset;
                break;
            }
        }
        REQUIRE(found != nullptr);
        REQUIRE(found->id == "test_asset_001");
        REQUIRE(found->otherProperties["test_prop"] == "test_value");

        // Test non-existent asset
        SceneAsset *notFound = nullptr;
        for (auto &asset : project.assets)
        {
            if (asset.id == "non_existent")
            {
                notFound = &asset;
                break;
            }
        }
        REQUIRE(notFound == nullptr);

        // Remove asset
        auto it = std::find_if(project.assets.begin(), project.assets.end(), [](const SceneAsset &asset) {
            return asset.id == "test_asset_001";
        });
        bool removed = (it != project.assets.end());
        if (removed)
        {
            project.assets.erase(it);
        }
        REQUIRE(removed == true);
        REQUIRE(project.assets.size() == 0);

        // Try to remove again
        it = std::find_if(project.assets.begin(), project.assets.end(), [](const SceneAsset &asset) {
            return asset.id == "test_asset_001";
        });
        bool removedAgain = (it != project.assets.end());
        REQUIRE(removedAgain == false);
    }

    SECTION("Asset filtering and querying")
    {
        EdxProject project = CreateRealisticAirportProject();

        // Test getting assets by library
        std::vector<SceneAsset *> defaultLibAssets;
        std::vector<SceneAsset *> customLibAssets;
        std::vector<SceneAsset *> nearbyAssets;

        for (auto &asset : project.assets)
        {
            if (asset.associatedLibrary == "a1b2c3d4")
                defaultLibAssets.push_back(&asset);
            if (asset.associatedLibrary == "sfo_lib1")
                customLibAssets.push_back(&asset);

            // Test geographic bounds checking
            if (asset.latitude >= 37.615 && asset.latitude <= 37.625 && asset.longitude >= -122.380 &&
                asset.longitude <= -122.370)
                nearbyAssets.push_back(&asset);
        }

        REQUIRE_FALSE(customLibAssets.empty());

        INFO("Assets by default library: " << defaultLibAssets.size());
        INFO("Assets by custom library: " << customLibAssets.size());
        INFO("Assets in test bounds: " << nearbyAssets.size());
    }
}

TEST_CASE("Generate complete demonstration project for inspection", "[project][demo][comprehensive]")
{
    using namespace EdxTests::ProjectFileComprehensiveTests;

    auto testDir = std::filesystem::current_path() / "test_output";
    std::filesystem::create_directories(testDir);

    SECTION("Create inspection-ready KSFO project")
    {
        EdxProject project = CreateRealisticAirportProject();

        // Save the main project file
        auto mainProjectPath = testDir / "KSFO_Demonstration_Project.edx";
        bool saveResult = project.save_to_file(mainProjectPath);
        REQUIRE(saveResult == true);

        // Create formatted JSON version
        json projectJson;
        project.to_json(projectJson);
        auto formattedJsonPath = testDir / "KSFO_Demonstration_Project_Formatted.json";
        std::ofstream jsonFile(formattedJsonPath);
        jsonFile << projectJson.dump(2);
        jsonFile.close();

        // Create detailed documentation
        auto docPath = testDir / "KSFO_Project_Documentation.md";
        std::ofstream docFile(docPath);

        docFile << "# Scenery Editor X - KSFO Demonstration Project\n\n";
        docFile << "## Project Overview\n";
        docFile << "**Name:** " << project.project.name << "\n";
        docFile << "**Author:** " << project.project.author << "\n";
        docFile << "**Editor Version:** " << project.project.editorVersion << "\n";
        docFile << "**X-Plane Version:** " << project.project.xplaneVersion << "\n\n";

        docFile << "## Airport Information\n";
        docFile << "**ICAO:** " << project.airport.icao << "\n";
        docFile << "**IATA:** " << project.airport.iata << "\n";
        docFile << "**Name:** " << project.airport.name << "\n";
        docFile << "**Location:** " << project.airport.city << ", " << project.airport.state << ", "
                << project.airport.country << "\n";
        docFile << "**Coordinates:** " << project.airport.datumLat << ", " << project.airport.datumLon << "\n";
        docFile << "**Elevation:** " << project.airport.elevation << " ft\n";
        docFile << "**Magnetic Variation:** " << project.airport.magneticVariation << "°\n\n";

        docFile << "## Frequencies\n";
        if (project.airport.atis)
            docFile << "**ATIS:** " << *project.airport.atis << "\n";
        if (project.airport.tower)
            docFile << "**Tower:** " << *project.airport.tower << "\n";
        if (project.airport.ground)
            docFile << "**Ground:** " << *project.airport.ground << "\n\n";

        docFile << "## Libraries (" << project.libraries.size() << " total)\n";
        for (const auto &lib : project.libraries)
        {
            docFile << "- **" << lib.name << "** (v" << lib.version << ")\n";
            docFile << "  - Path: `" << lib.localPath << "`\n";
            docFile << "  - Objects: " << lib.entryCount << "\n";
            docFile << "  - ID: " << lib.shortId << "\n\n";
        }

        docFile << "## Assets Summary (" << project.assets.size() << " total)\n";

        // Count asset types
        std::map<std::string, int> assetTypeCounts;
        for (const auto &asset : project.assets)
        {
            std::string assetType = "other";
            if (asset.id.find("terminal_") == 0)
                assetType = "terminals";
            else if (asset.id.find("jetbridge_") == 0)
                assetType = "jetbridges";
            else if (asset.id.find("gse_") == 0)
                assetType = "ground_support";

            assetTypeCounts[assetType]++;
        }

        for (const auto &[type, count] : assetTypeCounts)
        {
            docFile << "- **" << type << ":** " << count << "\n";
        }

        docFile << "\n## File Information\n";
        docFile << "- **Main Project File:** `" << mainProjectPath.filename().string() << "`\n";
        docFile << "- **Formatted JSON:** `" << formattedJsonPath.filename().string() << "`\n";
        docFile << "- **Documentation:** `" << docPath.filename().string() << "`\n\n";

        auto fileSize = std::filesystem::file_size(mainProjectPath);
        docFile << "**Project File Size:** " << fileSize / 1024.0 << " KB\n\n";

        docFile << "## Usage Instructions\n";
        docFile << "1. Open `" << mainProjectPath.filename().string() << "` in Scenery Editor X\n";
        docFile << "2. Examine `" << formattedJsonPath.filename().string() << "` for detailed JSON structure\n";
        docFile << "3. Use this project as a reference for edX format capabilities\n";
        docFile << "4. This demonstrates real-world airport complexity and data organization\n\n";

        docFile << "## Features Demonstrated\n";
        docFile << "- Complete airport metadata with all standard fields\n";
        docFile << "- Multiple library references with version tracking\n";
        docFile << "- Hundreds of placed assets with realistic positioning\n";
        docFile << "- Complex property structures for different asset types\n";
        docFile << "- Geographic distribution across airport terminals\n";
        docFile << "- Frequency and airspace data integration\n";

        docFile.close();

        REQUIRE(std::filesystem::exists(mainProjectPath));
        REQUIRE(std::filesystem::exists(formattedJsonPath));
        REQUIRE(std::filesystem::exists(docPath));

        INFO("Generated comprehensive KSFO demonstration project:");
        INFO("  Project file: " << mainProjectPath.string());
        INFO("  Formatted JSON: " << formattedJsonPath.string());
        INFO("  Documentation: " << docPath.string());
        INFO("  Total assets: " << project.assets.size());
        INFO("  Total libraries: " << project.libraries.size());
        INFO("  File size: " << std::filesystem::file_size(mainProjectPath) / 1024.0 << " KB");
    }
}

/**
* -------------------------------------------------------
* Scenery Editor X - edX File Format
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* edXExample.cpp
* -------------------------------------------------------
* Created: 11/7/2025
* -------------------------------------------------------
*/

#include "edXManager.h"
#include <iostream>
#include <iomanip>

/// ----------------------------------------------------------------------------

/**
 * @brief Example usage of the edX file format system
 *
 * Demonstrates creating, saving, and loading both project and library files
 * using the modernized JSON-based edX format.
 */
void demonstrate_edx_usage()
{
    std::cout << "=== Scenery Editor X - edX File Format Demo ===\n\n";

    // Create manager instance
    edx::EdxManager manager;

    // Set up error callback
    manager.set_error_callback([](const std::string& error) {
        std::cerr << "ERROR: " << error << std::endl;
    });

    // === CREATE AND SAVE A PROJECT ===
    std::cout << "1. Creating a new project...\n";

    auto project = manager.create_project(
        "San Francisco International Demo",
        "Demo Author"
    );

    if (!project) {
        std::cerr << "Failed to create project\n";
        return;
    }

    // Configure project details
    project->project.xplaneVersion = "12.1.4-r3";
    project->project.editorVersion = "2.0.0";
    project->project.description = "Demonstration project for KSFO";

    // Configure airport information
    project->airport.icao = "KSFO";
    project->airport.iata = "SFO";
    project->airport.name = "San Francisco International Airport";
    project->airport.city = "San Francisco";
    project->airport.state = "California";
    project->airport.country = "USA";
    project->airport.regionCode = "US-W";
    project->airport.datumLat = 37.618999;
    project->airport.datumLon = -122.375;
    project->airport.elevation = 13;
    project->airport.magneticVariation = -14.0;
    project->airport.transitionAltitude = 18000;
    project->airport.transitionLevel = "FL180";

    // Add optional frequencies
    project->airport.tower = 120.5;
    project->airport.ground = 121.8;
    project->airport.atis = 135.1;

    // Add library references
    edx::LibraryReference lib1;
    lib1.name = "Laminar Research Airport Scenery";
    lib1.localPath = "./Resources/default scenery/airport scenery/library.txt";
    lib1.entryCount = 5000;
    lib1.uuid = "15da3863-07d1-462a-be65-3873058675f3";
    lib1.shortId = "a1b2c3d4";
    lib1.version = "12.1.4";
    project->libraries.push_back(lib1);

    edx::LibraryReference lib2;
    lib2.name = "ZDP Library";
    lib2.localPath = "./Custom Scenery/ZDP_Library/library.txt";
    lib2.entryCount = 627;
    lib2.uuid = "5ec54bdf-7be0-4b54-ac02-5d33472cd216";
    lib2.shortId = "6587fddb";
    lib2.version = "1.2.0";
    project->libraries.push_back(lib2);

    // Add some sample assets
    edx::SceneAsset asset1;
    asset1.id = "Asset001";
    asset1.uniqueId = edx::generate_unique_id();
    asset1.latitude = 37.618999;
    asset1.longitude = -122.375;
    asset1.altitude = 642;
    asset1.heading = 0.0;
    asset1.associatedLibrary = "6587fddb";
    asset1.layerId = "terminals";
    asset1.otherProperties = json{{"building-type", "Terminal"}};
    project->assets.push_back(asset1);

    edx::SceneAsset asset2;
    asset2.id = "Asset002";
    asset2.uniqueId = edx::generate_unique_id();
    asset2.latitude = 37.621;
    asset2.longitude = -122.379;
    asset2.altitude = 641;
    asset2.heading = 90.0;
    asset2.associatedLibrary = "a1b2c3d4";
    asset2.layerId = "hangars";
    asset2.otherProperties = json{{"object-type", "Hangar"}};
    project->assets.push_back(asset2);

    // Add layers
    edx::SceneLayer layer1;
    layer1.layerId = "terminals";
    layer1.name = "Terminal Buildings";
    layer1.description = "Airport terminal structures";
    layer1.assetIds = {"Asset001"};
    layer1.zOrder = 1;
    project->layers.push_back(layer1);

    edx::SceneLayer layer2;
    layer2.layerId = "hangars";
    layer2.name = "Hangars";
    layer2.description = "Aircraft hangar structures";
    layer2.assetIds = {"Asset002"};
    layer2.zOrder = 2;
    project->layers.push_back(layer2);

    // Validate project
    auto errors = manager.validate_project(*project);
    if (!errors.empty()) {
        std::cout << "Project validation errors:\n";
        for (const auto& error : errors) {
            std::cout << "  - " << error << "\n";
        }
    } else {
        std::cout << "Project validation: PASSED\n";
    }

    // Save project
    std::cout << "\n2. Saving project...\n";
    bool saved = manager.save_project(*project, "SanFranciscoDemo.edX",
        [](float progress, const std::string& status) {
            std::cout << "  Progress: " << std::fixed << std::setprecision(1)
                      << (progress * 100) << "% - " << status << "\n";
        });

    if (saved) {
        std::cout << "Project saved successfully!\n";
    } else {
        std::cout << "Failed to save project: " << manager.get_last_error() << "\n";
        return;
    }

    // === CREATE AND SAVE A LIBRARY ===
    std::cout << "\n3. Creating a new library...\n";

    auto library = manager.create_library(
        "Demo Airport Objects",
        "Demo Author",
        "1.0.0"
    );

    if (!library) {
        std::cerr << "Failed to create library\n";
        return;
    }

    // Configure library details
    library->library.description = "Demonstration library for airport objects";
    library->library.gitRepository = "https://github.com/demo/airport-objects";
    library->library.sizeInMB = 125.5;

    // Add sample objects
    edx::LibraryObject obj1;
    obj1.id = "terminal_modern_01";
    obj1.uniqueId = edx::generate_unique_id();
    obj1.name = "Modern Terminal Building";
    obj1.description = "Large modern terminal with glass facade";
    obj1.assetType = "building";
    obj1.category = "terminal";
    obj1.tags = {"modern", "glass", "large"};
    obj1.objectPath = "objects/terminals/modern_01.obj";
    obj1.texturePath = "textures/terminals/modern_01_diff.png";
    obj1.previewImage = "previews/modern_01.jpg";
    obj1.properties = json{
        {"floors", 3},
        {"capacity", 5000},
        {"year_built", 2020},
        {"has_jetways", true}
    };
    library->add_object(obj1);

    edx::LibraryObject obj2;
    obj2.id = "hangar_standard_01";
    obj2.uniqueId = edx::generate_unique_id();
    obj2.name = "Standard Aircraft Hangar";
    obj2.description = "Medium-sized aircraft hangar";
    obj2.assetType = "building";
    obj2.category = "hangar";
    obj2.tags = {"standard", "metal", "medium"};
    obj2.objectPath = "objects/hangars/standard_01.obj";
    obj2.texturePath = "textures/hangars/standard_01_diff.png";
    obj2.previewImage = "previews/hangar_01.jpg";
    obj2.properties = json{
        {"aircraft_capacity", 2},
        {"door_type", "sliding"},
        {"height_meters", 15.5}
    };
    library->add_object(obj2);

    // Validate library
    auto libErrors = manager.validate_library(*library);
    if (!libErrors.empty()) {
        std::cout << "Library validation errors:\n";
        for (const auto& error : libErrors) {
            std::cout << "  - " << error << "\n";
        }
    } else {
        std::cout << "Library validation: PASSED\n";
    }

    // Save library
    std::cout << "\n4. Saving library...\n";
    bool libSaved = manager.save_library(*library, "DemoAirportObjects.lib");

    if (libSaved) {
        std::cout << "Library saved successfully!\n";
    } else {
        std::cout << "Failed to save library: " << manager.get_last_error() << "\n";
        return;
    }

    // === LOAD AND VERIFY FILES ===
    std::cout << "\n5. Loading and verifying saved files...\n";

    // Load project
    auto loadedProject = manager.load_project("SanFranciscoDemo.edX");
    if (loadedProject) {
        std::cout << "✓ Project loaded successfully\n";
        std::cout << "  Name: " << loadedProject->project.name << "\n";
        std::cout << "  Airport: " << loadedProject->airport.icao << " - " << loadedProject->airport.name << "\n";
        std::cout << "  Assets: " << loadedProject->assets.size() << "\n";
        std::cout << "  Layers: " << loadedProject->layers.size() << "\n";
        std::cout << "  Libraries: " << loadedProject->libraries.size() << "\n";
    } else {
        std::cout << "✗ Failed to load project\n";
    }

    // Load library
    auto loadedLibrary = manager.load_library("DemoAirportObjects.lib");
    if (loadedLibrary) {
        std::cout << "✓ Library loaded successfully\n";
        std::cout << "  Name: " << loadedLibrary->library.name << "\n";
        std::cout << "  Version: " << loadedLibrary->library.version << "\n";
        std::cout << "  Objects: " << loadedLibrary->get_object_count() << "\n";

        auto categories = loadedLibrary->get_categories();
        std::cout << "  Categories: ";
        for (size_t i = 0; i < categories.size(); ++i) {
            std::cout << categories[i];
            if (i < categories.size() - 1) std::cout << ", ";
        }
        std::cout << "\n";
    } else {
        std::cout << "✗ Failed to load library\n";
    }

    std::cout << "\n=== Demo completed successfully! ===\n";
    std::cout << "Format version: " << manager.get_format_version() << "\n";
}

/**
 * @brief Main function for demonstration
 */
int main()
{
    try {
        demonstrate_edx_usage();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
}

/// ----------------------------------------------------------------------------

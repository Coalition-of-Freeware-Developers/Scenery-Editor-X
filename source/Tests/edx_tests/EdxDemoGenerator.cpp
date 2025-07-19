/**
* -------------------------------------------------------
* Scenery Editor X - edX Format Demonstration Generator
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* EdxDemoGenerator.cpp
* -------------------------------------------------------
* Standalone program to generate demonstration edX files
* -------------------------------------------------------
*/
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include "../../edX/edXLibraryFile.h"
#include "../../edX/edXManager.h"
#include "../../edX/edXProjectFile.h"

/// -------------------------------------------------------

using namespace edx;

/**
 * @brief Create a comprehensive demonstration library
 */
static LibraryFile CreateDemoLibrary()
{
    std::cout << "Creating demonstration library...\n";

    LibraryFile library;

    // Set up library metadata
    library.library.name = "Scenery Editor X Demonstration Library";
    library.library.path = "demo/edx_demo_library";
    library.library.version = "1.0.0-demo";
    library.library.author = "Scenery Editor X Development Team";
    library.library.gitRepository = "https://github.com/Coalition-of-Freeware-Developers/edx-demo-library.git";
    library.library.description = "A comprehensive demonstration library showcasing the edX file format capabilities including various asset types, property structures, and real-world airport scenarios.";
    library.library.lastModified = std::chrono::system_clock::now();
    library.library.sizeInMB = 0.0; // Will be calculated

    // Define asset templates for different categories
    struct AssetTemplate {
        std::string type;
        std::string category;
        std::string namePrefix;
        std::vector<std::string> tags;
        json baseProperties;
    };

    std::vector<AssetTemplate> assetTemplates = {
        {
            "building",
            "terminals",
            "Terminal Building",
            {"passenger", "terminal", "modern"},
            {{"stories", 2}, {"capacity", 1000}, {"hasJetbridges", true}, {"material", "glass_steel"}}
        },
        {
            "building",
            "hangars",
            "Aircraft Hangar",
            {"maintenance", "aircraft", "storage"},
            {{"width", 100.0}, {"height", 25.0}, {"doors", 2}, {"heatedFacility", true}}
        },
        {
            "vehicle",
            "ground_support",
            "Pushback Tug",
            {"pushback", "tow", "electric"},
            {{"maxWeight", 75000}, {"electric", true}, {"batteryLife", 8.0}, {"autonomous", false}}
        },
        {
            "vehicle",
            "ground_support",
            "Baggage Cart",
            {"baggage", "transport", "electric"},
            {{"capacity", 500}, {"maxSpeed", 15.0}, {"autonomous", false}, {"weatherResistant", true}}
        },
        {
            "lighting",
            "runway",
            "Runway Edge Light",
            {"led", "navigation", "faa-approved"},
            {{"intensity", 1000}, {"colorTemp", 6500}, {"strobeCapable", true}, {"energyEfficient", true}}
        },
        {
            "lighting",
            "taxiway",
            "Taxiway Guidance Light",
            {"led", "guidance", "blue"},
            {{"intensity", 200}, {"colorTemp", 4500}, {"embedInPavement", true}, {"dimmer", true}}
        },
        {
            "signage",
            "runway",
            "Runway Identification Sign",
            {"guidance", "mandatory", "reflective"},
            {{"illuminated", true}, {"material", "aluminum"}, {"retroreflective", true}, {"weatherproof", true}}
        },
        {
            "signage",
            "taxiway",
            "Taxiway Direction Sign",
            {"guidance", "information", "yellow"},
            {{"illuminated", true}, {"background", "yellow"}, {"text", "black"}, {"multilingual", false}}
        },
        {
            "equipment",
            "fire_rescue",
            "ARFF Vehicle",
            {"emergency", "firefighting", "foam"},
            {{"waterCapacity", 3000}, {"foamCapacity", 420}, {"pumpRate", 1500}, {"crewSeats", 6}}
        },
        {
            "equipment",
            "fuel",
            "Aviation Fuel Truck",
            {"refueling", "jet-a", "mobile"},
            {{"capacity", 5000}, {"fuelType", "Jet A-1"}, {"flowRate", 600}, {"filtrationSystem", true}}
        },
        {
            "vegetation",
            "landscape",
            "Airport Landscaping Tree",
            {"landscaping", "native", "low-maintenance"},
            {{"height", 15.0}, {"species", "oak"}, {"seasonalChange", true}, {"droughtTolerant", true}}
        },
        {
            "pavement",
            "runway",
            "Runway Surface Section",
            {"asphalt", "grooved", "cat-iii"},
            {{"length", 100}, {"width", 45}, {"surface", "asphalt"}, {"groovedSurface", true}}
        }
    };

    int objectCounter = 1;
    for (const auto& template_ : assetTemplates)
    {
        for (int variant = 1; variant <= 3; ++variant)
        {
            LibraryObject obj;
            obj.id = "demo_" + std::to_string(objectCounter).insert(0, 3 - std::to_string(objectCounter).length(), '0');
            obj.uniqueId = generateRandomHexValue();
            obj.assetType = template_.type;
            obj.name = template_.namePrefix + " " + std::to_string(variant);
            obj.description = "Demonstration " + template_.type + " asset for " + template_.category + " applications. Variant " + std::to_string(variant) + " showcases different complexity levels and feature sets.";
            obj.category = template_.category;
            obj.tags = template_.tags;
            obj.tags.push_back("demo");
            obj.tags.push_back("variant-" + std::to_string(variant));

            // File paths following realistic structure
            obj.objectPath = "objects/" + template_.category + "/" + obj.id + ".obj";
            obj.texturePath = "textures/" + template_.category + "/" + obj.id + "_diffuse.dds";
            obj.previewImage = "previews/" + obj.id + "_preview.jpg";

            // Copy base properties and add variant-specific ones
            obj.properties = template_.baseProperties;
            obj.properties["variantNumber"] = variant;
            obj.properties["demonstrationAsset"] = true;
            obj.properties["complexityLevel"] = (variant == 1) ? "low" : (variant == 2) ? "medium" : "high";
            obj.properties["lastUpdated"] = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();

            // Add some variant-specific properties based on type
            if (template_.type == "building")
            {
                obj.properties["textureVariant"] = "variant_" + std::to_string(variant);
                obj.properties["nightLighting"] = (variant >= 2);
                obj.properties["detailLevel"] = variant * 0.33;
            }
            else if (template_.type == "vehicle")
            {
                obj.properties["colorScheme"] = (variant == 1) ? "standard" : (variant == 2) ? "airline1" : "airline2";
                obj.properties["weathered"] = (variant == 3);
                obj.properties["animationLevel"] = variant;
            }
            else if (template_.type == "lighting")
            {
                obj.properties["brightness"] = 0.5 + (variant * 0.25);
                obj.properties["energyEfficiency"] = 80 + (variant * 5);
                obj.properties["smartControl"] = (variant >= 2);
            }
            else if (template_.type == "signage")
            {
                obj.properties["fontSize"] = 12 + (variant * 2);
                obj.properties["visibility"] = 100 + (variant * 50);
                obj.properties["multilingual"] = (variant == 3);
            }

            library.objects.push_back(obj);
            objectCounter++;
        }
    }

    // Calculate estimated size
    library.library.sizeInMB = 15.0 + (library.objects.size() * 2.8);

    std::cout << "✓ Created library with " << library.objects.size() << " objects\n";
    return library;
}

/**
 * @brief Create a comprehensive demonstration project
 */
EdxProject CreateDemoProject()
{
    std::cout << "Creating demonstration project...\n";

    EdxProject project;

    // Project metadata
    project.project.name = "San Francisco International Airport (KSFO) - Demo";
    project.project.editorVersion = "2.0.0";
    project.project.xplaneVersion = "12.1.4";
    project.project.author = "Scenery Editor X Development Team";
    project.project.description = "Comprehensive demonstration of the edX project format featuring a realistic airport layout with terminals, ground support equipment, lighting systems, and detailed asset placement.";
    project.project.createDate = std::chrono::system_clock::now() - std::chrono::hours(24 * 7); // 1 week ago
    project.project.editDate = std::chrono::system_clock::now();

    // Airport information (realistic KSFO data)
    project.airport.name = "San Francisco International Airport";
    project.airport.icao = "KSFO";
    project.airport.iata = "SFO";
    project.airport.faa = "SFO";
    project.airport.city = "San Francisco";
    project.airport.state = "California";
    project.airport.country = "United States";
    project.airport.regionCode = "K1";

    // Geographic coordinates
    project.airport.datumLat = 37.618999;
    project.airport.datumLon = -122.375;
    project.airport.elevation = 13;
    project.airport.magneticVariation = -14.0;

    // Airspace
    project.airport.transitionAltitude = 18000;
    project.airport.transitionLevel = "FL180";

    // Frequencies (using unique_ptr for optional values)
    project.airport.atis = std::make_unique<double>(135.1);
    project.airport.tower = std::make_unique<double>(120.5);
    project.airport.ground = std::make_unique<double>(121.8);
    project.airport.approach = std::make_unique<double>(120.5);
    project.airport.departure = std::make_unique<double>(135.1);
    project.airport.clearance = std::make_unique<double>(121.8);

    // Library references
    LibraryReference defaultLib;
    defaultLib.name = "X-Plane Default Airport Objects";
    defaultLib.localPath = "Resources/default scenery/airport scenery/library.txt";
    defaultLib.uuid = "15da3863-07d1-462a-be65-3873058675f3";
    defaultLib.shortId = "xp_default";
    defaultLib.version = "12.1.4";
    defaultLib.entryCount = 5427;
    project.libraries.push_back(defaultLib);

    LibraryReference demoLib;
    demoLib.name = "Scenery Editor X Demo Library";
    demoLib.localPath = "demo/edx_demo_library.edxlib";
    demoLib.uuid = "8f4a2b1c-6d3e-4f2a-9b8c-1e5f7a9d3c8b";
    demoLib.shortId = "edx_demo";
    demoLib.version = "1.0.0-demo";
    demoLib.entryCount = 36; // Number of objects in demo library
    project.libraries.push_back(demoLib);

    // Add some sample assets demonstrating various scenarios
    std::vector<std::pair<double, double>> terminalPositions = {
        {37.616999, -122.390},  // Terminal 1
        {37.615999, -122.385},  // Terminal 2
        {37.612999, -122.380},  // Terminal 3
        {37.610999, -122.375}   // International Terminal
    };

    // Add terminal buildings
    for (size_t i = 0; i < terminalPositions.size(); ++i)
    {
        SceneAsset terminal;
        terminal.id = "terminal_" + std::to_string(i + 1);
        terminal.uniqueId = generateRandomHexValue();
        terminal.latitude = terminalPositions[i].first;
        terminal.longitude = terminalPositions[i].second;
        terminal.altitude = 13.0;
        terminal.heading = 90.0 + (i * 5);
        terminal.associatedLibrary = "edx_demo";
        terminal.layerId = "terminals";
        terminal.otherProperties["terminal_name"] = "Terminal " + std::to_string(i + 1);
        terminal.otherProperties["gates"] = (i + 1) * 15;
        terminal.otherProperties["year_built"] = 1960 + (i * 10);
        terminal.otherProperties["object_type"] = "terminal_building";
        project.assets.push_back(terminal);
    }

    // Add ground support equipment
    for (int i = 0; i < 20; ++i)
    {
        SceneAsset gse;
        gse.id = "gse_" + std::to_string(i + 1);
        gse.uniqueId = generateRandomHexValue();

        // Distribute around terminals
        auto terminalIdx = i % terminalPositions.size();
        gse.latitude = terminalPositions[terminalIdx].first + ((i % 20 - 10) * 0.0001);
        gse.longitude = terminalPositions[terminalIdx].second + ((i % 20 - 10) * 0.0001);
        gse.altitude = 13.0;
        gse.heading = i * 18.0; // Various headings
        gse.associatedLibrary = "edx_demo";
        gse.layerId = "ground_support";
        gse.otherProperties["vehicle_id"] = "GSE-" + std::to_string(1000 + i);
        gse.otherProperties["operational"] = (i % 5 != 0); // 80% operational
        gse.otherProperties["object_type"] = (i % 3 == 0) ? "pushback_tug" : "baggage_cart";
        project.assets.push_back(gse);
    }

    // Add runway lighting
    std::vector<std::pair<double, double>> runwayPoints = {
        {37.625, -122.395}, {37.625, -122.360},  // Runway 1
        {37.615, -122.395}, {37.615, -122.360}   // Runway 2
    };

    for (size_t i = 0; i < runwayPoints.size(); i += 2)
    {
        for (int lightPos = 0; lightPos < 10; ++lightPos)
        {
            SceneAsset light;
            light.id = "runway_light_" + std::to_string(i/2 + 1) + "_" + std::to_string(lightPos + 1);
            light.uniqueId = generateRandomHexValue();

            double t = lightPos / 9.0;
            light.latitude = runwayPoints[i].first + t * (runwayPoints[i+1].first - runwayPoints[i].first);
            light.longitude = runwayPoints[i].second + t * (runwayPoints[i+1].second - runwayPoints[i].second);
            light.altitude = 13.0;
            light.heading = 0.0;
            light.associatedLibrary = "xp_default";
            light.layerId = "lighting";
            light.otherProperties["light_type"] = "runway_edge";
            light.otherProperties["intensity"] = 100;
            light.otherProperties["runway_number"] = std::to_string(i/2 + 1);
            project.assets.push_back(light);
        }
    }

    // Create scene layers for organization
    SceneLayer terminalLayer;
    terminalLayer.layerId = "terminals";
    terminalLayer.name = "Terminal Buildings";
    terminalLayer.description = "Main passenger terminal buildings";
    terminalLayer.opacity = 1.0;
    terminalLayer.zOrder = 10;
    project.layers.push_back(terminalLayer);

    SceneLayer gseLayer;
    gseLayer.layerId = "ground_support";
    gseLayer.name = "Ground Support Equipment";
    gseLayer.description = "Airport ground support vehicles and equipment";
    gseLayer.opacity = 1.0;
    gseLayer.zOrder = 5;
    project.layers.push_back(gseLayer);

    SceneLayer lightingLayer;
    lightingLayer.layerId = "lighting";
    lightingLayer.name = "Airport Lighting";
    lightingLayer.description = "Runway and taxiway lighting systems";
    lightingLayer.opacity = 1.0;
    lightingLayer.zOrder = 1;
    project.layers.push_back(lightingLayer);

    std::cout << "✓ Created project with " << project.assets.size() << " assets and " << project.layers.size() << " layers\n";
    return project;
}

/**
 * @brief Generate documentation for the demo files
 */
void GenerateDocumentation(const std::filesystem::path& outputDir, const LibraryFile& library, const EdxProject& project)
{
    std::cout << "Generating documentation...\n";

    auto docPath = outputDir / "EdX_Format_Demonstration_README.md";
    std::ofstream doc(docPath);

    doc << "# Scenery Editor X - edX File Format Demonstration\n\n";
    doc << "This directory contains demonstration files showcasing the capabilities of the edX file format used by Scenery Editor X.\n\n";

    doc << "## Generated Files\n\n";
    doc << "### Library Files\n";
    doc << "- `scenery_editor_x_demo_library.edxlib` - Binary edX library file\n";
    doc << "- `scenery_editor_x_demo_library.json` - Human-readable JSON version\n\n";

    doc << "### Project Files\n";
    doc << "- `ksfo_demonstration_project.edx` - Binary edX project file\n";
    doc << "- `ksfo_demonstration_project.json` - Human-readable JSON version\n\n";

    doc << "### Documentation\n";
    doc << "- `EdX_Format_Demonstration_README.md` - This file\n\n";

    doc << "## Library Overview\n";
    doc << "**Name:** " << library.library.name << "\n";
    doc << "**Version:** " << library.library.version << "\n";
    doc << "**Author:** " << library.library.author << "\n";
    doc << "**Objects:** " << library.objects.size() << "\n";
    auto librarySizeMB = library.library.sizeInMB;
    doc << "**Estimated Size:** " << std::fixed << std::setprecision(1) << librarySizeMB << " MB\n\n";

    doc << "### Object Categories\n";
    auto categories = library.get_categories();
    for (const auto& category : categories)
    {
        auto count = std::count_if(library.objects.begin(), library.objects.end(),
            [&category](const LibraryObject& obj) { return obj.category == category; });
        doc << "- **" << category << ":** " << count << " objects\n";
    }
    doc << "\n";

    doc << "### Asset Types\n";
    auto assetTypes = library.get_asset_types();
    for (const auto& assetType : assetTypes)
    {
        auto count = std::count_if(library.objects.begin(), library.objects.end(),
            [&assetType](const LibraryObject& obj) { return obj.assetType == assetType; });
        doc << "- **" << assetType << ":** " << count << " objects\n";
    }
    doc << "\n";

    doc << "## Project Overview\n";
    doc << "**Name:** " << project.project.name << "\n";
    doc << "**Airport:** " << project.airport.name << " (" << project.airport.icao << ")\n";
    doc << "**Location:** " << project.airport.city << ", " << project.airport.state << "\n";
    doc << "**Assets:** " << project.assets.size() << "\n";
    doc << "**Libraries:** " << project.libraries.size() << "\n";
    doc << "**Layers:** " << project.layers.size() << "\n\n";

    doc << "### Airport Details\n";
    auto airportLat = project.airport.datumLat;
    auto airportLon = project.airport.datumLon;
    auto airportElevation = project.airport.elevation;
    auto magneticVariation = project.airport.magneticVariation;
    
    doc << "- **Coordinates:** " << std::fixed << std::setprecision(6) << airportLat << ", " << airportLon << "\n";
    doc << "- **Elevation:** " << airportElevation << " ft\n";
    doc << "- **Magnetic Variation:** " << std::fixed << std::setprecision(1) << magneticVariation << "°\n\n";

    // Handle frequencies safely
    if (project.airport.atis) {
        auto atisFreq = *project.airport.atis;
        doc << "- **ATIS:** " << std::fixed << std::setprecision(1) << atisFreq << "\n";
    }
    if (project.airport.tower) {
        auto towerFreq = *project.airport.tower;
        doc << "- **Tower:** " << std::fixed << std::setprecision(1) << towerFreq << "\n";
    }
    if (project.airport.ground) {
        auto groundFreq = *project.airport.ground;
        doc << "- **Ground:** " << std::fixed << std::setprecision(1) << groundFreq << "\n";
    }
    doc << "\n";

    doc << "### Scene Layers\n";
    for (const auto& layer : project.layers)
    {
        doc << "- **" << layer.name << "** (`" << layer.layerId << "`): " << layer.description << "\n";
    }
    doc << "\n";

    doc << "## File Format Features Demonstrated\n";
    doc << "- Complete library metadata with version tracking\n";
    doc << "- Diverse object types with varying property structures\n";
    doc << "- Complex project hierarchy with airport and scenery data\n";
    doc << "- Asset positioning with geographic coordinates\n";
    doc << "- Layer-based organization for scene management\n";
    doc << "- Library referencing and dependency management\n";
    doc << "- JSON-based serialization with human readability\n";
    doc << "- Extensible property system for custom attributes\n\n";

    doc << "## Usage Instructions\n";
    doc << "1. **Viewing edX Files:** Open `.edx` files directly in Scenery Editor X\n";
    doc << "2. **Examining Structure:** Review the `.json` files in any text editor\n";
    doc << "3. **Development Reference:** Use these files as examples for edX format implementation\n";
    doc << "4. **Testing:** Load these files to validate edX parser functionality\n\n";

    doc << "## Technical Notes\n";
    doc << "- All coordinates use WGS84 decimal degrees\n";
    doc << "- Elevations are in feet above mean sea level\n";
    doc << "- Headings are in degrees true north (0-359)\n";
    doc << "- Object properties support multiple data types (string, number, boolean, array, object)\n";
    doc << "- Unique IDs are generated using random hexadecimal values\n";
    doc << "- Timestamps use ISO 8601 format in UTC\n\n";

    doc << "---\n";
    doc << "*Generated by Scenery Editor X edX Format Demonstration Tool*\n";
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    doc << "*Date: " << timestamp << "*\n";

    doc.close();
    std::cout << "✓ Generated documentation\n";
}

/**
 * @brief Main demonstration function
 */
int main()
{
    std::cout << "=== Scenery Editor X - edX File Format Demonstration ===\n\n";

    try
    {
        // Create output directory
        auto outputDir = std::filesystem::current_path() / "edx_demonstration_files";
        std::filesystem::create_directories(outputDir);
        std::cout << "Output directory: " << outputDir << "\n\n";

        // Create demonstration library
        auto demoLibrary = CreateDemoLibrary();

        // Validate library
        if (!demoLibrary.validate())
        {
            std::cerr << "Library validation failed!\n";
            auto errors = demoLibrary.get_validation_errors();
            for (const auto& error : errors)
            {
                std::cerr << "  - " << error << "\n";
            }
            return 1;
        }

        // Save library files
        auto libraryPath = outputDir / "scenery_editor_x_demo_library.edxlib";
        auto libraryJsonPath = outputDir / "scenery_editor_x_demo_library.json";

        std::cout << "Saving library files...\n";
        if (!demoLibrary.save_to_file(libraryPath))
        {
            std::cerr << "Failed to save library file!\n";
            return 1;
        }

        // Save pretty JSON version
        json libraryJson;
        demoLibrary.to_json(libraryJson);
        std::ofstream libraryJsonFile(libraryJsonPath);
        libraryJsonFile << libraryJson.dump(2);
        libraryJsonFile.close();
        std::cout << "✓ Saved library files\n";

        // Create demonstration project
        auto demoProject = CreateDemoProject();

        // Validate project
        if (!demoProject.validate())
        {
            std::cerr << "Project validation failed!\n";
            auto errors = demoProject.get_validation_errors();
            for (const auto& error : errors)
            {
                std::cerr << "  - " << error << "\n";
            }
            return 1;
        }

        // Save project files
        auto projectPath = outputDir / "ksfo_demonstration_project.edx";
        auto projectJsonPath = outputDir / "ksfo_demonstration_project.json";

        std::cout << "Saving project files...\n";
        if (!demoProject.save_to_file(projectPath))
        {
            std::cerr << "Failed to save project file!\n";
            return 1;
        }

        // Save pretty JSON version
        json projectJson;
        demoProject.to_json(projectJson);
        std::ofstream projectJsonFile(projectJsonPath);
        projectJsonFile << projectJson.dump(2);
        projectJsonFile.close();
        std::cout << "✓ Saved project files\n";

        // Generate documentation
        GenerateDocumentation(outputDir, demoLibrary, demoProject);

        // Output summary
        std::cout << "\n=== Generation Complete ===\n";
        std::cout << "Generated Files:\n";
        std::cout << "  Library: " << libraryPath << " (" << std::filesystem::file_size(libraryPath) / 1024 << " KB)\n";
        std::cout << "  Library JSON: " << libraryJsonPath << " (" << std::filesystem::file_size(libraryJsonPath) / 1024 << " KB)\n";
        std::cout << "  Project: " << projectPath << " (" << std::filesystem::file_size(projectPath) / 1024 << " KB)\n";
        std::cout << "  Project JSON: " << projectJsonPath << " (" << std::filesystem::file_size(projectJsonPath) / 1024 << " KB)\n";
        std::cout << "  Documentation: " << outputDir / "EdX_Format_Demonstration_README.md" << "\n\n";

        std::cout << "Library Summary:\n";
        std::cout << "  Objects: " << demoLibrary.objects.size() << "\n";
        std::cout << "  Categories: " << demoLibrary.get_categories().size() << "\n";
        std::cout << "  Asset Types: " << demoLibrary.get_asset_types().size() << "\n\n";

        std::cout << "Project Summary:\n";
        std::cout << "  Assets: " << demoProject.assets.size() << "\n";
        std::cout << "  Libraries: " << demoProject.libraries.size() << "\n";
        std::cout << "  Layers: " << demoProject.layers.size() << "\n\n";

        std::cout << "✓ All demonstration files generated successfully!\n";
        std::cout << "✓ Files can be opened and inspected in Scenery Editor X\n";
        std::cout << "✓ JSON files can be viewed in any text editor for format reference\n";

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}

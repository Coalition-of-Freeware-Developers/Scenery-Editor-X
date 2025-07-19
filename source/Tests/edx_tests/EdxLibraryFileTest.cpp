/**
* -------------------------------------------------------
* Scenery Editor X - edX Library File Tests
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* EdxLibraryFileTest.cpp
* -------------------------------------------------------
* Tests for edX library file operations and serialization
* -------------------------------------------------------
*/
#include <algorithm>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <numbers>
#include <set>
#include "../../edX/edXConfig.h"
#include "../../edX/edXLibraryFile.h"

/// -------------------------------------------------------

using namespace edx;
using Catch::Approx;

namespace EdxTests
{
    namespace LibraryFileTests
    {
        // Helper function to create a sample library
        LibraryFile CreateSampleLibrary()
        {
            LibraryFile library;

            // Library metadata
            library.library.name = "Test Asset Library";
            library.library.path = "test_library";
            library.library.version = "1.5.0";
            library.library.author = "Test Author";
            library.library.gitRepository = "https://github.com/test/test-library.git";
            library.library.sizeInMB = 125.7;
            library.library.description = "A comprehensive test library for validation";
            library.library.lastModified = std::chrono::system_clock::now();

            // Add various objects
            LibraryObject obj1;
            obj1.id = "obj_001";
            obj1.uniqueId = generateRandomHexValue();
            obj1.assetType = "building";
            obj1.name = "Terminal Building";
            obj1.description = "Main airport terminal building";
            obj1.category = "buildings";
            obj1.tags = {"terminal", "passenger", "modern"};
            obj1.objectPath = "objects/terminal_main.obj";
            obj1.texturePath = "textures/terminal_diffuse.png";
            obj1.previewImage = "previews/terminal_thumb.jpg";
            obj1.properties["width"] = 200.0;
            obj1.properties["height"] = 30.0;
            obj1.properties["length"] = 150.0;
            obj1.properties["stories"] = 3;
            library.objects.push_back(obj1);

            LibraryObject obj2;
            obj2.id = "obj_002";
            obj2.uniqueId = generateRandomHexValue();
            obj2.assetType = "vehicle";
            obj2.name = "Baggage Cart";
            obj2.description = "Airport ground support baggage cart";
            obj2.category = "ground_support";
            obj2.tags = {"vehicle", "baggage", "electric"};
            obj2.objectPath = "objects/baggage_cart.obj";
            obj2.texturePath = "textures/cart_diffuse.png";
            obj2.previewImage = "previews/cart_thumb.jpg";
            obj2.properties["maxCapacity"] = 500.0;
            obj2.properties["batteryLife"] = 8.0;
            obj2.properties["electricVehicle"] = true;
            library.objects.push_back(obj2);

            LibraryObject obj3;
            obj3.id = "obj_003";
            obj3.uniqueId = generateRandomHexValue();
            obj3.assetType = "lighting";
            obj3.name = "Runway Light";
            obj3.description = "Standard runway edge lighting";
            obj3.category = "lighting";
            obj3.tags = {"runway", "led", "navigation"};
            obj3.objectPath = "objects/runway_light.obj";
            obj3.texturePath = "textures/light_diffuse.png";
            obj3.previewImage = "previews/light_thumb.jpg";
            obj3.properties["intensity"] = 1000.0;
            obj3.properties["colorTemperature"] = 6500;
            obj3.properties["ledType"] = "white";
            library.objects.push_back(obj3);

            return library;
        }

    } // namespace LibraryFileTests
} // namespace EdxTests

/// -------------------------------------------------------

TEST_CASE("Library Metadata Serialization", "[library][serialization]")
{
    using namespace EdxTests::LibraryFileTests;

    SECTION("Complete library metadata to/from JSON")
    {
        Library lib;
        lib.name = "Test Airport Assets";
        lib.path = "libraries/test_airport_assets";
        lib.version = "2.1.3";
        lib.author = "Airport Design Team";
        lib.gitRepository = "https://github.com/airportdesign/test-assets.git";
        lib.sizeInMB = 256.8;
        lib.description = "High-quality airport assets collection";
        lib.lastModified = std::chrono::system_clock::now();

        // Serialize to JSON
        json j;
        lib.to_json(j);

        REQUIRE(j["name"] == "Test Airport Assets");
        REQUIRE(j["path"] == "libraries/test_airport_assets");
        REQUIRE(j["version"] == "2.1.3");
        REQUIRE(j["author"] == "Airport Design Team");
        REQUIRE(j["gitRepository"] == "https://github.com/airportdesign/test-assets.git");
        REQUIRE(j["sizeInMB"] == Approx(256.8));
        REQUIRE(j["description"] == "High-quality airport assets collection");
        REQUIRE(j.contains("lastModified"));

        // Deserialize from JSON
        Library lib2;
        lib2.from_json(j);

        REQUIRE(lib2.name == lib.name);
        REQUIRE(lib2.path == lib.path);
        REQUIRE(lib2.version == lib.version);
        REQUIRE(lib2.author == lib.author);
        REQUIRE(lib2.gitRepository == lib.gitRepository);
        REQUIRE(lib2.sizeInMB == Approx(lib.sizeInMB));
        REQUIRE(lib2.description == lib.description);
    }
}

TEST_CASE("Library Object Serialization", "[library][objects][serialization]")
{
    using namespace EdxTests::LibraryFileTests;

    SECTION("Library object with properties to/from JSON")
    {
        LibraryObject obj;
        obj.id = "hangar_001";
        obj.uniqueId = "a1b2c3d4";
        obj.assetType = "building";
        obj.name = "Aircraft Hangar";
        obj.description = "Large commercial aircraft hangar";
        obj.category = "buildings";
        obj.tags = {"hangar", "maintenance", "large"};
        obj.objectPath = "objects/hangar_large.obj";
        obj.texturePath = "textures/hangar_metal.png";
        obj.previewImage = "previews/hangar_thumb.jpg";

        // Add various property types
        obj.properties["width"] = 80.0;
        obj.properties["height"] = 25.0;
        obj.properties["length"] = 120.0;
        obj.properties["doorCount"] = 2;
        obj.properties["heatedFacility"] = true;
        obj.properties["material"] = "steel";
        obj.properties["capacity"] = "wide-body";

        // Serialize to JSON
        json j;
        obj.to_json(j);

        REQUIRE(j["id"] == "hangar_001");
        REQUIRE(j["assetType"] == "building");
        REQUIRE(j["name"] == "Aircraft Hangar");
        REQUIRE(j["category"] == "buildings");
        REQUIRE(j["tags"].size() == 3);
        REQUIRE(j["properties"]["width"] == Approx(80.0));
        REQUIRE(j["properties"]["height"] == Approx(25.0));
        REQUIRE(j["properties"]["doorCount"] == 2);
        REQUIRE(j["properties"]["heatedFacility"] == true);
        REQUIRE(j["properties"]["material"] == "steel");

        // Deserialize from JSON
        LibraryObject obj2;
        obj2.from_json(j);

        REQUIRE(obj2.id == obj.id);
        REQUIRE(obj2.uniqueId == obj.uniqueId);
        REQUIRE(obj2.assetType == obj.assetType);
        REQUIRE(obj2.name == obj.name);
        REQUIRE(obj2.category == obj.category);
        REQUIRE(obj2.tags.size() == obj.tags.size());
        REQUIRE(obj2.properties["width"] == Approx(80.0));
        REQUIRE(obj2.properties["heatedFacility"] == true);
        REQUIRE(obj2.properties["material"] == "steel");
    }
}

TEST_CASE("Library File Operations", "[library][file-generation][integration]")
{
    using namespace EdxTests::LibraryFileTests;

    // Create test output directory
    auto testDir = std::filesystem::current_path() / "test_output";
    std::filesystem::create_directories(testDir);

    SECTION("Create, save, and load complete library")
    {
        // Create a sample library
        LibraryFile originalLibrary = CreateSampleLibrary();

        // Test validation
        REQUIRE(originalLibrary.validate() == true);
        auto errors = originalLibrary.get_validation_errors();
        REQUIRE(errors.empty());

        // Save to file
        auto testFilePath = testDir / "test_library.edxlib";
        bool saveResult = originalLibrary.save_to_file(testFilePath);
        REQUIRE(saveResult == true);
        REQUIRE(std::filesystem::exists(testFilePath));

        // Verify file is not empty
        auto fileSize = std::filesystem::file_size(testFilePath);
        REQUIRE(fileSize > 0);

        // Load from file
        LibraryFile loadedLibrary;
        bool loadResult = loadedLibrary.load_from_file(testFilePath);
        REQUIRE(loadResult == true);

        // Verify loaded data matches original
        REQUIRE(loadedLibrary.library.name == originalLibrary.library.name);
        REQUIRE(loadedLibrary.library.version == originalLibrary.library.version);
        REQUIRE(loadedLibrary.library.author == originalLibrary.library.author);
        REQUIRE(loadedLibrary.library.sizeInMB == Approx(originalLibrary.library.sizeInMB));

        REQUIRE(loadedLibrary.objects.size() == originalLibrary.objects.size());

        // Verify objects
        for (size_t i = 0; i < loadedLibrary.objects.size(); ++i)
        {
            REQUIRE(loadedLibrary.objects[i].id == originalLibrary.objects[i].id);
            REQUIRE(loadedLibrary.objects[i].name == originalLibrary.objects[i].name);
            REQUIRE(loadedLibrary.objects[i].assetType == originalLibrary.objects[i].assetType);
            REQUIRE(loadedLibrary.objects[i].category == originalLibrary.objects[i].category);
            REQUIRE(loadedLibrary.objects[i].tags.size() == originalLibrary.objects[i].tags.size());
        }
    }

    SECTION("Test JSON file format integrity")
    {
        LibraryFile library = CreateSampleLibrary();
        auto testFilePath = testDir / "json_integrity_test.edxlib";

        // Save library
        bool saveResult = library.save_to_file(testFilePath);
        REQUIRE(saveResult == true);

        // Manually read and parse JSON to verify structure
        std::ifstream file(testFilePath);
        REQUIRE(file.is_open());

        json j;
        file >> j;
        file.close();

        // Verify JSON structure
        REQUIRE(j.contains("library"));
        REQUIRE(j.contains("objects"));
        REQUIRE(j["library"].contains("name"));
        REQUIRE(j["library"].contains("version"));
        REQUIRE(j["library"].contains("author"));
        REQUIRE(j["objects"].is_array());
        REQUIRE(j["objects"].size() == library.objects.size());

        // Verify each object has required fields
        for (const auto& objJson : j["objects"])
        {
            REQUIRE(objJson.contains("id"));
            REQUIRE(objJson.contains("uniqueId"));
            REQUIRE(objJson.contains("assetType"));
            REQUIRE(objJson.contains("name"));
            REQUIRE(objJson.contains("category"));
            REQUIRE(objJson.contains("tags"));
            REQUIRE(objJson.contains("properties"));
        }
    }

    SECTION("Create comprehensive library for inspection", "[file-generation]")
    {
        LibraryFile library = CreateSampleLibrary();

        // Add more diverse objects for inspection
        std::vector<std::string> categories = {"buildings", "vehicles", "lighting", "signage", "vegetation", "equipment"};
        std::vector<std::string> assetTypes = {"building", "vehicle", "light", "sign", "tree", "equipment"};

        for (int i = 4; i <= 20; ++i)
        {
            LibraryObject obj;
            obj.id = "obj_" + std::to_string(i).insert(0, 3 - std::to_string(i).length(), '0');
            obj.uniqueId = generateRandomHexValue();
            obj.assetType = assetTypes[i % assetTypes.size()];
            obj.name = "Test Object " + std::to_string(i);
            obj.description = "Generated test object for category " + categories[i % categories.size()];
            obj.category = categories[i % categories.size()];
            obj.tags = {"test", "generated", obj.assetType};
            obj.objectPath = "objects/" + obj.id + ".obj";
            obj.texturePath = "textures/" + obj.id + "_diffuse.png";
            obj.previewImage = "previews/" + obj.id + "_thumb.jpg";

            // Add varied properties
            obj.properties["generatedId"] = i;
            obj.properties["testProperty"] = "value_" + std::to_string(i);
            obj.properties["scale"] = 1.0 + (i * 0.05);
            obj.properties["complexity"] = (i % 3 == 0) ? "high" : (i % 2 == 0) ? "medium" : "low";
            obj.properties["animated"] = (i % 4 == 0);

            library.objects.push_back(obj);
        }

        // Update library size based on object count
        library.library.sizeInMB = 50.0 + (library.objects.size() * 2.5);

        // Save for manual inspection
        auto inspectionFilePath = testDir / "comprehensive_library_for_inspection.edxlib";
        bool saveResult = library.save_to_file(inspectionFilePath);
        REQUIRE(saveResult == true);

        // Also save as pretty-printed JSON for easier inspection
        json j;
        library.to_json(j);

        auto prettyFilePath = testDir / "comprehensive_library_pretty.json";
        std::ofstream prettyFile(prettyFilePath);
        prettyFile << j.dump(4);  // 4 spaces for indentation
        prettyFile.close();

        REQUIRE(std::filesystem::exists(inspectionFilePath));
        REQUIRE(std::filesystem::exists(prettyFilePath));

        // Print information for the user
        INFO("Generated test library files for inspection:");
        INFO("  Library file: " << inspectionFilePath.string());
        INFO("  Pretty JSON:  " << prettyFilePath.string());
        INFO("  Library contains " << library.objects.size() << " objects");
        INFO("  Library size: " << library.library.sizeInMB << " MB");
    }

    SECTION("Error handling for file operations")
    {
        LibraryFile library = CreateSampleLibrary();

        // Test saving to invalid path
        auto invalidPath = std::filesystem::path("/invalid/nonexistent/path/test.edxlib");
        bool invalidSaveResult = library.save_to_file(invalidPath);
        REQUIRE(invalidSaveResult == false);

        // Test loading from non-existent file
        LibraryFile loadLibrary;
        auto nonExistentPath = testDir / "nonexistent_file.edxlib";
        bool invalidLoadResult = loadLibrary.load_from_file(nonExistentPath);
        REQUIRE(invalidLoadResult == false);

        // Test loading from corrupted file
        auto corruptedFilePath = testDir / "corrupted_file.edxlib";
        std::ofstream corruptedFile(corruptedFilePath);
        corruptedFile << "This is not valid JSON content!!!";
        corruptedFile.close();

        LibraryFile corruptedLibrary;
        bool corruptedLoadResult = corruptedLibrary.load_from_file(corruptedFilePath);
        REQUIRE(corruptedLoadResult == false);
    }

    SECTION("Test properties with different data types")
    {
        LibraryFile library;
        library.library.name = "Property Types Test Library";
        library.library.version = "1.0.0";
        library.library.author = "Test Author";

        LibraryObject obj;
        obj.id = "prop_test_001";
        obj.uniqueId = generateRandomHexValue();
        obj.assetType = "test";
        obj.name = "Property Test Object";
        obj.category = "test";

        // Add properties of different types
        obj.properties["stringProperty"] = "test string value";
        obj.properties["intProperty"] = 42;
        obj.properties["doubleProperty"] = std::numbers::pi;
        obj.properties["boolProperty"] = true;
        obj.properties["arrayProperty"] = json::array({"item1", "item2", "item3"});
        obj.properties["objectProperty"] = json::object({
            {"nestedString", "nested value"},
            {"nestedNumber", 123},
            {"nestedBool", false}
        });

        library.add_object(obj);

        // Save and reload
        auto propertyTestPath = testDir / "property_types_test.edxlib";
        bool saveResult = library.save_to_file(propertyTestPath);
        REQUIRE(saveResult == true);

        LibraryFile loadedLibrary;
        bool loadResult = loadedLibrary.load_from_file(propertyTestPath);
        REQUIRE(loadResult == true);

        auto* loadedObj = loadedLibrary.find_object("prop_test_001");
        REQUIRE(loadedObj != nullptr);

        // Verify property types are preserved
        REQUIRE(loadedObj->properties["stringProperty"] == "test string value");
        REQUIRE(loadedObj->properties["intProperty"] == 42);
        REQUIRE(loadedObj->properties["doubleProperty"] == Approx(3.14159));
        REQUIRE(loadedObj->properties["boolProperty"] == true);
        REQUIRE(loadedObj->properties["arrayProperty"].is_array());
        REQUIRE(loadedObj->properties["arrayProperty"].size() == 3);
        REQUIRE(loadedObj->properties["objectProperty"].is_object());
        REQUIRE(loadedObj->properties["objectProperty"]["nestedString"] == "nested value");
    }

    SECTION("Performance test with large library")
    {
        LibraryFile largeLibrary;
        largeLibrary.library.name = "Performance Test Library";
        largeLibrary.library.version = "1.0.0";
        largeLibrary.library.author = "Performance Tester";

        // Create 1000 objects for performance testing
        constexpr int objectCount = 1000;
        for (int i = 0; i < objectCount; ++i)
        {
            LibraryObject obj;
            obj.id = "perf_obj_" + std::to_string(i);
            obj.uniqueId = generateRandomHexValue();
            obj.assetType = "performance_test";
            obj.name = "Performance Object " + std::to_string(i);
            obj.description = "Generated for performance testing";
            obj.category = "performance";
            obj.tags = {"performance", "test", "generated"};
            obj.objectPath = "objects/perf_" + std::to_string(i) + ".obj";
            obj.texturePath = "textures/perf_" + std::to_string(i) + ".png";
            obj.previewImage = "previews/perf_" + std::to_string(i) + ".jpg";

            // Add multiple properties
            obj.properties["index"] = i;
            obj.properties["scale"] = 1.0 + (i * 0.001);
            obj.properties["category_index"] = i % 10;
            obj.properties["is_even"] = (i % 2 == 0);

            largeLibrary.add_object(obj);
        }

        REQUIRE(largeLibrary.get_object_count() == objectCount);

        auto perfTestPath = testDir / "performance_test_library.edxlib";

        // Time the save operation
        auto saveStart = std::chrono::high_resolution_clock::now();
        bool saveResult = largeLibrary.save_to_file(perfTestPath);
        auto saveEnd = std::chrono::high_resolution_clock::now();
        auto saveDuration = std::chrono::duration_cast<std::chrono::milliseconds>(saveEnd - saveStart);

        REQUIRE(saveResult == true);
        INFO("Save operation took: " << saveDuration.count() << " ms for " << objectCount << " objects");

        // Time the load operation
        LibraryFile loadedLargeLibrary;
        auto loadStart = std::chrono::high_resolution_clock::now();
        bool loadResult = loadedLargeLibrary.load_from_file(perfTestPath);
        auto loadEnd = std::chrono::high_resolution_clock::now();
        auto loadDuration = std::chrono::duration_cast<std::chrono::milliseconds>(loadEnd - loadStart);

        REQUIRE(loadResult == true);
        REQUIRE(loadedLargeLibrary.get_object_count() == objectCount);
        INFO("Load operation took: " << loadDuration.count() << " ms for " << objectCount << " objects");

        // Verify file size
        auto fileSize = std::filesystem::file_size(perfTestPath);
        INFO("Generated file size: " << fileSize / 1024.0 << " KB");
    }
}

TEST_CASE("Library Object Management", "[library][objects]")
{
    using namespace EdxTests::LibraryFileTests;

    SECTION("Add and remove objects")
    {
        LibraryFile library;
        library.library.name = "Test Management Library";
        library.library.version = "1.0.0";
        library.library.author = "Test Author";

        REQUIRE(library.get_object_count() == 0);

        // Add objects
        LibraryObject obj1;
        obj1.id = "test_001";
        obj1.name = "Test Object 1";
        obj1.assetType = "building";
        obj1.category = "test";

        LibraryObject obj2;
        obj2.id = "test_002";
        obj2.name = "Test Object 2";
        obj2.assetType = "vehicle";
        obj2.category = "test";

        library.add_object(obj1);
        library.add_object(obj2);

        REQUIRE(library.get_object_count() == 2);

        // Find objects
        auto* found1 = library.find_object("test_001");
        REQUIRE(found1 != nullptr);
        REQUIRE(found1->name == "Test Object 1");

        auto* found2 = library.find_object("test_002");
        REQUIRE(found2 != nullptr);
        REQUIRE(found2->name == "Test Object 2");

        auto* notFound = library.find_object("non_existent");
        REQUIRE(notFound == nullptr);

        // Remove object
        bool removed = library.remove_object("test_001");
        REQUIRE(removed == true);
        REQUIRE(library.get_object_count() == 1);

        auto* afterRemoval = library.find_object("test_001");
        REQUIRE(afterRemoval == nullptr);

        // Try to remove non-existent object
        bool notRemoved = library.remove_object("non_existent");
        REQUIRE(notRemoved == false);
    }

    SECTION("Get categories and asset types")
    {
        LibraryFile library = CreateSampleLibrary();

        auto categories = library.get_categories();
        REQUIRE_FALSE(categories.empty());

        bool hasBuildings = std::ranges::find(categories, "buildings") != categories.end();
        bool hasGroundSupport = std::ranges::find(categories, "ground_support") != categories.end();
        bool hasLighting = std::ranges::find(categories, "lighting") != categories.end();

        REQUIRE(hasBuildings);
        REQUIRE(hasGroundSupport);
        REQUIRE(hasLighting);

        auto assetTypes = library.get_asset_types();
        REQUIRE_FALSE(assetTypes.empty());

        bool hasBuilding = std::ranges::find(assetTypes, "building") != assetTypes.end();
        bool hasVehicle = std::ranges::find(assetTypes, "vehicle") != assetTypes.end();
        bool hasLightingType = std::ranges::find(assetTypes, "lighting") != assetTypes.end();

        REQUIRE(hasBuilding);
        REQUIRE(hasVehicle);
        REQUIRE(hasLightingType);
    }
}

TEST_CASE("Library Validation", "[library][validation]")
{
    using namespace EdxTests::LibraryFileTests;

    SECTION("Valid library passes validation")
    {
        LibraryFile library = CreateSampleLibrary();
        REQUIRE(library.validate() == true);
        auto errors = library.get_validation_errors();
        REQUIRE(errors.empty());
    }

    SECTION("Invalid library fails validation")
    {
        LibraryFile library;
        // Leave most fields empty/invalid

        REQUIRE(library.validate() == false);
        auto errors = library.get_validation_errors();
        REQUIRE_FALSE(errors.empty());

        // Should have validation errors
        bool hasLibraryNameError = false;
        bool hasVersionError = false;

        for (const auto& error : errors)
        {
            if (error.find("library name") != std::string::npos || error.find("name") != std::string::npos)
                hasLibraryNameError = true;
            if (error.find("version") != std::string::npos)
                hasVersionError = true;
        }

        // At least one of these should be true depending on validation implementation
        bool hasExpectedError = hasLibraryNameError || hasVersionError;
        REQUIRE(hasExpectedError);
    }
}

TEST_CASE("Random Hex Value Generation", "[library][utility]")
{
    using namespace EdxTests::LibraryFileTests;

    SECTION("Generate unique hex values")
    {
        std::set<std::string> generatedValues;

        // Generate many values and ensure they're unique
        for (int i = 0; i < 1000; ++i)
        {
            std::string hexValue = generateRandomHexValue();

            // Should be 8 characters long
            REQUIRE(hexValue.length() == 8);

            // Should contain only hexadecimal characters
            for (char c : hexValue)
            {
                bool isHex = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
                REQUIRE(isHex);
            }

            // Should be unique
            REQUIRE(generatedValues.find(hexValue) == generatedValues.end());
            generatedValues.insert(hexValue);
        }

        REQUIRE(generatedValues.size() == 1000);
    }
}

TEST_CASE("Generate demonstration files for manual inspection", "[file-generation][demo]")
{
    using namespace EdxTests::LibraryFileTests;

    // Create test output directory
    auto testDir = std::filesystem::current_path() / "test_output";
    std::filesystem::create_directories(testDir);

    SECTION("Create comprehensive demonstration library")
    {
        LibraryFile demoLibrary;

        // Set up library metadata
        demoLibrary.library.name = "Scenery Editor X Demonstration Library";
        demoLibrary.library.path = "demo/edx_demo_library";
        demoLibrary.library.version = "1.0.0-demo";
        demoLibrary.library.author = "Scenery Editor X Development Team";
        demoLibrary.library.gitRepository = "https://github.com/Coalition-of-Freeware-Developers/edx-demo-library.git";
        demoLibrary.library.description = "A comprehensive demonstration library showcasing the edX file format capabilities including various asset types, property structures, and real-world airport scenarios.";
        demoLibrary.library.lastModified = std::chrono::system_clock::now();

        // Create diverse set of airport objects
        struct AssetTemplate {
            std::string type;
            std::string category;
            std::string namePrefix;
            std::vector<std::string> tags;
            std::map<std::string, json> baseProperties;
        };

        std::vector<AssetTemplate> assetTemplates = {
            {
                .type = "building",
				.category = "terminals",
				.namePrefix = "Terminal",
				.tags = {"passenger", "terminal", "modern"},
				.baseProperties = {{"stories", 2}, {"capacity", 1000}, {"hasJetbridges", true}}
            },
            {
                .type = "building",
				.category = "hangars",
				.namePrefix = "Hangar",
				.tags = {"maintenance", "aircraft", "storage"},
				.baseProperties = {{"width", 100.0}, {"height", 25.0}, {"doors", 2}}
            },
            {
                .type = "vehicle",
				.category = "ground_support",
				.namePrefix = "Pushback Tug",
				.tags = {"pushback", "tow", "electric"},
				.baseProperties = {{"maxWeight", 75000}, {"electric", true}, {"batteryLife", 8.0}}
            },
            {
                .type = "vehicle",
				.category = "ground_support",
				.namePrefix = "Baggage Cart",
				.tags = {"baggage", "transport", "electric"},
				.baseProperties = {{"capacity", 500}, {"maxSpeed", 15.0}, {"autonomous", false}}
            },
            {
                .type = "lighting",
				.category = "runway",
				.namePrefix = "Runway Light",
				.tags = {"led", "navigation", "faa-approved"},
				.baseProperties = {{"intensity", 1000}, {"colorTemp", 6500}, {"strobeCapable", true}}
            },
            {
                .type = "lighting",
				.category = "taxiway",
				.namePrefix = "Taxiway Light",
				.tags = {"led", "guidance", "blue"},
				.baseProperties = {{"intensity", 200}, {"colorTemp", 4500}, {"embedInPavement", true}}
            },
            {
                .type = "signage",
				.category = "runway",
				.namePrefix = "Runway Sign",
				.tags = {"guidance", "mandatory", "reflective"},
				.baseProperties = {{"illuminated", true}, {"material", "aluminum"}, {"retroreflective", true}}
            },
            {
                .type = "signage",
				.category = "taxiway",
				.namePrefix = "Taxiway Sign",
				.tags = {"guidance", "information", "yellow"},
				.baseProperties = {{"illuminated", true}, {"background", "yellow"}, {"text", "black"}}
            },
            {
                .type = "equipment",
				.category = "fire_rescue",
				.namePrefix = "ARFF Vehicle",
				.tags = {"emergency", "firefighting", "foam"},
				.baseProperties = {{"waterCapacity", 3000}, {"foamCapacity", 420}, {"pumpRate", 1500}}
            },
            {
                .type = "equipment",
				.category = "fuel",
				.namePrefix = "Fuel Truck",
				.tags = {"refueling", "jet-a", "mobile"},
				.baseProperties = {{"capacity", 5000}, {"fuelType", "Jet A-1"}, {"flowRate", 600}}
            },
            {
                .type = "vegetation",
				.category = "landscape",
				.namePrefix = "Airport Tree",
				.tags = {"landscaping", "native", "low-maintenance"},
				.baseProperties = {{"height", 15.0}, {"species", "oak"}, {"seasonalChange", true}}
            },
            {
                .type = "pavement",
				.category = "runway",
				.namePrefix = "Runway Surface",
				.tags = {"asphalt", "grooved", "cat-iii"},
				.baseProperties = {{"length", 3000}, {"width", 45}, {"surface", "asphalt"}}
            }
        };

        int objectCounter = 1;
        for (const auto&[type, category, namePrefix, tags, baseProperties] : assetTemplates)
        {
            for (int variant = 1; variant <= 3; ++variant)
            {
                LibraryObject obj;
                obj.id = "demo_" + std::to_string(objectCounter).insert(0, 3 - std::to_string(objectCounter).length(), '0');
                obj.uniqueId = generateRandomHexValue();
                obj.assetType = type;
                obj.name = namePrefix + " " + std::to_string(variant);
                obj.description = "Demonstration " + type + " asset for " + category + " applications";
                obj.category = category;
                obj.tags = tags;
                obj.tags.emplace_back("demo");
                obj.tags.push_back("variant-" + std::to_string(variant));

                // File paths
                obj.objectPath = "objects/" + category + "/" + obj.id + ".obj";
                obj.texturePath = "textures/" + category + "/" + obj.id + "_diffuse.dds";
                obj.previewImage = "previews/" + obj.id + "_preview.jpg";

                // Copy base properties and add variant-specific ones
                obj.properties = baseProperties;
                obj.properties["variantNumber"] = variant;
                obj.properties["demonstrationAsset"] = true;
                obj.properties["complexityLevel"] = (variant == 1) ? "low" : (variant == 2) ? "medium" : "high";
                obj.properties["lastUpdated"] = std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();

                // Add some variant-specific properties
                if (type == "building")
                {
                    obj.properties["textureVariant"] = "variant_" + std::to_string(variant);
                    obj.properties["nightLighting"] = (variant >= 2);
                }
                else if (type == "vehicle")
                {
                    obj.properties["colorScheme"] = (variant == 1) ? "standard" : (variant == 2) ? "airline1" : "airline2";
                    obj.properties["weathered"] = (variant == 3);
                }
                else if (type == "lighting")
                {
                    obj.properties["brightness"] = 0.5 + (variant * 0.25);
                    obj.properties["energyEfficiency"] = 80 + (variant * 5);
                }

                demoLibrary.add_object(obj);
                objectCounter++;
            }
        }

        // Update library size estimation
        demoLibrary.library.sizeInMB = 15.0 + (demoLibrary.get_object_count() * 2.8);

        // Validate the demo library
        REQUIRE(demoLibrary.validate() == true);
        auto errors = demoLibrary.get_validation_errors();
        REQUIRE(errors.empty());

        // Save the comprehensive demo library
        auto demoLibPath = testDir / "scenery_editor_x_demo_library.edxlib";
        bool saveResult = demoLibrary.save_to_file(demoLibPath);
        REQUIRE(saveResult == true);

        // Also save as formatted JSON for inspection
        json demoJson;
        demoLibrary.to_json(demoJson);
        auto prettyJsonPath = testDir / "scenery_editor_x_demo_library_formatted.json";
        std::ofstream prettyFile(prettyJsonPath);
        prettyFile << demoJson.dump(2); // 2 spaces for readability
        prettyFile.close();

        // Create documentation file
        auto docPath = testDir / "demo_library_documentation.txt";
        std::ofstream docFile(docPath);
        docFile << "Scenery Editor X - edX File Format Demonstration Library\n";
        docFile << "=======================================================\n\n";
        docFile << "Generated: " << std::chrono::system_clock::now().time_since_epoch().count() << "\n";
        docFile << "Library: " << demoLibrary.library.name << "\n";
        docFile << "Version: " << demoLibrary.library.version << "\n";
        docFile << "Author: " << demoLibrary.library.author << "\n";
        docFile << "Objects: " << demoLibrary.get_object_count() << "\n";
        docFile << "Size: " << demoLibrary.library.sizeInMB << " MB\n\n";

        docFile << "Files Generated:\n";
        docFile << "- " << demoLibPath.filename().string() << " (edX library file)\n";
        docFile << "- " << prettyJsonPath.filename().string() << " (Human-readable JSON)\n";
        docFile << "- " << docPath.filename().string() << " (This documentation)\n\n";

        docFile << "Object Categories:\n";
        auto categories = demoLibrary.get_categories();
        for (const auto& category : categories)
        {
            auto categoryObjects = std::ranges::count_if(demoLibrary.objects,
                                                         [&category](const LibraryObject& obj) { return obj.category == category; });
            docFile << "- " << category << ": " << categoryObjects << " objects\n";
        }

        docFile << "\nAsset Types:\n";
        auto assetTypes = demoLibrary.get_asset_types();
        for (const auto& assetType : assetTypes)
        {
            auto typeObjects = std::ranges::count_if(demoLibrary.objects,
                                                     [&assetType](const LibraryObject& obj) { return obj.assetType == assetType; });
            docFile << "- " << assetType << ": " << typeObjects << " objects\n";
        }

        docFile << "\nUsage Instructions:\n";
        docFile << "1. Open " << demoLibPath.filename().string() << " with Scenery Editor X\n";
        docFile << "2. Or examine " << prettyJsonPath.filename().string() << " in any JSON editor\n";
        docFile << "3. Each object demonstrates different property types and structures\n";
        docFile << "4. Use this as a reference for the edX file format capabilities\n";

        docFile.close();

        REQUIRE(std::filesystem::exists(demoLibPath));
        REQUIRE(std::filesystem::exists(prettyJsonPath));
        REQUIRE(std::filesystem::exists(docPath));

        // Output information for the test runner
        INFO("Generated demonstration files:");
        INFO("  Library file: " << demoLibPath.string());
        INFO("  Formatted JSON: " << prettyJsonPath.string());
        INFO("  Documentation: " << docPath.string());
        INFO("  Total objects: " << demoLibrary.get_object_count());
        INFO("  Categories: " << categories.size());
        INFO("  Asset types: " << assetTypes.size());
    }

    SECTION("Create minimal example library")
    {
        LibraryFile minimalLibrary;

        minimalLibrary.library.name = "Minimal Example Library";
        minimalLibrary.library.version = "1.0.0";
        minimalLibrary.library.author = "Test Author";
        minimalLibrary.library.description = "Minimal example showing basic edX structure";
        minimalLibrary.library.sizeInMB = 5.2;
        minimalLibrary.library.lastModified = std::chrono::system_clock::now();

        // Add just one simple object
        LibraryObject obj;
        obj.id = "minimal_001";
        obj.uniqueId = generateRandomHexValue();
        obj.assetType = "building";
        obj.name = "Simple Terminal";
        obj.description = "Basic terminal building for minimal example";
        obj.category = "buildings";
        obj.tags = {"simple", "example", "terminal"};
        obj.objectPath = "objects/simple_terminal.obj";
        obj.texturePath = "textures/terminal.png";
        obj.previewImage = "previews/terminal_preview.jpg";
        obj.properties["width"] = 50.0;
        obj.properties["height"] = 8.0;
        obj.properties["stories"] = 1;
        obj.properties["example"] = true;

        minimalLibrary.add_object(obj);

        auto minimalPath = testDir / "minimal_example.edxlib";
        bool saveResult = minimalLibrary.save_to_file(minimalPath);
        REQUIRE(saveResult == true);

        // Also create minimal JSON
        json minimalJson;
        minimalLibrary.to_json(minimalJson);
        auto minimalJsonPath = testDir / "minimal_example.json";
        std::ofstream minimalFile(minimalJsonPath);
        minimalFile << minimalJson.dump(4); // 4 spaces for clarity
        minimalFile.close();

        INFO("Generated minimal example files:");
        INFO("  Library file: " << minimalPath.string());
        INFO("  JSON file: " << minimalJsonPath.string());
    }
}

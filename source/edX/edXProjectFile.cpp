/**
* -------------------------------------------------------
* Scenery Editor X - edX File Format
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* edXProjectFile.cpp
* -------------------------------------------------------
* Created: 11/7/2025
* -------------------------------------------------------
*/
#include <fstream>
#include <iostream>
#include <memory>
#include "edXProjectFile.h"
#include "edXTimeUtils.h"

/// ----------------------------------------------------------------------------

namespace edx
{
    // ProjectInfo JSON serialization
    void ProjectInfo::to_json(json& j) const
    {
        j = json{
            {"name", name},
            {"editor-version", editorVersion},
            {"XPlaneVersion", xplaneVersion},
            {"author", author},
            {"editdate", time_point_to_iso_string(editDate)},
            {"createdate", time_point_to_iso_string(createDate)},
            {"description", description}
        };
    }

    void ProjectInfo::from_json(const json& j)
    {
        name = j.value("name", "");
        editorVersion = j.value("editor-version", "");
        xplaneVersion = j.value("XPlaneVersion", "");
        author = j.value("author", "");
        description = j.value("description", "");

        if (j.contains("editdate")) {
            editDate = iso_string_to_time_point(j["editdate"]);
        } else {
            editDate = std::chrono::system_clock::now();
        }

        if (j.contains("createdate")) {
            createDate = iso_string_to_time_point(j["createdate"]);
        } else {
            createDate = std::chrono::system_clock::now();
        }
    }

    // AirportInfo JSON serialization
    void AirportInfo::to_json(json& j) const
    {
        j = json{
            {"Name", name},
            {"ICAO", icao},
            {"IATA", iata},
            {"FAA", faa},
            {"City", city},
            {"State", state},
            {"Country", country},
            {"RegionCode", regionCode},
            {"DatumLat", datumLat},
            {"DatumLon", datumLon},
            {"Elevation", elevation},
            {"MagVar", magneticVariation},
            {"TransitionAlt", transitionAltitude},
            {"TransitionLevel", transitionLevel}
        };

        // Add optional frequencies only if they have values
        if (ctaf != nullptr) j["CTAF"] = *ctaf;
        if (atis != nullptr) j["ATIS"] = *atis;
        if (tower != nullptr) j["Tower"] = *tower;
        if (ground != nullptr) j["Ground"] = *ground;
        if (approach != nullptr) j["Approach"] = *approach;
        if (departure != nullptr) j["Departure"] = *departure;
        if (clearance != nullptr) j["Clearance"] = *clearance;
    }

    void AirportInfo::from_json(const json& j)
    {
        name = j.value("Name", "");
        icao = j.value("ICAO", "");
        iata = j.value("IATA", "");
        faa = j.value("FAA", "");
        city = j.value("City", "");
        state = j.value("State", "");
        country = j.value("Country", "");
        regionCode = j.value("RegionCode", "");
        datumLat = j.value("DatumLat", 0.0);
        datumLon = j.value("DatumLon", 0.0);
        elevation = j.value("Elevation", 0);
        magneticVariation = j.value("MagVar", 0.0);
        transitionAltitude = j.value("TransitionAlt", 18000);
        transitionLevel = j.value("TransitionLevel", "FL180");

        // Read optional frequencies
        if (j.contains("CTAF")) ctaf = std::make_unique<double>(j["CTAF"].get<double>());
        if (j.contains("ATIS")) atis = std::make_unique<double>(j["ATIS"].get<double>());
        if (j.contains("Tower")) tower = std::make_unique<double>(j["Tower"].get<double>());
        if (j.contains("Ground")) ground = std::make_unique<double>(j["Ground"].get<double>());
        if (j.contains("Approach")) approach = std::make_unique<double>(j["Approach"].get<double>());
        if (j.contains("Departure")) departure = std::make_unique<double>(j["Departure"].get<double>());
        if (j.contains("Clearance")) clearance = std::make_unique<double>(j["Clearance"].get<double>());
    }

    // LibraryReference JSON serialization
    void LibraryReference::to_json(json& j) const
    {
        j = json{
            {"Library", name},
            {"local-path", localPath},
            {"entry-count", entryCount},
            {"uuid", uuid},
            {"short-id", shortId},
            {"version", version}
        };
    }

    void LibraryReference::from_json(const json& j)
    {
        name = j.value("Library", "");
        localPath = j.value("local-path", "");
        entryCount = j.value("entry-count", 0);
        uuid = j.value("uuid", "");
        shortId = j.value("short-id", "");
        version = j.value("version", "");
    }

    // SceneAsset JSON serialization
    void SceneAsset::to_json(json& j) const
    {
        j = json{
            {"id", id},
            {"unique-id", uniqueId},
            {"latitude", latitude},
            {"longitude", longitude},
            {"altitude", altitude},
            {"heading", heading},
            {"associated-library", associatedLibrary},
            {"layer-id", layerId},
            {"group-id", groupId},
            {"locked", locked},
            {"hidden", hidden},
            {"selected", selected}
        };

        // Add other properties if they exist
        if (!otherProperties.empty()) {
            j["other-properties"] = otherProperties;
        }
    }

    void SceneAsset::from_json(const json& j)
    {
        id = j.value("id", "");
        uniqueId = j.value("unique-id", "");
        latitude = j.value("latitude", 0.0);
        longitude = j.value("longitude", 0.0);
        altitude = j.value("altitude", 0.0);
        heading = j.value("heading", 0.0);
        associatedLibrary = j.value("associated-library", "");
        layerId = j.value("layer-id", "");
        groupId = j.value("group-id", "");
        locked = j.value("locked", false);
        hidden = j.value("hidden", false);
        selected = j.value("selected", false);

        if (j.contains("other-properties")) {
            otherProperties = j["other-properties"];
        }
    }

    // SceneLayer JSON serialization
    void SceneLayer::to_json(json& j) const
    {
        j = json{
            {"layer-id", layerId},
            {"name", name},
            {"description", description},
            {"locked", locked},
            {"hidden", hidden},
            {"opacity", opacity},
            {"z-order", zOrder},
            {"asset-ids", assetIds}
        };

        if (!layerProperties.empty()) {
            j["layer-properties"] = layerProperties;
        }
    }

    void SceneLayer::from_json(const json& j)
    {
        layerId = j.value("layer-id", "");
        name = j.value("name", "");
        description = j.value("description", "");
        locked = j.value("locked", false);
        hidden = j.value("hidden", false);
        opacity = j.value("opacity", 1.0);
        zOrder = j.value("z-order", 0);

        if (j.contains("asset-ids")) {
            assetIds = j["asset-ids"];
        }

        if (j.contains("layer-properties")) {
            layerProperties = j["layer-properties"];
        }
    }

    // EdxProject JSON serialization
    void EdxProject::to_json(json& j) const
    {
        json projectJson, airportJson;
        project.to_json(projectJson);
        airport.to_json(airportJson);

        json librariesJson = json::array();
        for (const auto& lib : libraries) {
            json libJson;
            lib.to_json(libJson);
            librariesJson.push_back(libJson);
        }

        json assetsJson = json::array();
        for (const auto& asset : assets) {
            json assetJson;
            asset.to_json(assetJson);
            assetsJson.push_back(assetJson);
        }

        json layersJson = json::array();
        for (const auto& layer : layers) {
            json layerJson;
            layer.to_json(layerJson);
            layersJson.push_back(layerJson);
        }

        j = json{
            {"Project", projectJson},
            {"Airport", airportJson},
            {"Libraries", librariesJson},
            {"Assets", assetsJson},
            {"Layers", layersJson}
        };

        if (!settings.empty()) {
            j["Settings"] = settings;
        }
    }

    void EdxProject::from_json(const json& j)
    {
        if (j.contains("Project")) {
            project.from_json(j["Project"]);
        }

        if (j.contains("Airport")) {
            airport.from_json(j["Airport"]);
        }

        libraries.clear();
        if (j.contains("Libraries")) {
            for (const auto& libJson : j["Libraries"]) {
                LibraryReference lib;
                lib.from_json(libJson);
                libraries.push_back(lib);
            }
        }

        assets.clear();
        if (j.contains("Assets")) {
            for (const auto& assetJson : j["Assets"]) {
                SceneAsset asset;
                asset.from_json(assetJson);
                assets.push_back(asset);
            }
        }

        layers.clear();
        if (j.contains("Layers")) {
            for (const auto& layerJson : j["Layers"]) {
                SceneLayer layer;
                layer.from_json(layerJson);
                layers.push_back(layer);
            }
        }

        if (j.contains("Settings")) {
            settings = j["Settings"];
        }
    }

    // File operations
    bool EdxProject::save_to_file(const std::filesystem::path& filePath) const
    {
        try {
            json j;
            to_json(j);

            std::ofstream file(filePath);
            if (!file.is_open()) {
                std::cerr << "Error: Cannot open file for writing: " << filePath << std::endl;
                return false;
            }

            // Pretty print JSON with 4-space indentation
            file << j.dump(4);
            file.close();

            std::cout << "Successfully saved project to: " << filePath << std::endl;
            return true;

        } catch (const std::exception& e) {
            std::cerr << "Error saving project file: " << e.what() << std::endl;
            return false;
        }
    }

    bool EdxProject::load_from_file(const std::filesystem::path& filePath)
    {
        try {
            if (!std::filesystem::exists(filePath)) {
                std::cerr << "Error: File does not exist: " << filePath << std::endl;
                return false;
            }

            std::ifstream file(filePath);
            if (!file.is_open()) {
                std::cerr << "Error: Cannot open file for reading: " << filePath << std::endl;
                return false;
            }

            json j;
            file >> j;
            file.close();

            from_json(j);

            std::cout << "Successfully loaded project from: " << filePath << std::endl;
            return true;

        } catch (const json::parse_error& e) {
            std::cerr << "JSON parse error: " << e.what() << std::endl;
            return false;
        } catch (const std::exception& e) {
            std::cerr << "Error loading project file: " << e.what() << std::endl;
            return false;
        }
    }

    // Validation
    bool EdxProject::validate() const
    {
        return get_validation_errors().empty();
    }

    std::vector<std::string> EdxProject::get_validation_errors() const
    {
        std::vector<std::string> errors;

        // Validate project info
        if (project.name.empty()) {
            errors.push_back("Project name cannot be empty");
        }

        if (project.editorVersion.empty()) {
            errors.push_back("Editor version cannot be empty");
        }

        // Validate airport info
        if (airport.icao.empty()) {
            errors.push_back("Airport ICAO code cannot be empty");
        }

        if (airport.icao.length() != 4) {
            errors.push_back("Airport ICAO code must be 4 characters");
        }

        // Validate geographic coordinates
        if (airport.datumLat < -90.0 || airport.datumLat > 90.0) {
            errors.push_back("Airport latitude must be between -90 and 90 degrees");
        }

        if (airport.datumLon < -180.0 || airport.datumLon > 180.0) {
            errors.push_back("Airport longitude must be between -180 and 180 degrees");
        }

        // Validate library references
        for (const auto& lib : libraries) {
            if (lib.name.empty()) {
                errors.push_back("Library name cannot be empty");
            }
            if (lib.shortId.empty()) {
                errors.push_back("Library short-id cannot be empty");
            }
        }

        // Validate assets
        for (const auto& asset : assets) {
            if (asset.id.empty()) {
                errors.push_back("Asset ID cannot be empty");
            }

            // Validate geographic coordinates
            if (asset.latitude < -90.0 || asset.latitude > 90.0) {
                errors.push_back("Asset latitude must be between -90 and 90 degrees");
            }

            if (asset.longitude < -180.0 || asset.longitude > 180.0) {
                errors.push_back("Asset longitude must be between -180 and 180 degrees");
            }

            // Validate heading
            if (asset.heading < 0.0 || asset.heading >= 360.0) {
                errors.push_back("Asset heading must be between 0 and 360 degrees");
            }
        }

        return errors;
    }

} // namespace edx

/// ----------------------------------------------------------------------------

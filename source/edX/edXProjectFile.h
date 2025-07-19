/**
* -------------------------------------------------------
* Scenery Editor X - edX File Format
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* edXProjectFile.h
* -------------------------------------------------------
* Created: 27/5/2025
* Updated: 11/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "edXConfig.h"

/// ----------------------------------------------------------------------------

namespace edx
{
    /**
     * @brief Main project information container
     *
     * Contains metadata about the scenery project including name, version,
     * editor information, and creation/modification timestamps.
     */
    struct EDX_API ProjectInfo
    {
        std::string name;
        std::string editorVersion;
        std::string xplaneVersion;
        std::string author;
        std::chrono::system_clock::time_point editDate;
        std::chrono::system_clock::time_point createDate;
        std::string description;

        // JSON serialization support
        void to_json(json& j) const;
        void from_json(const json& j);
    };

    /**
     * @brief Airport information structure
     *
     * Contains comprehensive airport data including location, identifiers,
     * communication frequencies, and elevation information.
     */
    struct EDX_API AirportInfo
    {
        std::string name;
        std::string icao;
        std::string iata;
        std::string faa;
        std::string city;
        std::string state;
        std::string country;
        std::string regionCode;

        // Geographic data (WGS84)
        double datumLat = 0.0;
        double datumLon = 0.0;
        int elevation = 0;
        double magneticVariation = 0.0;

        // Airspace data
        int transitionAltitude = 18000;
        std::string transitionLevel = "FL180";

        // Communication frequencies (using pointers for optional values)
        std::unique_ptr<double> ctaf;
        std::unique_ptr<double> atis;
        std::unique_ptr<double> tower;
        std::unique_ptr<double> ground;
        std::unique_ptr<double> approach;
        std::unique_ptr<double> departure;
        std::unique_ptr<double> clearance;

        // JSON serialization support
        void to_json(json& j) const;
        void from_json(const json& j);
    };

    /**
     * @brief Library reference structure
     *
     * References external libraries used in the project with path resolution
     * and unique identification for tracking dependencies.
     */
    struct EDX_API LibraryReference
    {
        std::string name;
        std::string localPath;
        std::string uuid;
        std::string shortId;
        int entryCount = 0;
        std::string version;

        // JSON serialization support
        void to_json(json& j) const;
        void from_json(const json& j);
    };

    /**
     * @brief Scene asset structure
     *
     * Represents individual assets placed in the scene with position,
     * orientation, and associated properties.
     */
    struct EDX_API SceneAsset
    {
        std::string id;
        std::string uniqueId;

        // Geographic positioning (WGS84)
        double latitude = 0.0;
        double longitude = 0.0;
        double altitude = 0.0;
        double heading = 0.0;

        // Asset organization
        std::string associatedLibrary;
        std::string layerId;
        std::string groupId;

        // Visual properties
        bool locked = false;
        bool hidden = false;
        bool selected = false;

        // Additional properties stored as JSON
        json otherProperties;

        // JSON serialization support
        void to_json(json& j) const;
        void from_json(const json& j);
    };

    /**
     * @brief Scene layer structure
     *
     * Organizes assets into logical layers for management and rendering
     * control within the scene hierarchy.
     */
    struct EDX_API SceneLayer
    {
        std::string layerId;
        std::string name;
        std::string description;

        // Layer properties
        bool locked = false;
        bool hidden = false;
        double opacity = 1.0;
        int zOrder = 0;

        // Asset management
        std::vector<std::string> assetIds;
        json layerProperties;

        // JSON serialization support
        void to_json(json& j) const;
        void from_json(const json& j);
    };

    /**
     * @brief Complete edX project file structure
     *
     * Root container for all project data including metadata, airport
     * information, library references, assets, and layer organization.
     */
    struct EDX_API EdxProject
    {
        ProjectInfo project;
        AirportInfo airport;
        std::vector<LibraryReference> libraries;
        std::vector<SceneAsset> assets;
        std::vector<SceneLayer> layers;

        // Project-wide settings
        json settings;

        // JSON serialization support
        void to_json(json& j) const;
        void from_json(const json& j);

        // File operations
        bool save_to_file(const std::filesystem::path& filePath) const;
        bool load_from_file(const std::filesystem::path& filePath);

        // Validation
        bool validate() const;
        [[nodiscard]] std::vector<std::string> get_validation_errors() const;
    };

}

/// ----------------------------------------------------------------------------



/**
* -------------------------------------------------------
* Scenery Editor X - edX File Format
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* edXLibraryFile.h
* -------------------------------------------------------
* Created: 27/5/2025
* Updated: 11/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <filesystem>
#include <random>
#include <sstream>
#include <string>
#include <vector>
#include "edXConfig.h"

/// ----------------------------------------------------------------------------

namespace edx
{
    /**
     * @brief Generates a random hexadecimal identifier
     *
     * Creates an 8-character hexadecimal string for use as unique identifiers
     * in library objects and references.
     *
     * @return Random 8-character hexadecimal string
     */
    inline std::string generateRandomHexValue()
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 15);

        std::stringstream ss;
        for (int i = 0; i < 8; ++i) // Generate 8 hexadecimal digits
        {
            ss << std::hex << dis(gen);
        }

        return ss.str();
    }

    /**
     * @brief Library metadata structure
     *
     * Contains information about an asset library including name, version,
     * author information, and repository details for tracking and management.
     */
    struct EDX_API Library
    {
        std::string name;
        std::string path;
        std::string version;
        std::string author;
        std::string gitRepository;
        double sizeInMB = 0.0;
        std::string description;
        std::chrono::system_clock::time_point lastModified;

        // JSON serialization support
        void to_json(json& j) const;
        void from_json(const json& j);
    };

    /**
     * @brief Library object definition
     *
     * Represents individual objects within a library with unique identification,
     * asset type classification, and extensible properties for different
     * object types and rendering requirements.
     */
    struct EDX_API LibraryObject
    {
        std::string id;
        std::string uniqueId;
        std::string assetType;
        std::string name;
        std::string description;

        // Asset properties stored as JSON for flexibility
        json properties;

        // Object classification
        std::string category;
        std::vector<std::string> tags;

        // File references
        std::string objectPath;
        std::string texturePath;
        std::string previewImage;

        // JSON serialization support
        void to_json(json& j) const;
        void from_json(const json& j);
    };

    /**
     * @brief Complete library file structure
     *
     * Root container for library information and all contained objects.
     * Provides file I/O operations and validation for library files.
     */
    struct EDX_API LibraryFile
    {
        Library library;
        std::vector<LibraryObject> objects;

        // JSON serialization support
        void to_json(json& j) const;
        void from_json(const json& j);

        // File operations
        bool save_to_file(const std::filesystem::path& filePath) const;
        bool load_from_file(const std::filesystem::path& filePath);

        // Validation
        bool validate() const;
        std::vector<std::string> get_validation_errors() const;

        // Object management
        void add_object(const LibraryObject& obj);
        bool remove_object(const std::string& id);
        LibraryObject* find_object(const std::string& id);
        const LibraryObject* find_object(const std::string& id) const;

        // Statistics
        size_t get_object_count() const { return objects.size(); }
        std::vector<std::string> get_categories() const;
        std::vector<std::string> get_asset_types() const;
    };

} // namespace edx

/// ----------------------------------------------------------------------------

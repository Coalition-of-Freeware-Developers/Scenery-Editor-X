/**
* -------------------------------------------------------
* Scenery Editor X - edX File Format
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* edXLibraryFile.cpp
* -------------------------------------------------------
* Created: 11/7/2025
* -------------------------------------------------------
*/

#include "edXLibraryFile.h"
#include "edXTimeUtils.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <set>

/// ----------------------------------------------------------------------------

namespace edx
{
    // Library JSON serialization
    void Library::to_json(json& j) const
    {
        j = json{
            {"name", name},
            {"path", path},
            {"version", version},
            {"author", author},
            {"git-repository", gitRepository},
            {"size-mb", sizeInMB},
            {"description", description},
            {"last-modified", time_point_to_iso_string(lastModified)}
        };
    }

    void Library::from_json(const json& j)
    {
        name = j.value("name", "");
        path = j.value("path", "");
        version = j.value("version", "");
        author = j.value("author", "");
        gitRepository = j.value("git-repository", "");
        sizeInMB = j.value("size-mb", 0.0);
        description = j.value("description", "");

        if (j.contains("last-modified")) {
            lastModified = iso_string_to_time_point(j["last-modified"]);
        } else {
            lastModified = std::chrono::system_clock::now();
        }
    }

    // LibraryObject JSON serialization
    void LibraryObject::to_json(json& j) const
    {
        j = json{
            {"id", id},
            {"unique-id", uniqueId},
            {"asset-type", assetType},
            {"name", name},
            {"description", description},
            {"category", category},
            {"tags", tags},
            {"object-path", objectPath},
            {"texture-path", texturePath},
            {"preview-image", previewImage}
        };

        if (!properties.empty()) {
            j["properties"] = properties;
        }
    }

    void LibraryObject::from_json(const json& j)
    {
        id = j.value("id", "");
        uniqueId = j.value("unique-id", "");
        assetType = j.value("asset-type", "");
        name = j.value("name", "");
        description = j.value("description", "");
        category = j.value("category", "");
        objectPath = j.value("object-path", "");
        texturePath = j.value("texture-path", "");
        previewImage = j.value("preview-image", "");

        if (j.contains("tags")) {
            tags = j["tags"];
        }

        if (j.contains("properties")) {
            properties = j["properties"];
        }
    }

    // LibraryFile JSON serialization
    void LibraryFile::to_json(json& j) const
    {
        json libraryJson;
        library.to_json(libraryJson);

        json objectsJson = json::array();
        for (const auto& obj : objects) {
            json objJson;
            obj.to_json(objJson);
            objectsJson.push_back(objJson);
        }

        j = json{
            {"Library", libraryJson},
            {"Objects", objectsJson}
        };
    }

    void LibraryFile::from_json(const json& j)
    {
        if (j.contains("Library")) {
            library.from_json(j["Library"]);
        }

        objects.clear();
        if (j.contains("Objects")) {
            for (const auto& objJson : j["Objects"]) {
                LibraryObject obj;
                obj.from_json(objJson);
                objects.push_back(obj);
            }
        }
    }

    // File operations
    bool LibraryFile::save_to_file(const std::filesystem::path& filePath) const
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

            std::cout << "Successfully saved library to: " << filePath << std::endl;
            return true;

        } catch (const std::exception& e) {
            std::cerr << "Error saving library file: " << e.what() << std::endl;
            return false;
        }
    }

    bool LibraryFile::load_from_file(const std::filesystem::path& filePath)
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

            std::cout << "Successfully loaded library from: " << filePath << std::endl;
            return true;

        } catch (const json::parse_error& e) {
            std::cerr << "JSON parse error: " << e.what() << std::endl;
            return false;
        } catch (const std::exception& e) {
            std::cerr << "Error loading library file: " << e.what() << std::endl;
            return false;
        }
    }

    // Validation
    bool LibraryFile::validate() const
    {
        return get_validation_errors().empty();
    }

    std::vector<std::string> LibraryFile::get_validation_errors() const
    {
        std::vector<std::string> errors;

        // Validate library info
        if (library.name.empty())
            errors.push_back("Library name cannot be empty");

        if (library.version.empty())
            errors.push_back("Library version cannot be empty");

        if (library.author.empty())
            errors.push_back("Library author cannot be empty");

        // Validate objects
        std::set<std::string> usedIds;
        std::set<std::string> usedUniqueIds;

        for (const auto& obj : objects) {
            if (obj.id.empty())
                errors.push_back("Object ID cannot be empty");
            else
			{
                if (usedIds.contains(obj.id))
                    errors.push_back("Duplicate object ID: " + obj.id);

                usedIds.insert(obj.id);
            }

            if (obj.uniqueId.empty())
                errors.push_back("Object unique ID cannot be empty");
            else
			{
                if (usedUniqueIds.contains(obj.uniqueId))
                    errors.push_back("Duplicate object unique ID: " + obj.uniqueId);

                usedUniqueIds.insert(obj.uniqueId);
            }

            if (obj.assetType.empty())
                errors.push_back("Object asset type cannot be empty for object: " + obj.id);

            if (obj.name.empty())
                errors.push_back("Object name cannot be empty for object: " + obj.id);
        }

        return errors;
    }

    // Object management
    void LibraryFile::add_object(const LibraryObject& obj)
    {
        // Check for duplicate IDs
        const auto it = std::ranges::find_if(objects,[&obj](const LibraryObject& existing) {
            return existing.id == obj.id || existing.uniqueId == obj.uniqueId;
        });

        if (it != objects.end())
		{
            std::cerr << "Warning: Object with ID " << obj.id << " already exists. Not adding." << std::endl;
            return;
        }

        objects.push_back(obj);
    }

    bool LibraryFile::remove_object(const std::string& id)
    {
        if (const auto it = std::ranges::find_if(objects,[&id](const LibraryObject& obj) {return obj.id == id;}); it != objects.end()) 
        {
            objects.erase(it);
            return true;
        }

        return false;
    }

    LibraryObject* LibraryFile::find_object(const std::string& id)
    {
        const auto it = std::ranges::find_if(objects,[&id](const LibraryObject& obj)
        {
            return obj.id == id;
        });

        return it != objects.end() ? &*it : nullptr;
    }

    const LibraryObject* LibraryFile::find_object(const std::string& id) const
    {
        const auto it = std::ranges::find_if(objects,[&id](const LibraryObject& obj)
        {
            return obj.id == id;
        });

        return it != objects.end() ? &*it : nullptr;
    }

    // Statistics
    std::vector<std::string> LibraryFile::get_categories() const
    {
        std::set<std::string> uniqueCategories;
        for (const auto& obj : objects)
		{
            if (!obj.category.empty())
                uniqueCategories.insert(obj.category);
        }

        return std::vector(uniqueCategories.begin(), uniqueCategories.end());
    }

    std::vector<std::string> LibraryFile::get_asset_types() const
    {
        std::set<std::string> uniqueTypes;
        for (const auto& obj : objects)
		{
            if (!obj.assetType.empty())
                uniqueTypes.insert(obj.assetType);
        }
        return std::vector(uniqueTypes.begin(), uniqueTypes.end());
    }

} // namespace edx

/// ----------------------------------------------------------------------------

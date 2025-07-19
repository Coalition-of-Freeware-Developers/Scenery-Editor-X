/**
* -------------------------------------------------------
* Scenery Editor X - edX File Format API
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* edXManager.cpp
* -------------------------------------------------------
* Created: 11/7/2025
* -------------------------------------------------------
*/

#include "edXManager.h"
#include "edXTimeUtils.h"
#include <fstream>
#include <iostream>
#include <algorithm>

/// ----------------------------------------------------------------------------

namespace edx
{
    // Private implementation struct
    struct EdxManager::Impl
    {
        ErrorCallback errorCallback;
        std::string lastError;

        void reportError(const std::string& error)
        {
            lastError = error;
            if (errorCallback)
            {
                errorCallback(error);
            }
        }
    };

    // Constructor
    EdxManager::EdxManager()
        : m_pImpl(std::make_unique<Impl>())
    {
    }

    // Destructor
    EdxManager::~EdxManager() = default;

    // Project operations
    std::unique_ptr<EdxProject> EdxManager::create_project(
        const std::string& projectName,
        const std::string& author,
        const std::string& icao)
    {
        auto project = std::make_unique<EdxProject>();

        // Initialize project info
        project->project.name = projectName;
        project->project.author = author;
        project->project.editorVersion = VERSION;
        project->project.createDate = std::chrono::system_clock::now();
        project->project.editDate = project->project.createDate;

        // Initialize airport info if ICAO provided
        if (!icao.empty())
        {
            project->airport.icao = icao;
        }

        return project;
    }

    std::unique_ptr<EdxProject> EdxManager::load_project(
        const std::string& filePath,
        ProgressCallback progressCallback)
    {
        try
        {
            if (progressCallback)
                progressCallback(0.0f, "Loading project file...");

            auto project = std::make_unique<EdxProject>();

            if (!project->load_from_file(filePath))
            {
                m_pImpl->reportError("Failed to load project from: " + filePath);
                return nullptr;
            }

            if (progressCallback)
                progressCallback(1.0f, "Project loaded successfully");

            return project;
        }
        catch (const std::exception& e)
        {
            m_pImpl->reportError("Exception loading project: " + std::string(e.what()));
            return nullptr;
        }
    }

    bool EdxManager::save_project(
        const EdxProject& project,
        const std::string& filePath,
        ProgressCallback progressCallback)
    {
        try
        {
            if (progressCallback)
                progressCallback(0.0f, "Validating project...");

            // Validate project before saving
            auto errors = validate_project(project);
            if (!errors.empty())
            {
                std::string errorMsg = "Project validation failed: ";
                for (const auto& error : errors)
                {
                    errorMsg += error + "; ";
                }
                m_pImpl->reportError(errorMsg);
                return false;
            }

            if (progressCallback)
                progressCallback(0.5f, "Saving project file...");

            // Update edit date
            const_cast<EdxProject&>(project).project.editDate = std::chrono::system_clock::now();

            bool result = project.save_to_file(filePath);

            if (progressCallback)
                progressCallback(1.0f, result ? "Project saved successfully" : "Failed to save project");

            if (!result)
            {
                m_pImpl->reportError("Failed to save project to: " + filePath);
            }

            return result;
        }
        catch (const std::exception& e)
        {
            m_pImpl->reportError("Exception saving project: " + std::string(e.what()));
            return false;
        }
    }

    // Library operations
    std::unique_ptr<LibraryFile> EdxManager::create_library(
        const std::string& libraryName,
        const std::string& author,
        const std::string& version)
    {
        auto library = std::make_unique<LibraryFile>();

        // Initialize library info
        library->library.name = libraryName;
        library->library.author = author;
        library->library.version = version;
        library->library.lastModified = std::chrono::system_clock::now();

        return library;
    }

    std::unique_ptr<LibraryFile> EdxManager::load_library(
        const std::string& filePath,
        ProgressCallback progressCallback)
    {
        try
        {
            if (progressCallback)
                progressCallback(0.0f, "Loading library file...");

            auto library = std::make_unique<LibraryFile>();

            if (!library->load_from_file(filePath))
            {
                m_pImpl->reportError("Failed to load library from: " + filePath);
                return nullptr;
            }

            if (progressCallback)
                progressCallback(1.0f, "Library loaded successfully");

            return library;
        }
        catch (const std::exception& e)
        {
            m_pImpl->reportError("Exception loading library: " + std::string(e.what()));
            return nullptr;
        }
    }

    bool EdxManager::save_library(
        const LibraryFile& library,
        const std::string& filePath,
        ProgressCallback progressCallback)
    {
        try
        {
            if (progressCallback)
                progressCallback(0.0f, "Validating library...");

            // Validate library before saving
            auto errors = validate_library(library);
            if (!errors.empty())
            {
                std::string errorMsg = "Library validation failed: ";
                for (const auto& error : errors)
                {
                    errorMsg += error + "; ";
                }
                m_pImpl->reportError(errorMsg);
                return false;
            }

            if (progressCallback)
                progressCallback(0.5f, "Saving library file...");

            // Update last modified time
            const_cast<LibraryFile&>(library).library.lastModified = std::chrono::system_clock::now();

            bool result = library.save_to_file(filePath);

            if (progressCallback)
                progressCallback(1.0f, result ? "Library saved successfully" : "Failed to save library");

            if (!result)
            {
                m_pImpl->reportError("Failed to save library to: " + filePath);
            }

            return result;
        }
        catch (const std::exception& e)
        {
            m_pImpl->reportError("Exception saving library: " + std::string(e.what()));
            return false;
        }
    }

    // Utility functions
    std::vector<std::string> EdxManager::validate_project(const EdxProject& project)
    {
        return project.get_validation_errors();
    }

    std::vector<std::string> EdxManager::validate_library(const LibraryFile& library)
    {
        return library.get_validation_errors();
    }

    std::string EdxManager::get_format_version() const
    {
        return VERSION;
    }

    bool EdxManager::is_valid_project_file(const std::string& filePath)
    {
        try
        {
            EdxProject project;
            return project.load_from_file(filePath);
        }
        catch (...)
        {
            return false;
        }
    }

    bool EdxManager::is_valid_library_file(const std::string& filePath)
    {
        try
        {
            LibraryFile library;
            return library.load_from_file(filePath);
        }
        catch (...)
        {
            return false;
        }
    }

    // Error and progress handling
    void EdxManager::set_error_callback(ErrorCallback callback)
    {
        m_pImpl->errorCallback = callback;
    }

    std::string EdxManager::get_last_error() const
    {
        return m_pImpl->lastError;
    }

    void EdxManager::clear_error()
    {
        m_pImpl->lastError.clear();
    }

    // Conversion utilities
    bool EdxManager::convert_legacy_project(
        const std::string& oldFormatFile,
        const std::string& newFormatFile)
    {
        // TODO: Implement legacy conversion when needed
        m_pImpl->reportError("Legacy conversion not yet implemented");
        return false;
    }

    std::string EdxManager::export_project_to_json(
        const EdxProject& project,
        bool prettyPrint)
    {
        try
        {
            json j;
            project.to_json(j);

            if (prettyPrint)
            {
                return j.dump(4); // 4-space indentation
            }
            else
            {
                return j.dump();
            }
        }
        catch (const std::exception& e)
        {
            m_pImpl->reportError("Failed to export project to JSON: " + std::string(e.what()));
            return "";
        }
    }

    std::unique_ptr<EdxProject> EdxManager::import_project_from_json(
        const std::string& jsonString)
    {
        try
        {
            json j = json::parse(jsonString);
            auto project = std::make_unique<EdxProject>();
            project->from_json(j);
            return project;
        }
        catch (const std::exception& e)
        {
            m_pImpl->reportError("Failed to import project from JSON: " + std::string(e.what()));
            return nullptr;
        }
    }

    // Private helper methods
    void EdxManager::report_error(const std::string& error)
    {
        m_pImpl->reportError(error);
    }

    void EdxManager::report_progress(float progress, const std::string& status)
    {
        // Progress reporting handled via callbacks
    }

    // Convenience functions
    std::unique_ptr<EdxProject> load_project_quick(const std::string& filePath)
    {
        EdxManager manager;
        return manager.load_project(filePath);
    }

    bool save_project_quick(const EdxProject& project, const std::string& filePath)
    {
        EdxManager manager;
        return manager.save_project(project, filePath);
    }

    std::unique_ptr<LibraryFile> load_library_quick(const std::string& filePath)
    {
        EdxManager manager;
        return manager.load_library(filePath);
    }

    bool save_library_quick(const LibraryFile& library, const std::string& filePath)
    {
        EdxManager manager;
        return manager.save_library(library, filePath);
    }

    std::string generate_unique_id()
    {
        return generateRandomHexValue();
    }

} // namespace edx

/// ----------------------------------------------------------------------------

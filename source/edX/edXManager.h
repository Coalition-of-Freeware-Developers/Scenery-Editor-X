/**
* -------------------------------------------------------
* Scenery Editor X - edX File Format API
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* edXManager.h
* -------------------------------------------------------
* Created: 11/7/2025
* -------------------------------------------------------
*/
#pragma once
#include "edXConfig.h"
#include "edXProjectFile.h"
#include "edXLibraryFile.h"
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <optional>

/// ----------------------------------------------------------------------------

namespace edx
{
    /**
     * @brief High-level manager for edX file operations
     *
     * Provides a simplified interface for working with edX project and library
     * files, following Scenery Editor X architecture patterns with proper
     * error handling, logging integration, and cross-platform compatibility.
     */
    class EDX_API EdxManager
    {
    public:
        // Callback types for progress and error reporting
        using ProgressCallback = std::function<void(float progress, const std::string& status)>;
        using ErrorCallback = std::function<void(const std::string& error)>;

        /**
         * @brief Construct a new EdxManager
         */
        EdxManager();

        /**
         * @brief Destroy the EdxManager
         */
        ~EdxManager();

        // Project file operations

        /**
         * @brief Create a new empty project
         *
         * @param projectName Name of the project
         * @param author Author name
         * @param icao Airport ICAO code (optional)
         * @return Unique pointer to the created project
         */
        std::unique_ptr<EdxProject> create_project(
            const std::string& projectName,
            const std::string& author,
            const std::string& icao = ""
        );

        /**
         * @brief Load a project from file
         *
         * @param filePath Path to the .edX file
         * @param progressCallback Optional progress callback
         * @return Unique pointer to the loaded project, nullptr on failure
         */
        std::unique_ptr<EdxProject> load_project(
            const std::string& filePath,
            ProgressCallback progressCallback = nullptr
        );

        /**
         * @brief Save a project to file
         *
         * @param project Project to save
         * @param filePath Path where to save the .edX file
         * @param progressCallback Optional progress callback
         * @return True if successful, false otherwise
         */
        bool save_project(
            const EdxProject& project,
            const std::string& filePath,
            ProgressCallback progressCallback = nullptr
        );

        // Library file operations

        /**
         * @brief Create a new empty library
         *
         * @param libraryName Name of the library
         * @param author Author name
         * @param version Version string
         * @return Unique pointer to the created library
         */
        std::unique_ptr<LibraryFile> create_library(
            const std::string& libraryName,
            const std::string& author,
            const std::string& version = "1.0.0"
        );

        /**
         * @brief Load a library from file
         *
         * @param filePath Path to the library file
         * @param progressCallback Optional progress callback
         * @return Unique pointer to the loaded library, nullptr on failure
         */
        std::unique_ptr<LibraryFile> load_library(
            const std::string& filePath,
            ProgressCallback progressCallback = nullptr
        );

        /**
         * @brief Save a library to file
         *
         * @param library Library to save
         * @param filePath Path where to save the library file
         * @param progressCallback Optional progress callback
         * @return True if successful, false otherwise
         */
        bool save_library(
            const LibraryFile& library,
            const std::string& filePath,
            ProgressCallback progressCallback = nullptr
        );

        // Utility functions

        /**
         * @brief Validate a project file
         *
         * @param project Project to validate
         * @return Vector of validation error messages (empty if valid)
         */
        std::vector<std::string> validate_project(const EdxProject& project);

        /**
         * @brief Validate a library file
         *
         * @param library Library to validate
         * @return Vector of validation error messages (empty if valid)
         */
        std::vector<std::string> validate_library(const LibraryFile& library);

        /**
         * @brief Get format version information
         *
         * @return Version string
         */
        std::string get_format_version() const;

        /**
         * @brief Check if a file is a valid edX project file
         *
         * @param filePath Path to check
         * @return True if valid, false otherwise
         */
        bool is_valid_project_file(const std::string& filePath);

        /**
         * @brief Check if a file is a valid edX library file
         *
         * @param filePath Path to check
         * @return True if valid, false otherwise
         */
        bool is_valid_library_file(const std::string& filePath);

        // Error and progress handling

        /**
         * @brief Set error callback for reporting errors
         *
         * @param callback Error callback function
         */
        void set_error_callback(ErrorCallback callback);

        /**
         * @brief Get the last error message
         *
         * @return Last error message string
         */
        std::string get_last_error() const;

        /**
         * @brief Clear the last error
         */
        void clear_error();

        // Conversion utilities

        /**
         * @brief Convert old format project to new format
         *
         * @param oldFormatFile Path to old format file
         * @param newFormatFile Path where to save new format
         * @return True if successful, false otherwise
         */
        bool convert_legacy_project(
            const std::string& oldFormatFile,
            const std::string& newFormatFile
        );

        /**
         * @brief Export project to JSON string
         *
         * @param project Project to export
         * @param prettyPrint Whether to format JSON with indentation
         * @return JSON string representation
         */
        std::string export_project_to_json(
            const EdxProject& project,
            bool prettyPrint = true
        );

        /**
         * @brief Import project from JSON string
         *
         * @param jsonString JSON string to import
         * @return Unique pointer to imported project, nullptr on failure
         */
        std::unique_ptr<EdxProject> import_project_from_json(
            const std::string& jsonString
        );

    private:
        // Internal implementation details
        struct Impl;
        std::unique_ptr<Impl> m_pImpl;

        // Helper methods
        void report_error(const std::string& error);
        void report_progress(float progress, const std::string& status);
    };

    // Convenience functions for common operations

    /**
     * @brief Quick load project function
     *
     * @param filePath Path to the .edX file
     * @return Unique pointer to loaded project, nullptr on failure
     */
    EDX_API std::unique_ptr<EdxProject> load_project_quick(const std::string& filePath);

    /**
     * @brief Quick save project function
     *
     * @param project Project to save
     * @param filePath Path where to save
     * @return True if successful, false otherwise
     */
    EDX_API bool save_project_quick(const EdxProject& project, const std::string& filePath);

    /**
     * @brief Quick load library function
     *
     * @param filePath Path to the library file
     * @return Unique pointer to loaded library, nullptr on failure
     */
    EDX_API std::unique_ptr<LibraryFile> load_library_quick(const std::string& filePath);

    /**
     * @brief Quick save library function
     *
     * @param library Library to save
     * @param filePath Path where to save
     * @return True if successful, false otherwise
     */
    EDX_API bool save_library_quick(const LibraryFile& library, const std::string& filePath);

    /**
     * @brief Generate a unique ID for assets/objects
     *
     * @return 8-character hexadecimal string
     */
    EDX_API std::string generate_unique_id();

} // namespace edx

/// ----------------------------------------------------------------------------

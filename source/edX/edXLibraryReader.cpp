/**
* -------------------------------------------------------
* Scenery Editor X - edX File Format
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* edXLibraryReader.cpp
* -------------------------------------------------------
* Created: 27/5/2025
* -------------------------------------------------------
*/
#include "edXLibraryFile.h"
#include <fstream>
#include <iostream>
#include <limits>

/// ----------------------------------------------------------------------------

namespace ProjectLibrarys
{
    class LibraryParser
    {
    public:
        static std::vector<edx::Library> parseLibraries(const std::string &filePath)
        {
            std::vector<edx::Library> libraries;
            std::ifstream file(filePath);
            if (!file.is_open())
            {
                std::cerr << "Failed to open file: " << filePath << '\n';
                return libraries;
            }

            std::string line;
            while (std::getline(file, line))
            {
                if (line == "[Library]")
                {
                    edx::Library library;
                    std::getline(file, library.name);
                    std::getline(file, library.path);
                    std::getline(file, library.version);
                    std::getline(file, library.author);
                    std::getline(file, library.gitRepository);
                    file >> library.sizeInMB;
                    file.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Ignore the rest of the line

                    libraries.push_back(library);
                }
            }

            return libraries;
        }

        static std::vector<edx::LibraryObject> parseLibraryObjects(const std::string &filePath)
        {
            std::vector<edx::LibraryObject> libraryObjects;
            std::ifstream file(filePath);
            if (!file.is_open())
            {
                std::cerr << "Failed to open file: " << filePath << '\n';
                return libraryObjects;
            }

            std::string line;
            while (std::getline(file, line))
            {
                if (line == "[LibraryObject]")
                {
                    edx::LibraryObject libraryObject;
                    std::getline(file, libraryObject.id);
                    std::getline(file, libraryObject.uniqueId);
                    std::getline(file, libraryObject.assetType);
                    
                    // Parse properties as JSON string
                    std::string propertiesStr;
                    std::getline(file, propertiesStr);
                    if (!propertiesStr.empty())
                    {
                        try
                        {
                            libraryObject.properties = json::parse(propertiesStr);
                        }
                        catch (const std::exception& e)
                        {
                            std::cerr << "Error parsing properties JSON: " << e.what() << '\n';
                            libraryObject.properties = json::object();
                        }
                    }

                    libraryObjects.push_back(libraryObject);
                }
            }

            return libraryObjects;
        }
    };

} // namespace ProjectLibrarys

/// ----------------------------------------------------------------------------

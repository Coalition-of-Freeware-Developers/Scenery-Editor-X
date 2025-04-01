/*
#include "edXLibraryFile.h"
#include <fstream>
#include <iostream>

namespace ProjectLibrarys
{
    class LibraryParser
    {
    public:
        static std::vector<Library> parseLibraries(const std::string &filePath)
        {
            std::vector<Library> libraries;
            std::ifstream file(filePath);
            if (!file.is_open())
            {
                std::cerr << "Failed to open file: " << filePath << std::endl;
                return libraries;
            }
    
            std::string line;
            while (std::getline(file, line))
            {
                if (line == "[Library]")
                {
                    Library library;
                    std::getline(file, library.library_name);
                    std::getline(file, library.library_path);
                    std::getline(file, library.library_version);
                    std::getline(file, library.library_author);
                    std::getline(file, library.libraryGit);
                    file >> library.librarySize;
                    file.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Ignore the rest of the line
    
                    libraries.push_back(library);
                }
            }
    
            return libraries;
        }
    
        static std::vector<LibraryObject> parseLibraryObjects(const std::string &filePath)
        {
            std::vector<LibraryObject> libraryObjects;
            std::ifstream file(filePath);
            if (!file.is_open())
            {
                std::cerr << "Failed to open file: " << filePath << std::endl;
                return libraryObjects;
            }
    
            std::string line;
            while (std::getline(file, line))
            {
                if (line == "[LibraryObject]")
                {
                    LibraryObject libraryObject;
                    std::getline(file, libraryObject.id);
                    std::getline(file, libraryObject.uniqueId);
                    std::getline(file, libraryObject.assetType);
                    std::getline(file, libraryObject.properties);
    
                    libraryObjects.push_back(libraryObject);
                }
            }
    
            return libraryObjects;
        }
    };
    
} // namespace ProjectLibrarys
*/

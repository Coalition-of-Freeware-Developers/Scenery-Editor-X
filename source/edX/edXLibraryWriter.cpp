/**
* -------------------------------------------------------
* Scenery Editor X - edX File Format
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* edXLibraryWriter.cpp
* -------------------------------------------------------
* Created: 27/5/2025
* -------------------------------------------------------
*/
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "../X-PlaneSceneryLibrary/XPLibraryPath.h"
#include "../edX/edXLibraryFile.h"

/// ----------------------------------------------------------------------------

using namespace edx;

std::string generateUniqueId()
{
    static std::set<std::string> uniqueIds; // Set to store generated uniqueIds

    std::string uniqueId;
    do
        uniqueId = edx::generateRandomHexValue(); // Generate a random hexadecimal value
    while (uniqueIds.contains(uniqueId));         // Check if the generated uniqueId already exists

    uniqueIds.insert(uniqueId); // Add the generated uniqueId to the set

    return uniqueId;
}

void writeEdxLibFile(const std::string &filename,
                     const std::string &libraryName,
                     const std::string &libraryVersion,
                     const std::string &libraryAuthor,
                     const std::string &libraryGit,
                     const std::vector<edx::LibraryObject> &objects)
{
    std::ofstream file(filename);

    if (!file.is_open())
    {
        std::cerr << "Error opening file for writing." << '\n';
        return;
    }

    file << "[Library]\n";
    file << "Name=" << libraryName << "\n";
    file << "Version=" << libraryVersion << "\n";
    file << "Author=" << libraryAuthor << "\n";
    file << "Git=" << libraryGit << "\n";
    file << "Objects=" << objects.size() << "\n\n";

    for (const auto &object : objects)
    {
        file << "[Object]" << "Id=" << object.id
             << "UniqueId=" << (object.uniqueId.empty() ? generateUniqueId() : object.uniqueId)
             << "AssetType=" << object.assetType << "Properties=" << object.properties << "\n";
    }
}

/// ----------------------------------------------------------------------------

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>

#include "../X-PlaneSceneryLibrary/XPLibraryPath.h"
#include "../edX/edXLibraryFile.h"

using namespace ProjectLibrarys;

std::string generateUniqueId()
{
    static std::set<std::string> uniqueIds; // Set to store generated uniqueIds

    std::string uniqueId;
    do
    {
        // Generate a random hexadecimal value
        uniqueId = generateRandomHexValue();
    } while (uniqueIds.count(uniqueId) > 0); // Check if the generated uniqueId already exists

    uniqueIds.insert(uniqueId); // Add the generated uniqueId to the set

    return uniqueId;
}

void writeEdxLibFile(const std::string &filename,
                     const std::string &libraryName,
                     const std::string &libraryVersion,
                     const std::string &libraryAuthor,
                     const std::string &libraryGit,
                     const std::vector<LibraryObject> &objects)
{
    std::ofstream file(filename);

    if (!file.is_open())
    {
        std::cerr << "Error opening file for writing." << std::endl;
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
        file << "[Object]"
        << "Id=" << object.id
        << "UniqueId=" << (object.uniqueId.empty() ? generateUniqueId() : object.uniqueId)
        << "AssetType=" << object.assetType
        << "Properties=" << object.properties << "\n";
    }

}

int main()
{
    std::vector<LibraryObject> objects = {
        {"object1", "", "type1", "properties1"},
        {"object2", "", "type2", "properties2"},
        {"object3", "", "type3", "properties3"}
    };

    writeEdxLibFile("library.edx",
                    "MyLibrary",
                    "1.0",
                    "John Doe",
                    "https://github.com/mylibrary",
                    objects);

    return 0;
}

/**
* -------------------------------------------------------
* Scenery Editor X - edX File Format
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* edXReader.cpp
* -------------------------------------------------------
* Created: 27/5/2025
* -------------------------------------------------------
*/
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

/// ----------------------------------------------------------------------------

struct Asset
{
    std::string id;
    double latitude;
    double longitude;
    double heading;
    std::string properties; // Other properties as a string for simplicity
};

// Assuming 'key' and 'value' are part of a map or similar structure
std::map<std::string, std::string> data = {{"key1", "value1"}, {"key2", "value2"}};

void readEdxFile(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Error opening file for reading." << '\n';
        return;
    }

    std::string line;
    std::string section;
    std::map<std::string, std::string> sceneryData;
    std::map<std::string, std::string> airportData;
    std::vector<std::string> libraries;
    std::vector<Asset> assets;

    while (std::getline(file, line))
    {
        // Remove leading/trailing spaces
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        // Skip empty lines or comments
        if (line.empty() || line[0] == '#')
            continue;

        // Check for section headers
        if (line[0] == '[')
        {
            section = line;
            continue;
        }

        // Parse lines based on current section
        if (section == "[Scenery]")
        {
            if (size_t eqPos = line.find('='); eqPos != std::string::npos)
            {
                std::string key = line.substr(0, eqPos);
                std::string value = line.substr(eqPos + 1);
                sceneryData[key] = value;
            }
        }
        else if (section == "[Airport]")
        {
            if (size_t eqPos = line.find('='); eqPos != std::string::npos)
            {
                std::string key = line.substr(0, eqPos);
                std::string value = line.substr(eqPos + 1);
                airportData[key] = value;
            }
        }
        else if (section == "[Libraries]")
        {
            if (size_t eqPos = line.find('='); eqPos != std::string::npos)
            {
                libraries.push_back(line.substr(eqPos + 1));
            }
        }
        else if (section == "[Assets]")
        {
            if (size_t eqPos = line.find('='); eqPos != std::string::npos)
            {
                std::string id = line.substr(0, eqPos);
                std::string value = line.substr(eqPos + 1);

                std::istringstream iss(value);
                Asset asset;
                asset.id = id;
                char comma;
                iss >> asset.latitude >> comma >> asset.longitude >> comma >> asset.heading >> comma;
                std::getline(iss, asset.properties);
                assets.push_back(asset);
            }
        }
    }

    file.close();

    // Output parsed data for demonstration purposes
    std::cout << "Scenery Data:\n";
    for (const auto &[key, value] : sceneryData)
    {
        std::cout << key << " = " << value << '\n';
    }

    std::cout << "\nAirport Data:\n";
    for (const auto &[key, value] : airportData)
    {
        std::cout << key << " = " << value << '\n';
    }

    std::cout << "\nLibraries:\n";
    for (const auto &library : libraries)
    {
        std::cout << library << '\n';
    }

    std::cout << "\nAssets:\n";
    for (const auto &[id, latitude, longitude, heading, properties] : assets)
    {
        std::cout << id << ": " << latitude << ", " << longitude << ", " << heading << ", "
                  << properties << '\n';
    }
}

/*
int main()
{
    readEdxFile("example.edX");
    return 0;
}
*/

/// ----------------------------------------------------------------------------

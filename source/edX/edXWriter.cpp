/**
* -------------------------------------------------------
* Scenery Editor X - edX File Format
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* edXWriter.cpp
* -------------------------------------------------------
* Created: 27/5/2025
* -------------------------------------------------------
*/
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "../X-PlaneSceneryLibrary/XPLibraryPath.h"
#include "../edX/edXProjectFile.h"

/// ----------------------------------------------------------------------------

void writeEdxFile(const std::string &filename,
                  const std::string &sceneryName,
                  const std::string &editorVersion,
                  const std::string &XPVersion,
                  const std::string &airportName,
                  const std::string &airportICAO,
                  const std::string &airportIATA,
                  const std::string &airportFAA,
                  const std::string &airportCity,
                  const std::string &airportState,
                  const std::string &airportCountry,
                  const std::string &airportRegion,
                  double airportLat,
                  double airportLon,
                  int airportTransAlt,
                  int airportTransLvl,
                  int airportElevation,
                  double airportCTAF,
                  double airportATIS,
                  double airportTower,
                  double airportGround,
                  double airportApproach,
                  double airportDeparture,
                  double airportClearance,
                  std::vector<edx::AirportInfo> &airport,
                  std::vector<edx::LibraryReference> &libraries,
                  std::vector<edx::SceneAsset> &assets)
{
    std::ofstream file(filename);

    if (!file.is_open())
    {
        std::cerr << "Error opening file for writing." << '\n';
        return;
    }

    /**
     * @brief Write the scenery section to the edX file.
     */
    file << "[Scenery]\n";
    file << "Name=" << sceneryName << "\n";
    file << "EditorVersion=" << editorVersion << "\n";
    file << "XPVersion=" << XPVersion << "\n\n";

    /**
     * @brief Write the libraries section to the edX file.
     */
    file << "[Libraries]\n";
    for (const auto &library : libraries)
    {
        file << "Library=" << library.name << "\n";
    }
    file << "\n";

    /**
     * @brief Write the airport data to the edX file.
     */
    file << "[Airport]\n";
    for (const auto &airportData : airport)
    {
        file << "Name=" << airportName << "\n";
        file << "ICAO=" << airportICAO << "\n";
        file << "IATA=" << airportIATA << "\n";
        file << "FAA=" << airportFAA << "\n";
        file << "City=" << airportCity << "\n";
        file << "State=" << airportState << "\n";
        file << "Country=" << airportCountry << "\n";
        file << "RegionCode=" << airportRegion << "\n";
        file << "DatumLat=" << airportLat << "\n";
        file << "DatumLon=" << airportLon << "\n";
        file << "TransitionAlt=" << airportTransAlt << "\n";
        file << "TransitionLevel=" << airportTransLvl << "\n";
        file << "Elevation=" << airportElevation << "\n";
        file << "ATC=" << airportCTAF << "\n";
        file << "ATIS=" << airportATIS << "\n";
        file << "Tower=" << airportTower << "\n";
        file << "Ground=" << airportGround << "\n";
        file << "Approach=" << airportApproach << "\n";
        file << "Departure=" << airportDeparture << "\n";
        file << "Clearance=" << airportClearance << "\n\n";
    }

    /**
     * @brief Write the assets section to the edX file.
     */
    file << "[Assets]\n";
    for (const auto &asset : assets)
    {
        file << asset.id << "=" << asset.uniqueId << ", " << asset.groupId << ", " << asset.latitude << ", "
             << asset.longitude << ", " << asset.heading << ", " << asset.altitude << ", " << asset.locked << ", "
             << asset.hidden << ", " << asset.otherProperties.dump() << "\n";
    }

    file.close();
}

int projectMain()
{
    std::vector<edx::AirportInfo> airportData;

    std::vector<edx::LibraryReference> libraries;
    edx::LibraryReference lib1;
    lib1.name = "Library1";
    lib1.localPath = "path/to/library1";
    lib1.uuid = "uuid1";
    lib1.shortId = "lib1";
    lib1.entryCount = 1;
    lib1.version = "1.0";
    libraries.push_back(lib1);

    edx::LibraryReference lib2;
    lib2.name = "Library2";
    lib2.localPath = "path/to/library2";
    lib2.uuid = "uuid2";
    lib2.shortId = "lib2";
    lib2.entryCount = 2;
    lib2.version = "1.0";
    libraries.push_back(lib2);
    
    std::vector<edx::SceneAsset> assets;
    
    edx::SceneAsset asset1;
    asset1.id = "Asset001";
    asset1.uniqueId = "unique1";
    asset1.latitude = 37.618999;
    asset1.longitude = -122.375;
    asset1.altitude = 0.0;
    asset1.heading = 0.0;
    asset1.associatedLibrary = "Library1";
    asset1.layerId = "layer1";
    asset1.groupId = "group1";
    asset1.locked = false;
    asset1.hidden = false;
    asset1.selected = false;
    asset1.otherProperties = json{{"Building_Type", "Terminal"}};
    assets.push_back(asset1);

    edx::SceneAsset asset2;
    asset2.id = "Asset002";
    asset2.uniqueId = "unique2";
    asset2.latitude = 37.621;
    asset2.longitude = -122.379;
    asset2.altitude = 0.0;
    asset2.heading = 90.0;
    asset2.associatedLibrary = "Library1";
    asset2.layerId = "layer1";
    asset2.groupId = "group1";
    asset2.locked = false;
    asset2.hidden = false;
    asset2.selected = false;
    asset2.otherProperties = json{{"Object_Type", "Hangar"}};
    assets.push_back(asset2);

    edx::SceneAsset asset3;
    asset3.id = "Asset003";
    asset3.uniqueId = "unique3";
    asset3.latitude = 37.6185;
    asset3.longitude = -122.380;
    asset3.altitude = 0.0;
    asset3.heading = 45.0;
    asset3.associatedLibrary = "Library2";
    asset3.layerId = "layer1";
    asset3.groupId = "group1";
    asset3.locked = false;
    asset3.hidden = false;
    asset3.selected = false;
    asset3.otherProperties = json{{"Object_Type", "ControlTower"}};
    assets.push_back(asset3);

    writeEdxFile("test.edx",
                 "San Francisco International",
                 "1.0",
                 "11.50",
                 "San Francisco International",
                 "KSFO",
                 "SFO",
                 "SFO",
                 "San Francisco",
                 "CA",
                 "USA",
                 "US-CA",
                 37.618999,
                 -122.375,
                 18000,
                 180,
                 13,
                 118.85,
                 135.1,
                 118.85,
                 121.8,
                 125.65,
                 123.75,
                 121.65,
                 airportData,
                 libraries,
                 assets);

    return 0;
}

/// ----------------------------------------------------------------------------


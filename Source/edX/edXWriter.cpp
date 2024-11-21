
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>


#include "../X-PlaneSceneryLibrary/XPLibraryPath.h"
#include "../edX/edXProjectFile.h"


using namespace ProjectFile;

void writeEdxFile(const std::string &filename, // Add filename parameter
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
                  std::vector<ProjectFile::Airport> &airport,
                  std::vector<ProjectFile::usedLibrary> &libraries,
                  std::vector<ProjectFile::sceneAssets> &assets)
{
    std::ofstream file(filename); // Use filename parameter

    if (!file.is_open())
    {
        std::cerr << "Error opening file for writing." << std::endl;
        return;
    }


    /**
     * @brief Write the scenery section to the edX file.
     * 
     * @param sceneryName The name of the scenery.
     * @param editorVersion The version of the editor.
     * @param XPVersion The version of X-Plane.
     */
    file << "[Scenery]\n";
    file << "Name=" << sceneryName << "\n";
    file << "EditorVersion=" << editorVersion << "\n";
    file << "XPVersion=" << XPVersion << "\n\n";

    /**
     * @brief Write the libraries section to the edX file.
     * 
     * @param libraries The vector of library names.
     */
    file << "[Libraries]\n";
    for (const auto &library : libraries)
    {
        file << "Library=" << library << "\n";
    }
    file << "\n";


    /**
     * @brief Write the airport data to the edX file.
     * 
     * @param airportName The name of the airport.
     * @param airportICAO The ICAO code of the airport.
     * @param airportIATA The IATA code of the airport.
     * @param airportFAA The FAA code of the airport.
     * @param airportCity The city where the airport is located.
     * @param airportState The state where the airport is located.
     * @param airportCountry The country where the airport is located.
     * @param airportRegion The region code of the airport.
     * @param airportLat The latitude of the airport.
     * @param airportLon The longitude of the airport.
     * @param airportTransAlt The transition altitude of the airport.
     * @param airportTransLvl The transition level of the airport.
     * @param airportElevation The elevation of the airport.
     * @param airportCTAF The CTAF frequency of the airport.
     * @param airportATIS The ATIS frequency of the airport.
     * @param airportTower The tower frequency of the airport.
     * @param airportGround The ground frequency of the airport.
     * @param airportApproach The approach frequency of the airport.
     * @param airportDeparture The departure frequency of the airport.
     * @param airportClearance The clearance frequency of the airport.
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
     * 
     * @param assets The vector of scene assets.
     * @param uniqueId The unique identifier of the asset.
     * @param datum_lat The latitude of the asset.
     * @param datum_lon The longitude of the asset.
     * @param heading The heading of the asset.
     * @param altitude The altitude of the asset.
     * @param locked The locked status of the asset.
     * @param hidden The hidden status of the asset.
     * @param groupId The group ID of the asset its parented to in a layer.
     * @param properties The properties of the asset.
     */
    file << "[Assets]\n";
    for (const auto &asset : assets)
    {
        file << asset.id << "=" << asset.uniqueId << ", " << asset.groupId << ", " << asset.datum_lat << ", "
             << asset.datum_lon << ", " << asset.heading << ", " << asset.altitude << ", " << asset.locked << ", "
             << asset.hidden << ", " << asset.properties << "\n";
    }

    file.close();
}

int projectMain()
{
    std::vector<ProjectFile::Airport> airportData = {};

    std::vector<ProjectFile::usedLibrary> libraries = {{"Library1", "path/to/library1", 1},
                                                       {"Library2", "path/to/library2", 2}};
    std::vector<ProjectFile::sceneAssets> assets = {
        {"Asset001", 1, 37.618999, -122.375, 0.0, 0.0, false, false, 0, "Building_Type=Terminal"},
        {"Asset002", 2, 37.621, -122.379, 90.0, 0.0, false, false, 0, "Object_Type=Hangar"},
        {"Asset003", 3, 37.6185, -122.380, 45.0, 0.0, false, false, 0, "Object_Type=ControlTower"}};
    std::vector<ProjectFile::sceneLayers> layers = {};

    /*
    * @brief Writes the data to an edX file.
    * 
    * @param filename The name of the file to write.
    * @param sceneryName The name of the scenery.
    * @param editorVersion The version of the editor.
    * @param XPVersion The version of X-Plane.
    * @param airportName The name of the airport.
    * @param airportICAO The ICAO code of the airport.
    * @param airportIATA The IATA code of the airport.
    * @param airportFAA The FAA code of the airport.
    * @param airportCity The city where the airport is located.
    * @param airportState The state where the airport is located.
    * @param airportCountry The country where the airport is located.
    * @param airportRegion The region code of the airport.
    * @param airportLat The latitude of the airport.
    * @param airportLon The longitude of the airport.
    * @param airportTransAlt The transition altitude of the airport.
    * @param airportTransLvl The transition level of the airport.
    * @param airportElevation The elevation of the airport.
    * @param airportCTAF The CTAF frequency of the airport.
    * @param airportATIS The ATIS frequency of the airport.
    * @param airportTower The tower frequency of the airport.
    * @param airportGround The ground frequency of the airport.
    * @param airportApproach The approach frequency of the airport.
    * @param airportDeparture The departure frequency of the airport.
    * @param airportClearance The clearance frequency of the airport.
    * @param airport The vector of airport data.
    * @param libraries The vector of library names.
    * @param assets The vector of scene assets.
    */
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

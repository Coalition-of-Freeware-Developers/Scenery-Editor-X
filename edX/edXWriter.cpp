#include <fstream>
#include <iostream>
#include <string>
#include <vector>

struct Asset
{
    std::string id;
    double latitude;
    double longitude;
    double heading;
    std::string properties; // Other properties as a string for simplicity
};

void writeEdxFile(const std::string &filename,
                  const std::string &sceneryName,
                  const std::string &editorVersion,
                  const std::string &xPlaneVersion,
                  const std::string &airportICAO,
                  const std::string &airportName,
                  int airportElevation,
                  const std::vector<std::string> &libraries,
                  const std::vector<Asset> &assets)
{
    std::ofstream file(filename);

    if (!file.is_open())
    {
        std::cerr << "Error opening file for writing." << std::endl;
        return;
    }

    // Write scenery section
    file << "[Scenery]\n";
    file << "Name=" << sceneryName << "\n";
    file << "EditorVersion=" << editorVersion << "\n";
    file << "XPVersion=" << xPlaneVersion << "\n\n";

    // Write airport section
    file << "[Airport]\n";
    file << "ICAO=" << airportICAO << "\n";
    file << "Name=" << airportName << "\n";
    file << "Elevation=" << airportElevation << "\n\n";

    // Write libraries section
    file << "[Libraries]\n";
    for (const auto &library : libraries)
    {
        file << "Library=" << library << "\n";
    }
    file << "\n";

    // Write assets section
    file << "[Assets]\n";
    for (const auto &asset : assets)
    {
        file << asset.id << "=" << asset.latitude << ", " << asset.longitude << ", " << asset.heading << ", "
             << asset.properties << "\n";
    }

    file.close();
}

int main()
{
    std::vector<std::string> libraries = {"Laminar Research", "Custom Library"};
    std::vector<Asset> assets = {{"Asset001", 37.618999, -122.375, 0.0, "Building_Type=Terminal"},
                                 {"Asset002", 37.621, -122.379, 90.0, "Object_Type=Hangar"},
                                 {"Asset003", 37.6185, -122.380, 45.0, "Object_Type=ControlTower"}};

    writeEdxFile("example.edX",
                 "MySceneryProject",
                 "1.0",
                 "12.00",
                 "KSFO",
                 "San Francisco International Airport",
                 13,
                 libraries,
                 assets);

    return 0;
}

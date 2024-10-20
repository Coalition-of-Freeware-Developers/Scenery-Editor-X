#pragma once

#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <string>
#include <vector>
#include <tuple>
#include <filesystem>
#include <map>

namespace ProjectFile
{
    struct Airport
    {
        std::string airport_name;
        std::string icao_code;
        std::string iata_code;
        std::string faa_code;
        std::string city;
        std::string state;
        std::string country;
        std::string region_code;
        double datum_lat;
        double datum_lon;
        int transition_alt;  
        int transition_level;
        int elevation;

        int atis;
        int tower;
        int ctaf;
        int ground;
        int approach;
        int departure;
        int clearance;
    };
    
    // Library structure: Lists librarys used and objects referenced in scenery project.
    struct usedLibrary
    {
        std::string library_name;
        std::string library_path;
        std::int32_t library_version;
    };

    // Define the operator<< for usedLibrary
    std::ostream &operator<<(std::ostream &os, const usedLibrary &lib)
    {
        os << lib.library_name << ", " << lib.library_path << ", " << lib.library_version;
        return os;
    }

    struct sceneAssets
    {
        std::string id;
        double uniqueId;
        double groupId;
        double datum_lat;
        double datum_lon;
        double heading;
        double altitude;
        bool locked;
        bool hidden;

        std::string properties; // Other properties as a string for simplicity
    };
    
    struct sceneLayers
    {
        std::string layerId;
        bool locked;
        bool hidden;
        double groupId;
        sceneAssets assets;
    
        std::string properties; // Other properties as a string for simplicity
    };
}

#endif

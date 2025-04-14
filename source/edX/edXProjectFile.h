#pragma once

#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include "edXConfig.h"

#include <filesystem>
#include <map>
#include <string>
#include <tuple>
#include <vector>


namespace ProjectFile
{
    struct EDX_API projectFile
    {
        std::string filename;
        std::string sceneryName;
        std::string editorVersion;
        std::string XPVersion;
    };
    
    struct EDX_API Airport
    {
        std::string airportName;
        std::string airportICAO;
        std::string airportIATA;
        std::string airportFAA;
        std::string airportCity;
        std::string airportState;
        std::string airportCountry;
        std::string airportRegion;
        double airportLat;
        double airportLon;
        int airportTransAlt;
        int airportTransLvl;
        int airportElevation;
        double airportCTAF;
        double airportATIS;
        double airportTower;
        double airportGround;
        double airportApproach;
        double airportDeparture;
        double airportClearance;
    };
    
    // Library structure: Lists librarys used and objects referenced in scenery project.
    struct EDX_API usedLibrary
    {
        std::string library_name;
        std::string library_path;
        std::int32_t library_version;
    };
    
    // Define the operator<< for usedLibrary
    inline std::ostream &operator<<(std::ostream &os, const usedLibrary &lib)
    {
        os << lib.library_name << ", " << lib.library_path << ", " << lib.library_version;
        return os;
    }
    
    struct EDX_API sceneAssets
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
    
    struct EDX_API sceneLayers
    {
        std::string layerId;
        bool locked;
        bool hidden;
        double groupId;
        sceneAssets assets;
    
        std::string properties; // Other properties as a string for simplicity
    };
} // namespace ProjectFile

#endif

#pragma once

#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <string>
#include <vector>
#include <tuple>
#include <filesystem>
#include <map>

/*
* This file contains the structures and classes that represent the data in the project. These are used directly in the editor, and in the project file.
* 
* Notes:
* UUIDs for source control/collaboration
* Tags for bulk selecting/editing of elements
* All elements have a string to identify what library they come from
*/

namespace ProjectFormat
{
    struct ProjectFormat
    {
        std::string filename;
        std::string sceneryName;
        std::string editorVersion;
        std::string XPVersion;
    };
    
    struct Airport
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
    struct UsedLibrary
    {
        std::string library_name;
        std::string library_path;
        std::int32_t library_version;
    };
    
    // Define the operator<< for usedLibrary
    std::ostream &operator<<(std::ostream &os, const UsedLibrary &lib)
    {
        os << lib.library_name << ", " << lib.library_path << ", " << lib.library_version;
        return os;
    }
    
    //Represents a group in the UI.
    class Group
    {
        //Note to Connor: STOP! We need UUIDs for source control, if we merge and the folder name changes, *we have a problem*. So, we need to keep UUIDs, even for the folders!
        //boost::uuid ID;
        //boost::uuid ParentID;         //The group this group is in
        std::string Name; //The name of the group
        bool Visible;     //True if the element is visible
        bool Locked;      //True if the element is locked
        std::vector<std::string>
            Tags; //Tags for the object. These are user generated and used for selecting/editing other items with the same tag.
    };
    
    //Parent class that children can inherit from. Contains global elements such as the source control UUID, group name, and tags
    class Element
    {
    public:
        //boost::uuid ID;
        std::vector<std::string>
            Tags; //Tags for the object. These are user generated and used for selecting/editing other items with the same tag.
        //boost::uuid ParentID;         //The group this object is in
        bool Visible;            //True if the element is visible
        bool Locked;             //True if the element is locked
        std::string FromLibrary; //The library this object is from.
    };
    
    //Represents an X-Plane .obj
    class Object : public Element
    {
    public:
        double Lat{0};             //Decimal degrees. Negative for south.
        double Lon{0};             //Decimal degrees. Negative for west.
        double Alt{0};             //Meters
        bool Agl{true};            //False for MSL, true for AGL
        bool PickAlt{false};       //True if the altitude is picked, if false X-Plane will put it directly on the ground.
        double Heading;            //Degrees. 0 is north, 90 is east, ect.
        std::string Resource;      //Path to the object file
        std::string ShowAtSetting; //The object setting to show this object. 1-6 (1 is lowest, 6 is highest)
    };
    
    //Represents an X-Plane .pol
    class Polygon : public Element
    {
    public:
        //Geometery::Winding Points;              //The boundary of the polygon
        //std::vector<Geometery::Winding> Holes;    //Holes in the polygon
        double Heading{0};    //Texture heading. Degrees. 0 is north, 90 is east, ect.
        std::string Resource; //Path to the .pol file
    };
    
    //Represents an X-Plane .lin Line (not an airport line that goes into the apt.dat
    class Line : public Element
    {
    public:
        //std::vector<Geometery::Point> Points;  //The points that make up the line
        bool Closed{false};   //True if the line is closed
        std::string Resource; //Path to the .lin file
    };
    
    //Represents an X-Plane Facade
    class Facade : public Element
    {
    public:
        //std::vector<Geometery::Winding> Points;   //The boundary of the facade. Each BezeirPoint in the Winding has a map of properties. The keys wall specify the wall idx that is choosen.
        double Heading{0};    //Texture heading. Degrees. 0 is north, 90 is east, ect.
        std::string Resource; //Path to the .fac file
        bool PickWalls{
            true}; //True if the walls are picked, if false X-Plane will pick them at runtime. We will default to wall idx 0.
    };
    
    //Represents an X-Plane .str (this is a "string" or chain of objects, not text)
    class String : public Element
    {
    public:
        //std::vector<Geometery::Point> Points;  //The points that make up the string
        std::string Resource; //Path to the .str file
    };
    
    //Represents an X-Plane Forest
    class Forest : public Element
    {
    public:
        //std::vector<Geometery::Point> Points;  //The points that make up the forest
        std::string Resource;       //Path to the .for file
        char FillMode{0};           //The fill mode. 0 is fill, 1 is line, 2 is point.
        unsigned char Density{255}; //The density of the forest. 0 is lowest, 255 is highest.
    };
    
    class Exclusion : public Element
    {
    public:
        //Exclusion types
        static const uint16_t OBJECTS = 0b0000000000000001;
        static const uint16_t FACADES = 0b0000000000000010;
        static const uint16_t FORESTS = 0b0000000000000100;
        static const uint16_t BEACHES = 0b0000000000001000;
        static const uint16_t ROADS = 0b0000000000010000;
        static const uint16_t LINES = 0b0000000000100000;
        static const uint16_t POLYGONS = 0b0000000001000000;
        static const uint16_t STRINGS = 0b0000000010000000;
    
        //Geometery::Winding Points;  //The boundary of the exclusion. Points are optional, if they are not used, just the bounding box will be used. More notes on this latter.
        double North{0}, South{0}, East{0}, West{0}; //The bounding box of the exclusion
        uint16_t Types{0};
    };
    
    //Apt.dat elements
    
    class AirportLine : public Element
    {
    public:
        //std::vector<Geometery::Point> Points; //The points that make up the line. Each BezeirPoint in the Winding has a map of properties. The keys line and light specify which lines and lights are used for each node
        bool Closed; //True if the line is closed
    };
    
    class Taxiway : public Element
    {
    public:
        //std::vector<Geometery::Point> Points; //The points that make up the taxiway. Each BezeirPoint in the Winding has a map of properties. The keys light and line specify what lights and lines are on the border of the taxiway
        std::string Name;    //The name of the taxiway
        std::string Surface; //The surface of the taxiway
        double Roughness;    //The roughness of the taxiway
    };
    
    class Runway : public Element
    {
        //Meters. -1 if no width
        double ShoulderWidth{-1};
        //Meters
        double Width;
        double Lat1;
        double Lon1;
        double Lat2;
        double Lon2;
        //Blastpad and displaced threshold lengths in meters
        double BlastpadLength1;
        double BlastpadLength2;
        double DisplacedThresholdLength1;
        double DisplacedThresholdLength2;
        //Ie 18R
        std::string Name1;
        //ie 36L
        std::string Name2;
    
        //Derived values
        double Length;
        double Heading;
        double LatCenter;
        double LonCenter;
    
        inline void CalculateDerivedValues()
        {
            //Length = GetWorldDistance(Lat1, Lon1, Lat2, Lon2);
            //Heading = GetWorldHeading(Lat1, Lon1, Lat2, Lon2);
            LatCenter = (Lat1 + Lat2) / 2;
            LonCenter = (Lon1 + Lon2) / 2;
        }
    };
    
    class Helipad : public Element
    {
        double Lat;
        double Lon;
        std::string SurfaceMaterial;
        std::string Markings;
        std::string ShoulderMaterial;
        double Roughness;
        bool EdgeLights;
    };
    
    class RampStart : public Element
    {
    public:
        unsigned char Size;
        //Ie GA, AIRLINE, CARGO, MILITARY
        unsigned char OperationType;
        //Ie FIGHTERS, HELICOPTERS, PROPS, TURBOPROPS, JETS, HEAVY_JETS
        unsigned char AircraftType;
        std::string AirlineCodes;
        std::string Name;
        double Lat;
        double Lon;
        double Heading;
        //Ie TIE_DOWN, GATE, HANGAR, MISC
        unsigned char StartType;
    
        //Operation types
        static const unsigned char GA{0b00000001};
        static const unsigned char AIRLINE{0b00000010};
        static const unsigned char CARGO{0b00000100};
        static const unsigned char MILITARY{0b00001000};
    
        //Aircraft types
        static const unsigned char FIGHTERS{0b00000001};
        static const unsigned char HELICOPTERS{0b00000010};
        static const unsigned char PROPS{0b00000100};
        static const unsigned char TURBOPROPS{0b00001000};
        static const unsigned char JETS{0b00010000};
        static const unsigned char HEAVY_JETS{0b00100000};
    
        //Aircarft sizes
        static const unsigned char SIZE_A{0b00000001};
        static const unsigned char SIZE_B{0b00000010};
        static const unsigned char SIZE_C{0b00000100};
        static const unsigned char SIZE_D{0b00001000};
        static const unsigned char SIZE_E{0b00010000};
        static const unsigned char SIZE_F{0b00100000};
    
        //Start types
        static const char MISC{0b00000001};
        static const char GATE{0b00000010};
        static const char TIE_DOWN{0b00000100};
        static const char HANGAR{0b00001000};
    };
    
    class Freqency : public Element
    {
    public:
        //Frequency in Hz
        std::string Freq;
        unsigned char Type;
        std::string Name;
    
        //AWOS, CTAF, CLEARANCE, GROUND, TOWER, APPROACH, DEPARTURE
        static const unsigned char AWOS{0};
        static const unsigned char CTAF{1};
        static const unsigned char CLEARANCE{2};
        static const unsigned char GROUND{3};
        static const unsigned char TOWER{4};
        static const unsigned char APPROACH{5};
        static const unsigned char DEPARTURE{6};
        static const unsigned char OTHER{7};
    };
    
    class TowerView : public Element
    {
    public:
        double Lat;
        double Lon;
        std::string Name;
    };
    
    class Boundary : public Element
    {
    public:
        //Geometery::Winding Points;  //The boundary of the airport
    };
    
    class Windsock : public Element
    {
    public:
        double Lat;
        double Lon;
        bool Lit;
    };
    
    class Taxisign : public Element
    {
    public:
        double Lat;
        double Lon;
        double Heading;
        unsigned char Type;
        std::vector<char> Text;
    
        //Defines for type
        //Sizes are small taxiway, medium taxiway, large taxiway, large distance remaining, small distance remaining
        static const unsigned char SMALL_TAXIWAY{0};
        static const unsigned char MEDIUM_TAXIWAY{1};
        static const unsigned char LARGE_TAXIWAY{2};
        static const unsigned char LARGE_DISTANCE_REMAINING{3};
        static const unsigned char SMALL_DISTANCE_REMAINING{4};
    
        //Defines for characters and formats
        // Formatting: yellow, red, black, back, distance
        // Characters are:
        // arrows: left, left upper, upper, right upper, right, right lower, lower, left lower
        // roman numerals: I, II, III
        // Special characters: critical, hazard, no-entry, safety
        //We can define these with ASCII codes 0 - 31, everything else is normal ASCII
        static const char YELLOW{0};
        static const char RED{1};
        static const char BLACK{2};
        static const char BACK{3};
        static const char LEFT{4};
        static const char LEFT_UPPER{5};
        static const char UPPER{6};
        static const char RIGHT_UPPER{7};
        static const char RIGHT{8};
        static const char RIGHT_LOWER{9};
        static const char LOWER{10};
        static const char LEFT_LOWER{11};
        static const char I{12};
        static const char II{13};
        static const char III{14};
        static const char CRITICAL{15};
        static const char HAZARD{16};
        static const char NO_ENTRY{17};
        static const char SAFETY{18};
        static const char DISTANCE{19};
    };
    
    class TaxirouteNode : public Element
    {
    public:
        double Lat;
        double Lon;
        char Mode;
    
        static const char INIT{0};
        static const char END{1};
        static const char BOTH{2};
        static const char JUNC{3};
    
        std::string Name;
    };
    
    class TaxirouteActivityRule : public Element
    {
    public:
        char Type;
        char Runway1;
        char Runway2;
    
        //Types
        static const char APPROACH{0};
        static const char DEPARTURE{1};
    };
    
    class TaxirouteEdge : public Element
    {
    public:
        size_t idx1;
        size_t idx2;
        char Size;
        bool Oneway;
        bool Vehicle;
        std::string Name;
        std::vector<TaxirouteActivityRule> ActivityRules;
    };
    
    class ServiceVehicle : public Element
    {
    public:
        double Lat;
        double Lon;
        double Heading;
        char Type;
        double Arguments;
        std::string CustomAsset;
    
        static const char BAGGAGE_CARTS{0};
        static const char BAGGAGE_LOADER{1};
        static const char GPU{2};
        static const char CATERING{3};
        static const char FUEL_TRUCK_GA{4};
        static const char FUEL_TRUCK_JET{5};
        static const char FUEL_TRUCK_AIRLINERS{6};
        static const char LIMO{7};
        static const char FERRARI{8};
        static const char CAR{9};
    };
    
    class ServiceVehicleStop : public Element
    {
    public:
        double Lat;
        double Lon;
        double Heading;
        std::vector<char> AllowedVehicles;
    };
} // namespace ProjectFile

#endif

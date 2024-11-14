//Project:	CPP-Libs
//Author:	Connor Russell
//Date:		3/15/2021
//Module:	DSFTile
//Purpose:	Abstract the adding of scenery elements and creating a DSF

//Compile once
#pragma once

#include "SystemHeaders.h"
#include "Node.h"
#include "GeoUtils.h"

//Exclusion definitions
#define EXCLUDE_OBJ "sim/exclude_obj"
#define EXCLUDE_FAC "sim/exclude_fac"
#define EXCLUDE_FOR "sim/exclude_for"
#define EXCLUDE_BCH "sim/exclude_bch"
#define EXCLUDE_NET "sim/exclude_net"
#define EXCLUDE_LIN "sim/exclude_lin"
#define EXCLUDE_POL "sim/exclude_pol"
#define EXCLUDE_STR "sim/exclude_str"

//Forest definition
#define FOR_FILL_ZONE 0
#define FOR_FILL_LINE 1
#define FOR_FILL_POINTS 2

/// <summary>
/// Internal data structures for DSFTile class. Not meant for standalone use.
/// </summary>
namespace DSF
{
	inline void ToTxt(std::filesystem::path InDSFPath, std::filesystem::path InDSFToolPath)
	{
		std::string strConvertedPath = InDSFPath.string() + ".txt";
		std::string strConvertCmd = InDSFToolPath.string() + " --dsf2text \"" + InDSFPath.string() + "\" \"" + strConvertedPath + "\"";
		std::system(strConvertCmd.c_str());
	}

	inline void ToDsf(std::filesystem::path InDSFPath, std::filesystem::path InDSFToolPath)
	{
		std::string strConvertedPath = InDSFPath.string() + ".dsf";
		std::string strConvertCmd = InDSFToolPath.string() + " --dsf2text \"" + InDSFPath.string() + "\" \"" + strConvertedPath + "\"";
		std::system(strConvertCmd.c_str());
	}

	class Feature
	{
	protected:
		unsigned char Type;
	public:
		std::string Airport;
		std::string Resource;

		//Type codes
		static const unsigned char FOREST = 0;
		static const unsigned char FACADE = 1;
		static const unsigned char OBJECT = 2;
		static const unsigned char POLYGON = 3;
		static const unsigned char STRING = 4;
		static const unsigned char LINE = 5;
		static const unsigned char NETWORK = 6;
		static const unsigned char EXCLUSION = 7;
		static const unsigned char TERRAIN_PATCH = 8;

		//Sorting, based on the Airport. We need this so they can be grouped by, and therefore excluded by, airport
		bool operator < (const Feature& Other) const
		{
			return Airport < Other.Airport;
		}
	};

	class PolygonalFeature : public Feature
	{
	public:
		XSLGeoutils::Winding Vertices;
		std::vector<XSLGeoutils::Winding> Holes;
		bool Curved;
		bool ExplicitUVs;
	};

	//Structs that encapsulate the data for scenery elements
	//Terrain patch
	class TerrainPatch : public Feature
	{
	public:
		std::vector<XSLGeoutils::Winding> Primitives;	//What even is a primitive? Why are they grouped this way?
	};

	//Forests
	class Forest : public PolygonalFeature
	{
	public:

		Forest() { Type = FOREST; }

		int FillMode;			//Fill mode (0 is zone, 1 is line, 2 is point mode)
		double Density;			//Forest density
	};

	//Facades
	class Facade : public PolygonalFeature
	{
	public:

		Facade() { Type = FACADE; }

		int Height;				//Facade height
		bool Closed;			//Is this facade closed?
		bool PickWalls;			//Are we picking the walls? If so, set the "wall" attribute in each node in the Winding
	};

	//Object
	class Object : public Feature
	{
	public:

		Object() : Feature() { Type = OBJECT; }

		double Lat;			//Lat coord
		double Lon;			//Lon coord
		double Heading;		//Heading
		double Alt;			//Altitude
	};

	//Polygon
	class Polygon : public PolygonalFeature
	{
	public:

		Polygon() { Type = POLYGON; }

		double Heading;			//Heading
	};

	//String
	class String : public PolygonalFeature
	{
	public:

		String() { Type = STRING; }

		double Spacing;			//Heading
	};

	//Polygon
	class Line : public PolygonalFeature
	{
	public:

		Line() { Type = LINE; }

		bool Closed{ false };
	};

	//Exclusions
	class Exclusion
	{
	public:

		double West;			//Western bound
		double South;			//Southern bound
		double East;			//Eastern bound
		double North;			//Northern bound
		std::string Type;			//Exclude type
	};

	//Road Segment
	class NetworkSegment : public Feature
	{
	public:

		NetworkSegment() { Type = NETWORK; }

		std::vector<double> Lats;		//Lats
		std::vector<double> Lons;		//Lons
		std::vector<double> Elevations;	//Elevations
		std::string Subtype;				//Road subtype
		int StartJunctionID;		//Junction at the start
		int EndJunctionID;			//Junction at the end
	};

	//Class definition
	class Tile
	{
		//Private members
	private:
		//Thread safety flags
		std::mutex mtForests;
		std::mutex mtFacades;
		std::mutex mtObjects;
		std::mutex mtPolygons;
		std::mutex mtStrings;
		std::mutex mtLines;
		std::mutex mtRoads;
		std::mutex mtExcludes;

		//Public methods
	public:
		std::vector<DSF::Forest> vctForests;
		std::vector<DSF::Facade> vctFacades;
		std::vector<DSF::Object> vctObjects;
		std::vector<DSF::Polygon> vctPolygons;
		std::vector<DSF::String> vctStrings;
		std::vector<DSF::Line> vctLines;
		std::vector<DSF::NetworkSegment> vctRoads;
		std::vector<DSF::Exclusion> vctExcludes;
		std::vector<DSF::TerrainPatch> vctTerPatches;

		//Default constructor
		inline Tile() {}

		//Copy constructor
		inline Tile(const Tile& Other)
		{
			vctForests = Other.vctForests;
			vctFacades = Other.vctFacades;
			vctObjects = Other.vctObjects;
			vctPolygons = Other.vctPolygons;
			vctStrings = Other.vctStrings;
			vctLines = Other.vctLines;
			vctRoads = Other.vctRoads;
			vctExcludes = Other.vctExcludes;
			vctTerPatches = Other.vctTerPatches;
		}

		//Assignment operator
		inline Tile& operator = (const Tile& Other)
		{
			vctForests = Other.vctForests;
			vctFacades = Other.vctFacades;
			vctObjects = Other.vctObjects;
			vctPolygons = Other.vctPolygons;
			vctStrings = Other.vctStrings;
			vctLines = Other.vctLines;
			vctRoads = Other.vctRoads;
			vctExcludes = Other.vctExcludes;
			vctTerPatches = Other.vctTerPatches;
			return *this;
		}

		inline bool IsEmpty()
		{
			bool Empty = true;
			Empty &= vctForests.empty();
			Empty &= vctFacades.empty();
			Empty &= vctObjects.empty();
			Empty &= vctPolygons.empty();
			Empty &= vctStrings.empty();
			Empty &= vctLines.empty();
			Empty &= vctRoads.empty();
			Empty &= vctExcludes.empty();
			return Empty;
		}

		/// <summary>
		/// Adds forest to DSF. No passed in params are modified
		/// </summary>
		/// <param name="For">Forest to add</param>
		void AddForest(const DSF::Forest& For);

		/// <summary>
		/// Adds facade to DSF. No passed in params are modified
		/// </summary>
		/// <param name="Fac">Facade to add</param>
		void AddFacade(const DSF::Facade& Fac);

		/// <summary>
		/// Adds object to DSF.
		/// </summary>
		/// <param name="Obj">Object to add</param>
		void AddObject(const DSF::Object& Obj);

		/// <summary>
		/// Adds polygon to DSF. No passed in params are modified
		/// </summary>
		/// <param name="Pol">Polygon to add</param>
		void AddPolygon(const DSF::Polygon& Pol);

		/// <summary>
		/// Adds string to DSF. No passed in params are modified
		/// </summary>
		/// <param name="Str">String to add</param>
		void AddString(const DSF::String& Str);

		/// <summary>
		/// Adds line to DSF. No passed in params are modified
		/// </summary>
		/// <param name="Lin">Line to add</param>
		void AddLine(const DSF::Line& Lin);

		/// <summary>
		/// Adds a road segment to tile.
		/// </summary>
		/// <param name="Seg">Road segment to add</param>
		void AddRoadSegment(const DSF::NetworkSegment& Seg);

		/// <summary>
		/// Adds exclusion zone to DSF
		/// </summary>
		/// <param name="Ex">Exclusion zone to add</param>
		void AddExclude(const DSF::Exclusion& Ex);

		/// <summary>
		/// Writes DSF contents to DSF file. DSF is in text format and must be run through DSFTool.
		/// </summary>
		/// <param name="InPath">Path to write DSF to</param>
		/// <param name="South">South tile coord</param>
		/// <param name="West">West tile coord</param>
		std::filesystem::path Write(std::filesystem::path InPath, int South, int West);

		/// <summary>
		/// Reads DSF contents from DSF file. DSF is in text format and must be run through DSFTool.
		/// </summary>
		/// <param name="InPath">Path to the DSF</param>
		/// <param name="InDSFToolPath">Path to DSFTool</param>
		/// <returns>True on success, false on failure</returns>
		bool Read(std::filesystem::path InPath, std::filesystem::path InDSFToolPath);

		/// <summary>
		/// Gains exclusive access on all locks, then clears vectors, and releases access.
		/// </summary>
		~Tile();

		/// <summary>
		/// Copies all other DSFTile scenery features into this DSFTile. Gains exclusive access on all locks. Releases locks when done
		/// </summary>
		/// <param name="Other">Reference to other DSFTile</param>
		void operator += (DSF::Tile& Other);
	};
}

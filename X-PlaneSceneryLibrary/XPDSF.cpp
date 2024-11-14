//Module:	DSFTile
//Author:	Connor Russell
//Date:		9/10/2024 1:38:16 PM
//Purpose:	Implements DSFTile.h

//Compile once
#pragma once

//Include necessary headers
#include "SystemHeaders.h"
#include "XPDSF.h"
#include "TextUtils.h"
#include "XSLMacros.h"

using namespace TextUtils;
namespace geo = XSLGeoutils;

//Compile once
#pragma once

/// <summary>
/// Adds forest to DSF. No passed in params are modified
/// </summary>
/// <param name="For">Forest to add</param>
void DSF::Tile::AddForest(const DSF::Forest& For)
{
	//Get access
	mtForests.lock();

	if (For.Vertices.Nodes.size() < 3)
	{
		//Release access
		mtForests.unlock();
		return;
	}

	//Push this new forest into the forest vector
	vctForests.push_back(For);

	DSF::Forest& NewForest = vctForests.back();

	//Check to make sure the last point isn't a duplicate of the first
	if (NewForest.Vertices.Nodes.back().Colocated(NewForest.Vertices.Nodes.back()))
	{
		NewForest.Vertices.Nodes.pop_back();
	}

	//Release access
	mtForests.unlock();
}

/// <summary>
/// Adds facade to DSF. No passed in params are modified
/// </summary>
/// <param name="Fac">Facade to add</param>
void DSF::Tile::AddFacade(const DSF::Facade& Fac)
{
	//Get access
	mtFacades.lock();

	if (Fac.Vertices.Nodes.size() < 3)
	{
		//Release access
		mtFacades.unlock();
		return;
	}

	vctFacades.push_back(Fac);

	DSF::Facade& NewFacade = vctFacades.back();

	//Release access
	mtFacades.unlock();
}

/// <summary>
/// Adds object to DSF.
/// </summary>
/// <param name="Obj">Object to add</param>
void DSF::Tile::AddObject(const DSF::Object& Obj)
{
	//Get access
	mtObjects.lock();

	//Push this new object into the object vector
	vctObjects.push_back(Obj);

	//Release access
	mtObjects.unlock();
}

/// <summary>
/// Adds polygon to DSF. No passed in params are modified
/// </summary>
/// <param name="Pol">Polygon to add</param>
void DSF::Tile::AddPolygon(const DSF::Polygon& Pol)
{
	//Get access
	mtPolygons.lock();

	if (Pol.Vertices.Nodes.size() < 3)
	{
		mtPolygons.unlock();
		return;
	}

	//Push this new polygon into the polygon vector
	vctPolygons.push_back(Pol);

	DSF::Polygon& NewPolygon = vctPolygons.back();

	//Reverse if needed


	//Release access
	mtPolygons.unlock();
}

/// <summary>
/// Adds string to DSF. No passed in params are modified
/// </summary>
/// <param name="Str">String to add</param>
void DSF::Tile::AddString(const DSF::String& Str)
{
	//Get access
	mtStrings.lock();

	//Make sure there are at least 2 points
	if (Str.Vertices.Nodes.size() < 2)
	{
		//Release access
		mtStrings.unlock();
		return;
	}

	//Push this new string into the string vector
	vctStrings.push_back(Str);

	//Release access
	mtStrings.unlock();
}

/// <summary>
/// Adds line to DSF. No passed in params are modified
/// </summary>
/// <param name="Lin">Line to add</param>
void DSF::Tile::AddLine(const DSF::Line& Lin)
{
	//Get access
	mtLines.lock();

	//Make sure there are at least 2 points
	if (Lin.Vertices.Nodes.size() < 2)
	{
		//Release access
		mtLines.unlock();
		return;
	}

	//Push this new line into the line vector
	vctLines.push_back(Lin);

	//Release access
	mtLines.unlock();
}

/// <summary>
/// Adds a road segment to tile.
/// </summary>
/// <param name="Seg">Road segment to add</param>
void DSF::Tile::AddRoadSegment(const DSF::NetworkSegment& Seg)
{
	//Get access
	mtRoads.lock();

	//Push it into the road vector
	vctRoads.push_back(Seg);

	//Release access
	mtRoads.unlock();
}

/// <summary>
/// Adds exclusion zone to DSF
/// </summary>
/// <param name="Ex">Exclusion zone to add</param>
void DSF::Tile::AddExclude(const DSF::Exclusion& Ex)
{
	//Get access
	mtExcludes.lock();

	//Add it to the vector
	vctExcludes.push_back(Ex);

	//Release access
	mtExcludes.unlock();
}

/// <summary>
/// Writes DSF contents to DSF file. DSF is in text format and must be run through DSFTool.
/// </summary>
/// <param name="Path">Path to write DSF to</param>
/// <param name="South">South tile coord</param>
/// <param name="West">West tile coord</param>
std::filesystem::path DSF::Tile::Write(std::filesystem::path InPath, int South, int West)
{
	//Get access
	mtForests.lock();
	mtFacades.lock();
	mtObjects.lock();
	mtPolygons.lock();
	mtStrings.lock();
	mtLines.lock();
	mtRoads.lock();
	mtExcludes.lock();

	std::filesystem::path OutPath;

	//Define a vector to hold all the resources
	std::vector<std::string> vctAllResources;
	std::vector<std::string> vctPolygonResources;
	std::vector<std::string> vctObjectResources;
	std::vector<std::string> vctAirports;

	//This is where we define the assets
	{
		//Add forest assets
		for (size_t i = 0; i < vctForests.size(); i++)
		{
			//Check if there is a match
			if (std::find(vctAllResources.begin(), vctAllResources.end(), vctForests[i].Resource) == vctAllResources.end())
			{
				//If not, add this to the list of assets
				vctAllResources.push_back(vctForests[i].Resource);
			}

			//Check if there is an airport name match, if not add it to the airport name vector
			if (std::find(vctAirports.begin(), vctAirports.end(), vctForests[i].Airport) == vctAirports.end())
			{
				//If not, add this to the list of assets
				vctAirports.push_back(vctForests[i].Airport);
			}
		}

		//Add facade assets
		for (size_t i = 0; i < vctFacades.size(); i++)
		{
			if (std::find(vctAllResources.begin(), vctAllResources.end(), vctFacades[i].Resource) == vctAllResources.end())
			{
				//If not, add this to the list of assets
				vctAllResources.push_back(vctFacades[i].Resource);
			}

			//Check if there is an airport name match, if not add it to the airport name vector
			if (std::find(vctAirports.begin(), vctAirports.end(), vctFacades[i].Airport) == vctAirports.end())
			{
				//If not, add this to the list of assets
				vctAirports.push_back(vctFacades[i].Airport);
			}
		}

		//Add object assets
		for (size_t i = 0; i < vctObjects.size(); i++)
		{
			if (std::find(vctAllResources.begin(), vctAllResources.end(), vctObjects[i].Resource) == vctAllResources.end())
			{
				//If not, add this to the list of assets
				vctAllResources.push_back(vctObjects[i].Resource);
			}

			//Check if there is an airport name match, if not add it to the airport name vector
			if (std::find(vctAirports.begin(), vctAirports.end(), vctObjects[i].Airport) == vctAirports.end())
			{
				//If not, add this to the list of assets
				vctAirports.push_back(vctObjects[i].Airport);
			}
		}

		//Add polygon assets
		for (size_t i = 0; i < vctPolygons.size(); i++)
		{
			if (std::find(vctAllResources.begin(), vctAllResources.end(), vctPolygons[i].Resource) == vctAllResources.end())
			{
				//If not, add this to the list of assets
				vctAllResources.push_back(vctPolygons[i].Resource);
			}

			//Check if there is an airport name match, if not add it to the airport name vector
			if (std::find(vctAirports.begin(), vctAirports.end(), vctPolygons[i].Airport) == vctAirports.end())
			{
				//If not, add this to the list of assets
				vctAirports.push_back(vctPolygons[i].Airport);
			}
		}

		//Add string assets
		for (size_t i = 0; i < vctStrings.size(); i++)
		{
			if (std::find(vctAllResources.begin(), vctAllResources.end(), vctStrings[i].Resource) == vctAllResources.end())
			{
				//If not, add this to the list of assets
				vctAllResources.push_back(vctStrings[i].Resource);
			}

			//Check if there is an airport name match, if not add it to the airport name vector
			if (std::find(vctAirports.begin(), vctAirports.end(), vctStrings[i].Airport) == vctAirports.end())
			{
				//If not, add this to the list of assets
				vctAirports.push_back(vctStrings[i].Airport);
			}
		}

		//Add line assets
		for (size_t i = 0; i < vctLines.size(); i++)
		{
			if (std::find(vctAllResources.begin(), vctAllResources.end(), vctLines[i].Resource) == vctAllResources.end())
			{
				//If not, add this to the list of assets
				vctAllResources.push_back(vctLines[i].Resource);
			}

			//Check if there is an airport name match, if not add it to the airport name vector
			if (std::find(vctAirports.begin(), vctAirports.end(), vctLines[i].Airport) == vctAirports.end())
			{
				//If not, add this to the list of assets
				vctAirports.push_back(vctLines[i].Airport);
			}
		}
	}

	//Now sort our asset list if it isn't empty
	if (vctAllResources.size() > 0 || vctRoads.size() > 0)
	{
		//Seperate our polygon and object resources
		for (size_t i = 0; i < vctAllResources.size(); i++)
		{
			//Store this asset in a string
			std::string strAsset = vctAllResources[i];

			//Check to make sure this isn't an empty string
			if (strAsset.length() > 4)
			{
				//Check what kind of resource this is
				if (strAsset.ends_with(".obj"))
				{
					//Put it in the object vector
					vctObjectResources.push_back(strAsset);
				}
				else
				{
					//Otherwise put it in the polygon vector
					vctPolygonResources.push_back(strAsset);
				}
			}

			//Otherwise add a dummy forest - WHY DID I EVER ADD THIS WHAT IS IT FOR?????????????? XGrinder issue bandaid maybe?
			else
			{
				//Otherwise put it in the polygon vector
				vctPolygonResources.push_back("lib/vegetation/trees/deciduous/birch_medium.for");
			}
		}

		//Sort the asset vectors
		std::sort(vctAllResources.begin(), vctAllResources.end());
		std::sort(vctPolygonResources.begin(), vctPolygonResources.end());
		std::sort(vctObjectResources.begin(), vctObjectResources.end());


		//Define a stringstream to hold our DSF data
		std::stringstream sstrDSF;

		//Set the format
		sstrDSF << std::setprecision(12) << std::showpoint;

		//Now add the introductory DSF data
		sstrDSF << "I\n800\nDSF2TEXT\n\n"
			<< "PROPERTY sim/west " << std::to_string(West) << std::endl
			<< "PROPERTY sim/east " << std::to_string(West + 1) << std::endl
			<< "PROPERTY sim/north " << std::to_string(South + 1) << std::endl
			<< "PROPERTY sim/south " << std::to_string(South) << std::endl
			<< "PROPERTY sim/planet earth" << std::endl
			<< "PROPERTY sim/creation_agent DSFTileClass" << std::endl
			<< "PROPERTY laminar/internal_revision 0" << std::endl
			<< "PROPERTY sim/overlay 1" << std::endl
			<< "PROPERTY sim/require_agpoint 1/0" << std::endl
			<< "PROPERTY sim/require_object 1/0" << std::endl
			<< "PROPERTY sim/require_facade 1/0" << std::endl;

		//Add all our airport filters
		for (auto& apt : vctAirports)
		{
			//Add this airport filter
			sstrDSF << "PROPERTY sim/filter/aptid " << apt << std::endl;
		}

		//Loop through our exclusions and add them
		for (auto ex : vctExcludes)
		{
			//Add this exclusion
			sstrDSF << "PROPERTY " << ex.Type << " " << ex.West << "/" << ex.South << "/" << ex.East << "/" << ex.North << std::endl;
		}

		//Add all the object assets to the polygon
		for (size_t i = 0; i < vctObjectResources.size(); i++)
		{
			//Add this polygon def
			sstrDSF << "OBJECT_DEF " << vctObjectResources[i] << std::endl;
		}

		//Loop through all the polygon assets and add them to the DSF
		for (size_t i = 0; i < vctPolygonResources.size(); i++)
		{
			//Add this polygon def
			sstrDSF << "POLYGON_DEF " << vctPolygonResources[i] << std::endl;
		}

		//Check if we have roads
		if (vctRoads.size() > 0)
		{
			//Add the network def
			sstrDSF << "NETWORK_DEF lib/g10/roads.net" << std::endl;
		}

		//------------------------------------------------------------------------------------------------------------------
		// BEGIN_POLYGON PARAMS:
		// Forest: Resource, density (0-255 = zone, 256-511 = linear, 512-767 = points), 2
		// Line: Resource, (1 closed, 0 not closed), (2 straight, 4 curve)
		// String: Resource, spacing (meters), 2
		// Polygon: Resource, (heading, or 65535 if explicit UVs), (2 or 4 if explicit UVs or curve)
		// Facade: Resource, height, 2 for auto walls, 3 from pick walls. If pick walls, polygon point has the wall index at the end
		// 
		// ROADS:
		// BEGIN_SEGMENT Type, Subtype, Junction ID Start, Lon, Lat, Level
		// SHAPE_POINT Lon, Lat, Level
		// END_SEGMENT Junction ID End, Lon, Lat, Level
		//------------------------------------------------------------------------------------------------------------------

		//Sort all the placement vectors
		std::sort(vctForests.begin(), vctForests.end());
		std::sort(vctFacades.begin(), vctFacades.end());
		std::sort(vctObjects.begin(), vctObjects.end());
		std::sort(vctPolygons.begin(), vctPolygons.end());
		std::sort(vctStrings.begin(), vctStrings.end());
		std::sort(vctLines.begin(), vctLines.end());

		//Define a string to track the last airport, so we know when we need to change the airport filter
		std::string strLastAirport = "";

		//Add forests
		for (size_t i = 0; i < vctForests.size(); i++)
		{
			//Determine the resource index
			int intResourceIndex = std::distance(vctPolygonResources.begin(), std::find(vctPolygonResources.begin(), vctPolygonResources.end(), vctForests[i].Resource));

			//Define the density
			int intDensity = 255 * vctForests[i].Density + 256 * vctForests[i].FillMode;

			sstrDSF << "BEGIN_POLYGON " << std::to_string(intResourceIndex) << " " << std::to_string(intDensity) << " " << "2" << std::endl << "BEGIN_WINDING" << std::endl;

			auto XPNodes = vctForests[i].Vertices.GetXPNodes();
			for (auto& v : XPNodes)
			{
				//Add this point
				sstrDSF << "POLYGON_POINT " << v.X << " " << v.Y << std::endl;
			}

			sstrDSF << "END_WINDING" << std::endl;

			//Check if there are holes
			if (vctForests[i].Holes.size() > 0)
			{
				//Loop through the holes
				for (int h = 0; h < vctForests[i].Holes.size(); h++)
				{
					//Start the winding
					sstrDSF << "BEGIN_WINDING" << std::endl;

					auto XPHoleNodes = vctForests[i].Vertices.GetXPNodes();
					for (auto& v : XPHoleNodes)
					{
						//Add this point
						sstrDSF << "POLYGON_POINT " << v.X << " " << v.Y << std::endl;
					}

					//End the winding
					sstrDSF << "END_WINDING" << std::endl;
				}
			}

			//Close the forest
			sstrDSF << "END_POLYGON" << std::endl;
		}

		//Add facades
		for (size_t i = 0; i < vctFacades.size(); i++)
		{
			//Determine the resource index
			size_t intResourceIndex = std::distance(vctPolygonResources.begin(), std::find(vctPolygonResources.begin(), vctPolygonResources.end(), vctFacades[i].Resource));

			//Determine the pick walls param
			int intPickWalls = 2;
			if (vctFacades[i].PickWalls == true) { intPickWalls = 3; }

			//Open the forest
			sstrDSF << "BEGIN_POLYGON " << std::to_string(intResourceIndex) << " " << std::to_string(vctFacades[i].Height) << " " << std::to_string(intPickWalls) << std::endl << "BEGIN_WINDING" << std::endl;

			//Define a reference to this facade
			auto& f = vctFacades[i];

			//Get the nodes in the XP format
			auto XPNodes = f.Vertices.GetRealNodes(10);

			//Loop through the coords and add these nodes
			for (size_t iN = 0; iN < XPNodes.size(); iN++)
			{
				if (f.PickWalls && f.Curved)
				{
					std::string strWallIdx = 0;
					try
					{
						strWallIdx = f.Vertices.Nodes[iN].Properties.at("wall");
					}
					catch (...)
					{
						strWallIdx = "0";
					}
					sstrDSF << "POLYGON_POINT " << f.Vertices.Nodes[iN].X << " " << f.Vertices.Nodes[iN].Y << " " << strWallIdx << " " << f.Vertices.Nodes[iN].U << " " << f.Vertices.Nodes[iN].V << std::endl;
				}
				else if (f.Curved)
				{
					sstrDSF << "POLYGON_POINT " << XPNodes[iN].X << " " << XPNodes[iN].Y << " " << XPNodes[iN].U << " " << XPNodes[iN].V << std::endl;
				}
				else if (f.PickWalls)
				{
					std::string strWallIdx = 0;
					try
					{
						strWallIdx = f.Vertices.Nodes[iN].Properties.at("wall");
					}
					catch (...)
					{
						strWallIdx = "0";
					}
					sstrDSF << "POLYGON_POINT " << XPNodes[iN].X << " " << XPNodes[iN].Y << " " << strWallIdx << std::endl;
				}
				else
				{
					sstrDSF << "POLYGON_POINT " << XPNodes[iN].X << " " << XPNodes[iN].Y << std::endl;
				}
			}

			//Close the forest
			sstrDSF << "END_WINDING\nEND_POLYGON" << std::endl;
		}

		//Add objects
		for (size_t i = 0; i < vctObjects.size(); i++)
		{
			size_t idxResource = std::distance(vctObjectResources.begin(), std::find(vctObjectResources.begin(), vctObjectResources.end(), vctObjects[i].Resource));

			sstrDSF << "OBJECT " << std::to_string(idxResource) << " " << vctObjects[i].Lon << " " << vctObjects[i].Lat << " " << vctObjects[i].Heading << std::endl;
		}

		//Add polygons
		for (size_t i = 0; i < vctPolygons.size(); i++)
		{
			//Determine the resource index
			size_t idxResourceIndex = std::distance(vctPolygonResources.begin(), std::find(vctPolygonResources.begin(), vctPolygonResources.end(), vctPolygons[i].Resource));

			//Open the forest
			//This heading code is wrong. Headings have a special encoding where the first 16(?) bits are the heading as an int, and the rest are a decimal? I have to check but it's a very weird encoding.
			sstrDSF << "BEGIN_POLYGON " << std::to_string(idxResourceIndex) << " " << (vctPolygons[i].ExplicitUVs ? std::to_string(vctPolygons[i].Heading) : "65535") << " " << (vctPolygons[i].Curved ? "4" : "2") << std::endl << "BEGIN_WINDING" << std::endl;

			auto XPNodes = vctPolygons[i].Vertices.GetXPNodes();

			//Loop through the coords and add these nodes
			for (auto& v : XPNodes)
			{
				if (vctPolygons[i].Curved || vctPolygons[i].ExplicitUVs)
				{
					sstrDSF << "POLYGON_POINT " << v.X << " " << v.Y << " " << v.U << " " << v.V << std::endl;
				}
				else
				{
					sstrDSF << "POLYGON_POINT " << v.X << " " << v.Y << std::endl;
				}
			}
			sstrDSF << "END_WINDING" << std::endl;

			//Check if there are holes
			if (vctPolygons[i].Holes.size() > 0)
			{
				//Loop through the holes
				for (int h = 0; h < vctPolygons[i].Holes.size(); h++)
				{
					//Start the winding
					sstrDSF << "BEGIN_WINDING" << std::endl;

					auto XPNodes = vctPolygons[i].Vertices.GetXPNodes();

					//Loop through the nodes
					for (auto& v : XPNodes)
					{
						//Add this point
						sstrDSF << "POLYGON_POINT " << v.X << " " << v.Y << std::endl;
					}

					//End the winding
					sstrDSF << "END_WINDING" << std::endl;
				}
			}

			//Close the forest
			sstrDSF << "END_POLYGON" << std::endl;
		}

		//Add strings
		for (size_t i = 0; i < vctStrings.size(); i++)
		{
			//Get the resource index
			size_t intResourceIndex = std::distance(vctPolygonResources.begin(), std::find(vctPolygonResources.begin(), vctPolygonResources.end(), vctStrings[i].Resource));

			//Open the forest
			sstrDSF << "BEGIN_POLYGON " << std::to_string(intResourceIndex) << " " << std::to_string(vctStrings[i].Spacing) << " " << "2" << std::endl << "BEGIN_WINDING" << std::endl;

			//Get the nodes in the XP format
			auto XPNodes = vctLines[i].Vertices.GetXPNodes();

			//Loop through the coords and add these nodes
			for (auto& v : XPNodes)
			{
				//Add this point
				if (vctLines[i].Curved)
				{
					sstrDSF << "POLYGON_POINT " << v.X << " " << v.Y << " " << v.U << " " << v.V << std::endl;
				}
				else
				{
					sstrDSF << "POLYGON_POINT " << v.X << " " << v.Y << std::endl;
				}
			}

			//Close the forest
			sstrDSF << "END_WINDING\nEND_POLYGON" << std::endl;
		}

		//Add lines
		for (size_t i = 0; i < vctLines.size(); i++)
		{
			int intResourceIndex = std::distance(vctPolygonResources.begin(), std::find(vctPolygonResources.begin(), vctPolygonResources.end(), vctLines[i].Resource));

			//Open
			sstrDSF << "BEGIN_POLYGON " << std::to_string(intResourceIndex) << " " << (vctLines[i].Closed ? "1" : "0") << " " << (vctLines[i].Curved ? "4" : "2") << std::endl << "BEGIN_WINDING" << std::endl;

			//Add nodes in the XP format
			auto XPNodes = vctLines[i].Vertices.GetXPNodes();
			for (auto& v : XPNodes)
			{
				if (vctLines[i].Curved)
				{
					sstrDSF << "POLYGON_POINT " << v.X << " " << v.Y << " " << v.U << " " << v.V << std::endl;
				}
				else
				{
					sstrDSF << "POLYGON_POINT " << v.X << " " << v.Y << std::endl;
				}
			}

			//Close
			sstrDSF << "END_WINDING\nEND_POLYGON" << std::endl;
		}

		//Add roads
		for (size_t i = 0; i < vctRoads.size(); i++)
		{
			//Start the segment
			sstrDSF << "BEGIN_SEGMENT " << "0" << " " << vctRoads[i].Subtype << " " << std::to_string(vctRoads[i].StartJunctionID) << " " << vctRoads[i].Lons[0] << " " << vctRoads[i].Lats[0] << " " << vctRoads[i].Elevations[0] << std::endl;

			//Loop through the coords and add these nodes
			for (size_t iN = 1; iN + 1 < vctRoads[i].Lats.size(); iN++)
			{
				//Add this point
				sstrDSF << "SHAPE_POINT " << vctRoads[i].Lons[iN] << " " << vctRoads[i].Lats[iN] << " " << vctRoads[i].Elevations[iN] << std::endl;
			}

			//Close the segment
			sstrDSF << "END_SEGMENT " << std::to_string(vctRoads[i].EndJunctionID) << " " << vctRoads[i].Lons[vctRoads[i].Lons.size() - 1] << " " << vctRoads[i].Lats[vctRoads[i].Lats.size() - 1] << " " << vctRoads[i].Elevations[vctRoads[i].Elevations.size() - 1] << std::endl;
		}

		//Now write the DSF
		//Define the write path
		OutPath = InPath.string() + "\\";

		//Check if lat is positive to see if we need to add a plus sign
		if (South > 0)
		{
			OutPath += "+" + std::to_string(South);
		}
		else
		{
			OutPath += std::to_string(South);
		}

		//Check if lon is positive to see if we need to add a plus sign
		if (West > 0)
		{
			if (West < 100)
			{
				OutPath += "+0" + std::to_string(West);
			}
			else
			{
				OutPath += "+" + std::to_string(West);
			}
		}
		else
		{
			if (West > -100)
			{
				OutPath += "-0" + std::to_string(std::abs(West));
			}
			else
			{
				OutPath += "-" + std::to_string(std::abs(West));
			}
		}

		//Add the .dsf
		OutPath += ".txt";

		//strDSFWritePath = "C:\\Users\\cmrbu\\Desktop\\+38-121.txt";

		//Define handle
		std::ofstream ofsDSFFile;

		//Open
		ofsDSFFile.open(OutPath);

		//Write
		ofsDSFFile << sstrDSF.str();

		//Close
		ofsDSFFile.close();
	}

	//Release access
	mtForests.unlock();
	mtFacades.unlock();
	mtObjects.unlock();
	mtPolygons.unlock();
	mtStrings.unlock();
	mtLines.unlock();
	mtRoads.unlock();
	mtExcludes.unlock();

	return OutPath;
}

/// <summary>
/// Reads DSF contents from DSF file. DSF is in text format and must be run through DSFTool.
/// </summary>
/// <param name="InPath">Path to the DSF</param>
/// <returns>True on success, false on failure</returns>
bool DSF::Tile::Read(std::filesystem::path InPath, std::filesystem::path InDSFToolPath)
{
	//First off we clear all our data
	vctForests.clear();
	vctFacades.clear();
	vctObjects.clear();
	vctPolygons.clear();
	vctStrings.clear();
	vctLines.clear();
	vctRoads.clear();
	vctExcludes.clear();

	ToTxt(InPath, InDSFToolPath);
	InPath = InPath.string() + ".txt";

	//Now open the file and read the contets
	//Per DSF buffers
	std::vector<std::string> vctObjAssets;
	std::vector<std::string> vctPolAssets;
	std::vector<std::string> vctTerAssets;
	std::vector<std::string> vctAirports;
	size_t idxCurAirportIdx = -1;
	bool bCurPolIsValid = false;
	std::vector<std::vector<geo::Node>> vctCurVerts;
	bool bCurPolClosed = false;
	bool bCurPolCurved = false;
	bool bCurPolExplicitUVs = false;
	double dblCurPolHeading = 0;
	double dblCurStrSpacing = 1;
	double dblFacHeight = 0;
	bool bCurFacPickWalls = false;
	bool bCurPolIsFac = false;
	std::string strCurPolAsset;

	//Open the converted file
	std::ifstream ifsDsf(InPath);

	//Token vector for file reading
	std::vector<std::string> vctTokens;
	std::string strCurLine;			//We also read the whole line

	//-----Read the lines-----
	// 
	//This code is not pretty cuz we can't necessarily just read tokens cuz assets
	//can have spaces in the file names, and with my luck some fool will put two spaces,
	//and recombining tokens with a single space won't fix it.
	//
	//The solution here is we read the whole line, and tokenize that
	//That way we have the tokens, and a raw string of the whole line
	//
	while (true)
	{
		//Break at end of file
		if (!ifsDsf.good()) { break; }

		//Clear token vector
		vctTokens.clear();

		//Read the line, put it in stringstream so we can tokenize. Also ensure that we actually got tokens
		getline(ifsDsf, strCurLine);
		vctTokens.clear();
		vctTokens = TokenizeString(strCurLine, { ' ', '\n', '\t' });
		if (vctTokens.size() == 0) { continue; }

		//Handle the line
		if (vctTokens[0] == "PROPERTY")
		{
			//There are many different things this can be. Currently we are only handling airport
			if (vctTokens[1] == "sim/filter/aptid" && vctTokens.size() == 3)
			{
				vctAirports.push_back(vctTokens[2]);
			}
		}
		else if (vctTokens[0] == "FILTER")
		{
			idxCurAirportIdx = stoi(vctTokens[1]);	//This sets the current airport index
		}
		else if (vctTokens[0] == "OBJECT_DEF")
		{
			//Get the asset from idx 11+ of CurLine and append it to our dsf specific obj vector
			ASSERTM(strCurLine.size() > 11, ("\"" + strCurLine + "\""));
			std::string strObjName = strCurLine.substr(11);
			vctObjAssets.push_back(strObjName);
		}
		else if (vctTokens[0] == "POLYGON_DEF")
		{
			//Get the asset from idx 12+ of CurLine and append it to our dsf specific pol vector
			ASSERTM(strCurLine.size() > 12, ("\"" + strCurLine + "\""));
			vctPolAssets.push_back(strCurLine.substr(12));
		}
		else if (vctTokens[0] == "TERRAIN_DEF")
		{
			//Get the asset from idx 12+ of CurLine and append it to our dsf specific pol vector
			ASSERTM(strCurLine.size() > 12, ("\"" + strCurLine + "\""));
			vctTerAssets.push_back(strCurLine.substr(12));
		}
		else if (vctTokens[0] == "OBJECT")
		{
			//Define a new DSFObject, set it's values appropriately. Remember, alts are all agl
			ASSERTM(vctTokens.size() == 5, ("\"" + strCurLine + "\""));

			size_t idxAsset = -1;
			try
			{
				idxAsset = stoi(vctTokens[1]);
			}
			catch (...)
			{
				ASSERTM(false, "Token 1 not int for OBJECT! " + vctTokens[1]);
			}
			ASSERTM(idxAsset >= 0 && idxAsset < vctObjAssets.size(), "Asset: " + std::to_string(idxAsset) + " declared objs: " + std::to_string(vctObjAssets.size()));

			try
			{

				DSF::Object newObj;
				newObj.Resource = idxAsset < vctObjAssets.size() ? vctObjAssets[idxAsset] : "";
				newObj.Lat = stod(vctTokens[3]);
				newObj.Lon = stod(vctTokens[2]);
				newObj.Alt = 0;
				newObj.Heading = stod(vctTokens[4]);
				if (idxCurAirportIdx < vctAirports.size()) { newObj.Airport = vctAirports[idxCurAirportIdx]; }
				vctObjects.push_back(newObj);
			}
			catch (...)
			{
				ASSERTM(false, "Error storing obj data... probably bad string for stod");
			}
		}
		else if (vctTokens[0] == "OBJECT_MSL")
		{
			//Define a new DSFObject, set it's values appropriately. Remember, alts are all agl
			ASSERTM(vctTokens.size() == 6, ("\"" + strCurLine + "\""));

			size_t idxAsset = -1;
			try
			{
				idxAsset = stoi(vctTokens[1]);
			}
			catch (...)
			{
				ASSERTM(false, "stoi failed!" + ("\n\"" + strCurLine + "\""));
			}
			ASSERTM(idxAsset >= 0 && idxAsset < vctObjAssets.size(), "Asset: " + std::to_string(idxAsset) + " declared objs: " + std::to_string(vctObjAssets.size()) + ("\n\"" + strCurLine + "\""));
			std::string strAsset = vctObjAssets[idxAsset];

			try
			{
				DSF::Object newObj;
				newObj.Resource = idxAsset < vctObjAssets.size() ? vctObjAssets[idxAsset] : "";
				newObj.Lat = stod(vctTokens[3]);
				newObj.Lon = stod(vctTokens[2]);
				newObj.Alt = stod(vctTokens[4]);
				newObj.Heading = stod(vctTokens[4]);
				if (idxCurAirportIdx < vctAirports.size()) { newObj.Airport = vctAirports[idxCurAirportIdx]; }
				vctObjects.push_back(newObj);
			}
			catch (...)
			{
				ASSERTM(false, "Error storing obj data... probably bad string for stod");
			}
		}
		else if (vctTokens[0] == "BEGIN_POLYGON")
		{
			ASSERTM(vctTokens.size() == 4, ("\"" + strCurLine + "\""));

			size_t idxAsset = -1;
			try
			{
				idxAsset = stoi(vctTokens[1]);
			}
			catch (...)
			{
				ASSERTM(false, "stoi failed! " + ("\n\"" + strCurLine + "\""));
			}
			ASSERTM(idxAsset >= 0 && idxAsset < vctPolAssets.size(), "Asset: " + std::to_string(idxAsset) + " declared pols: " + std::to_string(vctPolAssets.size()) + ("\n\"" + strCurLine + "\""));
			std::string strAsset = vctPolAssets[idxAsset];

			//If this asset doesn't end in .lin or .pol, skip it
			if (!(strAsset.ends_with(".lin") || strAsset.ends_with(".pol") || strAsset.ends_with(".str") || strAsset.ends_with(".fac")))
			{
				continue;
			}

			//Resets
			bCurPolIsFac = false;
			vctCurVerts.clear();

			bCurPolIsValid = true;
			strCurPolAsset = strAsset;

			bCurPolClosed = vctTokens[2] == "1" && strAsset.ends_with(".lin");
			bCurPolCurved = vctTokens[3] == "4" && vctTokens[2] != "65535";
			dblCurStrSpacing = stod(vctTokens[2]);

			if (strAsset.ends_with(".str"))
			{
				bCurPolClosed = false;	//Strings are never closed
			}

			if (strAsset.ends_with(".fac"))
			{
				//Height is the 3rd token
				dblFacHeight = stod(vctTokens[2]);

				//Switch the modes including wall picking and curved
				int iModeSwitch = stoi(vctTokens[3]);
				switch (iModeSwitch)
				{
				case 2:
					bCurFacPickWalls = false;
					bCurPolCurved = false;
					break;
				case 3:
					bCurFacPickWalls = true;
					bCurPolCurved = false;
					break;
				case 4:
					bCurFacPickWalls = false;
					bCurPolCurved = true;
					break;
				case 5:
					bCurFacPickWalls = true;
					bCurPolCurved = true;
					break;
				}

				//For different handling of POLYGON_POINT
				bCurPolIsFac = true;

			}

			//Check if explicit UVs. This is when token 3 is 4 (normally 2) and token 2 is 65535. Compare strings (it's faster)
			if (vctTokens[3] == "4" && vctTokens[2] == "65535")
			{
				bCurPolExplicitUVs = true;
			}
			else
			{
				bCurPolExplicitUVs = false;
			}

			//Get the heading if this isn't using explicit UVs. Heading is defined at token 2 / 360 + token 2 % 360
			if (!bCurPolExplicitUVs)
			{
				dblCurPolHeading = XSLGeoutils::ResolveHeading(360 - (stod(vctTokens[2]) / 360.0 + fmod(stod(vctTokens[2]), 360)));
			}

		}
		else if (vctTokens[0] == "BEGIN_WINDING")
		{
			if (!bCurPolIsValid) { continue; }
			vctCurVerts.push_back(std::vector<geo::Node>());
		}
		else if (vctTokens[0] == "POLYGON_POINT")
		{
			if (!bCurPolIsValid) { continue; }
			geo::Node NewVert;
			try
			{
				NewVert.X = stod(vctTokens[1]);	//Lon
				NewVert.Y = stod(vctTokens[2]);	//Lat

				if (bCurPolIsFac)
				{
					//If we pick walls, then token [3] is the wall index
					if (bCurFacPickWalls)
					{
						NewVert.Properties.emplace("wall", vctTokens[3]);

						//If we are curved too, then tokens [4] and [5] are the control points
						if (bCurPolCurved)
						{
							NewVert.U = stod(vctTokens[4]);
							NewVert.V = stod(vctTokens[5]);
						}
					}

					//If we are not picking walls but are curved, then tokens [3] and [4] are the control points
					else if (bCurPolCurved)
					{
						NewVert.U = stod(vctTokens[3]);
						NewVert.V = stod(vctTokens[4]);
					}
				}
				else
				{
					//If explicit UVs, save them into vertex
					if (bCurPolExplicitUVs)
					{
						NewVert.U = stod(vctTokens[3]);
						NewVert.V = stod(vctTokens[4]);
					}
					else if (bCurPolCurved)
					{
						//If curved, save the control points into U/V. These will be resolved later, here we are treating U/V simply as buffers
						//UVs are calculated later anyway so it doesn't matter
						NewVert.U = stod(vctTokens[3]);	//lon
						NewVert.V = stod(vctTokens[4]);//Lat
					}
				}
			}
			catch (...)
			{
				ASSERTM(false, "Storing vertex data failed! Likely stod fail." + ("\n\"" + strCurLine + "\""));
			}

			//Add the new vertex to the current winding
			vctCurVerts.back().push_back(NewVert);
		}
		else if (vctTokens[0] == "END_WINDING")
		{
			//Nothing to do
		}
		else if (vctTokens[0] == "END_POLYGON")
		{
			if (!bCurPolIsValid) { continue; }

			if (strCurPolAsset.ends_with(".pol"))
			{
				DSF::Polygon NewPol;
				NewPol.Curved = bCurPolCurved;
				NewPol.Heading = dblCurPolHeading;
				NewPol.ExplicitUVs = bCurPolExplicitUVs;
				NewPol.Resource = strCurPolAsset;
				NewPol.Vertices.LoadFromXPNodes(vctCurVerts[0], true);
				if (idxCurAirportIdx < vctAirports.size()) { NewPol.Airport = vctAirports[idxCurAirportIdx]; }

				//Add holes
				for (size_t i = 1; i < vctCurVerts.size(); i++)
				{
					auto w = geo::Winding();
					w.LoadFromXPNodes(vctCurVerts[i], true);
					NewPol.Holes.push_back(w);
				}

				vctPolygons.push_back(NewPol);
			}
			else if (strCurPolAsset.ends_with(".lin"))
			{
				DSF::Line NewLine;
				NewLine.Closed = bCurPolClosed;
				NewLine.Curved = bCurPolCurved;
				NewLine.Resource = strCurPolAsset;
				NewLine.Vertices.LoadFromXPNodes(vctCurVerts[0], NewLine.Closed);
				if (idxCurAirportIdx < vctAirports.size()) { NewLine.Airport = vctAirports[idxCurAirportIdx]; }

				vctLines.push_back(NewLine);
			}
			else if (strCurPolAsset.ends_with(".str"))
			{
				DSF::String NewStr;
				NewStr.Resource = strCurPolAsset;
				NewStr.Spacing = dblCurStrSpacing;
				NewStr.Vertices.LoadFromXPNodes(vctCurVerts[0], false);
				if (idxCurAirportIdx < vctAirports.size()) { NewStr.Airport = vctAirports[idxCurAirportIdx]; }

				vctStrings.push_back(NewStr);
			}
			else if (strCurPolAsset.ends_with(".fac"))
			{
				DSF::Facade NewFac;
				NewFac.Closed = true;	//Not implemented yet. All facades are closed unless the .fac file itself specifies it isn't, and we don't have that info here... In the future I see us having a an object containing everything that's loaded or loadable that we could just query to check this.
				NewFac.Curved = bCurPolCurved;
				NewFac.Height = dblFacHeight;
				NewFac.PickWalls = bCurFacPickWalls;
				NewFac.Vertices.LoadFromXPNodes(vctCurVerts[0], NewFac.Closed);
				NewFac.Resource = strCurPolAsset;
				if (idxCurAirportIdx < vctAirports.size()) { NewFac.Airport = vctAirports[idxCurAirportIdx]; }

				vctFacades.push_back(NewFac);
			}
			else if (strCurPolAsset.ends_with(".for"))
			{
				DSF::Forest NewFor;
				NewFor.Resource = strCurPolAsset;
				NewFor.Density = 255;	//Not yet implemented
				NewFor.FillMode = 0;	//Not yet implemented
				NewFor.Vertices.LoadFromXPNodes(vctCurVerts[0], true);
				if (idxCurAirportIdx < vctAirports.size()) { NewFor.Airport = vctAirports[idxCurAirportIdx]; }

				//Add holes
				for (size_t i = 1; i < vctCurVerts.size(); i++)
				{
					auto w = geo::Winding();
					w.LoadFromXPNodes(vctCurVerts[i], true);
					NewFor.Holes.push_back(w);
				}

				vctForests.push_back(NewFor);
			}

			//Reset all the current polygon buffers
			vctCurVerts.clear();
			bCurPolClosed = false;
			bCurPolCurved = false;
			bCurPolExplicitUVs = false;
			bCurPolIsValid = false;
			dblCurPolHeading = 0;
			dblCurStrSpacing = 0;
		}
		else if (vctTokens[0] == "BEGIN_PATCH")
		{
			//Format is resourceIdx 0 -1 something something. We only care about the resource idx
			size_t idxResource = stoull(vctTokens[1]);
			strCurPolAsset = idxResource < vctTerAssets.size() ? vctTerAssets[idxResource] : "";
		}
		else if (vctTokens[0] == "BEGIN_PRIMITIVE")
		{
			//Add a new winding
			vctCurVerts.push_back(std::vector<geo::Node>());
		}
		else if (vctTokens[0] == "PATCH_VERTEX")
		{
			//Format is lon, lat, elevation, normal x, normal y, U, V. U and V are optional. We ignore the normal
			geo::Node NewVert;
			NewVert.X = stod(vctTokens[1]);
			NewVert.Y = stod(vctTokens[2]);
			NewVert.Z = stod(vctTokens[3]);

			if (vctTokens.size() >= 7)
			{
				NewVert.U = stod(vctTokens[5]);
				NewVert.V = stod(vctTokens[6]);
			}

			vctCurVerts.back().push_back(NewVert);
		}
		else if (vctTokens[0] == "END_PATCH")
		{
			DSF::TerrainPatch NewPatch;
			NewPatch.Resource = strCurPolAsset;

			for (auto& w : vctCurVerts)
			{
				geo::Winding NewWinding;
				NewWinding.LoadFromStraightNodes(w, true);
				NewPatch.Primitives.push_back(NewWinding);
			}

			vctTerPatches.push_back(NewPatch);
		}
	}

	//Close the file
	ifsDsf.close();
}

/// <summary>
/// Gains exclusive access on all locks, then clears vectors, and releases access.
/// </summary>
DSF::Tile::~Tile()
{
	//Get access
	mtForests.lock();
	mtFacades.lock();
	mtObjects.lock();
	mtPolygons.lock();
	mtStrings.lock();
	mtLines.lock();
	mtRoads.lock();
	mtExcludes.lock();

	//Empty the vectors
	vctForests.clear();
	vctFacades.clear();
	vctObjects.clear();
	vctPolygons.clear();
	vctStrings.clear();
	vctLines.clear();

	//Release access
	mtForests.unlock();
	mtFacades.unlock();
	mtObjects.unlock();
	mtPolygons.unlock();
	mtStrings.unlock();
	mtLines.unlock();
	mtRoads.unlock();
	mtExcludes.unlock();
}

/// <summary>
/// Copies all other DSFTile scenery features into this DSFTile. Gains exclusive access on all locks. Releases locks when done
/// </summary>
/// <param name="Other">Reference to other DSFTile</param>
void DSF::Tile::operator += (DSF::Tile& Other)
{
	//Get access
	mtForests.lock();
	mtFacades.lock();
	mtObjects.lock();
	mtPolygons.lock();
	mtStrings.lock();
	mtLines.lock();
	mtRoads.lock();
	mtExcludes.lock();

	//Forests
	//Reserve
	vctForests.reserve(vctForests.capacity() + Other.vctForests.size());

	//Copy
	for (DSF::Forest& i : Other.vctForests) { vctForests.push_back(i); }

	//Facades
	//Reserve
	vctFacades.reserve(vctFacades.capacity() + Other.vctFacades.size());

	//Copy
	for (DSF::Facade& i : Other.vctFacades) { vctFacades.push_back(i); }

	//Objects
	//Reserve
	vctObjects.reserve(vctObjects.capacity() + Other.vctObjects.size());

	//Copy
	for (DSF::Object& i : Other.vctObjects) { vctObjects.push_back(i); }

	//Polygons
	//Reserve
	vctPolygons.reserve(vctPolygons.capacity() + Other.vctPolygons.size());

	//Copy
	for (DSF::Polygon& i : Other.vctPolygons) { vctPolygons.push_back(i); }

	//Strings
	//Reserve
	vctStrings.reserve(vctStrings.capacity() + Other.vctStrings.size());

	//Copy
	for (DSF::String& i : Other.vctStrings) { vctStrings.push_back(i); }

	//Lines
	//Reserve
	vctLines.reserve(vctLines.capacity() + Other.vctLines.size());

	//Copy
	for (DSF::Line& i : Other.vctLines) { vctLines.push_back(i); }

	//Roads
	//Reserve
	vctRoads.reserve(vctRoads.capacity() + Other.vctRoads.size());

	//Copy
	for (DSF::NetworkSegment& i : Other.vctRoads) { vctRoads.push_back(i); }

	//Reserve
	vctExcludes.reserve(vctExcludes.capacity() + Other.vctExcludes.size());

	//Copy
	for (DSF::Exclusion& i : Other.vctExcludes) { vctExcludes.push_back(i); }

	//Release access
	mtForests.unlock();
	mtFacades.unlock();
	mtObjects.unlock();
	mtPolygons.unlock();
	mtStrings.unlock();
	mtLines.unlock();
	mtRoads.unlock();
	mtExcludes.unlock();
}

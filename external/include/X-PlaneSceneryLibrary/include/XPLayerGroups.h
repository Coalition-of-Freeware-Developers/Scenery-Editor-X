//Module:	XPLayerGroups
//Author:	Connor Russell
//Date:		10/11/2024 7:39:04 PM
//Purpose:	

//Compile once
#pragma once

//Include necessary headers
#include <string>

namespace XPLayerGroups
{
	extern const int TERRAIN;
	extern const int BEACHES;
	extern const int SHOULDERS;
	extern const int TAXIWAYS;
	extern const int RUNWAYS;
	extern const int MARKINGS;
	extern const int AIRPORTS;
	extern const int ROADS;
	extern const int OBJECTS;
	extern const int LIGHT_OBJECTS;
	extern const int CARS;

	/// <summary>
	/// Resolves a layer group to a vertical offset
	/// </summary>
	/// <param name="InGroup">Layer group</param>
	/// <param name="InOffset">Offset from group</param>
	/// <returns>Vertical offset for proper layering</returns>
	int Resolve(std::string InGroup, int InOffset);

	/// <summary>
	/// Resolves a vertical offset to a layer group
	/// </summary>
	/// <param name="InOffset">Vertical offset</param>
	/// <returns>Pair containing the layer group, and offset, respectively <group> <offset></returns>
	std::pair<std::string, int> Resolve(int InOffset);
}
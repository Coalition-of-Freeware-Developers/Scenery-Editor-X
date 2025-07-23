//Module:	XPLayerGroups
//Author:	Connor Russell
//Date:		10/11/2024 7:39:04 PM
//Purpose:
#pragma once
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

    /**
     * @brief Resolves a layer group to a vertical offset
	 *
	 * @param InGroup = Layer group
	 * @param InOffset = Offset from group
	 * @return Vertical offset for proper layering
     */
    int Resolve(std::string InGroup, int InOffset);

    /**
     * @brief Resolves a vertical offset to a layer group
	 *
	 * @param InOffset = Vertical offset
	 * @return Pair containing the layer group, and offset, respectively <group> <offset>
     */
    std::pair<std::string, int> Resolve(int InOffset);

}

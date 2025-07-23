//Module:	XPLayerGroups
//Author:	Connor Russell
//Date:		10/11/2024 7:39:11 PM
//Purpose:	Implements XPLayerGroups.h
#include "XPLayerGroups.h"
#include "TextUtils.h"

const int XPLayerGroups::TERRAIN = 5;
const int XPLayerGroups::BEACHES = 16;
const int XPLayerGroups::SHOULDERS = 27;
const int XPLayerGroups::TAXIWAYS = 38;
const int XPLayerGroups::RUNWAYS = 49;
const int XPLayerGroups::MARKINGS = 60;
const int XPLayerGroups::AIRPORTS = 71;
const int XPLayerGroups::ROADS = 82;
const int XPLayerGroups::OBJECTS = 93;
const int XPLayerGroups::LIGHT_OBJECTS = 104;
const int XPLayerGroups::CARS = 115;

/**
 * @brief Resolves a layer group to a vertical InOffset
 *
 * This is used to determine the vertical offset for layering objects in X-Plane scenery.
 * The layer groups are defined in a specific order, and each group has a base offset that is added to the provided InOffset.
 * The first group starts with 5, and each subsequent group is 11 higher than the previous one.
 *
 * @param InGroup Layer group name as a string
 * @param InOffset Offset from group
 * @returns Vertical InOffset for proper layering
 */
int XPLayerGroups::Resolve(std::string InGroup, const int InOffset)
{
    ///< Remove whitespace just in case
    InGroup = TextUtils::TrimWhitespace(InGroup);

    //Layer groups in order are: terrain, beaches, shoulders, taxiways, runways, markings, airports, roads, objects, light_objects, cars
    //Each layer group is 11 higher than the previous. The first group starts with 5. So group 1 is 5 + InOffset, 2 is 16 + InOffset, etc. Then is combined with InOffset for final layering.
    if (InGroup == "terrain")
        return 5 + InOffset;
    if (InGroup == "beaches")
        return 16 + InOffset;
    if (InGroup == "shoulders")
        return 27 + InOffset;
    if (InGroup == "taxiways")
        return 38 + InOffset;
    if (InGroup == "runways")
        return 49 + InOffset;
    if (InGroup == "markings")
        return 60 + InOffset;
    if (InGroup == "airports")
        return 71 + InOffset;
    if (InGroup == "roads")
        return 82 + InOffset;
    if (InGroup == "objects")
        return 93 + InOffset;
    if (InGroup == "light_objects")
        return 104 + InOffset;
    if (InGroup == "cars")
        return 115 + InOffset;

    return 5 + InOffset;
}

/**
 * @brief Resolves a vertical InOffset to a layer group
 *
 * @param InOffset Vertical InOffset to resolve
 * @returns A pair containing the layer group name and the offset from that group.
 */
std::pair<std::string, int> XPLayerGroups::Resolve(const int InOffset)
{
    /**
     * Layer groups in order are: terrain, beaches, shoulders, taxiways, runways, markings, airports, roads, objects, light_objects, cars
     * Each layer group is 11 higher than the previous. We get the text with this pattern: "terrain " + std::to_string(Offset - 11);
     */
    if (InOffset < 11)
        return {"terrain ", InOffset - 5};
    if (InOffset < 22)
        return {"beaches ", InOffset - 16};
    if (InOffset < 33)
        return {"shoulders ", InOffset - 27};
    if (InOffset < 44)
        return {"taxiways ", InOffset - 38};
    if (InOffset < 55)
        return {"runways ", InOffset - 49};
    if (InOffset < 66)
        return {"markings ", InOffset - 60};
    if (InOffset < 77)
        return {"airports ", InOffset - 71};
    if (InOffset < 88)
        return {"roads ", InOffset - 82};
    if (InOffset < 99)
        return {"objects ", InOffset - 93};
    if (InOffset < 110)
        return {"light_objects ", InOffset - 104};
    if (InOffset < 121)
        return {"cars ", InOffset - 115};

    ///< Default if all else fails
    return {"terrain", 1};
}

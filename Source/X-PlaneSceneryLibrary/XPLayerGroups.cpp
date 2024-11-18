//Module:	XPLayerGroups
//Author:	Connor Russell
//Date:		10/11/2024 7:39:11 PM
//Purpose:	Implements XPLayerGroups.h

//Compile once
#pragma once

//Include necessary headers
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

/// <summary>
/// Resolves a layer group to a vertical InOffset
/// </summary>
/// <param name="InGroup">Layer group</param>
/// <param name="InOffset">Offset from group</param>
/// <returns>Vertical InOffset for proper layering</returns>
int XPLayerGroups::Resolve(std::string InGroup, int InOffset)
{
    //Remove whitespace just in case
    InGroup = TextUtils::TrimWhitespace(InGroup);

    //Layer groups in order are: terrain, beaches, shoulders, taxiways, runways, markings, airports, roads, objects, light_objects, cars
    //Each layer group is 11 higher than the previous. The first group starts with 5. So group 1 is 5 + InOffset, 2 is 16 + InOffset, etc. Then is combined with InOffset for final layering.
    if (InGroup == "terrain")
    {
        return 5 + InOffset;
    }
    else if (InGroup == "beaches")
    {
        return 16 + InOffset;
    }
    else if (InGroup == "shoulders")
    {
        return 27 + InOffset;
    }
    else if (InGroup == "taxiways")
    {
        return 38 + InOffset;
    }
    else if (InGroup == "runways")
    {
        return 49 + InOffset;
    }
    else if (InGroup == "markings")
    {
        return 60 + InOffset;
    }
    else if (InGroup == "airports")
    {
        return 71 + InOffset;
    }
    else if (InGroup == "roads")
    {
        return 82 + InOffset;
    }
    else if (InGroup == "objects")
    {
        return 93 + InOffset;
    }
    else if (InGroup == "light_objects")
    {
        return 104 + InOffset;
    }
    else if (InGroup == "cars")
    {
        return 115 + InOffset;
    }
    else
    {
        return 5 + InOffset;
    }
}

/// <summary>
/// Resolves a vertical InOffset to a layer group
/// </summary>
/// <param name="InOffset">Vertical InOffset</param>
/// <returns>Layer group and InOffset in the form of <group> <offset></returns>
std::pair<std::string, int> XPLayerGroups::Resolve(int InOffset)
{
    //Layer groups in order are: terrain, beaches, shoulders, taxiways, runways, markings, airports, roads, objects, light_objects, cars
    //Each layer group is 11 higher than the previous. We get the text with this pattern: "terrain " + std::to_string(Offset - 11);
    if (InOffset < 11)
    {
        return {"terrain ", InOffset - 5};
    }
    else if (InOffset < 22)
    {
        return {"beaches ", InOffset - 16};
    }
    else if (InOffset < 33)
    {
        return {"shoulders ", InOffset - 27};
    }
    else if (InOffset < 44)
    {
        return {"taxiways ", InOffset - 38};
    }
    else if (InOffset < 55)
    {
        return {"runways ", InOffset - 49};
    }
    else if (InOffset < 66)
    {
        return {"markings ", InOffset - 60};
    }
    else if (InOffset < 77)
    {
        return {"airports ", InOffset - 71};
    }
    else if (InOffset < 88)
    {
        return {"roads ", InOffset - 82};
    }
    else if (InOffset < 99)
    {
        return {"objects ", InOffset - 93};
    }
    else if (InOffset < 110)
    {
        return {"light_objects ", InOffset - 104};
    }
    else if (InOffset < 121)
    {
        return {"cars ", InOffset - 115};
    }

    //Default if all else fails
    return {"terrain", 1};
}

//Module:	XPAsset
//Author:	Connor Russell
//Date:		10/11/2024 7:15:04 PM
//Purpose:

//Compile once
#pragma once

//Include necessary headers
#include "XPLayerGroups.h"
#include <filesystem>
#include <string>


namespace XPAsset
{
/// <summary>
/// Base class for all X-Plane assets. Abstract.
/// </summary>
class Asset
{
private:
    virtual void MakeMeVirtual() = 0; //Dummy function to make the class abstract and uninstantiable.
public:
    std::filesystem::path pReal; //The real path of the asset, on the hard drive.

    //All paths are relative to the obj path
    std::filesystem::path pBaseTex;
    std::filesystem::path pNormalTex; //Material is typically in the b/alpha channel
    std::filesystem::path
        pMaterialTex; //This is only used if specified to have a dedicated material map - this is generally bundled in with the normal int he b/alpha channels
    bool bHasBaseTex{false};
    bool bHasNormalTex{false}; //Material is typically in the b/alpha channel
    bool bHasMaterialTex{
        false}; //This is only used if specified to have a dedicated material map - this is generally bundled in with the normal int he b/alpha channels

    double dblNormalScale{1};                  //Scape of the normal map as a ratio to the UVs
    int intLayerGroup{XPLayerGroups::TERRAIN}; //Layer group

    bool bSuperRoughness; //If true, the roughness is beyond 1.0, special shader case.
};
} // namespace XPAsset

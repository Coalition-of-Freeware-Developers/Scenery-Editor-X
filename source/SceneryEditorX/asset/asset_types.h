/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* asset_types.h
* -------------------------------------------------------
* Created: 11/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/logging/asserts.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

	enum class AssetType : uint8_t
	{
		None = 0,
		Scene,
		Prefab,
		Mesh,
		MeshSource,
		Material,
		Texture,
		EnvMap,
		Font,
		Script,
		ScriptFile,
		Animation,
		AnimationGraph,
        XPlaneAsset, /// This is a generic type for all X-Plane specific format assets
        Library,
	};

    enum class XPlaneAssetType : uint8_t
	{
		None = 0,
		DSF,
		Polygon,
		Object,
		Terrain,
		Forest,
		Library,
		AirportData,
		AutogenString,
		AutogenBlock,
		Line,
		Facade,
		RoadNetwork,
		ObjString
    };

	namespace Utils
    {
        /// -------------------------------------------------------

        /**
         * @brief Convert XPlaneAssetType enum to string representation
         * 
         * @param assetType The X-Plane asset type to convert
         * @return String representation of the X-Plane asset type
         */
        inline const char* XPlaneAssetTypeToString(const XPlaneAssetType assetType)
        {
            switch (assetType)
            {
                case XPlaneAssetType::None:             return "None";
                case XPlaneAssetType::DSF:              return "DSF";
                case XPlaneAssetType::Polygon:          return "Polygon";
                case XPlaneAssetType::Object:           return "Object";
                case XPlaneAssetType::Terrain:          return "Terrain";
                case XPlaneAssetType::Forest:           return "Forest";
                case XPlaneAssetType::Library:          return "Library";
                case XPlaneAssetType::AirportData:      return "AirportData";
                case XPlaneAssetType::AutogenString:    return "AutogenString";
                case XPlaneAssetType::AutogenBlock:     return "AutogenBlock";
                case XPlaneAssetType::Line:             return "Line";
                case XPlaneAssetType::Facade:           return "Facade";
                case XPlaneAssetType::RoadNetwork:      return "RoadNetwork";
                case XPlaneAssetType::ObjString:        return "ObjString";
                default:  // NOLINT(clang-diagnostic-covered-switch-default)
                    SEDX_CORE_ASSERT(false, "Unknown X-Plane Asset Type");
                    return "None";
            }
        }

        /// -------------------------------------------------------

        /**
         * @brief Convert string to XPlaneAssetType enum
         * 
         * @param assetType String representation of the X-Plane asset type
         * @return XPlaneAssetType enum value, XPlaneAssetType::None if not found
         */
        inline XPlaneAssetType XPlaneAssetTypeFromString(const std::string_view assetType)
        {
            if (assetType == "None")                return XPlaneAssetType::None;
            if (assetType == "DSF")                 return XPlaneAssetType::DSF;
            if (assetType == "Polygon")             return XPlaneAssetType::Polygon;
            if (assetType == "Object")              return XPlaneAssetType::Object;
            if (assetType == "Terrain")             return XPlaneAssetType::Terrain;
            if (assetType == "Forest")              return XPlaneAssetType::Forest;
            if (assetType == "Library")             return XPlaneAssetType::Library;
            if (assetType == "AirportData")         return XPlaneAssetType::AirportData;
            if (assetType == "AutogenString")       return XPlaneAssetType::AutogenString;
            if (assetType == "AutogenBlock")        return XPlaneAssetType::AutogenBlock;
            if (assetType == "Line")                return XPlaneAssetType::Line;
            if (assetType == "Facade")              return XPlaneAssetType::Facade;
            if (assetType == "RoadNetwork")         return XPlaneAssetType::RoadNetwork;
            if (assetType == "ObjString")           return XPlaneAssetType::ObjString;
            return XPlaneAssetType::None;
        }

        /// -------------------------------------------------------

	    inline AssetType AssetTypeFromString(const std::string_view assetType)
		{
			if (assetType == "None")                return AssetType::None;
			if (assetType == "Scene")               return AssetType::Scene;
			if (assetType == "Prefab")              return AssetType::Prefab;
			if (assetType == "Mesh")                return AssetType::Mesh;
			if (assetType == "MeshSource")          return AssetType::MeshSource;
			if (assetType == "Material")            return AssetType::Material;
			if (assetType == "Texture")             return AssetType::Texture;
			if (assetType == "EnvMap")              return AssetType::EnvMap;
			if (assetType == "Font")                return AssetType::Font;
			if (assetType == "Script")              return AssetType::Script;
			if (assetType == "ScriptFile")          return AssetType::ScriptFile;
			if (assetType == "Animation")           return AssetType::Animation;
			if (assetType == "AnimationGraph")      return AssetType::AnimationGraph;
			if (assetType == "XPlaneAsset")         return AssetType::XPlaneAsset;
			if (assetType == "SceneryEditorXLibrary") return AssetType::Library;
			return AssetType::None;
		}

        /// -------------------------------------------------------

	    inline const char* AssetTypeToString(const AssetType assetType)
		{
			switch (assetType)
			{
				case AssetType::None:               return "None";
				case AssetType::Scene:              return "Scene";
				case AssetType::Prefab:             return "Prefab";
				case AssetType::Mesh:               return "Mesh";
				case AssetType::MeshSource:         return "MeshSource";
				case AssetType::Material:           return "Material";
				case AssetType::Texture:            return "Texture";
				case AssetType::EnvMap:             return "EnvMap";
				case AssetType::Font:               return "Font";
				case AssetType::Script:             return "Script";
				case AssetType::ScriptFile:         return "ScriptFile";
				case AssetType::Animation:          return "Animation";
				case AssetType::AnimationGraph:     return "AnimationGraph";
                case AssetType::XPlaneAsset:		return "XPlaneAsset";
                case AssetType::Library:			return "SceneryEditorXLibrary";
                default:  // NOLINT(clang-diagnostic-covered-switch-default)
                    SEDX_CORE_ASSERT(false, "Unknown Asset Type");
                    return "None";
            }
		}

        /// -------------------------------------------------------

	    /**
		 * @brief Determines if an asset type string represents an X-Plane specific asset
		 * 
		 * This function checks whether a given asset type string corresponds to an
		 * X-Plane specific asset format rather than a general engine asset type.
		 * It evaluates the string against both AssetType and XPlaneAssetType enums
		 * to determine the asset category.
		 * 
		 * @param assetType The asset type string to check
		 * @return true if the asset type is X-Plane specific, false if it's a regular asset type
		 */
        inline bool isXPlaneAssetType(const std::string &assetType)
        {
            /*
            switch (assetType)
            {
                case AssetTypeFromString(assetType) != AssetType::None: return false;
                case XPlaneAssetTypeFromString(assetType) != XPlaneAssetType::None: return true;
                default:
                    return false;
            }
            */

        }

	}

}

/// -------------------------------------------------------

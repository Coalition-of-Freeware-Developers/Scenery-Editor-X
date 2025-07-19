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
	    Library,
	    Dsf,
	    Polygon,
		Object,
		Terrain,
		Forest,
		AirportData,
		AutogenString,
		AutogenBlock,
		Line,
		Facade,
		RoadNetwork,
		ObjString
	};

	namespace Utils {

		inline AssetType AssetTypeFromString(std::string_view assetType)
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

			return AssetType::None;
		}

		inline const char* AssetTypeToString(AssetType assetType)
		{
			switch (assetType)
			{
				case AssetType::None:                return "None";
				case AssetType::Scene:               return "Scene";
				case AssetType::Prefab:              return "Prefab";
				case AssetType::Mesh:                return "Mesh";
				case AssetType::MeshSource:          return "MeshSource";
				case AssetType::Material:            return "Material";
				case AssetType::Texture:             return "Texture";
				case AssetType::EnvMap:              return "EnvMap";
				case AssetType::Font:                return "Font";
				case AssetType::Script:              return "Script";
				case AssetType::ScriptFile:          return "ScriptFile";
				case AssetType::Animation:           return "Animation";
				case AssetType::AnimationGraph:      return "AnimationGraph";
			}

			SEDX_CORE_ASSERT(false, "Unknown Asset Type");
			return "None";
		}

	}
}

/// -------------------------------------------------------

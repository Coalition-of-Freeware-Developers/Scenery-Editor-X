/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* asset_extensions.h
* -------------------------------------------------------
* Created: 11/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/asset/asset_types.h>
#include <string>
#include <unordered_map>
#include <variant>

/// -------------------------------------------------------

namespace SceneryEditorX
{

	inline static std::unordered_map<std::string, std::variant<AssetType, XPlaneAssetType>> s_AssetExtensionMap =
	{
		///< Scenery Editor X - extension types
		{ ".edX.proj", 	AssetType::Scene },
		{ ".edX.mesh", 	AssetType::Mesh },
		{ ".edX.mat", 	AssetType::Material },
		{ ".edX.prefab",	AssetType::Prefab },
		{ ".edX.lib",		AssetType::Library },

		///< X-Plane Scenery extensions
		{ ".dsf",		XPlaneAssetType::DSF },
		{ ".pol",		XPlaneAssetType::Polygon },
		{ ".obj",		XPlaneAssetType::Object },
		{ ".ter",		XPlaneAssetType::Terrain },
		{ ".for",		XPlaneAssetType::Forest },
		{ ".dat",		XPlaneAssetType::AirportData },
		{ ".ags",		XPlaneAssetType::AutogenString },
		{ ".agb",		XPlaneAssetType::AutogenBlock },
		{ ".lin",		XPlaneAssetType::Line },
		{ ".fac",		XPlaneAssetType::Facade },
		{ ".net",		XPlaneAssetType::RoadNetwork },
		{ ".str",		XPlaneAssetType::ObjString },

		///< Python Plugin Script Extensions
		{ ".py",		AssetType::ScriptFile },

		///< Non X-Plane Mesh/animation extensions
		{ ".fbx",		AssetType::MeshSource },
		{ ".gltf",	AssetType::MeshSource },
		{ ".glb",		AssetType::MeshSource },
		{ ".obj",		AssetType::MeshSource },
		{ ".dae",		AssetType::MeshSource },
		{ ".usd",		AssetType::MeshSource },

		///< Textures
		{ ".png",		AssetType::Texture },
		{ ".jpg",		AssetType::Texture },
		{ ".jpeg",	AssetType::Texture },
		{ ".bmp",		AssetType::Texture },
		{ ".tga",		AssetType::Texture },
		{ ".hdr",		AssetType::Texture },
		{ ".dds",		AssetType::Texture },

		///< Fonts
		{ ".ttf",		AssetType::Font },
		{ ".ttc",		AssetType::Font },
		{ ".otf",		AssetType::Font },

	};

}

/// --------------------------------------------------------

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

/**
* @file asset_extensions.h
* @brief Asset file extension mapping system for type resolution
* 
* This file provides a comprehensive mapping system that associates file extensions
* with their corresponding asset types in Scenery Editor X. The system supports
* both native editor formats and external formats including X-Plane scenery files,
* mesh formats, textures, and fonts.
* 
* The extension mapping is used throughout the asset management system for:
* - Automatic asset type detection during import
* - File filtering in dialogs and browsers  
* - Asset pipeline routing and processing
* - Serialization format selection
* 
* @note - Some extensions like ".obj" appear multiple times with different asset types.
*       The asset manager uses additional context (file location, content analysis)
*       to resolve ambiguities between X-Plane objects and mesh source files.
*/
#pragma once
#include <unordered_map>
#include <variant>
#include <SceneryEditorX/asset/asset_types.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	* @brief Global asset extension to type mapping table
	* 
	* This static map provides a comprehensive lookup system for determining asset types
	* based on file extensions. The map uses std::variant to handle both native editor
	* asset types (AssetType) and X-Plane specific asset types (XPlaneAssetType).
	* 
	* **Extension Categories:**
	* 
	* **Native Scenery Editor X Formats (.edX.*)**
	* - `.edX.proj`		- Scene/Project files containing complete scene hierarchies
	* - `.edX.mesh`		- Processed mesh assets with optimization and LOD data
	* - `.edX.mat`		- Material definitions with PBR properties and texture references
	* - `.edX.prefab`	- Reusable object templates with component configurations
	* - `.edX.lib`		- X-Plane scenery asset libraries containing collections of related assets
	* 
	* **X-Plane Scenery Formats**
	* - `.dsf`		- Distributed Scenery Format files (terrain and vector data)
	* - `.pol`		- Polygon definition files for 2D airport layouts
	* - `.obj`		- X-Plane 3D object files (different from mesh .obj files)
	* - `.ter`		- Terrain definition and configuration files
	* - `.for`		- Forest and vegetation placement data
	* - `.dat`		- Airport layout and navigation data
	* - `.ags/.agb` - Autogen string and block definitions
	* - `.lin`		- Linear feature definitions (roads, railways, etc.)
	* - `.fac`		- Facade building definitions
	* - `.net`		- Road network topology data
	* - `.str`		- Object string placement data
	* 
	* **Mesh and Animation Sources**
	* - `.fbx`			- Autodesk FBX format with animations and materials
	* - `.gltf/.glb`	- Khronos GLTF format (JSON/Binary variants)
	* - `.obj`			- Wavefront OBJ geometry files (mesh source, not X-Plane)
	* - `.dae`			- COLLADA format with scene graph support
	* - `.usd`			- Universal Scene Description format
	* 
	* **Texture Formats**
	* - Standard formats: `.png`, `.jpg/.jpeg`, `.bmp`, `.tga`
	* - HDR formats: `.hdr` for high dynamic range textures
	* - Compressed: `.dds` with mipmap and compression support
	* 
	* **Font Assets**
	* - `.ttf/.ttc` - TrueType fonts (single/collection)
	* - `.otf`		- OpenType fonts with advanced typography
	* 
	* **Script Files**
	* - `.py` - Python plugin scripts for editor automation and custom tools
	* 
	* @warning Extension conflicts: The ".obj" extension maps to both XPlaneAssetType::Object
	*          and AssetType::MeshSource. Asset importers must use additional context
	*          (file location, header analysis) to resolve the correct type.
	* 
	* @note - This map is used extensively by:
	*       - AssetImporter for type detection during drag-and-drop operations
	*       - FileDialog filters for showing relevant files
	*       - AssetBrowser for icon selection and categorization
	*       - Project serialization for dependency tracking
	* 
	* **Usage Example:**
	* @code
	* std::string extension = ".edX.mat";
	* auto it = s_AssetExtensionMap.find(extension);
	* if (it != s_AssetExtensionMap.end())
	* {
	*     if (std::holds_alternative<AssetType>(it->second))
	*     {
	*         AssetType type = std::get<AssetType>(it->second);
	*         // Handle native editor asset type
	*     }
	*     else if (std::holds_alternative<XPlaneAssetType>(it->second))
	*     {
	*         XPlaneAssetType type = std::get<XPlaneAssetType>(it->second);
	*         // Handle X-Plane specific asset type
	*     }
	* }
	* @endcode
	* 
	* @see AssetType for native editor asset type definitions
	* @see XPlaneAssetType for X-Plane specific asset type definitions
	* @see AssetImporter for usage in asset import pipeline
	* @see AssetManager for asset type resolution and loading
	*/
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

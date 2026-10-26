/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray 
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * asset_extensions.h
 * -------------------------------------------------------
 * Created: 19/03/2026
 * -------------------------------------------------------
 */
#pragma once
#include "asset_types.h"

// -------------------------------------------------------

namespace SceneryEditorX
{

	/**
	 * @brief A mapping of file extensions to their corresponding asset types. 
	 * This is used to determine the type of asset based on its file extension when loading or importing assets into the editor.
	 */
	inline static std::unordered_map<std::string, AssetType> s_AssetExtensionMap =
	{
		// Scenery Editor X Format Types
		{ ".sex",  AssetType::Scene }, // Scenery Editor- X scene format
		{ ".edx",  AssetType::Scene }, // Scenery Editor- X compressed serialized binary format
		{ ".edm",  AssetType::Mesh },  // Scenery Editor- X mesh format
		{ ".edt",  AssetType::Texture },  // Scenery Editor- X texture format
		{ ".wed.xml", AssetType::Scene}, // WED (World Editor) XML project format
		{ ".wed.bak.xml", AssetType::Scene}, // WED (World Editor) XML project backup format

		// Mesh/Models
		{ ".fbx",  AssetType::Mesh },
		{ ".gltf", AssetType::Mesh },
		{ ".glb",  AssetType::Mesh },
		{ ".obj",  AssetType::Mesh },

		// Textures
		{ ".dds",  AssetType::Texture },
		{ ".ktx",  AssetType::Texture },
		{ ".png",  AssetType::Texture },
		{ ".tga",  AssetType::Texture },
		{ ".jpg",  AssetType::Texture },
		{ ".jpeg", AssetType::Texture },
		{ ".hdr",  AssetType::EnvMap },

		// Fonts
		{ ".ttf", AssetType::Font },
		{ ".ttc", AssetType::Font },
		{ ".otf", AssetType::Font },

	};

	/**
	 * @brief A mapping of X-Plane specific file extensions to their corresponding asset types.
	 * This is used to determine the type of asset based on its file extension when loading or importing X-Plane assets into the editor.
	 */
	inline static std::unordered_map<std::string, AssetType> s_XPlaneAssetExtensionMap =
	{
		{ ".obj", AssetType::Mesh }, // X-Plane OBJ8 format (with custom extensions for materials and animations)
		{ ".agp", AssetType::Mesh }, // X-Plane Autogen points
		{ ".agb", AssetType::Mesh }, // X-Plane Autogen blocks
		{ ".ags", AssetType::Mesh }, // X-Plane Autogen strings
		{ ".fac", AssetType::Mesh }, // X-Plane facade format
		{ ".for", AssetType::Mesh }, // X-Plane forest format
		{ ".net", AssetType::Mesh }, // X-Plane network format
		{ ".dsf", AssetType::Mesh }, // X-Plane DSF format
		{ ".ter", AssetType::Mesh }, // X-Plane terrain format
		{ ".lin", AssetType::Mesh }, // X-Plane painted line format
		{ ".pol", AssetType::Mesh }, // X-Plane polygon format
		{ ".str", AssetType::Mesh }, // X-Plane string format
		{ ".dat", AssetType::Mesh }, // X-Plane data format
	};

	/**
	 * @brief Checks if the given asset type has a valid extension.
	 * @param type The asset type to check.
	 * @return True if the asset type has a valid extension, false otherwise.
	 */
	inline static bool IsValidExtension(const AssetType type)
	{
		for (const auto &val : s_AssetExtensionMap | std::views::values)
		{
			if (val == type)
				return true;
		}

		return false;
	}

	/**
	 * @brief Gets the asset type based on the given asset type.
	 * @param type The asset type to check.
	 * @return The corresponding asset type if found, AssetType::None otherwise.
	 */
	inline static AssetType GetAssetType(const AssetType type)
	{
		for (const auto &assetType : s_AssetExtensionMap | std::views::values)
		{
			if (assetType == type)
				return assetType;
		}

		return AssetType::None;
	}

	/**
	 * @brief Gets the asset type based on the given file extension.
	 * @param type The file extension to check.
	 * @return The corresponding asset type if found, AssetType::None otherwise.
	 */
	inline static AssetType GetAssetType(const std::string &type)
	{
		for (const auto &[ext, assetType] : s_AssetExtensionMap)
		{
			if (ext == type)
				return assetType;
		}

		return AssetType::None;
	}
}

// -------------------------------------------------------

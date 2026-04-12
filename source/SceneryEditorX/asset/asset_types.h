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
 * asset_types.h
 * -------------------------------------------------------
 * Created: 09/03/2026
 * -------------------------------------------------------
 */
#pragma once

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @enum AssetFlag
	 * @brief Enumeration of asset flags used to indicate the status of an asset in the editor.
	 */
	enum class AssetFlag : uint16_t
	{
		None	= 0,
		Missing = BIT(0),
		Invalid = BIT(1)
	};

	/**
	 * @enum AssetType
	 * @brief Enumeration of asset types used in the editor. 
	 */
	enum class AssetType : uint16_t
	{
		None = 0,
		Scene,
		Prefab,
		Mesh,
		Material,
		Texture,
		EnvMap,
		Font,
		Script,
		ScriptFile,
		MeshCollider,
		Skeleton,
		Animation,
		MaxEnum
	};

	/**
	 * @brief Converts a string representation of an asset type to its corresponding AssetType enum value.
	 * @param assetType the string representation of the asset type to convert (e.g., "Mesh", "Material").
	 * @return the corresponding AssetType enum value, or AssetType::None if the string does not match any known asset type.
	 */
	inline AssetType AssetTypeFromString(std::string_view assetType)
	{
		if (assetType == "None")                return AssetType::None;
		if (assetType == "Scene")               return AssetType::Scene;
		if (assetType == "Prefab")              return AssetType::Prefab;
		if (assetType == "Mesh")                return AssetType::Mesh;
		if (assetType == "Material")            return AssetType::Material;
		if (assetType == "Texture")             return AssetType::Texture;
		if (assetType == "EnvMap")              return AssetType::EnvMap;
		if (assetType == "Font")                return AssetType::Font;
		if (assetType == "Script")              return AssetType::Script;
		if (assetType == "ScriptFile")          return AssetType::ScriptFile;
		if (assetType == "MeshCollider")        return AssetType::MeshCollider;
		if (assetType == "Skeleton")            return AssetType::Skeleton;

		return AssetType::None;
	}

	/**
	 * @brief Converts an AssetType enum value to its corresponding string representation.
	 * @param assetType the AssetType enum value to convert to a string.
	 * @return the string representation of the given AssetType enum value.
	 */
	inline const char* AssetTypeToString(AssetType assetType)
	{
		switch (assetType)
		{
			case AssetType::None:                return "None";
			case AssetType::Scene:               return "Scene";
			case AssetType::Prefab:              return "Prefab";
			case AssetType::Mesh:                return "Mesh";
			case AssetType::Material:            return "Material";
			case AssetType::Texture:             return "Texture";
			case AssetType::EnvMap:              return "EnvMap";
			case AssetType::Font:                return "Font";
			case AssetType::Script:              return "Script";
			case AssetType::ScriptFile:          return "ScriptFile";
			case AssetType::MeshCollider:        return "MeshCollider";
			case AssetType::Skeleton:            return "Skeleton";
			case AssetType::Animation:           return "Animation";
		}

		SEDX_CORE_ASSERT(false, "Unknown Asset Type");
		return "None";
	}

}

// -------------------------------------------------------

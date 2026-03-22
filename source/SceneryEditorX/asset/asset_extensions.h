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

}

// -------------------------------------------------------

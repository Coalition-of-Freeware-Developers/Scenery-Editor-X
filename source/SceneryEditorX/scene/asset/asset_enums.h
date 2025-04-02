/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* asset_enums.h
* -------------------------------------------------------
* Created: 2/4/2025
* -------------------------------------------------------
*/

#pragma once
#include <cstdint>
#include <SceneryEditorX/core/base.hpp>

// -------------------------------------------------------

namespace SceneryEditorX
{
	enum class AssetFlag : uint16_t
	{
	    None = 0,
	    Missing = BIT(0),
	    Invalid = BIT(1)
	};

	enum class AssetType : uint16_t
    {
        None = 0,
        Scene,
        Prefab,
        Mesh,
        StaticMesh,
        MeshSource,
        Material,
        Texture,
        Font,
        Script,
        ScriptFile,
        MeshCollider,
		Light
    };

} // namespace SceneryEditorX

// -------------------------------------------------------

/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* scene_environment.h
* -------------------------------------------------------
* Created: 23/12/2025
* -------------------------------------------------------
*/
#pragma once
#include "texture.h"
#include "SceneryEditorX/asset/asset.h"
#include "SceneryEditorX/asset/asset_types.h"

// -------------------------------------------------------

namespace SceneryEditorX
{
	class Environment : public Asset
	{
	public:
	    Ref<TextureCube> RadianceMap;
	    Ref<TextureCube> IrradianceMap;
	
	    Environment() = default;
	    Environment(const Ref<TextureCube> &radianceMap, const Ref<TextureCube> &irradianceMap) : RadianceMap(radianceMap), IrradianceMap(irradianceMap)
	    {
	    }
	
	    static AssetType GetStaticType() { return AssetType::EnvMap; }
	    AssetType GetAssetType() const { return GetStaticType(); }
	};

}

// -------------------------------------------------------

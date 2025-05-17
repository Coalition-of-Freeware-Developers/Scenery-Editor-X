/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* material_manager.cpp
* -------------------------------------------------------
* Created: 8/5/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/scene/material_manager.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	std::shared_ptr<MaterialAsset> MaterialManager::LoadMaterial(const std::string &path)
	{
        if (auto it = materials.find(path); it != materials.end())
	    {
	        return it->second;
	    }
	
	    auto material = std::make_shared<MaterialAsset>(path);
	    materials[path] = material;
	    return material;
	}
	
	void MaterialManager::UnloadMaterial(const std::string &path)
	{
        if (auto it = materials.find(path); it != materials.end())
	    {
	        materials.erase(it);
	    }
	}

} // namespace SceneryEditorX

// -------------------------------------------------------

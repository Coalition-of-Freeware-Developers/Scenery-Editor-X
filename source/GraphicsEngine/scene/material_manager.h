/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* material_manager.h
* -------------------------------------------------------
* Created: 8/5/2025
* -------------------------------------------------------
*/
#pragma once
#include <GraphicsEngine/scene/material.h>
#include <memory>
#include <string>
#include <unordered_map>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class MaterialManager
	{
	public:
	    MaterialManager() = default;
	    ~MaterialManager() = default;
	
	    std::shared_ptr<MaterialAsset> LoadMaterial(const std::string &path);
	    void UnloadMaterial(const std::string &path);
	
	private:
	    std::unordered_map<std::string, std::shared_ptr<MaterialAsset>> materials;
	};
	
} // namespace SceneryEditorX

// -------------------------------------------------------

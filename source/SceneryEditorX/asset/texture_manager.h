/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* texture_manager.h
* -------------------------------------------------------
* Created: 8/5/2025
* -------------------------------------------------------
*/
#pragma once
#include <memory>
#include <SceneryEditorX/scene/texture.h>
#include <string>
#include <unordered_map>

/// -------------------------------------------------------

namespace SceneryEditorX
{

	class TextureManager
	{
	public:
	    TextureManager() = default;
	    ~TextureManager() = default;
	
	    std::shared_ptr<TextureAsset> LoadTexture(const std::string &path);
	    void UnloadTexture(const std::string &path);
	
	private:
	    std::unordered_map<std::string, std::shared_ptr<TextureAsset>> textures;
	};
	
} // namespace SceneryEditorX

/// -------------------------------------------------------

/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* texture_manager.cpp
* -------------------------------------------------------
* Created: 8/5/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/asset/texture_manager.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

	std::shared_ptr<TextureAsset> TextureManager::LoadTexture(const std::string &path)
	{
        if (const auto it = textures.find(path); it != textures.end())
	    {
	        return it->second;
	    }
	
	    auto texture = std::make_shared<TextureAsset>(path);
	    textures[path] = texture;
	    return texture;
	}
	
	void TextureManager::UnloadTexture(const std::string &path)
	{
        if (const auto it = textures.find(path); it != textures.end())
	    {
	        textures.erase(it);
	    }
	}


}

/// -------------------------------------------------------

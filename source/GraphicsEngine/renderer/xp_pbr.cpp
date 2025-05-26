/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* xp_pbr.cpp
* -------------------------------------------------------
* Created: 9/5/2025
* -------------------------------------------------------
*/

#include <GraphicsEngine/scene/material.h>
#include <GraphicsEngine/renderer/xp_pbr.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

    PBR::PBR(const std::string &path) : MaterialAsset(path)
    {
        //TextureAsset::Load(path);
    }

    PBR::~PBR()
    {
        // Unload PBR texture data
        //TextureAsset::Unload();
    }
}

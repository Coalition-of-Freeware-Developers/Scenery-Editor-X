/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* editor.h
* -------------------------------------------------------
* Created: 25/3/2025
* -------------------------------------------------------
*/
#pragma once

#include <SceneryEditorX/core/base.hpp>
#include <SceneryEditorX/scene/asset_manager.h>

struct Camera;
struct AssetManager;
struct SceneAsset;

// -------------------------------------------------------
struct ImDrawData;

class Editor
{
    Editor();
    ~Editor();

	void beginFrame();
    ImDrawData *endFrame();

	// -------------------------------------------------------

    bool ViewportPanel(vkw::Image &image, glm::ivec2 &newSize);

	void ScenePanel(Ref<SceneAsset> &scene);
    void AssetsPanel(AssetManager &assetManager);
};

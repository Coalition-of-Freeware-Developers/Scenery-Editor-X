/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* scene.cpp
* -------------------------------------------------------
* Created: 2/4/2025
* -------------------------------------------------------
*/

#include <SceneryEditorX/core/editor_events.h>
#include <SceneryEditorX/scene/scene.h>
#include <SceneryEditorX/scene/scene_ref.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	
	Scene::Scene(const std::string &name, bool isEditorScene, bool initalize)
	{
    }

    Scene::~Scene()
    {
    }

	void Scene::SetViewportBounds(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom)
    {
        Viewport_Left = left;
        Viewport_Top = top;
        Viewport_Right = right;
        Viewport_Bottom = bottom;
    }

	Ref<Scene> Scene::CreateEmpty()
    {
        return Ref<Scene>::Create("Empty", false, false);
    }

} // namespace SceneryEditorX

// -------------------------------------------------------

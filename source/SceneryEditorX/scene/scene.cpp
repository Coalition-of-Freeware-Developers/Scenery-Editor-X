/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* scene.cpp
* -------------------------------------------------------
* Created: 7/4/2025
* -------------------------------------------------------
*/

#include <SceneryEditorX/scene/scene.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	Scene::Scene(const std::string &name) : m_Name(name)
	{
	    m_SceneRegistry = CreateRef<SceneRegistry>();
	    m_SceneRegistry->SetScene(this);
	}
	Scene::~Scene()
	{
	    m_SceneRegistry->Clear();
	}
	void Scene::OnUpdate(Timestep ts)
	{
	    // Update all entities in the scene
	    for (auto &entity : m_Entities)
	    {
	        entity.second->OnUpdate(ts);
	    }
	}

} // namespace SceneryEditorX

// -------------------------------------------------------

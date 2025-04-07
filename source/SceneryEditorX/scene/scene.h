/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* scene.h
* -------------------------------------------------------
* Created: 7/4/2025
* -------------------------------------------------------
*/
#pragma once
#include <rpcdce.h>
#include <SceneryEditorX/core/time/delta_time.h>
#include <SceneryEditorX/core/templates/ref_templates.h>
#include <string>
#include <unordered_map>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class Scene
	{
	public:
		Scene(const std::string &name);
		~Scene();
		void OnUpdate(Timestep ts);
		// Other scene methods...
	private:
		std::string m_Name;
		Ref<SceneRegistry> m_SceneRegistry;
		std::unordered_map<UUID, Ref<Entity>> m_Entities;
	};

} // namespace SceneryEditorX

// -----------------------------------------

/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* scene.h
* -------------------------------------------------------
* Created: 2/4/2025
* -------------------------------------------------------
*/
#pragma once

#include <SceneryEditorX/core/timer.h>
#include <SceneryEditorX/core/events.h>
#include <SceneryEditorX/scene/asset_manager.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class Scene
	{
	public:
        Scene(const std::string &name = "NewAirport", bool isEditorScene = false, bool initalize = true);
        ~Scene();

        void SetViewportBounds(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom);

       static SRef<Scene> CreateEmpty();


		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnUpdate(Timer ts) {}
		virtual void OnRender() {}
		virtual void OnImGuiRender() {}
		virtual void OnEvent(Event& e) {}

	private:
        std::string m_Name;
        bool m_IsEditorScene = false;
        uint32_t m_ViewportTop = 0;
        uint32_t m_ViewportLeft = 0;
        uint32_t m_ViewportRight = 0;
        uint32_t m_ViewportBottom = 0;
	};

} // namespace SceneryEditorX

// -------------------------------------------------------

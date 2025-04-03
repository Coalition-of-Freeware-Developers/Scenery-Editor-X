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

#include <SceneryEditorX/core/delta_time.h>
#include <SceneryEditorX/core/events.h>
#include <SceneryEditorX/scene/asset_manager.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class Scene : public Asset
	{
	public:
        Scene(const std::string &name = "NewAirport", bool isEditorScene = false, bool initalize = true);
        ~Scene();

        void SetViewportBounds(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom);

       static Ref<Scene> CreateEmpty();


		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnUpdate(DeltaTime ts) {}
		virtual void OnUIRender() {}
		virtual void OnImGuiRender() {}
		virtual void OnEvent(Event& e) {}

	private:
        std::string Name_;
        bool isEditorScene_ = false;
        uint32_t Viewport_Top = 0;
        uint32_t Viewport_Left = 0;
        uint32_t Viewport_Right = 0;
        uint32_t Viewport_Bottom = 0;
	};

} // namespace SceneryEditorX

// -------------------------------------------------------

/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* layer.h
* -------------------------------------------------------
* Created: 28/3/2025
* -------------------------------------------------------
*/

#pragma once

#include <SceneryEditorX/core/timer.h>
#include <SceneryEditorX/core/events.h>

// -------------------------------------------------------

namespace SceneryEditorX :: UI
{
	class Layer
	{
	public:
        Layer(const std::string &name = "Layer");
		virtual ~Layer();

		virtual void OnAttach() {}
        virtual void OnDetach() {}
        virtual void OnUpdate(Timer ts) {}
        virtual void OnRender() {}
        virtual void OnEvent(Event& event) {}

		inline const std::string& GetName() const { return m_DebugName; }
    protected:
		std::string m_DebugName;
	};

} // namespace SceneryEditorX :: UI

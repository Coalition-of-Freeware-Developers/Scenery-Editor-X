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

#include <SceneryEditorX/core/delta_time.h>
#include <SceneryEditorX/core/events.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class Layer
	{
	public:

        Layer(const std::string &name = "Layer");
		virtual ~Layer();

		virtual void Begin() = 0;
        virtual void End() = 0;

		virtual void OnAttach() {}
        virtual void OnDetach() {}
        virtual void OnUpdate(DeltaTime ts) {}
        virtual void OnUIRender() {}
        virtual void OnEvent(Event& event) {}

		inline const std::string& GetName() const { return DebugName_; }

    protected:
		std::string DebugName_;
	};

} // namespace SceneryEditorX

// -------------------------------------------------------

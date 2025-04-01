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
#include <string>

// -------------------------------------------------------

namespace UI
{
	class Layer
	{
	public:
        Layer(const std::string &name = "Layer");
		virtual ~Layer();
		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnUpdate() {}
		virtual void OnImGuiRender() {}
		virtual void on_event() {}

    protected:


	};

} // namespace SceneryEditorX

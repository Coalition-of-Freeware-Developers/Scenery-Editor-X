/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* layer_stack.cpp
* -------------------------------------------------------
* Created: 22/6/2025
* -------------------------------------------------------
*/
#include "layer_stack.h"

// -------------------------------------------------------

namespace SceneryEditorX
{
	void LayerStack::PushLayer(Layer *layer)
	{
        m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, layer);
        m_LayerInsertIndex++;
	}

	void LayerStack::PushOverlay(Layer *overlay)
	{
        m_Layers.emplace_back(overlay);
	}

	void LayerStack::PopLayer(Layer *layer)
	{
        if (const auto set = std::ranges::find(m_Layers, layer); set != m_Layers.end())
		{
			m_Layers.erase(set);
			m_LayerInsertIndex--;
		}
		else
            SEDX_CORE_ERROR("Layer not found in stage");
    }

	void LayerStack::PopOverlay(Layer *overlay)
	{
        if (const auto set = std::ranges::find(m_Layers.begin(), m_Layers.end(), overlay); set != m_Layers.end())
            m_Layers.erase(set);
        else
            SEDX_CORE_ERROR("Overlay not found in stage");
	}

}

// -------------------------------------------------------

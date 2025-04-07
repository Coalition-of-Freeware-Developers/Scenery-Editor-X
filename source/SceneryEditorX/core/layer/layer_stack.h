/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* layer_stack.h
* -------------------------------------------------------
* Created: 5/4/2025
* -------------------------------------------------------
*/

#pragma once

#include <SceneryEditorX/core/layer/layer.h>
#include <SceneryEditorX/logging/asserts.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class LayerStack
	{
	public:
		LayerStack();
		~LayerStack();

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);
		void PopLayer(Layer* layer);
		void PopOverlay(Layer* overlay);

		Layer* operator[](size_t index)
		{
			//SEDX_CORE_ASSERT(index >= 0 && index < m_Layers.size());
			return m_Layers[index];
		}

		const Layer* operator[](size_t index) const
		{
			//SEDX_CORE_ASSERT(index >= 0 && index < m_Layers.size());
			return m_Layers[index];
		}

		size_t Size() const { return m_Layers.size(); }

		std::vector<Layer*>::iterator begin() { return m_Layers.begin(); }
		std::vector<Layer*>::iterator end() { return m_Layers.end(); }
	private:
		std::vector<Layer*> m_Layers;
		unsigned int m_LayerInsertIndex = 0;
	};

} // namespace SceneryEditorX

// -------------------------------------------------------

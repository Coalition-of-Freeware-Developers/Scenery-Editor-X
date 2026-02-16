/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray 
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * layer_stack.cpp
 * -------------------------------------------------------
 * Created: 22/6/2025
 * -------------------------------------------------------
 */
#include "layer_stack.h"
#include <tracy/Tracy.hpp>

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

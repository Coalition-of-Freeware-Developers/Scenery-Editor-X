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
 * layer_stack.h
 * -------------------------------------------------------
 * Created: 22/6/2025
 * -------------------------------------------------------
 */
#pragma once
#include "layer.h"

// -------------------------------------------------------

namespace SceneryEditorX
{

	/**
	 * @brief Manages a collection of layers with proper ordering and lifecycle control
	 * 
	 * The LayerStack maintains a two-section container structure for managing application layers:
	 * - **Layers Section**: Core application layers inserted at the front (processed first)
	 * - **Overlays Section**: UI and debug overlays appended at the end (processed last)
	 * 
	 * This ordering ensures that core application logic executes before UI rendering and
	 * input processing, which is critical for proper event handling and rendering order.
	 * 
	 * **Usage Pattern:**
	 * ```cpp
	 * LayerStack layerStack;
	 * 
	 * // Push core layers first - these are inserted at the front
	 * layerStack.PushLayer(terrainLayer);
	 * layerStack.PushLayer(renderLayer);
	 * 
	 * // Push overlays last - these are appended to the end
	 * layerStack.PushOverlay(debugOverlay);
	 * layerStack.PushOverlay(uiOverlay);
	 * 
	 * // Final order: [terrainLayer, renderLayer, debugOverlay, uiOverlay]
	 * ```
	 * 
	 * @note The LayerStack does not manage layer lifetime - layers must be managed externally
	 * @note Layers are stored as raw pointers for performance and to avoid ownership issues
	 */
	class LayerStack
	{
	public:
		LayerStack() = default;
		~LayerStack() = default;

		/**
		 * @brief Pushes a layer to the front section of the stack
		 * 
		 * Inserts the layer at the current layer insert index, ensuring it processes before
		 * any overlays but after previously added layers. Use this for core application layers
		 * that contain primary logic and rendering.
		 * 
		 * **Order Example:**
		 * ```cpp
		 * PushLayer(layer1);  // [layer1]
		 * PushLayer(layer2);  // [layer1, layer2]
		 * PushOverlay(ui);    // [layer1, layer2, ui]
		 * PushLayer(layer3);  // [layer1, layer2, layer3, ui]
		 * ```
		 * 
		 * @param layer Pointer to the layer to add (must not be null)
		 * @note Does not take ownership of the layer - caller manages lifetime
		 */
		void PushLayer(Layer *layer);

		/**
		 * @brief Pushes an overlay to the end section of the stack
		 * 
		 * Appends the overlay to the back of the stack, ensuring it processes after all
		 * layers and previously added overlays. Use this for UI panels, debug visualizations,
		 * and other overlay systems that need to render on top.
		 * 
		 * **Order Example:**
		 * ```cpp
		 * PushLayer(gameLayer);     // [gameLayer]
		 * PushOverlay(debugUI);     // [gameLayer, debugUI]
		 * PushOverlay(consoleUI);   // [gameLayer, debugUI, consoleUI]
		 * ```
		 * 
		 * @param overlay Pointer to the overlay to add (must not be null)
		 * @note Does not take ownership of the overlay - caller manages lifetime
		 */
		void PushOverlay(Layer *overlay);

		/**
		 * @brief Removes a layer from the front section of the stack
		 * 
		 * Searches for the specified layer in the stack and removes it if found. This method
		 * should only be used for layers added with PushLayer(). The layer insert index is
		 * decremented to maintain proper ordering.
		 * 
		 * @param layer Pointer to the layer to remove
		 * @note Logs an error if the layer is not found in the stack
		 * @note Does not delete the layer - caller is responsible for cleanup
		 */
		void PopLayer(Layer *layer);

		/**
		 * @brief Removes an overlay from the end section of the stack
		 * 
		 * Searches for the specified overlay in the stack and removes it if found. This method
		 * should only be used for overlays added with PushOverlay().
		 * 
		 * @param overlay Pointer to the overlay to remove
		 * @note Logs an error if the overlay is not found in the stack
		 * @note Does not delete the overlay - caller is responsible for cleanup
		 */
		void PopOverlay(Layer *overlay);

		/**
		 * @brief Accesses a layer by index with bounds checking
		 * 
		 * Provides direct access to layers in the stack by their index position. Asserts
		 * if the index is out of bounds in debug builds.
		 * 
		 * @param index Zero-based index of the layer to access
		 * @return Pointer to the layer at the specified index
		 * @note Asserts if index is out of bounds (index < 0 or index >= Size())
		 */
		Layer *operator[](size_t index)
		{
			SEDX_CORE_ASSERT(index >= 0 && index < m_Layers.size());
			return m_Layers[index];
		}

		/**
		 * @brief Accesses a layer by index with bounds checking (const version).
		 * 
		 * Provides direct const access to layers in the stack by their index position.
		 * Asserts if the index is out of bounds in debug builds.
		 * 
		 * @param index Zero-based index of the layer to access.
		 * @return Const pointer to the layer at the specified index.
		 * @note Asserts if index is out of bounds (index < 0 or index >= Size()).
		 */
		const Layer *operator[](size_t index) const 
		{
			SEDX_CORE_ASSERT(index >= 0 && index < m_Layers.size());
			return m_Layers[index];
		}

		/**
		 * @brief Returns the total number of layers in the stack
		 * @return The count of all layers and overlays currently in the stack
		 */
		[[nodiscard]] size_t Size() const { return m_Layers.size(); }

		/**
		 * @brief Returns an iterator to the beginning of the layer stack.
		 * Enables range-based for loops and standard algorithm usage.
		 * @return Iterator pointing to the first layer (if any)
		 */
		std::vector<Layer*>::iterator Begin() { return m_Layers.begin(); }

		/**
		 * @brief Returns an iterator to the end of the layer stack.
		 *
		 * Enables range-based for loops and standard algorithm usage.
		 *
		 * @return Iterator pointing past the last layer
		 */
		std::vector<Layer*>::iterator End() { return m_Layers.end(); }

	private:
		std::vector<Layer*> m_Layers;
		unsigned int m_LayerInsertIndex = 0;
	};
}

// -------------------------------------------------------

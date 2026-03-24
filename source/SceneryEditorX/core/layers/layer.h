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
 * layer.h
 * -------------------------------------------------------
 * Created: 27/5/2025
 * -------------------------------------------------------
 */
#pragma once
#include <SceneryEditorX/core/events/event_system.h>
#include <SceneryEditorX/core/time/time.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @class Layer
	 * @brief Base "Module" class representing a logical section of the application.
	 *
	 * Layer are used to organize different functional components of the application
	 * that can be attached, detached and updated independently. Each module can handle
	 * its own rendering, events, and update logic.
	 */
	class Layer
	{
	public:
		/**
		 * @brief Construct a modular component that can be plugged into the application.
		 * @param name The name of the module.
		 */
		Layer(const std::string &name = "Layer");
		virtual ~Layer();

		/**
		 * @brief Called when the module is attached to the application.
		 * Use this for initialization of module resources.
		 */
		virtual void OnAttach() {}

		/**
		 * @brief Called when the module is detached from the application.
		 * Use this for cleanup of module resources.
		 */
		virtual void OnDetach() {}

		/**
		 * @brief Called each frame to update the module.
		 * Implement module logic that needs to execute each frame.
		 */
		virtual void Tick() {}

		/**
		 * @brief Renders ImGui elements for this module.
		 * Implement any ImGui rendering for debugging or UI components.
		 */
		virtual void OnRender() {}

		/**
		 * @brief Processes events for this module.
		 * Handle input or application events specific to this module.
		 */
		virtual void OnEvent(Event &event) {}

		inline const std::string& GetName() const { return m_DebugName; }

	protected:
		std::string m_DebugName; // The name of the module.
	};

}

// -------------------------------------------------------

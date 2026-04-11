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
 * editor_panel.h
 * -------------------------------------------------------
 * Created: 10/04/2026
 * -------------------------------------------------------
 */
#pragma once
#include <SceneryEditorX/core/events/event_system.h>
#include <SceneryEditorX/project/project.h>
#include <SceneryEditorX/scene/scene.h>

// -------------------------------------------------------

namespace SceneryEditorX::UI
{
	/**
	 * @class EditorPanel
	 * @brief Abstract base class for all editor panels.
	 */
	class EditorPanel : public RefCounted
	{
	public:
		/**
		 * @brief Destructor for the EditorPanel class.
		 */
		virtual ~EditorPanel() = default;

		/**
		 * @brief Renders the UI for the panel.
		 * @param isOpen A reference to a boolean indicating whether the panel is open.
		 */
		virtual void OnUIRender(bool& isOpen) = 0;

		/**
		 * @brief Handles an event.
		 * @param e Event to handle.
		 */
		virtual void OnEvent(Event& e) {}

		/**
		 * @brief Called when the active project changes.
		 * @param project The new active project.
		 */
		virtual void OnProjectChanged(const Ref<Project>& project) {}

		/**
		 * @brief Sets the scene context for the panel.
		 * @param context scene context to set for the panel
		 * @note This is called when the active scene changes, 
		 * allowing panels to update their context accordingly.
		 */
		virtual void SetSceneContext(const Ref<Scene>& context) {}

	};

}

// -------------------------------------------------------

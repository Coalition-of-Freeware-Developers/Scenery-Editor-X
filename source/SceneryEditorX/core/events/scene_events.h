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
 * scene_events.h
 * -------------------------------------------------------
 * Created: 14/7/2025
 * -------------------------------------------------------
 */
#pragma once
#include <SceneryEditorX/core/events/event_system.h>
#include <SceneryEditorX/scene/scene.h>

// -------------------------------------------------------------------


namespace SceneryEditorX
{
	// Forward-declare selection context enum so events can reference it
	// Ensure the forward declaration uses the same underlying type as the
	// definition in selection_manager.h to avoid mismatched underlying type
	// errors when compiling across translation units.
	enum class SelectionContext : unsigned char;

	class SceneEvent : public Event
	{
	public:
		[[nodiscard]] const Ref<Scene>& GetScene() const { return m_Scene; }
		Ref<Scene> GetScene() { return m_Scene; }

		EVENT_CLASS_CATEGORY(EventCategoryApplication | EventCategoryScene)
	protected:
		SceneEvent(const Ref<Scene>& scene) : m_Scene(scene) {}
		Ref<Scene> m_Scene;
	};

	class SelectionChangedEvent : public Event
	{
	public:
		// Selection events are emitted with a SelectionContext (enum) and the UUID of the
		// object that changed selection state.
		SelectionChangedEvent(SelectionContext contextID, UUID selectionID, const bool selected)
			: m_ContextID(contextID), m_SelectionID(std::move(selectionID)), m_Selected(selected) {}

		[[nodiscard]] SelectionContext GetContextID() const { return m_ContextID; }
		[[nodiscard]] UUID GetSelectionID() const { return m_SelectionID; }
		[[nodiscard]] bool IsSelected() const { return m_Selected; }
		[[nodiscard]] virtual std::string ToString() const override
		{
			std::stringstream ss;
			ss << "EntitySelectionChangedEvent: Context(" << static_cast<int>(m_ContextID) << "), Selection(" << m_SelectionID << "), " << m_Selected;
			return ss.str();
		}

		EVENT_CLASS_CATEGORY(EventCategoryScene)
		EVENT_CLASS_TYPE(SelectionChanged)
	private:
		SelectionContext m_ContextID;
		UUID m_SelectionID;
		bool m_Selected;
	};

}

// -------------------------------------------------------------------

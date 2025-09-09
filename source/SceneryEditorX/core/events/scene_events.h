/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* scene_events.h
* -------------------------------------------------------
* Created: 14/7/2025
* -------------------------------------------------------
*/
#pragma once
//#include <SceneryEditorX/core/events/event_system.h>
//#include <SceneryEditorX/scene/scene.h>

/// -------------------------------------------------------------------

/*
namespace SceneryEditorX
{
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

    /// -------------------------------------------------------------------

	class ScenePreStartEvent : public SceneEvent
	{
	public:
		ScenePreStartEvent(const Ref<Scene>& scene) : SceneEvent(scene) {}

		[[nodiscard]] virtual std::string ToString() const override
		{
			std::stringstream ss;
			ss << "ScenePreStartEvent: " << m_Scene->GetName();
			return ss.str();
		}

		EVENT_CLASS_TYPE(ScenePreStart)
	};

	class ScenePostStartEvent : public SceneEvent
	{
	public:
		ScenePostStartEvent(const Ref<Scene>& scene) : SceneEvent(scene) {}

		[[nodiscard]] virtual std::string ToString() const override
		{
			std::stringstream ss;
			ss << "ScenePostStartEvent: " << m_Scene->GetName();
			return ss.str();
		}

		EVENT_CLASS_TYPE(ScenePostStart)
	};

	class ScenePreStopEvent : public SceneEvent
	{
	public:
		ScenePreStopEvent(const Ref<Scene>& scene) : SceneEvent(scene) {}

        [[nodiscard]] virtual std::string ToString() const override
		{
			std::stringstream ss;
			ss << "ScenePreStopEvent: " << m_Scene->GetName();
			return ss.str();
		}

		EVENT_CLASS_TYPE(ScenePreStop)
	};

	class ScenePostStopEvent : public SceneEvent
	{
	public:
        explicit ScenePostStopEvent(const Ref<Scene>& scene) : SceneEvent(scene) {}

        [[nodiscard]] virtual std::string ToString() const override
		{
			std::stringstream ss;
			ss << "ScenePostStopEvent: " << m_Scene->GetName();
			return ss.str();
		}

		EVENT_CLASS_TYPE(ScenePostStop)
	};

	class SelectionChangedEvent : public Event
	{
	public:
		SelectionChangedEvent(const UUID &contextID, const UUID &selectionID, const bool selected) : m_ContextID(contextID), m_SelectionID(selectionID), m_Selected(selected) {}

        [[nodiscard]] UUID GetContextID() const { return m_ContextID; }
        [[nodiscard]] UUID GetSelectionID() const { return m_SelectionID; }
        [[nodiscard]] bool IsSelected() const { return m_Selected; }
        [[nodiscard]] virtual std::string ToString() const override
		{
			std::stringstream ss;
			ss << "EntitySelectionChangedEvent: Context(" << m_ContextID << "), Selection(" << m_SelectionID << "), " << m_Selected;
			return ss.str();
		}

		EVENT_CLASS_CATEGORY(EventCategoryScene)
		EVENT_CLASS_TYPE(SelectionChanged)
	private:
		UUID m_ContextID;
		UUID m_SelectionID;
		bool m_Selected;
	};


}
*/

/// -------------------------------------------------------------------

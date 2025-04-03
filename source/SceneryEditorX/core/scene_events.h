/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* scene_events.h
* -------------------------------------------------------
* Created: 2/4/2025
* -------------------------------------------------------
*/

#pragma once

#include <SceneryEditorX/core/events.h>
#include <SceneryEditorX/core/key_codes.h>
#include <SceneryEditorX/scene/scene.h>
#include <SceneryEditorX/scene/scene_ref.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class SceneEvent : public Event
	{
	public:
		const Ref<Scene>& GetScene() const { return Scene_; }
		Ref<Scene> GetScene() { return Scene_; }

		EVENT_CLASS_CATEGORY(EventCategoryApplication | EventCategoryScene)
	protected:
		SceneEvent(const Ref<Scene>& scene) : Scene_(scene) {}

		Ref<Scene> Scene_;
	};

	// -------------------------------------------------------

	class ScenePreStartEvent : public SceneEvent
	{
	public:
		ScenePreStartEvent(const Ref<Scene>& scene) : SceneEvent(scene) {}

		EVENT_CLASS_TYPE(ScenePreStart)
	};

	// -------------------------------------------------------

	class ScenePostStartEvent : public SceneEvent
	{
	public:
		ScenePostStartEvent(const Ref<Scene>& scene) : SceneEvent(scene) {}

		EVENT_CLASS_TYPE(ScenePostStart)
	};

	// -------------------------------------------------------

	class ScenePreStopEvent : public SceneEvent
	{
	public:
		ScenePreStopEvent(const Ref<Scene>& scene) : SceneEvent(scene) {}

		EVENT_CLASS_TYPE(ScenePreStop)
	};

	// -------------------------------------------------------

	class ScenePostStopEvent : public SceneEvent
	{
	public:
		ScenePostStopEvent(const Ref<Scene>& scene) : SceneEvent(scene) {}

		EVENT_CLASS_TYPE(ScenePostStop)
	};

	// -------------------------------------------------------

	//class SelectionChangedEvent : public Event
	//{
	//public:
	//	SelectionChangedEvent(UUID contextID, UUID selectionID, bool selected) : m_ContextID(contextID), m_SelectionID(selectionID), m_Selected(selected) {}
	//	
	//	UUID GetContextID() const { return m_ContextID; }
	//	UUID GetSelectionID() const { return m_SelectionID; }
	//	bool IsSelected() const { return m_Selected; }
	//
	//
	//	EVENT_CLASS_CATEGORY(EventCategoryScene)
	//	EVENT_CLASS_TYPE(SelectionChanged)
	//private:
	//	UUID m_ContextID;
	//	UUID m_SelectionID;
	//	bool m_Selected;
	//};

} // namespace SceneryEditorX

// -------------------------------------------------------

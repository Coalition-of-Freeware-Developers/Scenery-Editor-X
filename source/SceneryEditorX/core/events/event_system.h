/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* event_system.h
* -------------------------------------------------------
* Created: 14/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <functional>
#include <ostream>
#include <string>

/// -------------------------------------------------------------------

namespace SceneryEditorX
{

	enum class EventType : uint8_t
	{
		None = 0,
		WindowClose,
	    WindowMinimize,
	    WindowResize,
	    WindowFocus,
	    WindowLostFocus,
	    WindowMoved,
	    WindowTitleBarHitTest,
		AppTick,
	    AppUpdate,
	    AppRender,
		KeyPressed,
	    KeyReleased,
	    KeyTyped,
		MouseButtonPressed,
	    MouseButtonReleased,
	    MouseButtonDown,
	    MouseMoved,
	    MouseScrolled,
		ScenePreStart,
	    ScenePostStart,
	    ScenePreStop,
	    ScenePostStop,
		EditorExitPlayMode,
		SelectionChanged,
		AssetReloaded,
		AnimationGraphCompiled
	};
	
	enum EventCategory : uint8_t
	{
		None = 0,
		EventCategoryApplication    = BIT(0),
		EventCategoryInput          = BIT(1),
		EventCategoryKeyboard       = BIT(2),
		EventCategoryMouse          = BIT(3),
		EventCategoryMouseButton    = BIT(4),
		EventCategoryScene			= BIT(5),
		EventCategoryEditor			= BIT(6)
	};

    #define EVENT_CLASS_TYPE(type) static EventType GetStaticType() { return EventType::type; }\
								virtual EventType GetEventType() const override { return GetStaticType(); }\
								virtual const char* GetName() const override { return #type; }

    #define EVENT_CLASS_CATEGORY(category) virtual int GetCategoryFlags() const override { return category; }

    /// -------------------------------------------------------------------

	class Event
	{
	public:
		bool Handled = false;
		bool Synced = false; // Queued events are only processed if this is true.  It is set true when asset thread syncs with main thread.

		virtual ~Event() = default;
        [[nodiscard]] virtual EventType GetEventType() const = 0;
		[[nodiscard]] virtual const char* GetName() const = 0;
        [[nodiscard]] virtual int GetCategoryFlags() const = 0;
        [[nodiscard]] virtual std::string ToString() const { return GetName(); }

		inline bool IsInCategory(EventCategory category) const
        {
			return GetCategoryFlags() & category;
		}
	};


    /// -------------------------------------------------------------------

	class EventDispatcher
	{
	template<typename T>
	using EventFn = std::function<bool(T&)>;
	public:
        explicit EventDispatcher(Event& event) : m_Event(event) {}

		template<typename T>
		bool Dispatch(EventFn<T> func)
		{
			if (m_Event.GetEventType() == T::GetStaticType() && !m_Event.Handled)
			{
				m_Event.Handled = func(*(T*)&m_Event);
				return true;
			}

			return false;
		}
	private:
		Event& m_Event;
	};

    /// -------------------------------------------------------------------

	inline std::ostream& operator<<(std::ostream& os, const Event& e)
	{
		return os << e.ToString();
	}

    /// -------------------------------------------------------------------

}

/// -------------------------------------------------------------------

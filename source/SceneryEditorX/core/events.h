/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* events.h
* -------------------------------------------------------
* Created: 2/4/2025
* -------------------------------------------------------
*/

#pragma once

#include <functional>
#include <string>

// -------------------------------------------------------

namespace SceneryEditorX
{
	enum class EventType
	{
	    None = 0,
	    WindowClose,
		WindowMinimize,
	    WindowResize,
	    WindowFocus,
	    WindowLostFocus,
	    WindowMoved,
    // -- Application Events
	    AppTick,
	    AppUpdate,
	    AppRender,
    // -- Key Events
	    KeyPressed,
	    KeyReleased,
	    KeyTyped,
    // -- Mouse Events
	    MouseButtonPressed,
	    MouseButtonReleased,
		MouseButtonDown,
	    MouseMoved,
	    MouseScrolled,
    // -- Editor Events
		ScenePreStart,
		ScenePostStart,
		ScenePreStop,
		ScenePostStop,
	// -- In Editor
		SelectionChanged,
		AssetReload,
	};

	enum EventCategory
	{
	    None = 0,
	    EventCategoryApplication	= BIT(0),
	    EventCategoryInput			= BIT(1),
	    EventCategoryKeyboard		= BIT(2),
	    EventCategoryMouse			= BIT(3),
	    EventCategoryMouseButton	= BIT(4),
		EventCategoryScene			= BIT(5),
        EventCategoryEditor			= BIT(6)
	};

	// -------------------------------------------------------

#define EVENT_CLASS_TYPE(type) static EventType GetStaticType() { return EventType::type; }\
							virtual EventType GetEventType() const override { return GetStaticType(); }\
							virtual const char* GetName() const override { return #type; }

#define EVENT_CLASS_CATEGORY(category) virtual int GetCategoryFlags() const override { return category; }

    // -------------------------------------------------------

	class Event
    {
    public:
        bool Handled = false;
        bool Synced  = false; // Whether the event is synced with the main thread

		virtual ~Event() {}
		virtual EventType GetEventType() const  = 0;
		virtual const char* GetName() const		= 0;
		virtual int GetCategoryFlags() const	= 0;
		virtual std::string ToString() const { return GetName(); }

		inline bool IsInCategory(EventCategory category)
		{
			return GetCategoryFlags() & category;
		}
	};

	// -------------------------------------------------------

	class EventDispatcher
    {
        template <typename T>
        using EventFn = std::function<bool(T &)>;

    public:
        EventDispatcher(Event &event) : Event_(event)
        {
        }

		template <typename T>
        bool Dispatch(EventFn<T> func)
        {
            if (Event_.GetEventType() == T::GetStaticType() && !Event_.Handled)
            {
                Event_.Handled = func(*(T *)&Event_);
                return true;
            }
            return false;
        }

	private:
        Event &Event_;
    };


    inline std::ostream &operator<<(std::ostream &os, const Event &e)
    {
        return os << e.ToString();
    }

} // namespace SceneryEditorX

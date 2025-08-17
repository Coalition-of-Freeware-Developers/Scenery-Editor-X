/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* application_events.h
* -------------------------------------------------------
* Created: 14/7/2025
* -------------------------------------------------------
*/
#pragma once
#include "event_system.h"

/// -------------------------------------------------------------------

namespace SceneryEditorX
{
	class WindowResizeEvent : public Event
	{
	public:
		WindowResizeEvent(unsigned int width, unsigned int height) : m_Width(width), m_Height(height) {}

		inline unsigned int GetWidth() const { return m_Width; }
		inline unsigned int GetHeight() const { return m_Height; }

        [[nodiscard]] virtual std::string ToString() const override
		{
			std::stringstream ss;
			ss << "WindowResizeEvent: " << m_Width << ", " << m_Height;
			return ss.str();
		}

		EVENT_CLASS_TYPE(WindowResize)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	private:
		unsigned int m_Width, m_Height;
	};

    /// -------------------------------------------------------------------

    class WindowMinimizeEvent : public Event
	{
	public:
        explicit WindowMinimizeEvent(const bool minimized) : m_Minimized(minimized) {}

        [[nodiscard]] bool IsMinimized() const { return m_Minimized; }

		EVENT_CLASS_TYPE(WindowMinimize)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	private:
		bool m_Minimized = false;
	};

    /// -------------------------------------------------------------------

	class WindowCloseEvent : public Event
	{
	public:
		WindowCloseEvent() = default;

        EVENT_CLASS_TYPE(WindowClose)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

    /// -------------------------------------------------------------------

	class WindowTitleBarHitTestEvent : public Event
	{
	public:
		WindowTitleBarHitTestEvent(int x, int y, int& hit) : m_X(x), m_Y(y), m_Hit(hit) {}

		inline int GetX() const { return m_X; }
		inline int GetY() const { return m_Y; }
		inline void SetHit(const bool hit) const { m_Hit = (int)hit; }

		EVENT_CLASS_TYPE(WindowTitleBarHitTest)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	private:
		int m_X;
		int m_Y;
		int& m_Hit;
	};

    /// -------------------------------------------------------------------

	class AppTickEvent : public Event
	{
	public:
		AppTickEvent() = default;

        EVENT_CLASS_TYPE(AppTick)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

    /// -------------------------------------------------------------------

	class AppUpdateEvent : public Event
	{
	public:
		AppUpdateEvent() = default;

        EVENT_CLASS_TYPE(AppUpdate)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

    /// -------------------------------------------------------------------

	class AppRenderEvent : public Event
	{
	public:
		AppRenderEvent() = default;

        EVENT_CLASS_TYPE(AppRender)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

    /// -------------------------------------------------------------------

}

/// -------------------------------------------------------------------

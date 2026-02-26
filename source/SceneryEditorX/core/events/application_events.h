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
 * application_events.h
 * -------------------------------------------------------
 * Created: 14/7/2025
 * -------------------------------------------------------
 */
#pragma once
#include "event_system.h"
#include <sstream>

// -------------------------------------------------------------------

namespace SceneryEditorX
{
	class WindowResizeEvent : public Event
	{
	public:
		WindowResizeEvent(const unsigned int width, const unsigned int height) : m_Width(width), m_Height(height) {}

		inline unsigned int GetWidth() const { return m_Width; }
		inline unsigned int GetHeight() const { return m_Height; }

        [[nodiscard]] std::string ToString() const override
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

    // -------------------------------------------------------------------

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

    // -------------------------------------------------------------------

    class WindowMaximizeEvent : public Event
    {
    public:
        explicit WindowMaximizeEvent(const bool maximize) : m_Maximized(maximize) {}

        [[nodiscard]] bool IsMaximized() const { return m_Maximized; }

        EVENT_CLASS_TYPE(WindowMaximize)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)
    private:
        bool m_Maximized = false;
    };

    // -------------------------------------------------------------------

	class WindowCloseEvent : public Event
	{
	public:
		WindowCloseEvent() = default;

        EVENT_CLASS_TYPE(WindowClose)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

    // ------------------------------------------------------------------

    class WindowShowEvent : public Event
    {
    public:
		explicit WindowShowEvent(const bool shown) : m_IsShown(shown) {}

        [[nodiscard]] bool IsShown() const { return m_IsShown; }

        EVENT_CLASS_TYPE(WindowShow)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
    private:
        bool m_IsShown = false;
    };

    // -------------------------------------------------------------------
	
    class WindowHiddenEvent : public Event
    {
    public:
		explicit WindowHiddenEvent(const bool hidden) : m_IsHidden(hidden) {}

        [[nodiscard]] bool IsHidden() const { return m_IsHidden; }

        EVENT_CLASS_TYPE(WindowHide)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)

    private:
        bool m_IsHidden = false;
    };

    // -------------------------------------------------------------------

	class WindowTitleBarHitTestEvent : public Event
	{
	public:
		WindowTitleBarHitTestEvent(const int x, const int y, int& hit) : m_X(x), m_Y(y), m_Hit(hit) {}

		inline int GetX() const { return m_X; }
		inline int GetY() const { return m_Y; }
		inline void SetHit(const bool hit) const { m_Hit = static_cast<int>(hit); }

		EVENT_CLASS_TYPE(WindowTitleBarHitTest)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	private:
		int m_X;
		int m_Y;
		int& m_Hit;
	};

    // -------------------------------------------------------------------

	class AppTickEvent : public Event
	{
	public:
		AppTickEvent() = default;

        EVENT_CLASS_TYPE(AppTick)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

    // -------------------------------------------------------------------

	class AppUpdateEvent : public Event
	{
	public:
		AppUpdateEvent() = default;

        EVENT_CLASS_TYPE(AppUpdate)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

    // -------------------------------------------------------------------

	class AppRenderEvent : public Event
	{
	public:
		AppRenderEvent() = default;

        EVENT_CLASS_TYPE(AppRender)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

    // -------------------------------------------------------------------

	class ScreenshotCapturedEvent : public Event
	{
	public:
		ScreenshotCapturedEvent(std::string path, bool success) : m_Path(std::move(path)), m_Success(success) {}

		[[nodiscard]] const std::string &GetPath() const { return m_Path; }
		[[nodiscard]] bool IsSuccess() const { return m_Success; }

		[[nodiscard]] std::string ToString() const override
		{
			std::stringstream ss;
		    ss << "ScreenshotCapturedEvent: " << (m_Success ? "Success" : "Failure") << ", path='" << m_Path << "'"; 
		    return ss.str();
		}

		EVENT_CLASS_TYPE(ScreenshotCaptured)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	private:
		std::string m_Path;
		bool m_Success = false;
	};

}

// -------------------------------------------------------------------

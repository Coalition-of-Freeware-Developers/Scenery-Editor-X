/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* mouse_events.h
* -------------------------------------------------------
* Created: 14/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/core/events/event_system.h>
#include <SceneryEditorX/core/input/key_codes.h>

/// -------------------------------------------------------------------

namespace SceneryEditorX
{

	class MouseMovedEvent : public Event
	{
	public:
		MouseMovedEvent(float x, float y) : m_MouseX(x), m_MouseY(y) {}

		inline float GetX() const { return m_MouseX; }
		inline float GetY() const { return m_MouseY; }

        [[nodiscard]] virtual std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseMovedEvent: " << m_MouseX << ", " << m_MouseY;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseMoved)
		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
	private:
		float m_MouseX, m_MouseY;
	};

    /// -------------------------------------------------------------------

	class MouseScrolledEvent : public Event
	{
	public:
		MouseScrolledEvent(float xOffset, float yOffset) : m_XOffset(xOffset), m_YOffset(yOffset) {}

		inline float GetXOffset() const { return m_XOffset; }
		inline float GetYOffset() const { return m_YOffset; }

        [[nodiscard]] virtual std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseScrolledEvent: " << GetXOffset() << ", " << GetYOffset();
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseScrolled)
		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
	private:
		float m_XOffset, m_YOffset;
	};

    /// -------------------------------------------------------------------

	class MouseButtonEvent : public Event
	{
	public:
		inline MouseButton GetMouseButton() const { return m_Button; }

		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
	protected:
		MouseButtonEvent(MouseButton button) : m_Button(button) {}

		MouseButton m_Button;
	};

    /// -------------------------------------------------------------------

	class MouseButtonPressedEvent : public MouseButtonEvent
	{
	public:
        explicit MouseButtonPressedEvent(MouseButton button) : MouseButtonEvent(button) {}

        [[nodiscard]] virtual std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseButtonPressedEvent: " << m_Button;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseButtonPressed)
	};

    /// -------------------------------------------------------------------

	class MouseButtonReleasedEvent : public MouseButtonEvent
	{
	public:
        explicit MouseButtonReleasedEvent(MouseButton button) : MouseButtonEvent(button) {}

        [[nodiscard]] virtual std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseButtonReleasedEvent: " << m_Button;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseButtonReleased)
	};

    /// -------------------------------------------------------------------

	class MouseButtonDownEvent : public MouseButtonEvent
	{
	public:
        explicit MouseButtonDownEvent(MouseButton button) : MouseButtonEvent(button) {}

        [[nodiscard]] virtual std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseButtonDownEvent: " << m_Button;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseButtonDown)
	};

    /// -------------------------------------------------------------------

}

/// -------------------------------------------------------------------

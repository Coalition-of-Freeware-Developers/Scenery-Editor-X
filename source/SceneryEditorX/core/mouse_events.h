/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* mouse_events.h
* -------------------------------------------------------
* Created: 2/4/2025
* -------------------------------------------------------
*/

#pragma once

#include <sstream>
#include <SceneryEditorX/core/events.h>
#include <SceneryEditorX/core/key_codes.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class MouseMovedEvent : public Event
	{
	public:
		MouseMovedEvent(float x, float y) : MouseX_(x), MouseY_(y) {}

		inline float GetX() const { return MouseX_; }
		inline float GetY() const { return MouseY_; }


		EVENT_CLASS_TYPE(MouseMoved)
		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
	private:
		float MouseX_, MouseY_;
	};

	// -------------------------------------------------------

	class MouseScrolledEvent : public Event
	{
	public:
		MouseScrolledEvent(float xOffset, float yOffset) : OffsetX_(xOffset), OffsetY_(yOffset) {}

		inline float GetXOffset() const { return OffsetX_; }
		inline float GetYOffset() const { return OffsetY_; }

		EVENT_CLASS_TYPE(MouseScrolled)
		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
	private:
		float OffsetX_, OffsetY_;
	};

	// -------------------------------------------------------

	class MouseButtonEvent : public Event
	{
	public:
		inline MouseButton GetMouseButton() const { return Button_; }

		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
	protected:
		MouseButtonEvent(MouseButton button)
			: Button_(button) {}

		MouseButton Button_;
	};

	// -------------------------------------------------------

	class MouseButtonPressedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonPressedEvent(MouseButton button) : MouseButtonEvent(button) {}

		EVENT_CLASS_TYPE(MouseButtonPressed)
	};

	// -------------------------------------------------------

	class MouseButtonReleasedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonReleasedEvent(MouseButton button) : MouseButtonEvent(button) {}

		EVENT_CLASS_TYPE(MouseButtonReleased)
	};

	// -------------------------------------------------------

	class MouseButtonDownEvent : public MouseButtonEvent
	{
	public:
		MouseButtonDownEvent(MouseButton button) : MouseButtonEvent(button) {}

		EVENT_CLASS_TYPE(MouseButtonDown)
	};

} // namespace SceneryEditorX

// -------------------------------------------------------

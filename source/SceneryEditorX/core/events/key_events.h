/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* key_events.h
* -------------------------------------------------------
* Created: 14/7/2025
* -------------------------------------------------------
*/
#pragma once
#include "event_system.h"
#include "SceneryEditorX/core/input/key_codes.h"

/// -------------------------------------------------------------------

namespace SceneryEditorX
{

    /// -------------------------------------------------------------------
	
	class KeyEvent : public Event
	{
	public:
        [[nodiscard]] KeyCode GetKeyCode() const { return m_KeyCode; }

		EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)
	protected:
        explicit KeyEvent(const KeyCode keycode) : m_KeyCode(keycode) {}

		KeyCode m_KeyCode;
	};

    /// -------------------------------------------------------------------

	class KeyPressedEvent : public KeyEvent
	{
	public:
		KeyPressedEvent(const KeyCode keycode, const int repeatCount) : KeyEvent(keycode), m_RepeatCount(repeatCount) {}

        [[nodiscard]] int GetRepeatCount() const { return m_RepeatCount; }

        [[nodiscard]] virtual std::string ToString() const override
		{
			std::stringstream ss;
			ss << "KeyPressedEvent: " << m_KeyCode << " (" << m_RepeatCount << " repeats)";
			return ss.str();
		}

		EVENT_CLASS_TYPE(KeyPressed)
	private:
		int m_RepeatCount;
	};

    /// -------------------------------------------------------------------

	class KeyReleasedEvent : public KeyEvent
	{
	public:
        explicit KeyReleasedEvent(const KeyCode keycode) : KeyEvent(keycode) {}

        [[nodiscard]] virtual std::string ToString() const override
		{
			std::stringstream ss;
			ss << "KeyReleasedEvent: " << m_KeyCode;
			return ss.str();
		}

		EVENT_CLASS_TYPE(KeyReleased)
	};

    /// -------------------------------------------------------------------

	class KeyTypedEvent : public KeyEvent
	{
	public:
        explicit KeyTypedEvent(const KeyCode keycode) : KeyEvent(keycode) {}

        [[nodiscard]] virtual std::string ToString() const override
		{
			std::stringstream ss;
			ss << "KeyTypedEvent: " << m_KeyCode;
			return ss.str();
		}

		EVENT_CLASS_TYPE(KeyTyped)
	};

}

/// -------------------------------------------------------------------

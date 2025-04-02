/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* key_events.h
* -------------------------------------------------------
* Created: 2/4/2025
* -------------------------------------------------------
*/

#pragma once

#include <SceneryEditorX/core/events.h>
#include <SceneryEditorX/core/key_codes.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class KeyEvent : public Event
	{
	public:
		inline KeyCode GetKeyCode() const { return KeyCode_; }

		EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)
	protected:
		KeyEvent(KeyCode keycode) : KeyCode_(keycode) {}

		KeyCode KeyCode_;
	};

	// -------------------------------------------------------

	class KeyPressedEvent : public KeyEvent
    {
    public:
        KeyPressedEvent(KeyCode keycode, int repeatCount) : KeyEvent(keycode), RepeatCount_(repeatCount)
        {
        }

        inline int GetRepeatCount() const
        {
            return RepeatCount_;
        }

        EVENT_CLASS_TYPE(KeyPressed)
    private:
        int RepeatCount_;
    };

	// -------------------------------------------------------

	class KeyReleasedEvent : public KeyEvent
	{
	public:
		KeyReleasedEvent(KeyCode keycode) : KeyEvent(keycode) {}


		EVENT_CLASS_TYPE(KeyReleased)
	};

	// -------------------------------------------------------

	class KeyTypedEvent : public KeyEvent
	{
	public:
		KeyTypedEvent(KeyCode keycode) : KeyEvent(keycode) {}

		EVENT_CLASS_TYPE(KeyTyped)
	};

} // namespace SceneryEditorX

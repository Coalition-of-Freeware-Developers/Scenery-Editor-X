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
 * input.cpp
 * -------------------------------------------------------
 * Created: 14/7/2025
 * -------------------------------------------------------
 */
#include "input.h"
#include "key_codes.h"
#include <SDL3/SDL.h>
#include <SceneryEditorX/core/window/window.h>

// -------------------------------------------------------

namespace
{
	using SceneryEditorX::KeyCode;
	using SceneryEditorX::KeyMods;
	using SceneryEditorX::KeyState;
	using SceneryEditorX::MouseButton;

	SDL_Scancode KeyCodeToSDLScancode(const KeyCode keycode)
	{
		const int key = static_cast<int>(keycode);

		if (key >= static_cast<int>(KeyCode::A) && key <= static_cast<int>(KeyCode::Z))
		{
			return static_cast<SDL_Scancode>(SDL_SCANCODE_A + (key - static_cast<int>(KeyCode::A)));
		}

		if (key >= static_cast<int>(KeyCode::D1) && key <= static_cast<int>(KeyCode::D9))
		{
			return static_cast<SDL_Scancode>(SDL_SCANCODE_1 + (key - static_cast<int>(KeyCode::D1)));
		}

		if (key == static_cast<int>(KeyCode::D0))
		{
			return SDL_SCANCODE_0;
		}

		switch (keycode)
		{
		case KeyCode::Space: return SDL_SCANCODE_SPACE;
		case KeyCode::Apostrophe: return SDL_SCANCODE_APOSTROPHE;
		case KeyCode::Comma: return SDL_SCANCODE_COMMA;
		case KeyCode::Minus: return SDL_SCANCODE_MINUS;
		case KeyCode::Period: return SDL_SCANCODE_PERIOD;
		case KeyCode::Slash: return SDL_SCANCODE_SLASH;
		case KeyCode::Semicolon: return SDL_SCANCODE_SEMICOLON;
		case KeyCode::Equal: return SDL_SCANCODE_EQUALS;
		case KeyCode::LeftBracket: return SDL_SCANCODE_LEFTBRACKET;
		case KeyCode::Backslash: return SDL_SCANCODE_BACKSLASH;
		case KeyCode::RightBracket: return SDL_SCANCODE_RIGHTBRACKET;
		case KeyCode::GraveAccent: return SDL_SCANCODE_GRAVE;
		case KeyCode::Escape: return SDL_SCANCODE_ESCAPE;
		case KeyCode::Enter: return SDL_SCANCODE_RETURN;
		case KeyCode::Tab: return SDL_SCANCODE_TAB;
		case KeyCode::Backspace: return SDL_SCANCODE_BACKSPACE;
		case KeyCode::Insert: return SDL_SCANCODE_INSERT;
		case KeyCode::Delete: return SDL_SCANCODE_DELETE;
		case KeyCode::Right: return SDL_SCANCODE_RIGHT;
		case KeyCode::Left: return SDL_SCANCODE_LEFT;
		case KeyCode::Down: return SDL_SCANCODE_DOWN;
		case KeyCode::Up: return SDL_SCANCODE_UP;
		case KeyCode::PageUp: return SDL_SCANCODE_PAGEUP;
		case KeyCode::PageDown: return SDL_SCANCODE_PAGEDOWN;
		case KeyCode::Home: return SDL_SCANCODE_HOME;
		case KeyCode::End: return SDL_SCANCODE_END;
		case KeyCode::CapsLock: return SDL_SCANCODE_CAPSLOCK;
		case KeyCode::ScrollLock: return SDL_SCANCODE_SCROLLLOCK;
		case KeyCode::NumLock: return SDL_SCANCODE_NUMLOCKCLEAR;
		case KeyCode::PrintScreen: return SDL_SCANCODE_PRINTSCREEN;
		case KeyCode::Pause: return SDL_SCANCODE_PAUSE;
		case KeyCode::F1: return SDL_SCANCODE_F1;
		case KeyCode::F2: return SDL_SCANCODE_F2;
		case KeyCode::F3: return SDL_SCANCODE_F3;
		case KeyCode::F4: return SDL_SCANCODE_F4;
		case KeyCode::F5: return SDL_SCANCODE_F5;
		case KeyCode::F6: return SDL_SCANCODE_F6;
		case KeyCode::F7: return SDL_SCANCODE_F7;
		case KeyCode::F8: return SDL_SCANCODE_F8;
		case KeyCode::F9: return SDL_SCANCODE_F9;
		case KeyCode::F10: return SDL_SCANCODE_F10;
		case KeyCode::F11: return SDL_SCANCODE_F11;
		case KeyCode::F12: return SDL_SCANCODE_F12;
		case KeyCode::F13: return SDL_SCANCODE_F13;
		case KeyCode::F14: return SDL_SCANCODE_F14;
		case KeyCode::F15: return SDL_SCANCODE_F15;
		case KeyCode::F16: return SDL_SCANCODE_F16;
		case KeyCode::F17: return SDL_SCANCODE_F17;
		case KeyCode::F18: return SDL_SCANCODE_F18;
		case KeyCode::F19: return SDL_SCANCODE_F19;
		case KeyCode::F20: return SDL_SCANCODE_F20;
		case KeyCode::F21: return SDL_SCANCODE_F21;
		case KeyCode::F22: return SDL_SCANCODE_F22;
		case KeyCode::F23: return SDL_SCANCODE_F23;
		case KeyCode::F24: return SDL_SCANCODE_F24;
		case KeyCode::KP0: return SDL_SCANCODE_KP_0;
		case KeyCode::KP1: return SDL_SCANCODE_KP_1;
		case KeyCode::KP2: return SDL_SCANCODE_KP_2;
		case KeyCode::KP3: return SDL_SCANCODE_KP_3;
		case KeyCode::KP4: return SDL_SCANCODE_KP_4;
		case KeyCode::KP5: return SDL_SCANCODE_KP_5;
		case KeyCode::KP6: return SDL_SCANCODE_KP_6;
		case KeyCode::KP7: return SDL_SCANCODE_KP_7;
		case KeyCode::KP8: return SDL_SCANCODE_KP_8;
		case KeyCode::KP9: return SDL_SCANCODE_KP_9;
		case KeyCode::KPDecimal: return SDL_SCANCODE_KP_DECIMAL;
		case KeyCode::KPDivide: return SDL_SCANCODE_KP_DIVIDE;
		case KeyCode::KPMultiply: return SDL_SCANCODE_KP_MULTIPLY;
		case KeyCode::KPSubtract: return SDL_SCANCODE_KP_MINUS;
		case KeyCode::KPAdd: return SDL_SCANCODE_KP_PLUS;
		case KeyCode::KPEnter: return SDL_SCANCODE_KP_ENTER;
		case KeyCode::KPEqual: return SDL_SCANCODE_KP_EQUALS;
		case KeyCode::LeftShift: return SDL_SCANCODE_LSHIFT;
		case KeyCode::LeftControl: return SDL_SCANCODE_LCTRL;
		case KeyCode::LeftAlt: return SDL_SCANCODE_LALT;
		case KeyCode::LeftSuper: return SDL_SCANCODE_LGUI;
		case KeyCode::RightShift: return SDL_SCANCODE_RSHIFT;
		case KeyCode::RightControl: return SDL_SCANCODE_RCTRL;
		case KeyCode::RightAlt: return SDL_SCANCODE_RALT;
		case KeyCode::RightSuper: return SDL_SCANCODE_RGUI;
		case KeyCode::Menu: return SDL_SCANCODE_MENU;
		default: return SDL_SCANCODE_UNKNOWN;
		}
	}

	std::optional<KeyCode> SDLScancodeToKeyCode(const SDL_Scancode scancode)
	{
		if (scancode >= SDL_SCANCODE_A && scancode <= SDL_SCANCODE_Z)
		{
			return static_cast<KeyCode>(static_cast<int>(KeyCode::A) + (scancode - SDL_SCANCODE_A));
		}

		if (scancode >= SDL_SCANCODE_1 && scancode <= SDL_SCANCODE_9)
		{
			return static_cast<KeyCode>(static_cast<int>(KeyCode::D1) + (scancode - SDL_SCANCODE_1));
		}

		if (scancode == SDL_SCANCODE_0)
		{
			return KeyCode::D0;
		}

		switch (scancode)
		{
		case SDL_SCANCODE_SPACE: return KeyCode::Space;
		case SDL_SCANCODE_APOSTROPHE: return KeyCode::Apostrophe;
		case SDL_SCANCODE_COMMA: return KeyCode::Comma;
		case SDL_SCANCODE_MINUS: return KeyCode::Minus;
		case SDL_SCANCODE_PERIOD: return KeyCode::Period;
		case SDL_SCANCODE_SLASH: return KeyCode::Slash;
		case SDL_SCANCODE_SEMICOLON: return KeyCode::Semicolon;
		case SDL_SCANCODE_EQUALS: return KeyCode::Equal;
		case SDL_SCANCODE_LEFTBRACKET: return KeyCode::LeftBracket;
		case SDL_SCANCODE_BACKSLASH: return KeyCode::Backslash;
		case SDL_SCANCODE_RIGHTBRACKET: return KeyCode::RightBracket;
		case SDL_SCANCODE_GRAVE: return KeyCode::GraveAccent;
		case SDL_SCANCODE_ESCAPE: return KeyCode::Escape;
		case SDL_SCANCODE_RETURN: return KeyCode::Enter;
		case SDL_SCANCODE_TAB: return KeyCode::Tab;
		case SDL_SCANCODE_BACKSPACE: return KeyCode::Backspace;
		case SDL_SCANCODE_INSERT: return KeyCode::Insert;
		case SDL_SCANCODE_DELETE: return KeyCode::Delete;
		case SDL_SCANCODE_RIGHT: return KeyCode::Right;
		case SDL_SCANCODE_LEFT: return KeyCode::Left;
		case SDL_SCANCODE_DOWN: return KeyCode::Down;
		case SDL_SCANCODE_UP: return KeyCode::Up;
		case SDL_SCANCODE_PAGEUP: return KeyCode::PageUp;
		case SDL_SCANCODE_PAGEDOWN: return KeyCode::PageDown;
		case SDL_SCANCODE_HOME: return KeyCode::Home;
		case SDL_SCANCODE_END: return KeyCode::End;
		case SDL_SCANCODE_CAPSLOCK: return KeyCode::CapsLock;
		case SDL_SCANCODE_SCROLLLOCK: return KeyCode::ScrollLock;
		case SDL_SCANCODE_NUMLOCKCLEAR: return KeyCode::NumLock;
		case SDL_SCANCODE_PRINTSCREEN: return KeyCode::PrintScreen;
		case SDL_SCANCODE_PAUSE: return KeyCode::Pause;
		case SDL_SCANCODE_F1: return KeyCode::F1;
		case SDL_SCANCODE_F2: return KeyCode::F2;
		case SDL_SCANCODE_F3: return KeyCode::F3;
		case SDL_SCANCODE_F4: return KeyCode::F4;
		case SDL_SCANCODE_F5: return KeyCode::F5;
		case SDL_SCANCODE_F6: return KeyCode::F6;
		case SDL_SCANCODE_F7: return KeyCode::F7;
		case SDL_SCANCODE_F8: return KeyCode::F8;
		case SDL_SCANCODE_F9: return KeyCode::F9;
		case SDL_SCANCODE_F10: return KeyCode::F10;
		case SDL_SCANCODE_F11: return KeyCode::F11;
		case SDL_SCANCODE_F12: return KeyCode::F12;
		case SDL_SCANCODE_F13: return KeyCode::F13;
		case SDL_SCANCODE_F14: return KeyCode::F14;
		case SDL_SCANCODE_F15: return KeyCode::F15;
		case SDL_SCANCODE_F16: return KeyCode::F16;
		case SDL_SCANCODE_F17: return KeyCode::F17;
		case SDL_SCANCODE_F18: return KeyCode::F18;
		case SDL_SCANCODE_F19: return KeyCode::F19;
		case SDL_SCANCODE_F20: return KeyCode::F20;
		case SDL_SCANCODE_F21: return KeyCode::F21;
		case SDL_SCANCODE_F22: return KeyCode::F22;
		case SDL_SCANCODE_F23: return KeyCode::F23;
		case SDL_SCANCODE_F24: return KeyCode::F24;
		case SDL_SCANCODE_KP_0: return KeyCode::KP0;
		case SDL_SCANCODE_KP_1: return KeyCode::KP1;
		case SDL_SCANCODE_KP_2: return KeyCode::KP2;
		case SDL_SCANCODE_KP_3: return KeyCode::KP3;
		case SDL_SCANCODE_KP_4: return KeyCode::KP4;
		case SDL_SCANCODE_KP_5: return KeyCode::KP5;
		case SDL_SCANCODE_KP_6: return KeyCode::KP6;
		case SDL_SCANCODE_KP_7: return KeyCode::KP7;
		case SDL_SCANCODE_KP_8: return KeyCode::KP8;
		case SDL_SCANCODE_KP_9: return KeyCode::KP9;
		case SDL_SCANCODE_KP_DECIMAL: return KeyCode::KPDecimal;
		case SDL_SCANCODE_KP_DIVIDE: return KeyCode::KPDivide;
		case SDL_SCANCODE_KP_MULTIPLY: return KeyCode::KPMultiply;
		case SDL_SCANCODE_KP_MINUS: return KeyCode::KPSubtract;
		case SDL_SCANCODE_KP_PLUS: return KeyCode::KPAdd;
		case SDL_SCANCODE_KP_ENTER: return KeyCode::KPEnter;
		case SDL_SCANCODE_KP_EQUALS: return KeyCode::KPEqual;
		case SDL_SCANCODE_LSHIFT: return KeyCode::LeftShift;
		case SDL_SCANCODE_LCTRL: return KeyCode::LeftControl;
		case SDL_SCANCODE_LALT: return KeyCode::LeftAlt;
		case SDL_SCANCODE_LGUI: return KeyCode::LeftSuper;
		case SDL_SCANCODE_RSHIFT: return KeyCode::RightShift;
		case SDL_SCANCODE_RCTRL: return KeyCode::RightControl;
		case SDL_SCANCODE_RALT: return KeyCode::RightAlt;
		case SDL_SCANCODE_RGUI: return KeyCode::RightSuper;
		case SDL_SCANCODE_MENU: return KeyCode::Menu;
		default: return std::nullopt;
		}
	}

	std::optional<KeyCode> CharacterToKeyCode(const char c)
	{
		if (c >= 'a' && c <= 'z')
		{
			return static_cast<KeyCode>(static_cast<int>(KeyCode::A) + (c - 'a'));
		}

		if (c >= 'A' && c <= 'Z')
		{
			return static_cast<KeyCode>(static_cast<int>(KeyCode::A) + (c - 'A'));
		}

		if (c >= '1' && c <= '9')
		{
			return static_cast<KeyCode>(static_cast<int>(KeyCode::D1) + (c - '1'));
		}

		if (c == '0')
		{
			return KeyCode::D0;
		}

		switch (c)
		{
		case ' ': return KeyCode::Space;
		case '`': return KeyCode::GraveAccent;
		case '-': return KeyCode::Minus;
		case '=': return KeyCode::Equal;
		case '[': return KeyCode::LeftBracket;
		case ']': return KeyCode::RightBracket;
		case '\\': return KeyCode::Backslash;
		case ';': return KeyCode::Semicolon;
		case '\'': return KeyCode::Apostrophe;
		case ',': return KeyCode::Comma;
		case '.': return KeyCode::Period;
		case '/': return KeyCode::Slash;
		default: return std::nullopt;
		}
	}

	std::optional<MouseButton> SDLMouseButtonToMouseButton(const uint8_t button)
	{
		switch (button)
		{
		case SDL_BUTTON_LEFT: return MouseButton::Left;
		case SDL_BUTTON_RIGHT: return MouseButton::Right;
		case SDL_BUTTON_MIDDLE: return MouseButton::Middle;
		case SDL_BUTTON_X1: return MouseButton::Button3;
		case SDL_BUTTON_X2: return MouseButton::Button4;
		default: return std::nullopt;
		}
	}

	std::optional<KeyMods> KeyModsFromSDLKeymod(const SDL_Keymod mods)
	{
		if (mods == 0)
		{
			return std::nullopt;
		}

		KeyMods result = KeyMods::None;
		if (mods & SDL_KMOD_SHIFT) result |= KeyMods::Shift;
		if (mods & SDL_KMOD_CTRL) result |= KeyMods::Ctrl;
		if (mods & SDL_KMOD_ALT) result |= KeyMods::Alt;
		if (mods & SDL_KMOD_GUI) result |= KeyMods::Super;
		if (mods & SDL_KMOD_CAPS) result |= KeyMods::CapsLock;
		if (mods & SDL_KMOD_NUM) result |= KeyMods::NumLock;
		return result;
	}
}

namespace SceneryEditorX
{

	void Input::Tick()
	{
		/*
		// Cleanup disconnected controller
		for (auto it = s_Controllers.begin(); it != s_Controllers.end(); )
		{
			int id = it->first;
			if (glfwJoystickPresent(id) != GLFW_TRUE)
				it = s_Controllers.erase(it);
			else
				it++;
		}

		// Tick controllers
		for (int id = GLFW_JOYSTICK_1; id < GLFW_JOYSTICK_LAST; id++)
		{
			if (glfwJoystickPresent(id) == GLFW_TRUE)
			{
				Controller& controller = s_Controllers[id];
				controller.ID = id;
				controller.Name = glfwGetJoystickName(id);

				int buttonCount;
				const unsigned char* buttons = glfwGetJoystickButtons(id, &buttonCount);
				for (int i = 0; i < buttonCount; i++)
				{
					if(buttons[i] == GLFW_PRESS && !controller.ButtonDown[i])
						controller.ButtonStates[i].State = KeyState::Pressed;
					else if(buttons[i] == GLFW_RELEASE && controller.ButtonDown[i])
						controller.ButtonStates[i].State = KeyState::Released;

					controller.ButtonDown[i] = buttons[i] == GLFW_PRESS;
				}

				int axisCount;
				const float* axes = glfwGetJoystickAxes(id, &axisCount);
				for (int i = 0; i < axisCount; i++)
					controller.AxisStates[i] = abs(axes[i]) > controller.DeadZones[i] ? axes[i] : 0.0f;

				int hatCount;
				const unsigned char* hats = glfwGetJoystickHats(id, &hatCount);
				for (int i = 0; i < hatCount; i++)
					controller.HatStates[i] = hats[i];
			}
		}
		*/
	}

	void Input::OnKeyEvent(const SDL_KeyboardEvent &event)
	{
		UpdateKeyMods(KeyModsFromSDLKeymod(event.mod));

		const auto keyCode = SDLScancodeToKeyCode(event.scancode);
		if (!keyCode)
		{
			return;
		}

		if (event.type == SDL_EVENT_KEY_DOWN)
		{
			UpdateKeyState(*keyCode, event.repeat ? KeyState::Held : KeyState::Pressed);
		}
		else
		{
			UpdateKeyState(*keyCode, KeyState::Released);
		}
	}

	void Input::OnMouseButtonEvent(const SDL_MouseButtonEvent &event)
	{
		const auto button = SDLMouseButtonToMouseButton(event.button);
		if (!button)
		{
			return;
		}

		UpdateButtonState(*button, event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ? KeyState::Pressed : KeyState::Released);
	}

	std::optional<KeyCode> Input::FromSDLScancode(const SDL_Scancode scancode)
	{
		return SDLScancodeToKeyCode(scancode);
	}

	std::optional<MouseButton> Input::FromSDLMouseButton(const uint8_t button)
	{
		return SDLMouseButtonToMouseButton(button);
	}

	std::optional<KeyCode> Input::FromCharacter(const char c)
	{
		return CharacterToKeyCode(c);
	}

	bool Input::IsKeyPressed(const KeyCode key)
	{
		return s_KeyData.contains(key) && s_KeyData[key].State == KeyState::Pressed;
	}

	bool Input::IsKeyHeld(const KeyCode key)
	{
		return s_KeyData.contains(key) && s_KeyData[key].State == KeyState::Held;
	}

	bool Input::IsKeyDown(KeyCode keycode)
	{
		const SDL_Scancode scancode = KeyCodeToSDLScancode(keycode);
		if (scancode == SDL_SCANCODE_UNKNOWN)
		{
			return false;
		}

		const bool *state = SDL_GetKeyboardState(nullptr);
		return state != nullptr && state[scancode];
	}

	bool Input::IsKeyReleased(const KeyCode keycode)
	{
		return s_KeyData.contains(keycode) && s_KeyData[keycode].State == KeyState::Released;
	}

	bool Input::IsKeyToggledOn(const KeyCode keycode)
	{

		if (keycode == KeyCode::CapsLock)
		{
			return (s_Mods & KeyMods::CapsLock) != KeyMods::None;
		}

		if (keycode == KeyCode::NumLock)
		{
			return (s_Mods & KeyMods::NumLock) != KeyMods::None;
		}


		// SDL3 has support for ScrollLock, but using modifier flags is the preferred approach
		/*
		if (keycode == KeyCode::ScrollLock)
		{
			return (s_Mods & KeyMods::ScrollLock) != KeyMods::None;
		}
		*/

		return false;
	}

	bool Input::IsMouseButtonPressed(MouseButton button)
	{
		return s_MouseData.contains(button) && s_MouseData[button].State == KeyState::Pressed;
	}

	bool Input::IsMouseButtonHeld(MouseButton button)
	{
		return s_MouseData.contains(button) && s_MouseData[button].State == KeyState::Held;
	}

	bool Input::IsMouseButtonDown(MouseButton button)
	{
		// SDL3 mouse button state
		const Uint32 mouseState = SDL_GetMouseState(nullptr, nullptr);

		// Map MouseButton enum to SDL button masks
		switch (button)
		{
			case MouseButton::Left: return (mouseState & SDL_BUTTON_LMASK) != 0;
			case MouseButton::Right: return (mouseState & SDL_BUTTON_RMASK) != 0;
			case MouseButton::Middle: return (mouseState & SDL_BUTTON_MMASK) != 0;
			case MouseButton::Button3: return (mouseState & SDL_BUTTON_X1MASK) != 0;
			case MouseButton::Button4: return (mouseState & SDL_BUTTON_X2MASK) != 0;
			default: return false;
		}
	}

	bool Input::IsMouseButtonReleased(const MouseButton button)
	{
		return s_MouseData.contains(button) && s_MouseData[button].State == KeyState::Released;
	}

	float Input::GetMouseX()
	{
		auto [x, y] = GetMousePosition();
		return x;
	}

	float Input::GetMouseY()
	{
		auto [x, y] = GetMousePosition();
		return y;
	}

	std::pair<float, float> Input::GetMousePosition()
	{
		float x, y;
		SDL_GetMouseState(&x, &y);
		return { x, y };
	}

	// TODO: A better way to do this is to handle it internally, and simply move the cursor the opposite side of the screen when it reaches the edge
	void Input::SetCursorMode(CursorMode mode)
	{
		SDL_Window* window = Window::GetWindow();
		if (!window)
		{
			SEDX_CORE_ERROR_TAG("INPUT", "Cannot set cursor mode: Window is not initialized");
			return;
		}

		switch (mode)
		{
			case CursorMode::Normal:
				SDL_SetWindowRelativeMouseMode(window, false);
				SDL_ShowCursor();
				break;
			case CursorMode::Hidden:
				SDL_SetWindowRelativeMouseMode(window, false);
				SDL_HideCursor();
				break;
			case CursorMode::Locked:
				SDL_SetWindowRelativeMouseMode(window, true);
				break;
		}
	}

	CursorMode Input::GetCursorMode()
	{
		SDL_Window* window = Window::GetWindow();
		if (!window)
		{
			SEDX_CORE_WARN_TAG("INPUT", "Cannot get cursor mode: Window is not initialized");
			return CursorMode::Normal;
		}

		// Check if cursor is visible and if relative mouse mode is enabled
		const bool cursorVisible = SDL_CursorVisible();
		if (SDL_GetWindowRelativeMouseMode(window))
		{
			return CursorMode::Locked;
		}
		else if (!cursorVisible)
		{
			return CursorMode::Hidden;
		}
		else
		{
			return CursorMode::Normal;
		}
	}

	void Input::TransitionPressedKeys()
	{
		for (const auto& [key, keyData] : s_KeyData)
		{
			if (keyData.State == KeyState::Pressed)
			{
				UpdateKeyState(key, KeyState::Held);
			}
		}
	}

	void Input::TransitionPressedButtons()
	{
		for (const auto& [button, buttonData] : s_MouseData)
		{
			if (buttonData.State == KeyState::Pressed)
			{
				UpdateButtonState(button, KeyState::Held);
			}
		}
	}

	void Input::UpdateKeyState(const KeyCode key, const KeyState newState)
	{
		auto&[Key, State, OldState] = s_KeyData[key];
		Key = key;
		OldState = State;
		State = newState;
	}

	void Input::UpdateKeyMods(const std::optional<KeyMods> mods)
	{
		if (mods.has_value())
		{
			s_Mods = mods.value();
		}
		else
		{
			s_Mods = KeyMods::None;
		}
	}

	void Input::UpdateButtonState(const MouseButton button, const KeyState newState)
	{
		auto& mouseData = s_MouseData[button];
		mouseData.Button = button;
		mouseData.OldState = mouseData.State;
		mouseData.State = newState;
	}

	void Input::ClearReleasedKeys()
	{
		for (const auto& [key, keyData] : s_KeyData)
		{
			if (keyData.State == KeyState::Released)
			{
				UpdateKeyState(key, KeyState::None);
			}
		}

		for (const auto& [button, buttonData] : s_MouseData)
		{
			if (buttonData.State == KeyState::Released)
			{
				UpdateButtonState(button, KeyState::None);
			}
		}
	}

}

// -------------------------------------------------------

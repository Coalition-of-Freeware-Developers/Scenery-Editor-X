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
 * input.h
 * -------------------------------------------------------
 * Created: 14/7/2025
 * -------------------------------------------------------
 */
#pragma once
#include "key_codes.h"
#include <map>
#include <optional>
#include <utility>
#include <SDL3/SDL.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

	enum class KeyMods : int  // NOLINT(performance-enum-size)
	{
		None	 = 0,
		Shift	 = BIT(0),
		Ctrl	 = BIT(1),
		Alt		 = BIT(2),
		Super	 = BIT(3),
		CapsLock = BIT(4),
		NumLock  = BIT(5)
	};

	// Bitwise operators for KeyMods enum
	inline KeyMods operator&(KeyMods lhs, KeyMods rhs)
	{
		return static_cast<KeyMods>(static_cast<int>(lhs) & static_cast<int>(rhs));
	}

	inline KeyMods operator|(KeyMods lhs, KeyMods rhs)
	{
		return static_cast<KeyMods>(static_cast<int>(lhs) | static_cast<int>(rhs));
	}

	inline KeyMods operator^(KeyMods lhs, KeyMods rhs)
	{
		return static_cast<KeyMods>(static_cast<int>(lhs) ^ static_cast<int>(rhs));
	}

	inline KeyMods operator~(KeyMods rhs)
	{
		return static_cast<KeyMods>(~static_cast<int>(rhs));
	}

	inline KeyMods& operator&=(KeyMods& lhs, KeyMods rhs)
	{
		lhs = lhs & rhs;
		return lhs;
	}

	inline KeyMods& operator|=(KeyMods& lhs, KeyMods rhs)
	{
		lhs = lhs | rhs;
		return lhs;
	}

	inline KeyMods& operator^=(KeyMods& lhs, KeyMods rhs)
	{
		lhs = lhs ^ rhs;
		return lhs;
	}

	struct KeyData
	{
		KeyCode Key;
		KeyState State = KeyState::None;
		KeyState OldState = KeyState::None;
	};

	struct ButtonData
	{
		MouseButton Button;
		KeyState State = KeyState::None;
		KeyState OldState = KeyState::None;
	};

	// -------------------------------------------------------

	class Input
	{
	public:
		static void Tick();
		static void OnKeyEvent(const SDL_KeyboardEvent &event);
		static void OnMouseButtonEvent(const SDL_MouseButtonEvent &event);
		static std::optional<KeyCode> FromSDLScancode(SDL_Scancode scancode);
		static std::optional<MouseButton> FromSDLMouseButton(uint8_t button);
		static std::optional<KeyCode> FromCharacter(char c);

		static bool IsKeyPressed(KeyCode keycode);
		static bool IsKeyHeld(KeyCode keycode);
		static bool IsKeyDown(KeyCode keycode);
		static bool IsKeyReleased(KeyCode keycode);
		static bool IsKeyToggledOn(KeyCode keycode);

		static bool IsMouseButtonPressed(MouseButton button);
		static bool IsMouseButtonHeld(MouseButton button);
		static bool IsMouseButtonDown(MouseButton button);
		static bool IsMouseButtonReleased(MouseButton button);
		static float GetMouseX();
		static float GetMouseY();
		static std::pair<float, float> GetMousePosition();

		static void SetCursorMode(CursorMode mode);
		static CursorMode GetCursorMode();

		// Internal use only...
		static void TransitionPressedKeys();
		static void TransitionPressedButtons();
		static void UpdateKeyState(KeyCode key, KeyState newState);
		static void UpdateKeyMods(std::optional<KeyMods> mods);
		static void UpdateButtonState(MouseButton button, KeyState newState);
		static void ClearReleasedKeys();

	private:
		inline static std::map<KeyCode, KeyData> s_KeyData;
		inline static std::map<MouseButton, ButtonData> s_MouseData;
		inline static KeyMods s_Mods = KeyMods::None;
	};

}

// -------------------------------------------------------

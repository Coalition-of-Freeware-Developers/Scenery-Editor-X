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

	/**
	 * @enum KeyMods
	 * @brief Represents modifier keys (Shift, Ctrl, Alt, etc.) as bit flags for efficient combination and checking.
	 */
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

	/**
	 * @brief Bitwise AND operator for KeyMods enum, allowing combination of modifier keys using bitwise operations.
	 * @param lhs The left-hand side operand.
	 * @param rhs The right-hand side operand.
	 * @return The result of the bitwise AND operation on the two KeyMods values.
	 */
	inline KeyMods operator&(KeyMods lhs, KeyMods rhs)
	{
		return static_cast<KeyMods>(static_cast<int>(lhs) & static_cast<int>(rhs));
	}

	/**
	 * @brief Bitwise OR operator for KeyMods enum, allowing combination of modifier keys using bitwise operations.
	 * @param lhs The left-hand side operand.
	 * @param rhs The right-hand side operand.
	 * @return The result of the bitwise OR operation on the two KeyMods values.
	 */
	inline KeyMods operator|(KeyMods lhs, KeyMods rhs)
	{
		return static_cast<KeyMods>(static_cast<int>(lhs) | static_cast<int>(rhs));
	}

	/**
	 * @brief Bitwise XOR operator for KeyMods enum, allowing combination of modifier keys using bitwise operations.
	 * @param lhs The left-hand side operand.
	 * @param rhs The right-hand side operand.
	 * @return The result of the bitwise XOR operation on the two KeyMods values.
	 */
	inline KeyMods operator^(KeyMods lhs, KeyMods rhs)
	{
		return static_cast<KeyMods>(static_cast<int>(lhs) ^ static_cast<int>(rhs));
	}

	/**
	 * @brief Bitwise NOT operator for KeyMods enum, allowing inversion of modifier keys using bitwise operations.
	 * @param rhs The operand to be inverted.
	 * @return The result of the bitwise NOT operation on the KeyMods value.
	 */
	inline KeyMods operator~(KeyMods rhs)
	{
		return static_cast<KeyMods>(~static_cast<int>(rhs));
	}

	/**
	 * @brief Bitwise AND assignment operator for KeyMods enum, allowing combination of modifier keys using bitwise operations.
	 * @param lhs The left-hand side operand.
	 * @param rhs The right-hand side operand.
	 * @return The result of the bitwise AND assignment operation on the two KeyMods values.
	 */
	inline KeyMods& operator&=(KeyMods& lhs, KeyMods rhs)
	{
		lhs = lhs & rhs;
		return lhs;
	}

	/**
	 * @brief Bitwise OR assignment operator for KeyMods enum, allowing combination of modifier keys using bitwise operations.
	 * @param lhs The left-hand side operand.
	 * @param rhs The right-hand side operand.
	 * @return The result of the bitwise OR assignment operation on the two KeyMods values.
	 */
	inline KeyMods& operator|=(KeyMods& lhs, KeyMods rhs)
	{
		lhs = lhs | rhs;
		return lhs;
	}

	/**
	 * @brief Bitwise XOR assignment operator for KeyMods enum, allowing combination of modifier keys using bitwise operations.
	 * @param lhs The left-hand side operand.
	 * @param rhs The right-hand side operand.
	 * @return The result of the bitwise XOR assignment operation on the two KeyMods values.
	 */
	inline KeyMods& operator^=(KeyMods& lhs, KeyMods rhs)
	{
		lhs = lhs ^ rhs;
		return lhs;
	}

	/**
	 * @struct KeyData
	 * @brief Structure representing key data, including the key code and its current and previous states.
	 */
	struct KeyData
	{
		KeyCode key;
		KeyState state = KeyState::None;
		KeyState oldState = KeyState::None;
	};

	/**
	 * @struct ButtonData
	 * @brief Structure representing button data, including the button code and its current and previous states.
	 */
	struct ButtonData
	{
		MouseButton button;
		KeyState state = KeyState::None;
		KeyState oldState = KeyState::None;
	};

	// -------------------------------------------------------

	/**
	 * @class Input
	 * @brief Static class responsible for handling input events, tracking key and mouse button states, 
	 * and providing an interface for querying input states in the editor application. 
	 */
	class Input
	{
	public:
		/* @brief Updates the input states for all keys and mouse buttons. This should be called once per frame. */
		static void Tick();

		/**
		 * @brief Handles a key event from SDL, updating the internal state of the corresponding key.
		 * @param event The SDL_KeyboardEvent to be processed.
		 */
		static void OnKeyEvent(const SDL_KeyboardEvent &event);

		/**
		 * @brief Handles a mouse button event from SDL, updating the internal state of the corresponding mouse button.
		 * @param event The SDL_MouseButtonEvent to be processed.
		 */
		static void OnMouseButtonEvent(const SDL_MouseButtonEvent &event);

		/**
		 * @brief Converts an SDL_Scancode value to its corresponding KeyCode value, allowing for mapping between SDL's scancodes and the engine's key codes for input handling.
		 * @param scancode The SDL_Scancode value to be converted.
		 * @return The corresponding KeyCode value, or std::nullopt if the scancode does not have a corresponding KeyCode.
		 */
		static std::optional<KeyCode> FromSDLScancode(SDL_Scancode scancode);

		/**
		 * @brief Converts an SDL mouse button value to its corresponding MouseButton value, allowing for mapping between SDL's mouse buttons and the engine's mouse buttons for input handling.
		 * @param button The SDL mouse button value to be converted.
		 * @return The corresponding MouseButton value, or std::nullopt if the button does not have a corresponding MouseButton.
		 */
		static std::optional<MouseButton> FromSDLMouseButton(uint8_t button);

		/**
		 * @brief Converts a character to its corresponding KeyCode value, allowing for mapping between characters and the engine's key codes for input handling.
		 * @param c The character to be converted.
		 * @return The corresponding KeyCode value, or std::nullopt if the character does not have a corresponding KeyCode.
		 */
		static std::optional<KeyCode> FromCharacter(char c);

		/**
		 * @brief Checks if a key is currently pressed.
		 * @param keycode The KeyCode value of the key to be checked.
		 * @return True if the key is pressed, false otherwise.
		 */
		static bool IsKeyPressed(KeyCode keycode);

		/**
		 * @brief Checks if a key is currently held down.
		 * @param keycode The KeyCode value of the key to be checked.
		 * @return True if the key is held down, false otherwise.
		 */
		static bool IsKeyHeld(KeyCode keycode);

		/**
		 * @brief Checks if a key was pressed during the current frame.
		 * @param keycode The KeyCode value of the key to be checked.
		 * @return True if the key was pressed during the current frame, false otherwise.
		 */
		static bool IsKeyDown(KeyCode keycode);

		/**
		 * @brief Checks if a key was released during the current frame.
		 * @param keycode The KeyCode value of the key to be checked.
		 * @return True if the key was released during the current frame, false otherwise.
		 */
		static bool IsKeyReleased(KeyCode keycode);

		/**
		 * @brief Checks if a key is toggled on (e.g., Caps Lock, Num Lock).
		 * @param keycode The KeyCode value of the key to be checked.
		 * @return True if the key is toggled on, false otherwise.
		 */
		static bool IsKeyToggledOn(KeyCode keycode);

		/**
		 * @brief Checks if a mouse button is currently pressed.
		 * @param button The MouseButton value of the button to be checked.
		 * @return True if the button is pressed, false otherwise.
		 */
		static bool IsMouseButtonPressed(MouseButton button);

		/**
		 * @brief Checks if a mouse button is currently held down.
		 * @param button The MouseButton value of the button to be checked.
		 * @return True if the button is held down, false otherwise.
		 */
		static bool IsMouseButtonHeld(MouseButton button);

		/**
		 * @brief Checks if a mouse button was pressed during the current frame.
		 * @param button The MouseButton value of the button to be checked.
		 * @return True if the button was pressed during the current frame, false otherwise.
		 */
		static bool IsMouseButtonDown(MouseButton button);

		/**
		 * @brief Checks if a mouse button was released during the current frame.
		 * @param button The MouseButton value of the button to be checked.
		 * @return True if the button was released during the current frame, false otherwise.
		 */
		static bool IsMouseButtonReleased(MouseButton button);

		/**
		 * @brief Gets the current X position of the mouse cursor.
		 * @return The X position of the mouse cursor.
		 */
		static float GetMouseX();

		/**
		 * @brief Gets the current Y position of the mouse cursor.
		 * @return The Y position of the mouse cursor.
		 */
		static float GetMouseY();

		/**
		 * @brief Gets the current position of the mouse cursor.
		 * @return A pair containing the X and Y positions of the mouse cursor.
		 */
		static std::pair<float, float> GetMousePosition();

		/**
		 * @brief Sets the cursor mode.
		 * @param mode The cursor mode to be set.
		 */
		static void SetCursorMode(CursorMode mode);

		/**
		 * @brief Gets the current cursor mode.
		 * @return The current cursor mode.
		 */
		static CursorMode GetCursorMode();

		/* @brief Transitions the state of pressed keys to the next frame. */
		static void TransitionPressedKeys();

		/* @brief Transitions the state of pressed mouse buttons to the next frame. */
		static void TransitionPressedButtons();

		/**
		 * @brief Updates the state of a key.
		 * @param key The KeyCode value of the key to be updated.
		 * @param newState The new state of the key.
		 */
		static void UpdateKeyState(KeyCode key, KeyState newState);

		/**
		 * @brief Updates the state of modifier keys.
		 * @param mods The new state of the modifier keys.
		 */
		static void UpdateKeyMods(std::optional<KeyMods> mods);

		/**
		 * @brief Updates the state of a mouse button.
		 * @param button The MouseButton value of the button to be updated.
		 * @param newState The new state of the button.
		 */
		static void UpdateButtonState(MouseButton button, KeyState newState);

		/* @brief Clears the state of released keys. */
		static void ClearReleasedKeys();

	private:
		inline static std::map<KeyCode, KeyData> s_KeyData; // Maps to store the state of each key and mouse button
		inline static std::map<MouseButton, ButtonData> s_MouseData; // Maps to store the state of each key and mouse button
		inline static KeyMods s_Mods = KeyMods::None; // Current state of modifier keys (Shift, Ctrl, Alt, etc.)
	};

}

// -------------------------------------------------------

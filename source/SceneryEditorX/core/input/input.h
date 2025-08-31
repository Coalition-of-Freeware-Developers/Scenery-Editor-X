/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* input.h
* -------------------------------------------------------
* Created: 14/7/2025
* -------------------------------------------------------
*/
#pragma once
#include "key_codes.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{

    /// -------------------------------------------------------

	enum class KeyMods : int  // NOLINT(performance-enum-size)
	{
		None = 0,
		Shift = BIT(0),
		Ctrl = BIT(1),
		Alt = BIT(2),
		Super = BIT(3),
		CapsLock = BIT(4),
		NumLock = BIT(5)
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

    /// -------------------------------------------------------

	class Input
	{
	public:
		static void Update();

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

		/// Internal use only...
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

/// -------------------------------------------------------

/*
template <>
struct magic_enum::customize::enum_range<SceneryEditorX::KeyMods>
{
    static constexpr bool is_flags = true;
};
*/

/// -------------------------------------------------------

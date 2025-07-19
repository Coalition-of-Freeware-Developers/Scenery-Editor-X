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
#include <SceneryEditorX/core/input/key_codes.h>

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
		GLOBAL void Update();

		LOCAL bool IsKeyPressed(KeyCode keycode);
		LOCAL bool IsKeyHeld(KeyCode keycode);
		LOCAL bool IsKeyDown(KeyCode keycode);
		LOCAL bool IsKeyReleased(KeyCode keycode);
		LOCAL bool IsKeyToggledOn(KeyCode keycode);

		LOCAL bool IsMouseButtonPressed(MouseButton button);
		LOCAL bool IsMouseButtonHeld(MouseButton button);
		LOCAL bool IsMouseButtonDown(MouseButton button);
		LOCAL bool IsMouseButtonReleased(MouseButton button);
		GLOBAL float GetMouseX();
		GLOBAL float GetMouseY();
		GLOBAL std::pair<float, float> GetMousePosition();

		LOCAL void SetCursorMode(CursorMode mode);
		INTERNAL CursorMode GetCursorMode();

		// Internal use only...
		INTERNAL void TransitionPressedKeys();
		INTERNAL void TransitionPressedButtons();
		INTERNAL void UpdateKeyState(KeyCode key, KeyState newState);
		INTERNAL void UpdateKeyMods(std::optional<KeyMods> mods);
		INTERNAL void UpdateButtonState(MouseButton button, KeyState newState);
		INTERNAL void ClearReleasedKeys();
	private:
		inline LOCAL std::map<KeyCode, KeyData> s_KeyData;
		inline LOCAL std::map<MouseButton, ButtonData> s_MouseData;
		inline LOCAL KeyMods s_Mods = KeyMods::None;
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

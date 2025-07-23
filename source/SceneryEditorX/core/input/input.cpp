/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* input.cpp
* -------------------------------------------------------
* Created: 14/7/2025
* -------------------------------------------------------
*/
#include <GLFW/glfw3.h>
#include <imgui/imgui_internal.h>
#include <SceneryEditorX/core/application/application.h>
#include "input.h"
#include "key_codes.h"
//#include <magic_enum.hpp>

/// -------------------------------------------------------

namespace SceneryEditorX
{

	void Input::Update()
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

		// Update controllers
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
        if (const bool enableImGui = Application::Get().GetAppData().EnableImGui; !enableImGui)
		{
			auto &window = WindowData::window;
			auto state = glfwGetKey(static_cast<GLFWwindow*>(window), static_cast<int32_t>(keycode));
			return state == GLFW_PRESS || state == GLFW_REPEAT;
		}

		ImGuiContext* context = ImGui::GetCurrentContext();
		bool pressed = false;
		for (const ImGuiViewport* viewport : context->Viewports)
		{
			if (!viewport->PlatformUserData)
				continue;

			GLFWwindow* windowHandle = *static_cast<GLFWwindow **>(viewport->PlatformUserData); /// First member is GLFWwindow
			if (!windowHandle)
			{
                continue;
			}

            if (const auto state = glfwGetKey(windowHandle, static_cast<int32_t>(keycode)); state == GLFW_PRESS || state == GLFW_REPEAT)
			{
				pressed = true;
				break;
			}
		}

		return pressed;
	}

	bool Input::IsKeyReleased(const KeyCode keycode)
	{
		return s_KeyData.contains(keycode) && s_KeyData[keycode].State == KeyState::Released;
	}

	bool Input::IsKeyToggledOn(const KeyCode keycode)
	{

		if (keycode == KeyCode::CapsLock)
            return (s_Mods & KeyMods::CapsLock) != KeyMods::None;

        if (keycode == KeyCode::NumLock)
            return (s_Mods & KeyMods::NumLock) != KeyMods::None;


        // not (yet) supported by GLFW.  Refer GLFW GitHub issue 1727
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
        if (bool enableImGui = Application::Get().GetAppData().EnableImGui; !enableImGui)
		{
            auto &window = WindowData::window;
			auto state = glfwGetMouseButton(static_cast<GLFWwindow*>(window.GetWindow()), static_cast<int32_t>(button));
			return state == GLFW_PRESS;
		}

		ImGuiContext* context = ImGui::GetCurrentContext();
		bool pressed = false;
		for (const ImGuiViewport* viewport : context->Viewports)
		{
			if (!viewport->PlatformUserData)
				continue;

			GLFWwindow* windowHandle = *(GLFWwindow**)viewport->PlatformUserData; // First member is GLFWwindow
			if (!windowHandle)
				continue;

            if (auto state = glfwGetMouseButton(static_cast<GLFWwindow*>(windowHandle), static_cast<int32_t>(button)); state == GLFW_PRESS || state == GLFW_REPEAT)
			{
				pressed = true;
				break;
			}
		}
		return pressed;
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
        auto &window = static_cast<Window &>(WindowData::window);

		double x, y;
		glfwGetCursorPos(static_cast<GLFWwindow*>(window), &x, &y);
		return { (float)x, (float)y };
	}

	///< TODO: A better way to do this is to handle it internally, and simply move the cursor the opposite side of the screen when it reaches the edge

    /*
    void Input::SetCursorMode(CursorMode mode)
	{
        auto &window = static_cast<Window&>(WindowData::window);
		glfwSetInputMode(static_cast<GLFWwindow*>(window), GLFW_CURSOR, GLFW_CURSOR_NORMAL + (int)mode);

		if (Application::Get().GetAppData().EnableImGui)
			UI::SetInputEnabled(mode == CursorMode::Normal);
	}
	*/

	CursorMode Input::GetCursorMode()
	{
        auto &window = static_cast<Window &>(WindowData::window);
		return (CursorMode)(glfwGetInputMode(static_cast<GLFWwindow*>(window), GLFW_CURSOR) - GLFW_CURSOR_NORMAL);
	}

	void Input::TransitionPressedKeys()
	{
		for (const auto& [key, keyData] : s_KeyData)
		{
			if (keyData.State == KeyState::Pressed)
				UpdateKeyState(key, KeyState::Held);
		}

	}

	void Input::TransitionPressedButtons()
	{
		for (const auto& [button, buttonData] : s_MouseData)
		{
			if (buttonData.State == KeyState::Pressed)
				UpdateButtonState(button, KeyState::Held);
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
			s_Mods = mods.value();
		else
			s_Mods = KeyMods::None;
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
				UpdateKeyState(key, KeyState::None);
		}

		for (const auto& [button, buttonData] : s_MouseData)
		{
			if (buttonData.State == KeyState::Released)
				UpdateButtonState(button, KeyState::None);
		}
	}

}

/// -------------------------------------------------------

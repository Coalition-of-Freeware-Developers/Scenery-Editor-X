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
 * window.cpp
 * -------------------------------------------------------
 * Created: 16/3/2025
 * -------------------------------------------------------
 */
#include "window.h"
#include "monitor_data.h"
#include "SceneryEditorX/core/input/input.h"
#include <SceneryEditorX/core/application/application.h>
#include <SceneryEditorX/core/events/application_events.h>
#include <SceneryEditorX/core/events/key_events.h>
#include <SceneryEditorX/core/events/mouse_events.h>
#include <imgui/imgui.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
    // Static member definitions
	Window::EventCallbackFn Window::s_EventCallback = nullptr;
    SDL_Window *Window::window = nullptr;
	SDL_DisplayID *Window::displays = nullptr;

    // -------------------------------------------------------

	const char *Window::name = "Scenery Editor X";
	int Window::width = 1280;
	int Window::height = 720;
	int Window::posX = 0;
	int Window::posY = 30;
	int Window::displayIndex = 0;
	int Window::displayCount = 0;
	int Window::displayModeIndex = 0;

    // -------------------------------------------------------

	bool Window::framebufferResized = false;
	std::chrono::high_resolution_clock::time_point Window::lastTime;
	float Window::deltaTime = 0.0f;

    // -------------------------------------------------------

	std::vector<std::string> Window::pathsDrop;
	float Window::scroll = 0.0f;
	float Window::deltaScroll = 0.0f;
	Vec2 Window::mousePos = Vec2(.0f, .0f);
	Vec2 Window::deltaMousePos = Vec2(.0f, .0f);

    // -------------------------------------------------------

	char Window::lastKeyState[SDL_SCANCODE_COUNT];
	WindowMode Window::mode = WindowMode::Windowed;
	bool Window::dirty = true;
	bool Window::resizable = true;
	bool Window::decorated = true;
	bool Window::maximized = true;
	bool Window::shouldClose = false;

    // -------------------------------------------------------

	static std::string VideoModeText(const SDL_DisplayMode *mode)
    {
        if (!mode)
        {
            return "Unknown";
        }

        return ToString(mode->w) + "x" + ToString(mode->h) + " " + ToString(static_cast<int>(mode->refresh_rate)) +
               " Hz";
    };

    // -------------------------------------------------------

	void Window::HandleEvent(const SDL_Event &event)
	{
	    switch (event.type)
	    {
	    case SDL_EVENT_QUIT:
	    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
	    {
	        Window::shouldClose = true;
	        if (s_EventCallback)
	        {
	            WindowCloseEvent closeEvent;
	            s_EventCallback(closeEvent);
	        }
	        break;
	    }
	    case SDL_EVENT_WINDOW_RESIZED:
	    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
	    {
	        Window::width = event.window.data1;
	        Window::height = event.window.data2;
	        Window::framebufferResized = true;
	        if (s_EventCallback)
	        {
	            WindowResizeEvent resizeEvent(static_cast<unsigned int>(Window::width),
	                                         static_cast<unsigned int>(Window::height));
	            s_EventCallback(resizeEvent);
	        }
	        break;
	    }
	    case SDL_EVENT_WINDOW_MINIMIZED:
	    {
	        if (s_EventCallback)
	        {
	            WindowMinimizeEvent minimizeEvent(true);
	            s_EventCallback(minimizeEvent);
	        }
	        break;
	    }
	    case SDL_EVENT_WINDOW_RESTORED:
	    {
	        Window::maximized = false;
	        if (s_EventCallback)
	        {
	            WindowMinimizeEvent minimizeEvent(false);
	            s_EventCallback(minimizeEvent);
	            WindowMaximizeEvent maximizeEvent(false);
	            s_EventCallback(maximizeEvent);
	        }
	        break;
	    }
	    case SDL_EVENT_WINDOW_MAXIMIZED:
	    {
	        Window::maximized = true;
	        if (s_EventCallback)
	        {
	            WindowMaximizeEvent maximizeEvent(true);
	            s_EventCallback(maximizeEvent);
	        }
	        break;
	    }
	    case SDL_EVENT_WINDOW_MOVED:
	        Window::posX = event.window.data1;
	        Window::posY = event.window.data2;
	        break;
	    case SDL_EVENT_MOUSE_WHEEL:
	    {
	        Window::scroll += event.wheel.y;
	        Window::deltaScroll += event.wheel.y;
	        if (s_EventCallback)
	        {
	            MouseScrolledEvent scrollEvent(static_cast<float>(event.wheel.x), static_cast<float>(event.wheel.y));
	            s_EventCallback(scrollEvent);
	        }
	        break;
	    }
	    case SDL_EVENT_MOUSE_MOTION:
	    {
	        if (s_EventCallback)
	        {
	            MouseMovedEvent moveEvent(static_cast<float>(event.motion.x), static_cast<float>(event.motion.y));
	            s_EventCallback(moveEvent);
	        }
	        break;
	    }
	    case SDL_EVENT_MOUSE_BUTTON_DOWN:
	    case SDL_EVENT_MOUSE_BUTTON_UP:
	    {
	        Input::OnMouseButtonEvent(event.button);
	        if (s_EventCallback)
	        {
	            if (const auto button = Input::FromSDLMouseButton(event.button.button))
	            {
	                if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
	                {
	                    MouseButtonPressedEvent mouseEvent(*button);
	                    s_EventCallback(mouseEvent);
	                }
	                else
	                {
	                    MouseButtonReleasedEvent mouseEvent(*button);
	                    s_EventCallback(mouseEvent);
	                }
	            }
	        }
	        break;
	    }
	    case SDL_EVENT_KEY_DOWN:
	    case SDL_EVENT_KEY_UP:
	    {
	        Input::OnKeyEvent(event.key);
	        if (s_EventCallback)
	        {
	            if (const auto key = Input::FromSDLScancode(event.key.scancode))
	            {
	                if (event.type == SDL_EVENT_KEY_DOWN)
	                {
	                    KeyPressedEvent keyEvent(*key, event.key.repeat ? 1 : 0);
	                    s_EventCallback(keyEvent);
	                }
	                else
	                {
	                    KeyReleasedEvent keyEvent(*key);
	                    s_EventCallback(keyEvent);
	                }
	            }
	        }
	        break;
	    }
	    case SDL_EVENT_TEXT_INPUT:
	    {
	        if (s_EventCallback && event.text.text[0] != '\0')
	        {
	            if (const auto key = Input::FromCharacter(event.text.text[0]))
	            {
	                KeyTypedEvent typedEvent(*key);
	                s_EventCallback(typedEvent);
	            }
	        }
	        break;
	    }
	    case SDL_EVENT_DROP_FILE:
	    {
	        if (event.drop.data)
	        {
	            pathsDrop.emplace_back(event.drop.data);
	            SDL_free(const_cast<char *>(event.drop.data));
	        }
	        break;
	    }
	    default:
	        break;
	    }
	}
	
	void Window::Create()
	{
	    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
	    {
	        return;
	    }

	    displays = SDL_GetDisplays(&displayCount);
		
	    SDL_WindowFlags flags = SDL_WINDOW_VULKAN;
	    if (resizable)
	    {
	        flags |= SDL_WINDOW_RESIZABLE;
	    }
	    if (maximized)
	    {
	        flags |= SDL_WINDOW_MAXIMIZED;
	    }

	    window = SDL_CreateWindow(name, width, height, flags);
	    if (!window)
	    {
	        SDL_Quit();
	        return;
	    }

	    SDL_SetWindowPosition(window, posX, posY);

	    dirty = false;
	    ApplyChanges();
	}
	
	void Window::ApplyChanges()
	{
	    if (!window)
	    {
	        return;
	    }
	
		displays = SDL_GetDisplays(&displayCount);
		if (!displays || displayCount <= 0)
		{
		    return;
		}
		if (displayIndex >= displayCount)
		{
		    displayIndex = 0;
		}

	    SDL_DisplayID display = displays[displayIndex];
	    int modesCount = 0;
	    SDL_DisplayMode **displayModes = SDL_GetFullscreenDisplayModes(display, &modesCount);
	    if (modesCount <= 0)
	    {
	        displayModes = nullptr;
	        modesCount = 0;
	    }
	    if (displayModeIndex >= modesCount && modesCount > 0)
	    {
	        displayModeIndex = modesCount - 1;
	    }

	    switch (mode)
	    {
	    case WindowMode::Windowed:
	        SDL_SetWindowFullscreen(window, false);
	        SDL_SetWindowSize(window, width, height);
	        SDL_SetWindowPosition(window, posX, posY);
	        SDL_SetWindowResizable(window, resizable);
	        SDL_SetWindowBordered(window, decorated);
	        if (maximized)
	        {
	            SDL_MaximizeWindow(window);
	        }
	        else
	        {
	            SDL_RestoreWindow(window);
	        }
	        break;
	    case WindowMode::WindowedFullScreen:
	    {
	        if (const SDL_DisplayMode *desktopMode = SDL_GetDesktopDisplayMode(display))
	        {
	            SDL_SetWindowFullscreenMode(window, desktopMode);
	            SDL_SetWindowFullscreen(window, true);
	        }
	    }
	    break;
	    case WindowMode::FullScreen:
	        if (displayModes && displayModeIndex >= 0 && displayModeIndex < modesCount)
	        {
	            SDL_SetWindowFullscreenMode(window, displayModes[displayModeIndex]);
	            SDL_SetWindowFullscreen(window, true);
	        }
	        break;
	    }

	    if (displayModes)
	    {
	        SDL_free(static_cast<void *>(displayModes));
	    }

	    framebufferResized = false;
	    dirty = false;
	}
	
	/**
	 * @brief Sets the window title
	 * @param title The new title for the window
	 */
	void Window::SetTitle(const std::string &title)
	{
	    if (window)
	    {
	        SDL_SetWindowTitle(window, title.c_str());
	    }
	}
	
	bool Window::IsMouseDown(uint8_t buttonCode)
	{
	    return (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_MASK(buttonCode)) != 0;
	}
	
	void Window::SetMode(WindowMode newMode)
	{
	    mode = newMode;
	    dirty = true;
	}
	
	std::vector<std::string> Window::GetAndClearPaths()
	{
	    auto paths = pathsDrop;
	    pathsDrop.clear();
	    return paths;
	}
	
	void Window::Destroy()
	{
	    if (window)
	    {
	        SDL_GetWindowPosition(window, &posX, &posY);
	        SDL_DestroyWindow(window);
	        window = nullptr;
	    }
	    if (displays)
	    {
	        SDL_free(displays);
	        displays = nullptr;
	    }
	    SDL_Quit();
	}
	
	void Window::Update()
	{
		const bool *keyboardState = SDL_GetKeyboardState(nullptr);
		for (int i = 0; i < SDL_SCANCODE_COUNT; i++)
		{
		    lastKeyState[i] = static_cast<char>(keyboardState[i]);
		}

		auto newTime = std::chrono::high_resolution_clock::now();
		const auto elapsed = std::chrono::duration<float, std::milli>(newTime - lastTime);
		deltaTime = elapsed.count();
		lastTime = newTime;

		float x, y;
		SDL_GetMouseState(&x, &y);
		deltaMousePos = mousePos - Vec2(x, y);
		mousePos = Vec2(x, y);
    };

	void Window::ProcessEvents()
	{
	    deltaScroll = 0.0f;
	
	    SDL_Event event;
	    while (SDL_PollEvent(&event))
	    {
	        HandleEvent(event);
	    }
	}
	
	void Window::UpdateFramebufferSize()
	{
	    framebufferResized = false;
	    if (window)
	    {
	        SDL_GetWindowSizeInPixels(window, &width, &height);
	    }
	}

	/*
	void Window::OnImgui()
	{
	    const float totalWidth = ImGui::GetContentRegionAvail().x;
	    if (ImGui::CollapsingHeader("Window"))
	    {
	        // mode
	        {
	            const char* modeNames[] = {"Windowed", "Windowed FullScreen", "FullScreen"};
	            ImGui::Text("Mode");
	            ImGui::SameLine(totalWidth / 2.0f);
	            ImGui::SetNextItemWidth(totalWidth / 2.0f);
	            ImGui::PushID("modeCombo");
	            if (ImGui::BeginCombo("", modeNames[(int)mode]))
	            {
	                for (int i = 0; i < 3; i++)
	                {
	                    bool selected = (int)mode == i;
	                    if (ImGui::Selectable(modeNames[i], selected))
	                    {
	                        mode  = (WindowMode)i;
	                        dirty = true;
	                    }
	                    if (selected) { ImGui::SetItemDefaultFocus(); }
	                }
	                ImGui::EndCombo();
	            }
	            ImGui::PopID();
	        }
	        if (mode != WindowMode::Windowed)
	        {
	            // display
	            // display
	            {
	                ImGui::Text("Display");
	                ImGui::Text("Display");
	                ImGui::SameLine(totalWidth / 2.0f);
	                ImGui::SetNextItemWidth(totalWidth / 2.0f);
	                ImGui::PushID("displayCombo");
	                const char* displayName = SDL_GetDisplayName(displays[displayIndex]);
	                if (ImGui::BeginCombo("", displayName ? displayName : "Unknown"))
	                {
	                    for (int i = 0; i < displayCount; i++)
	                    {
	                        bool selected = displayIndex == i;
	                        ImGui::PushID("displayCombo");
	                        const char* displayName = SDL_GetDisplayName(displays[displayIndex]);
	                        if (ImGui::BeginCombo("", displayName ? displayName : "Unknown"))
	                        {
	                            for (int i = 0; i < displayCount; i++)
	                            {
	                                bool selected = displayIndex == i;
	                                ImGui::PushID(i);
	                                const char* name = SDL_GetDisplayName(displays[i]);
	                                if (ImGui::Selectable(name ? name : "Unknown", selected))
	                                {
	                                    displayIndex     = i;
	                                    const char* name = SDL_GetDisplayName(displays[i]);
	                                    if (ImGui::Selectable(name ? name : "Unknown", selected))
	                                    {
	                                        displayIndex = i;
	                                        dirty        = true;
	                                    }
	                                    if (selected) { ImGui::SetItemDefaultFocus(); }
	                                    ImGui::PopID();
	                                }
	                                ImGui::EndCombo();
	                            }
	                            ImGui::PopID();
	                        }
	                    }
	                    // resolution
	                    {
	                        if (mode == WindowMode::FullScreen)
	                        {
	                            ImGui::Text("Resolution");
	                            ImGui::SameLine(totalWidth / 2.0f);
	                            ImGui::SetNextItemWidth(totalWidth / 4.0f);
	                            ImGui::PushID("displayRes");
	                            ImGui::PushID("displayRes");
	                            int modesCount;
	                            const SDL_DisplayMode** displayModes =
	                                SDL_GetFullscreenDisplayModes(displays[displayIndex], &modesCount);
	                            if (displayModes && modesCount > 0)
	                            {
	                                if (displayModeIndex >= modesCount) displayModeIndex = 0;
	                                const SDL_DisplayMode* currMode = displayModes[displayModeIndex];
	                                std::string modeText            = VideoModeText(currMode);
	                                if (ImGui::BeginCombo("", modeText.c_str()))
	                                {
	                                    for (int i = 0; i < modesCount; i++)
	                                    {
	                                        bool selected = displayModeIndex == i;
	                                        currMode      = displayModes[i];
	                                        ImGui::PushID(i);
	                                        modeText = VideoModeText(currMode);
	                                        if (ImGui::Selectable(modeText.c_str(), selected))
	                                        {
	                                            displayModeIndex = i;
	                                            dirty            = true;
	                                        }
	                                        if (selected) { ImGui::SetItemDefaultFocus(); }
	                                        ImGui::PopID();
	                                    }
	                                    ImGui::EndCombo();
	                                }
	                                SDL_free((void*)displayModes);
	                            }
	                            if (displayModes) { SDL_free((void*)displayModes); }
	                            ImGui::PopID();
	                        }
	                    }
	                    // windowed only
	                    {
	                        if (mode == WindowMode::Windowed)
	                        {
	                            // maximized
	                            {
	                                ImGui::Text("Maximized");
	                                ImGui::SameLine(totalWidth / 2.0f);
	                                ImGui::SetNextItemWidth(totalWidth / 2.0f);
	                                ImGui::PushID("maximized");
	                                if (ImGui::Checkbox("", &maximized)) { dirty = true; }
	                                ImGui::PopID();
	                            }
	                            // decorated
	                            {
	                                ImGui::Text("Decorated");
	                                ImGui::SameLine(totalWidth / 2.0f);
	                                ImGui::SetNextItemWidth(totalWidth / 2.0f);
	                                ImGui::PushID("decorated");
	                                if (ImGui::Checkbox("", &decorated)) { dirty = true; }
	                                ImGui::PopID();
	                            }
	                            // resizable
	                            {
	                                ImGui::Text("Resizable");
	                                ImGui::SameLine(totalWidth / 2.0f);
	                                ImGui::SetNextItemWidth(totalWidth / 2.0f);
	                                ImGui::PushID("resizable");
	                                if (ImGui::Checkbox("", &resizable)) { dirty = true; }
	                                ImGui::PopID();
	                            }
	                        }
	                    }
	                }
	            }
	
	            void Window::UpdateFramebufferSize()
	            {
	                framebufferResized = false;
	                if (window) { SDL_GetWindowSizeInPixels(window, &width, &height); }
	                if (window) { SDL_GetWindowSizeInPixels(window, &width, &height); }
	            }
	
	            bool Window::IsKeyPressed(SDL_Scancode keyCode)
	            {
	                if (keyCode >= SDL_NUM_SCANCODES) return false;
	                const uint8_t* currentState = SDL_GetKeyboardState(nullptr);
	                return lastKeyState[keyCode] && !currentState[keyCode];
	                bool Window::IsKeyPressed(SDL_Scancode keyCode)
	                {
	                    if (keyCode >= SDL_NUM_SCANCODES) return false;
	                    const uint8_t* currentState = SDL_GetKeyboardState(nullptr);
	                    return lastKeyState[keyCode] && !currentState[keyCode];
	                }
	            }
	        }
	    }
	}
	*/
	
	bool Window::IsKeyPressed(SDL_Scancode keyCode)
    {
        const bool *keyboardState = SDL_GetKeyboardState(nullptr);
        return keyboardState[keyCode] && !lastKeyState[keyCode];
    }

	uint32_t Window::GetWidth() { return width; }
	uint32_t Window::GetHeight() { return height; }
	Vec2 Window::GetWindowSize() { return {static_cast<float>(width), static_cast<float>(height)}; }

	// Accessor implementations (non-inline to avoid static member export issues across DLL boundaries)
    bool Window::IsDirty() { return dirty; }
    void Window::WaitEvents() { SDL_WaitEvent(nullptr); }
	float Window::GetDeltaTime() { return deltaTime; }
	bool Window::GetShouldClose() { return shouldClose; }
	float Window::GetDeltaScroll() { return deltaScroll; }
	Vec2 Window::GetDeltaMouse() { return deltaMousePos; }
	bool Window::GetFramebufferResized() { return framebufferResized; }
	bool Window::IsKeyDown(SDL_Scancode keyCode) { return SDL_GetKeyboardState(nullptr)[keyCode]; }
	void Window::SetShouldClose(bool close) { shouldClose = close; }
	void Window::SetEventCallback(const EventCallbackFn &callback) { s_EventCallback = callback; }
	
	void Window::Maximize()
    {
        if (!window)
            return;

        SDL_MaximizeWindow(window);
        maximized = true;
    }

    bool Window::IsMaximized()
    {
        return SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN;
    }

    void Window::Minimize()
    {
        if (!window)
            return;

        SDL_MinimizeWindow(window);
    }

    bool Window::IsMinimized()
    {
        return SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED;
    }

    void Window::CenterWindow()
    {
        if (!window)
            return;

        SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        SDL_GetWindowPosition(window, &posX, &posY);
    }
	
	void Window::SetResizable(bool value)
    {
        resizable = value;

        if (window)
            SDL_SetWindowResizable(window, value);
    }
	
	void Window::SetDecorated(bool value)
    {
        decorated = value;

        if (window)
            SDL_SetWindowBordered(window, value);
    }
		
}

// -------------------------------------------------------

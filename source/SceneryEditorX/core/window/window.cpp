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
#include <tracy/Tracy.hpp>

// -------------------------------------------------------

namespace SceneryEditorX
{
    // Static member definitions
	Window::EventCallbackFn Window::s_EventCallback = nullptr;
	Window Window::m_Window;
    SDL_Window *s_Window = nullptr;
	SDL_DisplayID *s_Displays = nullptr;

    // -------------------------------------------------------

	const char *Window::name = "Scenery Editor X";
	int Window::width = 1280;
	int Window::height = 720;
	int Window::posX = 0;
	int Window::posY = 30;
	int Window::displayIndex = 0;
	int Window::displayCount = 0;
	int Window::displayModeIndex = 0;
    static float s_DPI_Scale = 1.0f;

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
    bool Window::borderless = false;
	bool Window::resizable = true;
	bool Window::decorated = true;
	bool Window::maximized = true;
	bool Window::shouldClose = false;

    // Custom Title Bar
    static float s_Titlebar_Height = 40.0f;        // default height, updated by editor
    static float s_Titlebar_Button_Width = 150.0f; // default width, updated by editor
    static const float RESIZE_BORDER = 8.0f;     // thickness of resize borders
    static int s_Titlebar_HoveredFrames = 0;      // persistence counter for hover state

    // -------------------------------------------------------


    static SDL_HitTestResult HitTestCallback(SDL_Window *win, const SDL_Point *area, void *data)
    {
        int w, h;
        SDL_GetWindowSize(win, &w, &h);

        const int x = area->x;
        const int y = area->y;
        const int resizeMargin = static_cast<int>(RESIZE_BORDER * s_DPI_Scale);

        // check corners first (for diagonal resize)
        bool top = y < resizeMargin;
        bool bottom = y >= h - resizeMargin;
        bool left = x < resizeMargin;
        bool right = x >= w - resizeMargin;

        // corner hit tests
        if (top && left)
            return SDL_HITTEST_RESIZE_TOPLEFT;
        if (top && right)
            return SDL_HITTEST_RESIZE_TOPRIGHT;
        if (bottom && left)
            return SDL_HITTEST_RESIZE_BOTTOMLEFT;
        if (bottom && right)
            return SDL_HITTEST_RESIZE_BOTTOMRIGHT;

        // edge hit tests
        if (top)
            return SDL_HITTEST_RESIZE_TOP;
        if (bottom)
            return SDL_HITTEST_RESIZE_BOTTOM;
        if (left)
            return SDL_HITTEST_RESIZE_LEFT;
        if (right)
            return SDL_HITTEST_RESIZE_RIGHT;

        // title bar area - make draggable only when no imgui items are hovered
        if (y < static_cast<int>(s_Titlebar_Height))
        {
            // exclude window buttons area on the right
            if (x < w - static_cast<int>(s_Titlebar_Button_Width))
            {
                // only allow dragging when no imgui items were hovered recently
                // use persistence to avoid timing issues between hit test and imgui frame
                if (s_Titlebar_HoveredFrames == 0)
                {
                    return SDL_HITTEST_DRAGGABLE;
                }
            }
        }

        return SDL_HITTEST_NORMAL;
    }

    static void InitSDLSubSystems()
	{
        if (!SDL_WasInit(SDL_INIT_AUDIO))
        {
            if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
            {
                SEDX_CORE_ERROR_TAG("Window","Failed to initialise SDL audio subsystem: %s.", SDL_GetError());
            }
        }

        if (!SDL_WasInit(SDL_INIT_VIDEO))
        {
            if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
            {
                SEDX_CORE_ERROR_TAG("Window", "Failed to initialise SDL video subsystem: %s.", SDL_GetError());
            }
        }

        if (!SDL_WasInit(SDL_INIT_GAMEPAD))
        {
            if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD))
            {
                SEDX_CORE_ERROR_TAG("Window", "Failed to initialise SDL gamepad subsystem: %s.", SDL_GetError());
            }
        }
    }

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
	    case SDL_EVENT_WINDOW_SHOWN:
		{
			if (s_EventCallback)
			{
				WindowHiddenEvent hiddenEvent(false);
				WindowShowEvent showEvent(true);
				s_EventCallback(showEvent);
			}
			break;
        }
        case SDL_EVENT_WINDOW_HIDDEN:
	    {
			Window::maximized = false;
	        if (s_EventCallback)
			{
				WindowHiddenEvent hiddenEvent(true);
				s_EventCallback(hiddenEvent);
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
                WindowHiddenEvent hiddenEvent(false);
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
        // set the process to be per monitor DPI aware - Windows 10 v1607+ (Creators Tick)
        #ifdef SEDX_PLATFORM_WINDOWS
		#pragma comment(lib, "Shcore.lib")

        if (HMODULE user32 = LoadLibrary(TEXT("user32.dll")))
        {
            typedef DPI_AWARENESS_CONTEXT(WINAPI * pfn)(DPI_AWARENESS_CONTEXT);
            if (pfn SetThreadDpiAwarenessContext = (pfn)GetProcAddress(user32, "SetThreadDpiAwarenessContext"))
            {
                SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
            }
            FreeLibrary(user32);
        }
        #endif
        
		InitSDLSubSystems();

	    s_Displays = SDL_GetDisplays(&displayCount);
		
        uint32_t flags = SDL_WINDOW_VULKAN;
	    if (resizable)
	    {
	        flags |= SDL_WINDOW_RESIZABLE;
	    }
	    if (maximized)
	    {
	        flags |= SDL_WINDOW_MAXIMIZED;
	    }
        if (borderless)
		{
			flags |= SDL_WINDOW_BORDERLESS;
		}

	    s_Window = SDL_CreateWindow(name, width, height, flags);
		SEDX_CORE_TRACE_TAG("Window", "Window created with flags: {}", flags);
		SEDX_CORE_TRACE_TAG("Window", "Window created with dimensions: {}x{}", width, height);
	    if (!s_Window)
	    {
            SEDX_CORE_ERROR_TAG("Window", "Failed to create window: {}", SDL_GetError());
	        return;
	    }

	    // set up hit test callback for custom title bar dragging and resizing
        if (!SDL_SetWindowHitTest(s_Window, HitTestCallback, nullptr))
        {
            SEDX_CORE_WARN_TAG("Window","Failed to set window hit test callback: %s", SDL_GetError());
        }

	    SDL_SetWindowPosition(s_Window, posX, posY);

	    // get the DPI scale - has to be done after window creation
    #ifdef SEDX_PLATFORM_WINDOWS
        s_DPI_Scale = static_cast<float>(GetDpiForWindow(static_cast<HWND>(GetRawHandle()))) / 96.0f;
        SEDX_CORE_TRACE_TAG("Window", "DPI Scale factor: {}", s_DPI_Scale);
    #endif

		Show();
	    dirty = false;
	    //ApplyChanges();
		SEDX_CORE_TRACE_TAG("Window", "Window created: {}", s_Window ? "success" : "failure");
	}
	
	void Window::ApplyChanges()
	{
        SEDX_CORE_ASSERT(s_Window != nullptr, "Window not created");
	
		s_Displays = SDL_GetDisplays(&displayCount);
		if (!s_Displays || displayCount <= 0)
		{
		    return;
		}
		if (displayIndex >= displayCount)
		{
		    displayIndex = 0;
		}

	    SDL_DisplayID display = s_Displays[displayIndex];
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
	        SDL_SetWindowFullscreen(s_Window, false);
	        SDL_SetWindowSize(s_Window, width, height);
	        SDL_SetWindowPosition(s_Window, posX, posY);
	        SDL_SetWindowResizable(s_Window, resizable);
	        SDL_SetWindowBordered(s_Window, decorated);
	        if (maximized)
	        {
	            SDL_MaximizeWindow(s_Window);
	        }
	        else
	        {
	            SDL_RestoreWindow(s_Window);
	        }
	        break;
	    case WindowMode::WindowedFullScreen:
	    {
	        if (const SDL_DisplayMode *desktopMode = SDL_GetDesktopDisplayMode(display))
	        {
	            SDL_SetWindowFullscreenMode(s_Window, desktopMode);
	            SDL_SetWindowFullscreen(s_Window, true);
	        }
	    }
	    break;
	    case WindowMode::FullScreen:
	        if (displayModes && displayModeIndex >= 0 && displayModeIndex < modesCount)
	        {
	            SDL_SetWindowFullscreenMode(s_Window, displayModes[displayModeIndex]);
	            SDL_SetWindowFullscreen(s_Window, true);
	        }
	        break;
	    }

	    if (displayModes)
	    {
	        SDL_free(displayModes);
	    }

	    framebufferResized = false;
	    dirty = false;
	}
	
    void *Window::GetRawHandle()
    {
	    SEDX_CORE_ASSERT(s_Window, "Window is not initialized");

        SDL_PropertiesID props = SDL_GetWindowProperties(s_Window);

        // windows
        if (void *handle = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr))
            return handle;

        // wayland
        if (void *handle = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr))
            return handle;

        // x11
        if (Uint64 x11_window = SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0))
            return reinterpret_cast<void *>(x11_window);

        return nullptr;
    }

    /**
	 * @brief Sets the window title
	 * @param title The new title for the window
	 */
	void Window::SetTitle(const std::string &title)
	{
	    if (s_Window)
	    {
	        SDL_SetWindowTitle(s_Window, title.c_str());
	    }
	}

    SDL_Window *Window::GetWindow()
    {
        return s_Window;
    }

    bool Window::IsMouseDown(const uint8_t buttonCode)
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
	    if (s_Window)
	    {
	        SDL_GetWindowPosition(s_Window, &posX, &posY);
	        SDL_DestroyWindow(s_Window);
	        s_Window = nullptr;
	    }

	    if (s_Displays)
	    {
	        SDL_free(s_Displays);
	        s_Displays = nullptr;
	    }

	    SDL_Quit();
	}
	
	void Window::Tick()
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
	    if (s_Window)
	    {
	        SDL_GetWindowSizeInPixels(s_Window, &width, &height);
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
        SEDX_CORE_ASSERT(s_Window, "Window is not initialized");

        SDL_MaximizeWindow(s_Window);
		SDL_Event event;
        event.type = SDL_EVENT_WINDOW_MAXIMIZED;
        event.window.windowID = SDL_GetWindowID(s_Window);
        SDL_PushEvent(&event);
        maximized = true;
    }

	bool Window::IsVisible()
	{
        SEDX_CORE_ASSERT(s_Window, "Window is not initialized");

	    uint64_t flags = SDL_GetWindowFlags(s_Window);
		if (flags & SDL_EVENT_WINDOW_SHOWN) return true;
		if (flags & SDL_EVENT_WINDOW_MAXIMIZED) return true;
        if (flags & SDL_EVENT_WINDOW_RESIZED) return true;

        return false;
	}

    bool Window::IsMaximized() { return SDL_GetWindowFlags(s_Window) & SDL_WINDOW_FULLSCREEN; }

    void Window::Minimize()
    {
        SEDX_CORE_ASSERT(s_Window, "Window is not initialized");

        SDL_MinimizeWindow(s_Window);
        SDL_Event event;
	    event.type = SDL_EVENT_WINDOW_MINIMIZED;
        event.window.windowID = SDL_GetWindowID(s_Window);
        SDL_PushEvent(&event);
    }

    void Window::Show()
    {
        SEDX_CORE_ASSERT(s_Window, "Window is not initialized");

        SDL_ShowWindow(s_Window);
		SDL_Event event;
		event.type = SDL_EVENT_WINDOW_SHOWN;
		event.window.windowID = SDL_GetWindowID(s_Window);
		SDL_PushEvent(&event);
    }

    void Window::Hide()
    {
        SEDX_CORE_ASSERT(s_Window, "Window is not initialized");

		SDL_HideWindow(s_Window);
		SDL_Event event;
        event.type = SDL_EVENT_WINDOW_HIDDEN;
        event.window.windowID = SDL_GetWindowID(s_Window);
        SDL_PushEvent(&event);
    }

    void Window::Focus()
    {
        SEDX_CORE_ASSERT(s_Window, "Window is not initialized");

        SDL_RaiseWindow(s_Window);
		SDL_Event event;
		event.type = SDL_EVENT_WINDOW_FOCUS_GAINED;
		event.window.windowID = SDL_GetWindowID(s_Window);
        SDL_PushEvent(&event);
    }

    bool Window::IsMinimized() { return SDL_GetWindowFlags(s_Window) & SDL_WINDOW_MINIMIZED; }

    void Window::CenterWindow()
    {
        if (!s_Window)
            return;

        SDL_SetWindowPosition(s_Window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        SDL_GetWindowPosition(s_Window, &posX, &posY);
    }
	
	void Window::SetResizable(bool value)
    {
        resizable = value;

        if (s_Window)
        {
            SDL_SetWindowResizable(s_Window, value);
        }
    }
	
	void Window::SetDecorated(bool value)
    {
        decorated = value;

        if (s_Window)
        {
            SDL_SetWindowBordered(s_Window, value);
        }
    }
		
}

// -------------------------------------------------------

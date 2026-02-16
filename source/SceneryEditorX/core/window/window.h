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
 * window.h
 * -------------------------------------------------------
 * Created: 16/3/2025
 * -------------------------------------------------------
 */
#pragma once
#include <functional>
#include <SDL3/SDL.h>
#include <SceneryEditorX/renderer/vulkan/render_context.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class Event;

	enum class WindowMode : uint8_t
	{
	    Windowed,
	    WindowedFullScreen,
	    FullScreen
	};

    // -------------------------------------------------------

	class Window
	{
        // Forward declared private event types to avoid circular dependency with event_system.h
        typedef std::function<void(Event &)> EventCallbackFn;
        static EventCallbackFn s_EventCallback;

	public:
	    static void Create();
	    static void Tick();
	    static void ProcessEvents();
	    static void OnImgui();
	    static void Destroy();

	    static void ApplyChanges();
        static void UpdateFramebufferSize();
	    static bool IsKeyPressed(SDL_Scancode keyCode);
	    static void SetTitle(const std::string &title);

	    static SDL_Window *GetWindow();
        static void *GetRawHandle();
        static Window Get() { return m_Window; }
	    static uint32_t GetWidth();
	    static uint32_t GetHeight();
        static Vec2 GetWindowSize();

	    static bool IsDirty();
        static void WaitEvents();

	    static float GetDeltaTime();
	    static bool GetShouldClose();
	    static float GetDeltaScroll();
	    static Vec2 GetDeltaMouse();
	    static bool GetFramebufferResized();

	    static bool IsKeyDown(SDL_Scancode keyCode);
	    static bool IsMouseDown(uint8_t buttonCode);
	    static void SetMode(WindowMode newMode);
	    static void SetShouldClose(bool close);
	    static std::vector<std::string> GetAndClearPaths();
	    static void SetEventCallback(const EventCallbackFn &callback);

	    static void Maximize();
        static bool IsMaximized();
		static void Minimize();
        static void Show();
        static void Hide();
        static void Focus();
        static bool IsMinimized();
	    static void CenterWindow();

	    static void SetResizable(bool value);
	    static void SetDecorated(bool value);

	private:
        static Window m_Window;
        static const char *name;
        static int width;
        static int height;
        static int posX;
        static int posY;
        static int displayIndex;
        static int displayCount;
        static int displayModeIndex;
        static bool framebufferResized;

        static std::chrono::high_resolution_clock::time_point lastTime;
        static float deltaTime;

        static std::vector<std::string> pathsDrop;

        static float scroll;
        static float deltaScroll;
        static Vec2 mousePos;
        static Vec2 deltaMousePos;

        static char lastKeyState[SDL_SCANCODE_COUNT];
        static WindowMode mode;
        static bool dirty;
        static bool resizable;
        static bool decorated;
        static bool maximized;
        static bool shouldClose;

        static void HandleEvent(const SDL_Event &event);
	};
	
}

// -------------------------------------------------------


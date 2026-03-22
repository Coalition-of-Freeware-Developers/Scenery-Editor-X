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

// -------------------------------------------------------

namespace SceneryEditorX
{
	class Event;
	struct Flag;

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

		typedef std::function<void(Event &)> EventCallbackFn; // Type alias for event callback function that takes an Event reference and returns void
		static EventCallbackFn s_EventCallback; // Global event callback function pointer

	public:
		/**
		 * @brief 
		 */
		static void Create();

		/**
		 * @brief 
		 */
		static void Tick();

		/**
		 * @brief 
		 */
		static void ProcessEvents();

		/**
		 * @brief 
		 */
		static void Destroy();

		/**
		 * @brief 
		 */
		static void ApplyChanges();

		/**
		 * @brief 
		 */
		static void UpdateFramebufferSize();

		/**
		 * @brief 
		 * @param keyCode 
		 * @return 
		 */
		static bool IsKeyPressed(SDL_Scancode keyCode);

		/**
		 * @brief 
		 * @param title 
		 */
		static void SetTitle(const std::string &title);

		/**
		 * @brief 
		 * @return 
		 */
		static SDL_Window *GetWindow();

		/**
		 * @brief 
		 * @return 
		 */
		static void *GetRawHandle();

		/**
		 * @brief 
		 * @return 
		 */
		static Window Get() { return m_Window; }

		/**
		 * @brief 
		 * @return 
		 */
		static uint32_t GetWidth();

		/**
		 * @brief 
		 * @return 
		 */
		static uint32_t GetHeight();

		/**
		 * @brief 
		 * @return 
		 */
		static Vec2 GetWindowSize();

		/**
		 * @brief 
		 * @return 
		 */
		static Flag IsDirty();

		/**
		 * @brief 
		 */
		static void WaitEvents();

		/**
		 * @brief 
		 * @return 
		 */
		static float GetDeltaTime();

		/**
		 * @brief 
		 * @return 
		 */
		static bool GetShouldClose();

		/**
		 * @brief 
		 * @return 
		 */
		static float GetDeltaScroll();

		/**
		 * @brief 
		 * @return 
		 */
		static Vec2 GetDeltaMouse();

		/**
		 * @brief 
		 * @return 
		 */
		static bool GetFramebufferResized();

		/**
		 * @brief 
		 * @param keyCode 
		 * @return 
		 */
		static bool IsKeyDown(SDL_Scancode keyCode);

		/**
		 * @brief 
		 * @param buttonCode 
		 * @return 
		 */
		static bool IsMouseDown(uint8_t buttonCode);

		/**
		 * @brief 
		 * @param newMode 
		 */
		static void SetMode(WindowMode newMode);

		/**
		 * @brief 
		 * @param close 
		 */
		static void SetShouldClose(bool close);

		/**
		 * @brief 
		 * @return 
		 */
		static std::vector<std::string> GetAndClearPaths();

		/**
		 * @brief 
		 * @param callback 
		 */
		static void SetEventCallback(const EventCallbackFn &callback);

		/* @brief Maximizes the window. */
		static void Maximize();

		/**
		 * @brief 
		 * @return 
		 */
		static bool IsVisible();

		/**
		 * @brief 
		 * @return 
		 */
		static bool IsMaximized();

		/**
		 * @brief 
		 */
		static void Minimize();

		/**
		 * @brief 
		 */
		static void Show();

		/**
		 * @brief 
		 */
		static void Hide();

		/**
		 * @brief 
		 */
		static void Focus();

		/**
		 * @brief 
		 * @return 
		 */
		static bool IsMinimized();

		/* @brief Centers the window on the screen. */
		static void CenterWindow();

		/**
		 * @brief 
		 * @param value 
		 */
		static void SetResizable(bool value);

		/**
		 * @brief 
		 * @param value 
		 */
		static void SetDecorated(bool value);

		/**
		 * @brief 
		 * @return 
		 */
		static float GetDpiScale();

		/**
		 * @brief 
		 * @param height 
		 */
		static void SetTitleBarHeight(float height);

		/**
		 * @brief 
		 * @param width 
		 */
		static void SetTitleBarButtonWidth(float width);

		/**
		 * @brief 
		 * @param hovered 
		 */
		static void SetTitleBarHovered(bool hovered);

	private:
		static Window m_Window;		// Singleton instance of the Window class
		static const char *name;	// Window title
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
		static bool borderless;
		static bool dirty;
		static bool resizable;
		static bool decorated;
		static bool maximized;
		static bool shouldClose;

		/**
		 * @brief 
		 * @param event 
		 */
		static void HandleEvent(const SDL_Event &event);
	};
	
}

// -------------------------------------------------------


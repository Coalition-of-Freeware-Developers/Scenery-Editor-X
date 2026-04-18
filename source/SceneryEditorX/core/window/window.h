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

	/**
	 * @enum WindowMode
	 * @brief Enumeration representing different window modes for the application.
	 */
	enum class WindowMode : uint8_t
	{
		Windowed,
		WindowedFullScreen,
		FullScreen
	};

	// -------------------------------------------------------

	/**
	 * @class Window
	 * @brief The Window class provides a static interface for managing the application window, 
	 * handling input, and processing events. 
	 * It encapsulates SDL window creation and event handling logic, allowing other parts of the 
	 * application to interact with the window without needing to directly use SDL APIs.
	 */
	class Window
	{
		// Forward declared private event types to avoid circular dependency with event_system.h
		typedef std::function<void(Event &)> EventCallbackFn; // Type alias for event callback function that takes an Event reference and returns void
		static EventCallbackFn s_EventCallback; // Global event callback function pointer

	public:
		/* @brief Initializes the window and related resources. */
		static void Create();

		/* @brief Updates the window state and handles per-frame logic. */
		static void Tick();

		/* @brief Processes all pending window events. */
		static void ProcessEvents();

		/* @brief Destroys the window and releases associated resources. */
		static void Destroy();

		/* @brief Applies any pending changes to the window state, such as resizing or mode changes. */
		static void ApplyChanges();

		/* @brief Updates the framebuffer size based on the current window dimensions. */
		static void UpdateFramebufferSize();

		/**
		 * @brief Checks if a specific key is currently pressed.
		 * @param keyCode The SDL scancode of the key to check.
		 * @return True if the key is pressed, false otherwise.
		 */
		static bool IsKeyPressed(SDL_Scancode keyCode);

		/**
		 * @brief Sets the title of the window.
		 * @param title The new title for the window.
		 */
		static void SetTitle(const std::string &title);

		/**
		 * @brief Gets the SDL window pointer.
		 * @return The SDL window pointer.
		 */
		static SDL_Window *GetWindow();

		/**
		 * @brief Gets the raw handle of the window.
		 * @return The raw window handle.
		 */
		static void *GetRawHandle();

		/**
		 * @brief Gets the singleton instance of the Window class.
		 * @return The singleton instance of the Window class.
		 */
		static Window Get() { return m_Window; }

		/**
		 * @brief Gets the width of the window.
		 * @return The width of the window in pixels.
		 */
		static uint32_t GetWidth();

		/**
		 * @brief Gets the height of the window.
		 * @return The height of the window in pixels.
		 */
		static uint32_t GetHeight();

		/**
		 * @brief Gets the size of the window.
		 * @return A Vec2 representing the width and height of the window.
		 */
		static Vec2 GetWindowSize();

		/**
		 * @brief Checks if the window state is dirty.
		 * @return A Flag indicating the dirty state of the window.
		 */
		static Flag IsDirty();

		/**
		 * @brief Waits for events to be available before returning.
		 */
		static void WaitEvents();

		/**
		 * @brief Gets the time elapsed since the last frame.
		 * @return The delta time in seconds.
		 */
		static float GetDeltaTime();

		/**
		 * @brief Checks if the window should close.
		 * @return True if the window should close, false otherwise.
		 */
		static bool GetShouldClose();

		/**
		 * @brief Gets the scroll delta since the last frame.
		 * @return The scroll delta.
		 */
		static float GetDeltaScroll();

		/**
		 * @brief Gets the mouse delta since the last frame.
		 * @return The mouse delta as a Vec2.
		 */
		static Vec2 GetDeltaMouse();

		/**
		 * @brief Checks if the framebuffer has been resized.
		 * @return True if the framebuffer has been resized, false otherwise.
		 */
		static bool GetFramebufferResized();

		/**
		 * @brief Checks if a specific key is currently pressed.
		 * @param keyCode The SDL scancode of the key to check.
		 * @return True if the key is pressed, false otherwise.
		 */
		static bool IsKeyDown(SDL_Scancode keyCode);

		/**
		 * @brief Checks if a specific mouse button is currently pressed.
		 * @param buttonCode The SDL button code of the mouse button to check.
		 * @return True if the mouse button is pressed, false otherwise.
		 */
		static bool IsMouseDown(uint8_t buttonCode);

		/**
		 * @brief Sets the window mode.
		 * @param newMode The new window mode to set.
		 */
		static void SetMode(WindowMode newMode);

		/**
		 * @brief Sets whether the window should close.
		 * @param close True to close the window, false otherwise.
		 */
		static void SetShouldClose(bool close);

		/**
		 * @brief Gets and clears the paths of dropped files.
		 * @return A vector of strings representing the paths of dropped files.
		 */
		static std::vector<std::string> GetAndClearPaths();

		/**
		 * @brief Sets the event callback function.
		 * @param callback The callback function to set.
		 */
		static void SetEventCallback(const EventCallbackFn &callback);

		/* @brief Maximizes the window. */
		static void Maximize();

		/**
		 * @brief Checks if the window is visible.
		 * @return True if the window is visible, false otherwise.
		 */
		static bool IsVisible();

		/**
		 * @brief Checks if the window is maximized.
		 * @return True if the window is maximized, false otherwise.
		 */
		static bool IsMaximized();

		/* @brief Minimizes the window. */
		static void Minimize();

		/**
		 * @brief Checks if the window is minimized.
		 * @return True if the window is minimized, false otherwise.
		 */
		static void Show();

		/* @brief Hides the window. */
		static void Hide();

		/* @brief Focuses the window. */
		static void Focus();

		/**
		 * @brief Checks if the window is minimized.
		 * @return True if the window is minimized, false otherwise.
		 */
		static bool IsMinimized();

		/* @brief Centers the window on the screen. */
		static void CenterWindow();

		/**
		 * @brief Sets whether the window is resizable.
		 * @param value True to make the window resizable, false otherwise.
		 */
		static void SetResizable(bool value);

		/**
		 * @brief Sets whether the window is decorated.
		 * @param value True to make the window decorated, false otherwise.
		 */
		static void SetDecorated(bool value);

		/**
		 * @brief Gets the DPI scale of the window.
		 * @return The DPI scale.
		 */
		static float GetDpiScale();

		/**
		 * @brief Sets the height of the title bar.
		 * @param height The height of the title bar.
		 */
		static void SetTitleBarHeight(float height);

		/**
		 * @brief 
		 * @param width 
		 */
		static void SetTitleBarButtonWidth(float width);

		/**
		 * @brief Sets whether the title bar is hovered.
		 * @param hovered True if the title bar is hovered, false otherwise.
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
		 * @brief Handles an SDL event.
		 * @param event The SDL event to handle.
		 */
		static void HandleEvent(const SDL_Event &event);
	};
	
}

// -------------------------------------------------------


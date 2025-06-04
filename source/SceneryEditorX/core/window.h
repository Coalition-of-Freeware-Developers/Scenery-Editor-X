/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* window.h
* -------------------------------------------------------
* Created: 16/3/2025
* -------------------------------------------------------
*/
#pragma once
#include <chrono>
#include <GLFW/glfw3.h>
#include <GraphicsEngine/vulkan/render_data.h>
#include <SceneryEditorX/core/base.hpp>
#include <SceneryEditorX/core/window/icon.h>
#include <SceneryEditorX/core/pointers.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	
	enum class WindowMode : uint8_t
	{
		Windowed,
		WindowedFullScreen,
		FullScreen
	};

	/// -------------------------------------------------------
	
	struct WindowData
	{
	    GLOBAL inline GLFWwindow *window		= nullptr;
	    GLOBAL inline const char *title			= "Scenery Editor X";
	    GLOBAL inline int width					= 1280;
	    GLOBAL inline int height				= 720;
	    GLOBAL inline int posX					= 0;
	    GLOBAL inline int posY					= 30;
        GLOBAL inline bool framebufferResized   = false;
        GLOBAL inline bool dirty				= true;
        GLOBAL inline bool resizable			= true;
        GLOBAL inline bool decorated			= true;
        GLOBAL inline bool maximized			= true;
		GLOBAL inline float scroll				= .0f;
        GLOBAL inline float deltaScroll			= .0f;
        GLOBAL inline Vec2 mousePos				= Vec2(.0f, .0f);
        GLOBAL inline Vec2 deltaMousePos		= Vec2(.0f, .0f);
	};
	
	/// -------------------------------------------------------
	
	struct WindowCallbacks
	{
	    void (*scrollCallback)(GLFWwindow *window, double x, double y) = nullptr;
        void (*keyCallback)(GLFWwindow *window, int key, int scancode, int action, int mods) = nullptr;
        void (*mouseButtonCallback)(GLFWwindow *window, int button, int action, int mods) = nullptr;
        void (*cursorPosCallback)(GLFWwindow *window, double x, double y) = nullptr;
        void (*frameBufferSizeCallback)(GLFWwindow *window, int width, int height) = nullptr;
        void (*framebufferResizeCallback)(GLFWwindow *window, int width, int height) = nullptr;
	    void (*windowMaximizeCallback)(GLFWwindow *window, int maximize) = nullptr;
	    void (*windowChangePosCallback)(GLFWwindow *window, int x, int y) = nullptr;
        void (*mousePositionCallback)(GLFWwindow *window, double x, double y) = nullptr;
	    void (*windowDropCallback)(GLFWwindow *window, int count, const char *paths[]) = nullptr;
        void (*windowCloseCallback)(GLFWwindow *window) = nullptr;
        void (*windowFocusCallback)(GLFWwindow *window, int focused) = nullptr;
	};
	
	/// -------------------------------------------------------
	
	class Window : public RefCounted
	{
	public:
	    Window();
        virtual ~Window();
        RenderData GetRenderData() { return renderData; }
		IconData GetIconData() { return iconData; }

        GLOBAL void Update();
        GLOBAL std::string  VideoModeText(const GLFWvidmode &mode);
		GLOBAL void			OnImgui();
        GLOBAL void			ApplyChanges();
		GLOBAL void			UpdateFramebufferSize();
        GLOBAL bool			IsKeyPressed(uint16_t keyCode);
        GLOBAL void SetFramebufferResized(const bool resized)   { WindowData::framebufferResized = resized; }
	    GLOBAL void			SetTitle(const std::string& title)  { glfwSetWindowTitle(WindowData::window,title.c_str()); }
		GLOBAL void			WaitEvents()						{ glfwWaitEvents(); }

	    GLOBAL GLFWwindow  *GetWindow()                         { return WindowData::window; }
		GLOBAL uint32_t		GetWidth()                          { return WindowData::width; }
		GLOBAL uint32_t		GetHeight()                         { return WindowData::height; }

		GLOBAL bool			GetShouldClose()                    { return glfwWindowShouldClose(WindowData::window); }
		GLOBAL float		GetDeltaScroll()                    { return WindowData::deltaScroll; }
		GLOBAL float		GetDeltaTime()					    { return deltaTime; }
	    GLOBAL Vec2			GetDeltaMouse()                     { return WindowData::deltaMousePos; }
		GLOBAL bool			GetFramebufferResized()             { return WindowData::framebufferResized; }
		GLOBAL bool			IsKeyDown(uint16_t keyCode)         { return glfwGetKey(WindowData::window,keyCode); }
		GLOBAL bool			IsMouseDown(uint16_t buttonCode)    { return glfwGetMouseButton(WindowData::window,buttonCode); }
        GLOBAL bool         IsDirty()                           { return WindowData::dirty; }

	private:
        WindowCallbacks windowCallbacks;
        WindowData windowData;
        GLFWmonitor **mainMonitor;
        RenderData renderData;
        IconData iconData;
        bool captureMovement;
        bool mousePressed;
        bool initState;

        INTERNAL void DisableJoystickHandling();
        INTERNAL inline std::chrono::high_resolution_clock::time_point lastTime;
        INTERNAL inline std::vector<std::string> pathsDrop;
        INTERNAL inline float deltaTime = .0f;
        INTERNAL inline char lastKeyState[GLFW_KEY_LAST + 1];
        INTERNAL inline WindowMode mode = WindowMode::Windowed;

        INTERNAL void ScrollCallback(GLFWwindow *window, double x, double y);
        INTERNAL void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
        INTERNAL void MouseClickCallback(GLFWwindow *window, int button, int action, int mod);
        INTERNAL void MousePositionCallback(GLFWwindow *window, WindowData *data, double x, double y);
        INTERNAL void FramebufferResizeCallback(GLFWwindow *window, int width, int height);
        INTERNAL void WindowMaximizeCallback(GLFWwindow *window, int maximize);
        INTERNAL void WindowChangePosCallback(GLFWwindow *window, int x, int y);
        INTERNAL void WindowDropCallback(GLFWwindow *window, int count, const char *paths[]);
        INTERNAL void SetWindowIcon(GLFWwindow *window);
	};
	
} /// namespace SceneryEditorX

/// -------------------------------------------------------


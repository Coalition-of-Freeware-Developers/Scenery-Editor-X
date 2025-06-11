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
#include <SceneryEditorX/core/base.hpp>
#include <SceneryEditorX/core/pointers.h>
#include <SceneryEditorX/core/window/icon.h>
#include <SceneryEditorX/core/window/monitor_data.h>
#include <SceneryEditorX/renderer/render_context.h>
#include <SceneryEditorX/renderer/vulkan/render_data.h>
#include <SceneryEditorX/renderer/vulkan/vk_swapchain.h>

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
		GLOBAL inline WindowMode mode			= WindowMode::Windowed;
	    GLOBAL inline const char *title			= "Scenery Editor X";
	    GLOBAL inline uint32_t width			= 1280;
        GLOBAL inline uint32_t height			= 720;
	    GLOBAL inline int posX					= 0;
	    GLOBAL inline int posY					= 30;
        GLOBAL inline bool framebufferResized   = false;
        GLOBAL inline bool dirty				= true;
        GLOBAL inline bool resizable			= true;
        GLOBAL inline bool decorated			= true;
        GLOBAL inline bool maximized			= true;
		GLOBAL inline bool focused				= true;
		GLOBAL inline bool vsync				= false;
        GLOBAL inline bool startMaximized		= false;
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
        void (*windowIconifyCallback)(GLFWwindow *window, int iconified) = nullptr;
        void (*windowSizeCallback)(GLFWwindow *window, int width, int height) = nullptr;
        void (*charCallback)(GLFWwindow *window, unsigned int codepoint) = nullptr;
	};

	/// -------------------------------------------------------

    class SwapChain;

	/// -------------------------------------------------------

	class Window
	{
	public:
        Window(const WindowData &winData);
        virtual ~Window();

        virtual void Init();
        virtual Ref<RenderContext> GetRenderContext() { return renderContext; }
        virtual void Maximize();
        virtual void CenterWindow();
        virtual const std::string &GetTitle() const { static std::string titleStr = ToString(WindowData::title); return titleStr; }
        virtual SwapChain &GetSwapChain();
        virtual void SetResizable(bool resizable) const;
		virtual void ChangeWindowMode();
        virtual VkExtent2D GetSize(Window *windowPtr);

	    RenderData			GetRenderData()								{ return renderData; }
		IconData			GetIconData()								{ return iconData; }

        GLOBAL Window *Create(const WindowData &windowSpecs = WindowData());
	    GLOBAL GLFWwindow	*GetWindow()								{ return WindowData::window; }
	    GLOBAL uint32_t		GetWidth()									{ return WindowData::width; }
		GLOBAL uint32_t		GetHeight()									{ return WindowData::height; }

        GLOBAL void			Update();
        GLOBAL std::string  VideoModeText(const GLFWvidmode &mode);
		GLOBAL void			OnImgui();
        GLOBAL void			ApplyChanges();
		GLOBAL void			UpdateFramebufferSize();
	    GLOBAL void			SetFramebufferResized(const bool resized)   { WindowData::framebufferResized = resized; }
	    GLOBAL void			SetTitle(const std::string& title)			{ glfwSetWindowTitle(WindowData::window,title.c_str()); }
		GLOBAL void			WaitEvents()								{ glfwWaitEvents(); }

	    GLOBAL Vec2			GetDeltaMouse()								{ return WindowData::deltaMousePos; }
		GLOBAL bool			GetFramebufferResized()						{ return WindowData::framebufferResized; }
		GLOBAL bool			IsKeyDown(uint16_t keyCode)					{ return glfwGetKey(WindowData::window,keyCode); }
		GLOBAL bool			IsMouseDown(uint16_t buttonCode)			{ return glfwGetMouseButton(WindowData::window,buttonCode); }
        GLOBAL bool         IsDirty()									{ return WindowData::dirty; }
	    GLOBAL bool			IsKeyPressed(uint16_t keyCode);
	    GLOBAL bool			GetShouldClose()							{ return glfwWindowShouldClose(WindowData::window); }

	    GLOBAL float		GetDeltaScroll()							{ return WindowData::deltaScroll; }
		GLOBAL float		GetDeltaTime()								{ return deltaTime; }

	private:
        MonitorData **mainMonitor;
        IconData iconData;
        SwapChain *swapChain;
        RenderData renderData;
        WindowData windowSpecs;
        WindowCallbacks windowCallbacks;
        Ref<RenderContext> renderContext;

		GLFWcursor *ImGuiMouseCursors[9] = { nullptr };

		bool initState;
        bool mousePressed;
        bool captureMovement;

		virtual void Shutdown();
        virtual void SwapBuffers();
        virtual std::pair<float, float> GetWindowPos() const;

        INTERNAL inline std::chrono::high_resolution_clock::time_point lastTime;
        INTERNAL inline std::vector<std::string> pathsDrop;
        INTERNAL inline float deltaTime = .0f;
        INTERNAL inline char lastKeyState[GLFW_KEY_LAST + 1];
        INTERNAL inline WindowMode mode = WindowMode::Windowed;

        INTERNAL void SetWindowIcon(GLFWwindow *window);
        INTERNAL void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
        INTERNAL void ScrollCallback(GLFWwindow *window, double x, double y);
        INTERNAL void MouseClickCallback(GLFWwindow *window, int button, int action, int mod);
        INTERNAL void WindowDropCallback(GLFWwindow *window, int count, const char *paths[]);
	    INTERNAL void MousePositionCallback(GLFWwindow *window, double x, double y);
        INTERNAL void WindowMaximizeCallback(GLFWwindow *window, int maximize);
        INTERNAL void WindowChangePosCallback(GLFWwindow *window, int x, int y);
        INTERNAL void DisableJoystickHandling();
	    INTERNAL void FramebufferResizeCallback(GLFWwindow *window, int width, int height);
	};
	
} /// namespace SceneryEditorX

/// -------------------------------------------------------


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
#include <GLFW/glfw3.h>
#include "icon.h"
#include "monitor_data.h"
#include "SceneryEditorX/core/events/event_system.h"
#include "SceneryEditorX/renderer/render_context.h"
#include "SceneryEditorX/renderer/vulkan/vk_data.h"
#include "SceneryEditorX/renderer/vulkan/vk_includes.h"

// -------------------------------------------------------

namespace SceneryEditorX
{
    class Window;

    enum class WindowMode : uint8_t
	{
		Windowed,
		WindowedFullScreen,
		FullScreen
	};

	// -------------------------------------------------------

	struct WindowData
    {
        static inline GLFWwindow* window;
        int posX = 0;
        int posY = 30;
        WindowMode mode = WindowMode::Windowed;
        bool framebufferResized = false;
        bool dirty = true;
        bool resizable = true;
        bool decorated = true;
        bool maximized = true;
        bool focused = true;
        bool vsync = false;
        bool startMaximized = false;
        float scroll = .0f;
        float deltaScroll = .0f;
        Vec2 mousePos = Vec2(.0f, .0f);
        Vec2 deltaMousePos = Vec2(.0f, .0f);
    };

	// -------------------------------------------------------

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

	// -------------------------------------------------------

    class RenderContext;

    // -------------------------------------------------------

	class Window
	{
	public:
	    using EventCallbackFn = std::function<void(Event&)>;

        Window();
        Window(WindowData m_WinData);
        virtual ~Window();

        virtual void Init();
        virtual void Update();
        virtual void Maximize();
        virtual void CenterWindow();
        virtual void SetResizable(bool resizable) const;
        virtual const std::string &GetTitle() const { return m_WindowSpecs.m_title; }
        virtual void SetTitle(const std::string &title);

        GLFWwindow* GetWindow() const { return m_Window; }
	    uint32_t GetWidth()		const { return m_WindowSpecs.m_width; }
		uint32_t GetHeight()	const { return m_WindowSpecs.m_height; }

	    static void ProcessEvents();
		virtual void ChangeWindowMode();
        virtual void ApplyChanges();

        virtual VkExtent2D GetSize() const								{ return {m_WindowSpecs.m_width, m_WindowSpecs.m_height}; }
	    virtual Ref<RenderContext> GetRenderContext()                   { return m_RenderContext; }

	    RenderData			GetRenderData()								{ return m_RenderData; }
		IconData			GetIconData()								{ return m_IconData; }

        static Window*		Create(const WindowData &windowSpecs = WindowData());
        static std::string  VideoModeText(const GLFWvidmode &mode);

		void				UpdateFramebufferSize();
        void			    SetFramebufferResized(const bool resized)   { m_WinData.framebufferResized = resized; }
		void			    WaitEvents()								{ glfwWaitEvents(); }
	    Vec2			    GetDeltaMouse()								{ return m_WinData.deltaMousePos; }
        bool			    GetFramebufferResized()						{ return m_WinData.framebufferResized; }
		bool				IsKeyDown(uint16_t keyCode) const			{ return glfwGetKey(m_Window, keyCode); }
		bool				IsMouseDown(uint16_t buttonCode) const		{ return glfwGetMouseButton(m_Window, buttonCode); }
        bool				IsDirty()									{ return m_WinData.dirty; }
        bool				IsMinimized()								{ return glfwGetWindowAttrib(m_Window, GLFW_ICONIFIED) == GLFW_TRUE; }
	    bool				IsKeyPressed(uint16_t keyCode) const;
	    bool				GetShouldClose() const						{ return glfwWindowShouldClose(m_Window); }
	    static float		GetDeltaTime()								{ return m_DeltaTime; }

	private:
        GLFWwindow* m_Window = nullptr;
        IconData m_IconData;
        RenderData m_RenderData;
        WindowCallbacks m_WindowCallbacks;
        Ref<RenderContext> m_RenderContext;

		GLFWcursor *m_ImGuiMouseCursors[9] = { nullptr };
        WindowData m_WinData;

        struct WindowSpecs
		{
            std::string m_title;
            uint32_t m_width = 1280;
            uint32_t m_height = 720;
		};
		WindowSpecs m_WindowSpecs;

		bool m_InitState;
        bool m_MousePressed;
        bool m_CaptureMovement;

		virtual void Shutdown();
        virtual std::pair<float, float> GetWindowPos() const;

        static std::chrono::high_resolution_clock::time_point m_LastTime;
        static std::vector<std::string> m_PathsDrop;
        static float m_DeltaTime;
        static char m_LastKeyState[GLFW_KEY_LAST + 1];
        static WindowMode m_WindowMode;

        static void SetWindowIcon(GLFWwindow *window);
        static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
        static void ScrollCallback(GLFWwindow *window, double x, double y);
        static void MouseClickCallback(GLFWwindow *window, int button, int action, int mod);
        static void WindowDropCallback(GLFWwindow *window, int count, const char *paths[]);
	    static void MousePositionCallback(GLFWwindow *window, double x, double y);
        static void WindowMaximizeCallback(GLFWwindow *window, int maximize);
        static void WindowChangePosCallback(GLFWwindow *window, int x, int y);
        static void DisableJoystickHandling();
	    static void FramebufferResizeCallback(GLFWwindow *window, int width, int height);
	};

}

// -------------------------------------------------------


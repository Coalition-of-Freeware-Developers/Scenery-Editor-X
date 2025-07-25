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
#include <functional>
#include <GLFW/glfw3.h>
#include <SceneryEditorX/core/events/event_system.h>
#include <SceneryEditorX/core/window/icon.h>
#include <SceneryEditorX/core/window/monitor_data.h>
#include <SceneryEditorX/renderer/render_context.h>
#include <SceneryEditorX/renderer/vulkan/vk_data.h>
#include <vulkan/vulkan.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    class Window;

    enum class WindowMode : uint8_t
	{
		Windowed,
		WindowedFullScreen,
		FullScreen
	};

	/// -------------------------------------------------------

	struct WindowData
    {
        GLOBAL inline WindowMode mode = WindowMode::Windowed;
        GLOBAL Window &window;
        std::string title = "Scenery Editor X";
        uint32_t width = 1280;
        uint32_t height = 720;
        int posX = 0;
        int posY = 30;
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

    bool glfwGetKey(const Window & window, uint16_t key);
    bool glfwGetMouseButton(const Window & window, uint16_t uint16);
    bool glfwWindowShouldClose(const Window & window);

    /// -------------------------------------------------------

	class Window
	{
	public:
	    using EventCallbackFn = std::function<void(Event&)>;

        explicit Window(const WindowData &winData);
        virtual ~Window();

        virtual void Init();
        virtual void Update();
        virtual void Maximize();
        virtual void CenterWindow();
        virtual void SetResizable(bool resizable) const;

        virtual const std::string &GetTitle() const { return winData.title; }
        virtual void SetTitle(const std::string &title);

        GLOBAL Window GetWindow()	  { return WindowData::window; }
	    uint32_t GetWidth()		const { return m_winSpecs.width; }
		uint32_t GetHeight()	const { return m_winSpecs.height; }

	    INTERNAL void ProcessEvents();
		virtual void ChangeWindowMode();
        virtual void ApplyChanges();
        virtual VkExtent2D GetSize() const								{ return {m_winSpecs.width, m_winSpecs.height}; }
        virtual SwapChain &GetSwapChain();
	    virtual Ref<RenderContext> GetRenderContext() { return renderContext; }

	    RenderData			GetRenderData()								{ return renderData; }
		IconData			GetIconData()								{ return iconData; }

        GLOBAL Window	   *Create(const WindowData &windowSpecs = WindowData());
        GLOBAL std::string  VideoModeText(const GLFWvidmode &mode);
		void				UpdateFramebufferSize();
	    void				SetFramebufferResized(const bool resized)   { winData.framebufferResized = resized; }
		GLOBAL void			WaitEvents()								{ glfwWaitEvents(); }
	    Vec2				GetDeltaMouse()								{ return winData.deltaMousePos; }
		bool				GetFramebufferResized()						{ return winData.framebufferResized; }
		bool				IsKeyDown(uint16_t keyCode)					{ return glfwGetKey(WindowData::window,keyCode); }
		bool				IsMouseDown(uint16_t buttonCode)			{ return glfwGetMouseButton(WindowData::window,buttonCode); }
        bool				IsDirty()									{ return winData.dirty; }
	    bool				IsKeyPressed(uint16_t keyCode) const;
	    bool				GetShouldClose()							{ return glfwWindowShouldClose(WindowData::window); }
	    GLOBAL float		GetDeltaTime()								{ return deltaTime; }

	private:
        IconData iconData;
        SwapChain *swapChain;
        RenderData renderData;
        WindowCallbacks windowCallbacks;
        Ref<RenderContext> renderContext;

		GLFWcursor *ImGuiMouseCursors[9] = { nullptr };

        LOCAL WindowData winData;
        int leftAlt;

        struct WindowSpecs
		{
            std::string title;
            uint32_t width = 1280;
            uint32_t height = 720;
		};
		WindowSpecs m_winSpecs;

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

}

/// -------------------------------------------------------


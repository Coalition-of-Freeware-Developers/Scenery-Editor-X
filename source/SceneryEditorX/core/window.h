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
#include <filesystem>
#include <functional>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <SceneryEditorX/core/base.hpp>
#include <SceneryEditorX/core/events/events.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	enum class WindowMode
	{
		Windowed,
		WindowedFullScreen,
		FullScreen
	};

	struct WindowSpecification
	{
		std::string Title = "Scenery Editor X";
		uint32_t Width = 1280;
        uint32_t Height = 720;
		bool Decorated = true;
		bool Fullscreen = false;
		bool VSync = true;
		std::filesystem::path IconPath;
	};
	
	class Window : public RefCounted
	{
	public:
		using EventCallbackFn = std::function<void(Event&)>;
        Window(const WindowSpecification &specification);
        virtual ~Window() = default;

		virtual void Init() = 0;
		virtual void Shutdown() = 0;
		virtual void ProcessEvents() = 0;
		virtual void SwapBuffers() = 0;

		virtual inline uint32_t GetWidth()  {return Window::width;}
		virtual inline uint32_t GetHeight() {return Window::height;}

		inline void* GetNativeWindow() const { return window; }

		GLOBAL void Create();
		GLOBAL void Update();
		GLOBAL void OnImgui();
		GLOBAL void Destroy();
		GLOBAL void ApplyChanges();
		GLOBAL void UpdateFramebufferSize();
        GLOBAL bool IsKeyPressed(uint16_t keyCode);

		void setFramebufferResized(bool resized) { framebufferResized = resized; }

		virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
		virtual void SetVSync(bool enabled) = 0;
		virtual bool IsVSync() const = 0;
		virtual void SetResizable(bool resizable) const = 0;

		virtual void Maximize() = 0;
		virtual void CenterWindow() = 0;

		virtual const std::string& GetTitle() const = 0;
		virtual void SetTitle(const std::string& title) = 0;
        virtual std::pair<float, float> GetWindowPos() const;

		//TODO: Add Vulkan SwapChain
        //VulkanSwapChain &GetSwapChain();

		GLOBAL Ref<Window>          Create(const WindowSpecification &specification = WindowSpecification());
		GLOBAL inline GLFWwindow*   GetGLFWwindow()                     {return window;}
		GLOBAL inline bool          IsDirty()                           {return dirty;}
		GLOBAL inline void          WaitEvents()                        {glfwWaitEvents();}
		GLOBAL inline float         GetDeltaTime()                      {return deltaTime;}
		GLOBAL inline bool          GetShouldClose()                    {return glfwWindowShouldClose(window);}
		GLOBAL inline float         GetDeltaScroll()                    {return deltaScroll;}
		GLOBAL inline Vec2          GetDeltaMouse()                     {return deltaMousePos;}
		GLOBAL inline bool          GetFramebufferResized()             {return framebufferResized;}
		GLOBAL inline bool          IsKeyDown(uint16_t keyCode)         {return glfwGetKey(window,keyCode);}
		GLOBAL inline bool          IsMouseDown(uint16_t buttonCode)    {return glfwGetMouseButton(window,buttonCode);}
		GLOBAL inline void          SetTitle(const std::string& title)  {glfwSetWindowTitle(window,title.c_str());}

	private:
        WindowSpecification m_Specification;
        GLFWcursor *m_ImGuiMouseCursors[9] = {0};
		INTERNAL inline GLFWwindow *window              = nullptr;
		INTERNAL inline GLFWmonitor **monitors          = nullptr;
		INTERNAL inline const char *title               = "Scenery Editor X";
		INTERNAL inline int         width               = 1280;
		INTERNAL inline int         height              = 720;
		INTERNAL inline int         posX                = 0;
		INTERNAL inline int         posY                = 30;
		INTERNAL inline int         monitorIndex        = 0;
		INTERNAL inline int         monitorCount        = 0;
		INTERNAL inline int         videoModeIndex      = 0;
		INTERNAL inline bool        framebufferResized  = false;

		struct WindowData
		{
			std::string Title;
			uint32_t Width, Height;

			EventCallbackFn EventCallback;
		};

		WindowData m_Data;
        float m_LastFrameTime = 0.0f;

		//TODO: Add Vulkan SwapChain
        //VulkanSwapChain *m_SwapChain; 

		INTERNAL inline std::chrono::high_resolution_clock::time_point lastTime;
		INTERNAL inline std::vector<std::string> pathsDrop;
	
		INTERNAL inline float       deltaTime           = .0f;
		INTERNAL inline float       scroll              = .0f;
		INTERNAL inline float       deltaScroll         = .0f;
		INTERNAL inline Vec2        mousePos            = Vec2(.0f,.0f);
		INTERNAL inline Vec2        deltaMousePos       = Vec2(.0f,.0f);
	
		INTERNAL inline char        lastKeyState[GLFW_KEY_LAST + 1];
		INTERNAL inline WindowMode  mode                = WindowMode::Windowed;
		INTERNAL inline bool        dirty               = true;
		INTERNAL inline bool        resizable           = true;
		INTERNAL inline bool        decorated           = true;
		INTERNAL inline bool        maximized           = true;
	
		GLOBAL void ScrollCallback(GLFWwindow *window, double x, double y);
	    GLOBAL void FramebufferResizeCallback(GLFWwindow *window, int width, int height);
	    GLOBAL void WindowMaximizeCallback(GLFWwindow *window, int maximize);
	    GLOBAL void WindowChangePosCallback(GLFWwindow *window, int x, int y);
	    GLOBAL void WindowDropCallback(GLFWwindow *window, int count, const char *paths[]);
	
		INTERNAL void SetWindowIcon(GLFWwindow *window);

	};
	
	 // ---------------------------------------------------------
	
	
} // namespace SceneryEditorX

// ---------------------------------------------------------

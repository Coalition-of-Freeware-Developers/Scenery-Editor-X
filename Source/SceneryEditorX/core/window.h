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
#include <glm/glm.hpp>
#include <SceneryEditorX/core/base.hpp>

// -------------------------------------------------------

enum class WindowMode
{
	Windowed,
	WindowedFullScreen,
	FullScreen
};

class Window
{
private:
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

	INTERNAL void SetWindowIcon(GLFWwindow *window);
	INTERNAL void ScrollCallback(GLFWwindow* window,double x,double y);
	INTERNAL void FramebufferResizeCallback(GLFWwindow* window,int width,int height);
	INTERNAL void WindowMaximizeCallback(GLFWwindow* window,int maximized);
	INTERNAL void WindowChangePosCallback(GLFWwindow* window,int x,int y);
	INTERNAL void WindowDropCallback(GLFWwindow* window,int count,const char* paths[]);

public:

	GLOBAL void Create();
	GLOBAL void Update();
	GLOBAL void OnImgui();
	GLOBAL void Destroy();
	GLOBAL void ApplyChanges();
	GLOBAL void UpdateFramebufferSize();
	GLOBAL bool IsKeyPressed(uint16_t keyCode);

	int get_width();
	int get_height();

	GLOBAL inline GLFWwindow*   GetGLFWwindow()                     {return window;}
	static inline bool          IsDirty()                           {return dirty;}
	GLOBAL inline void          WaitEvents()                        {glfwWaitEvents();}
	GLOBAL inline uint32_t      GetWidth()                          {return Window::width;}
	GLOBAL inline uint32_t      GetHeight()                         {return Window::height;}
	GLOBAL inline float         GetDeltaTime()                      {return deltaTime;}
	GLOBAL inline bool          GetShouldClose()                    {return glfwWindowShouldClose(window);}
	GLOBAL inline float         GetDeltaScroll()                    {return deltaScroll;}
	GLOBAL inline Vec2          GetDeltaMouse()                     {return deltaMousePos;}
	GLOBAL inline bool          GetFramebufferResized()             {return framebufferResized;}
	GLOBAL inline bool          IsKeyDown(uint16_t keyCode)         {return glfwGetKey(window,keyCode);}
	GLOBAL inline bool          IsMouseDown(uint16_t buttonCode)    {return glfwGetMouseButton(window,buttonCode);}
	GLOBAL inline void          SetTitle(const std::string& title)  {glfwSetWindowTitle(window,title.c_str());}
};


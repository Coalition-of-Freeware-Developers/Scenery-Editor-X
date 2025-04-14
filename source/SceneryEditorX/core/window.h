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
#include <SceneryEditorX/renderer/render_data.h>

// -------------------------------------------------------

enum class WindowMode : uint8_t
{
	Windowed,
	WindowedFullScreen,
	FullScreen
};

// -------------------------------------------------------

struct IconData
{
    std::string path;                  // Path to the icon file
    int width;                         // Icon width
    int height;                        // Icon height
    int channels;                      // Number of channels
    std::vector<unsigned char> buffer; // Buffer for the icon data
    mutable unsigned char* pixels;     // Pixel data after loading

	IconData() :
    path(R"(..\..\assets\icon.png)"),
    width(0),
    height(0),
    channels(0),
    pixels(nullptr) {}

	~IconData() 
    {
        if (pixels) {
            stbi_image_free(pixels);
            pixels = nullptr;
        }
    }
};

// -------------------------------------------------------

class Window
{
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

	LOCAL void ScrollCallback(GLFWwindow *window, double x, double y);
    LOCAL void FramebufferResizeCallback(GLFWwindow *window, int width, int height);
    LOCAL void WindowMaximizeCallback(GLFWwindow *window, int maximize);
    LOCAL void WindowChangePosCallback(GLFWwindow *window, int x, int y);
    LOCAL void WindowDropCallback(GLFWwindow *window, int count, const char *paths[]);

	INTERNAL void SetWindowIcon(GLFWwindow *window);

public:
    Window();
    ~Window();

    SceneryEditorX::RenderData gfxData;

	GLOBAL void Update();
	//GLOBAL void OnImgui();
	GLOBAL void ApplyChanges();
	GLOBAL void UpdateFramebufferSize();
	GLOBAL bool IsKeyPressed(uint16_t keyCode);
    GLOBAL void SetFramebufferResized(bool resized) { framebufferResized = resized; }

    GLOBAL GLFWwindow*   GetWindow()                         {return window;}
	GLOBAL bool          IsDirty()                           {return dirty;}
	GLOBAL void          WaitEvents()                        {glfwWaitEvents();}
	GLOBAL uint32_t      GetWidth()                          {return width;}
	GLOBAL uint32_t      GetHeight()                         {return height;}
	GLOBAL float         GetDeltaTime()                      {return deltaTime;}
	GLOBAL bool          GetShouldClose()                    {return glfwWindowShouldClose(window);}
	GLOBAL float         GetDeltaScroll()                    {return deltaScroll;}
	GLOBAL Vec2          GetDeltaMouse()                     {return deltaMousePos;}
	GLOBAL bool          GetFramebufferResized()             {return framebufferResized;}
	GLOBAL bool          IsKeyDown(uint16_t keyCode)         {return glfwGetKey(window,keyCode);}
	GLOBAL bool          IsMouseDown(uint16_t buttonCode)    {return glfwGetMouseButton(window,buttonCode);}
	GLOBAL void          SetTitle(const std::string& title)  {glfwSetWindowTitle(window,title.c_str());}
};

// -------------------------------------------------------


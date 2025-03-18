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
#include <SceneryEditorX/core/base.hpp>
#include <string>

// -------------------------------------------------------

class Window;

enum class WindowMode
{
    Windowed,
    WindowedFullScreen,
    FullScreen
};

struct WindowProps
{
    std::string Title = "Scenery Editor X";
    GLOBAL inline int Width = 1280;
    GLOBAL inline int Height = 720;
    GLOBAL inline bool resizable = true;
    GLOBAL inline bool decorated = true;
    GLOBAL inline bool maximized = true;
    bool Fullscreen = false;
    bool VSync = false;
};

struct WindowBase : public WindowProps
{
    GLFWwindow* window = nullptr;

    //Window* window{nullptr};
};

class Window
{
public:
    Window() : Window(WindowProps().Title, WindowProps().Width, WindowProps().Height) {}
    Window(const std::string& Title, int Width, int Height);
    ~Window();

private:
    GLOBAL inline GLFWwindow* window = nullptr;
    GLOBAL inline GLFWmonitor** monitors = nullptr;
    GLOBAL inline int posX = 0;
    GLOBAL inline int posY = 30;
    GLOBAL inline int monitorIndex = 0;
    GLOBAL inline int monitorCount = 0;
    GLOBAL inline int videoModeIndex = 0;
    GLOBAL inline bool framebufferResized = false;

    GLOBAL inline std::chrono::high_resolution_clock::time_point lastTime;
    GLOBAL inline float deltaTime = .0f;

    GLOBAL inline float scroll = .0f;
    GLOBAL inline float deltaScroll = .0f;
    GLOBAL inline Vec2 mousePos = Vec2(.0f,.0f);
    GLOBAL inline Vec2 deltaMousePos = Vec2(.0f,.0f);

    GLOBAL inline char lastKeyState[GLFW_KEY_LAST + 1];

    GLOBAL inline WindowMode mode = WindowMode::Windowed;
    GLOBAL inline bool dirty = true;

    INTERNAL void WindowIcon();
    INTERNAL void ScrollCallback(GLFWwindow* window,double x,double y);
    INTERNAL void FramebufferResizeCallback(GLFWwindow* window,int width,int height);
    INTERNAL void WindowMaximizeCallback(GLFWwindow* window,int maximized);
    INTERNAL void WindowChangePosCallback(GLFWwindow* window,int x,int y);
    INTERNAL void WindowDropCallback(GLFWwindow* window,int count,const char* paths[]);

public:

    static void Create();
    static void Update();
    static void OnImgui();
    static void Destroy();
    static void ApplyChanges();
    static void UpdateFramebufferSize();
    static bool IsKeyPressed(uint16_t keyCode);

    int get_width();
	int get_height();

    static inline GLFWwindow* GetGLFWwindow()
    {
        HWND glfwGetWin32Window(GLFWwindow* window);
    }
    static inline void WaitEvents()
    {
        glfwWaitEvents();
    }
    static inline uint32_t GetWidth()
    {
        return Window::GetWindowProps().Width;
    }
    static inline uint32_t GetHeight()
    {
        return Window::GetWindowProps().Height;
    }
    static inline float GetDeltaTime()
    {
        return deltaTime;
    }
    static inline bool GetShouldClose()
    {
        return glfwWindowShouldClose(window);
    }
    static inline float GetDeltaScroll()
    {
        return deltaScroll;
    }
    static inline Vec2 GetDeltaMouse()
    {
        return deltaMousePos;
    }
    static inline bool GetFramebufferResized()
    {
        return framebufferResized;
    }
    static inline bool IsKeyDown(uint16_t keyCode)
    {
        return glfwGetKey(window,keyCode);
    }
    static inline bool IsMouseDown(uint16_t buttonCode)
    {
        return glfwGetMouseButton(window,buttonCode);
    }
    static inline void SetTitle(const std::string& title)
    {
        glfwSetWindowTitle(window,title.c_str());
    }
    static inline WindowProps& GetWindowProps()
    {
        GLOBAL WindowProps props;
        return props;
    }
};


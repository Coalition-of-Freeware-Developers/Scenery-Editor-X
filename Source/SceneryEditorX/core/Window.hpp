#pragma once

#include <core/Base.hpp>

#include <GLFW/glfw3.h>
#include <string>

enum class WindowMode
{
    Windowed,
    WindowedFullScreen,
    FullScreen
};

struct WindowProperties
{
    std::string Title = "Scenery Editor X";
    static inline uint32_t Width = 1280;
    static inline uint32_t Height = 720;
    bool Fullscreen = false;
    bool VSync = true;
};

class Window
{
private:
    static inline GLFWwindow *window = nullptr;
    static inline GLFWmonitor **monitors = nullptr;
    static inline std::string name = "Scenery Editor X";
    static inline int width = 1280;
    static inline int height = 720;
    static inline int posX = 0;
    static inline int posY = 30;
    static inline int monitorIndex = 0;
    static inline int monitorCount = 0;
    static inline int videoModeIndex = 0;
    static inline bool framebufferResized = false;

    static inline std::chrono::high_resolution_clock::time_point lastTime;
    static inline float deltaTime = .0f;

    static inline float scroll = .0f;
    static inline float deltaScroll = .0f;
    static inline glm::vec2 mousePos = glm::vec2(.0f, .0f);
    static inline glm::vec2 deltaMousePos = glm::vec2(.0f, .0f);

    static inline char lastKeyState[GLFW_KEY_LAST + 1];

    static inline WindowMode mode = WindowMode::Windowed;
    static inline bool dirty = true;
    static inline bool resizable = true;
    static inline bool decorated = true;
    static inline bool maximized = true;

    static void ScrollCallback(GLFWwindow *window, double x, double y);
    static void FramebufferResizeCallback(GLFWwindow *window, int width, int height);
    static void WindowMaximizeCallback(GLFWwindow *window, int maximized);
    static void WindowChangePosCallback(GLFWwindow *window, int x, int y);
    static void WindowDropCallback(GLFWwindow *window, int count, const char *paths[]);

public:
    static void Create();
    static void Update();
    static void OnImgui();
    static void Destroy();
    static void ApplyChanges();
    static void UpdateFramebufferSize();
    static bool IsKeyPressed(uint16_t keyCode);

    static inline GLFWwindow *GetGLFWwindow()
    {
        return window;
    }
    static inline void WaitEvents()
    {
        glfwWaitEvents();
    }
    static inline uint32_t GetWidth()
    {
        return width;
    }
    static inline uint32_t GetHeight()
    {
        return height;
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
    static inline glm::vec2 GetDeltaMouse()
    {
        return deltaMousePos;
    }
    static inline bool GetFramebufferResized()
    {
        return framebufferResized;
    }
    static inline bool IsKeyDown(uint16_t keyCode)
    {
        return glfwGetKey(window, keyCode);
    }
    static inline bool IsMouseDown(uint16_t buttonCode)
    {
        return glfwGetMouseButton(window, buttonCode);
    }
};

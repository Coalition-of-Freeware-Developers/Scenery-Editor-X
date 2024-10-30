#pragma once

#include "../src/core/Base.h"
#include "../src/xpeditorpch.h"

#include <chrono>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace SceneryEditorX
{
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

	class Window {
    private:
        //Window(const WindowProperties &properties);
        static inline bool framebufferResized = false;
        static inline GLFWwindow* window = nullptr;
        static inline GLFWmonitor** monitors = nullptr;
        static inline std::string name = WindowProperties().Title;
        static inline int width = WindowProperties::Width;
        static inline int height = WindowProperties::Height;
        static inline int posX = 0;
        static inline int posY = 30;
        static inline int monitorIndex = 0;
        static inline int monitorCount = 0;
        static inline int videoModeIndex = 0;

	private:
      
        static inline std::chrono::high_resolution_clock::time_point lastTime;
        static inline float deltaTime = .0f;

        /**
        * @brief A vector to store file paths dropped into the window.
        * 
        * This vector is used to keep track of file paths that are dragged and dropped
        * into the application window. The paths are stored as strings.
        */
        static inline std::vector<std::string> pathsDrop;

        static inline float scroll = .0f;
        static inline float deltaScroll = .0f;
        static inline glm::vec2 mousePos = glm::vec2(.0f, .0f);
        static inline glm::vec2 deltaMousePos = glm::vec2(.0f, .0f);
               
        static inline char lastKeyState[GLFW_KEY_LAST + 1];
        static inline WindowMode mode = WindowMode::Windowed;
        static inline bool dirty = true;
        static inline bool resizable = true;
        static inline bool maximized = true;
               
        static void ScrollCallback(GLFWwindow *window, double x, double y);
        static void FramebufferResizeCallback(GLFWwindow *window, int Width, int Height);
        static void WindowMaximizeCallback(GLFWwindow *window, int maximized);
        static void WindowChangePosCallback(GLFWwindow *window, int x, int y);
        static void WindowDropCallback(GLFWwindow *window, int count, const char *paths[]);

        //virtual void Shutdown();

    public:
        static  void Create();
        static  void Update();
        static  void OnImgui();
        static  void Destroy();
        static  void ApplyChanges();
        static  void UpdateFramebufferSize();
        virtual void SwapBuffers();
        static  bool IsKeyPressed(uint16_t keyCode);

        static  inline GLFWwindow *GetGLFWwindow() {return window;}
        using   EventCallbackFn = std::function<void()>;

        static  inline uint32_t GetWidth() {return WindowProperties::Width;}
        static  inline uint32_t GetHeight() {return WindowProperties::Height;}

        virtual void SetEventCallback(const EventCallbackFn &callback) = 0;
        virtual void SetVSync(bool enabled) = 0;
        virtual bool IsVSync() const = 0;
        static  inline bool GetShouldClose() {return glfwWindowShouldClose(window);}
        static  inline float GetDeltaScroll() {return deltaScroll;}
        static  inline glm::vec2 GetDeltaMouse() {return deltaMousePos;}
        static  inline bool GetFramebufferResized() {return framebufferResized;}
        static  inline bool IsKeyDown(uint16_t keyCode) {return glfwGetKey(window, keyCode);}
        static  inline bool IsMouseDown(uint16_t buttonCode) {return glfwGetMouseButton(window, buttonCode);}
        static  inline void SetMode(WindowMode newMode) {mode = newMode;}
        static  inline std::vector<std::string> GetAndClearPaths()
        {
            auto paths = pathsDrop;
            pathsDrop.clear();
            return paths;
        }

        //virtual void* GetNativeWindow() const = 0;

    //private:
    //    GLFWwindow *m_Window;
    //    GLFWcursor *m_ImGuiMouseCursors[9] = {0};
    //    WindowProperties m_Specification;
    //    struct WindowData
    //    {
    //        std::string Title;
    //        uint32_t Width, Height;
    //        EventCallbackFn EventCallback;
    //    };
    //    WindowData WinData;
	};
}

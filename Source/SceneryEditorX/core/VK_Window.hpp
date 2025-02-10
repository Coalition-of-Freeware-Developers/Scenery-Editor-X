#pragma once

#include <GLFW/glfw3.h>
#include <string>

namespace SceneryEditorX
{
    class VK_Window
    {
    public:
        VK_Window(int width, int height, std::string windowName);
        ~VK_Window();

        VK_Window(const VK_Window &) = delete;
        VK_Window &operator=(const VK_Window &) = delete;

        bool shouldClose() {return glfwWindowShouldClose(window);}

    private:

        void initWindow();

        const int width;
        const int height;

        std::string windowName;
        GLFWwindow *window;
    };
}

#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

namespace SceneryEditorX
{
    class SEDXwindow
    {
    public:
        SEDXwindow(int width, int height, std::string name);
        ~SEDXwindow();

    private:
        void initWindow();

        const int width;
        const int height;

        std::string windowTitle;
        GLFWwindow *window;
    };
} // namespace SceneryEditorX

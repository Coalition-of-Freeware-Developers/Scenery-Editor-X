#include "VK_Window.hpp"

namespace SceneryEditorX
{
    VK_Window::VK_Window(int width, int height, std::string windowName)
        : width(width), height(height), windowName(windowName)
    {
        initWindow();
    }
    
    VK_Window::~VK_Window()
    {
        glfwDestroyWindow(window);
        glfwTerminate();
    }
    
    void VK_Window::initWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    
        window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
    }
}

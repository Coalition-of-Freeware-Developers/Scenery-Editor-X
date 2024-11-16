#include "VK_Window.hpp"

namespace SceneryEditorX
{
    /**
     * @brief Constructs a VK_Window object with specified width, height, and window name.
     *        Initializes the window by calling initWindow().
     * 
     * @param width The width of the window.
     * @param height The height of the window.
     * @param windowName The name of the window.
     */
    VK_Window::VK_Window(int width, int height, std::string windowName)
        : width(width), height(height), windowName(windowName)
    {
        initWindow();
    }
    
    /**
     * @brief Destroys the VK_Window object.
     *        Cleans up the GLFW window and terminates GLFW.
     */
    VK_Window::~VK_Window()
    {
        glfwDestroyWindow(window);
        glfwTerminate();
    }
    
    /**
     * @brief Initializes the GLFW window with specified width, height, and window name.
     *        Sets the window hints for no API and non-resizable window.
     *        Creates the GLFW window.
     */
    void VK_Window::initWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
    }
}

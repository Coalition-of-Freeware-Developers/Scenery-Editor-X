#include "Application.hpp"

namespace SceneryEditorX
{
    void Application::run()
    {
        while (!VK_Window.shouldClose())
        {
            glfwPollEvents();
        }
    }
}

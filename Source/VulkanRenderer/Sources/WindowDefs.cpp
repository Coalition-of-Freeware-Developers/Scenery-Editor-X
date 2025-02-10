#include "WindowDefs.h"

namespace SceneryEditorX
{
    SEDXwindow::SEDXwindow(int w, int h, std::string name) : width(w), height(h), windowTitle(name)
    {
        initWindow();
    }

    SEDXwindow::~SEDXwindow()
    {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void SEDXwindow::initWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(width, height, windowTitle.c_str(), nullptr, nullptr);
    }

} // namespace SceneryEditorX

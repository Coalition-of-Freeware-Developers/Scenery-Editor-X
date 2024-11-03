#include "../src/xpeditorpch.h"
#include "Window.hpp"


/**
 * @brief Callback function for handling scroll events.
 * 
 * This function is called whenever a scroll event occurs in the window.
 * It updates the scroll and deltaScroll values based on the scroll input.
 * 
 * @param window The GLFW window where the event occurred.
 * @param x The scroll offset along the x-axis.
 * @param y The scroll offset along the y-axis.
 */
void Window::ScrollCallback(GLFWwindow *window, double x, double y)
{
    Window::scroll += y;
    Window::deltaScroll += y;
}

/**
 * @brief Callback function for handling framebuffer resize events.
 * 
 * This function is called whenever the framebuffer is resized.
 * It updates the width and height of the window and sets the framebufferResized flag to true.
 * 
 * @param window The GLFW window where the event occurred.
 * @param width The new width of the framebuffer.
 * @param height The new height of the framebuffer.
 */
void Window::FramebufferResizeCallback(GLFWwindow *window, int width, int height)
{
    Window::width = width;
    Window::height = height;
    Window::framebufferResized = true;
}

/**
 * @brief Callback function for handling window maximize events.
 * 
 * This function is called whenever the window is maximized or restored.
 * It updates the maximized state of the window.
 * 
 * @param window The GLFW window where the event occurred.
 * @param maximize The maximized state of the window (1 if maximized, 0 if restored).
 */
void Window::WindowMaximizeCallback(GLFWwindow *window, int maximize)
{
    maximized = maximize;
}

/**
 * @brief Callback function for handling window position change events.
 * 
 * This function is called whenever the window position is changed.
 * It updates the posX and posY values based on the new window position.
 * 
 * @param window The GLFW window where the event occurred.
 * @param x The new x-coordinate of the window.
 * @param y The new y-coordinate of the window.
 */
void Window::WindowChangePosCallback(GLFWwindow *window, int x, int y)
{
    Window::posX = x;
    Window::posY = y;
}

/**
 * @brief Creates a new window and initializes GLFW.
 * 
 * This function initializes the GLFW library, sets the necessary window hints,
 * retrieves the available monitors and video modes, creates a new window, and sets
 * the window position and various callback functions. It also applies any pending
 * changes to the window configuration.
 */
void Window::Create()
{
    SEDX_PROFILE_FUNC();
    // initializing glfw
    glfwInit();

    // glfw uses OpenGL context by default
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    monitors = glfwGetMonitors(&monitorCount); // get all monitors

    glfwGetVideoModes(monitors[monitorIndex], &videoModeIndex);
    videoModeIndex -= 1;

    window = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr); // create window
    glfwSetWindowPos(window, posX, posY);                                     // set window position

    glfwSetFramebufferSizeCallback(window, Window::FramebufferResizeCallback); // set framebuffer resize callback
    glfwSetScrollCallback(window, Window::ScrollCallback);
    glfwSetWindowMaximizeCallback(window, Window::WindowMaximizeCallback);
    glfwSetWindowPosCallback(window, Window::WindowChangePosCallback);

    dirty = false;
    Window::ApplyChanges();
}

/**
 * @brief Applies changes to the window configuration.
 * 
 * This function retrieves the available monitors and video modes, validates the video mode index,
 * and applies the necessary changes to the window based on the current mode (Windowed, Windowed FullScreen, or FullScreen).
 * It updates the window's position, size, and attributes accordingly.
 */
void Window::ApplyChanges()
{
    monitors = glfwGetMonitors(&monitorCount);
    // ASSERT(monitorIndex < monitorCount, "Invalid monitorIndex inside Window creation!");
    auto monitor = monitors[monitorIndex];
    auto monitorMode = glfwGetVideoMode(monitor);

    int modesCount;
    const GLFWvidmode *videoModes = glfwGetVideoModes(monitors[monitorIndex], &modesCount);
    if (videoModeIndex >= modesCount)
    {
        videoModeIndex = modesCount - 1;
    }

    // creating window
    switch (mode)
    {
    case Mode::Windowed:
        posY = std::max(posY, 31);
        glfwSetWindowMonitor(window, nullptr, posX, posY, width, height, GLFW_DONT_CARE);
        if (maximized)
        {
            glfwMaximizeWindow(window);
        }
        glfwSetWindowAttrib(window, GLFW_MAXIMIZED, maximized);
        glfwSetWindowAttrib(window, GLFW_RESIZABLE, resizable);
        glfwSetWindowAttrib(window, GLFW_DECORATED, decorated);
        break;
    case Mode::WindowedFullScreen:
        glfwWindowHint(GLFW_RED_BITS, monitorMode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, monitorMode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, monitorMode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, monitorMode->refreshRate);
        glfwSetWindowMonitor(window, monitor, 0, 0, monitorMode->width, monitorMode->height, monitorMode->refreshRate);
        break;
    case Mode::FullScreen:
        GLFWvidmode videoMode = videoModes[videoModeIndex];
        glfwSetWindowMonitor(window, monitor, 0, 0, videoMode.width, videoMode.height, videoMode.refreshRate);
        break;
    }

    framebufferResized = false;
    dirty = false;
}

/**
 * @brief Destroys the window and terminates GLFW.
 * 
 * This function retrieves the current window position, destroys the window,
 * and terminates the GLFW library.
 */
void Window::Destroy()
{
    glfwGetWindowPos(window, &posX, &posY);
    glfwDestroyWindow(window);
    glfwTerminate();
}

/**
 * @brief Updates the window state.
 * 
 * This function updates the state of the window, including the key states,
 * scroll delta, time delta, mouse position, and processes any pending events.
 * It captures the current time to calculate the time delta since the last update,
 * retrieves the current cursor position, and polls for any pending events.
 */
void Window::Update()
{
    for (int i = 0; i < GLFW_KEY_LAST + 1; i++)
    {
        lastKeyState[i] = glfwGetKey(window, i);
    }
    deltaScroll = 0;
    auto newTime = std::chrono::high_resolution_clock::now();
    deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(newTime - lastTime).count();
    deltaTime /= 1000.0f;
    lastTime = newTime;
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    deltaMousePos = mousePos - glm::vec2(x, y);
    mousePos = glm::vec2(x, y);
    glfwPollEvents();
}

/**
 * @brief Generates a string representation of a video mode.
 * 
 * This function takes a GLFWvidmode structure and returns a string
 * that describes the video mode in terms of its width, height, and refresh rate.
 * 
 * @param mode The GLFWvidmode structure containing the video mode information.
 * @return A string representing the video mode in the format "widthxheight refreshRate Hz".
 */
std::string VideoModeText(GLFWvidmode mode)
{
    return std::to_string(mode.width) + "x" + std::to_string(mode.height) + " " + std::to_string(mode.refreshRate) +
           " Hz";
}

/**
 * @brief Renders the ImGui interface for the window settings.
 * 
 * This function creates an ImGui interface for configuring the window settings.
 * It allows the user to change the window mode, monitor, resolution, and other
 * window attributes such as maximized, decorated, and resizable.
 */
void Window::OnImgui()
{
    const float totalWidth = ImGui::GetContentRegionAvail().x;
    if (ImGui::CollapsingHeader("Window"))
    {
        // mode
        {
            const char *modeNames[] = {"Windowed", "Windowed FullScreen", "FullScreen"};
            ImGui::Text("Mode");
            ImGui::SameLine(totalWidth / 2.0f);
            ImGui::SetNextItemWidth(totalWidth / 2.0f);
            ImGui::PushID("modeCombo");
            if (ImGui::BeginCombo("", modeNames[(int)mode]))
            {
                for (int i = 0; i < 3; i++)
                {
                    bool selected = (int)mode == i;
                    if (ImGui::Selectable(modeNames[i], selected))
                    {
                        mode = (Window::Mode)i;
                        dirty = true;
                    }
                    if (selected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopID();
        }
        if (mode != Mode::Windowed)
        {
            // monitor
            {
                ImGui::Text("Monitor");
                ImGui::SameLine(totalWidth / 2.0f);
                ImGui::SetNextItemWidth(totalWidth / 2.0f);
                ImGui::PushID("monitorCombo");
                if (ImGui::BeginCombo("", glfwGetMonitorName(monitors[monitorIndex])))
                {
                    for (int i = 0; i < monitorCount; i++)
                    {
                        bool selected = monitorIndex == i;
                        ImGui::PushID(i);
                        if (ImGui::Selectable(glfwGetMonitorName(monitors[i]), selected))
                        {
                            monitorIndex = i;
                            dirty = true;
                        }
                        if (selected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                        ImGui::PopID();
                    }
                    ImGui::EndCombo();
                }
                ImGui::PopID();
            }
        }
        // resolution
        {
            if (mode == Mode::FullScreen)
            {
                ImGui::Text("Resolution");
                ImGui::SameLine(totalWidth / 2.0f);
                ImGui::SetNextItemWidth(totalWidth / 4.0f);
                ImGui::PushID("monitorRes");
                int modesCount;
                const GLFWvidmode *videoModes = glfwGetVideoModes(monitors[monitorIndex], &modesCount);
                GLFWvidmode currMode = videoModes[videoModeIndex];
                std::string modeText = VideoModeText(currMode);
                if (ImGui::BeginCombo("", modeText.c_str()))
                {
                    for (int i = 0; i < modesCount; i++)
                    {
                        bool selected = videoModeIndex == i;
                        currMode = videoModes[i];
                        ImGui::PushID(i);
                        modeText = VideoModeText(currMode);
                        if (ImGui::Selectable(modeText.c_str(), selected))
                        {
                            videoModeIndex = i;
                            dirty = true;
                        }
                        if (selected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                        ImGui::PopID();
                    }
                    ImGui::EndCombo();
                }
                ImGui::PopID();
            }
        }
        // windowed only
        {
            if (mode == Mode::Windowed)
            {
                // maximized
                {
                    ImGui::Text("Maximized");
                    ImGui::SameLine(totalWidth / 2.0f);
                    ImGui::SetNextItemWidth(totalWidth / 2.0f);
                    ImGui::PushID("maximized");
                    if (ImGui::Checkbox("", &maximized))
                    {
                        dirty = true;
                    }
                    ImGui::PopID();
                }
                // decorated
                {
                    ImGui::Text("Decorated");
                    ImGui::SameLine(totalWidth / 2.0f);
                    ImGui::SetNextItemWidth(totalWidth / 2.0f);
                    ImGui::PushID("decorated");
                    if (ImGui::Checkbox("", &decorated))
                    {
                        dirty = true;
                    }
                    ImGui::PopID();
                }
                // resizable
                {
                    ImGui::Text("Resizable");
                    ImGui::SameLine(totalWidth / 2.0f);
                    ImGui::SetNextItemWidth(totalWidth / 2.0f);
                    ImGui::PushID("resizable");
                    if (ImGui::Checkbox("", &resizable))
                    {
                        dirty = true;
                    }
                    ImGui::PopID();
                }
            }
        }
    }
}

/**
 * @brief Updates the framebuffer size.
 * 
 * This function resets the framebufferResized flag to false and retrieves the current
 * framebuffer size, updating the width and height of the window accordingly.
 */
void Window::UpdateFramebufferSize()
{
    framebufferResized = false;
    glfwGetFramebufferSize(window, &width, &height);
}

/**
 * @brief Checks if a key is pressed.
 * 
 * This function checks if a specific key is currently pressed. It compares the last
 * recorded state of the key with the current state to determine if the key was pressed.
 * 
 * @param keyCode The key code of the key to check.
 * @return true if the key is pressed, false otherwise.
 */
bool Window::IsKeyPressed(uint16_t keyCode)
{
    return lastKeyState[keyCode] && !glfwGetKey(window, keyCode);
}


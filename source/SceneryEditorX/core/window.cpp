/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* window.cpp
* -------------------------------------------------------
* Created: 16/3/2025
* -------------------------------------------------------
*/

#include <SceneryEditorX/core/window.h>
#include <stb_image.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

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
	void Window::ScrollCallback(GLFWwindow* window,double x,double y)
	{
	    const Window *windowInstance = static_cast<Window *>(glfwGetWindowUserPointer(window));
	    auto instance = windowInstance;
        WindowData::scroll += x, WindowData::deltaScroll += y;
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
	    // Store the new width and height directly in the Window class
        WindowData::width = width;
        WindowData::height = height;
        WindowData::framebufferResized = true;
	
	    SEDX_CORE_INFO("Window framebuffer resized to: {}x{}", width, height);
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
	void Window::WindowMaximizeCallback(GLFWwindow* window, const int maximize)
	{
	    static_cast<Window *>(glfwGetWindowUserPointer(window));
        WindowData::maximized = maximize;
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
	void Window::WindowChangePosCallback(GLFWwindow* window,int x,int y)
	{
	    auto windowInstance = static_cast<Window *>(glfwGetWindowUserPointer(window));
        WindowData::posX = x, WindowData::posY = y;
	}
	
	/**
	 * @brief Callback function for handling window drop events.
	 *
	 * This function is called whenever files are dropped onto the window.
	 * It stores the paths of the dropped files in a vector.
	 *
	 * @param window The GLFW window where the event occurred.
	 * @param count The number of files dropped.
	 * @param paths An array of C-strings containing the paths of the dropped files.
	 */
	void Window::WindowDropCallback(GLFWwindow *window, const int count, const char *paths[])
	{
	    auto windowInstance = static_cast<Window *>(glfwGetWindowUserPointer(window));
	    for (auto i = 0; i < count; i++)
	    {
	        pathsDrop.emplace_back(paths[i]);
	    }
	}
	
	/**
	 * @brief Creates a new window and initializes GLFW.
	 *
	 * This function initializes the GLFW library, sets the necessary window hints,
	 * retrieves the available monitors and video modes, creates a new window, and sets
	 * the window position and various callback functions. It also applies any pending
	 * changes to the window configuration.
	 */
	Window::Window() : renderData()
	{
	    glfwInit();
		glfwSetErrorCallback([](int error, const char* description) {
			SEDX_CORE_ERROR_TAG("Window","GLFW Error ({0}): {1}", error, description);
		});
	    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	
	    WindowData::monitors = glfwGetMonitors(&WindowData::monitorCount); // get all monitors
	
	    glfwGetVideoModes(WindowData::monitors[WindowData::monitorIndex], &WindowData::videoModeIndex);
        WindowData::videoModeIndex -= 1;
	
	    WindowData::window = glfwCreateWindow(WindowData::width, WindowData::height, WindowData::title, nullptr, nullptr); // create window
	
	    if (!WindowData::window)
	    {
	        glfwTerminate();
	        SEDX_CORE_ERROR("Failed to create GLFW window!");
	        throw std::runtime_error("Failed to create GLFW window");
	    }
	
	    glfwSetWindowPos(WindowData::window, WindowData::posX, WindowData::posY); // set window position
        glfwSetWindowUserPointer(WindowData::window, this);                       // set user pointer to window instance
	    glfwSetFramebufferSizeCallback(WindowData::window, windowCallbacks.framebufferResizeCallback); // set framebuffer resize callback
        glfwSetScrollCallback(WindowData::window, windowCallbacks.scrollCallback);
        glfwSetWindowMaximizeCallback(WindowData::window, windowCallbacks.windowMaximizeCallback);
        glfwSetWindowPosCallback(WindowData::window, windowCallbacks.windowChangePosCallback);
        glfwSetDropCallback(WindowData::window, windowCallbacks.windowDropCallback);
	
	    SetWindowIcon(WindowData::window);
	
	    WindowData::dirty = false;
	    ApplyChanges();
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
        WindowData::monitors = glfwGetMonitors(&WindowData::monitorCount);
        SEDX_CORE_ASSERT(WindowData::monitorIndex < WindowData::monitorCount, "Invalid monitorIndex inside Window creation!");
        const auto monitor = WindowData::monitors[WindowData::monitorIndex];
		const auto monitorMode = glfwGetVideoMode(monitor);
	
		int modesCount;
        const GLFWvidmode *videoModes = glfwGetVideoModes(WindowData::monitors[WindowData::monitorIndex], &modesCount);
        if (WindowData::videoModeIndex >= modesCount)
		{
            WindowData::videoModeIndex = modesCount - 1;
		}
	
		// Window Creation
		switch (mode)
		{
			case WindowMode::Windowed:
				WindowData::posY = std::max(WindowData::posY, 31);
				glfwSetWindowMonitor(WindowData::window, nullptr, WindowData::posX, WindowData::posY, WindowData::width, WindowData::height, GLFW_DONT_CARE);
				if (WindowData::maximized)
				{
				    glfwMaximizeWindow(WindowData::window);
				}
				glfwSetWindowAttrib(WindowData::window,GLFW_MAXIMIZED,WindowData::maximized);
				glfwSetWindowAttrib(WindowData::window,GLFW_RESIZABLE,WindowData::resizable);
				glfwSetWindowAttrib(WindowData::window,GLFW_DECORATED,WindowData::decorated);
				break;
			case WindowMode::WindowedFullScreen:
				glfwWindowHint(GLFW_RED_BITS,monitorMode->redBits);
				glfwWindowHint(GLFW_GREEN_BITS,monitorMode->greenBits);
				glfwWindowHint(GLFW_BLUE_BITS,monitorMode->blueBits);
				glfwWindowHint(GLFW_REFRESH_RATE,monitorMode->refreshRate);
				glfwSetWindowMonitor(WindowData::window,monitor,0,0,monitorMode->width,monitorMode->height,monitorMode->refreshRate);
				break;
			case WindowMode::FullScreen:
				GLFWvidmode videoMode = videoModes[WindowData::videoModeIndex];
				glfwSetWindowMonitor(WindowData::window,monitor,0,0,videoMode.width,videoMode.height,videoMode.refreshRate);
				break;
		}
	
		WindowData::framebufferResized = false;
		WindowData::dirty = false;
	}
	
	/**
	 * @brief Destroys the window and terminates GLFW.
	 *
	 * This function retrieves the current window position, destroys the window,
	 * and terminates the GLFW library.
	 */
	Window::~Window()
	{
        glfwGetWindowPos(WindowData::window, &WindowData::posX, &WindowData::posY);
        glfwDestroyWindow(WindowData::window);
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
		for (auto i = 0; i < GLFW_KEY_LAST + 1; i++)
		{
            lastKeyState[i] = glfwGetKey(WindowData::window, i);
		}

        WindowData::deltaScroll = 0;
		auto newTime = std::chrono::high_resolution_clock::now();
		deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(newTime - lastTime).count();
		deltaTime /= 1000.0f;
		lastTime = newTime;
		double x,y;
        glfwGetCursorPos(WindowData::window, &x, &y);
        WindowData::deltaMousePos = WindowData::mousePos - Vec2(x, y);
        WindowData::mousePos = Vec2(x, y);
		glfwPollEvents();
	}
	
	/**
	 * @brief Generates a string representation of a video mode.
	 *
	 * This function takes a GLFWvidmode structure and returns a string
	 * that describes the video mode in terms of its width, height, and refresh rate.
	 *
	 * @param mode The GLFWvidmode structure containing the video mode information.
	 * @return A string representing the video mode in the format "width x height refreshRate Hz".
	 */
    std::string Window::VideoModeText(const GLFWvidmode &mode)
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
				const char* modeNames[] = {"Windowed", "Windowed FullScreen", "FullScreen"};
				ImGui::Text("Mode");
				ImGui::SameLine(totalWidth / 2.0f);
				ImGui::SetNextItemWidth(totalWidth / 2.0f);
				ImGui::PushID("modeCombo");
				if (ImGui::BeginCombo("",modeNames[(int)mode]))
				{
					for (int i = 0; i < 3; i++)
					{
						bool selected = (int)mode == i;
						if (ImGui::Selectable(modeNames[i],selected))
						{
							mode = (WindowMode)i;
                            WindowData::dirty = true;
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
			if (mode != WindowMode::Windowed)
			{
				// monitor
				{
					ImGui::Text("Monitor");
					ImGui::SameLine(totalWidth / 2.0f);
					ImGui::SetNextItemWidth(totalWidth / 2.0f);
					ImGui::PushID("monitorCombo");
                    if (ImGui::BeginCombo("", glfwGetMonitorName(WindowData::monitors[WindowData::monitorIndex])))
					{
                        for (int i = 0; i < WindowData::monitorCount; i++)
						{
                            bool selected = WindowData::monitorIndex == i;
							ImGui::PushID(i);
                            if (ImGui::Selectable(glfwGetMonitorName(WindowData::monitors[i]), selected))
							{
								WindowData::monitorIndex = i;
								WindowData::dirty = true;
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
				if (mode == WindowMode::FullScreen)
				{
					ImGui::Text("Resolution");
					ImGui::SameLine(totalWidth / 2.0f);
					ImGui::SetNextItemWidth(totalWidth / 4.0f);
					ImGui::PushID("monitorRes");
					int modesCount;
                    const GLFWvidmode *videoModes = glfwGetVideoModes(WindowData::monitors[WindowData::monitorIndex], &modesCount);
                    GLFWvidmode currMode = videoModes[WindowData::videoModeIndex];
					std::string modeText = VideoModeText(currMode);
					if (ImGui::BeginCombo("",modeText.c_str()))
					{
						for (int i = 0; i < modesCount; i++)
						{
                            bool selected = WindowData::videoModeIndex == i;
							currMode = videoModes[i];
							ImGui::PushID(i);
							modeText = VideoModeText(currMode);
							if (ImGui::Selectable(modeText.c_str(),selected))
							{
								WindowData::videoModeIndex = i;
								WindowData::dirty = true;
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
				if (mode == WindowMode::Windowed)
				{
					// maximized
					{
						ImGui::Text("Maximized");
						ImGui::SameLine(totalWidth / 2.0f);
						ImGui::SetNextItemWidth(totalWidth / 2.0f);
						ImGui::PushID("maximized");
                        if (ImGui::Checkbox("", &WindowData::maximized))
						{
                            WindowData::dirty = true;
						}
						ImGui::PopID();
					}
					// decorated
					{
						ImGui::Text("Decorated");
						ImGui::SameLine(totalWidth / 2.0f);
						ImGui::SetNextItemWidth(totalWidth / 2.0f);
						ImGui::PushID("decorated");
                        if (ImGui::Checkbox("", &WindowData::decorated))
						{
                            WindowData::dirty = true;
						}
						ImGui::PopID();
					}
					// resizable
					{
						ImGui::Text("Resizable");
						ImGui::SameLine(totalWidth / 2.0f);
						ImGui::SetNextItemWidth(totalWidth / 2.0f);
						ImGui::PushID("resizable");
                        if (ImGui::Checkbox("", &WindowData::resizable))
						{
                            WindowData::dirty = true;
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
        WindowData::framebufferResized = false;
        glfwGetFramebufferSize(WindowData::window, &WindowData::width, &WindowData::height);
	}
	
	/**
	 * @brief Gets the window icon image.
	 * 
	 */
	void Window::SetWindowIcon(GLFWwindow *window)
	{
	
	    IconData iconData;
	
	    std::ifstream file(iconData.path, std::ios::binary | std::ios::ate);
	    if (!file.is_open())
	    {
	        SEDX_CORE_ERROR("Failed to open icon file!");
	        return;
	    }
	
	    // Get file size and read the data
	    const std::streamsize size = file.tellg();
	    file.seekg(0, std::ios::beg);
	
	    iconData.buffer.resize(size);
	    if (!file.read(reinterpret_cast<char *>(iconData.buffer.data()), size))
	    {
	        SEDX_CORE_ERROR("Failed to read icon file data!");
	        return;
	    }
	
	    iconData.pixels = stbi_load_from_memory(iconData.buffer.data(),
	                                            static_cast<int>(size),
	                                            &iconData.width,
	                                            &iconData.height,
	                                            &iconData.channels,
	                                            4);
	
	    if (iconData.pixels)
	    {
	        const GLFWimage icon = {iconData.width, iconData.height, iconData.pixels};
	        glfwSetWindowIcon(window, 1, &icon);
	        stbi_image_free(iconData.pixels);
	        iconData.pixels = nullptr;
	    }
	    else
	    {
	        SEDX_CORE_ERROR("Failed to load window icon!");
	    }
	}
	
	/**
	 * @brief Checks if a key is pressed.
	 *
	 * This function checks if a specific key is currently pressed. It compares the last
	 * recorded state of the key with the current state to determine if the key was pressed.
	 *
	 * @param keyCode The key code of the key to check.
	 * @return True if the key is pressed, false otherwise.
	 */
	bool Window::IsKeyPressed(const uint16_t keyCode)
    {
        return lastKeyState[keyCode] && !glfwGetKey(WindowData::window, keyCode);
	}
	
} // namespace SceneryEditorX

// -------------------------------------------------------

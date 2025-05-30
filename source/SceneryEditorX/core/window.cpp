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
#include <imgui/imgui.h>
#include <SceneryEditorX/core/window.h>
#include <SceneryEditorX/core/window/icon.h>
#include <SceneryEditorX/utils/monitor_data.h>
#include <stb_image.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
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
	    glfwSetErrorCallback([](int error, const char *description)
		{
	        /// Filter out joystick-related errors (codes around 65539 GLFW_INVALID_ENUM)
	        if (error == 0x10003 && strstr(description, "joystick"))
			{
	            /// Silently ignore joystick-related GLFW_INVALID_ENUM errors
	            return;
	        }

	        SEDX_CORE_ERROR_TAG("Window", "GLFW Error ({0}): {1}", error, description);
	    });

	    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	    
	    /// Initialize GLFW with joystick hat buttons disabled to reduce errors
	    glfwInitHint(GLFW_JOYSTICK_HAT_BUTTONS, GLFW_FALSE);

	    /// Initialize monitor information
	    MonitorInfo::RefreshMonitors();
	    
	    /// Get available video modes for the current monitor
	    int modeCount = 0;
	    MonitorInfo::GetVideoModes(MonitorInfo::GetCurrentMonitorIndex(), &modeCount);
	    MonitorInfo::SetVideoModeIndex(modeCount > 0 ? modeCount - 1 : 0);

		mainMonitor = MonitorInfo::GetPriMonitor();
		if (!mainMonitor)
		{
			SEDX_CORE_ERROR("No monitors found! Cannot create window.");
			return;
        }

		////////////////////////////////////////////////////////////////
		/// GLFW Application Window Creation
        ////////////////////////////////////////////////////////////////
	    WindowData::window = glfwCreateWindow(WindowData::width, WindowData::height, WindowData::title, *mainMonitor, nullptr);
	
	    if (!WindowData::window)
	    {
	        glfwTerminate();
	        SEDX_CORE_ERROR("Failed to create GLFW window!");
	    }
	
	    glfwSetFramebufferSizeCallback(WindowData::window, windowCallbacks.framebufferResizeCallback);
	    glfwSetWindowPos(WindowData::window, WindowData::posX, WindowData::posY);
	    glfwSetWindowUserPointer(WindowData::window, this);
	
	    glfwSetCursorPosCallback(WindowData::window, windowCallbacks.cursorPosCallback);
	    glfwSetKeyCallback(WindowData::window, windowCallbacks.keyCallback);
	    glfwSetMouseButtonCallback(WindowData::window, windowCallbacks.mouseButtonCallback);
	    glfwSetScrollCallback(WindowData::window, windowCallbacks.scrollCallback);
	
	    glfwSetWindowMaximizeCallback(WindowData::window, windowCallbacks.windowMaximizeCallback);
	    glfwSetWindowPosCallback(WindowData::window, windowCallbacks.windowChangePosCallback);
	    glfwSetDropCallback(WindowData::window, windowCallbacks.windowDropCallback);
	
	    SetWindowIcon(WindowData::window);
	
	    WindowData::dirty = false;
	    ApplyChanges();
	    
	    /// Disable joystick detection to avoid errors with flight sim hardware
	    DisableJoystickHandling();
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
     * @brief Disables joystick handling to avoid errors with flight simulator hardware.
     * 
     * This function disables all joystick-related callbacks and event processing
     * to prevent GLFW errors when dealing with complex flight simulator controllers.
     */
    void Window::DisableJoystickHandling()
    {
        /// Detach any registered joystick callback
        glfwSetJoystickCallback(nullptr);
        
        SEDX_CORE_INFO_TAG("Window", "Joystick handling disabled to prevent conflicts with flight simulator hardware");
    }

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
        auto *windowInstance = static_cast<Window *>(glfwGetWindowUserPointer(window));
        if (windowInstance->windowCallbacks.scrollCallback)
        {
            //windowInstance->cameraMovement.zoomValue = 4.0f * (float)y;
            WindowData::dirty = true;
        }

        //WindowData::scroll += x, WindowData::deltaScroll += y;
	}
    /**
	 * @brief Callback function for handling mouse button events.
	 *
	 * This function is called whenever a mouse button is pressed or released.
	 * It updates the captureMovement flag based on the button state.
	 *
	 * @param window The GLFW window where the event occurred.
	 * @param button The mouse button that was pressed or released.
	 * @param action The action (press, release).
	 * @param mods The modifier keys (Shift, Control, Alt).
	 */
    void Window::MouseClickCallback(GLFWwindow *window, int button, int action, int mods)
    {
        auto windowInstance = static_cast<Window *>(glfwGetWindowUserPointer(window));
        if (windowInstance->captureMovement)
        {
            GLFWcursor *hand = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
            GLFWcursor *cursor = glfwCreateStandardCursor(GLFW_CURSOR_NORMAL);

			if (button == GLFW_MOUSE_BUTTON_RIGHT)
            {
                if (action == GLFW_PRESS)
                {
                    windowInstance->mousePressed = true;
                    glfwSetCursor(GetWindow(), hand);
                }
                else if (action == GLFW_RELEASE)
                {
                    windowInstance->mousePressed = false;
                    glfwSetCursor(GetWindow(), cursor);
                }
            }
        }
    }

    /**
     * @brief Callback function for handling mouse position/movement events.
     *
     * This function is called whenever the mouse cursor position changes within the window.
     * When movement capture is enabled, it tracks the mouse position and calculates position
     * deltas for camera or other movement controls. The function handles initialization of
     * tracking on first call and applies sensitivity scaling to the movement offsets.
     *
     * @param window The GLFW window where the event occurred.
     * @param data Pointer to the WindowData structure containing window state information.
     * @param x The new x-coordinate of the cursor relative to the window.
     * @param y The new y-coordinate of the cursor relative to the window.
     */
    void Window::MousePositionCallback(GLFWwindow *window, WindowData *data, double x, double y)
    {
        auto windowInstance = static_cast<Window *>(glfwGetWindowUserPointer(window));
        if (windowInstance->captureMovement)
        {
            WindowData::mousePos.x = static_cast<float>(x);
            WindowData::mousePos.y = static_cast<float>(y);

			auto pointerX = (float)x;
            auto pointerY = (float)y;
            if (windowInstance->initState)
            {
                WindowData::deltaMousePos.x = pointerX;
                WindowData::deltaMousePos.y = pointerY;
                windowInstance->initState = false;
            }

			float xOffset = x - pointerX - WindowData::deltaMousePos.x;
            float yOffset = WindowData::deltaMousePos.y - pointerY; /// Invert the sign here

			WindowData::deltaMousePos.x = pointerX;
            WindowData::deltaMousePos.y = pointerY;

			xOffset *= 0.01;
            yOffset *= 0.01;

			//TODO: Finish rest of mouse position callback when camera is implemented
        }
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
	    /// Store the new width and height directly in the Window class
        auto windowInstance = static_cast<Window *>(glfwGetWindowUserPointer(window));
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
	    auto windowInstance = static_cast<Window *>(glfwGetWindowUserPointer(window));
        WindowData::maximized = maximize;
	}

	/**
	 * @brief Callback function for handling key events.
	 *
	 * This function is called whenever a key is pressed or released.
	 * It updates the key state and captures movement if necessary.
	 *
	 * @param window The GLFW window where the event occurred.
	 * @param key The key that was pressed or released.
	 * @param scancode The scancode of the key.
	 * @param action The action (press, release, repeat).
	 * @param mods The modifier keys (Shift, Control, Alt).
	 */
    void Window::KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
    {
        auto windowInstance = static_cast<Window *>(glfwGetWindowUserPointer(window));
        if (windowInstance->captureMovement)
        {
            WindowData::dirty = true;

			const float movementSpeed = 2.5f;

			//if (key == GLFW_KEY_LEFT_ALT && action == GLFW_PRESS)
            //    windowInstance->leftAlt = true;
            //if (key == GLFW_KEY_LEFT_ALT && action == GLFW_RELEASE)
            //    windowInstance->leftAlt = false;

            //windowInstance->windowCallbacks.keyCallback(window, key, scancode, action, mods);

        }
        
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
	void Window::WindowChangePosCallback(GLFWwindow* window, const int x, const int y)
	{
	    auto windowInstance = static_cast<Window *>(glfwGetWindowUserPointer(window));
        WindowData::posX = x;
        WindowData::posY = y;
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
	 * @brief Applies changes to the window configuration.
	 *
	 * This function retrieves the available monitors and video modes, validates the video mode index,
	 * and applies the necessary changes to the window based on the current mode (Windowed, Windowed FullScreen, or FullScreen).
	 * It updates the window's position, size, and attributes accordingly.
	 */
	void Window::ApplyChanges()
	{
        /// Refresh monitor information
        MonitorInfo::RefreshMonitors();
        
        /// Get the current monitor
        const int monitorIndex = MonitorInfo::GetCurrentMonitorIndex();
        SEDX_CORE_ASSERT(monitorIndex < MonitorInfo::GetMonitorCount(), "Invalid monitorIndex inside Window creation!");
        
        const auto monitor = MonitorInfo::GetPriMonitor()[monitorIndex];
		const auto monitorMode = glfwGetVideoMode(monitor);
	
		/// Get video modes for the current monitor
		int modesCount;
        const GLFWvidmode *videoModes = MonitorInfo::GetVideoModes(monitorIndex, &modesCount);
        
        /// Validate video mode index
        int videoModeIndex = MonitorInfo::GetVideoModeIndex();
        if (videoModeIndex >= modesCount)
		{
            videoModeIndex = modesCount - 1;
            MonitorInfo::SetVideoModeIndex(videoModeIndex);
		}
	
		/// Window Creation
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
				GLFWvidmode videoMode = videoModes[videoModeIndex];
				glfwSetWindowMonitor(WindowData::window,monitor,0,0,videoMode.width,videoMode.height,videoMode.refreshRate);
				break;
		}
	
		WindowData::framebufferResized = false;
		WindowData::dirty = false;
	}

	/**
	 * @brief Updates the window state for the current frame.
	 * 
	 * This method is called once per frame to:
	 * 1. Update the key state tracking buffer for all keyboard keys
	 * 2. Reset the delta scroll value for the current frame
	 * 3. Calculate the delta time between frames in milliseconds
	 * 4. Update the current and delta mouse position values
	 * 5. Process window events via glfwPollEvents()
	 * 
	 * The delta time calculation converts microseconds to milliseconds for consistent
	 * timing across different hardware. Mouse position is tracked to calculate movement
	 * delta between frames for camera or UI interaction.
	 * 
	 * This method should be called at the beginning of each frame's update cycle.
	 */
	void Window::Update()
	{
        for (auto i = 0; i < GLFW_KEY_LAST + 1; i++)
        {
            lastKeyState[i] = static_cast<char>(glfwGetKey(WindowData::window, i));
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
		return std::to_string(mode.width) + "x" + std::to_string(mode.height) + " " + std::to_string(mode.refreshRate) + " Hz";
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
			/// mode
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
				/// monitor
				{
					ImGui::Text("Monitor");
					ImGui::SameLine(totalWidth / 2.0f);
					ImGui::SetNextItemWidth(totalWidth / 2.0f);
					ImGui::PushID("monitorCombo");
					
					/// Get current monitor name
					int currentMonitorIndex = MonitorInfo::GetCurrentMonitorIndex();
					const char* currentMonitorName = glfwGetMonitorName(MonitorInfo::GetPriMonitor()[currentMonitorIndex]);
					
                    if (ImGui::BeginCombo("", currentMonitorName))
					{
                        for (int i = 0; i < MonitorInfo::GetMonitorCount(); i++)
						{
                            bool selected = currentMonitorIndex == i;
							ImGui::PushID(i);
                            if (ImGui::Selectable(glfwGetMonitorName(MonitorInfo::GetPriMonitor()[i]), selected))
							{
								MonitorInfo::SetCurrentMonitorIndex(i);
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
			/// resolution
			{
				if (mode == WindowMode::FullScreen)
				{
					ImGui::Text("Resolution");
					ImGui::SameLine(totalWidth / 2.0f);
					ImGui::SetNextItemWidth(totalWidth / 4.0f);
					ImGui::PushID("monitorRes");
					
					/// Get video modes for current monitor
					int modesCount;
                    const GLFWvidmode *videoModes = MonitorInfo::GetVideoModes(
                        MonitorInfo::GetCurrentMonitorIndex(), &modesCount);
                    
                    /// Get current video mode
                    int videoModeIndex = MonitorInfo::GetVideoModeIndex();
                    if (videoModeIndex >= modesCount) {
                        videoModeIndex = modesCount - 1;
                        MonitorInfo::SetVideoModeIndex(videoModeIndex);
                    }
                    
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
								MonitorInfo::SetVideoModeIndex(i);
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

			/// windowed only
			{
				if (mode == WindowMode::Windowed)
				{
					/// maximized
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
					/// decorated
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
					/// resizable
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
	 * @brief Sets the application window icon using a PNG image file.
	 * 
	 * This method loads an icon from the predefined path in IconData and sets it as the
	 * window's icon using GLFW. The process involves:
	 * 1. Opening the icon file in binary mode
	 * 2. Reading the file data into a memory buffer
	 * 3. Using stb_image to decode the PNG data into RGBA pixel data
	 * 4. Creating a GLFWimage structure with the decoded pixel data
	 * 5. Setting the window icon using glfwSetWindowIcon
	 * 6. Properly freeing the pixel data to prevent memory leaks
	 * 
	 * The method includes error handling for file operations and image decoding.
	 * If any step fails, appropriate error messages are logged and the function returns
	 * without setting the icon.
	 * 
	 * @param window The GLFW window handle for which to set the icon
	 * 
	 * @note The icon file path is defined in the IconData structure, typically pointing
	 *       to "..\\..\\assets\\icon.png"
	 * @note This implementation uses stb_image for PNG decoding and requires it to be included
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
	
	    /// Get file size and read the data
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
            SEDX_CORE_ERROR("Failed to load window icon!");
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

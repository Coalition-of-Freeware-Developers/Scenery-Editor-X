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
#include <SceneryEditorX/core/events/application_events.h>
#include <SceneryEditorX/core/events/key_events.h>
#include <SceneryEditorX/core/events/mouse_events.h>
#include <SceneryEditorX/core/input/input.h>
#include <SceneryEditorX/core/window.h>
#include <stb_image.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	// -----------------------------------------------------------

    static bool s_GLFWInitialized = false;

	static void GLFWErrorCallback(int error, const char *description)
    {
        SEDX_CORE_ERROR_TAG("GLFW", "GLFW Error ({0}): {1}", error, description);
    }

    Window::Window(const WindowSpecification& props) : m_Specification(props) {}

	void Window::Init()
    {
        m_Data.Title = m_Specification.Title;
        m_Data.Width = m_Specification.Width;
        m_Data.Height = m_Specification.Height;

		SEDX_CORE_INFO_TAG("GLFW", "Creating window {0} ({1}, {2})", m_Specification.Title, m_Specification.Width, m_Specification.Height);

		if (!s_GLFWInitialized)
		{
			// TODO: glfwTerminate on system shutdown
			int success = glfwInit();
			//SEDX_CORE_ASSERT(success, "Could not intialize GLFW!");
			glfwSetErrorCallback(GLFWErrorCallback);

			s_GLFWInitialized = true;
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_DECORATED, false);

		if (m_Specification.Fullscreen)
		{
			GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
			const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);

			glfwWindowHint(GLFW_DECORATED, false);
			glfwWindowHint(GLFW_RED_BITS, mode->redBits);
			glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
			glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
			glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

			window = glfwCreateWindow(mode->width, mode->height, m_Data.Title.c_str(), primaryMonitor, nullptr);
		}
		else
		{
			window = glfwCreateWindow((int)m_Specification.Width, (int)m_Specification.Height, m_Data.Title.c_str(), nullptr, nullptr);
		}

		SetWindowIcon(window);

		glfwSetWindowUserPointer(window, &m_Data);

		bool isRawMouseMotionSupported = glfwRawMouseMotionSupported();
		if (isRawMouseMotionSupported)
			glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
		else
			SEDX_CORE_WARN_TAG("Platform", "Raw mouse motion not supported.");

		// Set GLFW callbacks
		glfwSetWindowSizeCallback(window, [](GLFWwindow* window, int width, int height)
		{
			auto& data = *((WindowData*)glfwGetWindowUserPointer(window));

			WindowResizeEvent event((uint32_t)width, (uint32_t)height);
			data.EventCallback(event);
			data.Width = width;
			data.Height = height;
		});

		glfwSetWindowCloseCallback(window, [](GLFWwindow* window)
		{
			auto& data = *((WindowData*)glfwGetWindowUserPointer(window));

			WindowCloseEvent event;
			data.EventCallback(event);
		});

		glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			auto& data = *((WindowData*)glfwGetWindowUserPointer(window));

			switch (action)
			{
				case GLFW_PRESS:
				{
					Input::UpdateKeyState((KeyCode)key, KeyState::Pressed);
					KeyPressedEvent event((KeyCode)key, 0);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					Input::UpdateKeyState((KeyCode)key, KeyState::Released);
					KeyReleasedEvent event((KeyCode)key);
					data.EventCallback(event);
					break;
				}
				case GLFW_REPEAT:
				{
					Input::UpdateKeyState((KeyCode)key, KeyState::Held);
					KeyPressedEvent event((KeyCode)key, 1);
					data.EventCallback(event);
					break;
				}
			}
		});

		glfwSetCharCallback(window, [](GLFWwindow* window, uint32_t codepoint)
		{
			auto& data = *((WindowData*)glfwGetWindowUserPointer(window));

			KeyTypedEvent event((KeyCode)codepoint);
			data.EventCallback(event);
		});

		glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods)
		{
			auto& data = *((WindowData*)glfwGetWindowUserPointer(window));

			switch (action)
			{
				case GLFW_PRESS:
				{
					Input::UpdateButtonState((MouseButton)button, KeyState::Pressed);
					MouseButtonPressedEvent event((MouseButton)button);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					Input::UpdateButtonState((MouseButton)button, KeyState::Released);
					MouseButtonReleasedEvent event((MouseButton)button);
					data.EventCallback(event);
					break;
				}
			}
		});

		glfwSetScrollCallback(window, [](GLFWwindow* window, double xOffset, double yOffset)
		{
			auto& data = *((WindowData*)glfwGetWindowUserPointer(window));

			MouseScrolledEvent event((float)xOffset, (float)yOffset);
			data.EventCallback(event);
		});

		glfwSetCursorPosCallback(window, [](GLFWwindow* window, double x, double y)
		{
			auto& data = *((WindowData*)glfwGetWindowUserPointer(window));
			MouseMovedEvent event((float)x, (float)y);
			data.EventCallback(event);
		});

		/*
		glfwSetTitlebarHitTestCallback(window, [](GLFWwindow* window, int x, int y, int* hit)
		{
			auto& data = *((WindowData*)glfwGetWindowUserPointer(window));
			WindowTitleBarHitTestEvent event(x, y, *hit);
			data.EventCallback(event);
		});
		*/

		glfwSetWindowIconifyCallback(window, [](GLFWwindow* window, int iconified)
		{
			auto& data = *((WindowData*)glfwGetWindowUserPointer(window));
			WindowMinimizeEvent event((bool)iconified);
			data.EventCallback(event);
		});

		m_ImGuiMouseCursors[ImGuiMouseCursor_Arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
		m_ImGuiMouseCursors[ImGuiMouseCursor_TextInput] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
		m_ImGuiMouseCursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);   // FIXME: GLFW doesn't have this.
		m_ImGuiMouseCursors[ImGuiMouseCursor_ResizeNS] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
		m_ImGuiMouseCursors[ImGuiMouseCursor_ResizeEW] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
		m_ImGuiMouseCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);  // FIXME: GLFW doesn't have this.
		m_ImGuiMouseCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);  // FIXME: GLFW doesn't have this.
		m_ImGuiMouseCursors[ImGuiMouseCursor_Hand] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);

		// Update window size to actual size
		{
			int width, height;
			glfwGetWindowSize(window, &width, &height);
			m_Data.Width = width;
			m_Data.Height = height;
		}

	}

	inline std::pair<float, float> Window::GetWindowPos() const
    {
        int x, y;
        glfwGetWindowPos(window, &x, &y);
        return {(float)x, (float)y};
    }

	void Window::ProcessEvents()
	{
		glfwPollEvents();
		Input::Update();
	}

	void Window::SwapBuffers()
	{
		//m_SwapChain->Present();
	}

	void Window::SetVSync(bool enabled)
	{
		m_Specification.VSync = enabled;

		/*
		Application::Get().QueueEvent([&]()
		{
			m_SwapChain->SetVSync(m_Specification.VSync);
			m_SwapChain->OnResize(m_Specification.Width, m_Specification.Height);
		});
		*/
	}

	bool Window::IsVSync() const
	{
		return m_Specification.VSync;
	}

	void Window::SetResizable(bool resizable) const
	{
		glfwSetWindowAttrib(window, GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);
	}

	void Window::Maximize()
	{
		glfwMaximizeWindow(window);
	}

	void Window::CenterWindow()
	{
		const GLFWvidmode* videmode = glfwGetVideoMode(glfwGetPrimaryMonitor());
		int x = (videmode->width / 2) - (m_Data.Width / 2);
		int y = (videmode->height / 2) - (m_Data.Height / 2);
		glfwSetWindowPos(window, x, y);
	}

	void Window::SetTitle(const std::string& title)
	{
		m_Data.Title = title;
		glfwSetWindowTitle(window, m_Data.Title.c_str());
	}

	/*
	VulkanSwapChain& Window::GetSwapChain()
	{
		return *m_SwapChain;
	}
	*/

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
	    Window *windowInstance = static_cast<Window *>(glfwGetWindowUserPointer(window));
	    windowInstance->scroll += y;
	    windowInstance->deltaScroll += y;
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
	    Window::width = width;
	    Window::height = height;
	    Window::framebufferResized = true;
	
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
	void Window::WindowMaximizeCallback(GLFWwindow* window,int maximize)
	{
	    Window *windowInstance = static_cast<Window *>(glfwGetWindowUserPointer(window));
	    windowInstance->maximized = maximize;
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
	    Window *windowInstance = static_cast<Window *>(glfwGetWindowUserPointer(window));
	    windowInstance->posX = x;
	    windowInstance->posY = y;
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
	void Window::WindowDropCallback(GLFWwindow *window, int count, const char *paths[])
	{
	    Window *windowInstance = static_cast<Window *>(glfwGetWindowUserPointer(window));
	    for (int i = 0; i < count; i++)
	    {
	        windowInstance->pathsDrop.push_back(paths[i]);
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
	void Window::Create()
	{
		glfwInit();
	
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	
		monitors = glfwGetMonitors(&monitorCount); // get all monitors
	
		glfwGetVideoModes(monitors[monitorIndex], &videoModeIndex);
		videoModeIndex -= 1;
	
		window = glfwCreateWindow(width, height, title, nullptr, nullptr); // create window
	
		if (!window)
		{
			glfwTerminate();
			EDITOR_LOG_ERROR("Failed to create GLFW window!");
			throw std::runtime_error("Failed to create GLFW window");
		}
	
		glfwSetWindowPos(window, posX, posY); // set window position
	
		glfwSetFramebufferSizeCallback(window, Window::FramebufferResizeCallback); // set framebuffer resize callback
		glfwSetScrollCallback(window, Window::ScrollCallback);
		glfwSetWindowMaximizeCallback(window, Window::WindowMaximizeCallback);
		glfwSetWindowPosCallback(window, Window::WindowChangePosCallback);
	    glfwSetDropCallback(window, Window::WindowDropCallback); 
	
		SetWindowIcon(window);
	
		dirty = false;
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
		monitors = glfwGetMonitors(&monitorCount);
		// ASSERT(monitorIndex < monitorCount, "Invalid monitorIndex inside Window creation!");
		auto monitor = monitors[monitorIndex];
		auto monitorMode = glfwGetVideoMode(monitor);
	
		int modesCount;
		const GLFWvidmode* videoModes = glfwGetVideoModes(monitors[monitorIndex],&modesCount);
		if (videoModeIndex >= modesCount)
		{
			videoModeIndex = modesCount - 1;
		}
	
		// creating window
		switch (mode)
		{
			case WindowMode::Windowed:
			posY = std::max(posY,31);
			glfwSetWindowMonitor(window,nullptr,posX,posY,Window::width,Window::height,GLFW_DONT_CARE);
			if (Window::maximized)
			{
				glfwMaximizeWindow(window);
			}
			glfwSetWindowAttrib(window,GLFW_MAXIMIZED,Window::maximized);
			glfwSetWindowAttrib(window,GLFW_RESIZABLE,Window::resizable);
			glfwSetWindowAttrib(window,GLFW_DECORATED,Window::decorated);
			break;
			case WindowMode::WindowedFullScreen:
			glfwWindowHint(GLFW_RED_BITS,monitorMode->redBits);
			glfwWindowHint(GLFW_GREEN_BITS,monitorMode->greenBits);
			glfwWindowHint(GLFW_BLUE_BITS,monitorMode->blueBits);
			glfwWindowHint(GLFW_REFRESH_RATE,monitorMode->refreshRate);
			glfwSetWindowMonitor(window,monitor,0,0,monitorMode->width,monitorMode->height,monitorMode->refreshRate);
			break;
			case WindowMode::FullScreen:
			GLFWvidmode videoMode = videoModes[videoModeIndex];
			glfwSetWindowMonitor(window,monitor,0,0,videoMode.width,videoMode.height,videoMode.refreshRate);
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
		glfwGetWindowPos(window,&posX,&posY);
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
			lastKeyState[i] = glfwGetKey(window,i);
		}
		deltaScroll = 0;
		auto newTime = std::chrono::high_resolution_clock::now();
		deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(newTime - lastTime).count();
		deltaTime /= 1000.0f;
		lastTime = newTime;
		double x,y;
		glfwGetCursorPos(window,&x,&y);
		deltaMousePos = mousePos - Vec2(x,y);
		mousePos = Vec2(x,y);
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
	
	/*
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
			if (mode != WindowMode::Windowed)
			{
				// monitor
				{
					ImGui::Text("Monitor");
					ImGui::SameLine(totalWidth / 2.0f);
					ImGui::SetNextItemWidth(totalWidth / 2.0f);
					ImGui::PushID("monitorCombo");
					if (ImGui::BeginCombo("",glfwGetMonitorName(monitors[monitorIndex])))
					{
						for (int i = 0; i < monitorCount; i++)
						{
							bool selected = monitorIndex == i;
							ImGui::PushID(i);
							if (ImGui::Selectable(glfwGetMonitorName(monitors[i]),selected))
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
				if (mode == WindowMode::FullScreen)
				{
					ImGui::Text("Resolution");
					ImGui::SameLine(totalWidth / 2.0f);
					ImGui::SetNextItemWidth(totalWidth / 4.0f);
					ImGui::PushID("monitorRes");
					int modesCount;
					const GLFWvidmode* videoModes = glfwGetVideoModes(monitors[monitorIndex],&modesCount);
					GLFWvidmode currMode = videoModes[videoModeIndex];
					std::string modeText = VideoModeText(currMode);
					if (ImGui::BeginCombo("",modeText.c_str()))
					{
						for (int i = 0; i < modesCount; i++)
						{
							bool selected = videoModeIndex == i;
							currMode = videoModes[i];
							ImGui::PushID(i);
							modeText = VideoModeText(currMode);
							if (ImGui::Selectable(modeText.c_str(),selected))
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
				if (mode == WindowMode::Windowed)
				{
					// maximized
					{
						ImGui::Text("Maximized");
						ImGui::SameLine(totalWidth / 2.0f);
						ImGui::SetNextItemWidth(totalWidth / 2.0f);
						ImGui::PushID("maximized");
						if (ImGui::Checkbox("",&maximized))
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
						if (ImGui::Checkbox("",&decorated))
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
						if (ImGui::Checkbox("",&resizable))
						{
							dirty = true;
						}
						ImGui::PopID();
					}
				}
			}
		}
	}
	*/
	
	/**
	 * @brief Updates the framebuffer size.
	 *
	 * This function resets the framebufferResized flag to false and retrieves the current
	 * framebuffer size, updating the width and height of the window accordingly.
	 */
	void Window::UpdateFramebufferSize()
	{
		framebufferResized = false;
		glfwGetFramebufferSize(window,&width,&height);
	}
	
	/**
	 * @brief Gets the window icon image.
	 * 
	 */
	void Window::SetWindowIcon(GLFWwindow *window)
	{
		int width,height,channels;
	
		std::ifstream file("..\\..\\assets\\icon.png",std::ios::binary | std::ios::ate);
		if (!file.is_open())
		{
			EDITOR_LOG_ERROR("Failed to open icon file!");
			return;
		}
	
		// Get file size and read the data
		std::streamsize size = file.tellg();
		file.seekg(0,std::ios::beg);
	
		std::vector<unsigned char> buffer(size);
		if (!file.read(reinterpret_cast<char*>(buffer.data()),size))
		{
			EDITOR_LOG_ERROR("Failed to read icon file data!");
			return;
		}
	
		unsigned char* data = stbi_load_from_memory(
			buffer.data(),
			static_cast<int>(size),
			&width,
			&height,
			&channels,
			4
		);
	
		if (data)
		{
			GLFWimage icon = {width, height, data};
			glfwSetWindowIcon(window,1,&icon);
			stbi_image_free(data);
		}
		else
		{
			EDITOR_LOG_ERROR("Failed to load window icon!");
		}
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
		return lastKeyState[keyCode] && !glfwGetKey(window,keyCode);
	}
	
	//int Window::GetWidth()
	//{
	//    // If we have a window, get the actual size from it
	//    if (window)
	//    {
	//        glfwGetWindowSize(window, &width, &height);
	//    }
	//    return width;
	//}
	
	//int Window::GetHeight()
	//{
	//    // If we have a window, get the actual size from it
	//    if (window)
	//    {
	//        glfwGetWindowSize(window, &width, &height);
	//    }
	//    return height;
	//}


} // namespace SceneryEditorX

// -------------------------------------------------------


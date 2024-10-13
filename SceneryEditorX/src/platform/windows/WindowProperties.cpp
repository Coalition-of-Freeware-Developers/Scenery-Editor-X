#include "../src/pch.h"
#include "../src/core/MainWindow.h"


namespace SceneryEditorX
{
	static unit8_t s_GLFWWindowCount = 0;

	static void GLFWErrorCallback(int error, const char* description)
	{
		SCENERYEDITORX_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
	}

	WindowProperties::WindowProperties(const WindowProperties& properties)
	{
		SCENERYEDITORX_PROFILE_FUNCTION();

		Init(properties);
	}

	WindowProperties::~WindowProperties()
	{
		SCENERYEDITORX_PROFILE_FUNCTION();

		Shutdown();
	}

	void MainWindow::Init(const WindowProperties& properties)
	{
		SCENERYEDITORX_PROFILE_FUNCTION();

		m_Data.Title = properties.GetTitle();
		m_Data.Width = properties.GetWidth();
		m_Data.Height = properties.GetHeight();

		SCENERYEDITORX_CORE_INFO("Creating window {0} ({1}, {2})", properties.GetTitle(), properties.GetWidth(), properties.GetHeight());

		if (s_GLFWWindowCount == 0)
		{
			SCENERYEDITORX_PROFILE_SCOPE("glfwInit");
			int success = glfwInit();
			SCENERYEDITORX_CORE_ASSERT(success, "Could not initialize GLFW!");
			glfwSetErrorCallback(GLFWErrorCallback);
		}
		{
			SCENERYEDITORX_PROFILE_SCOPE("glfwCreateWindow");
			m_Window = glfwCreateWindow((int)properties.GetWidth(), (int)properties.GetHeight(), m_Data.Title.c_str(), nullptr, nullptr);
			++s_GLFWWindowCount;
		}

		m_Context = GraphicsContext::Create(m_Window);
		m_Context->Init();

		glfwSetWindowUserPointer(m_Window, &m_Data);
		SetVSync(true);

		// Set GLFW callbacks
		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				data.Width = width;
				data.Height = height;

				WindowResizeEvent event(width, height);
				data.EventCallback(event);
			});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				WindowCloseEvent event;
				data.EventCallback(event);
			});

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				switch (action)
				{
				case GLFW_PRESS:
				{
					KeyPressedEvent event(key, 0);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					KeyReleasedEvent event(key);
					data.EventCallback(event);
					break;
				}
				case GLFW_REPEAT:
				{
					KeyPressedEvent event(key, 1);
					data.EventCallback(event);
					break;
				}
				}
			});

		glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int keycode)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				KeyTypedEvent event(keycode);
				data.EventCallback(event);
			});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				switch (action)
				{
				case GLFW_PRESS:
				{
					MouseButtonPressedEvent event(button);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					MouseButtonReleasedEvent event(button);
					data.EventCallback(event);
					break;
				}
				}
			});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				MouseScrolledEvent event((float)xOffset, (float)yOffset);
				data.EventCallback(event);
			});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				MouseMovedEvent event((float)xPos, (float)yPos);
				data.EventCallback(event);
			});

		void MainWindow::Shutdown()
		{
			SCENERYEDITORX_PROFILE_FUNCTION();

			glfwDestroyWindow(m_Window);

			--s_GLFWWindowCount;
			if (s_GLFWWindowCount == 0)
			{
				SCENERYEDITORX_PROFILE_SCOPE("glfwTerminate");
				glfwTerminate();
			}
		}
	}

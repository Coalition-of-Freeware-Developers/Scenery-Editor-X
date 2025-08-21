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
#include "window.h"
#include <stb_image.h>
#include <imgui/imgui.h>
#include "icon.h"
#include "monitor_data.h"
#include "SceneryEditorX/core/input/input.h"
#include "SceneryEditorX/core/memory/memory.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{
    // Definition of static members
    std::chrono::high_resolution_clock::time_point Window::lastTime;
    std::vector<std::string> Window::pathsDrop;
    float Window::deltaTime = 0.0f;
    char Window::lastKeyState[GLFW_KEY_LAST + 1] = {};
    WindowMode Window::mode = WindowMode::Windowed;

    // Definition of WindowData static member
    //GLFWwindow* WindowData::window = nullptr;

    /**
     * @brief Global flag indicating whether GLFW has been initialized.
     *
     * This boolean tracks the initialization state of the GLFW library across the application.
     * When set to true, it indicates that glfwInit() has been successfully called.
     * This prevents multiple initialization attempts which could cause resource leaks.
     * The Window::Init() method checks this flag before calling glfwInit().
     *
     * @note - This flag is reset to false in Window::Shutdown() when glfwTerminate() is called.
     */
    GLOBAL bool windowInit = false;

    /**
     * @brief Callback function for handling GLFW error events.
     *
     * This function is registered with GLFW to handle error events that occur during
     * window operations. It specifically filters out joystick-related errors (which
     * commonly occur with GLFW_INVALID_ENUM code 0x10003) to prevent log spam when
     * flight simulator hardware is connected.
     *
     * All other GLFW errors are logged to the application's error logging system with
     * appropriate error code and description information.
     *
     * @param error The GLFW error code (e.g., GLFW_INVALID_ENUM, GLFW_INVALID_VALUE)
     * @param description A string describing the error that occurred
     *
     * @note - This function is set as the GLFW error callback during Window::Init()
     */
    GLOBAL void WindowErrorCallback(int error, const char *description)
	{
		/// Filter out joystick-related errors (codes around 65539 GLFW_INVALID_ENUM)
		if (error == 0x10003 && strstr(description, "joystick"))
            return; /// Silently ignore joystick-related GLFW_INVALID_ENUM errors

        SEDX_CORE_ERROR_TAG("Window", "GLFW Error ({0}): {1}", error, description);
    }

    /**
     * @brief Creates a new Window instance using the provided specifications.
     *
     * This static factory method encapsulates the creation of a Window object,
     * allowing for a consistent interface for window instantiation throughout the
     * application. It allocates a new Window instance on the heap using the
     * provided window specifications.
     *
     * @param windowSpecs The configuration data for the new window, including properties
     *                   like dimensions, title, and display mode.
     * @return A pointer to the newly created Window instance.
     *
     * @note - The caller is responsible for managing the memory of the returned Window
     *       pointer, typically by using smart pointers or explicit deletion.
     * @note - This method only creates the Window object but doesn't initialize it.
     *       Call Init() on the returned object to complete window creation.
     */
    Window *Window::Create(const WindowData &windowSpecs)
    {
        return new Window(windowSpecs);
    }

    /**
     * @brief Default constructor for Window.
     *
     * Creates a Window instance with default WindowData values.
     */
    Window::Window() : swapChain(nullptr), renderData(), winData(WindowData{}), leftAlt(0), initState(false),
          mousePressed(false), captureMovement(false)
    {
    }

    /**
     * @brief Constructs a Window instance with the specified window data.
     *
     * This constructor initializes a new Window object using the provided window specifications.
     * It stores the configuration data but doesn't create the actual window yet - the window
     * is created later when the Init() method is called.
     *
     * @param winData The window configuration data containing properties like width, height,
     *                title, decoration status, and other window attributes.
     *
     * @note - This is a lightweight constructor that only stores configuration. The actual window
     *       creation, monitor setup, and renderer initialization happens in the Init() method.
     */
    Window::Window(WindowData winData) : swapChain(nullptr), renderData(), winData(std::move(winData)), leftAlt(0), initState(false),
          mousePressed(false), captureMovement(false)
    {
    }

    /**
	 * @brief Destroys the window and terminates GLFW.
	 *
	 * This function retrieves the current window position, destroys the window,
	 * and terminates the GLFW library.
	 */
    Window::~Window()
    {
        Window::Shutdown();
    }

    /**
	 * @brief Creates a new window and initializes GLFW.
	 *
	 * This function initializes the GLFW library, sets the necessary window hints,
	 * retrieves the available monitors and video modes, creates a new window, and sets
	 * the window position and various callback functions. It also applies any pending
	 * changes to the window configuration.
	 */
    void Window::Init()
    {
        m_winSpecs.title = winData.title;
        m_winSpecs.width = winData.width;
        m_winSpecs.height = winData.height;

        /// Initialize GLFW if not already initialized
        if (!windowInit)
        {
            SEDX_CORE_INFO("Initializing GLFW");
            if (int success = glfwInit(); !success)
			{
                SEDX_CORE_ERROR("Failed to initialize GLFW!");
                return;
            }
            glfwSetErrorCallback(WindowErrorCallback);
            windowInit = true;
            SEDX_CORE_INFO("GLFW initialized successfully");
        }

        /// Set window hints
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, winData.resizable ? GLFW_TRUE : GLFW_FALSE);

        if (!winData.decorated)
        {
            SEDX_CORE_INFO("Creating window without decorations");
            glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        }

        /// Initialize monitor data
        MonitorData monitorData;
        bool monitorInitSuccess;
        try
		{
            monitorData.RefreshMonitorList();
            monitorInitSuccess = true;
            SEDX_CORE_INFO("Monitor data initialized successfully");
        }
        catch (const std::exception& e)
		{
            SEDX_CORE_ERROR("Failed to initialize monitor data: {}", e.what());
            monitorInitSuccess = false;
        }

        /// Create window based on mode
        SEDX_CORE_INFO("Creating window: {}x{} - '{}'", static_cast<int>(winData.width), static_cast<int>(winData.height), winData.title);
        bool windowCreated = false;

        /// First attempt - create with specified settings
        if (mode == WindowMode::FullScreen && monitorInitSuccess)
        {
            if (GLFWmonitor* primaryMonitor = monitorData.GetPrimaryMonitor())
			{
                if (const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor))
				{
                    SEDX_CORE_INFO("Creating fullscreen window on primary monitor: {}x{} @ {}Hz", mode->width, mode->height, mode->refreshRate);
                    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
                    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
                    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
                    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
                    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
                    m_window = glfwCreateWindow(mode->width, mode->height, winData.title.c_str(), primaryMonitor, nullptr);
                    windowCreated = (m_window != nullptr);
                }
            }
        }

        /// If fullscreen creation failed or not in fullscreen mode, create windowed
        if (!windowCreated) {
            SEDX_CORE_INFO("Creating window in windowed mode: {}x{}", static_cast<int>(winData.width), static_cast<int>(winData.height));
            m_window = glfwCreateWindow(static_cast<int>(winData.width), static_cast<int>(winData.height),winData.title.c_str(), nullptr, nullptr);
            windowCreated = m_window != nullptr;
        }

        /// Final fallback - try creating a minimal window
        if (!windowCreated)
		{
            SEDX_CORE_WARN("Window creation failed with specified parameters, trying fallback settings");
            /// Reset window hints to defaults
            glfwDefaultWindowHints();
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

            /// Try creating a basic window
            m_window = glfwCreateWindow(800, 600, "Scenery Editor X (Fallback)", nullptr, nullptr);
            windowCreated = (m_window != nullptr);

            if (windowCreated)
			{
                SEDX_CORE_INFO("Created fallback window successfully");
                m_winSpecs.width = 800;
                m_winSpecs.height = 600;
                winData.width = 800;
                winData.height = 600;
            }
            else
			{
                SEDX_CORE_ERROR("All window creation attempts failed!");
                return;
            }
        }

        SEDX_CORE_INFO("Window created successfully");

        /// Continue with window setup
        if (m_window)
		{
            SetWindowIcon(m_window);
            winData.dirty = false;

            if (winData.maximized)
			{
                SEDX_CORE_INFO("Maximizing window");
                glfwMaximizeWindow(m_window);
            }

            renderContext = RenderContext::Get();
            renderContext->Init();

            glfwSetWindowUserPointer(m_window, &winData);
            DisableJoystickHandling();

            if (glfwRawMouseMotionSupported())
                glfwSetInputMode(m_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
            else
                SEDX_CORE_WARN_TAG("Window", "Raw mouse motion not supported.");

            /// Set up window callbacks
            glfwSetWindowSizeCallback(m_window, [](GLFWwindow *window, int width, int height)
            {
                auto &data = *(WindowData *)glfwGetWindowUserPointer(window);
                data.width = width;
                data.height = height;
            });

            winData.framebufferResized = true;

            /// Set all the callbacks
            glfwSetWindowCloseCallback(m_window, windowCallbacks.windowCloseCallback);
            glfwSetFramebufferSizeCallback(m_window, windowCallbacks.framebufferResizeCallback);
            glfwSetWindowPos(m_window, winData.posX, winData.posY);
            glfwSetCharCallback(m_window, windowCallbacks.charCallback);
            glfwSetCursorPosCallback(m_window, windowCallbacks.cursorPosCallback);
            glfwSetKeyCallback(m_window, windowCallbacks.keyCallback);
            glfwSetMouseButtonCallback(m_window, windowCallbacks.mouseButtonCallback);
            glfwSetScrollCallback(m_window, windowCallbacks.scrollCallback);
            glfwSetWindowMaximizeCallback(m_window, windowCallbacks.windowMaximizeCallback);
            glfwSetWindowPosCallback(m_window, windowCallbacks.windowChangePosCallback);
            glfwSetDropCallback(m_window, windowCallbacks.windowDropCallback);
            glfwSetWindowIconifyCallback(m_window, windowCallbacks.windowIconifyCallback);

            /// Create mouse cursors for ImGui
            ImGuiMouseCursors[ImGuiMouseCursor_Arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
            ImGuiMouseCursors[ImGuiMouseCursor_TextInput] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
            ImGuiMouseCursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);   /// FIXME: GLFW doesn't have this.
            ImGuiMouseCursors[ImGuiMouseCursor_ResizeNS] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
            ImGuiMouseCursors[ImGuiMouseCursor_ResizeEW] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
            ImGuiMouseCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);  /// FIXME: GLFW doesn't have this.
            ImGuiMouseCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);  /// FIXME: GLFW doesn't have this.
            ImGuiMouseCursors[ImGuiMouseCursor_Hand] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);

            /// Update window dimensions
            int width, height;
            glfwGetWindowSize(m_window, &width, &height);
            m_winSpecs.width = width;
            m_winSpecs.height = height;

            SEDX_CORE_INFO("Window setup complete: {}x{}", width, height);
        }
    }

    /**
     * @brief Releases all rendering resources and terminates the window system.
     *
     * This method performs a complete cleanup of all resources associated with the window:
     * 1. Destroys the swap chain by calling its Destroy() method
     * 2. Deallocates the swap chain object from memory
     * 3. Retrieves the Vulkan logical device from the render context and destroys it
     * 4. Terminates the GLFW library, releasing all window resources
     * 5. Resets the global window initialization flag
     *
     * This method should be called when the application is shutting down or
     * when the window is being closed to ensure proper cleanup of all GPU and
     * system resources.
     *
     * @note - This must be called before the application exits to prevent resource leaks.
     */
    void Window::Shutdown()
    {
        if (m_window)
		{
            glfwDestroyWindow(m_window);
            m_window = nullptr;
        }

        if (renderContext)
            renderContext.As<RenderContext>()->GetLogicDevice()->Destroy();

        if (windowInit)
		{
            glfwTerminate();
            windowInit = false;
        }
    }

	void Window::SwapBuffers()
    {
        //swapChain->Present();
    }

    /**
     * @brief Retrieves the current window position.
     *
     * This method gets the current window position in screen coordinates using the GLFW API.
     * It updates the stored position values in the WindowData struct and returns them as a pair
     * of floating-point values for use in positioning calculations.
     *
     * @return A pair of floating-point values representing the window's x and y position in screen coordinates.
     *
     * @note - The position coordinates are relative to the top-left corner of the screen, with positive y values
     *       moving downward and positive x values moving rightward.
     */
	inline std::pair<float, float> Window::GetWindowPos() const
    {
        int x, y;
        if (!m_window)
            return {0.0f, 0.0f};

        glfwGetWindowPos(m_window, &x, &y);
        return { static_cast<float>(x), static_cast<float>(y) };
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
            windowInstance->winData.dirty = true;
        }

        //WindowData::scroll += x, WindowData::deltaScroll += y;
	}

    /**
     * @brief Processes all pending events in the GLFW event queue.
     *
     * This function is called to handle all events that have occurred since the last call.
     * It processes input events, updates the state of the window, and handles any user input.
     *
     * @note - This function should be called regularly in the main application loop to ensure
     */
    void Window::ProcessEvents()
    {
        glfwPollEvents();
        Input::Update();
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
        if (auto windowInstance = static_cast<Window *>(glfwGetWindowUserPointer(window)); windowInstance->captureMovement)
        {
            GLFWcursor *hand = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
            GLFWcursor *cursor = glfwCreateStandardCursor(GLFW_CURSOR_NORMAL);

			if (button == GLFW_MOUSE_BUTTON_RIGHT)
            {
                if (action == GLFW_PRESS)
                {
                    windowInstance->mousePressed = true;
                    glfwSetCursor(window, hand);
                }
                else if (action == GLFW_RELEASE)
                {
                    windowInstance->mousePressed = false;
                    glfwSetCursor(window, cursor);
                }
            }
        }
    }

    /**
	 * @brief Maximizes the application window.
	 *
	 * This method calls the GLFW function to maximize the window associated with this instance.
	 * It is typically used to expand the window to fill the available screen space on the current monitor.
	 * The maximized state is managed by GLFW and may trigger window maximize callbacks if registered.
	 */
	void Window::Maximize()
	{
	    if (!m_window)
		{
	        SEDX_CORE_WARN("Cannot maximize window - window not created yet");
	        return;
	    }

		auto &data = *static_cast<WindowData *>(glfwGetWindowUserPointer(m_window));
		if (data.maximized)
            return;

	    glfwMaximizeWindow(m_window);
        data.maximized = true;
        SEDX_CORE_INFO("Window Maximized");
	}

	/**
	 * @brief Centers the application window on the primary monitor.
	 *
	 * This method calculates the position required to center the window
	 * based on the current resolution of the primary monitor and the
	 * window's width and height. It then sets the window's position
	 * accordingly using GLFW.
	 *
	 * The calculation ensures that the window appears centered horizontally
	 * and vertically on the user's main display.
	 */
	void Window::CenterWindow()
	{
	    if (!m_window)
		{
	        SEDX_CORE_WARN("Cannot center window - window not created yet");
	        return;
	    }

	    const GLFWvidmode *videoMode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	    if (!videoMode)
		{
	        SEDX_CORE_WARN("Cannot center window - failed to get primary monitor video mode");
	        return;
	    }

        const int x = videoMode->width / 2 - (m_winSpecs.width / 2);
        const int y = videoMode->height / 2 - (m_winSpecs.height / 2);
	    glfwSetWindowPos(m_window, x, y);
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
     * @param x The new x-coordinate of the cursor relative to the window.
     * @param y The new y-coordinate of the cursor relative to the window.
     */
    void Window::MousePositionCallback(GLFWwindow* window, double x, double y)
    {
        if (auto windowInstance = static_cast<Window *>(glfwGetWindowUserPointer(window)); windowInstance->captureMovement)
        {
            windowInstance->winData.mousePos.x = static_cast<float>(x);
            windowInstance->winData.mousePos.y = static_cast<float>(y);

			const auto pointerX = static_cast<float>(x);
            const auto pointerY = static_cast<float>(y);
            if (windowInstance->initState)
            {
                windowInstance->winData.deltaMousePos.x = pointerX;
                windowInstance->winData.deltaMousePos.y = pointerY;
                windowInstance->initState = false;
            }

			float xOffset = x - pointerX - windowInstance->winData.deltaMousePos.x;
            float yOffset = windowInstance->winData.deltaMousePos.y - pointerY; /// Invert the sign here

			windowInstance->winData.deltaMousePos.x = pointerX;
            windowInstance->winData.deltaMousePos.y = pointerY;

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
	    /// Retrieve the Window instance from the GLFW user pointer
        if (auto windowInstance = static_cast<Window *>(glfwGetWindowUserPointer(window)))
		{
            windowInstance->winData.width = width;
            windowInstance->winData.height = height;
            windowInstance->winData.framebufferResized = true;
            SEDX_CORE_INFO("Window framebuffer resized to: {}x{}", width, height);
        }
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
        windowInstance->winData.maximized = maximize;
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
        if (auto windowInstance = static_cast<Window *>(glfwGetWindowUserPointer(window)); windowInstance->captureMovement)
        {
            windowInstance->winData.dirty = true;

            constexpr float movementSpeed = 2.5f;

			if (key == GLFW_KEY_LEFT_ALT && action == GLFW_PRESS)
                windowInstance->leftAlt = true;
            if (key == GLFW_KEY_LEFT_ALT && action == GLFW_RELEASE)
                windowInstance->leftAlt = false;

            windowInstance->windowCallbacks.keyCallback(window, key, scancode, action, mods);

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
        windowInstance->winData.posX = x;
        windowInstance->winData.posY = y;
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
	    if (!m_window)
		{
	        SEDX_CORE_WARN("Cannot apply window changes - window not created yet");
	        return;
	    }

        /// Create a MonitorData instance to access monitor information
        MonitorData monitorData;

        try
		{
            monitorData.RefreshDisplayCount();
            monitorData.RefreshMonitorList();
        }
        catch (const std::exception& e)
		{
            SEDX_CORE_ERROR("Failed to refresh monitor data: {}", e.what());
            return;
        }

        /// Get the current monitor
        GLFWmonitor* currentMonitor = monitorData.GetCurrentMonitor();
        if (!currentMonitor)
		{
            SEDX_CORE_ERROR("Failed to get current monitor");
            return;
        }

        /// Ensure the monitor index is valid
        int currentMonitorIndex = monitorData.GetCurrentMonitorIndex();
        if (int monitorCount = monitorData.GetMonitorCount(); currentMonitorIndex >= monitorCount)
		{
            SEDX_CORE_ERROR("Invalid monitor index: {} (total monitors: {})", currentMonitorIndex, monitorCount);
            return;
        }

        /// Get current monitor's video mode
        const GLFWvidmode* monitorMode = monitorData.GetCurrentVideoMode();
        if (!monitorMode)
		{
            SEDX_CORE_ERROR("Failed to get current monitor video mode");
            return;
        }

        /// Get video modes for the current monitor
        int modesCount = 0;
        const GLFWvidmode* videoModes = monitorData.GetVideoModes(currentMonitorIndex, &modesCount);

        /// Validate video mode index
        int videoModeIndex = monitorData.GetVideoModeIndex();
        if (videoModeIndex >= modesCount)
        {
            videoModeIndex = modesCount - 1;
            monitorData.SetVideoModeIndex(videoModeIndex);
        }

        /// Apply window configuration based on current mode
        switch (mode)
        {
            case WindowMode::Windowed:
                winData.posY = std::max(winData.posY, 31);
                glfwSetWindowMonitor(m_window, nullptr, winData.posX, winData.posY, m_winSpecs.width, m_winSpecs.height, GLFW_DONT_CARE);

                if (winData.maximized)
                    glfwMaximizeWindow(m_window);

                glfwSetWindowAttrib(m_window, GLFW_MAXIMIZED, winData.maximized ? GLFW_TRUE : GLFW_FALSE);
                glfwSetWindowAttrib(m_window, GLFW_RESIZABLE, winData.resizable ? GLFW_TRUE : GLFW_FALSE);
                glfwSetWindowAttrib(m_window, GLFW_DECORATED, winData.decorated ? GLFW_TRUE : GLFW_FALSE);
                break;

            case WindowMode::WindowedFullScreen:
                glfwSetWindowMonitor(m_window, currentMonitor, 0, 0, monitorMode->width, monitorMode->height, monitorMode->refreshRate);
                break;

            case WindowMode::FullScreen:
                if (videoModes && videoModeIndex < modesCount)
				{
                    const GLFWvidmode videoMode = videoModes[videoModeIndex];
                    glfwSetWindowMonitor(m_window, currentMonitor, 0, 0, videoMode.width, videoMode.height, videoMode.refreshRate);
                }
                else
                {
                    SEDX_CORE_ERROR("Invalid video mode index or no video modes available");
                }

                break;
        }

        winData.framebufferResized = false;
        winData.dirty = false;
    }

    /**
     * @brief Applies the current window mode setting and updates rendering resources.
     *
     * This method performs the following operations when the window mode changes:
     * 1. Calls ApplyChanges() to update window properties based on the current mode setting
     *    (Windowed, WindowedFullScreen, or FullScreen)
     * 2. If a swap chain exists, calls its Recreate() method to rebuild the rendering
     *    surface to match the new window dimensions and properties
     * 3. Logs an informational message about the window mode change
     *
     * The window mode affects several aspects of the application:
     * - Window size and position
     * - Decoration status (borders, title bar)
     * - Monitor assignment
     * - Resolution and refresh rate settings
     *
     * This method should be called whenever the user changes the window mode through
     * the UI or when the application needs to programmatically change display settings.
     */
    void Window::ChangeWindowMode()
    {
		/// Apply changes to the window based on the current mode
		ApplyChanges();

		/// Update the swap chain if it exists
		if (swapChain)
		{
			//swapChain->Present();
		}

		SEDX_CORE_INFO("Window mode changed to: {}", static_cast<int>(mode));
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
	    if (!m_window)
		{
	        SEDX_CORE_WARN("Cannot update window - window not created yet");
	        return;
	    }

        for (auto i = 0; i < GLFW_KEY_LAST + 1; i++)
            lastKeyState[i] = static_cast<char>(glfwGetKey(m_window, i));

        winData.deltaScroll = 0;
		auto newTime = std::chrono::high_resolution_clock::now();
		deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(newTime - lastTime).count();
		deltaTime /= 1000.0f;
		lastTime = newTime;

		double x, y;
        glfwGetCursorPos(m_window, &x, &y);

        float prevX = winData.mousePos.x;
        float prevY = winData.mousePos.y;
        winData.deltaMousePos.x = prevX - static_cast<float>(x);
        winData.deltaMousePos.y = prevY - static_cast<float>(y);

        winData.mousePos.x = static_cast<float>(x);
        winData.mousePos.y = static_cast<float>(y);

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
		return ToString(mode.width) + "x" + ToString(mode.height) + " " + std::to_string(mode.refreshRate) + " Hz";
	}

    /**
	 * @brief Updates the framebuffer size.
	 *
	 * This function retrieves the current framebuffer size using GLFW, stores
	 * the size in the WindowData structure, and resets the framebufferResized flag.
	 * It properly handles the conversion between int (GLFW parameter type) and
	 * uint32_t (WindowData storage type).
	 */
    void Window::UpdateFramebufferSize()
    {
        if (!m_window)
		{
            SEDX_CORE_WARN("Cannot update framebuffer size - window not created yet");
            return;
        }

        int width, height;
        glfwGetFramebufferSize(m_window, &width, &height);
        m_winSpecs.width = static_cast<uint32_t>(width);
        m_winSpecs.height = static_cast<uint32_t>(height);
        winData.framebufferResized = false;
    }

    /**
     * @brief
     *
     * @param title 
     */
    void Window::SetTitle(const std::string &title)
    {
        m_winSpecs.title = title;
        if (m_window)
            glfwSetWindowTitle(m_window, m_winSpecs.title.c_str());
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
	 * @note - The icon file path is defined in the IconData structure, typically pointing
	 *       to "..\\..\\assets\\icon.png"
	 * @note - This implementation uses stb_image for PNG decoding and requires it to be included
	 */
	void Window::SetWindowIcon(GLFWwindow *window)
	{
	    if (!window)
		{
	        SEDX_CORE_WARN("Cannot set window icon - window handle is null");
	        return;
	    }

	    IconData iconData;

	    std::ifstream file(iconData.path, std::ios::binary | std::ios::ate);
	    if (!file.is_open())
	    {
	        SEDX_CORE_ERROR("Failed to open icon file: {}", iconData.path);
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
	        SEDX_CORE_INFO("Window icon set successfully");
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
	bool Window::IsKeyPressed(const uint16_t keyCode) const
    {
        if (!m_window)
            return false;

        return lastKeyState[keyCode] && !glfwGetKey(m_window, keyCode);
	}

    /**
     * @brief Retrieves the swap chain associated with this window.
     *
     * This method returns a reference to the SwapChain object used for rendering
     * in this window. The swap chain manages the presentation of rendered images
     * to the window surface and is a core component of the Vulkan rendering pipeline.
     *
     * @return Reference to the SwapChain used by this window.
     */
    SwapChain &Window::GetSwapChain() { return *swapChain; }


	void Window::SetResizable(bool resizable) const
    {
        if (m_window)
            glfwSetWindowAttrib(m_window, GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);
    }

}

/// -------------------------------------------------------

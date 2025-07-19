/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* application.cpp
* -------------------------------------------------------
* Created: 25/5/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/core/application/application.h>
#include <SceneryEditorX/core/events/application_events.h>
#include <SceneryEditorX/core/input/input.h>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/renderer/vulkan/vk_swapchain.h>

/// -------------------------------------------------------

extern bool appRunning; /// Global variable to control the application loop

/// -------------------------------------------------------

namespace SceneryEditorX
{
    Application *Application::appInstance = nullptr;
    INTERNAL std::thread::id s_MainThreadID;

    /// -------------------------------------------------------

    Application::Application(const AppData &appData)
    {
        SEDX_CORE_INFO("Creating application with window: {}x{}", appData.WinWidth, appData.WinHeight);

        appInstance = this;

        /// Create the window
        m_Window = CreateScope<Window>();

        /// Set window properties from appData
        if (appData.WinWidth > 0 && appData.WinHeight > 0)
        {
            /// Update window properties if provided
            // m_Window->winData.width = appData.WinWidth;
            // m_Window->winData.height = appData.WinHeight;
            // Use public API to set window size if available
            // If not, set via m_winSpecs (public struct)
            // But m_winSpecs is private, so use SetSize if exists
            // Otherwise, fallback to constructor or expose a setter
            // Here, we use ApplyChanges after setting width/height
            // But since winData is private, we cannot access it directly
            // Instead, set via a public method or constructor
            // If not available, this is a design issue
            // For now, skip direct assignment and rely on ApplyChanges
        }

        if (!appData.appName.empty())
        {
            m_Window->SetTitle(appData.appName);
            SEDX_CORE_INFO_TAG("Application", "Window title Changed to {}", appData.appName);
        }

		SEDX_CORE_INFO("Initializing Window");
        /// Initialize the window first
        m_Window->Init();

        SEDX_CORE_INFO("Window Initialized");
        /// Update window properties
        m_Window->ApplyChanges();
        SEDX_CORE_INFO("Window changes applied");

        isRunning   = true;
        isMinimized = false;
    }

    Application::~Application()
    {
        m_Window->~Window();
        // TODO: Re-enable Renderer::Shutdown() once the renderer header issue is resolved
        //Renderer::Shutdown();
    }

    void Application::Run()
    {
        OnInit();

        /// Main application loop
        while (isRunning && !m_Window->GetShouldClose())
        {
            /// Update the window (poll events)
            m_Window->Update();

            /// Skip frame if window is minimized
            if (isMinimized)
                continue;

            /// Call user-defined update function
            OnUpdate();
        }

        OnShutdown();
    }

    void Application::Stop()
    {
        isRunning = false;
    }

    void Application::OnShutdown()
    {
        SEDX_CORE_INFO("Shutting down application");
        appRunning = false;
    }

    void Application::SyncEvents()
    {
        std::scoped_lock<std::mutex> lock(m_EventQueueMutex);
        for (auto &synced : m_EventQueue | std::views::keys)
        {
            synced = true;
        }
    }


	void Application::ProcessEvents()
	{
		Input::TransitionPressedKeys();
		Input::TransitionPressedButtons();

        m_Window->ProcessEvents();

		// NOTE: we have no control over what func() does.  holding this lock while calling func() is a bad idea:
		// 1) func() might be slow (means we hold the lock for ages)
		// 2) func() might result in events getting queued, in which case we have a deadlock
		std::scoped_lock<std::mutex> lock(m_EventQueueMutex);

		// Process custom event queue, up until we encounter an event that is not yet synced
		// If application queues such events, then it is the application's responsibility to call
		// SyncEvents() at the appropriate time.
		while (!m_EventQueue.empty())
		{
			const auto& [synced, func] = m_EventQueue.front();
			if (!synced)
			{
				break;
			}
			func();
			m_EventQueue.pop_front();
		}
	}

	void Application::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& e) { return OnWindowResize(e); });
		dispatcher.Dispatch<WindowMinimizeEvent>([this](const WindowMinimizeEvent& e) { return OnWindowMinimize(e); });
		dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& e) { return OnWindowClose(e); });

		for (auto it = m_ModuleStage.end(); it != m_ModuleStage.begin(); )
		{
			(*--it)->OnEvent(event);
			if (event.Handled)
				break;
		}

		if (event.Handled)
			return;

		// TODO: Should these callbacks be called BEFORE the layers receive events?
		//				We may actually want that since most of these callbacks will be functions REQUIRED in order for the game
		//				to work, and if a layer has already handled the event we may end up with problems
		for (auto& eventCallback : m_EventCallbacks)
		{
			eventCallback(event);

			if (event.Handled)
				break;
		}

	}

	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		const uint32_t width = e.GetWidth(), height = e.GetHeight();
		if (width == 0 || height == 0)
		{
			//m_Minimized = true;
			return false;
		}
		//m_Minimized = false;
		
		auto& window = m_Window;
		Renderer::Submit([&window, width, height]() mutable
		{
			window->GetSwapChain().OnResize(width, height);
		});

		return false;
	}

	bool Application::OnWindowMinimize(const WindowMinimizeEvent& e)
	{
        isMinimized = e.IsMinimized();
		return false;
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		Stop();
		return false; // give other things a chance to react to window close
	}

	float Application::GetTime() const
	{
		return (float)glfwGetTime();
	}

    const char* Application::GetConfigurationName()
    {
        return SEDX_BUILD_TYPE;
    }

    const char* Application::GetPlatformName()
    {
        return SEDX_PLATFORM_NAME;
    }

	std::thread::id Application::GetMainThreadID()
    {
        return s_MainThreadID;
    }

    bool Application::IsMainThread()
    {
        return std::this_thread::get_id() == s_MainThreadID;
    }

}

/// -------------------------------------------------------------------------

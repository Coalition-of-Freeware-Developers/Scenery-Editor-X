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
#include "application.h"
#include "SceneryEditorX/core/events/application_events.h"
#include "SceneryEditorX/core/input/input.h"
#include "SceneryEditorX/logging/logging.hpp"
#include "SceneryEditorX/renderer/renderer.h"
#include "SceneryEditorX/renderer/swapchain.h"
#include "SceneryEditorX/ui/ui_layer.h"

// -------------------------------------------------------

extern bool appRunning; /// Global variable to control the application loop

// -------------------------------------------------------

namespace SceneryEditorX
{
    Application *Application::appInstance = nullptr;
    static std::thread::id s_MainThreadID;

    // -------------------------------------------------------

    Application::Application(const std::vector<std::string> &args)
    {
        appInstance = this;
        s_MainThreadID = std::this_thread::get_id();
        AppData specification;

        /*
        if (!specification.WorkingDirectory.empty())
            std::filesystem::current_path(specification.WorkingDirectory);
        */

        // Create the window
        m_Window = CreateScope<Window>();

		SEDX_CORE_INFO("Initializing Window");

        m_Window->Init();         // Initialize the window first
        //m_Window->ApplyChanges(); // Update window properties
        m_Window->SetEventCallback([this](Event &e) { OnEvent(e); });

        // Init renderer and execute command queue to compile all shaders
        Renderer::Init();
        //m_RenderThread.Pump(); // Render one frame (TODO: maybe make a func called Pump or something)

        if (specification.StartMaximized)
        {
            m_Window->Maximize();
        }
        else
        {
            m_Window->CenterWindow();
        }
        m_Window->SetResizable(specification.Resizable);

        m_UILayer = UI::UILayer::Create();
        PushOverlay(m_UILayer);

        m_IsRunning   = true;
        m_IsMinimized = false;
    }

    Application::~Application()
    {
        /** 
         * Let RAII handle Window destruction, or explicitly reset the RefCounter once
         * to avoid double-destruction. Do NOT call the destructor directly.
         */
        if (m_Window)
            m_Window.reset();

        // Ensure renderer subsystems are torn down first
        Renderer::Shutdown();
    }

    void Application::Run()
    {
        OnInit();

        // Main application loop
        while (m_IsRunning && !m_Window->GetShouldClose())
        {
            static uint32_t frameCount = 0;

            m_Window->Update(); // Update the window (poll events)
            //m_RenderThread.BlockUntilRenderComplete();

            //m_RenderThread.NextFrame();

            //m_RenderThread.Kick(); // Start rendering previous frame

            if (!m_IsMinimized)
            {

                Renderer::BeginFrame();
                {
					for (Layer *module : m_ModuleStage)
                        module->OnUpdate(m_DeltaTime);
                }

                // Render ImGui on render thread
                Application *app = this;
                if (m_AppData.EnableImGui)
                {
                    Renderer::Submit([app]() { app->RenderUI(); });
                    Renderer::Submit([=]() { m_UILayer->End(); });
                }
                Renderer::EndFrame();

                // On Render thread
                Renderer::Submit([&]() {
                    // m_Window->GetSwapChain().BeginFrame();
                    // Renderer::WaitAndRender();
                    //m_Window->SwapBuffers();
                });

                m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % Renderer::GetRenderData().framesInFlight;
                m_PerformanceTimers.MainThreadWorkTime = cpuTimer.ElapsedMillis();
            }

            OnUpdate();	// Call user-defined update function

            Input::ClearReleasedKeys();

            float time = GetTime();
            m_FrameTime = time - m_LastFrameTime;
            m_DeltaTime = xMath::Min(m_FrameTime <=> 0.333f);
            m_LastFrameTime = time;
			frameCount++;
        }

        OnShutdown();
    }

    void Application::Stop() { m_IsRunning = false; }

    void Application::OnShutdown()
    {
        SEDX_CORE_INFO("Shutting down application");
        m_EventCallbacks.clear();
        appRunning = false;
    }

    void Application::PushLayer(Layer *module)
    {
        m_ModuleStage.PushModule(module);
        module->OnAttach();
    }

    void Application::PushOverlay(Layer *module)
    {
		m_ModuleStage.PushOverlay(module);
        module->OnAttach();
    }

    void Application::PopLayer(Layer *module)
    {
		m_ModuleStage.PopModule(module);
        module->OnDetach();
    }

    void Application::PopOverlay(Layer *module)
    {
		m_ModuleStage.PopOverlay(module);
        module->OnDetach();
    }

    void Application::SyncEvents()
    {
        std::scoped_lock lock(m_EventQueueMutex);
        for (auto &synced : m_EventQueue | std::views::keys)
        {
            synced = true;
        }
    }

	void Application::ProcessEvents()
	{
		Input::TransitionPressedKeys();
		Input::TransitionPressedButtons();
        Window::ProcessEvents();

		/*
		 * NOTE: we have no control over what func() does.  holding this lock while calling func() is a bad idea:
		 * 1) func() might be slow (means we hold the lock for ages)
		 * 2) func() might result in events getting queued, in which case we have a deadlock
         */
		std::scoped_lock lock(m_EventQueueMutex);

		/*
		 * Process custom event queue, up until we encounter an event that is not yet synced
		 * If application queues such events, then it is the application's responsibility to call
		 * SyncEvents() at the appropriate time.
         */
		while (!m_EventQueue.empty())
		{
			const auto& [synced, func] = m_EventQueue.front();
			if (!synced) break;

            func();
			m_EventQueue.pop_front();
		}
	}

	void Application::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowResizeEvent>([this](const WindowResizeEvent& e) { return OnWindowResize(e); });
		dispatcher.Dispatch<WindowMinimizeEvent>([this](const WindowMinimizeEvent& e) { return OnWindowMinimize(e); });
		dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& e) { return OnWindowClose(e); });

		for (auto it = m_ModuleStage.end(); it != m_ModuleStage.begin(); )
		{
			(*--it)->OnEvent(event);
			if (event.m_Handled) break;
		}

		if (event.m_Handled) return;

		/*
		 * TODO: Should these callbacks be called BEFORE the layers receive events?
		 * We may actually want that since most of these callbacks will be functions REQUIRED in order for the game
		 * to work, and if a layer has already handled the event we may end up with problems.
         */
		for (auto& eventCallback : m_EventCallbacks)
		{
			eventCallback(event);
			if (event.m_Handled) break;
		}

	}

	bool Application::OnWindowResize(const WindowResizeEvent& e)
	{
		const uint32_t width = e.GetWidth(), height = e.GetHeight();
		if (width == 0 || height == 0)
		{
			//m_IsMinimized = true;
			return false;
		}
        //m_IsMinimized = false;

		return false;
	}

	bool Application::OnWindowMinimize(const WindowMinimizeEvent& e)
	{
        m_IsMinimized = e.IsMinimized();
		return false;
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		Stop();
		return false; // give other things a chance to react to window close
	}

	float Application::GetTime() const { return static_cast<float>(glfwGetTime()); }
    const char* Application::GetConfigurationName() { return SEDX_BUILD_TYPE; }
    const char* Application::GetPlatformName() { return SEDX_PLATFORM_NAME; }
	std::thread::id Application::GetMainThreadID() { return s_MainThreadID; }
    bool Application::IsMainThread() { return std::this_thread::get_id() == s_MainThreadID; }

}

// -------------------------------------------------------------------------

/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray 
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * application.cpp
 * -------------------------------------------------------
 * Created: 25/5/2025
 * -------------------------------------------------------
 */
#include "application.h"
#include "SceneryEditorX/core/input/input.h"
#include "SceneryEditorX/logging/logging.hpp"
#include "SceneryEditorX/renderer/vulkan/renderer.h"
#include "SceneryEditorX/renderer/vulkan/swapchain.h"
#include "SceneryEditorX/ui/ui_layer.h"
#include <imgui_impl_sdl3.h>
#include <tracy/Tracy.hpp>

// -------------------------------------------------------

bool appRunning = true; // Global variable to control the application loop

// -------------------------------------------------------

namespace SceneryEditorX
{
    Application *Application::appInstance = nullptr;
    static std::thread::id s_MainThreadID;

    // -------------------------------------------------------

    /**
     * @brief Helper function to initialize the application with common setup
     * @param appData The application data configuration
     */
    void Application::InitializeApplication(const AppData &appData)
    {
        m_AppData = appData;

        // Set working directory if specified
        if (!appData.WorkingDirectory.empty())
        {
            std::filesystem::current_path(appData.WorkingDirectory);
        }

		// Create window
		m_Window = CreateScope<Window>();
		SEDX_CORE_INFO("Initializing Window");
		m_Window->Create();
		
		// Configure window properties BEFORE renderer init
		if (appData.SplashScreen)
		{
		    m_Window->SetResizable(false);
		    m_Window->SetDecorated(false);
		}
		else
		{
		    m_Window->SetResizable(appData.Resizable);
		    m_Window->SetDecorated(appData.Decorated);
		}
		
		if (appData.StartMaximized)
		{
		    m_Window->Maximize();
		}
		else
		{
		    m_Window->CenterWindow();
		}
		
		// Set event callback before renderer init (so it can handle any initialization events)
		m_Window->SetEventCallback([this](Event &e) { OnEvent(e); });

		Renderer::Init();

		m_IsRunning = true;
		m_IsMinimized = false;
    }

    // -------------------------------------------------------

    Application::Application(const PlatformContext& context) : m_PlatformContext(&context)
    {
        appInstance = this;
        s_MainThreadID = std::this_thread::get_id();

        // Set working directory to application root (2 levels up from bin/Debug)
        std::filesystem::path exePath = std::filesystem::current_path();
        std::filesystem::path repoRoot = exePath.parent_path().parent_path();
        std::filesystem::current_path(repoRoot);

        SEDX_CORE_INFO("Executable directory: {}", exePath.string());
        SEDX_CORE_INFO("Repository root: {}", repoRoot.string());
        SEDX_CORE_INFO("Working directory set to: {}", std::filesystem::current_path().string());

        // -------------------------------------------------------

        SEDX_CORE_INFO("=== Initializing Application with PlatformContext ===");
        SEDX_CORE_INFO("  Working Directory: {}", context.GetWorkingDirectory());
        SEDX_CORE_INFO("  Temp Directory: {}", context.GetTempDirectory());
        SEDX_CORE_INFO("  Command Line Args: {}", context.GetCommandLineArgs().size());


        AppData specification;
        
        // Apply platform context settings to app data
        if (!context.GetWorkingDirectory().empty())
        {
            specification.WorkingDirectory = context.GetWorkingDirectory();
        }
        
        InitializeApplication(specification);
    }

    Application::Application(const PlatformContext& context, const AppData& appData) : m_PlatformContext(&context)
    {
        appInstance = this;
        s_MainThreadID = std::this_thread::get_id();

        // Set working directory to application root (2 levels up from bin/Debug)
        std::filesystem::path exePath = std::filesystem::current_path();
        std::filesystem::path repoRoot = exePath.parent_path().parent_path();
        std::filesystem::current_path(repoRoot);

        SEDX_CORE_INFO("Executable directory: {}", exePath.string());
        SEDX_CORE_INFO("Repository root: {}", repoRoot.string());
        SEDX_CORE_INFO("Working directory set to: {}", std::filesystem::current_path().string());

        // -------------------------------------------------------

        SEDX_CORE_INFO("=== Initializing Application with PlatformContext ===");
        SEDX_CORE_INFO("  Working Directory: {}", context.GetWorkingDirectory());
        SEDX_CORE_INFO("  Temp Directory: {}", context.GetTempDirectory());
        SEDX_CORE_INFO("  Command Line Args: {}", context.GetCommandLineArgs().size());

        AppData specification = appData;
        
        // Apply platform context settings if not already set in appData
        if (specification.WorkingDirectory.empty() && !context.GetWorkingDirectory().empty())
        {
            specification.WorkingDirectory = context.GetWorkingDirectory();
        }
        
        InitializeApplication(specification);
    }

    Application::~Application()
    {
        // Ensure renderer subsystems are torn down first
        Renderer::Shutdown();

        /** 
         * Let RAII handle Window destruction, or explicitly reset the RefCounter once
         * to avoid double-destruction. Do NOT call the destructor directly.
         */
        if (m_Window)
        {
            m_Window->Destroy();
            m_Window.reset();
        }
    }

    void Application::Tick()
    {
        Input::Tick();

        // Per-frame housekeeping
        Window::Tick();
        Renderer::Tick();
    }

    void Application::Run()
    {
        OnInit(); // Call user-defined initialization function

        // Main application loop
        while (m_IsRunning && !m_Window->GetShouldClose())
        {
            static uint64_t frameCount = 0;

            // Poll events
            ProcessEvents();

            // Skip rendering if minimized
            if (m_IsMinimized || Window::IsMinimized())
            {
                continue;
            }

            // Begin frame rendering
            if (!Renderer::BeginFrame())
            {
                // Frame acquisition failed (e.g., swapchain out of date)
                continue;
            }

            // Call application/editor update
            Tick();

            // ImGui frame
            //ImGui_ImplSDL3_NewFrame();
            //ImGui::NewFrame();

            // Call user rendering (panels, viewports, etc.)
            OnRender();

            // Finalize ImGui rendering
            //ImGui::Render();
            
            // Handle multi-viewport windows
            /*
            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }
            */

            // End frame and submit
            Renderer::EndFrame();
            Renderer::SubmitAndPresent();

            frameCount++;
        }

        //SEDX_CORE_INFO_TAG("APP", "=== Exiting Application Main Loop (frames rendered: {}) ===", frameCount);
        OnShutdown();
    }

    void Application::Stop() { m_IsRunning = false; }

    void Application::OnRender()
    {
        // Override in derived class (Editor::OnRender())
    }

    void Application::OnShutdown()
    {
        SEDX_CORE_INFO_TAG("APP", "Application::OnShutdown()");
        SEDX_CORE_INFO("Shutting down application");
        m_EventCallbacks.clear();
        appRunning = false;
    }

    void Application::PushLayer(Layer *module)
    {
        m_ModuleStage.PushLayer(module);
        module->OnAttach();
    }

    void Application::PushOverlay(Layer *module)
    {
		m_ModuleStage.PushOverlay(module);
        module->OnAttach();
    }

    void Application::PopLayer(Layer *module)
    {
		m_ModuleStage.PopLayer(module);
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
        
		for (auto it = m_ModuleStage.End(); it != m_ModuleStage.Begin(); )
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
		const uint32_t height = e.GetHeight();
		if (const uint32_t width = e.GetWidth(); width == 0 || height == 0)
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

	float Application::GetTime() { return Time::GetTime(); }
    const char* Application::GetConfigurationName() { return SEDX_BUILD_TYPE; }
    const char* Application::GetPlatformName() { return SEDX_PLATFORM_NAME; }
	std::thread::id Application::GetMainThreadID() { return s_MainThreadID; }
    bool Application::IsMainThread() { return std::this_thread::get_id() == s_MainThreadID; }

}

// -------------------------------------------------------------------------

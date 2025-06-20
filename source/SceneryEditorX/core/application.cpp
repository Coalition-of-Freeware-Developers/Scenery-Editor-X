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
#include <SceneryEditorX/core/application.h>
#include <SceneryEditorX/logging/logging.hpp>
// #include <SceneryEditorX/renderer/renderer.h>

/// -------------------------------------------------------

extern bool appRunning; /// Global variable to control the application loop

/// -------------------------------------------------------

namespace SceneryEditorX
{
    Application *Application::appInstance = nullptr;

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

    const char* Application::GetConfigurationName()
    {
        return SEDX_BUILD_TYPE;
    }

    const char* Application::GetPlatformName()
    {
        return SEDX_PLATFORM_NAME;
    }

}

/// -------------------------------------------------------------------------

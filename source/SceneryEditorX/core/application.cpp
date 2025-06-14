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

/// -------------------------------------------------------

extern bool appRunning; /// Global variable to control the application loop

/// -------------------------------------------------------

namespace SceneryEditorX
{
    Application *Application::appInstance = nullptr;

    Application::Application(const AppData &appData)
    {
        SEDX_CORE_INFO("Creating application with window: {}x{}", appData.WinWidth, appData.WinHeight);

        appInstance = this;

        /// Create the window
        m_Window = CreateScope<Window>();

        /// Set window properties from appData
        if (WindowData::width > 0 && WindowData::height > 0)
        {
            /// Update window properties if provided
            WindowData::width = appData.WinWidth;
            WindowData::height = appData.WinHeight;
        }

        if (WindowData::title)
            m_Window->SetTitle(WindowData::title);




        /// Update window properties
        m_Window->ApplyChanges();

        isRunning   = true;
        isMinimized = false;
    }

    void Application::Run()
    {
        OnInit();
        
        /// Main application loop
        while (isRunning && !m_Window->GetShouldClose())
        {
            /// Update the window (poll events)
            Window::Update();
            
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

    const char* Application::GetConfigType()
    {
        #ifdef SEDX_DEBUG
        return "Debug";
        #else
        return "Release";
        #endif
    }

    const char* Application::GetPlatform()
    {
        #ifdef _WIN32
        return "Windows";
        #elif defined(__APPLE__)
        return "macOS";
        #elif defined(__linux__)
        return "Linux";
        #else
        return "Unknown";
        #endif
    }

}

/// -------------------------------------------------------------------------

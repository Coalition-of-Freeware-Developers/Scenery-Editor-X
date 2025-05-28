/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* main.cpp
* -------------------------------------------------------
* Created: 16/3/2025
* -------------------------------------------------------
*/
#include <CrashHandler/crash_handler.h>
#include <Launcher/core/launcher_main.h>
#include <SceneryEditorX/core/window.h>
#include <SceneryEditorX/EntryPoint.h>
#include <SceneryEditorX/platform/settings.h>

#include "editor.h"

#ifdef SEDX_PLATFORM_WINDOWS
#include <Shlobj.h>
#endif // SEDX_PLATFORM_WINDOWS

/// -------------------------------------------------------

/*
std::string getDumpDirectory()
{
    return std::filesystem::temp_directory_path().string();
}
*/

// Initialize the application log and crash handler
void initCrashHandlerServices()
{
    // Initialize the application logging system
    SceneryEditorX::Log::Init();
    SceneryEditorX::Log::LogHeader();
    EDITOR_INFO("Scenery Editor X Engine is starting...");

    // Initialize the Crash Handler
    /*
    CrashHandler::CrashService::CrashHandlerConfig config;
    config.appVersion = SEDX_VERSION_STRING;
    config.dumpDir = getDumpDirectory();
    config.onCrashDetectedCallback = [](const std::string &dumpPath)
    {
        // Optional callback - called in the crash context, keep it minimal
        spdlog::critical("Crash detected, dump saved to: {}", dumpPath);
    };
    CrashHandler::CrashService::Init(config);
    */
}

// Shutdown application services
void endCrashHandlerServices()
{
    // Shut down crash handler before exit
    // CrashHandler::CrashService::Shutdown();

    EDITOR_INFO("Scenery Editor X Engine is shutting down...");
    SceneryEditorX::Log::FlushAll(); // Make sure to flush logs before shutdown
    SceneryEditorX::Log::ShutDown();
}

namespace SceneryEditorX
{
    class EditorX : public Application
    {
    public:
        EditorX(const WindowData &windowData, std::string_view projPath) 
            : Application(windowData), m_ProjectPath(projPath)
        {
            if (projPath.empty())
                m_ProjectPath = "SceneryEditorX/Projects/Default.edX";
                
            // Initialize application services
            initCrashHandlerServices();
        }

        virtual ~EditorX() override
        {
            // Clean up application services
            endCrashHandlerServices();
        }

        virtual void OnInit() override  
        {  
            // Initialize the user settings  
            m_UserSettings = CreateRef<ApplicationSettings>("settings.cfg");
            if (!m_UserSettings->ReadSettings())  
            {  
                EDITOR_ERROR_TAG("Core", "Failed to initialize user settings for project: {}", m_ProjectPath);  
                return;  
            }  
            
            try 
            {
                // Create and initialize the editor application
                m_EditorApp = CreateScope<EditorApplication>();
                m_EditorApp->InitEditor();
            }
            catch (const std::exception &e)
            {
                EDITOR_ERROR_TAG("Core", "Failed to initialize editor: {}", e.what());
            }
        }
        
        virtual void OnUpdate() override
        {
            if (m_EditorApp)
            {
                m_EditorApp->Update();
            }
        }
        
        virtual void OnShutdown() override
        {
            // Clean up editor application before the main Application is destroyed
            if (m_EditorApp)
            {
                m_EditorApp.reset();
            }
            
            Application::OnShutdown();
        }

    private:
        std::string m_ProjectPath;
        Ref<ApplicationSettings> m_UserSettings{};
        Scope<EditorApplication> m_EditorApp{};
    };

    // Implementation of CreateApplication within the SceneryEditorX namespace
    // This matches the declaration in EntryPoint.h
    Application* CreateApplication(int argc, char** argv)
    {
        // Parse command line arguments for project path
        std::string_view projectPath;
        if (argc > 1)
        {
            projectPath = argv[1];
        }

        // Configure window data
        WindowData windowData;
        windowData.title = "Scenery Editor X";
        windowData.width = 1280;
        windowData.height = 720;
        windowData.resizable = true;
        windowData.maximized = true;

        // Create and return the editor application
        return new EditorX(windowData, projectPath);
    }
}

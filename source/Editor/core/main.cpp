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
 * main.cpp
 * -------------------------------------------------------
 * Created: 16/3/2025
 * -------------------------------------------------------
 */
#include <Editor/core/editor.h>
#include <SceneryEditorX/core/platform/entrypoint.h>
#include <SceneryEditorX/core/platform/settings/settings.h>

/// -------------------------------------------------------

/*
std::string getDumpDirectory()
{
    return std::filesystem::temp_directory_path().string();
}
*/

// Initialize the application log and crash handler
void InitCrashHandlerServices()
{
    // Initialize the application logging system
    //SceneryEditorX::Log::Init();
    //SceneryEditorX::Log::LogHeader();
    //EDITOR_INFO("Scenery Editor X Engine is starting...");

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
void EndCrashHandlerServices()
{
    // Shut down crash handler before exit
    // CrashHandler::CrashService::Shutdown();

    //EDITOR_INFO("Scenery Editor X Engine is shutting down...");
    //SceneryEditorX::Log::FlushAll(); // Make sure to flush logs before shutdown
    //SceneryEditorX::Log::ShutDown();
}

/*
namespace SceneryEditorX
{
    class EditorX : public Application
    {
    public:
        EditorX(const AppData &appData, std::string_view projPath) : Application(appData), m_ProjectPath(projPath)
        {
            if (projPath.empty()) m_ProjectPath = "..\\Projects\\Default.edX";
                
            // Initialize application services
            InitCrashHandlerServices();
        }

        virtual ~EditorX() override
        {
            // Clean up application services
            EndCrashHandlerServices();
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
                if (!m_EditorApp)
                    m_EditorApp = CreateScope<Editor>();

                m_EditorApp->InitEditor();
            }
            catch (const std::exception &e)
            {
                EDITOR_ERROR_TAG("Core", "Failed to initialize editor: {}", e.what());
            }
        }
        
        void Tick() override
        {
            if (m_EditorApp)
                m_EditorApp->Tick();
        }
        
        void OnShutdown() override
        {
            // Clean up editor application before the main Application is destroyed
            if (m_EditorApp) m_EditorApp.reset();

            m_UserSettings.Reset();
            Application::OnShutdown();
        }

    private:
        std::string m_ProjectPath;
        Ref<ApplicationSettings> m_UserSettings{};
        Scope<Editor> m_EditorApp{};
    };
}
*/

// -------------------------------------------------------

Application* SceneryEditorX::CreateApplication(const PlatformContext& context)
{
    // Return a new instance of the editor application using PlatformContext
    return new Editor(context);
}

// -------------------------------------------------------

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
#include <cstdlib>
#include <exception>
#include <Editor/core/editor.h>
#include <Launcher/core/directory_manager.hpp>
#include <Launcher/core/launcher.h>
#include <Launcher/core/splash_handler.h>
#include <Launcher/registry/reg_check.h>
#include <SceneryEditorX/EntryPoint.h>
#include <SceneryEditorX/platform/settings/settings.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

	/*
	class Launcher : public Application
	{
	public:
        Launcher(const AppData &appData, std::string_view projPath) : Application(appData), m_ProjectPath(projPath)
        {
            if (projPath.empty()) m_ProjectPath = "SceneryEditorX/Projects/Default.edX";
        }

        virtual ~Launcher() override;

		virtual void OnInit() override
        {
            // Initialize the user settings
            m_UserSettings = CreateRef<ApplicationSettings>("settings.cfg");
            if (!m_UserSettings->ReadSettings())
            {
                LAUNCHER_ERROR_TAG("Core", "Failed to initialize user settings for project: {}", m_ProjectPath);
                return;
            }

            try
            {
                m_Launcher->InitLauncher();
            }
            catch (const std::exception &e)
            {
                LAUNCHER_ERROR_TAG("Core", "Failed to initialize Launcher: {}", e.what());
            }
        }

	    void OnUpdate() override
        {
            if (m_EditorXLauncher)
                m_EditorXLauncher->Update();
        }

	    void OnShutdown() override
        {
            if (m_EditorXLauncher)
                m_EditorXLauncher.reset();

            LAUNCHER_CORE_TRACE("Launcher has completed execution.");
            Application::OnShutdown();
        }

	private:
        std::string m_ProjectPath;
        Ref<ApplicationSettings> m_UserSettings{};
        Scope<Launcher> m_EditorXLauncher{};
	};
	*/

}

void SplashHandler::CreateSplashScreen(GLFWwindow* splash)
{
    glfwWindowHint(GLFW_CLIENT_API,GLFW_NO_API);
    splash = glfwCreateWindow(978,526,"Scenery Editor X",nullptr,nullptr);

    if (!splash)
    {
        LAUNCHER_CORE_ERROR("Failed to create splash screen window.");
        return;
    }

    glfwMakeContextCurrent(splash);
    while (!glfwWindowShouldClose(splash))
    {
        glClear(GL_COLOR_BUFFER_BIT);
        glfwSwapBuffers(splash);
        glfwPollEvents();
    }
}

static void SplashImg()
{
    int width,height,channels;

    std::ifstream file(R"(..\..\resources\splash_screen.png)",std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        LAUNCHER_CORE_ERROR("Failed to open splash screen image!");
        return;
    }

    std::streamsize size = file.tellg();
    file.seekg(0,std::ios::beg);
}

SceneryEditorX::Application *SceneryEditorX::CreateApplication(const std::vector<std::string> &args)
{

    // Return a new instance of the editor application
    //return new Launcher(args);
}

/// -------------------------------------------------------

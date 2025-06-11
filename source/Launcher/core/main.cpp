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
#include <Launcher/core/directory_manager.hpp>
#include <Launcher/core/launcher_main.h>
#include <Launcher/core/splash_handler.h>
#include <Launcher/core/updater.h>
#include <Launcher/registry/reg_check.h>
#include <SceneryEditorX/core/window.h>
#include <SceneryEditorX/EntryPoint.h>
#include <SceneryEditorX/platform/settings.h>
#include <SceneryEditorX/renderer/vulkan/vk_core.h>
#include <synchapi.h>

/// -------------------------------------------------------

#ifdef SEDX_PLATFORM_WINDOWS
#endif // SEDX_PLATFORM_WINDOWS

/// -------------------------------------------------------

namespace Launcher
{
    class PreLoader : public SceneryEditorX::Application
    {
    public:
        PreLoader(const SceneryEditorX::WindowData &appData, std::string_view projPath)
            : Application(appData), splashHandler(nullptr), m_ProjectPath(projPath)
        {
            if (projPath.empty())
                m_ProjectPath = "SceneryEditorX/Projects/Default.edX";

        }

		virtual ~PreLoader() override;

		virtual void OnInit() override
		{
            m_UserSettings = CreateRef<SceneryEditorX::ApplicationSettings>("settings.cfg");
            if (!m_UserSettings->ReadSettings())
                EDITOR_ERROR_TAG("Core", "Failed to initialize user settings for project: {}", m_ProjectPath);

		    //TODO: Implement the admin check function

            //AdminCheck();
            CreateSplash();
            MainLoop();

        }

		virtual void OnUpdate() override
        {

        }

		virtual void OnShutdown() override
        {

            SplashHandler::DestroySplashScreen(); /// Close splash screen
			delete splashHandler;                 /// Delete splash screen object

			LAUNCHER_LOG_INFO("Launcher has completed execution.");

        }

    private:
        SplashHandler *splashHandler;
        std::string m_ProjectPath;
        Ref<SceneryEditorX::ApplicationSettings> m_UserSettings{};
        Ref<SceneryEditorX::Window> m_Window;

        /// Counter to make sure we wait for all threads to finish
        std::mutex mtThreadCounter;
        int intThreadCount{0};

		/// ---------------------------------------------------------

		static void InitPreloader();
        void PerformPreloading();
        void CreateSplash();
        void MainLoop();
        void OperationThreads();
		void CreateViewportResources();
        void CleanupViewportResources();
        void OnSurfaceUpdate(uint32_t width, uint32_t height);
        void RecreateFrameResources();
    };
	namespace UI
	{
	    class UIContextImpl;
	}

    /*
    GLOBAL void AdminCheck()
    {

        LAUNCHER_LOG_INFO("Current working directory: {}",std::filesystem::current_path().string());
        LAUNCHER_LOG_INFO("Launcher is starting...");

        // Get the command line arguments
        LPSTR lpCmdLine = GetCommandLine();

        // Check if the application is running with the "--elevated" argument
        bool isElevated = strstr(lpCmdLine,"--elevated") != nullptr;
        LAUNCHER_LOG_INFO("Command line: {}",lpCmdLine);
        //std::cout << "Command line: " << lpCmdLine << std::endl;
        LAUNCHER_LOG_INFO("Is elevated: {}",isElevated);
        //std::cout << "Is elevated: " << isElevated << std::endl;

        if (!isElevated  && !RunningAsAdmin())   // Check if the application is running as administrator
        {
            LAUNCHER_LOG_INFO("Not running as administrator. Attempting to relaunch...");
            //std::cout << "Not running as administrator. Attempting to relaunch..." << std::endl;

            RelaunchAsAdmin(); // Relaunch the application as administrator
        }

        LAUNCHER_LOG_INFO("Running as administrator. Proceeding with execution.");
        //std::cout << "Running as administrator. Proceeding with execution." << std::endl;

        Launcher::Loader loader{};          // Create the loader object
        loader.run();                       // Run the loader
    }
    */


    void PreLoader::PerformPreloading()
    {
        /// TODO: Add Scenery Gateway API pull and cache
        /// Get All Airports (GET /apiv1/airports)
        /// ------------------------------------------
        /// TODO: Add Check for app updates
        /// TODO: Add Check for directory to X-Plane 12
        /// This can be done by checking the registry for the X-Plane 12 directory
        ///	TODO: Add precompiling of shaders
        ///	TODO: Add loading of textures
        ///	TODO: Add loading of models
        ///	TODO: If launched from a project, load the project data

        Updater updater;
        updater.UpdateCheck();

        Sleep(6000);

        LAUNCHER_LOG_INFO("Preloading tasks started.");

        RegistryCheck();

        LAUNCHER_LOG_INFO("Registry check complete");

        DirectoryInit();

        LAUNCHER_LOG_INFO("Directory check complete");

        Sleep(6000);

        LAUNCHER_LOG_INFO("Loading resources.");

		/*
        TODO: Add loading of resources here
         - Add Precompiled Shaders
         - Add Textures
         - Add Models
         - Config Files
         - Add Scenery Gateway Data
         - Add Default edX Scene files
		*/

        LAUNCHER_LOG_INFO("Preloading tasks completed.");

        mtThreadCounter.lock();
        intThreadCount--;
        mtThreadCounter.unlock();

    }

    void PreLoader::CreateSplash()
    {
        ///Create the splash screen window
        splashHandler = new SplashHandler();
        GLFWwindow* splash = nullptr;
        SplashHandler::CreateSplashScreen(splash);
    }

    void PreLoader::MainLoop()
    {
        /// Perform the operations in separate threads
        OperationThreads();

	    while (!SceneryEditorX::Window::GetShouldClose())
        {
            //Sleep(1000);
            mtThreadCounter.lock();
            if (intThreadCount == 0)
            {
                mtThreadCounter.unlock();
                break;
            }
            mtThreadCounter.unlock();
			DrawFrame();
            SceneryEditorX::Window::Update();
        }

		vkDeviceWaitIdle(device);
    }

    void PreLoader::OperationThreads()
    {
        /// Create a thread to perform the preloading tasks
        mtThreadCounter.lock();
        intThreadCount++;
        mtThreadCounter.unlock();
        std::thread preloadThread(&PreLoader::PerformPreloading,this);
        preloadThread.detach();
    }

} // namespace Launcher

void SplashHandler::CreateSplashScreen(GLFWwindow* splash)
{
    glfwWindowHint(GLFW_CLIENT_API,GLFW_NO_API);
    splash = glfwCreateWindow(978,526,"Scenery Editor X",nullptr,nullptr);

    if (!splash)
    {
        LAUNCHER_LOG_ERROR("Failed to create splash screen window.");
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

    std::ifstream file(R"(..\..\assets\splash_screen.png)",std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        LAUNCHER_LOG_ERROR("Failed to open splash screen image!");
        return;
    }

    std::streamsize size = file.tellg();
    file.seekg(0,std::ios::beg);
}

namespace SceneryEditorX
{
    Application* CreateApplication(int argc, char** argv)
    {
        /// Parse command line arguments for project path
        std::string_view projectPath;
        if (argc > 1)
        {
            projectPath = argv[1];
        }

        /// Configure window data
        WindowData windowData;
        windowData.title = "Scenery Editor X";
        windowData.width = 978;
        windowData.height = 526;
        windowData.resizable = false;
        windowData.maximized = false;
        windowData.decorated = false;

        return new Launcher::PreLoader(windowData, projectPath);
    }

} // namespace SceneryEditorX

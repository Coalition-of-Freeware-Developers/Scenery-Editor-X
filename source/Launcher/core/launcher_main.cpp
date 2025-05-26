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
#include <SceneryEditorX/core/window.h>
#include <SceneryEditorX/renderer/vk_core.h>
#include <Launcher/core/directory_manager.hpp>
#include <Launcher/core/launcher_main.h>
#include <Launcher/core/splash_handler.h>
#include <Launcher/core/updater.h>
#include <Launcher/registry/reg_check.h>
#include <synchapi.h>

// -------------------------------------------------------

namespace Launcher
{
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

	void Loader::run()
	{
		//TODO: Implement the admin check function

		//AdminCheck();
		CreateSplash();
		MainLoop();
		CleanUp();
	}

    void Loader::PerformPreloading()
    {
        // TODO: Add Scenery Gateway API pull and cache
        // Get All Airports (GET /apiv1/airports)
        // ------------------------------------------
        // TODO: Add Check for app updates
        // TODO: Add Check for directory to X-Plane 12
        // This can be done by checking the registry for the X-Plane 12 directory

        Updater updater;
        updater.UpdateCheck();

        Sleep(6000);

        LAUNCHER_LOG_INFO("Preloading tasks started.");
        //std::cout << "Preloading tasks started." << std::endl;

        RegistryCheck();

        LAUNCHER_LOG_INFO("Registry check complete");
        //std::cout << "Registry check complete" << std::endl;

        DirectoryInit();

        LAUNCHER_LOG_INFO("Directory check complete");
        //std::cout << "Directory check complete" << std::endl;

        Sleep(6000);

        LAUNCHER_LOG_INFO("Loading resources.");
        //std::cout << "Loading resources." << std::endl;

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
        //std::cout << "Preloading tasks completed." << std::endl;

        mtThreadCounter.lock();
        intThreadCount--;
        mtThreadCounter.unlock();

    }

    void Loader::CreateSplash()
    {
        //Create the splash screen window
        splashHandler = new SplashHandler();
        GLFWwindow* splash = nullptr;
        SplashHandler::CreateSplashScreen(splash);
    }

    void Loader::MainLoop()
    {
        // Perform the operations in separate threads
        OperationThreads();

	    while (!Window::GetShouldClose())
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
			Window::Update();
        }

		vkDeviceWaitIdle(device);
    }

    void Loader::OperationThreads()
    {
        // Create a thread to perform the preloading tasks
        mtThreadCounter.lock();
        intThreadCount++;
        mtThreadCounter.unlock();
        std::thread preloadThread(&Loader::PerformPreloading,this);
        preloadThread.detach();
    }

    void Loader::CleanUp() const
    {
        LAUNCHER_LOG_INFO("Cleaning up before relaunch.");
        //std::cout << "Cleaning up before relaunch." << std::endl;

        splashHandler->DestroySplashScreen(); // Close splash screen
        delete splashHandler;                 // Delete splash screen object

        LAUNCHER_LOG_INFO("Launcher has completed execution.");
        //std::cout << "Launcher has completed execution." << std::endl;

    }

	void Loader::DrawFrame()
	{
        if (viewportData.viewportSize.x != newViewportSize.x || viewportData.viewportSize.y != newViewportSize.y)
	    {
	        if (newViewportSize.x > 0 && newViewportSize.y > 0)
	        {
                viewportData.viewportSize = newViewportSize;
	            CleanupViewportResources();
	            CreateViewportResources();
	        }
	    }

        // Get a command buffer to render into
        VkCommandBuffer commandBuffer = vkRenderer.BeginSingleTimeCommands();

        // Set the command buffer for ImGui to render into
        ui.SetActiveCommandBuffer(commandBuffer);

        // Begin ImGui UI
        if (uiContext)
        {
            uiContext->Begin();

            // Draw your UI elements here
            ui.ShowDemoWindow();

            // Draw viewport if needed
            ui.ViewportWindow(viewportData.viewportSize, viewportData.viewportHovered, viewportData.viewportImageView);

            uiContext->End();
        }

        // End and submit the command buffer
        vkRenderer.EndSingleTimeCommands(commandBuffer);

        // Update frame counter
        frameCount = (frameCount + 1) % (1 << 15);
	}

	Loader::~Loader()
	{
	    vkRenderer.~Loader();
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

void SplashHandler::DestroySplashScreen()
{
    /*
    if (m_splashWindow)
    {
        glfwDestroyWindow(m_splashWindow);
        m_splashWindow = nullptr;
    }
    */
}

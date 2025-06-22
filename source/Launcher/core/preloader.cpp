/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* preloader.cpp
* -------------------------------------------------------
* Created: 21/6/2025
* -------------------------------------------------------
*/
#include <Launcher/core/preloader.h>
#include <thread>

#include <Launcher/registry/reg_check.h>

#include <SceneryEditorX/logging/logging.hpp>

#include "directory_manager.hpp"
#include "launcher.h"
#include "splash_handler.h"

/// -------------------------------------------------------
namespace SceneryEditorX
{
	void PreLoader::OperationThreads()
	{
	    /// Create a thread to perform the preloading tasks
	    mtThreadCounter.lock();
	    intThreadCount++;
	    mtThreadCounter.unlock();
	    std::thread preloadThread(&PreLoader::PerformPreloading, this);
	    preloadThread.detach();
	}

    /*
    GLOBAL void AdminCheck()
    {

        LAUNCHER_CORE_TRACE("Current working directory: {}",std::filesystem::current_path().string());
        LAUNCHER_CORE_TRACE("Launcher is starting...");

        // Get the command line arguments
        LPSTR lpCmdLine = GetCommandLine();

        // Check if the application is running with the "--elevated" argument
        bool isElevated = strstr(lpCmdLine,"--elevated") != nullptr;
        LAUNCHER_CORE_TRACE("Command line: {}",lpCmdLine);
        //std::cout << "Command line: " << lpCmdLine << std::endl;
        LAUNCHER_CORE_TRACE("Is elevated: {}",isElevated);
        //std::cout << "Is elevated: " << isElevated << std::endl;

        if (!isElevated  && !RunningAsAdmin())   // Check if the application is running as administrator
        {
            LAUNCHER_CORE_TRACE("Not running as administrator. Attempting to relaunch...");
            //std::cout << "Not running as administrator. Attempting to relaunch..." << std::endl;

            RelaunchAsAdmin(); // Relaunch the application as administrator
        }

        LAUNCHER_CORE_TRACE("Running as administrator. Proceeding with execution.");
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

        const Launcher updater;
        updater.UpdateCheck();

        Sleep(6000);

        LAUNCHER_CORE_TRACE("Preloading tasks started.");

        RegistryCheck();

        LAUNCHER_CORE_TRACE("Registry check complete");

        DirectoryInit();

        LAUNCHER_CORE_TRACE("Directory check complete");

        Sleep(6000);

        LAUNCHER_CORE_TRACE("Loading resources.");

        /**
         *TODO: Add loading of resources here
         * - Add Precompiled Shaders
         * - Add Textures
         * - Add Models
         * - Config Files
         * - Add Scenery Gateway Data
         * - Add Default edX Scene files
		 */

        LAUNCHER_CORE_TRACE("Preloading tasks completed.");

        mtThreadCounter.lock();
        intThreadCount--;
        mtThreadCounter.unlock();
    }

    void PreLoader::CreateSplash()
    {
        ///Create the splash screen window
        splashHandler = new SplashHandler();
        GLFWwindow *splash = nullptr;
        SplashHandler::CreateSplashScreen(splash);
    }


}

/// --------------------------------------------------------

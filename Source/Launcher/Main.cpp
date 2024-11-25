
#include <windows.h>
#include <iostream>
#include <thread>

#include "DirectoryManager.hpp"
#include "ElevatePerms.h"
#include "Logging.hpp"
#include "RegCheck.h"
#include "SplashHandler.h"
#include "Updater.h"
#include <cstdlib>
#include <exception>
#include <spdlog/spdlog.h>
#include <synchapi.h>

namespace Launcher
{
    class Loader
    {
    public:
        void run()
        {
            adminCheck();
            CreateSplash();
            MainLoop();
            CleanUp();
        }
    
    private:
        static void PerformPreloading()
        {
            // TODO: Add Scenery Gateway API pull and cache
            // Get All Airports (GET /apiv1/airports)
            //
            // TODO: Add Check for app updates
            // TODO: Add Check for directory to X-Plane 12
            // This can be done by checking the registry for the X-Plane 12 directory
            Updater updater;
            updater.UpdateCheck();
            Sleep(600);
            spdlog::info("Preloading tasks started.");
            RegistryCheck();
            spdlog::info("Registry check complete");
            DirectoryInit();
            spdlog::info("Directory check complete");
            Sleep(600);
            spdlog::info("Loading resources.");
    
            spdlog::info("Preloading tasks completed.");
        }
    
        static void CreateSplash()
        {
            // Create the splash screen window
            SplashHandler splashHandler{};
            splashHandler.CreateSplashScreen();
        }
    
        static void MainLoop()
        {
            // Perform the operations in separate threads
            OperationThreads();
        }
    
        static void OperationThreads()
        {
            // Create a thread to perform the preloading tasks
            std::thread preloadThread(&Launcher::Loader::PerformPreloading);
            preloadThread.detach();
        }
    
        static void CleanUp()
        {
            spdlog::info("Cleaning up before relaunch.");
            SplashHandler splashHandler{};
            splashHandler.DestroySplashScreen(); // Close splash screen
            Log::Shutdown();                     // Shut down logging system
        }
    };
} // namespace Launcher

// Entry point
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    Log::Init();
    Log::LogHeader();
    spdlog::info("Launcher is starting...");

    if (strstr(lpCmdLine, "--elevated") == nullptr) // Check if elevated flag is present
    {
        if (!RunningAsAdmin())
        {
            spdlog::info("Not running as administrator. Attempting to relaunch...");
            RelaunchAsAdmin();
            return EXIT_SUCCESS; // Exit non-elevated instance
        }
    }

    spdlog::info("Running as administrator. Proceeding with execution.");
    try
    {
        Launcher::Loader splashScreen{};
        splashScreen.run();
    }
    catch (const std::exception &e)
    {
        spdlog::error("An exception occurred: {}", e.what());
        return EXIT_FAILURE;
    }

    return 0;
}

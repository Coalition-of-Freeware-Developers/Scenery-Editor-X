#include <windows.h>
#include <shellapi.h>
#include <thread>
#include <iostream>

#include "SplashHandler.h"
#include "Logging.hpp"
#include "DirectoryManager.hpp"
#include "RegCheck.h"
#include <synchapi.h>
#include <cstdlib>
#include <exception>
#include <sal.h>
#include <spdlog/spdlog.h>

namespace Launcher
{
    class Loader
    {
    public:

        void run()
        {
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
            UpdateCheck();
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
    
        static void OperationThreads()
        {
            // Create a thread to perform the preloading tasks
            std::thread preloadThread(&Launcher::Loader::PerformPreloading);
            preloadThread.detach();
        }
    
        static void MainLoop()
        {
            // Main loop
            MSG msg;
            while (GetMessage(&msg, nullptr, 0, 0))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    
        static void CleanUp()
        {
            spdlog::info("Launching main program.");
            // Clean up
            spdlog::info("Launcher is shutting down...");
            Log::Shutdown();
            // After preloading, launch the main program
            ShellExecute(nullptr, "open", "SceneryEditorX.exe", nullptr, nullptr, SW_SHOWDEFAULT);
    
            // Destroy the splash screen
            SplashHandler splashHandler{};
            splashHandler.DestroySplashScreen();
        }
    };
}

// Entry point
int APIENTRY WinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPSTR lpCmdLine,
                     _In_ int nCmdShow)
{
    Log::Init();
    Log::LogHeader();
    spdlog::info("Launcher is starting...");

    Launcher::Loader SplashScreen{};
    try
    {
        SplashScreen.run();
    }
    catch (const std::exception &e)
    {
        spdlog::error("An exception occurred: {}", e.what());
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return 0;
}

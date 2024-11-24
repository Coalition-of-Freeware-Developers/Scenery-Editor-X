#include <windows.h>
#include <shellapi.h>
#include <thread>
#include <iostream>
#include <string>
#include <sysinfoapi.h>

#include "resource.h"
#include "DirectoryManager.hpp"
#include "Logging.hpp"
#include "RegInit.h"
#include <filesystem>
#include "ElevatePerms.h"
#include "RegistryCheck.h"

namespace Launcher
{
    class Loader
    {
    public:
        void run()
        {
            CreateSplash();
            Setup();
            MainLoop();
            CleanUp();
        }

        //void Setup()
        //{
        //    OperationThreads();
        //    PerformPreloading();
        //}



    void CreateSplash()
    {

        }

        inline void OperationThreads()
        {
            // Create a thread to perform the preloading tasks
            std::thread preloadThread(&Loader::PerformPreloading, this);
            preloadThread.detach();
        }

        // Function to perform preloading tasks
        void PerformPreloading()
        {
            Sleep(600);
            spdlog::info("Preloading tasks started.");
    
            // TODO: Add Scenery Gateway API pull and cache
            // Get All Airports (GET /apiv1/airports)
            //
            // TODO: Add Check for app updates
            // TODO: Add Check for directory to X-Plane 12
            // This can be done by checking the registry for the X-Plane 12 directory
    
            Sleep(600);
            spdlog::info("Loading resources.");


            RegistryCheck();
            spdlog::info("Registry init complete");

            Sleep(8000);
            spdlog::info("Preloading tasks completed.");
        }

        /*
        // Function to perform preloading tasks
        void PerformPreloading()
        {
            // Simulate preloading tasks (replace with actual functions)
            Sleep(8000);
            spdlog::info("Preloading tasks completed.");

            spdlog::info("Launching main program.");
            // After preloading, launch the main program
            ShellExecute(nullptr, "open", "SceneryEditorX.exe", nullptr, nullptr, SW_SHOWDEFAULT);

            // Close the splash screen
            spdlog::info("Closing splash screen.");
            PostMessage(hSplashWnd, WM_CLOSE, 0, 0);
        }
        */

        void MainLoop()
        {
            // Main loop
            MSG msg;
            while (GetMessage(&msg, nullptr, 0, 0))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        void CleanUp() const
        {

            spdlog::info("Launching main program.");
            // After preloading, launch the main program
            ShellExecute(nullptr, "open", "SceneryEditorX.exe", nullptr, nullptr, SW_SHOWDEFAULT);

            // Close the splash screen
            spdlog::info("Closing splash screen.");
            PostMessage(hSplashWnd, WM_CLOSE, 0, 0);
        }
    };
} // namespace Launcher

// Entry point
int APIENTRY WinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPSTR lpCmdLine,
                     _In_ int nCmdShow)
{
    Launcher::Log::Init();
    Launcher::Log::LogHeader();
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

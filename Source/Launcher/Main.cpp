
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
            //adminCheck();
            CreateSplash();
            MainLoop();
            CleanUp();
        }
    
    private:
        SplashHandler *splashHandler{nullptr};

        //Counter to make sure we wait for all threads to finish
        std::mutex mtThreadCounter;
        int intThreadCount{0};

        void PerformPreloading()
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
            std::cout << "Preloading tasks started." << std::endl;

            RegistryCheck();

            spdlog::info("Registry check complete");
            std::cout << "Registry check complete" << std::endl;

            DirectoryInit();

            spdlog::info("Directory check complete");
            std::cout << "Directory check complete" << std::endl;

            Sleep(600);

            spdlog::info("Loading resources.");
            std::cout << "Loading resources." << std::endl;

            //TODO: Add loading of resources here
            // - Add Precompiled Shaders
            // - Add Textures
            // - Add Models
            // - Config Files
            // - Add Scenery Gateway Data
            // - Add Default edX Scene files
    
            spdlog::info("Preloading tasks completed.");
            std::cout << "Preloading tasks completed." << std::endl;

            mtThreadCounter.lock();
            intThreadCount--;
            mtThreadCounter.unlock(); 

        }
    
        void CreateSplash()
        {
            // Create the splash screen window
            splashHandler = new SplashHandler();
            splashHandler->CreateSplashScreen();
        }
    
        void MainLoop()
        {
            // Perform the operations in separate threads
            OperationThreads();

            while (true)
            {
                Sleep(1000);
                mtThreadCounter.lock();
                if (intThreadCount == 0)
                {
                    mtThreadCounter.unlock();
                    break;
                }
                mtThreadCounter.unlock();
            }
        }
    
        void OperationThreads()
        {
            // Create a thread to perform the preloading tasks
            mtThreadCounter.lock();
            intThreadCount++;
            mtThreadCounter.unlock();
            std::thread preloadThread(&Loader::PerformPreloading, this);
            preloadThread.detach();
        }
    
        void CleanUp()
        {
            spdlog::info("Cleaning up before relaunch.");
            std::cout << "Cleaning up before relaunch." << std::endl;

            splashHandler->DestroySplashScreen(); // Close splash screen
            delete splashHandler;                 // Delete splash screen object

            spdlog::info("Launcher has completed execution.");
            std::cout << "Launcher has completed execution." << std::endl;

            Log::Shutdown();                      // Shut down logging system
        }
    };
} // namespace Launcher

// Entry point
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    Log::Init();

    spdlog::info("Logger initialized. Starting application...");
    std::cout << "Logger initialized. Starting application..." << std::endl;
    spdlog::info("Current working directory: {}", std::filesystem::current_path().string());
    std::cout << "Current working directory: " << std::filesystem::current_path().string() << std::endl;

    Log::LogHeader();

    spdlog::info("Launcher is starting...");
    std::cout << "Launcher is starting..." << std::endl;

    // Check if the application is running with the "--elevated" argument
    bool isElevated = strstr(lpCmdLine, "--elevated") != nullptr;
    spdlog::info("Command line: {}", lpCmdLine);
    std::cout << "Command line: " << lpCmdLine << std::endl;
    spdlog::info("Is elevated: {}", isElevated);
    std::cout << "Is elevated: " << isElevated << std::endl;

    if (!isElevated && !RunningAsAdmin())   // Check if the application is running as administrator
    {
        spdlog::info("Not running as administrator. Attempting to relaunch...");
        std::cout << "Not running as administrator. Attempting to relaunch..." << std::endl;

        RelaunchAsAdmin();                  // Relaunch the application as administrator
    }

    spdlog::info("Running as administrator. Proceeding with execution.");
    std::cout << "Running as administrator. Proceeding with execution." << std::endl;

    Launcher::Loader loader{};          // Create the loader object
    loader.run();                       // Run the loader

    return 0;
}

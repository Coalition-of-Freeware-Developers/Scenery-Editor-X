/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* launcher_main.h
* -------------------------------------------------------
* Created: 17/3/2025
* -------------------------------------------------------
*/

#pragma once

#include <Launcher/core/splash_handler.h>

// -------------------------------------------------------

namespace Launcher
{
    class Loader
    {
    public:
        void run()
        {
            //TODO: Implement the admin check function

            //adminCheck();
            CreateSplash();
            MainLoop();
            CleanUp();
        }

    private:
        SplashHandler* splashHandler{nullptr};

        //Counter to make sure we wait for all threads to finish
        std::mutex mtThreadCounter;
        int intThreadCount{0};

        void PerformPreloading();
        void CreateSplash();
        void MainLoop();
        void OperationThreads();
        void CleanUp() const;
    };

    //GLOBAL void AdminCheck();

} // namespace Launcher

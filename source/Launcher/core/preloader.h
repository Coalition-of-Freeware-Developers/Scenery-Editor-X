/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* preloader.h
* -------------------------------------------------------
* Created: 21/6/2025
* -------------------------------------------------------
*/
#pragma once
#include <Launcher/core/splash_handler.h>
#include <SceneryEditorX/core/application.h>
#include <SceneryEditorX/logging/logging.hpp>

/// -------------------------------------------------------
namespace SceneryEditorX
{
    class PreLoader
    {
    public:
        PreLoader() : splashHandler(new SplashHandler())
		{
			LAUNCHER_CORE_TRACE("PreLoader initialized.");
        }

        ~PreLoader();

		virtual void OnInit()
		{
		    //TODO: Implement the admin check function
            //AdminCheck();
            CreateSplash();
            MainLoop();
        }

		virtual void OnShutdown()
        {
            SplashHandler::DestroySplashScreen(); /// Close splash screen
			delete splashHandler;                 /// Delete splash screen object
        }

    private:
        SplashHandler *splashHandler;
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

} // namespace SceneryEditorX

/// --------------------------------------------------------

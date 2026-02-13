/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray 
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * preloader.h
 * -------------------------------------------------------
 * Created: 21/6/2025
 * -------------------------------------------------------
 */
#pragma once
#include <Launcher/core/splash_handler.h>
#include <SceneryEditorX/core/application/application.h>
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

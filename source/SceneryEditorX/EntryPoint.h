/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* EntryPoint.h
* -------------------------------------------------------
* Created: 25/5/2025
* -------------------------------------------------------
*/
#pragma once
#include "core/initializer.h"
#include "core/application/application.h"
#include "logging/asserts.h"
#include <vector>
#include <string>

// ----------------------------------------------------

/**
* @brief Creates the application instance (implemented by client)
* 
* This function must be defined by the client application to create
* the specific Application subclass instance.
* 
* @param args Vector of command-line argument strings
* @return Pointer to the created Application instance
*/
extern SceneryEditorX::Application* CreateApplication(const std::vector<std::string>& args);

/**
* @brief Global flag controlling the application run loop
* Set to false to exit the application after the current iteration
*/
inline bool appRunning = true;

// ----------------------------------------------------

namespace SceneryEditorX
{
	/**
	* @brief Main application entry point (platform-independent)
	* 
	* This function initializes the core systems, creates the application,
	* runs the main loop, and performs cleanup. The loop continues while
	* appRunning is true, allowing for application restart functionality.
	* 
	* @param args Vector of command-line argument strings
	* @return Exit code (0 for success)
	*/
    int Main(const std::vector<std::string> &args)
	{
	    while (appRunning)
	    {
	        InitCore();

	        Application *app = CreateApplication(args);
	        SEDX_CORE_ASSERT(app, "Application is null!");
	        app->Run();
	        
	        delete app;
	        Shutdown();
	    }
	
	    return 0;
	}
}

// ----------------------------------------------------
// Platform-specific entry points
// ----------------------------------------------------

#if defined(SEDX_PLATFORM_WINDOWS)

	#include <Windows.h>
	#include <shellapi.h>

	/**
	* @brief Windows-specific entry point (GUI application)
	* 
	* Converts Windows command-line arguments from wide characters (UTF-16)
	* to UTF-8 strings and passes them to the platform-independent Main function.
	* 
	* @param hInstance Handle to the current instance of the application
	* @param hPrevInstance Always NULL (legacy parameter)
	* @param lpCmdLine Command-line string (unused, we use GetCommandLineW instead)
	* @param nCmdShow Controls how the window is to be shown
	* @return Exit code from Main function
	*/
	int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
	{
	    // Convert command line to argv-like format from wide characters
	    int argc;
        LPWSTR *argv_w = CommandLineToArgvW(GetCommandLineW(), &argc);
        std::vector<std::string> args;
        args.reserve(argc);
        for (int i = 0; i < argc; ++i)
        {
            // convert wide characters to normal string
            char arg[1024];
            WideCharToMultiByte(CP_UTF8, 0, argv_w[i], -1, arg, sizeof(arg), nullptr, nullptr);
            args.push_back(std::string(arg));
        }
        LocalFree(argv_w);

	    return SceneryEditorX::Main(args);
	}

	/**
	* @brief Windows console application entry point
	* 
	* Fallback entry point for when the application is built as a console application.
	* Converts standard argc/argv to vector and passes to Main function.
	* 
	* @param argc Number of command-line arguments
	* @param argv Array of command-line argument strings
	* @return Exit code from Main function
	*/
	int main(int argc, char** argv)
	{
	    std::vector<std::string> args;
	    args.reserve(argc);
	    for (int i = 0; i < argc; ++i)
	    {
	        args.push_back(std::string(argv[i]));
	    }
	    return SceneryEditorX::Main(args);
	}

#elif defined(SEDX_PLATFORM_LINUX) || defined(SEDX_PLATFORM_MACOS)

	/**
	* @brief Unix-like platform entry point (Linux and macOS)
	* 
	* Standard main function for POSIX-compliant platforms.
	* Converts argc/argv to vector and passes to the platform-independent Main function.
	* 
	* @param argc Number of command-line arguments
	* @param argv Array of command-line argument strings
	* @return Exit code from Main function
	*/
	int main(int argc, char **argv)
	{
	    std::vector<std::string> args;
	    args.reserve(argc);
	    for (int i = 0; i < argc; ++i)
	    {
	        args.push_back(std::string(argv[i]));
	    }
	    return SceneryEditorX::Main(args);
	}

#else
	#error "Unsupported platform! Please define SEDX_PLATFORM_WINDOWS, SEDX_PLATFORM_LINUX, or SEDX_PLATFORM_MACOS"
#endif

// ----------------------------------------------------

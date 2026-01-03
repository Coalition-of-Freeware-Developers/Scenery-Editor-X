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
#include "initializer.h"
#include <string>
#include <vector>
#include <SceneryEditorX/core/application/application.h>
#include <SceneryEditorX/core/platform/platform_context.h>
#include <SceneryEditorX/logging/asserts.h>

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
//extern SceneryEditorX::Application* CreateApplication(const std::vector<std::string>& args);

#if defined(PLATFORM__MACOS)
    #include <TargetConditionals.h>
#endif

/**
* @brief Global flag controlling the application run loop
* Set to false to exit the application after the current iteration
*/
//inline bool appRunning = true;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Platform-specific Entry Points																				//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
#if defined(SEDX_PLATFORM_WINDOWS)
	#include <Windows.h>

    extern std::unique_ptr<SceneryEditorX::PlatformContext> CreateApplication(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow);

    #define APP_MAIN(context_type)																		\
	int platform_initializer(const SceneryEditorX::PlatformContext &);									\																									\
	int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow)		\
	{																									\
		auto ctx = Create(hInstance, hPrevInstance, lpCmdLine, nCmdShow);								\
		return SceneryEditorX::Main(ctx->GetCommandLineArgs());											\
	}																									\
	int platform_initializer(const SceneryEditorX::PlatformContext &context_type)						\

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
    extern std::unique_ptr<SceneryEditorX::PlatformContext> CreateApplication(int argc, char **argv);

    #define APP_MAIN(context_type)													\
		int platform_initializer(const SceneryEditorX::PlatformContext &);			\
		int main(int argc, char *argv[])											\
		{																			\
		    auto ctx = Create(argc, argv);											\
			return SceneryEditorX::Main(*ctx);										\
		}																			\
		int platform_initializer(const SceneryEditorX::PlatformContext &context_type)

#else

	#include <stdexcept>
	#define APP_MAIN(constex_type)													\
		int main(int argc, char *argv[])											\
		{																			\
			throw std::runtime_error("Unsupported platform");						\
		}																			\
	int unused(const SceneryEditorX::PlatformContext &constex_type)

#endif

// ----------------------------------------------------

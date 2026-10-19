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
 * EntryPoint.h
 * -------------------------------------------------------
 * Created: 25/5/2025
 * -------------------------------------------------------
 */
#pragma once
#include "initializer.h"
#include <memory>
#include <SceneryEditorX/core/base.h>
#include <SceneryEditorX/core/application/application.h>
#include <SceneryEditorX/core/platform/platform_context.h>
#include <SceneryEditorX/logging/asserts.h>

// ----------------------------------------------------

#ifdef PLATFORM__MACOS
	#include <TargetConditionals.h>
#endif

/**
 * @brief Global flag controlling the application run loop
 * Set to false to exit the application after the current iteration
 */
extern bool appRunning;

// ----------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @brief Platform-independent main function
	 * 
	 * This is the core application entry point that handles initialization,
	 * application creation, running, and cleanup.
	 * 
	 * @param context The platform-specific context containing command-line args and paths
	 * @return Exit code (0 for success)
	 */
	inline int Main(const PlatformContext& context)
	{

		while (appRunning)
		{
			InitCore(context);
			Application* app = CreateApplication(context);
			SEDX_CORE_ASSERT(app != nullptr, "Failed to create application");
			app->Run();
			delete app;
			Shutdown();
		}

		return 0;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Platform-specific Entry Points                                                                               //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef SEDX_PLATFORM_WINDOWS
	#include <Windows.h>

	/**
	 * @brief Creates the platform context for Windows
	 * @note Implemented in windows/entrypoint.cpp
	 */
	extern SceneryEditorX::Scope<SceneryEditorX::PlatformContext> CreatePlatformContext(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow);

	/**
	 * @brief Windows-specific entry point macro
	 * 
	 * Creates the WinMain entry point and a platform_initializer function
	 * that receives the platform context for additional initialization.
	 * 
	 * @param context_type The parameter name for the PlatformContext in platform_initializer
	 */
	#define APP_MAIN(context_type)                                                                      \
		int platform_initializer(const SceneryEditorX::PlatformContext&);                               \
		int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow) \
		{                                                                                               \
			auto ctx = CreatePlatformContext(hInstance, hPrevInstance, lpCmdLine, nCmdShow);            \
			platform_initializer(*ctx);                                                                 \
			return SceneryEditorX::Main(*ctx);                                                          \
		}                                                                                               \
		int platform_initializer(const SceneryEditorX::PlatformContext& context_type)

#elif defined(SEDX_PLATFORM_LINUX) || defined(SEDX_PLATFORM_MACOS)

	/**
	 * @brief Creates the platform context for Unix-like systems (Linux/macOS)
	 * @note Implemented in linux/entrypoint.cpp or mac/entrypoint.cpp
	 */
	extern SceneryEditorX::Scope<SceneryEditorX::PlatformContext> CreatePlatformContext(int argc, char **argv);

	/**
	 * @brief Unix-like platform entry point macro
	 * 
	 * Creates the standard main entry point and a platform_initializer function
	 * that receives the platform context for additional initialization.
	 * 
	 * @param context_type The parameter name for the PlatformContext in platform_initializer
	 */
	#define APP_MAIN(context_type)                                                  \
		int platform_initializer(const SceneryEditorX::PlatformContext&);           \
		int main(int argc, char* argv[])                                            \
		{                                                                           \
			auto ctx = CreatePlatformContext(argc, argv);                           \
			platform_initializer(*ctx);                                             \
			return SceneryEditorX::Main(*ctx);                                      \
		}                                                                           \
		int platform_initializer(const SceneryEditorX::PlatformContext& context_type)

#else

	#include <stdexcept>
	
	/**
	 * @brief Fallback for unsupported platforms
	 */
	#define APP_MAIN(context_type)                                                  \
		int main(int argc, char* argv[])                                            \
		{                                                                           \
			throw std::runtime_error("Unsupported platform");                       \
		}                                                                           \
		int unused(const SceneryEditorX::PlatformContext& context_type)

#endif

// ----------------------------------------------------

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
 * main.cpp
 * -------------------------------------------------------
 * Created: 16/3/2025
 * -------------------------------------------------------
 */
#include <cstdlib>
#include <exception>
#include <Launcher/core/directory_manager.hpp>
#include <Launcher/core/launcher.h>
#include <Launcher/core/splash_handler.h>
#include <Launcher/registry/reg_check.h>
#include <SceneryEditorX/core/platform/entryPoint.h>
#include <SceneryEditorX/settings/user_settings.h>

// -------------------------------------------------------

static std::filesystem::path s_ProjectSolutionPath = "";
static std::vector<std::string> s_ClArguments;
static uint32_t s_ClArg_Flags = 0;
static auto operator<(const ImVec2 &lhs, const ImVec2 &rhs) { return lhs.x < rhs.x && lhs.y < rhs.y; }
static SDL_Window* s_SplashScreen_Window = nullptr;


/**
 * @brief 
 * @param argument 
 * @return 
 */
static bool HasArgument(const std::string &argument)
{
	for (const auto &arg : s_ClArguments)
	{
		if (arg == argument)
			return true;
	}

	return false;
}

/**
 * @brief 
 * @param value 
 */
static void WriteCiTestFile(const uint32_t value)
{
	if (HasArgument("-ci_test"))
	{
		if (std::ofstream file("ci_test.txt"); file.is_open())
		{
			file << value;
			file.close();
		}
	}
}

/**
 * @brief 
 */
static void ProcessClArgs()
{
	// Common simple flags that exist in the repo already:
	// -ci_test  -> used by CI to write a small indicator file (see WriteCiTestFile above)
	// You can add more handlers here for other flags (e.g. --headless, --log=level, --no-gui, etc.)

	EDITOR_TRACE_TAG("Editor", "Processing command line arguments");

	// Example: existing helper writes a CI file when -ci_test is present
	if (HasArgument("-ci_test"))
	{
		// The helper in this TU will write "ci_test.txt" containing 1
		WriteCiTestFile(1);
	}

	// Iterate and parse key=value style args
	for (const auto &arg : s_ClArguments)
	{
		// --headless or -no-ui : run without showing UI (example usage, implement the mode as needed)
		if (arg == "--headless" || arg == "-no-ui")
		{
			EDITOR_INFO_TAG("Editor", "Starting in headless/no-ui mode due to argument: {}", arg);
			// Set any internal flags or call methods to enter headless mode
			// e.g., Application::Get().SetHeadless(true);   // implement as needed
		}

		// Change logging level at startup
		// THIS HAS BEEN MOVED TO PLATFORM CONTEXT INIT
		/*
		constexpr std::string logPrefix = "--verbose";
		if (arg.starts_with(logPrefix))
		{
			std::string level = arg.substr(logPrefix.size());
			EDITOR_INFO_TAG("Editor", "Requested log level: {}", level);
			// Use your logging API to set the level, e.g. Log::SetLevelFromString(level);
			// If you don't have such a helper, map strings to levels here and call Log::SetLevel(...)
		}
		*/

		// Example: --run-task=name  -> you could dispatch internal tasks or tests
		constexpr std::string_view taskPrefix = "--run-task=";
		if (arg.starts_with(taskPrefix))
		{
			std::string taskName = arg.substr(taskPrefix.size());
			EDITOR_INFO_TAG("Editor", "Dispatching startup task: {}", taskName);
			// Dispatch your task: if (taskName == "build-shaders") BuildShaderPack();
		}
	}
}

class Launcher : public SceneryEditorX::Application
{
public:
	Launcher(const SceneryEditorX::PlatformContext &context, SceneryEditorX::AppData appData) : Application(context), m_AppData(std::move(appData))
	{
	}

	Launcher(const SceneryEditorX::PlatformContext &context, const SceneryEditorX::Ref<SceneryEditorX::UserPreferences> &userPreferences) 
	: Application(context), m_UserPreferences(userPreferences)
	{
		s_ClArguments = context.GetCommandLineArgs();
		ProcessClArgs(); // Process command line arguments to set internal flags before initialization
		EDITOR_INFO_TAG("Editor", "=== Initializing Editor with PlatformContext and UserPreferences ===");
		EDITOR_INFO_TAG("Editor", "Working Directory: {}", context.GetWorkingDirectory());
		EDITOR_INFO_TAG("Editor", "Temp Directory: {}", context.GetTempDirectory());

		if (m_ProjectPath.empty())
			m_ProjectPath = "SceneryEditorX/Projects/Default.edX";
	}

private:
	std::string m_ProjectPath;
	SceneryEditorX::AppData m_AppData;
	std::filesystem::path m_PersistentStoragePath;
	SceneryEditorX::Ref<SceneryEditorX::UserPreferences> m_UserPreferences;
};

void SplashHandler::CreateSplashScreen()
{
	// load splash screen image
	SDL_Surface* image = SDL_LoadPNG("resources/splash_screen.png");
	if (!image)
	{
		LAUNCHER_CORE_ERROR("Failed to load splash screen image: %s", SDL_GetError());
		return;
	}

	// create splash screen window centered on screen
	s_SplashScreen_Window = SDL_CreateWindow(
		"splash_screen",
		image->w,
		image->h,
		SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALWAYS_ON_TOP
	);

	if (!s_SplashScreen_Window)
	{
		LAUNCHER_CORE_ERROR("Failed to create splash screen window: %s", SDL_GetError());
		SDL_DestroySurface(image);
		return;
	}

	// get window surface
	SDL_Surface* window_surface = SDL_GetWindowSurface(s_SplashScreen_Window);
	if (!window_surface)
	{
		LAUNCHER_CORE_ERROR("Failed to get window surface: %s", SDL_GetError());
		SDL_DestroyWindow(s_SplashScreen_Window);
		SDL_DestroySurface(image);
		return;
	}

	// blit image to window surface
	if (!SDL_BlitSurface(image, nullptr, window_surface, nullptr))
	{
		LAUNCHER_CORE_ERROR("Failed to blit surface: %s", SDL_GetError());
		SDL_DestroyWindow(s_SplashScreen_Window);
		SDL_DestroySurface(image);
		return;
	}

	// update window surface to display the image
	if (!SDL_UpdateWindowSurface(s_SplashScreen_Window))
	{
		LAUNCHER_CORE_ERROR("Failed to update window surface: %s", SDL_GetError());
		SDL_DestroyWindow(s_SplashScreen_Window);
		SDL_DestroySurface(image);
		return;
	}

	SDL_DestroySurface(image);
}

SceneryEditorX::Application* SceneryEditorX::CreateApplication(const PlatformContext& context)
{
	s_ClArguments = context.GetCommandLineArgs();
	ProcessClArgs(); // Process command line arguments to set internal flags before initialization

	AppData appData;
	appData.splashScreen	= true;	 // Ensure splash screen is enabled for the launcher
	appData.resizable		= false; // Disable resizing for the launcher window
	appData.fullscreen		= false; // Ensure the launcher does not start in fullscreen
	appData.decorated		= false;

	// Return a new instance of the launcher application
	return new Launcher(context, std::move(appData));
}

/// -------------------------------------------------------

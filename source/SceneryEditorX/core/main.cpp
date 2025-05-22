/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* main.cpp
* -------------------------------------------------------
* Created: 16/3/2025
* -------------------------------------------------------
*/
#include <CrashHandler/crash_handler.h>
#include <Launcher/core/launcher_main.h>
#include <SceneryEditorX/core/window.h>
#include <SceneryEditorX/editor/editor.h>

/// -------------------------------------------------------

/*
std::string getDumpDirectory()
{
    return std::filesystem::temp_directory_path().string();
}
*/

int main(const int argc, const char** argv[])
{
    /// Initialize the application
    SceneryEditorX::Log::Init();

	/// Initialize the Crash Handler
	
	/*
    CrashHandler::CrashService::CrashHandlerConfig config;
    config.appVersion = SEDX_VERSION_STRING;
    config.dumpDir = getDumpDirectory();
    config.onCrashDetectedCallback = [](const std::string &dumpPath)
    {
        /// Optional callback - called in the crash context, keep it minimal
        spdlog::critical("Crash detected, dump saved to: {}", dumpPath);
    };
    CrashHandler::CrashService::Init(config);
	*/

	/// ----------------------------------------------------

    try
    {
        SceneryEditorX::EditorApplication app;
        app.Run();
    }
    catch (const std::runtime_error &error)
    {
        SEDX_CORE_ERROR_TAG("Core", "Runtime error: {}", error.what());
        SceneryEditorX::Log::FlushAll();
        return -1;
    }
    catch (const std::exception &error)
    {
        SEDX_CORE_ERROR_TAG("Core", "Application error: {}", error.what());
        SceneryEditorX::Log::FlushAll();
        //CrashHandler::CrashService::WriteDump("Unhandled exception: " + std::string(error.what()));
        return EXIT_FAILURE;
    }

    /// Shut down crash handler before exit
    //CrashHandler::CrashService::Shutdown();

    SEDX_CORE_INFO("Scenery Editor X Engine is shutting down...");
    SceneryEditorX::Log::FlushAll(); /// Make sure to flush logs before shutdown
    SceneryEditorX::Log::ShutDown();

    return 0;
}

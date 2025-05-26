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
#include <Editor/core/editor.h>
#include <Launcher/core/launcher_main.h>
#include <SceneryEditorX/core/window.h>
#include <SceneryEditorX/EntryPoint.h>
#include <SceneryEditorX/platform/settings.h>

#ifdef SEDX_PLATFORM_WINDOWS
#include <Shlobj.h>
#endif // SEDX_PLATFORM_WINDOWS

/// -------------------------------------------------------

/*
std::string getDumpDirectory()
{
    return std::filesystem::temp_directory_path().string();
}
*/

class EditorX : public SceneryEditorX::Application
{
public:
    EditorX(const SceneryEditorX::WindowData &windowData, std::string_view projPath) : Application(windowData), projectPath(projPath)
	{
		if (projPath.empty())
            projectPath = "SceneryEditorX/Projects/Default.edX";
	}

    virtual void OnInit() override  
    {  
        /// Initialize the user settings  
        userSettings = CreateRef<SceneryEditorX::ApplicationSettings>("settings.cfg");
        if (!userSettings->ReadSettings())  
        {  
            EDITOR_ERROR_TAG("Core", "Failed to initialize user settings for project: {}", projectPath);  
            return;  
        }  
        /// Initialize the editor  
        //SceneryEditorX::EditorApplication::InitEditor(userSettings, projPath);  
    }

private:
    std::string projPath;
    std::string_view projectPath;
    Ref<SceneryEditorX::ApplicationSettings> userSettings{};
};

SceneryEditorX::Application *SceneryEditorX::CreateApplication(int argc, char** argv)
{
    std::string_view projectPath;

    SceneryEditorX::WindowData appData;
    appData.title = "Scenery Editor X";
    appData.width = 1280;
    appData.height = 720;

    return new EditorX(appData, projectPath);
}


/*
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
	#1#

	/// ----------------------------------------------------

    try
    {
        SceneryEditorX::EditorApplication app;
        app.Run();
    }
    catch (const std::runtime_error &error)
    {
        EDITOR_ERROR_TAG("Core", "Runtime error: {}", error.what());
        SceneryEditorX::Log::FlushAll();
        return -1;
    }
    catch (const std::exception &error)
    {
        EDITOR_ERROR_TAG("Core", "Application error: {}", error.what());
        SceneryEditorX::Log::FlushAll();
        //CrashHandler::CrashService::WriteDump("Unhandled exception: " + std::string(error.what()));
        return EXIT_FAILURE;
    }

    /// Shut down crash handler before exit
    //CrashHandler::CrashService::Shutdown();

    EDITOR_INFO("Scenery Editor X Engine is shutting down...");
    SceneryEditorX::Log::FlushAll(); /// Make sure to flush logs before shutdown
    SceneryEditorX::Log::ShutDown();

    return 0;
}
*/

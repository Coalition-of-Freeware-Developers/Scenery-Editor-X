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
#include <Launcher/core/launcher_main.h>
#include <SceneryEditorX/core/editor/editor.h>
#include <SceneryEditorX/core/window.h>

// -------------------------------------------------------

int main(const int argc, const char** argv[])
{
    SceneryEditorX::Log::Init();
    
    // Log header information immediately after init and flush to ensure it's written
    SceneryEditorX::Log::LogHeader();
    SceneryEditorX::Log::FlushAll();

    try
    {
        SceneryEditorX::EditorApplication app;
        app.run();
    }
    catch (std::exception error)
    {
        SEDX_CORE_ERROR_TAG("Core", "Application error: {}", error.what());
        SceneryEditorX::Log::FlushAll();
        return -1;
    }

    SEDX_CORE_INFO("Scenery Editor X Engine is shutting down...");
    SceneryEditorX::Log::FlushAll(); // Make sure to flush logs before shutdown
    SceneryEditorX::Log::ShutDown();

    return 0;
}

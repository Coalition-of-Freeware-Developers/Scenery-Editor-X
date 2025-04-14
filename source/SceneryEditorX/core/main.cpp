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

#include <SceneryEditorX/core/window.h>
#include <Launcher/core/launcher_main.h>
#include <SceneryEditorX/core/editor/editor.h>

// -------------------------------------------------------

int main(const int argc, const char *argv[])
{
    SceneryEditorX::Log::Init();

    try
    {
        SceneryEditorX::EditorApplication app;
        app.run();
	}
	catch (std::exception error)
	{
		return -1;
	}

	SEDX_CORE_INFO("Scenery Editor X Engine is shutting down...");
    SceneryEditorX::Log::ShutDown();

	return 0;
}

/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* editor.h
* -------------------------------------------------------
* Created: 16/3/2025
* -------------------------------------------------------
*/

#pragma once
#include <SceneryEditorX/core/application.h>
#include <SceneryEditorX/core/editor.h>
//#include <SceneryEditorX/core/launcher.h>

// -------------------------------------------------------

extern SceneryEditorX::EditorApplication* SceneryEditorX::CreateApplication(const int argc, const char *argv[]);
//bool Launcher::AdminCheck();
bool isRunning = true;

// -------------------------------------------------------

namespace SceneryEditorX
{
	int main(const int argc, const char *argv[])
	{
        while (isRunning)
        {
            //InitializeLauncher();
            //ShutdownLauncher();
            InitializeCore();
            EditorApplication *app = CreateApplication(argc, argv);
            app->run();
            delete app;
            ShutdownCore();
        }
	    return 0;
	}
}

// -------------------------------------------------------

#if defined(SEDX_RELEASE) && defined(SEDX_PLATFORM_WINDOWS)
	
	int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
	{
	    return SceneryEditorX::main(__argc, __argv);
	}
	
#else
	
	int main(int argc, char **argv)
	{
	    return SceneryEditorX::main(argc, argv);
	}

#endif // SEDX_RELEASE && SEDX_PLATFORM_WINDOWS

// -------------------------------------------------------

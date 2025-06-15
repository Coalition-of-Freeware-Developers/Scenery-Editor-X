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
#include <SceneryEditorX/core/application.h>
#include <SceneryEditorX/core/initializer.h>

/// ----------------------------------------------------

extern SceneryEditorX::Application *CreateApplication(int argc, char** argv);
bool appRunning = true;

namespace SceneryEditorX
{
	int Main(int argc, char **argv)
	{
        while (appRunning)
		{
            InitCore();
            Application *app = CreateApplication(argc, argv);
            SEDX_CORE_ASSERT(app, "Application is null!");
            app->Run();
            delete app;
            Shutdown();
	    }
        return 0;
	}

} // namespace SceneryEditorX

/// ----------------------------------------------------

#ifdef SEDX_RELEASE && SEDX_PLATFORM_WINDOWS
	int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
	{
	    return SceneryEditorX::Main(__argc, __argv);
	}
#else

	int main(int argc, char** argv)
	{
        return SceneryEditorX::Main(argc, argv);
	}

#endif // SEDX_RELEASE && SEDX_PLATFORM_WINDOWS

/// ----------------------------------------------------

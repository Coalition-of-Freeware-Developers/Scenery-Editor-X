#pragma once

#ifdef WL_PLATFORM_WINDOWS

extern SceneryEditorX::Application* SceneryEditorX::CreateApplication(int argc, char** argv);
bool g_ApplicationRunning = true;

namespace SceneryEditorX {

	int Main(int argc, char** argv)
	{
		while (g_ApplicationRunning)
		{
			SceneryEditorX::Application* app = SceneryEditorX::CreateApplication(argc, argv);
			app->Run();
			delete app;
		}

		return 0;
	}

}

#ifdef WL_DIST

#include <Windows.h>

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
	return SceneryEditorX::Main(__argc, __argv);
}

#else

int main(int argc, char** argv)
{
	return SceneryEditorX::Main(argc, argv);
}

#endif // WL_DIST

#endif // WL_PLATFORM_WINDOWS

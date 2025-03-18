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
#include <SceneryEditorX/renderer/vk_core.h>
#include <SceneryEditorX/renderer/graphics_defs.h>
#include <Launcher/core/launcher_main.h>

// -------------------------------------------------------

GLOBAL std::unique_ptr<Window> g_Window = {};

// -------------------------------------------------------

class Application
{
public:

	void run()
	{
		Setup();
		Create();
		MainLoop();
		Shutdown();
	}

private:
	bool viewportResized = false;

	void Setup()
	{    
		Log::LogHeader();
		EDITOR_LOG_INFO("Scenery Editor X Engine is starting...");

		//Launcher::AdminCheck();
		//Launcher::Loader loader{};
		//loader.run();

		//SceneryEditorX::ReadCache();
	}

	void Create()
	{
		//window = new Window;
		g_Window->Window::Create();
		void initVulkan();
		//Window::SetTitle("Scenery Editor X | " + assetManager.GetProjectName());
		//SceneryEditorX::Init(Window::GetGLFWwindow(),Window::GetWidth(),Window::GetHeight());
		//SceneryEditorX::CreateEditor();
	}

	void MainLoop()
	{
		while (!Window::GetShouldClose())
		{
			Window::Update();
			if (viewportResized)
			{
				//SceneryEditorX::ResizeViewport();
				viewportResized = false;
			}
		}

		//SceneryEditorX::WaitIdle();
	}

	void setupImgui()
	{

	}

	void updateCommandBuffer()
	{

	}

	void drawFrame()
	{

	}

	void Shutdown()
	{
		Window::Destroy();
	}
};

INTERNAL void initVulkan()
{

	GLFWwindow* window = Window::GetGLFWwindow();
	GraphicsEngine graphicsEngine(*window);
	graphicsEngine.initEngine();

	/*
	const SwapChainInfo swapChainInfo = {
		.width = static_cast<uint32_t>(g_Window->get_width()),
		.height = static_cast<uint32_t>(g_Window->get_height()),
		.bufferCount = 3,
		.format = VK_FORMAT_R8G8B8A8_UNORM,
		.vsync = true
	};
	*/
}


int main(const int argc,const char* argv[])
{
	Log::Init();

	Application app;
	try
	{
		app.run();
	}
	catch (std::exception error)
	{
		return -1;
	}

	EDITOR_LOG_INFO("Scenery Editor X Engine is shutting down...");
	Log::Shutdown();
	return 0;
}

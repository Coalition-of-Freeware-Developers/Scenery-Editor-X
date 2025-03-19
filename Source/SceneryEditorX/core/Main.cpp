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
#include <SceneryEditorX/core/window.h>
#include <SceneryEditorX/renderer/graphics_checks.h>
#include <SceneryEditorX/renderer/graphics_defs.h>
#include <SceneryEditorX/renderer/vk_core.h>
#include <SceneryEditorX/scene/asset_manager.h>

/*
* -------------------------------------------------------
* Application Global Variables
* -------------------------------------------------------
*/

GLOBAL Scope<Window> g_Window;

/**
 * -------------------------------------------------------
 * FORWARD FUNCTION DECLARATIONS
 * -------------------------------------------------------
 */

//INTERNAL void initVulkan(GraphicsEngine &vkRenderer);


// -------------------------------------------------------

class Application
{
public:
    Application() : vkRenderer(nullptr)
    {
    }

    ~Application() {}

	void run()
	{
		Setup();
		Create();
		MainLoop();
		Shutdown();
	}

private:
    glm::ivec2 viewportSize = {64, 48};
    glm::ivec2 newViewportSize = viewportSize;
	bool viewportResized = false;
    bool fullscreen = false;
    //AssetManager assetManager;

// -------------------------------------------------------

	void Setup()
	{    
		Log::LogHeader();
		EDITOR_LOG_INFO("Scenery Editor X Engine is starting...");

		//Launcher::AdminCheck();
		//Launcher::Loader loader{};
		//loader.run();
        //SceneryEditorX::ReadCache();
        //assetManager.LoadProject(cacheData.projectPath, cacheData.binPath);
        //scene = assetManager.GetInitialScene();
        //camera = assetManager.GetMainCamera(scene);

	}

	void Create()
	{
        g_Window = CreateScope<Window>(); 
        g_Window->Create();

        //VulkanChecks vulkanChecks;           // Create a new instance of the VulkanChecks class
        //vulkanChecks.InitChecks({}, {});     // Initialize the Vulkan checks

        //Window::SetTitle("Scenery Editor X | " + assetManager.GetProjectName());
        //SceneryEditorX::Init(Window::GetGLFWwindow(),Window::GetWidth(),Window::GetHeight());
        //SceneryEditorX::CreateEditor();

        vkRenderer = CreateRef<GraphicsEngine>(*Window::GetGLFWwindow());
        vkRenderer->initEngine();

        //camera->extent = {viewportSize.x, viewportSize.y};
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

    Ref<GraphicsEngine> vkRenderer; //Vulkan renderer instance


};

/*
INTERNAL void initVulkan(GraphicsEngine &vkRenderer)
{
	GLFWwindow* window = Window::GetGLFWwindow();
    vkRenderer.initEngine();
}
*/

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

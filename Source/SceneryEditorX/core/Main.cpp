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
#include <SceneryEditorX/renderer/vk_checks.h>
#include <SceneryEditorX/renderer/vk_core.h>
#include <SceneryEditorX/scene/asset_manager.h>

/*
* -------------------------------------------------------
* EditorApplication Global Variables
* -------------------------------------------------------
*/

//GLOBAL Scope<Window> g_Window;

/**
 * -------------------------------------------------------
 * FORWARD FUNCTION DECLARATIONS
 * -------------------------------------------------------
 */

//INTERNAL void initVulkan(GraphicsEngine &vkRenderer);

// -------------------------------------------------------

class EditorApplication
{
public:
	void run()
	{
		initialize_editor();
        create();
		main_loop();
		shut_down();
	}
    //bool GetSwapChainDirty();

private:
    GLFWwindow* window;
    SceneryEditorX::GraphicsEngine vkRenderer;
    uint32_t currentFrame = 0;
	glm::ivec2 viewportSize = {64, 48};
	glm::ivec2 newViewportSize = viewportSize;
    bool viewportHovered = false;
	bool viewportResized = false;
	bool fullscreen = false;
    bool swapChainDirty = true;
    uint32_t frameCount = 0;
	//AssetManager assetManager;

// -------------------------------------------------------

	void initialize_editor()
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
        Window::Create();
	}

	void create()
	{
		//g_Window = CreateScope<Window>();
		//g_Window->Create();

		VulkanChecks vulkanChecks;           // Create a new instance of the VulkanChecks class
		vulkanChecks.InitChecks({}, {});     // Initialize the Vulkan checks

		//Window::SetTitle("Scenery Editor X | " + assetManager.GetProjectName());
        vkRenderer.initEngine(Window::GetGLFWwindow(), Window::GetWidth(), Window::GetHeight());

		//SceneryEditorX::CreateEditor();

		//vkRenderer = CreateRef<GraphicsEngine>(*Window::GetGLFWwindow());
		//vkRenderer->initEngine();

		//camera->extent = {viewportSize.x, viewportSize.y};
	}

	void main_loop()
	{
		while (!Window::GetShouldClose())
		{
			if (viewportResized)
			{
                vkRenderer.recreateSwapChain();
				viewportResized = false;
			}

            DrawFrame();
            bool ctrlPressed = Window::IsKeyPressed(GLFW_KEY_LEFT_CONTROL) || Window::IsKeyDown(GLFW_KEY_LEFT_CONTROL);
            Window::Update();
		}

		WaitIdle();
	}

	void setupImgui()
	{

	}

	void updateCommandBuffer()
	{

	}

	void WaitIdle()
    {
        auto result = vkDeviceWaitIdle(vkRenderer.GetDevice());
        if (result != VK_SUCCESS)
        {
            EDITOR_LOG_ERROR("Failed to wait for device to become idle: {}", ToString(result));
        }
    }

    // -------------------------------------------------------

	void OnSurfaceUpdate(uint32_t width, uint32_t height)
	{
	    vkRenderer.cleanupSwapChain();
	    vkRenderer.recreateSurfaceFormats();
	    vkRenderer.createSwapChain();
	}

	void RecreateFrameResources()
    {
        while (Window::GetWidth() == 0 || Window::GetHeight() == 0)
        {
            Window::WaitEvents();
        }

        viewportSize = newViewportSize;

        if (viewportSize.x == 0 || viewportSize.y == 0)
        {
            return;
        }

        WaitIdle();

        if (Window::GetFramebufferResized() || Window::IsDirty())
        {
            if (Window::IsDirty())
            {
                Window::ApplyChanges();
            }
            Window::UpdateFramebufferSize();
            OnSurfaceUpdate(Window::GetWidth(), Window::GetHeight());
        }

	}

	/*
    void DrawEditor()
    {
        //SceneryEditorX::DrawEditor();
        if (!fullscreen)
        {

		}
        else
        {
            newViewportSize = {Window::GetWidth(), Window::GetHeight()};
            viewportHovered = true;
        }
    }
	*/

	void DrawFrame()
	{
        //DrawEditor();
        vkRenderer.renderFrame();
        /*
        if (GetSwapChainDirty())
        {
            return;
        }
		*/
        //SceneryEditorX::SubmitAndPresent();
        frameCount = (frameCount + 1) % (1 << 15);
	}

	/*
	void renderFrame()
    {

    }
	*/

	void shut_down()
	{
        vkRenderer.cleanup();
        Window::Destroy();
	}

    /*
    bool GetSwapChainDirty()
    {
        return swapChainDirty;
    }
	*/

    //Ref<SceneryEditorX::GraphicsEngine> vkRenderer; //Vulkan renderer instance

};

int main(const int argc, const char *argv[])
{
	Log::Init();

	EditorApplication app;
	try
	{
		app.run();
	}
	catch (std::exception error)
	{
		return -1;
	}

	EDITOR_LOG_INFO("Scenery Editor X Engine is shutting down...");
	Log::shut_down();
	return 0;
}

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
#include <SceneryEditorX/scene/perspective_camera.h>

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

	Camera::PerspectiveCamera mainCamera{"MainCamera"};
    float lastFrameTime = 0.0f;
    float currentFrameTime = 0.0f;

	glm::vec2 lastMousePos = {0.0f, 0.0f};
    bool firstMouse = true;
    bool cameraActive = false;

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

		mainCamera.set_perspective(60.0f, static_cast<float>(Window::GetWidth()) / Window::GetHeight(), 0.1f, 100.0f);
        mainCamera.set_position(glm::vec3(0.0f, 0.0f, 3.0f));
        mainCamera.type = CameraType::FirstPerson;
		//camera->extent = {viewportSize.x, viewportSize.y};
	}

	void main_loop()
	{
        lastFrameTime = static_cast<float>(glfwGetTime());

		while (!Window::GetShouldClose())
		{
            // Calculate delta time
            currentFrameTime = static_cast<float>(glfwGetTime());
            float deltaTime = currentFrameTime - lastFrameTime;
            lastFrameTime = currentFrameTime;

			// Process camera input
            ProcessCameraInput(deltaTime);

            // Update camera
            mainCamera.update(deltaTime);

			if (viewportResized)
			{
                vkRenderer.recreateSwapChain();
				viewportResized = false;

                // Update camera aspect ratio when viewport resizes
                mainCamera.update_aspect_ratio(static_cast<float>(viewportSize.x) / viewportSize.y);
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
	    vkRenderer.DestroySwapChain();
	    vkRenderer.recreateSurfaceFormats();
	    vkRenderer.createSwapChain();
	}

	// Add a new method to process camera input
    void ProcessCameraInput(float deltaTime)
    {
        // Camera movement
        mainCamera.keys.left = Window::IsKeyDown(GLFW_KEY_A);
        mainCamera.keys.right = Window::IsKeyDown(GLFW_KEY_D);
        mainCamera.keys.up = Window::IsKeyDown(GLFW_KEY_W);
        mainCamera.keys.down = Window::IsKeyDown(GLFW_KEY_S);

        // Camera rotation with mouse - activate with right mouse button
        if (Window::IsMouseDown(GLFW_MOUSE_BUTTON_RIGHT))
        {
            cameraActive = true;

            // Hide cursor for camera control
            glfwSetInputMode(Window::GetGLFWwindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

            glm::vec2 currentMousePos = Window::GetMousePosition();

            if (firstMouse)
            {
                lastMousePos = currentMousePos;
                firstMouse = false;
            }

            // Calculate mouse movement delta
            glm::vec2 mouseDelta = currentMousePos - lastMousePos;
            lastMousePos = currentMousePos;

            // Apply rotation, adjusting sensitivity as needed
            const float sensitivity = 0.1f;
            mainCamera.rotate(glm::vec3(-mouseDelta.y * sensitivity, -mouseDelta.x * sensitivity, 0.0f));
        }
        else if (cameraActive)
        {
            // Reset cursor when camera control is released
            glfwSetInputMode(Window::GetGLFWwindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            cameraActive = false;
            firstMouse = true;
        }
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
        // Update the view and projection matrices in the Vulkan uniform buffer
        UpdateUniformBufferMatrices();
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

	void UpdateUniformBufferMatrices()
    {
        // Get the current image index
        uint32_t imageIndex = vkRenderer.getCurrentImageIndex();

        // Create the transformation for the model
        SceneryEditorX::UniformBufferObject ubo{};

        // Identity model matrix for now
        ubo.model = glm::mat4(1.0f);

        // Get view and projection matrices from camera
        ubo.view = mainCamera.matrices.view;
        ubo.proj = mainCamera.get_projection();

        // Update the uniform buffer with our new matrices
        vkRenderer.updateUniformBuffer(imageIndex, ubo);
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

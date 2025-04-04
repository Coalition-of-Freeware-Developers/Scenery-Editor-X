/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* editor.h
* -------------------------------------------------------
* Created: 4/4/2025
* -------------------------------------------------------
*/
#pragma once
#include <Launcher/core/launcher_main.h>
#include <SceneryEditorX/core/window.h>
#include <SceneryEditorX/renderer/vk_checks.h>
#include <SceneryEditorX/renderer/vk_core.h>
#include <SceneryEditorX/platform/editor_config.hpp>
#include <SceneryEditorX/scene/asset_manager.h>
#include <SceneryEditorX/ui/ui.h>
#include <SceneryEditorX/logging/Logging.hpp>

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct EditorSpecification
	{
		std::string name;
		uint32_t width = 1280;
		uint32_t height = 720;
		std::string projectPath;
		std::string binPath;
        std::filesystem::path IconPath;
		bool StartMaximized = true;
		bool VSync = true;
		bool FullScreen = false;
		bool Resizable = true;
		bool Decorated = false;
		bool Dockspace = true;
		bool ShowDemoWindow = false;
		bool ShowImGuiMetrics = false;
		bool ShowImGuiStyleEditor = false;
		bool ShowImGuiAbout = false;
	};

	class EditorApplication
	{
		//using EventCallbackFn = std::function<void(Event&)>;
	public:
        EditorApplication(const EditorSpecification &specification, std::string_view projectPath);
        virtual ~EditorApplication();

		void Run();
        void Close();

	    //void run()
	    //{
        //    InitializeEditor();
	    //    create();
	    //    main_loop();
	    //    ShutDown();
	    //}
	    //bool GetSwapChainDirty();


		//inline Ref<GLFWwindow> GetWindow() { return window.As<GLFWwindow>(); }


	private:
        Ref<GLFWwindow> window;
	    //GLFWwindow *window;
	    GraphicsEngine vkRenderer;
	    GUI ui;
	    uint32_t currentFrame = 0;
	    glm::ivec2 viewportSize = {64, 48};
	    glm::ivec2 newViewportSize = viewportSize;
	    bool viewportHovered = false;
	    bool viewportResized = false;
	    bool fullscreen = false;
	    bool swapChainDirty = true;
	    uint32_t frameCount = 0;
	    bool Dockspace_ = false;
	    //AssetManager assetManager;
	
	    // -------------------------------------------------------
	
	    void InitializeEditor()
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
	
	        VulkanChecks vulkanChecks;       // Create a new instance of the VulkanChecks class
	        vulkanChecks.InitChecks({}, {}); // Initialize the Vulkan checks
	
	        //Window::SetTitle("Scenery Editor X | " + assetManager.GetProjectName());
	        vkRenderer.initEngine(Window::GetGLFWwindow(), Window::GetWidth(), Window::GetHeight());
	
	        ui.initGUI(Window::GetGLFWwindow(), vkRenderer);
	
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
	
	    void ShutDown()
	    {
	        vkRenderer.cleanUp();
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

	EditorApplication* CreateApplication(const int argc, const char *argv[]);

} // namespace SceneryEditorX

// -------------------------------------------------------

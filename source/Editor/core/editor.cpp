/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* editor.cpp
* -------------------------------------------------------
* Created: 13/4/2025
* -------------------------------------------------------
*/
#include <Editor/core/editor.h>
#include <imgui/imgui.h>
#include <SceneryEditorX/core/application.h>
#include <SceneryEditorX/core/window/window.h>
#include <SceneryEditorX/platform/settings.h>
#include <SceneryEditorX/project/project.h>
#include <SceneryEditorX/renderer/render_context.h>
#include <SceneryEditorX/ui/ui.h>
#include <SceneryEditorX/ui/ui_context.h>

/// ---------------------------------------------------------

namespace SceneryEditorX
{
	namespace UI
	{
	    class UIContextImpl;
	}

    /**
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
	
	//INTERNAL void initVulkan(GraphicsEngine &gfxEngine);

	/// -------------------------------------------------------

    EditorApplication::EditorApplication() = default;

    EditorApplication::~EditorApplication() = default;

    void EditorApplication::Run()
    {
        //const Ref<Window> &window = GetWindow();
        const auto start = std::chrono::high_resolution_clock::now();

		InitEditor();

		const auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

		MainLoop();
    }

    void EditorApplication::InitEditor()
    {
	    /// Log header information immediately after init and flush to ensure it's written
	    EDITOR_INFO("Scenery Editor X Graphics Engine is starting...");

        GraphicsEngine::Init();

	    //Launcher::AdminCheck();
	    //Launcher::Loader loader{};
	    //loader.run();
	    //SceneryEditorX::ReadCache();
	    //assetManager.LoadProject(cacheData.projectPath, cacheData.binPath);

        //ImGui::CreateContext(); //TODO: Not sure if this is the right location for this. Maybe move to UI initialization.

	    //scene = assetManager.GetInitialScene();
	    //camera = assetManager.GetMainCamera(scene);
	}
	
	void EditorApplication::Create()
	{
        //auto physDevice = vkDevice->GetPhysicalDevice();
        //physDevice->SelectDevice(VK_QUEUE_GRAPHICS_BIT, true);

        /// Set up the features we need
        //VkPhysicalDeviceFeatures deviceFeatures{};
		//deviceFeatures = vkDeviceFeatures.GetPhysicalDeviceFeatures();

        /// Update the vkDevice handle
        //device = vkDevice->GetDevice();

	    //Window::SetTitle("Scenery Editor X | " + assetManager.GetProjectName());
        //gfxEngine.CreateInstance(editorWindow);
	
	    //createViewportResources();
	
        /// Initialize UI components
        ui.InitGUI();

	    //SceneryEditorX::CreateEditor();
	
	    //gfxEngine = CreateRef<GraphicsEngine>(*Window::GetWindow());
	    //gfxEngine->CreateInstance();
	
	    //camera->extent = {viewportSize.x, viewportSize.y};
	}
	
	void EditorApplication::MainLoop()
	{


        vkDeviceWaitIdle(device);
	}

    void EditorApplication::Update()
    {
        /// Update the viewport size if it has changed
        if (viewportData.viewportResized)
            return;
            //RecreateFrameResources();

    }

	void EditorApplication::UpdateWindowTitle(const std::string &sceneName)
    {
        const std::string title = std::format("{0} ({1}) - Scenery Editor X {2}", sceneName, Project::GetActive()->GetConfig().name, SEDX_VERSION);
        Application::Get().GetWindow().SetTitle(title);
    }

    /**
     * @brief Recreates frame-related resources.
     * 
     * This method is called when the viewport size has changed to rebuild
     * swap chain images, framebuffers, and other resources needed for rendering.
     * It properly handles cleanup of old resources and initialization of new ones
     * based on the current viewport dimensions.
     */
    /*
    void EditorApplication::RecreateFrameResources()
    {
        // Wait for the device to finish all operations
        if (device != VK_NULL_HANDLE)
            vkDeviceWaitIdle(device);

        // Clean up existing viewport resources
        CleanupViewportResources();

        // Update viewport size from the new size
        viewportData.SetViewportSize(newViewportSize.GetViewportSize());
        viewportData.viewportResized = false;

        // Create new viewport resources with updated dimensions
        CreateViewportResources();

        // Log the viewport recreation
        EDITOR_INFO("Viewport resources recreated with size: {}x{}", 
            viewportData.GetViewportSize().width, 
            viewportData.GetViewportSize().height);
    }
    */

    /// -------------------------------------------------------
} // namespace SceneryEditorX

/// -------------------------------------------------------

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
#include <imgui/imgui.h>
#include <SceneryEditorX/core/editor/editor.h>
#include <SceneryEditorX/core/window.h>
#include <SceneryEditorX/platform/settings.h>
#include <SceneryEditorX/ui/ui.h>
#include <SceneryEditorX/ui/ui_context.h>
#include <SceneryEditorX/vulkan/vk_checks.h>
#include <SceneryEditorX/vulkan/vk_core.h>

// ---------------------------------------------------------

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

	// -------------------------------------------------------

    EditorApplication::EditorApplication() = default;

    EditorApplication::~EditorApplication()
    {
        gfxEngine.~GraphicsEngine();
    }

    void EditorApplication::Run()
    {
        const auto start = std::chrono::high_resolution_clock::now();

		InitEditor();

		const auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

		MainLoop();

    }

    void EditorApplication::InitEditor()
    {
	    /// Log header information immediately after init and flush to ensure it's written
        Log::LogHeader();
        //Log::FlushAll();
	    SEDX_CORE_INFO("Scenery Editor X Engine is starting...");
	
	    //Launcher::AdminCheck();
	    //Launcher::Loader loader{};
	    //loader.run();
	    //SceneryEditorX::ReadCache();
	    //assetManager.LoadProject(cacheData.projectPath, cacheData.binPath);
        ApplicationSettings settings("settings.cfg");
        settings.ReadSettings();

        ImGui::CreateContext(); //TODO: Not sure if this is the right location for this. Maybe move to UI initialization.

	    //scene = assetManager.GetInitialScene();
	    //camera = assetManager.GetMainCamera(scene);
	}
	
	void EditorApplication::Create()
	{
        const Ref<Window> editorWindow = CreateRef<Window>();

        gfxEngine.Init(editorWindow);
	
        Ref<VulkanChecks> vulkanChecks = CreateRef<VulkanChecks>();
        vulkanChecks->InitChecks({}, {}, {});

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
        ui.InitGUI(Window::GetWindow(), gfxEngine);

	    //SceneryEditorX::CreateEditor();
	
	    //gfxEngine = CreateRef<GraphicsEngine>(*Window::GetWindow());
	    //gfxEngine->CreateInstance();
	
	    //camera->extent = {viewportSize.x, viewportSize.y};
	}
	
	void EditorApplication::MainLoop()
	{
	    while (!Window::GetShouldClose())
	    {
            if (viewportData.viewportResized)
	        {
                vkSwapChain->OnResize(viewportData.x, viewportData.y);
                viewportData.viewportResized = false;
	        }

			//Update();
	        DrawFrame();
	        bool ctrlPressed = Window::IsKeyPressed(GLFW_KEY_LEFT_CONTROL) || Window::IsKeyDown(GLFW_KEY_LEFT_CONTROL);
	        Window::Update();
	    }

        vkDeviceWaitIdle(device);
	}
	
	void EditorApplication::CreateViewportResources()
	{

	    /// Create image for the viewport
        vkSwapChain->CreateImage(viewportData.x, viewportData.y, 1,
	                           VK_SAMPLE_COUNT_1_BIT,
	                           vkSwapChain->GetColorFormat(),
	                           VK_IMAGE_TILING_OPTIMAL,
	                           VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
	                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                               viewportData.viewportImage,
                               viewportData.viewportImageMemory);
	
	    /// Create image view for the viewport
        viewportData.viewportImageView = vkSwapChain->CreateImageView(viewportData.viewportImage, vkSwapChain->GetColorFormat(), VK_IMAGE_ASPECT_COLOR_BIT, 1);
	
	    /// Create render pass for the viewport
	    VkAttachmentDescription colorAttachment{};
        colorAttachment.format = vkSwapChain->GetColorFormat();
	    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	
	    VkAttachmentReference colorAttachmentRef{};
	    colorAttachmentRef.attachment = 0;
	    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	
	    VkSubpassDescription subpass{};
	    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	    subpass.colorAttachmentCount = 1;
	    subpass.pColorAttachments = &colorAttachmentRef;
	
	    VkRenderPassCreateInfo renderPassInfo{};
	    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	    renderPassInfo.attachmentCount = 1;
	    renderPassInfo.pAttachments = &colorAttachment;
	    renderPassInfo.subpassCount = 1;
	    renderPassInfo.pSubpasses = &subpass;
	
	    SEDX_CORE_ASSERT(vkCreateRenderPass(gfxEngine.GetLogicDevice()->GetDevice(), &renderPassInfo, nullptr, &viewportData.viewportRenderPass) == VK_SUCCESS);

        /// Create framebuffer for the viewport
	    VkFramebufferCreateInfo framebufferInfo{};
	    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = viewportData.viewportRenderPass;
	    framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &viewportData.viewportImageView;
        framebufferInfo.width = viewportData.x;
        framebufferInfo.height = viewportData.y;
	    framebufferInfo.layers = 1;
	
	    SEDX_CORE_ASSERT(vkCreateFramebuffer(gfxEngine.GetLogicDevice()->GetDevice(), &framebufferInfo, nullptr, &viewportData.viewportFramebuffer) == VK_SUCCESS);

	}
	
	void EditorApplication::CleanupViewportResources()
	{
	
	    if (viewportData.viewportFramebuffer != VK_NULL_HANDLE)
	    {
            vkDestroyFramebuffer(device, viewportData.viewportFramebuffer, nullptr);
            viewportData.viewportFramebuffer = VK_NULL_HANDLE;
	    }
	
	    if (viewportData.viewportRenderPass != VK_NULL_HANDLE)
	    {
            vkDestroyRenderPass(device, viewportData.viewportRenderPass, nullptr);
            viewportData.viewportRenderPass = VK_NULL_HANDLE;
	    }
	
	    if (viewportData.viewportImageView != VK_NULL_HANDLE)
	    {
            vkDestroyImageView(device, viewportData.viewportImageView, nullptr);
            viewportData.viewportImageView = VK_NULL_HANDLE;
	    }
	
	    if (viewportData.viewportImage != VK_NULL_HANDLE)
	    {
            vkDestroyImage(device, viewportData.viewportImage, nullptr);
            viewportData.viewportImage = VK_NULL_HANDLE;
	    }
	
	    if (viewportData.viewportImageMemory != VK_NULL_HANDLE)
	    {
            vkFreeMemory(device, viewportData.viewportImageMemory, nullptr);
            viewportData.viewportImageMemory = VK_NULL_HANDLE;
	    }
	}

    void EditorApplication::Update()
    {
        // Update the viewport size if it has changed
        if (viewportData.viewportResized)
        {
            RecreateFrameResources();
        }
        // Update the UI context
        //if (uiContext)
        //{
        //    uiContext->Update();
        //}
    }
	// -------------------------------------------------------
	
	void EditorApplication::OnSurfaceUpdate(uint32_t width, uint32_t height)
	{
        vkSwapChain->OnResize(viewportData.x, viewportData.y);
        viewportData.viewportResized = false;
	}
	
	void EditorApplication::RecreateFrameResources()
	{
	    while (Window::GetWidth() == 0 || Window::GetHeight() == 0)
	    {
	        Window::WaitEvents();
	    }
	
	    viewportData.GetViewportSize() = newViewportSize;
	
	    if (viewportData.x == 0 || viewportData.y == 0)
	    {
	        return;
	    }
	
	    vkDeviceWaitIdle(device);
	
	    if (Window::GetFramebufferResized() || WindowData::dirty == true)
	    {
            if (WindowData::dirty == true)
	        {
	            Window::ApplyChanges();
	        }
	        Window::UpdateFramebufferSize();
	        OnSurfaceUpdate(Window::GetWidth(), Window::GetHeight());
	    }
	}

    // -------------------------------------------------------
	
	void EditorApplication::DrawFrame()
	{
        /// Check if the viewport size has changed
        if (viewportData.x != newViewportSize.x || viewportData.y != newViewportSize.y)
	    {
	        if (newViewportSize.x > 0 && newViewportSize.y > 0)
	        {
                viewportData.GetViewportSize() = newViewportSize;
	            CleanupViewportResources();
	            CreateViewportResources();
	        }
	    }

        // Get a command buffer to render into
        VkCommandBuffer commandBuffer = gfxEngine.BeginSingleTimeCommands();

        // Set the command buffer for ImGui to render into
        ui.SetActiveCommandBuffer(commandBuffer);

        // Begin ImGui UI
        if (uiContext)
        {
            uiContext->Begin();

            // Draw your UI elements here
            ui.ShowDemoWindow();

            // Draw viewport if needed
            ui.ViewportWindow(viewportData, viewportData.viewportHovered, viewportData.viewportImageView);

            uiContext->End();
        }

        // End and submit the command buffer
        gfxEngine.EndSingleTimeCommands(commandBuffer);

        // Update frame counter
        frameCount = (frameCount + 1) % (1 << 15);
	}

} // namespace SceneryEditorX

// -------------------------------------------------------

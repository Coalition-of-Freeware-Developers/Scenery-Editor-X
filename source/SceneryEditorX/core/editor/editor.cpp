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

#include <SceneryEditorX/core/editor/editor.h>
#include <SceneryEditorX/core/window.h>
#include <SceneryEditorX/renderer/vk_core.h>
#include <SceneryEditorX/ui/ui.h>
#include <SceneryEditorX/ui/ui_context.h>

// ---------------------------------------------------------

namespace SceneryEditorX
{

	namespace UI
	{
	    class UIContextImpl;
	}


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

	void EditorApplication::InitEditor()
	{
	    Log::LogHeader();
	    SEDX_CORE_INFO("Scenery Editor X Engine is starting...");
	
	    //Launcher::AdminCheck();
	    //Launcher::Loader loader{};
	    //loader.run();
	    //SceneryEditorX::ReadCache();
	    //assetManager.LoadProject(cacheData.projectPath, cacheData.binPath);
	
	    //scene = assetManager.GetInitialScene();
	    //camera = assetManager.GetMainCamera(scene);
	    Window();
	}
	
	void EditorApplication::Create()
	{
	    //g_Window = CreateScope<Window>();
	    //g_Window->Create();
	
	    VulkanChecks vulkanChecks;           // Create a new instance of the VulkanChecks class
	    vulkanChecks.InitChecks({}, {}, {}); // Initialize the Vulkan checks

		// Create and configure the physical vkDevice
        auto physDevice = vkDevice->GetPhysicalDevice();
        physDevice->SelectDevice(VK_QUEUE_GRAPHICS_BIT, true);

        // Set up the features we need
        VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures = vkDeviceFeatures.GetPhysicalDeviceFeatures();

        // Update the vkDevice handle
        device = vkDevice->GetDevice();

	    //Window::SetTitle("Scenery Editor X | " + assetManager.GetProjectName());
        Ref<Window> windowRef = CreateRef<Window>(); 
        vkRenderer.CreateInstance(windowRef);
	
	    //createViewportResources();
	
        // Initialize UI components
        ui.InitGUI(Window::GetWindow(), vkRenderer);
        uiContext = std::shared_ptr<UI::UIContext>(UI::UIContext::Create());
        uiContext->SetGUI(&ui);

	    //SceneryEditorX::CreateEditor();
	
	    //vkRenderer = CreateRef<GraphicsEngine>(*Window::GetWindow());
	    //vkRenderer->CreateInstance();
	
	    //camera->extent = {viewportSize.x, viewportSize.y};
	}
	
	void EditorApplication::MainLoop()
	{
	    while (!Window::GetShouldClose())
	    {
            if (viewportData.viewportResized)
	        {
                vkSwapChain->OnResize(viewportData.viewportSize.x, viewportData.viewportSize.y);
                viewportData.viewportResized = false;
	        }
	
	        DrawFrame();
	        bool ctrlPressed = Window::IsKeyPressed(GLFW_KEY_LEFT_CONTROL) || Window::IsKeyDown(GLFW_KEY_LEFT_CONTROL);
	        Window::Update();
	    }

        vkDeviceWaitIdle(device);
	}
	
	void EditorApplication::CreateViewportResources()
	{

	    // Create image for the viewport
        vkSwapChain->CreateImage(viewportData.viewportSize.x, viewportData.viewportSize.y, 1,
	                           VK_SAMPLE_COUNT_1_BIT,
	                           vkSwapChain->GetColorFormat(),
	                           VK_IMAGE_TILING_OPTIMAL,
	                           VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
	                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                               viewportData.viewportImage,
                               viewportData.viewportImageMemory);
	
	    // Create image view for the viewport
        viewportData.viewportImageView = vkSwapChain->CreateImageView(viewportData.viewportImage, vkSwapChain->GetColorFormat(), VK_IMAGE_ASPECT_COLOR_BIT, 1);
	
	    // Create render pass for the viewport
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
	
	    if (vkCreateRenderPass(vkDevice->GetDevice(), &renderPassInfo, nullptr, &viewportData.viewportRenderPass) != VK_SUCCESS)
            throw std::runtime_error("Failed to create viewport render pass");

        // Create framebuffer for the viewport
	    VkFramebufferCreateInfo framebufferInfo{};
	    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = viewportData.viewportRenderPass;
	    framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &viewportData.viewportImageView;
        framebufferInfo.width = viewportData.viewportSize.x;
        framebufferInfo.height = viewportData.viewportSize.y;
	    framebufferInfo.layers = 1;
	
	    if (vkCreateFramebuffer(vkDevice->GetDevice(), &framebufferInfo, nullptr, &viewportData.viewportFramebuffer) != VK_SUCCESS)
	    {
	        throw std::runtime_error("Failed to create viewport framebuffer");
	    }
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

	// -------------------------------------------------------
	
	void EditorApplication::OnSurfaceUpdate(uint32_t width, uint32_t height)
	{
        vkSwapChain->OnResize(viewportData.viewportSize.x, viewportData.viewportSize.y);
        viewportData.viewportResized = false;
	}
	
	void EditorApplication::RecreateFrameResources()
	{
	    while (Window::GetWidth() == 0 || Window::GetHeight() == 0)
	    {
	        Window::WaitEvents();
	    }
	
	    viewportData.viewportSize = newViewportSize;
	
	    if (viewportData.viewportSize.x == 0 || viewportData.viewportSize.y == 0)
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
        if (viewportData.viewportSize.x != newViewportSize.x || viewportData.viewportSize.y != newViewportSize.y)
	    {
	        if (newViewportSize.x > 0 && newViewportSize.y > 0)
	        {
                viewportData.viewportSize = newViewportSize;
	            CleanupViewportResources();
	            CreateViewportResources();
	        }
	    }

        // Get a command buffer to render into
        VkCommandBuffer commandBuffer = vkRenderer.BeginSingleTimeCommands();

        // Set the command buffer for ImGui to render into
        ui.SetActiveCommandBuffer(commandBuffer);

        // Begin ImGui UI
        if (uiContext)
        {
            uiContext->Begin();

            // Draw your UI elements here
            ui.ShowDemoWindow();

            // Draw viewport if needed
            ui.ViewportWindow(viewportData.viewportSize, viewportData.viewportHovered, viewportData.viewportImageView);

            uiContext->End();
        }

        // End and submit the command buffer
        vkRenderer.EndSingleTimeCommands(commandBuffer);

        // Update frame counter
        frameCount = (frameCount + 1) % (1 << 15);
	}

    EditorApplication::~EditorApplication()
	{
	    vkRenderer.~GraphicsEngine();
	}

} // namespace SceneryEditorX

// -------------------------------------------------------

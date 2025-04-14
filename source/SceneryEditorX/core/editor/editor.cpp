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
#include <SceneryEditorX/scene/asset_manager.h>
#include <SceneryEditorX/ui/ui.h>

// ---------------------------------------------------------

namespace SceneryEditorX
{

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
	
	    //Window::SetTitle("Scenery Editor X | " + assetManager.GetProjectName());
	    vkRenderer.InitEngine(Window::GetWindow(), Window::GetWidth(), Window::GetHeight());
	
	    createViewportResources();
	
	    ui.initGUI(Window::GetWindow(), vkRenderer);
	
	    //SceneryEditorX::CreateEditor();
	
	    //vkRenderer = CreateRef<GraphicsEngine>(*Window::GetWindow());
	    //vkRenderer->InitEngine();
	
	    //camera->extent = {viewportSize.x, viewportSize.y};
	}
	
	void EditorApplication::MainLoop()
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
	
	/*
	void WaitIdle()
	{
	    vkDeviceWaitIdle(vkRenderer.GetDevice());
	    if (result != VK_SUCCESS)
	    {
	        SEDX_CORE_ERROR("Failed to wait for device to become idle: {}", ToString(result));
	    }
	}
	*/
	
	void EditorApplication::CreateViewportResources()
	{
	    // Create image for the viewport
	    vkRenderer.createImage(viewportSize.x,
	                           viewportSize.y,
	                           1,
	                           VK_SAMPLE_COUNT_1_BIT,
	                           vkRenderer.GetSwapChainImageFormat(),
	                           VK_IMAGE_TILING_OPTIMAL,
	                           VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
	                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	                           viewportImage,
	                           viewportImageMemory);
	
	    // Create image view for the viewport
	    viewportImageView = vkRenderer.createImageView(viewportImage,
	                                                   vkRenderer.GetSwapChainImageFormat(),
	                                                   VK_IMAGE_ASPECT_COLOR_BIT,
	                                                   1);
	
	    // Create render pass for the viewport
	    VkAttachmentDescription colorAttachment{};
	    colorAttachment.format = vkRenderer.GetSwapChainImageFormat();
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
	
	    if (vkCreateRenderPass(vkRenderer.GetDevice(), &renderPassInfo, nullptr, &viewportRenderPass) != VK_SUCCESS)
	    {
	        throw std::runtime_error("Failed to create viewport render pass");
	    }
	
	    // Create framebuffer for the viewport
	    VkFramebufferCreateInfo framebufferInfo{};
	    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	    framebufferInfo.renderPass = viewportRenderPass;
	    framebufferInfo.attachmentCount = 1;
	    framebufferInfo.pAttachments = &viewportImageView;
	    framebufferInfo.width = viewportSize.x;
	    framebufferInfo.height = viewportSize.y;
	    framebufferInfo.layers = 1;
	
	    if (vkCreateFramebuffer(vkRenderer.GetDevice(), &framebufferInfo, nullptr, &viewportFramebuffer) != VK_SUCCESS)
	    {
	        throw std::runtime_error("Failed to create viewport framebuffer");
	    }
	}
	
	void EditorApplication::CleanupViewportResources()
	{
	    VkDevice device = vkRenderer.GetDevice();
	
	    if (viewportFramebuffer != VK_NULL_HANDLE)
	    {
	        vkDestroyFramebuffer(device, viewportFramebuffer, nullptr);
	        viewportFramebuffer = VK_NULL_HANDLE;
	    }
	
	    if (viewportRenderPass != VK_NULL_HANDLE)
	    {
	        vkDestroyRenderPass(device, viewportRenderPass, nullptr);
	        viewportRenderPass = VK_NULL_HANDLE;
	    }
	
	    if (viewportImageView != VK_NULL_HANDLE)
	    {
	        vkDestroyImageView(device, viewportImageView, nullptr);
	        viewportImageView = VK_NULL_HANDLE;
	    }
	
	    if (viewportImage != VK_NULL_HANDLE)
	    {
	        vkDestroyImage(device, viewportImage, nullptr);
	        viewportImage = VK_NULL_HANDLE;
	    }
	
	    if (viewportImageMemory != VK_NULL_HANDLE)
	    {
	        vkFreeMemory(device, viewportImageMemory, nullptr);
	        viewportImageMemory = VK_NULL_HANDLE;
	    }
	}
	// -------------------------------------------------------
	
	void EditorApplication::OnSurfaceUpdate(uint32_t width, uint32_t height)
	{
	    vkRenderer.cleanupSwapChain();
	    vkRenderer.recreateSurfaceFormats();
	    vkRenderer.createSwapChain();
	}
	
	void EditorApplication::RecreateFrameResources()
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
	
	void EditorApplication::DrawFrame()
	{
	    if (viewportSize.x != newViewportSize.x || viewportSize.y != newViewportSize.y)
	    {
	        if (newViewportSize.x > 0 && newViewportSize.y > 0)
	        {
	            viewportSize = newViewportSize;
	            cleanupViewportResources();
	            createViewportResources();
	        }
	    }
	
	    static UI::UIManager uiManager;
	
	    vkRenderer.renderFrame();
	
	    uiManager.ViewportWindow(newViewportSize, viewportHovered, viewportImageView);
	
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

    EditorApplication::EditorApplication()
    {
    }

	EditorApplication::~EditorApplication()
	{
	    vkRenderer.~GraphicsEngine();
	}
	
	/*
	bool GetSwapChainDirty()
	{
	    return swapChainDirty;
	}
	*/
	
	//Ref<SceneryEditorX::GraphicsEngine> vkRenderer; //Vulkan renderer instance


} // namespace SceneryEditorX

// -------------------------------------------------------

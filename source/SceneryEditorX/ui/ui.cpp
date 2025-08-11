/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* ui.cpp
* -------------------------------------------------------
* Created: 25/3/2025
* -------------------------------------------------------
*/
#include <Editor/core/viewport.h>

#include <SceneryEditorX/core/application/application.h>
#include <SceneryEditorX/core/window/window.h>
#include <SceneryEditorX/renderer/render_context.h>
#include <SceneryEditorX/renderer/vulkan/vk_device.h>
#include <SceneryEditorX/renderer/vulkan/vk_swapchain.h>
#include <SceneryEditorX/renderer/vulkan/vk_util.h>
#include <SceneryEditorX/ui/ui.h>
#include <imgui/imconfig.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

/// -------------------------------------------------------

/**
 * Implementation of missing ImGui functions to fix linker errors
 * @note: This is a compatibility layer to ensure ImGui works correctly
 *
 * This implementation may need to be adjusted based on your ImGui version.
 * It provides stubs for functionality that might be missing in your current build.
 */

/*
extern "C"
{
    // Implement missing ImGui functions referenced in the codebase
    IMGUI_API bool ImGui_BeginTable(const char *str_id,
                                    int columns,
                                    ImGuiTableFlags flags,
                                    const ImVec2 &outer_size,
                                    float inner_width)
    {
        // Simplified implementation - if you're not using tables extensively
        // Just return false or implement a basic version
        return false; // Or implement based on your needs
    }

    // Implementation for GetStyle
    IMGUI_API ImGuiStyle &ImGui_GetStyle(void)
    {
        static ImGuiStyle style;
        return style;
    }

    // Implementation for ColorConvertU32ToFloat4
    IMGUI_API ImVec4 ImGui_ColorConvertU32ToFloat4(unsigned int color)
    {
        // Convert RGBA color to ImVec4
        float r = (color >> 0 & 0xFF) / 255.0f;
        float g = (color >> 8 & 0xFF) / 255.0f;
        float b = (color >> 16 & 0xFF) / 255.0f;
        float a = (color >> 24 & 0xFF) / 255.0f;
        return {r, g, b, a};
    }
}
*/

/// -------------------------------------------------------

namespace SceneryEditorX::UI
{
	/// Initialize static members
	bool GUI::visible = true;
	const std::string GUI::defaultFont = "Roboto-Regular";
    static std::vector<VkCommandBuffer> uiCommandBuffers;

	/// Additional ImGui initialization functions can be placed here if needed
	void initImGuiExtensions()
	{
	    /// This function can be called from main ImGui setup to initialize any extensions
	    /// Currently empty, but could be expanded if more ImGui features need integration
	}
	
    GUI::GUI() = default;

    GUI::~GUI()
    {
        CleanUp();
    }

    /// -------------------------------------------------------

    bool GUI::CreateDescriptorPool()
    {
        auto device = RenderContext::GetCurrentDevice()->GetDevice();
        if (!device)
        {
            SEDX_CORE_ERROR("Cannot create ImGui descriptor pool: device is null");
            return false;
        }

        /// Create separate descriptor pool for ImGui with FREE_DESCRIPTOR_SET_BIT
        const VkDescriptorPoolSize poolSizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 100},
                                            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100},
                                            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 100},
                                            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100},
                                            {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 100},
                                            {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 100},
                                            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100},
                                            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100},
                                            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100},
                                            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 100},
                                            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 100}};

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets = 1000; /// Increased from 100 to handle more UI elements
        poolInfo.poolSizeCount = std::size(poolSizes);
        poolInfo.pPoolSizes = poolSizes;

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &imguiPool) != VK_SUCCESS)
        {
            SEDX_CORE_ERROR("Failed to create ImGui descriptor pool!");
            return false;
        }

        return true;
    }

    /// -------------------------------------------------------

    void GUI::UpdateDpiScale()
    {
        const auto window = static_cast<GLFWwindow *>(Application::Get().GetWindow().GetWindow());
        /// Get content scale from GLFW
        float xScale, yScale;
        glfwGetWindowContentScale(window, &xScale, &yScale);
        contentScaleFactor = xScale;

        /// Get monitor DPI info if available
        if (GLFWmonitor *monitor = glfwGetPrimaryMonitor())
        {
            float xDpi, yDpi;
            glfwGetMonitorContentScale(monitor, &xDpi, &yDpi);
            dpiFactor = xDpi;
        }
        else
            dpiFactor = xScale;

        /// Update ImGui style to reflect DPI changes
        ImGuiStyle &style = ImGui::GetStyle();
        style.ScaleAllSizes(dpiFactor);
    }

    /// -------------------------------------------------------

    bool GUI::InitGUI()
    {
        auto device = RenderContext::GetCurrentDevice()->GetDevice();
        auto window = static_cast<GLFWwindow *>(Application::Get().GetWindow().GetWindow());
        if (initialized)
        {
            SEDX_CORE_WARN("GUI already initialized");
            return true;
        }

		/*
        if (!device || !swapchain)
        {
            SEDX_CORE_ERROR("Failed to get valid Vulkan device or swapchain");
            return false;
        }
        */

        /// Create descriptor pool
        if (!CreateDescriptorPool())
            return false;

        /// Initialize ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO(); (void)io;

        /// Configure ImGui features
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   /// Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;		/// Enable docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;		/// Enable multi-viewport
        io.ConfigDockingWithShift = false;						/// Don't require shift for docking
        io.ConfigWindowsResizeFromEdges = true;					/// Enable resizing windows from edges
        io.ConfigWindowsMoveFromTitleBarOnly = false;			/// Allow moving windows from anywhere

        /// Set ImGui style and fonts
        SetStyle();
        SetFonts();

        /// Initialize GLFW backend
        ImGui_ImplGlfw_InitForVulkan(window, true);
		
        /// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
        ImGuiStyle &style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.15f, 0.15f, style.Colors[ImGuiCol_WindowBg].w);

	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// GUI Descriptor Pool Creation
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        VkDescriptorPool descriptorPool;
        VkDescriptorPoolSize pool_sizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 100},
                                             {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100},
                                             {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 100},
                                             {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100},
                                             {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 100},
                                             {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 100},
                                             {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100},
                                             {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100},
                                             {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100},
                                             {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 100},
                                             {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 100}};


		VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 100 * IM_ARRAYSIZE(pool_sizes);
        pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        VK_CHECK_RESULT(vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptorPool))

        /// -------------------------------------------------------

        RenderData renderData;
        auto physDevice = RenderContext::GetCurrentDevice()->GetPhysicalDevice();
        SwapChain &swapChain = Application::Get().GetWindow().GetSwapChain();

		ImGui_ImplGlfw_InitForVulkan(window, true);
        ImGui_ImplVulkan_InitInfo info{};
        info.Instance = RenderContext::GetInstance();
        info.PhysicalDevice = physDevice->Selected().physicalDevice;
        info.Device = device;
        info.QueueFamily = physDevice->GetQueueFamilyIndices().GetGraphicsFamily();
        info.Queue = RenderContext::GetCurrentDevice()->GetGraphicsQueue();
        info.DescriptorPool = imguiPool;
        info.PipelineCache = nullptr;
        info.Allocator = nullptr;
        info.MinImageCount = 2;
        info.ImageCount = swapChain.GetSwapChainImageCount();
        info.MSAASamples = VK_SAMPLE_COUNT_1_BIT; /// Use MSAA samples from renderer later
#if defined (VK_VERSION_1_2) || defined (VK_VERSION_1_3) || defined (VK_VERSION_1_4)
        info.UseDynamicRendering = true;
#else
        info.UseDynamicRendering = false; /// Dynamic rendering requires Vulkan 1.3 or higher
        info.RenderPass = renderData.renderPass; /// Use a valid render pass if dynamic rendering is not available
#endif
        info.CheckVkResultFn = VulkanCheckResult;

        /*
        info.CheckVkResultFn = [](const VkResult result)
        {
            if (result != VK_SUCCESS)
                SEDX_CORE_ERROR("Vulkan UI Error: {}", static_cast<int>(result));
        };
        */

        /// Initialize Vulkan implementation
        ImGui_ImplVulkan_Init(&info);


        {
            /// Font upload objects are now handled by ImGui internally
			/// Upload fonts to GPU
			//VkCommandBuffer commandBuffer = renderer.BeginSingleTimeCommands();
			//renderer.EndSingleTimeCommands(commandBuffer);
        }

        /// Wait for font upload to complete
        vkDeviceWaitIdle(device);
        /// Update DPI scale
        UpdateDpiScale();

		uint32_t framesInFlight = renderData.framesInFlight;
        uiCommandBuffers.resize(framesInFlight);
        for (uint32_t i = 0; i < framesInFlight; ++i)
            uiCommandBuffers[i] = RenderContext::GetCurrentDevice()->CreateUICmdBuffer("UI-CommandBuffer");

        initialized = true;
        SEDX_CORE_INFO("ImGui initialized successfully");
        return true;
    }

    void GUI::BeginFrame() const
    {
        if (!initialized || !visible)
            return;

        /// Start the ImGui frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        //ImGuizmo::BeginFrame();
    }

    void GUI::EndFrame() const
    {
        if (!initialized || !visible)
            return;

        /// Render the ImGui frame
        ImGui::Render();

		SwapChain &swapChain = Application::Get().GetWindow().GetSwapChain();

        VkClearValue clearValues[2];
        clearValues[0].color = {{0.1f, 0.1f, 0.1f, 1.0f}};
        clearValues[1].depthStencil = {.depth = 1.0f, .stencil = 0};

        uint32_t width	= swapChain.GetWidth();
        uint32_t height = swapChain.GetHeight();

        uint32_t commandBufferIndex = swapChain.GetBufferIndex();

        VkCommandBufferBeginInfo drawCmdBufInfo = {};
        drawCmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        drawCmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        drawCmdBufInfo.pNext = nullptr;

        VkCommandBuffer drawCommandBuffer = swapChain.GetActiveDrawCommandBuffer();
        VK_CHECK_RESULT(vkBeginCommandBuffer(drawCommandBuffer, &drawCmdBufInfo))

        VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.pNext = nullptr;
		renderPassBeginInfo.renderPass = swapChain.GetRenderPass();
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = width;
		renderPassBeginInfo.renderArea.extent.height = height;
		renderPassBeginInfo.clearValueCount = 2; // Color + depth
		renderPassBeginInfo.pClearValues = clearValues;
		renderPassBeginInfo.framebuffer = swapChain.GetActiveFramebuffer();

        vkCmdBeginRenderPass(drawCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

        VkCommandBufferInheritanceInfo inheritanceInfo = {};
        inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        inheritanceInfo.renderPass = swapChain.GetRenderPass();
        inheritanceInfo.framebuffer = swapChain.GetActiveFramebuffer();

        VkCommandBufferBeginInfo cmdBufInfo = {};
        cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
        cmdBufInfo.pInheritanceInfo = &inheritanceInfo;

        VK_CHECK_RESULT(vkBeginCommandBuffer(uiCommandBuffers[commandBufferIndex], &cmdBufInfo))

        VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = (float)height;
		viewport.height = -(float)height;
		viewport.width = (float)width;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(uiCommandBuffers[commandBufferIndex], 0, 1, &viewport);

        VkRect2D scissor = {};
        scissor.extent.width = width;
        scissor.extent.height = height;
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        vkCmdSetScissor(uiCommandBuffers[commandBufferIndex], 0, 1, &scissor);

        ImDrawData *main_draw_data = ImGui::GetDrawData();
        ImGui_ImplVulkan_RenderDrawData(main_draw_data, uiCommandBuffers[commandBufferIndex]);

        VK_CHECK_RESULT(vkEndCommandBuffer(uiCommandBuffers[commandBufferIndex]))

        std::vector<VkCommandBuffer> commandBuffers;
        commandBuffers.push_back(uiCommandBuffers[commandBufferIndex]);

        vkCmdExecuteCommands(drawCommandBuffer, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

        vkCmdEndRenderPass(drawCommandBuffer);

        VK_CHECK_RESULT(vkEndCommandBuffer(drawCommandBuffer))

        /// Update and render additional platform windows
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }
	
    /// -------------------------------------------------------

    void GUI::CleanUp()
    {
        const auto device = RenderContext::GetCurrentDevice()->GetDevice();
        if (!initialized)
            return;

        vkDeviceWaitIdle(device);

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();

        if (imguiPool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(device, imguiPool, nullptr);
            imguiPool = VK_NULL_HANDLE;
        }

        ImGui::DestroyContext();
        initialized = false;

        SEDX_CORE_INFO("GUI resources cleaned up");
    }

    /// -------------------------------------------------------

    void GUI::Resize(uint32_t width, uint32_t height)
    {
        if (!initialized)
            return;

        /// Update DPI scale if needed
        UpdateDpiScale();

        SEDX_CORE_INFO("GUI resized to {}x{}", width, height);
    }

    /// -------------------------------------------------------

    void GUI::Update(float deltaTime) const
    {
        if (!initialized || !visible)
            return;

        /// Any per-frame updates not related to drawing would go here
    }

#ifdef SEDX_DEBUG
    void GUI::ShowDemoWindow(bool *open) const
    {
        if (!initialized || !visible)
            return;

        ImGui::ShowDemoWindow(open);
    }
#endif

    /// -------------------------------------------------------

    void GUI::ShowAppInfo(const std::string &appName) const
    {
        if (!initialized || !visible)
            return;

        ImGui::Begin("Application Info", nullptr, commonFlags | infoFlags);

        ImGui::Text("Application: %s", appName.c_str());
        ImGui::Text("Renderer: %s", "X-Plane 12 Graphics Emulator");
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Text("Frame Time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);

        /// Add system info
        ImGui::Separator();
        ImGui::Text("Window Size: %d x %d",
                    static_cast<int>(ImGui::GetIO().DisplaySize.x),
                    static_cast<int>(ImGui::GetIO().DisplaySize.y));
        ImGui::Text("DPI Scale: %.2f", dpiFactor);

        ImGui::End();
    }

    /// -------------------------------------------------------

    bool GUI::InitViewport(const Viewport &size, VkImageView imageView)
    {
        if (!initialized)
            return false;

        viewportInitialized = true;
        return true;
    }

    /// -------------------------------------------------------

    void GUI::ViewportWindow(ImVec2 &size, bool &hovered, VkImageView imageView) const
    {
        if (!initialized || !visible || !viewportInitialized)
            return;

        /// Start viewport window with dockable behavior
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        /// Get content region size
        ImVec2 viewportSize = ImGui::GetContentRegionAvail();
        size.x = viewportSize.x;
        size.y = viewportSize.y;
        hovered = ImGui::IsWindowHovered();

        /// Display the image view as a texture
        if (imageView != VK_NULL_HANDLE)
        {
            ImTextureID texID = GetTextureID(imageView, VK_NULL_HANDLE);
            ImGui::Image(texID, viewportSize);
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }

    /// -------------------------------------------------------

    ImTextureID GUI::GetTextureID(const VkImageView imageView, VkSampler sampler, const VkImageLayout layout) const
    {
        const auto device = RenderContext::GetCurrentDevice();
        if (!initialized || imageView == VK_NULL_HANDLE)
            return reinterpret_cast<ImTextureID>(nullptr);

        /// Check if device is valid
        if (device == nullptr)
        {
            SEDX_CORE_ERROR("Cannot get texture ID: device is null");
            return reinterpret_cast<ImTextureID>(nullptr);
        }

        /// Get a sampler if none was provided
        VkSampler actualSampler = sampler;
        if (actualSampler == VK_NULL_HANDLE)
        {
            /// If we have a valid device, use its sampler
            actualSampler = device->GetSampler();

            /// If we still don't have a valid sampler, we can't proceed
            if (actualSampler == VK_NULL_HANDLE)
            {
                SEDX_CORE_ERROR("Cannot get texture ID: no valid sampler available");
                return reinterpret_cast<ImTextureID>(nullptr);
            }
        }

        /// Now we have a valid sampler and imageView
        /// ImGui_ImplVulkan_AddTexture returns VkDescriptorSet which needs to be cast to ImTextureID
        VkDescriptorSet descriptorSet = ImGui_ImplVulkan_AddTexture(actualSampler, imageView, layout);
        return reinterpret_cast<ImTextureID>(descriptorSet);
    }

    /// -------------------------------------------------------

    void GUI::SetStyle()
    {
        constexpr auto ColorFromBytes = [](uint8_t r, uint8_t g, uint8_t b)
	    {
            return ImVec4((float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, 1.0f);
        };

        auto &style = ImGui::GetStyle();
        ImVec4 *colors = style.Colors;

        constexpr ImVec4 bgColor = ColorFromBytes(37, 37, 38);
        constexpr ImVec4 lightBgColor = ColorFromBytes(82, 82, 85);
        constexpr ImVec4 veryLightBgColor = ColorFromBytes(90, 90, 95);

        constexpr ImVec4 panelColor = ColorFromBytes(51, 51, 55);
        constexpr ImVec4 panelHoverColor = ColorFromBytes(29, 151, 236);
        constexpr ImVec4 panelActiveColor = ColorFromBytes(0, 119, 200);

        constexpr ImVec4 textColor = ColorFromBytes(255, 255, 255);
        constexpr ImVec4 textDisabledColor = ColorFromBytes(151, 151, 151);
        constexpr ImVec4 borderColor = ColorFromBytes(78, 78, 78);

        colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.15f, 0.15f, 0.95f); // More opaque
        colors[ImGuiCol_Text] = textColor;
        colors[ImGuiCol_TextDisabled] = textDisabledColor;
        colors[ImGuiCol_TextSelectedBg] = panelActiveColor;
        colors[ImGuiCol_ChildBg] = bgColor;
        colors[ImGuiCol_PopupBg] = ImVec4(0.15f, 0.15f, 0.15f, 0.98f); // More opaque
        colors[ImGuiCol_Border] = borderColor;
        colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.25f);
        colors[ImGuiCol_FrameBg] = panelColor;
        colors[ImGuiCol_FrameBgHovered] = panelHoverColor;
        colors[ImGuiCol_FrameBgActive] = panelActiveColor;
        colors[ImGuiCol_TitleBg] = bgColor;
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.15f, 0.15f, 0.15f, 0.75f);
        colors[ImGuiCol_MenuBarBg] = panelColor;
        colors[ImGuiCol_ScrollbarBg] = panelColor;
        colors[ImGuiCol_ScrollbarGrab] = lightBgColor;
        colors[ImGuiCol_ScrollbarGrabHovered] = veryLightBgColor;
        colors[ImGuiCol_ScrollbarGrabActive] = veryLightBgColor;
        colors[ImGuiCol_CheckMark] = ColorFromBytes(0, 213, 255); // Brighter check mark
        colors[ImGuiCol_SliderGrab] = panelHoverColor;
        colors[ImGuiCol_SliderGrabActive] = panelActiveColor;
        colors[ImGuiCol_Button] = panelColor;
        colors[ImGuiCol_ButtonHovered] = panelHoverColor;
        colors[ImGuiCol_ButtonActive] = panelActiveColor;
        colors[ImGuiCol_Header] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
        colors[ImGuiCol_HeaderHovered] = panelHoverColor;
        colors[ImGuiCol_HeaderActive] = panelActiveColor;
        colors[ImGuiCol_Separator] = borderColor;
        colors[ImGuiCol_SeparatorHovered] = ColorFromBytes(95, 95, 95);
        colors[ImGuiCol_SeparatorActive] = ColorFromBytes(120, 120, 120);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.2f, 0.2f, 0.2f, 0.5f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.3f, 0.3f, 0.3f, 0.75f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
        colors[ImGuiCol_PlotLines] = panelActiveColor;
        colors[ImGuiCol_PlotLinesHovered] = panelHoverColor;
        colors[ImGuiCol_PlotHistogram] = panelActiveColor;
        colors[ImGuiCol_PlotHistogramHovered] = panelHoverColor;
        colors[ImGuiCol_DragDropTarget] = ImVec4(0.0f, 0.5f, 1.0f, 0.9f);
        colors[ImGuiCol_NavHighlight] = panelActiveColor;
        colors[ImGuiCol_DockingPreview] = ImVec4(0.0f, 0.5f, 1.0f, 0.7f);
        colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.15f, 0.15f, 0.9f);
        colors[ImGuiCol_TabActive] = panelActiveColor;
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.15f, 0.15f, 0.15f, 0.8f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
        colors[ImGuiCol_TabHovered] = panelHoverColor;

        /// Improved style settings
        style.WindowRounding = 2.0f;
        style.ChildRounding = 2.0f;
        style.FrameRounding = 2.0f;
        style.GrabRounding = 2.0f;
        style.PopupRounding = 2.0f;
        style.ScrollbarRounding = 2.0f;
        style.TabRounding = 2.0f;

        /// Better sizing and spacing
        style.WindowPadding = ImVec2(10, 10);
        style.WindowBorderSize = 1.0f;
        style.FramePadding = ImVec2(8, 4);
        style.ItemSpacing = ImVec2(10, 4);
        style.ItemInnerSpacing = ImVec2(4, 4);

        /// Improve usability
        style.TouchExtraPadding = ImVec2(0, 0);
        style.IndentSpacing = 21.0f;
        style.ScrollbarSize = 14.0f;
        style.GrabMinSize = 10.0f;
    }

    /// -------------------------------------------------------

    void GUI::SetFonts() const
    {
        ImGuiIO &io = ImGui::GetIO();

        /// Clear existing fonts
        io.Fonts->Clear();

        /// Default font size based on DPI
        const float fontSize = 16.0f * dpiFactor;

        /// Add default font
        io.Fonts->AddFontDefault();

        /// Add roboto font if available
        /// (path would need to be determined based on your asset system)
        std::string fontPath = "assets/fonts/Roboto-Regular.ttf";

        /// Example: Add font with icon merging
        /// Configure icon font
        static constexpr ImWchar iconRanges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
        ImFontConfig iconConfig;
        iconConfig.MergeMode = true;
        iconConfig.PixelSnapH = true;
        iconConfig.GlyphMinAdvanceX = fontSize;

        /// Standard font
        ImFontConfig fontConfig;
        fontConfig.SizePixels = fontSize;

        /// Default to using a built-in font if loading fails
        ImFont *mainFont = nullptr;

        /// Try to load custom font
        // mainFont = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize, &fontConfig);

        // If not available, use default
        if (!mainFont)
            mainFont = io.Fonts->AddFontDefault();

        /// Add icons to the font
        // io.Fonts->AddFontFromMemoryCompressedTTF(
        //     FontAwesome_compressed_data,
        //     FontAwesome_compressed_size,
        //     fontSize * 0.8f,
        //     &iconConfig,
        //     iconRanges);

        /// Set default font
        io.FontDefault = mainFont;

        /// Build font atlas
        io.Fonts->Build();
    }

}

/// -------------------------------------------------------

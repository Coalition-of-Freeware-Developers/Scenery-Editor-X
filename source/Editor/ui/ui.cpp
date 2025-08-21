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
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/imconfig.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <SceneryEditorX/core/window/window.h>
#include <SceneryEditorX/renderer/render_context.h>
#include <SceneryEditorX/renderer/vulkan/vk_device.h>
#include <SceneryEditorX/ui/ui.h>

// -------------------------------------------------------

// Implementation of missing ImGui functions to fix linker errors
// Note: This is a compatibility layer to ensure ImGui works correctly

// This implementation may need to be adjusted based on your ImGui version
// It provides stubs for functionality that might be missing in your current build

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


namespace SceneryEditorX::UI
{
	/// Initialize static members
	bool GUI::visible = true;
	const std::string GUI::defaultFont = "Roboto-Regular";


	/// Additional ImGui initialization functions can be placed here if needed
	void initImGuiExtensions()
	{
	    // This function can be called from main ImGui setup to initialize any extensions
	    // Currently empty, but could be expanded if more ImGui features need integration
	}
	
    GUI::GUI() = default;

    GUI::~GUI()
    {
        CleanUp();
    }

    bool GUI::CreateDescriptorPool()
    {
        auto device = RenderContext::GetCurrentDevice();
        RenderContext context;
        if (!device)
        {
            EDITOR_ERROR("Cannot create ImGui descriptor pool: device is null");
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

        if (vkCreateDescriptorPool(device->GetDevice(), &poolInfo, context.allocatorCallback, &imguiPool) != VK_SUCCESS)
        {
            EDITOR_ERROR("Failed to create ImGui descriptor pool!");
            return false;
        }

        return true;
    }

    void GUI::UpdateDpiScale()
    {
        auto &app = Application::Get();
        GLFWwindow *window = static_cast<GLFWwindow *>(app.GetWindow().GetWindow());
        
        // Get content scale from GLFW
        float xScale, yScale;
        glfwGetWindowContentScale(window, &xScale, &yScale);
        contentScaleFactor = xScale;

        // Get monitor DPI info if available
        if (GLFWmonitor *monitor = glfwGetPrimaryMonitor())
        {
            float xDpi, yDpi;
            glfwGetMonitorContentScale(monitor, &xDpi, &yDpi);
            dpiFactor = xDpi;
        }
        else
        {
            dpiFactor = xScale;
        }

        // Update ImGui style to reflect DPI changes
        ImGuiStyle &style = ImGui::GetStyle();
        style.ScaleAllSizes(dpiFactor);
        
        EDITOR_INFO("Updated DPI scale: {}", dpiFactor);
    }

    bool GUI::InitGUI()
    {
        auto device = RenderContext::GetCurrentDevice();
        auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetWindow());

        if (initialized)
        {
            EDITOR_WARN("GUI already initialized");
            return true;
        }

        /// Get essential Vulkan objects
        //device = GraphicsEngine::GetDevice()->GetDevice();
        //swapchain = renderer.GetSwapChain().Get();

        if (!device /*|| !swapchain*/)
        {
            EDITOR_ERROR("Failed to get valid Vulkan device or swapchain");
            return false;
        }

        /// Create descriptor pool
        if (!CreateDescriptorPool())
            return false;

        /// Initialize ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();

        /// Configure ImGui features
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   /// Enable docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; /// Enable multi-viewport
        io.ConfigDockingWithShift = false;                  /// Don't require shift for docking
        io.ConfigWindowsResizeFromEdges = true;             /// Enable resizing windows from edges
        io.ConfigWindowsMoveFromTitleBarOnly = false;       /// Allow moving windows from anywhere

        /// Initialize GLFW backend
        ImGui_ImplGlfw_InitForVulkan(window, true);

        /// Get queue family info
        RenderData renderData;

        /// Initialize Vulkan backend
        ImGui_ImplVulkan_InitInfo info{};
        info.Instance = RenderContext::GetInstance();
        info.PhysicalDevice = device->GetPhysicalDevice()->GetGPUDevices();
        info.QueueFamily = device->GetPhysicalDevice()->GetQueueFamilyIndices().GetGraphicsFamily();
        info.Queue = device->GetGraphicsQueue();
        info.DescriptorPool = imguiPool;
        //info.RenderPass = renderer.GetRenderPass();
        info.MinImageCount = 2;
        info.ImageCount = renderData.imageIndex;
        info.MSAASamples = VK_SAMPLE_COUNT_1_BIT; /// Use MSAA samples from renderer later
        info.Allocator = nullptr;
        info.CheckVkResultFn = [](const VkResult result)
        {
            if (result != VK_SUCCESS)
                EDITOR_ERROR("ImGui Vulkan Error: {}", static_cast<int>(result));
        };

        /// Initialize Vulkan implementation
        if (!ImGui_ImplVulkan_Init(&info))
        {
            EDITOR_ERROR("Failed to initialize ImGui Vulkan implementation");
            return false;
        }

        /// Upload fonts to GPU
        //VkCommandBuffer commandBuffer = renderer.BeginSingleTimeCommands();
        //renderer.EndSingleTimeCommands(commandBuffer);

        /// Wait for font upload to complete
        vkDeviceWaitIdle(device->GetDevice());

        /// Font upload objects are now handled by ImGui internally

        /// Set ImGui style and fonts
        SetStyle();
        SetFonts();

        /// Update DPI scale
        UpdateDpiScale();

        initialized = true;
        EDITOR_INFO("ImGui initialized successfully");
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
    }

    void GUI::EndFrame() const
    {
        if (!initialized || !visible)
            return;

        /// Render the ImGui frame
        ImGui::Render();

        if (activeCommandBuffer != VK_NULL_HANDLE)
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), activeCommandBuffer);

        /// Update and render additional platform windows
        ImGuiIO &io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }
	
	
    void GUI::CleanUp()
    {
        auto device = RenderContext::GetCurrentDevice();
        if (!initialized)
            return;

        vkDeviceWaitIdle(device->GetDevice());

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();

        if (imguiPool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(device->GetDevice(), imguiPool, nullptr);
            imguiPool = VK_NULL_HANDLE;
        }

        ImGui::DestroyContext();
        initialized = false;

        EDITOR_INFO("ImGui resources cleaned up");
    }

    void GUI::Resize(uint32_t width, uint32_t height)
    {
        if (!initialized)
            return;

        /// Update DPI scale if needed
        UpdateDpiScale();

        EDITOR_INFO("GUI resized to {}x{}", width, height);
    }

    void GUI::Update(float deltaTime) const
    {
        if (!initialized || !visible)
            return;

    }

    void GUI::ShowDemoWindow(bool *open) const
    {
        if (!initialized || !visible)
            return;

        ImGui::ShowDemoWindow(open);
    }
	
	/*
	void GUI::InitGUI(GLFWwindow *window, SceneryEditorX::GraphicsEngine &renderer)
	{
	    // Store the engine reference
	    this->renderer = &renderer;
	    this->window = window;
	
	    // Create separate descriptor pool for ImGui with FREE_DESCRIPTOR_SET_BIT
	    VkDescriptorPoolSize pool_sizes[] = {
	        {VK_DESCRIPTOR_TYPE_SAMPLER, 100},
	        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100},
	        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 100},
	        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100},
	        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 100},
	        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 100},
	        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100},
	        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100},
	        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100},
	        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 100},
	        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 100}
	    };
	
	    VkDescriptorPoolCreateInfo pool_info = {};
	    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	    pool_info.maxSets = 100;
	    pool_info.poolSizeCount = std::size(pool_sizes);
	    pool_info.pPoolSizes = pool_sizes;
	
	    VkDescriptorPool imguiPool;
	    if (vkCreateDescriptorPool(device->GetDevice(), &pool_info, nullptr, &imguiPool) != VK_SUCCESS)
	    {
	        EDITOR_ERROR("Failed to create ImGui descriptor pool!");
	        return;
	    }
	
	    SceneryEditorX::QueueFamilyIndices indices = device->GetPhysicalDevice()->GetQueueFamilyIndices();
	
	    ImGui::CreateContext();
	    ImGuiIO& io = ImGui::GetIO();
	    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Enable docking
	    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;    // Enable multi-viewport / platform windows
	    io.ConfigDockingWithShift = false;                     // Don't require shift for docking
	    io.ConfigWindowsResizeFromEdges = true;                // Enable resizing windows from edges
	
	    ImGui_ImplGlfw_InitForVulkan(Window::GetWindow(), true);
	
	    ImGui_ImplVulkan_InitInfo info{};
	    info.ApiVersion = VK_API_VERSION_1_3;
	    info.Instance = SceneryEditorX::GraphicsEngine::GetInstance();
	    info.PhysicalDevice = device->GetPhysicalDevice()->GetGPUDevice();
	    info.Device = device->GetDevice();
	    info.QueueFamily = indices.graphicsFamily.value();
	    info.Queue = device->GetGraphicsQueue();
	    info.DescriptorPool = imguiPool;
	    info.RenderPass = renderer->GetRenderPass();
	    //info.Subpass = 0;
	    info.MinImageCount = swapchain->GetSwapChainImages().size();
	    info.ImageCount = swapchain->GetSwapChainImages().size();
	    info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;  /* TODO: Replace when MSAA is implemented properly. #1#
	    //info.MSAASamples = renderer->msaaSamples;
	    //info.Allocator = nullptr;
	    info.UseDynamicRendering = false;
	    info.CheckVkResultFn = [](const VkResult result)
	    {
	        if (result != VK_SUCCESS)
	        {
	            EDITOR_ERROR("ImGui Vulkan Error: {}", ToString(result));
	        }
	    };
	
	    ImGui_ImplVulkan_Init(&info);
	
	    // Upload fonts to GPU
	    VkCommandBuffer commandBuffer = renderer->beginSingleTimeCommands();
	    ImGui_ImplVulkan_CreateFontsTexture(); // Removed Nov 10, 2023 Commit #79a9e2f
	    renderer->endSingleTimeCommands(commandBuffer);
	
	    vkDeviceWaitIdle(device->GetDevice());// Wait for font upload to complete
	    //ImGui_ImplVulkan_DestroyFontUploadObjects(); // This is no longer needed in newer versions of ImGui
	
	    SetStyle();
	
	    initialized = true;
	    EDITOR_INFO("ImGui initialized successfully");
	}
	*/

    /*
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
    */

    /*
    bool GUI::InitViewport(const Viewport &size, VkImageView imageView)
    {
        if (!initialized)
            return false;

        viewportInitialized = true;
        return true;
    }
    */

    /*
    void GUI::ViewportWindow(Viewport &size, bool &hovered, VkImageView imageView)
    {
        if (!initialized || !visible || !viewportInitialized)
            return;

        /// Start viewport window with dockable behavior
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        /// Get content region size
        ImVec2 viewportSize = ImGui::GetContentRegionAvail();
        size = {viewportSize.x, viewportSize.y};
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
    */

    ImTextureID GUI::GetTextureID(const VkImageView imageView, VkSampler sampler, const VkImageLayout layout) const
    {
        auto device = RenderContext::GetCurrentDevice();
        if (!initialized || imageView == VK_NULL_HANDLE)
            return reinterpret_cast<ImTextureID>(nullptr);

        /// Check if device is valid
        if (device == nullptr)
        {
            EDITOR_ERROR("Cannot get texture ID: device is null");
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
                EDITOR_ERROR("Cannot get texture ID: no valid sampler available");
                return reinterpret_cast<ImTextureID>(nullptr);
            }
        }

        /// Now we have a valid sampler and imageView
        /// ImGui_ImplVulkan_AddTexture returns VkDescriptorSet which needs to be cast to ImTextureID
        VkDescriptorSet descriptorSet = ImGui_ImplVulkan_AddTexture(actualSampler, imageView, layout);
        return reinterpret_cast<ImTextureID>(descriptorSet);
    }
	
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

} // namespace SceneryEditorX::UI

/// -------------------------------------------------------

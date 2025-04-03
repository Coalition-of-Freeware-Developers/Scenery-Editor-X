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

#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/imconfig.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <SceneryEditorX/core/window.h>
#include <SceneryEditorX/renderer/vk_core.h>
#include <SceneryEditorX/renderer/vk_device.h>
#include <SceneryEditorX/ui/ui.h>


// -------------------------------------------------------

// Implementation of missing ImGui functions to fix linker errors
// Note: This is a compatibility layer to ensure ImGui works correctly

// This implementation may need to be adjusted based on your ImGui version
// It provides stubs for functionality that might be missing in your current build

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
        float r = ((color >> 0) & 0xFF) / 255.0f;
        float g = ((color >> 8) & 0xFF) / 255.0f;
        float b = ((color >> 16) & 0xFF) / 255.0f;
        float a = ((color >> 24) & 0xFF) / 255.0f;
        return ImVec4(r, g, b, a);
    }
}

namespace SceneryEditorX
{

    // Additional ImGui initialization functions can be placed here if needed
    void initImGuiExtensions()
    {
        // This function can be called from main ImGui setup to initialize any extensions
        // Currently empty, but could be expanded if more ImGui features need integration
    }

    EditorUI::EditorUI()
    {
    }

    EditorUI::~EditorUI()
    {
    }

    void EditorUI::OnAttach()
    {
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        SetDarkThemeV2Colors();

        // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
        ImGuiStyle &style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.15f, 0.15f, style.Colors[ImGuiCol_WindowBg].w);

        ContextHandler *instance = this;
        Renderer::Submit([instance]() {
            VkDescriptorPool descriptorPool;

            // Create Descriptor Pool
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

            VkDescriptorPool imguiPool;
            if (vkCreateDescriptorPool(renderer->GetDevice(), &pool_info, nullptr, &imguiPool) != VK_SUCCESS)
            {
                EDITOR_LOG_ERROR("Failed to create ImGui descriptor pool!");
                return;
            }
        });
    }

	void EditorUI::OnDetach()
    {
        //VK_CHECK_RESULT(vkDeviceWaitIdle(device));
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

	void EditorUI::Begin()
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
    }

    void EditorUI::End()
    {
        ImGui::Render();

        VulkanSwapChain &swapChain = Application::Get().GetWindow()->GetSwapChain();

        VkClearValue clearValues[2];
        clearValues[0].color = {{0.1f, 0.1f, 0.1f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        uint32_t width = swapChain.GetWidth();
        uint32_t height = swapChain.GetHeight();

        uint32_t commandBufferIndex = swapChain.GetCurrentBufferIndex();

        VkCommandBufferBeginInfo drawCmdBufInfo = {};
        drawCmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        drawCmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        drawCmdBufInfo.pNext = nullptr;

        VkCommandBuffer drawCommandBuffer = swapChain.GetCurrentDrawCommandBuffer();

        //VK_CHECK_RESULT(vkBeginCommandBuffer(drawCommandBuffer, &drawCmdBufInfo));

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
        renderPassBeginInfo.framebuffer = swapChain.GetCurrentFramebuffer();

        vkCmdBeginRenderPass(drawCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

        VkCommandBufferInheritanceInfo inheritanceInfo = {};
        inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        inheritanceInfo.renderPass = swapChain.GetRenderPass();
        inheritanceInfo.framebuffer = swapChain.GetCurrentFramebuffer();

        VkCommandBufferBeginInfo cmdBufInfo = {};
        cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
        cmdBufInfo.pInheritanceInfo = &inheritanceInfo;

        VK_CHECK_RESULT(vkBeginCommandBuffer(ImGuiCmdBuff[commandBufferIndex], &cmdBufInfo));

        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = (float)height;
        viewport.height = -(float)height;
        viewport.width = (float)width;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(ImGuiCmdBuff[commandBufferIndex], 0, 1, &viewport);

        VkRect2D scissor = {};
        scissor.extent.width = width;
        scissor.extent.height = height;
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        vkCmdSetScissor(ImGuiCmdBuff[commandBufferIndex], 0, 1, &scissor);

        ImDrawData *main_draw_data = ImGui::GetDrawData();
        ImGui_ImplVulkan_RenderDrawData(main_draw_data, ImGuiCmdBuff[commandBufferIndex]);

        VK_CHECK_RESULT(vkEndCommandBuffer(ImGuiCmdBuff[commandBufferIndex]));

        std::vector<VkCommandBuffer> commandBuffers;
        commandBuffers.push_back(ImGuiCmdBuff[commandBufferIndex]);

        vkCmdExecuteCommands(drawCommandBuffer, uint32_t(commandBuffers.size()), commandBuffers.data());

        vkCmdEndRenderPass(drawCommandBuffer);

        VK_CHECK_RESULT(vkEndCommandBuffer(drawCommandBuffer));

        ImGuiIO &io = ImGui::GetIO();
        (void)io;
        // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }


    void EditorUI::newFrame()
    {
        if (!initialized || !renderer)
        {
            return;
        }

        // Start the ImGui frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow();

        // Add more UI elements as needed


        ImGui::Render();

        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), activeCommandBuffer);
    }

    void EditorUI::cleanUp()
    {
        if (renderer && initialized)
        {
            ImGui_ImplVulkan_Shutdown();
            ImGui_ImplGlfw_Shutdown();

            if (imguiPool != VK_NULL_HANDLE)
            {
                vkDestroyDescriptorPool(renderer->GetDevice(), imguiPool, nullptr);
                imguiPool = VK_NULL_HANDLE;
            }

            ImGui::DestroyContext();
            initialized = false;
        }
    }

    void EditorUI::initUI(GLFWwindow * window, SceneryEditorX::GraphicsEngine & engineRenderer)
    {
        // Store the engine reference
        this->renderer = &engineRenderer;

        // Create separate descriptor pool for ImGui with FREE_DESCRIPTOR_SET_BIT
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
        pool_info.maxSets = 100;
        pool_info.poolSizeCount = std::size(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;

        VkDescriptorPool imguiPool;
        if (vkCreateDescriptorPool(renderer->GetDevice(), &pool_info, nullptr, &imguiPool) != VK_SUCCESS)
        {
            EDITOR_LOG_ERROR("Failed to create ImGui descriptor pool!");
            return;
        }

        SceneryEditorX::QueueFamilyIndices indices = renderer->GetQueueFamilyIndices();

        ImGui::CreateContext();
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ImGui_ImplGlfw_InitForVulkan(Window::GetGLFWwindow(), true);

        ImGui_ImplVulkan_InitInfo info{};
        info.ApiVersion = VK_API_VERSION_1_3;
        info.Instance = renderer->GetInstance();
        info.PhysicalDevice = renderer->GetPhysicalDevice();
        info.Device = renderer->GetDevice();
        //info.QueueFamily = renderer->GetQueueFamilyIndices().graphicsFamily.value();
        info.Queue = renderer->GetGraphicsQueue();
        info.DescriptorPool = imguiPool;
        info.RenderPass = renderer->GetRenderPass();
        //info.Subpass = 0;
        info.MinImageCount = renderer->GetSwapChainImages().size();
        info.ImageCount = renderer->GetSwapChainImages().size();
        info.MSAASamples = VK_SAMPLE_COUNT_1_BIT; // Replace when MSAA is implemented properly
        //info.MSAASamples = renderer->msaaSamples;
        //info.Allocator = nullptr;
        info.UseDynamicRendering = false;
        info.CheckVkResultFn = [](VkResult result) {
            if (result != VK_SUCCESS)
            {
                EDITOR_LOG_ERROR("ImGui Vulkan Error: {}", ToString(result));
            }
        };

        ImGui_ImplVulkan_Init(&info);

        // Upload fonts to GPU
        VkCommandBuffer commandBuffer = renderer->beginSingleTimeCommands();
        ImGui_ImplVulkan_CreateFontsTexture(); // Removed Nov 10, 2023 Commit #79a9e2f
        renderer->endSingleTimeCommands(commandBuffer);

        vkDeviceWaitIdle(renderer->GetDevice()); // Wait for font upload to complete
        //ImGui_ImplVulkan_DestroyFontUploadObjects(); // This is no longer needed in newer versions of ImGui

        setStyle();

        initialized = true;
        EDITOR_LOG_INFO("ImGui initialized successfully");
    }

    void EditorUI::resize(const uint32_t width, const uint32_t height) const
    {
    }

    void EditorUI::update(const float delta_time)
    {
    }

    void EditorUI::show_demo_window()
    {
    }

    void EditorUI::show_app_info(const std::string &app_name)
    {
    }

    void EditorUI::setStyle()
    {
        constexpr auto ColorFromBytes = [](uint8_t r, uint8_t g, uint8_t b) {
            return ImVec4((float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, 1.0f);
        };

        auto &style = ImGui::GetStyle();
        ImVec4 *colors = style.Colors;

        const ImVec4 bgColor = ColorFromBytes(37, 37, 38);
        const ImVec4 lightBgColor = ColorFromBytes(82, 82, 85);
        const ImVec4 veryLightBgColor = ColorFromBytes(90, 90, 95);

        const ImVec4 panelColor = ColorFromBytes(51, 51, 55);
        const ImVec4 panelHoverColor = ColorFromBytes(29, 151, 236);
        const ImVec4 panelActiveColor = ColorFromBytes(0, 119, 200);

        const ImVec4 textColor = ColorFromBytes(255, 255, 255);
        const ImVec4 textDisabledColor = ColorFromBytes(151, 151, 151);
        const ImVec4 borderColor = ColorFromBytes(78, 78, 78);

        colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.15f, 0.15f, 0.65f);
        colors[ImGuiCol_Text] = textColor;
        colors[ImGuiCol_TextDisabled] = textDisabledColor;
        colors[ImGuiCol_TextSelectedBg] = panelActiveColor;
        colors[ImGuiCol_ChildBg] = bgColor;
        colors[ImGuiCol_PopupBg] = bgColor;
        colors[ImGuiCol_Border] = borderColor;
        colors[ImGuiCol_BorderShadow] = borderColor;
        colors[ImGuiCol_FrameBg] = panelColor;
        colors[ImGuiCol_FrameBgHovered] = panelHoverColor;
        colors[ImGuiCol_FrameBgActive] = panelActiveColor;
        colors[ImGuiCol_TitleBg] = bgColor;
        colors[ImGuiCol_TitleBgActive] = bgColor;
        colors[ImGuiCol_TitleBgCollapsed] = bgColor;
        colors[ImGuiCol_MenuBarBg] = panelColor;
        colors[ImGuiCol_ScrollbarBg] = panelColor;
        colors[ImGuiCol_ScrollbarGrab] = lightBgColor;
        colors[ImGuiCol_ScrollbarGrabHovered] = veryLightBgColor;
        colors[ImGuiCol_ScrollbarGrabActive] = veryLightBgColor;
        colors[ImGuiCol_CheckMark] = panelActiveColor;
        colors[ImGuiCol_SliderGrab] = panelHoverColor;
        colors[ImGuiCol_SliderGrabActive] = panelActiveColor;
        colors[ImGuiCol_Button] = panelColor;
        colors[ImGuiCol_ButtonHovered] = panelHoverColor;
        colors[ImGuiCol_ButtonActive] = panelHoverColor;
        colors[ImGuiCol_Header] = panelColor;
        colors[ImGuiCol_HeaderHovered] = panelHoverColor;
        colors[ImGuiCol_HeaderActive] = panelActiveColor;
        colors[ImGuiCol_Separator] = borderColor;
        colors[ImGuiCol_SeparatorHovered] = borderColor;
        colors[ImGuiCol_SeparatorActive] = borderColor;
        colors[ImGuiCol_ResizeGrip] = bgColor;
        colors[ImGuiCol_ResizeGripHovered] = panelColor;
        colors[ImGuiCol_ResizeGripActive] = lightBgColor;
        colors[ImGuiCol_PlotLines] = panelActiveColor;
        colors[ImGuiCol_PlotLinesHovered] = panelHoverColor;
        colors[ImGuiCol_PlotHistogram] = panelActiveColor;
        colors[ImGuiCol_PlotHistogramHovered] = panelHoverColor;
        colors[ImGuiCol_DragDropTarget] = bgColor;
        colors[ImGuiCol_NavHighlight] = bgColor;
        colors[ImGuiCol_DockingPreview] = panelActiveColor;
        colors[ImGuiCol_Tab] = bgColor;
        colors[ImGuiCol_TabActive] = panelActiveColor;
        colors[ImGuiCol_TabUnfocused] = bgColor;
        colors[ImGuiCol_TabUnfocusedActive] = panelActiveColor;
        colors[ImGuiCol_TabHovered] = panelHoverColor;

        style.WindowRounding = 0.0f;
        style.ChildRounding = 0.0f;
        style.FrameRounding = 0.0f;
        style.GrabRounding = 0.0f;
        style.PopupRounding = 0.0f;
        style.ScrollbarRounding = 0.0f;
        style.TabRounding = 0.0f;
    }

    void EditorUI::setFonts()
    {
        // Originally Defined in Editor.cpp,
        // TODO: Create a better font handling and customization system here.

        //const float fontSize = 17.0f;
        //io.FontDefault = io.Fonts->AddFontFromFileTTF((config.fontFolder + "/roboto.ttf").c_str(), fontSize);
        //ImFontConfig imgFontConfig;
        //const float iconSize = fontSize * 2.8f / 3.0f;
        //imgFontConfig.MergeMode = true;
        //imgFontConfig.GlyphMinAdvanceX = iconSize;

        // Load Fonts
        // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
        // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
        // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
        // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
        // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
        // - Read 'docs/FONTS.md' for more instructions and details.
        // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
        //io.Fonts->AddFontDefault();
        //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
        //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
        //IM_ASSERT(font != nullptr);
    }


} // namespace SceneryEditorX

// -------------------------------------------------------

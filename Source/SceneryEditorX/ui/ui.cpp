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
#include <imgui/imgui.h>
#include <SceneryEditorX/core/window.h>
#include <SceneryEditorX/renderer/vk_core.h>
#include <SceneryEditorX/ui/ui.h>
#include <SceneryEditorX/ui/ui_manager.h>

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
	
	GUI::GUI()
	{
	}
	
	GUI::~GUI()
	{
	}
	
	void GUI::newFrame()
	{
	    if (!initialized || !renderer)
	    {
	        return;
	    }
	
	    // Start the ImGui frame
	    ImGui_ImplVulkan_NewFrame();
	    ImGui_ImplGlfw_NewFrame();
	    ImGui::NewFrame();
	
		{
	        if (Dockspace_)
	        {
	            static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
	            ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
	            if (MenubarCallback_)
	                window_flags |= ImGuiWindowFlags_MenuBar;
	            const ImGuiViewport *viewport = ImGui::GetMainViewport();
	            ImGui::SetNextWindowPos(viewport->WorkPos);
	            ImGui::SetNextWindowSize(viewport->WorkSize);
	            ImGui::SetNextWindowViewport(viewport->ID);
	            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
	                            ImGuiWindowFlags_NoMove;
	            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	
	            // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
	            // and handle the pass-thru hole, so we ask Begin() to not render a background.
	            if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
	                window_flags |= ImGuiWindowFlags_NoBackground;
	
	            // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
	            // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
	            // all active windows docked into it will lose their parent and become undocked.
	            // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
	            // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
	            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	            ImGui::Begin("DockSpace Demo", nullptr, window_flags);
	            ImGui::PopStyleVar();
	
	            ImGui::PopStyleVar(2);
	
	            // Submit the DockSpace
	            ImGuiIO &io = ImGui::GetIO();
	            if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
	            {
	                ImGuiID dockspace_id = ImGui::GetID("VulkanAppDockspace");
	                ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
	            }
	
	            if (MenubarCallback_)
	            {
	                if (ImGui::BeginMenuBar())
	                {
	                    MenubarCallback_();
	                    ImGui::EndMenuBar();
	                }
	            }
	        }
	        if (Dockspace_)
	            ImGui::End();
		}
	
	    // Your ImGui UI code goes here
	    ImGui::ShowDemoWindow();
	
	    // Add more UI elements as needed
	
	    UI::MainMenuBar();
	    UI::EditorViewport();
	    UI::LayerStack();
	    // Complete the ImGui frame
	    ImGui::Render();
	
	    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), activeCommandBuffer);
	}
	
	void GUI::cleanUp()
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
	
	void GUI::initGUI(GLFWwindow *window, SceneryEditorX::GraphicsEngine &engineRenderer)
	{
	    // Store the engine reference
	    this->renderer = &engineRenderer;
	
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
	    info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;  // Replace when MSAA is implemented properly
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
	
		vkDeviceWaitIdle(renderer->GetDevice());// Wait for font upload to complete
	    //ImGui_ImplVulkan_DestroyFontUploadObjects(); // This is no longer needed in newer versions of ImGui
	
		setStyle();
	
		initialized = true;
	    EDITOR_LOG_INFO("ImGui initialized successfully");
	}
	
	void GUI::resize(const uint32_t width, const uint32_t height) const
	{
	}
	
	void GUI::update(const float delta_time)
	{
	}
	
	void GUI::show_demo_window()
	{
	
	}
	
	void GUI::show_app_info(const std::string &app_name)
	{
	}
	
	void GUI::setStyle()
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
	
	void GUI::setFonts()
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

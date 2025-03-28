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
#include <SceneryEditorX/renderer/vk_device.h>
#include <SceneryEditorX/core/window.h>
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
        return ImGui::BeginTable(str_id, columns, flags, outer_size, inner_width);
    }

    // Additional functions may need to be implemented based on other linker errors
}

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

void GUI::new_frame()
{
    ImGui::NewFrame();
}

void GUI::init(GLFWwindow *window, SceneryEditorX::GraphicsEngine &engineRenderer)
{
    this->renderer = &engineRenderer;

    ImGui_ImplGlfw_InitForVulkan(Window::GetGLFWwindow(), true);
    ImGui_ImplVulkan_InitInfo info{};
    info.ApiVersion = VK_API_VERSION_1_3;
    info.Instance = renderer->GetInstance();
    info.PhysicalDevice = renderer->GetPhysicalDevice();
    info.Device = renderer->GetDevice();
    info.QueueFamily = renderer->GetQueueFamilyIndices().graphicsFamily.value();
    info.Queue = renderer->GetGraphicsQueue();
    //info.PipelineCache        = g_PipelineCache;
    info.DescriptorPool = renderer->GetDescriptorPool();
    info.RenderPass = renderer->GetRenderPass();
    info.Subpass = 0;
    info.MinImageCount = renderer->GetSwapChainImages().size();
    info.ImageCount = renderer->GetSwapChainImages().size();
    info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    info.Allocator = nullptr;
    info.UseDynamicRendering = false;
    info.CheckVkResultFn = [](VkResult result) {
        if (result != VK_SUCCESS)
        {
            EDITOR_LOG_ERROR("ImGui Vulkan Error: {}", ToString(result));
        }
    };

    ImGui_ImplVulkan_Init(&info);
}

void GUI::resize(const uint32_t width, const uint32_t height) const
{
}

void GUI::update(const float delta_time)
{
}

bool GUI::update_buffers()
{
    return false;
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

// -------------------------------------------------------

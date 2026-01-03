/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* ui_layer.h
* -------------------------------------------------------
* Created: 24/12/2025
* -------------------------------------------------------
*/
#include "ui_layer.h"
#include "colors.h"
#include "ui_fonts.h"
#include "SceneryEditorX/core/input/input.h"
#include "SceneryEditorX/core/input/key_codes.h"
#include "SceneryEditorX/renderer/command_buffer.h"
#include "SceneryEditorX/renderer/renderer.h"
#include "SceneryEditorX/renderer/swapchain.h"
#include "SceneryEditorX/renderer/vulkan_utils.h"
#include "SceneryEditorX/renderer/fonts/font_awesome.h"

#include <ImGuizmo.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

// -------------------------------------------------------

namespace SceneryEditorX::UI
{
    static std::vector<VkCommandBuffer> s_ImGuiCommandBuffers;

	UILayer::UILayer()
	{

	} 

    UILayer::~UILayer()
    {
    }

    void UILayer::OnAttach()
    {
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;	// Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;		// Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;		// Enable Multi-Viewport / Platform Windows
        io.ConfigWindowsMoveFromTitleBarOnly = true;

        // Configure Fonts
        {
            const float LargeFontSize = 36.0f;

            UI::FontConfiguration robotoBold;
            robotoBold.FontName = "Bold";
            robotoBold.FilePath = "Resources/Fonts/Roboto/Roboto-Bold.ttf";
            robotoBold.Size = 18.0f;
            UI::Fonts::Add(robotoBold);

            UI::FontConfiguration robotoLarge;
            robotoLarge.FontName = "Large";
            robotoLarge.FilePath = "Resources/Fonts/Roboto/Roboto-Regular.ttf";
            robotoLarge.Size = LargeFontSize;
            UI::Fonts::Add(robotoLarge);

            UI::FontConfiguration robotoDefault;
            robotoDefault.FontName = "Default";
            robotoDefault.FilePath = "Resources/Fonts/Roboto/Roboto-SemiMedium.ttf";
            robotoDefault.Size = 15.0f;
            UI::Fonts::Add(robotoDefault, true);

            static const ImWchar s_FontAwesomeRanges[] = {SEDX_ICON_MIN, SEDX_ICON_MAX, 0};
            UI::FontConfiguration fontAwesome;
            fontAwesome.FontName = "FontAwesome";
            fontAwesome.FilePath = "Resources/Fonts/FontAwesome/fontawesome-webfont.ttf";
            fontAwesome.Size = 16.0f;
            fontAwesome.GlyphRanges = s_FontAwesomeRanges;
            fontAwesome.MergeWithLast = true;
            UI::Fonts::Add(fontAwesome);

            UI::FontConfiguration robotoMedium;
            robotoMedium.FontName = "Medium";
            robotoMedium.FilePath = "Resources/Fonts/Roboto/Roboto-SemiMedium.ttf";
            robotoMedium.Size = 18.0f;
            UI::Fonts::Add(robotoMedium);

            UI::FontConfiguration robotoSmall;
            robotoSmall.FontName = "Small";
            robotoSmall.FilePath = "Resources/Fonts/Roboto/Roboto-SemiMedium.ttf";
            robotoSmall.Size = 12.0f;
            UI::Fonts::Add(robotoSmall);

            UI::FontConfiguration robotoExtraSmall;
            robotoExtraSmall.FontName = "ExtraSmall";
            robotoExtraSmall.FilePath = "Resources/Fonts/Roboto/Roboto-SemiMedium.ttf";
            robotoExtraSmall.Size = 10.0f;
            UI::Fonts::Add(robotoExtraSmall);

            UI::FontConfiguration robotoBoldTitle;
            robotoBoldTitle.FontName = "BoldTitle";
            robotoBoldTitle.FilePath = "Resources/Fonts/Roboto/Roboto-Bold.ttf";
            robotoBoldTitle.Size = 16.0f;
            UI::Fonts::Add(robotoBoldTitle);

            UI::FontConfiguration robotoBoldLarge;
            robotoBold.FontName = "BoldLarge";
            robotoBold.FilePath = "Resources/Fonts/Roboto/Roboto-Bold.ttf";
            robotoBold.Size = LargeFontSize;
            UI::Fonts::Add(robotoBold);
        }

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

        UILayer *instance = this;
        Renderer::Submit([instance]()
        {
            Application &app = Application::Get();
            GLFWwindow *window = static_cast<GLFWwindow *>(app.GetWindow().GetWindow());

            auto vulkanContext = RenderContext::Get();
            auto device = RenderContext::GetCurrentDevice()->GetDevice();

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
            VK_CHECK_RESULT(vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptorPool));

            // Setup Platform/Renderer bindings
            ImGui_ImplGlfw_InitForVulkan(window, true);
            ImGui_ImplVulkan_InitInfo init_info = {};
            init_info.Instance = RenderContext::GetInstance();
            init_info.PhysicalDevice = RenderContext::GetCurrentDevice()->GetPhysicalDevice()->GetDevice();
            init_info.Device = device;
            init_info.QueueFamily = RenderContext::GetCurrentDevice()->GetPhysicalDevice()->GetQueueFamilyIndices().graphics;
            init_info.Queue = RenderContext::GetCurrentDevice()->GetGraphicsQueue();
            init_info.PipelineCache = nullptr;
            init_info.DescriptorPool = descriptorPool;
            init_info.Allocator = nullptr;
            init_info.MinImageCount = 2;
            SwapChain *swapChain = Renderer::GetSwapChain();
            init_info.ImageCount = swapChain->GetSwapChainImageCount();
            init_info.CheckVkResultFn = VulkanCheckResult;
            ImGui_ImplVulkan_Init(&init_info);

            /*
            // Upload Fonts
            {
                // Use any command queue

                VkCommandBuffer commandBuffer = vulkanContext->GetCurrentDevice()->GetCommandBuffer(true);
                ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
                vulkanContext->GetCurrentDevice()->FlushCommandBuffer(commandBuffer);

                VK_CHECK_RESULT(vkDeviceWaitIdle(device));
                ImGui_ImplVulkan_DestroyFontUploadObjects();
            }
            */

            uint32_t framesInFlight = Renderer::GetCurrentFrameIndex();
            s_ImGuiCommandBuffers.resize(framesInFlight);
            for (uint32_t i = 0; i < framesInFlight; i++)
            {
                s_ImGuiCommandBuffers[i] = RenderContext::GetCurrentDevice()->CreateSecondaryCommandBuffer("UI Secondary CommandBuffer");
            }
        });
    }

    void UILayer::OnDetach()
    {
        Renderer::Submit([]()
        {
            auto device = RenderContext::GetCurrentDevice()->GetDevice();

            VK_CHECK_RESULT(vkDeviceWaitIdle(device));
            ImGui_ImplVulkan_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();
        });
    }

    void UILayer::OnUIRender()
    {
    }

    void UILayer::Begin()
	{
        ImGui::SetMouseCursor(Input::GetCursorMode() == CursorMode::Normal ? ImGui::GetMouseCursor() : ImGuiMouseCursor_None);

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
	}

	void UILayer::End()
	{
        ImGui::Render();

        SwapChain *swapChain = Renderer::GetSwapChain();

        VkClearValue clearValues[2];
        clearValues[0].color = {{0.1f, 0.1f, 0.1f, 1.0f}};
        clearValues[1].depthStencil = {.depth = 1.0f, .stencil = 0};

        uint32_t width = swapChain->GetWidth();
        uint32_t height = swapChain->GetHeight();

        uint32_t commandBufferIndex = swapChain->GetCurrentBufferIndex();

        VkCommandBufferBeginInfo drawCmdBufInfo = {};
        drawCmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        drawCmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        drawCmdBufInfo.pNext = nullptr;

        VkCommandBuffer drawCommandBuffer = swapChain->GetActiveDrawCommandBuffer();
        VK_CHECK_RESULT(vkBeginCommandBuffer(drawCommandBuffer, &drawCmdBufInfo));

        VkRenderPassBeginInfo renderPassBeginInfo = {};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.pNext = nullptr;
        renderPassBeginInfo.renderPass = swapChain->GetRenderPass();
        renderPassBeginInfo.renderArea.offset.x = 0;
        renderPassBeginInfo.renderArea.offset.y = 0;
        renderPassBeginInfo.renderArea.extent.width = width;
        renderPassBeginInfo.renderArea.extent.height = height;
        renderPassBeginInfo.clearValueCount = 2; // Color + depth
        renderPassBeginInfo.pClearValues = clearValues;
        renderPassBeginInfo.framebuffer = swapChain->GetActiveFramebuffer();

        vkCmdBeginRenderPass(drawCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

        VkCommandBufferInheritanceInfo inheritanceInfo = {};
        inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        inheritanceInfo.renderPass = swapChain->GetRenderPass();
        inheritanceInfo.framebuffer = swapChain->GetActiveFramebuffer();

        VkCommandBufferBeginInfo cmdBufInfo = {};
        cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
        cmdBufInfo.pInheritanceInfo = &inheritanceInfo;

        VK_CHECK_RESULT(vkBeginCommandBuffer(s_ImGuiCommandBuffers[commandBufferIndex], &cmdBufInfo));

        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = (float)height;
        viewport.height = -(float)height;
        viewport.width = (float)width;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(s_ImGuiCommandBuffers[commandBufferIndex], 0, 1, &viewport);

        VkRect2D scissor = {};
        scissor.extent.width = width;
        scissor.extent.height = height;
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        vkCmdSetScissor(s_ImGuiCommandBuffers[commandBufferIndex], 0, 1, &scissor);

        ImDrawData *main_draw_data = ImGui::GetDrawData();
        ImGui_ImplVulkan_RenderDrawData(main_draw_data, s_ImGuiCommandBuffers[commandBufferIndex]);

        VK_CHECK_RESULT(vkEndCommandBuffer(s_ImGuiCommandBuffers[commandBufferIndex]));

        std::vector<VkCommandBuffer> commandBuffers;
        commandBuffers.push_back(s_ImGuiCommandBuffers[commandBufferIndex]);

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

	void UILayer::SetDarkThemeColors()
	{
        auto &colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_WindowBg] = ImVec4{0.1f, 0.105f, 0.11f, 1.0f};

        // Headers
        colors[ImGuiCol_Header] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
        colors[ImGuiCol_HeaderHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
        colors[ImGuiCol_HeaderActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

        // Buttons
        colors[ImGuiCol_Button] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
        colors[ImGuiCol_ButtonHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
        colors[ImGuiCol_ButtonActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

        // Frame BG
        colors[ImGuiCol_FrameBg] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
        colors[ImGuiCol_FrameBgHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
        colors[ImGuiCol_FrameBgActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

        // Tabs
        colors[ImGuiCol_Tab] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
        colors[ImGuiCol_TabHovered] = ImVec4{0.38f, 0.3805f, 0.381f, 1.0f};
        colors[ImGuiCol_TabActive] = ImVec4{0.28f, 0.2805f, 0.281f, 1.0f};
        colors[ImGuiCol_TabUnfocused] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};

        // Title
        colors[ImGuiCol_TitleBg] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
        colors[ImGuiCol_TitleBgActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

        // Resize Grip
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.91f, 0.91f, 0.91f, 0.25f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.81f, 0.81f, 0.81f, 0.67f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.46f, 0.46f, 0.46f, 0.95f);

        // Scrollbar
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.0f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.0f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.0f);

        // Check Mark
        colors[ImGuiCol_CheckMark] = ImVec4(0.94f, 0.94f, 0.94f, 1.0f);

        // Slider
        colors[ImGuiCol_SliderGrab] = ImVec4(0.51f, 0.51f, 0.51f, 0.7f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.66f, 0.66f, 0.66f, 1.0f);
	}

	void UILayer::SetDarkThemeV2Colors()
	{
        auto &style = ImGui::GetStyle();
        auto &colors = ImGui::GetStyle().Colors;

        //========================================================
        /// Colours

        // Headers
        colors[ImGuiCol_Header] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::groupHeader);
        colors[ImGuiCol_HeaderHovered] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::groupHeader);
        colors[ImGuiCol_HeaderActive] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::groupHeader);

        // Buttons
        colors[ImGuiCol_Button] = ImColor(56, 56, 56, 200);
        colors[ImGuiCol_ButtonHovered] = ImColor(70, 70, 70, 255);
        colors[ImGuiCol_ButtonActive] = ImColor(56, 56, 56, 150);

        // Frame BG
        colors[ImGuiCol_FrameBg] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::propertyField);
        colors[ImGuiCol_FrameBgHovered] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::propertyField);
        colors[ImGuiCol_FrameBgActive] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::propertyField);

        // Tabs
        colors[ImGuiCol_Tab] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::titlebar);
        colors[ImGuiCol_TabHovered] = ImColor(255, 225, 135, 30);
        colors[ImGuiCol_TabActive] = ImColor(255, 225, 135, 60);
        colors[ImGuiCol_TabUnfocused] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::titlebar);
        colors[ImGuiCol_TabUnfocusedActive] = colors[ImGuiCol_TabHovered];

        // Title
        colors[ImGuiCol_TitleBg] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::titlebar);
        colors[ImGuiCol_TitleBgActive] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::titlebar);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

        // Resize Grip
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.91f, 0.91f, 0.91f, 0.25f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.81f, 0.81f, 0.81f, 0.67f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.46f, 0.46f, 0.46f, 0.95f);

        // Scrollbar
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.0f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.0f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.0f);

        // Check Mark
        colors[ImGuiCol_CheckMark] = ImColor(200, 200, 200, 255);

        // Slider
        colors[ImGuiCol_SliderGrab] = ImVec4(0.51f, 0.51f, 0.51f, 0.7f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.66f, 0.66f, 0.66f, 1.0f);

        // Text
        colors[ImGuiCol_Text] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::text);

        // Checkbox
        colors[ImGuiCol_CheckMark] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::text);

        // Separator
        colors[ImGuiCol_Separator] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::backgroundDark);
        colors[ImGuiCol_SeparatorActive] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::highlight);
        colors[ImGuiCol_SeparatorHovered] = ImColor(39, 185, 242, 150);

        // Window Background
        colors[ImGuiCol_WindowBg] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::titlebar);
        colors[ImGuiCol_ChildBg] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::background);
        colors[ImGuiCol_PopupBg] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::backgroundPopup);
        colors[ImGuiCol_Border] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::backgroundDark);

        // Tables
        colors[ImGuiCol_TableHeaderBg] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::groupHeader);
        colors[ImGuiCol_TableBorderLight] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::backgroundDark);

        // Menubar
        colors[ImGuiCol_MenuBarBg] = ImVec4{0.0f, 0.0f, 0.0f, 0.0f};

        //========================================================
        /// Style
        style.FrameRounding = 2.5f;
        style.FrameBorderSize = 1.0f;
        style.IndentSpacing = 11.0f;
	}

	void UILayer::AllowInputEvents(bool allowEvents)
	{
        // TODO: Implement this function properly
        // g_DisableImGuiEvents = !allowEvents;
	}
	
} // namespace SceneryEditorX::UI

// -------------------------------------------------------

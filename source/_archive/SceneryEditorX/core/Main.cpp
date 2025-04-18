#include <core/Application.hpp>
#include <core/AssetManager.hpp>
#include <core/Window.hpp>
#include <logging/Logging.hpp>
#include <platform/windows/FileManager.hpp>
#include <renderer/DeferredRenderer.hpp>
#include <renderer/VK_Wrapper.h>
#include <scene/Scene.hpp>

#include <stb_image.h>

#include <GLFW/glfw3.h>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <ImGuizmo.h>
#include <misc/cpp/imgui_stdlib.h>

#include <iostream>
#include <string>
#include <sysinfoapi.h>

#ifdef _DEBUG
#define IMGUI_VULKAN_DEBUG_REPORT
#endif

#ifndef EDITOR
#define EDITOR
#endif // !EDITOR

class Application
{
public:
    void run()
    {
        Setup();
        Create();
        MainLoop();
        Finish();
    }

private:
    u32 frameCount = 0;
    ImDrawData *imguiDrawData = nullptr;
    bool drawUi = true;
    bool viewportResized = false;
    glm::ivec2 viewportSize = {64, 48};

    void WaitToInit(float seconds)
    {
        auto t0 = std::chrono::high_resolution_clock::now();
        auto t1 = std::chrono::high_resolution_clock::now();
        while (std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() < seconds * 1000.0f)
        {
            //SEDX_PROFILE_FRAME();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            t1 = std::chrono::high_resolution_clock::now();
        }
    }

    void Setup()
    {
        //SEDX_PROFILE_FUNC();
        AssetManager::Setup();
        SetupImgui();
        Scene::Setup();
    }

    void Create()
    {
        //SEDX_PROFILE_FUNC();
        CreateVulkan();
    }

    void CreateVulkan()
    {
        //SEDX_PROFILE_FUNC();
        Window::Create();
        vkw::Init(Window::GetGLFWwindow(), Window::GetWidth(), Window::GetHeight());
        DEBUG_TRACE("Finish creating SwapChain.");
        CreateImgui();
        DeferredShading::CreateImages(Window::GetWidth(), Window::GetHeight());
        DeferredShading::CreateShaders();
        AssetManager::Create();
        Scene::CreateResources();
        createUniformProjection();
    }

    void Finish()
    {
        //SEDX_PROFILE_FUNC();
        DestroyVulkan();
        AssetManager::Finish();
        FinishImgui();
    }

    void DestroyVulkan()
    {
        //SEDX_PROFILE_FUNC();
        Scene::DestroyResources();
        DeferredShading::Destroy();
        AssetManager::Destroy();
        DestroyImgui();
        vkw::Destroy();
        Window::Destroy();
    }

    void MainLoop()
    {
        while (!Window::GetShouldClose())
        {
            //SEDX_PROFILE_FRAME();
            Window::Update();
            AssetManager::UpdateResources();
            Transform *selectedTransform = nullptr;
            if (Scene::selectedEntity != nullptr)
            {
                selectedTransform = &Scene::selectedEntity->transform;
            }
            Scene::camera.Update(selectedTransform);
            drawFrame();
            if (Window::IsKeyPressed(GLFW_KEY_F1))
            {
                drawUi = !drawUi;
            }
            if (Window::IsKeyPressed(GLFW_KEY_R))
            {
                vkw::WaitIdle();
                DeferredShading::CreateShaders();
            }
            else if (DirtyFrameResources())
            {
                RecreateFrameResources();
            }
            else if (Window::IsDirty())
            {
                Window::ApplyChanges();
            }
        }
        vkw::WaitIdle();
    }

    bool DirtyFrameResources()
    {
        bool dirty = false;
        dirty |= viewportResized;
        dirty |= vkw::GetSwapChainDirty();
        dirty |= Window::GetFramebufferResized();
        return dirty;
    }

    ImVec2 ToScreenSpace(glm::vec3 position)
    {
        glm::vec4 cameraSpace = Scene::camera.GetProj() * Scene::camera.GetView() * glm::vec4(position, 1.0f);
        ImVec2 screenSpace = ImVec2(cameraSpace.x / cameraSpace.w, cameraSpace.y / cameraSpace.w);
        glm::ivec2 ext = viewportSize;
        screenSpace.x = (screenSpace.x + 1.0) * ext.x / 2.0;
        screenSpace.y = (screenSpace.y + 1.0) * ext.y / 2.0;
        return screenSpace;
    }

    /**
     * @brief Draws the ImGui frame and handles ImGui windows and widgets.
     * 
     * This function sets up a new ImGui frame, creates various ImGui windows
     * such as the demo window, viewport, configuration, assets, inspector, profiler,
     * and scene windows. It also handles resizing of the viewport and updates the
     * ImGui draw data.
     */
    void imguiDrawFrame()
    {
        //SEDX_PROFILE_FUNC();
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        //ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);

        ImGui::ShowDemoWindow();

        if (ImGui::Begin("Viewport", 0))
        {
            ImGui::BeginChild("##ChildViewport");
            glm::ivec2 newViewportSize = {ImGui::GetWindowSize().x, ImGui::GetWindowSize().y};
            if (newViewportSize != viewportSize)
            {
                viewportResized = true;
                viewportSize = newViewportSize;
            }
            DeferredShading::ViewportOnImGui();
            ImGui::EndChild();
        }
        ImGui::End();

        if (ImGui::Begin("Scenery Editor X Engine"))
        {
            if (ImGui::BeginTabBar("SceneryEditorX | MainTab"))
            {
                if (ImGui::BeginTabItem("Configuration"))
                {
                    Window::OnImgui();
                    Scene::camera.OnImgui();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Assets"))
                {
                    AssetManager::OnImgui();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }
        ImGui::End();

        if (ImGui::Begin("Inspector"))
        {
            if (Scene::selectedEntity != nullptr)
            {
                Scene::InspectEntity(Scene::selectedEntity);
            }
        }
        ImGui::End();

        if (ImGui::Begin("Profiler"))
        {
            std::map<std::string, float> timeTable;
            vkw::GetTimeStamps(timeTable);
            for (const auto &pair : timeTable)
            {
                ImGui::Text("%s: %.3f", pair.first.c_str(), pair.second);
            }
        }
        ImGui::End();

        DeferredShading::OnImgui(0);

        bool sceneOpen = true;
        if (ImGui::Begin("Scene"))
        {
            Scene::OnImgui();
        }
        ImGui::End();

        ImGui::Render();
        imguiDrawData = ImGui::GetDrawData();
    }

    /**
     * @brief Updates the command buffer with the necessary commands for rendering.
     * 
     * This function begins a new command buffer, records commands to copy scene and model data,
     * builds the top-level acceleration structure (TLAS) for ray tracing, and records commands
     * for various rendering passes including opaque pass, light pass, and compose pass. It also
     * handles ImGui rendering if the UI is enabled.
     */
    void updateCommandBuffer()
    {
        //SEDX_PROFILE_FUNC();
        vkw::BeginCommandBuffer(vkw::Queue::Graphics);
        auto totalTS = vkw::CmdBeginTimeStamp("GPU::Total");

        vkw::CmdCopy(Scene::sceneBuffer, &Scene::scene, sizeof(Scene::scene));
        vkw::CmdCopy(Scene::modelsBuffer, &Scene::models, sizeof(Scene::models));
        vkw::CmdTimeStamp("GPU::BuildTLAS", [&] {
            std::vector<vkw::BLASInstance> vkwInstances(Scene::modelEntities.size());
            for (int i = 0; i < Scene::modelEntities.size(); i++)
            {
                MeshResource &mesh = AssetManager::meshes[Scene::modelEntities[i]->mesh];
                vkwInstances[i] = {
                    .blas = mesh.blas,
                    .modelMat = Scene::modelEntities[i]->transform.GetMatrix(),
                    .customIndex = Scene::modelEntities[i]->id,
                };
            }
            vkw::CmdBuildTLAS(Scene::tlas, vkwInstances);
        });

        vkw::CmdBarrier(); // TLAS build is a write operation

        auto opaqueTS = vkw::CmdBeginTimeStamp("GPU::OpaquePass"); // Opaque pass
        DeferredShading::BeginOpaquePass();

        DeferredShading::OpaqueConstants constants;
        constants.sceneBufferIndex = Scene::sceneBuffer.RID();
        constants.modelBufferIndex = Scene::modelsBuffer.RID();

        for (Model *model : Scene::modelEntities)
        {
            constants.modelID = model->id;
            vkw::CmdPushConstants(&constants, sizeof(constants));
            DeferredShading::RenderMesh(model->mesh);
        }

        if (Scene::renderLightGizmos)
        {
            for (Light *light : Scene::lightEntities)
            {
                constants.modelID = light->id;
                vkw::CmdPushConstants(&constants, sizeof(constants));
                DeferredShading::RenderMesh(Scene::lightMeshes[light->block.type]);
            }
        }

        DeferredShading::EndPass();
        vkw::CmdEndTimeStamp(opaqueTS);

        auto lightTS = vkw::CmdBeginTimeStamp("LightPass");
        DeferredShading::LightConstants lightPassConstants;
        lightPassConstants.sceneBufferIndex = constants.sceneBufferIndex;
        lightPassConstants.frameID = frameCount;
        DeferredShading::LightPass(lightPassConstants);
        vkw::CmdEndTimeStamp(lightTS);

        auto composeTS = vkw::CmdBeginTimeStamp("GPU::ComposePass");
        DeferredShading::ComposePass();
        vkw::CmdEndTimeStamp(composeTS);

        DeferredShading::BeginPresentPass();
        auto imguiTS = vkw::CmdBeginTimeStamp("GPU::ImGui");
        if (drawUi)
        {
            vkw::CmdDrawImGui(imguiDrawData);
        }
        vkw::CmdEndTimeStamp(imguiTS);
        DeferredShading::EndPresentPass();
        vkw::CmdEndTimeStamp(totalTS);
    }

    /**
     * @brief Draws a single frame.
     * 
     * This function handles the drawing of a single frame. It first draws the ImGui frame,
     * then acquires the next image from the swap chain. If the swap chain is dirty, it returns early.
     * It then updates the uniform buffer and command buffer, and finally submits the command buffer
     * and presents the image. The frame count is incremented at the end.
     */
    void DrawFrame()
    {
        //SEDX_PROFILE_FUNC();
        DrawEditor();
        RenderFrame();
        if (vkw::GetSwapChainDirty())
        {
            return;
        }
        vkw::SubmitAndPresent();
        frameCount = (frameCount + 1) % (1 << 15);
    }

    /**
     * @brief Recreates frame resources when the viewport size changes or the framebuffer is resized.
     * 
     * This function handles the recreation of frame resources, ensuring that the application
     * correctly handles changes in the viewport size or framebuffer size. It waits for the window
     * to be restored from a minimized state, updates the framebuffer size, and recreates necessary
     * Vulkan resources.
     */
    void RecreateFrameResources()
    {
        //SEDX_PROFILE_FUNC();
        // busy wait while the window is minimized
        while (Window::GetWidth() == 0 || Window::GetHeight() == 0)
        {
            Window::WaitEvents();
        }
        if (viewportSize.x == 0 || viewportSize.y == 0)
        {
            return;
        }
        vkw::WaitIdle();
        if (Window::GetFramebufferResized())
        {
            Window::UpdateFramebufferSize();
            vkw::OnSurfaceUpdate(Window::GetWidth(), Window::GetHeight());
        }
        DeferredShading::CreateImages(viewportSize.x, viewportSize.y);
        createUniformProjection();
        viewportResized = false;
    }

    /**
     * @brief Sets the extent of the camera's viewport.
     * 
     * This function updates the camera's viewport size to match the current
     * viewport dimensions. It accounts for the fact that glm was designed for
     * OpenGL, where the Y coordinate of the clip coordinates is inverted.
     * 
     * @note The easiest way to fix this is by flipping the scaling factor of the Y axis.
     */
    void createUniformProjection()
    {
        // glm was designed for OpenGL, where the Y coordinate of the clip coordinates is inverted
        // the easiest way to fix this is fliping the scaling factor of the y axis
        Scene::camera.SetExtent(viewportSize.x, viewportSize.y);
    }

    /**
     * @brief Updates the uniform buffer with the latest scene resources.
     * 
     * This function profiles the update process and calls the Scene::UpdateResources
     * method to ensure that the uniform buffer contains the most recent data for rendering.
     */
    void updateUniformBuffer()
    {
        //SEDX_PROFILE_FUNC();
        Scene::UpdateResources();
    }

    /**
     * @brief Sets up the ImGui context and configures the ImGui style.
     * 
     * This function initializes the ImGui context, enables docking, and sets up
     * the ImGui style with custom colors and settings. It defines a helper function
     * to convert color values from bytes to ImVec4 and applies a custom style to
     * various ImGui elements.
     */
    void SetupImgui()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        //ImGui::Dock
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

            colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.15f, 0.15f, 0.25f);
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
    }

    /**
     * @brief Initializes ImGui for Vulkan and GLFW.
     * 
     * This function sets up ImGui to work with Vulkan and GLFW by initializing
     * the necessary ImGui implementations. It also profiles the initialization process.
     */
    void CreateImgui()
    {
        //SEDX_PROFILE_FUNC();
        ImGui_ImplGlfw_InitForVulkan(Window::GetGLFWwindow(), true);
        vkw::InitImGui();
    }

    /**
     * @brief Shuts down the ImGui context for Vulkan and GLFW.
     * 
     * This function cleans up and releases all resources allocated by ImGui
     * for Vulkan and GLFW, ensuring a proper shutdown of the ImGui context.
     */
    void DestroyImgui()
    {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
    }

    /**
     * @brief Destroys the ImGui context.
     * 
     * This function destroys the ImGui context, releasing all resources
     * allocated by ImGui and ensuring a proper cleanup.
     */
    void FinishImgui()
    {
        ImGui::DestroyContext();
    }
};


/**
 * @brief The main entry point of the application.
 * 
 * This function initializes the logging system, creates an instance of the 
 * SceneryEditorX::Application class, and runs the application. It also handles 
 * any exceptions that might be thrown during the execution of the application 
 * and prints the error message to the standard error output.
 * 
 * @return int Returns EXIT_SUCCESS on successful execution, or EXIT_FAILURE if 
 * an exception is caught.
 */
int main(int argc, char *argv[])
{
    Log::Init();
    Log::LogHeader();
    spdlog::info("Scenery Editor X Engine is starting...");

    SceneryEditorX::Application app;
    try
    {
        app.run();
    }
    catch (const std::exception &e)
    {
        spdlog::error("An exception occurred: {}", e.what());
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    spdlog::info("Scenery Editor X Engine is running.");
    return EXIT_SUCCESS;
}

/*
##########################################################
			IMGUI UI PANEL RENDERING FUNCTIONS
##########################################################
*/

/*
    void RenderMainMenu() 
{
	MainMenuBar();
}

void RenderPanels()
{
    LayerStack();
    AssetBrowser();
    SettingsPanel();
}
}

    return EXIT_SUCCESS;
}
*/

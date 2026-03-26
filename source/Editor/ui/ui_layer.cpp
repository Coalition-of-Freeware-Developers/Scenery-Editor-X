/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray 
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * ui_layer.cpp
 * -------------------------------------------------------
 * Created: 23/03/2026
 * -------------------------------------------------------
 */
#include "ui_layer.h"
#include "ui_impl.h"
#include "Editor/core/child_window.h"
#include "panels/asset_browser.h"
#include "panels/menu_bar.h"
#include "panels/properties.h"
#include "panels/render_options.h"
#include "panels/texure_viewer.h"
#include "source/imgui/imgui_internal.h"
#include "source/imgui/backends/imgui_impl_sdl3.h"
#include <Editor/ui/panels/scene_viewport.h>
#include <SceneryEditorX/core/resource/resource_cache.h>
#include <SceneryEditorX/core/window/window.h>
#include <SceneryEditorX/renderer/renderer.h>

using namespace SceneryEditorX;

// -------------------------------------------------------

static float s_FontSize  = 18.0f;
static float s_FontScale = 1.0f;

// -------------------------------------------------------

UILayer::UILayer() = default;

UILayer::UILayer(const std::string &name)
{
    (void)name; // name is unused in this simple stub
}

void UILayer::Tick()
{
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    BeginWindow();

    for (Ref<Widget>& widget : m_Widgets)
    {
        widget->Tick();
    }
    MenuBar::Tick();
    ImGui::End();

    // various windows that don't belong to a certain widget
    for (UI::ChildWindow& window : m_ChildWindows)
    {
        if (window.IsVisible())
            window.Tick();
    }

    ImGui::Render();

    // main window
    UI::Render(ImGui::GetDrawData());
    Renderer::SubmitAndPresent();

    // child windows
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }

}

void UILayer::OnAttach()
{
    ImGui::CreateContext();
    std::filesystem::path appdata = IO::FileSystem::GetPersistentStoragePath();

    // configure ImGui
    ImGuiIO& io                      = ImGui::GetIO();
    io.ConfigFlags                  |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags                  |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags                  |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigFlags                  |= ImGuiConfigFlags_NoMouseCursorChange; // cursor control is given to ImGui, but dynamically, from the engine
    io.ConfigWindowsResizeFromEdges  = true;
    io.IniFilename                   = (appdata / "editor.ini").string().c_str();

    // font_bold configuration
    ImFontConfig config; // config for bold font (mainly for use in headers)
    config.GlyphOffset.y = -2.0f; 

    const std::string dir_fonts = ResourceCache::GetResourceDirectory(ResourceDirectory::Fonts);
    fontNormal            = io.Fonts->AddFontFromFileTTF((dir_fonts + "opensans/OpenSans-Medium.ttf").c_str(), s_FontSize * Window::GetDpiScale());
    fontBold              = io.Fonts->AddFontFromFileTTF((dir_fonts + "opensans/OpenSans-Bold.ttf").c_str(), s_FontSize * Window::GetDpiScale(), &config);
    io.FontGlobalScale    = s_FontScale;

    // initialize imgui backends
    SEDX_CORE_ASSERT(ImGui_ImplSDL3_InitForVulkan(static_cast<SDL_Window*>(Window::GetRawHandle())), "Failed to initialize ImGui's SDL backend");
    UI::Initialize();

    // create all imgui widgets
    //m_Widgets.emplace_back(CreateRef<Style>(this));
    //m_Widgets.emplace_back(CreateRef<ProgressDialog>(this));
    //m_Widgets.emplace_back(CreateRef<ResourceViewer>(this));
    // Note: these widgets expect an EditorLayer* but UILayer isn't an EditorLayer.
    // Pass nullptr for now to satisfy constructor signatures. The EditorLayer
    // should supply proper pointers when integrating widgets into the editor.
    m_Widgets.emplace_back(CreateRef<RenderOptions>(nullptr));
    m_Widgets.emplace_back(CreateRef<TextureViewer>(nullptr));
    m_Widgets.emplace_back(CreateRef<SceneViewport>("viewport", nullptr));
    m_Widgets.emplace_back(CreateRef<AssetBrowser>(nullptr));
    m_Widgets.emplace_back(CreateRef<Properties>(nullptr));
    //m_Widgets.emplace_back(CreateRef<SceneViewer>(this));
}

void UILayer::OnDetach()
{
    if (ImGui::GetCurrentContext())
    {
        UI::Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }
}

void UILayer::BeginWindow()
{
    // note: don't use ImGuiWindowFlags_MenuBar here since we use BeginMainMenuBar() separately
    const auto window_flags =
        ImGuiWindowFlags_NoDocking             |
        ImGuiWindowFlags_NoTitleBar            |
        ImGuiWindowFlags_NoCollapse            |
        ImGuiWindowFlags_NoResize              |
        ImGuiWindowFlags_NoMove                |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus;

    // set window position and size to the work area (excludes main menu bar)
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    // draw window border for borderless window (use full viewport for border around entire window)
    {
        ImDrawList* draw_list = ImGui::GetForegroundDrawList();
        ImVec2 min = viewport->Pos;
        ImVec2 max = ImVec2(viewport->Pos.x + viewport->Size.x, viewport->Pos.y + viewport->Size.y);
        ImU32 border_color = IM_COL32(40, 40, 42, 255);
        draw_list->AddRect(min, max, border_color, 0.0f, 0, 1.0f);
    }

    // set window style
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,    ImVec2(0.0f, 0.0f));

    // begin window
    const char* name = "##main_window";
    bool open = true;
    ImGui::Begin(name, &open, window_flags);
    ImGui::PopStyleVar(3);

    // begin dock space
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        // dock space
        const auto window_id = ImGui::GetID(name);
        if (!ImGui::DockBuilderGetNode(window_id))
        {
            // reset current docking state
            ImGui::DockBuilderRemoveNode(window_id);
            ImGui::DockBuilderAddNode(window_id, ImGuiDockNodeFlags_None);
            ImGui::DockBuilderSetNodeSize(window_id, ImGui::GetMainViewport()->Size);

            // dockBuilderSplitNode(ImGuiID node_id, ImGuiDir split_dir, float size_ratio_for_node_at_dir, ImGuiID* out_id_dir, ImGuiID* out_id_other);
            ImGuiID dock_main_id       = window_id;
            ImGuiID dock_right_id      = ImGui::DockBuilderSplitNode(dock_main_id,  ImGuiDir_Right, 0.17f, nullptr, &dock_main_id);
            ImGuiID dock_right_down_id = ImGui::DockBuilderSplitNode(dock_right_id, ImGuiDir_Down,  0.6f,  nullptr, &dock_right_id);
            ImGuiID dock_down_id       = ImGui::DockBuilderSplitNode(dock_main_id,  ImGuiDir_Down,  0.22f, nullptr, &dock_main_id);
            ImGuiID dock_down_right_id = ImGui::DockBuilderSplitNode(dock_down_id,  ImGuiDir_Right, 0.3f,  nullptr, &dock_down_id);

            // dock windows
            ImGui::DockBuilderDockWindow("World",      dock_right_id);
            ImGui::DockBuilderDockWindow("Properties", dock_right_down_id);
            ImGui::DockBuilderDockWindow("Console",    dock_down_id);
            ImGui::DockBuilderDockWindow("Assets",     dock_down_right_id);
            ImGui::DockBuilderDockWindow("Viewport",   dock_main_id);

            ImGui::DockBuilderFinish(dock_main_id);
        }

        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        ImGui::DockSpace(window_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
        ImGui::PopStyleVar();
    }
}

void UILayer::OnRender()
{
    // No-op stub
}

void UILayer::SetDarkThemeColors()
{
    // Intentionally empty - keep compatibility shim
}

void UILayer::SetDarkThemeV2Colors()
{
    // Intentionally empty - keep compatibility shim
}

void UILayer::AllowInputEvents(bool allowInput)
{
    (void)allowInput;
}

// -------------------------------------------------------

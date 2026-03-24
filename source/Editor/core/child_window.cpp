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
 * child_window.cpp
 * -------------------------------------------------------
 * Created: 20/03/2026
 * -------------------------------------------------------
 */
#include "child_window.h"
#include "editor_layer.h"
#include <Editor/ui/panels/scene_viewport.h>
#include <Editor/ui/source/imgui/imgui.h>
#include <SceneryEditorX/core/window/window.h>

using namespace SceneryEditorX;

// ---------------------------------------------------------

namespace UI
{

    Ref<EditorLayer> s_Editor = nullptr;
    static bool s_Visible = true;

    void ChildWindow::CenterWindow()
    {
        Ref<EditorLayer> editor = s_Editor.Get();
        const Vec2 center = editor->GetWidget<SceneViewport>()->GetCenter();

        ImGui::SetNextWindowPos(ImVec2(center.x, center.y), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    }

    void ChildWindow::Init(const std::string &name)
    {
        if (!s_Visible)
            return;

        Ref<EditorLayer> editor = s_Editor.Get();
        const Vec2 center = editor->GetWidget<SceneViewport>()->GetCenter();

        ImGui::SetNextWindowPos(ImVec2(center.x, center.y), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::Begin(name.c_str(), &s_Visible, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize))
        {
            float contentWidth = 500.0f * Window::GetDpiScale();
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + contentWidth);
        }

        ImGui::End();
    }

}

// ---------------------------------------------------------


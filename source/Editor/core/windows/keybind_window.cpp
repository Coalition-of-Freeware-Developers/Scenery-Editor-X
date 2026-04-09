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
 * keybind_window.cpp
 * -------------------------------------------------------
 * Created: 20/03/2026
 * -------------------------------------------------------
 */
#include "keybind_window.h"
#include "Editor/core/editor_layer.h"
#include <SceneryEditorX/core/window/window.h>
#include <SceneryEditorX/renderer/ui/panels/scene_viewport.h>
#include <SceneryEditorX/renderer/ui/source/imgui/imgui.h>

// ---------------------------------------------------------

static bool s_Visible = false;
static SceneryEditorX::EditorLayer* editor = nullptr;

KeybindWindow::KeybindWindow()
{
	if (!s_Visible)
		return;
}

void KeybindWindow::Tick()
{
	if (!s_Visible)
		return;

	// center the window on first use, but let user move it freely afterward
	// Convert the project's Vec2 center to ImGui's ImVec2 explicitly
	{
		Vec2 center = editor->GetWidget<SceneryEditorX::SceneViewport>()->GetCenter();
		ImGui::SetNextWindowPos(ImVec2(center.x, center.y), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
	}

	// set a reasonable default size (wider for the three-column layout)
	ImGui::SetNextWindowSize(ImVec2(600.0f * SceneryEditorX::Window::GetDpiScale(), 400.0f * SceneryEditorX::Window::GetDpiScale()), ImGuiCond_FirstUseEver);

	if (ImGui::Begin("Controls & Shortcuts", &s_Visible, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking))
	{
		if (ImGui::BeginTabBar("##controls_tabs"))
		{
			if (ImGui::BeginTabItem("Editor Shortcuts"))
			{
				ImGui::Spacing();
				//ShowShortcutTable("##editor_shortcuts_table", editor_shortcuts, std::size(editor_shortcuts));
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Camera"))
			{
				ImGui::Spacing();
				//show_control_binding_table("##camera_controls_table", camera_controls_full, std::size(camera_controls_full));
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}
	}

	ImGui::End();
}

bool *KeybindWindow::ShowWindow()
{
	s_Visible = true;
	return &s_Visible;
}

bool KeybindWindow::IsVisible()
{
	return s_Visible;
}


// ---------------------------------------------------------

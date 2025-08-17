/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* layer_stack.cpp
* -------------------------------------------------------
* Created: 29/3/2025
* -------------------------------------------------------
*/
#include "SceneryEditorX/ui/ui.h"
#include "SceneryEditorX/ui/ui_manager.h"
#include <imgui/imgui.h>

/// -------------------------------------------------------

namespace SceneryEditorX::UI
{
	void UIManager::LayerStack()
    {
		ImGui::Begin("Layer Stack");
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{4, 4});
        ImGui::Separator();
        ImGui::PopStyleVar();
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
        ImGui::Columns(3, "TreeViewColumns", true);
        ImGui::Text("Name");
        ImGui::NextColumn();
        ImGui::Text("Type");
        ImGui::NextColumn();
        ImGui::Text("Property");
        ImGui::Spacing();
		ImGui::End();
    }

}

/// -------------------------------------------------------


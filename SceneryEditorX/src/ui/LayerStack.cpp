
#include "imgui.h"
#include <GLFW/glfw3.h>

#include <string>
#include <vector>

#include "UI.h"

namespace SceneryEditorX::UI
{
    void LayerStack()
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

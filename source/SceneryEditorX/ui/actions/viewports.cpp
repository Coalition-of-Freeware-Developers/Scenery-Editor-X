/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* viewports.cpp
* -------------------------------------------------------
* Created: 29/3/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/renderer/vk_util.h>
#include <SceneryEditorX/ui/ui_manager.h>
#include <SceneryEditorX/ui/ui.h>

// -------------------------------------------------------

namespace UI
{
    //bool ViewportPanel(glm::ivec2 & newViewportSize)
    //{
    //    bool hovered = false;
    //    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
    //    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    //    if (ImGui::Begin("Viewport"))
    //    {
    //        ImGui::BeginChild("##ChildViewport");
    //        newViewportSize = {ImGui::GetWindowSize().x, ImGui::GetWindowSize().y};
    //        ImGui::Image(image.ImGuiRID(), ImGui::GetWindowSize());
    //        ImGuizmo::SetDrawlist();
    //        ImGuizmo::SetRect(ImGui::GetWindowPos().x,
    //                          ImGui::GetWindowPos().y,
    //                          ImGui::GetWindowSize().x,
    //                          ImGui::GetWindowSize().y);
    //        hovered = ImGui::IsWindowHovered() && !ImGuizmo::IsUsing();
    //        ImGui::EndChild();
    //    }
    //    ImGui::PopStyleVar(2);
    //    ImGui::End();
    //    return hovered;
    //}

} // namespace UI

// -------------------------------------------------------

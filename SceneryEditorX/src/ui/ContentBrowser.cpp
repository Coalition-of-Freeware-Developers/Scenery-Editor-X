#include "../xpeditorpch.h"
#include <imgui.h>
#include <imgui_internal.h>

namespace SceneryEditorX::UI
{
    void AssetBrowser()
    {
        ImGui::Begin("Asset Browser",
                     nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoCollapse);
        ImGuiWindowFlags child_flags =
            ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar;
        ImGui::SetScrollHereX(0.25f); // 0.0f:left, 0.5f:center, 1.0f:right

            ImGui::BeginChild("Library Items", ImVec2(0, 0), false, child_flags);
            ImGuiTableFlags table_flags_for_sort_specs = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Borders;
            if (ImGui::BeginTable("for_sort_specs_only", 2, table_flags_for_sort_specs, ImVec2(0.0f, ImGui::GetFrameHeight())))
            {
                //ImGui::TableSetupColumn("Index");
                //ImGui::TableSetupColumn("Type");
                //ImGui::TableHeadersRow();
                if (ImGuiTableSortSpecs *sort_specs = ImGui::TableGetSortSpecs())
                    if (sort_specs->SpecsDirty) { }
                ImGui::EndTable();
            }
            ImGui::Text("Selected: %d/%d items" /*, Selection.Size, Items.Size */ );
            ImGui::EndChild();

        ImGui::End();
    }
}

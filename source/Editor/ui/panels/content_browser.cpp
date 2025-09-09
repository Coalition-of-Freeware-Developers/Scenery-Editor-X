/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* content_browser.cpp
* -------------------------------------------------------
* Created: 29/3/2025
* -------------------------------------------------------
*/
//#include <imgui/imgui.h>
//#include <imgui/imgui_internal.h>
//#include <SceneryEditorX/ui/ui.h>

// -------------------------------------------------------

/*
namespace UI
{
	void UIManager::AssetBrowser()
    {
        ImGui::Begin("Asset Browser",
                     nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoCollapse);
        constexpr ImGuiWindowFlags child_flags =
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
            ImGui::Text(R"(Selected: %d/%d items)" /*, Selection.Size, Items.Size #1# );
            ImGui::EndChild();

        ImGui::End();
    }
} // namespace UI
*/

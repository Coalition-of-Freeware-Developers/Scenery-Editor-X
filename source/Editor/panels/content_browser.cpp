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

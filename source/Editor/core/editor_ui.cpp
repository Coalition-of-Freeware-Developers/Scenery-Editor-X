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
 * editor_ui.cpp
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#include "editor_ui.h"
#include "editor_theme.h"
#include <SceneryEditorX/renderer/ui/resources/fonts.h>
#include <SceneryEditorX/renderer/ui/source/imgui/imgui_internal.h>

// -------------------------------------------------------

namespace SceneryEditorX::EditorUI
{
	bool BeginPanel(const char *name, bool *open, const ImGuiWindowFlags flags)
	{
		const bool visible = ImGui::Begin(name, open, flags);

		// Substance marks the active panel tab with an accent underline rather than a filled tab.
		ImGuiWindow *window = ImGui::GetCurrentWindow();
		ImGuiDockNode *node = window->DockNode;
		if (window->DockIsActive && window->DockTabIsVisible && node && node->TabBar && !node->IsHiddenTabBar() && !node->IsNoTabBar())
		{
			const ImRect &tab = window->DC.DockTabItemRect;
			ImDrawList *drawList = node->HostWindow ? node->HostWindow->DrawList : nullptr;
			if (drawList && tab.GetWidth() > 0.0f)
			{
				const float thickness = ImMax(1.0f, 2.0f * ImGui::GetStyle().FontScaleDpi);
				drawList->PushClipRectFullScreen();
				drawList->AddRectFilled(ImVec2(tab.Min.x, tab.Max.y - thickness), tab.Max, EditorTheme::ToU32(EditorTheme::Palette::Accent));
				drawList->PopClipRect();
			}
		}

		return visible;
	}

	void EndPanel()
	{
		ImGui::End();
	}

	bool SectionHeader(const char *label, const bool defaultOpen)
	{
		// Plain uppercase label like Substance's "GENERAL PROPERTIES"; no frame, still collapsible.
		ImGui::Spacing();
		UI::Fonts::PushFont("SemiBold");
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, IM_COL32(0, 0, 0, 0));

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_NoAutoOpenOnLog;
		if (defaultOpen)
			flags |= ImGuiTreeNodeFlags_DefaultOpen;

		const bool open = ImGui::TreeNodeEx(label, flags);

		ImGui::PopStyleColor(2);
		UI::Fonts::PopFont();

		if (open)
			ImGui::Spacing();

		return open;
	}

	bool BeginPropertyGrid(const char *id, const float labelWidth)
	{
		if (!ImGui::BeginTable(id, 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_PadOuterX))
			return false;

		ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, labelWidth * ImGui::GetStyle().FontScaleDpi);
		ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);
		return true;
	}

	void PropertyLabel(const char *label)
	{
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
		ImGui::TextUnformatted(label);
		ImGui::PopStyleColor();
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-FLT_MIN);
	}

	void EndPropertyGrid()
	{
		ImGui::EndTable();
	}

	bool StripButton(const char *icon, const char *tooltip, const bool active, const float size, const StripStyle style)
	{
		using namespace EditorTheme;

		const bool filled = active && style == StripStyle::Filled;
		ImVec4 text = ToStyle(Palette::TextLabel);
		if (filled)
			text = ToStyle(ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
		else if (active)
			text = ToStyle(Palette::AccentHovered);

		ImGui::PushStyleColor(ImGuiCol_Button, filled ? ToStyle(Palette::Accent) : ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, filled ? ToStyle(Palette::AccentHovered) : ToStyle(Palette::ButtonHovered));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ToStyle(Palette::Accent));
		ImGui::PushStyleColor(ImGuiCol_Text, text);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);

		const bool clicked = ImGui::Button(icon, ImVec2(size, size));

		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(4);

		ItemTooltip(tooltip);
		return clicked;
	}

	void EmptyState(const char *text)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
		ImGui::TextUnformatted(text);
		ImGui::PopStyleColor();
	}

	void SearchFilter(const char *id, ImGuiTextFilter &filter, const float width)
	{
		ImGui::SetNextItemWidth(width);
		if (ImGui::InputTextWithHint(id, "Search", filter.InputBuf, IM_COUNTOF(filter.InputBuf)))
			filter.Build();
	}

	void ItemTooltip(const char *text)
	{
		if (text && text[0] != '\0' && ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
			ImGui::SetTooltip("%s", text);
	}

	std::string IconLabel(const char *icon, const char *label)
	{
		return std::string(icon) + "  " + label;
	}

}

// -------------------------------------------------------

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
 * facade_inspector_panel.cpp
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#include "facade_inspector_panel.h"
#include <Editor/core/editor_project.h>
#include <Editor/core/editor_theme.h>
#include <Editor/core/editor_tools.h>
#include <Editor/core/editor_ui.h>
#include <Editor/core/facade_tool.h>
#include <algorithm>
#include <cmath>
#include <SceneryEditorX/filesystem/file_system.h>
#include <SceneryEditorX/project/scenery_project.h>
#include <SceneryEditorX/renderer/font/font_awesome.h>
#include <SceneryEditorX/renderer/ui/resources/fonts.h>
#include <SceneryEditorX/xplane/xp_facade.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		std::string Meters(const double meters)
		{
			char text[32];
			snprintf(text, sizeof(text), "%.2f m", meters);
			return text;
		}

		/* @brief Segment colors for the spelling bar (cycled by segment index). */
		ImU32 SegmentColor(const int segment)
		{
			static constexpr ImU32 COLORS[] = { IM_COL32(31, 138, 214, 255), IM_COL32(237, 180, 66, 255), IM_COL32(94, 186, 125, 255), IM_COL32(200, 95, 160, 255),
				IM_COL32(120, 120, 220, 255), IM_COL32(220, 110, 80, 255), IM_COL32(90, 190, 200, 255), IM_COL32(170, 170, 90, 255) };
			return COLORS[static_cast<size_t>(std::max(0, segment)) % std::size(COLORS)];
		}

		size_t CountTriangles(const XPlane::FacadeAsset::Segment &segment)
		{
			size_t triangles = 0;
			for (const XPlane::FacadeAsset::Mesh &mesh : segment.meshes)
				triangles += mesh.indices.size() / 3;
			return triangles;
		}

		void Row(const char *label, const std::string &value, const char *tooltip = nullptr)
		{
			EditorUI::PropertyLabel(label);
			ImGui::TextWrapped("%s", value.empty() ? "-" : value.c_str());
			if (tooltip)
				EditorUI::ItemTooltip(tooltip);
		}
	}

	FacadeInspectorPanel::FacadeInspectorPanel(EditorContext *context) : m_Context(context)
	{
		m_Title = "FACADES";
	}

	void FacadeInspectorPanel::OnUIRender(bool &isOpen)
	{
		if (FacadeTool::ConsumeInspectRequest())
			m_Follow = false;

		if (EditorUI::BeginPanel("FACADES###FacadeInspector", &isOpen))
		{
			// Which facade: the selected one, else the armed one; or the one sent with Inspect
			std::string resource;
			if (m_Follow)
			{
				const std::vector<std::string> selected = FacadeTool::GetSelectedFacades();
				ProjectFile::ProjectDocument *document = EditorProjects::GetDocument();
				const ProjectFile::SceneryItem *item = !selected.empty() && document ? document->FindItem(selected.front()) : nullptr;
				resource = item ? item->resource : FacadeTool::GetSettings().resource;
				if (resource.empty())
					resource = FacadeTool::GetInspected();
			}
			else
				resource = FacadeTool::GetInspected();

			ImGui::Checkbox("Follow selection", &m_Follow);
			EditorUI::ItemTooltip("On: the selected facade (or the one armed in the facade tool). Off: the facade last sent here with Inspect");
			ImGui::SameLine();
			if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_REFRESH, "Reload").c_str()))
				EditorProjects::ReloadArt();
			EditorUI::ItemTooltip("Read the facades (and the project's other art) again after editing them on disk, and redraw");
			ImGui::SameLine();
			ImGui::BeginDisabled(resource.empty() || !EditorProjects::GetDocument());
			if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_PENCIL, "Draw with it").c_str()))
			{
				FacadeTool::SetResource(resource);
				EditorTools::SetTool(EditorTools::Tool::Facade);
			}
			ImGui::EndDisabled();

			const XPlane::FacadeAsset *asset = resource.empty() ? nullptr : FacadeTool::GetAsset(resource);
			if (resource.empty())
				EditorUI::EmptyState("Select a facade, arm one in the facade tool (B), or Inspect one from the X-PLANE LIBRARY panel");
			else if (!asset)
			{
				ImGui::TextColored(EditorTheme::ToStyle(EditorTheme::Palette::Error), "%s can't be found or read (see the console)", resource.c_str());
			}
			else
			{
				DrawSummary(resource, *asset);
				DrawFloors(*asset);
				DrawWalls(*asset);
				DrawSpellingPreview(*asset);
				DrawSegments(*asset);
			}
		}
		EditorUI::EndPanel();
	}

	void FacadeInspectorPanel::DrawSummary(const std::string &resource, const XPlane::FacadeAsset &asset)
	{
		UI::Fonts::PushFont("Bold");
		ImGui::TextWrapped("%s", resource.c_str());
		UI::Fonts::PopFont();
		if (!EditorUI::SectionHeader("FACADE") || !EditorUI::BeginPropertyGrid("##FacadeSummary", 110.0f))
			return;

		const std::filesystem::path file = EditorProjects::ResolveResourceFile(resource);
		EditorUI::PropertyLabel("File");
		ImGui::TextWrapped("%s", file.filename().string().c_str());
		EditorUI::ItemTooltip(file.string().c_str());
		if (ImGui::IsItemClicked() && !file.empty())
			IO::FileSystem::OpenExternally(file.parent_path());

		Row("Type", std::string(asset.IsType2() ? "2 (segment meshes, version 1000)" : "1 (texture panels, version 800)"));
		Row("Kind", XPlane::GetFacadeFamilyName(XPlane::GetFacadeFamily(asset)),
			"How the editor draws it: closed footprints (buildings, ground facades such as 3D sidewalks, enclosures) or open lines (RING 0: fences, hangars, doors)");
		Row("Footprint", std::string(asset.ring ? "Closed ring" : "Open line (RING 0)") + (asset.graded ? ", graded (flat on its first wall)" : ", draped on the terrain") +
			(asset.twoSided ? ", two-sided" : "") + (asset.doubled ? ", doubled walls" : ""));
		Row("Heights", XPlane::DescribeFacadeHeights(XPlane::GetFacadeHeights(asset)));
		Row("Wall types", std::to_string(XPlane::GetFacadeWallCount(asset)));
		Row("Wall texture", asset.wallTexture + (asset.wallTextureNormal.empty() ? std::string() : "  (normal: " + asset.wallTextureNormal + ")"));
		if (asset.hasRoof)
			Row("Roof", (asset.noRoofMesh ? std::string("No roof mesh") : asset.roofTexture.empty() ? std::string("Wall texture") : asset.roofTexture) + ", " +
				Meters(asset.roofScaleS) + " x " + Meters(asset.roofScaleT) + " per repeat");
		else
			Row("Roof", "None");
		if (!asset.layerGroup.empty() || !asset.drapedLayerGroup.empty())
			Row("Layer groups", (asset.layerGroup.empty() ? std::string("objects") : asset.layerGroup + " " + std::to_string(asset.layerOffset)) +
				(asset.drapedLayerGroup.empty() ? std::string() : ", draped: " + asset.drapedLayerGroup + " " + std::to_string(asset.drapedLayerOffset)));
		if (!asset.hardWall.empty() || !asset.hardRoof.empty())
			Row("Hard surfaces", (asset.hardWall.empty() ? std::string() : "walls " + asset.hardWall + " ") + (asset.hardRoof.empty() ? std::string() : "roof " + asset.hardRoof));
		if (!asset.scrapers.empty())
		{
			const XPlane::FacadeAsset::Scraper &scraper = asset.scrapers.front();
			char text[128];
			snprintf(text, sizeof(text), "%zu: %.0f - %.0f m AGL, %d models, %d pads", asset.scrapers.size(), scraper.minAgl, scraper.maxAgl, scraper.models, scraper.pads);
			Row("Skyscraper", text, "FACADE_SCRAPER: above these heights X-Plane builds base and tower models on the footprint instead");
		}
		if (!asset.objects.empty())
		{
			EditorUI::PropertyLabel("Objects");
			if (ImGui::TreeNode("##Objects", "%zu attached / roof objects", asset.objects.size()))
			{
				for (size_t i = 0; i < asset.objects.size(); ++i)
					ImGui::BulletText("%zu  %s", i, asset.objects[i].c_str());
				ImGui::TreePop();
			}
		}
		EditorUI::EndPropertyGrid();
	}

	void FacadeInspectorPanel::DrawFloors(const XPlane::FacadeAsset &asset)
	{
		if (!asset.IsType2() || asset.floors.empty() || !EditorUI::SectionHeader("FLOORS"))
			return;
		ImGui::TextDisabled("X-Plane builds the floor whose top roof is closest to the facade's height");
		constexpr ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit;
		if (!ImGui::BeginTable("##Floors", 6, flags))
			return;
		ImGui::TableSetupColumn("#");
		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Height");
		ImGui::TableSetupColumn("Roofs");
		ImGui::TableSetupColumn("Segments");
		ImGui::TableSetupColumn("Walls");
		ImGui::TableHeadersRow();
		for (size_t f = 0; f < asset.floors.size(); ++f)
		{
			const XPlane::FacadeAsset::Floor &floor = asset.floors[f];
			ImGui::TableNextRow();
			ImGui::PushID(static_cast<int>(f));
			ImGui::TableNextColumn();
			if (ImGui::Selectable(std::to_string(f).c_str(), m_Floor == static_cast<int>(f), ImGuiSelectableFlags_SpanAllColumns))
				m_Floor = static_cast<int>(f);
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(floor.name.c_str());
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(Meters(floor.height).c_str());
			ImGui::TableNextColumn();
			std::string roofs;
			size_t roofObjects = 0;
			for (const XPlane::FacadeAsset::Roof &roof : floor.roofs)
			{
				char text[32];
				snprintf(text, sizeof(text), "%s%.2g%s", roofs.empty() ? "" : ", ", roof.height, roof.twoSided ? "*" : "");
				roofs += text;
				roofObjects += roof.objects.size();
			}
			ImGui::TextUnformatted(roofs.empty() ? "-" : roofs.c_str());
			if (roofObjects || !floor.hardRoof.empty())
				EditorUI::ItemTooltip((std::to_string(roofObjects) + " roof objects" + (floor.hardRoof.empty() ? std::string() : ", hard roof: " + floor.hardRoof) + "\n* two-sided").c_str());
			ImGui::TableNextColumn();
			ImGui::Text("%zu%s", floor.segments.size(), floor.curvedSegments.size() == floor.segments.size() && !floor.segments.empty() ? " + curved" : "");
			ImGui::TableNextColumn();
			ImGui::Text("%zu", floor.walls.size());
			ImGui::PopID();
		}
		ImGui::EndTable();
		m_Floor = std::clamp(m_Floor, 0, static_cast<int>(asset.floors.size()) - 1);
	}

	void FacadeInspectorPanel::DrawWalls(const XPlane::FacadeAsset &asset)
	{
		if (!EditorUI::SectionHeader("WALLS"))
			return;
		const std::vector<std::string> names = XPlane::GetFacadeWallNames(asset);
		constexpr ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit;
		if (!asset.IsType2())
		{
			// Type 1: panels across and floors up
			if (!ImGui::BeginTable("##Walls1", 5, flags))
				return;
			ImGui::TableSetupColumn("#");
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Rules");
			ImGui::TableSetupColumn("Texture");
			ImGui::TableSetupColumn("Panels / floors");
			ImGui::TableHeadersRow();
			for (size_t w = 0; w < asset.walls.size(); ++w)
			{
				const XPlane::FacadeAsset::Wall &wall = asset.walls[w];
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text("%zu", w + 1);
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(w < names.size() ? names[w].c_str() : "");
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(XPlane::DescribeFacadeWall(asset, static_cast<int>(w)).c_str());
				ImGui::TableNextColumn();
				ImGui::Text("%.3g x %.3g m", wall.scaleS, wall.scaleT);
				ImGui::TableNextColumn();
				ImGui::Text("L%zu C%zu R%zu / B%zu M%zu T%zu", wall.leftPanels.size(), wall.centerPanels.size(), wall.rightPanels.size(), wall.bottomFloors.size(),
					wall.middleFloors.size(), wall.topFloors.size());
			}
			ImGui::EndTable();
			return;
		}

		if (asset.floors.empty())
			return;
		const XPlane::FacadeAsset::Floor &floor = asset.floors[static_cast<size_t>(std::clamp(m_Floor, 0, static_cast<int>(asset.floors.size()) - 1))];
		ImGui::TextDisabled("Floor %d \"%s\". Natural lengths: what the wall's spellings fill without stretching (fit hangar doors to these)", m_Floor, floor.name.c_str());
		if (!ImGui::BeginTable("##Walls2", 5, flags))
			return;
		ImGui::TableSetupColumn("#");
		ImGui::TableSetupColumn("Name");
		ImGui::TableSetupColumn("Rules", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Spellings");
		ImGui::TableSetupColumn("Natural lengths");
		ImGui::TableHeadersRow();
		for (size_t w = 0; w < floor.walls.size(); ++w)
		{
			const XPlane::FacadeAsset::WallRule &wall = floor.walls[w];
			ImGui::TableNextRow();
			ImGui::PushID(static_cast<int>(w));
			ImGui::TableNextColumn();
			if (ImGui::Selectable(std::to_string(w + 1).c_str(), m_Wall == static_cast<int>(w), ImGuiSelectableFlags_SpanAllColumns))
				m_Wall = static_cast<int>(w);
			EditorUI::ItemTooltip("Preview this wall's spelling below");
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(wall.name.empty() ? ("Wall " + std::to_string(w + 1)).c_str() : wall.name.c_str());
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(XPlane::DescribeFacadeWall(asset, static_cast<int>(w), m_Floor).c_str());
			ImGui::TableNextColumn();
			ImGui::Text("%zu", wall.spellings.size());
			ImGui::TableNextColumn();
			const std::vector<double> natural = XPlane::GetFacadeNaturalLengths(asset, m_Floor, static_cast<int>(w), 100.0, 7);
			std::string text;
			for (size_t i = 0; i < natural.size() && i < 6; ++i)
			{
				char value[24];
				snprintf(value, sizeof(value), "%s%.4g", text.empty() ? "" : ", ", natural[i]);
				text += value;
			}
			if (natural.size() > 6)
				text += ", ...";
			ImGui::TextUnformatted(text.empty() ? "-" : (text + " m").c_str());
			ImGui::PopID();
		}
		ImGui::EndTable();
	}

	void FacadeInspectorPanel::DrawSpellingPreview(const XPlane::FacadeAsset &asset)
	{
		if (!asset.IsType2() || asset.floors.empty() || !EditorUI::SectionHeader("SPELLING PREVIEW"))
			return;
		const XPlane::FacadeAsset::Floor &floor = asset.floors[static_cast<size_t>(std::clamp(m_Floor, 0, static_cast<int>(asset.floors.size()) - 1))];
		if (floor.walls.empty())
			return;
		m_Wall = std::clamp(m_Wall, 0, static_cast<int>(floor.walls.size()) - 1);
		const float dpi = ImGui::GetStyle().FontScaleDpi;

		const std::string wallName = floor.walls[static_cast<size_t>(m_Wall)].name.empty() ? "Wall " + std::to_string(m_Wall + 1) : floor.walls[static_cast<size_t>(m_Wall)].name;
		ImGui::SetNextItemWidth(160.0f * dpi);
		if (ImGui::BeginCombo("##PreviewWall", wallName.c_str()))
		{
			for (size_t w = 0; w < floor.walls.size(); ++w)
			{
				const std::string name = floor.walls[w].name.empty() ? "Wall " + std::to_string(w + 1) : floor.walls[w].name;
				if (ImGui::Selectable(name.c_str(), static_cast<int>(w) == m_Wall))
					m_Wall = static_cast<int>(w);
			}
			ImGui::EndCombo();
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::SliderFloat("##PreviewLength", &m_Length, 0.5f, 120.0f, "Wall length %.2f m", ImGuiSliderFlags_Logarithmic);

		double total = 0.0;
		const std::vector<int> spelling = XPlane::PickFacadeSpelling(floor, floor.walls[static_cast<size_t>(m_Wall)], m_Length, total);
		if (spelling.empty() || total <= 0.0)
		{
			ImGui::TextDisabled("This wall has no spellings with any width");
			return;
		}
		const double stretch = m_Length / total;

		// The wall as X-Plane spells it: each segment at its stretched width, numbered
		ImDrawList *draw = ImGui::GetWindowDrawList();
		const ImVec2 min = ImGui::GetCursorScreenPos();
		const float width = ImGui::GetContentRegionAvail().x;
		const float height = 34.0f * dpi;
		draw->AddRectFilled(min, ImVec2(min.x + width, min.y + height), EditorTheme::ToU32(EditorTheme::Palette::Frame), 3.0f);
		float x = min.x;
		for (const int segment : spelling)
		{
			if (segment < 0 || static_cast<size_t>(segment) >= floor.segments.size())
				continue;
			const float w = static_cast<float>(floor.segments[static_cast<size_t>(segment)].width / total) * width;
			draw->AddRectFilled(ImVec2(x + 1.0f, min.y + 2.0f), ImVec2(x + w - 1.0f, min.y + height - 2.0f), SegmentColor(segment), 2.0f);
			const std::string label = std::to_string(segment);
			const ImVec2 size = ImGui::CalcTextSize(label.c_str());
			if (size.x + 4.0f < w)
				draw->AddText(ImVec2(x + (w - size.x) * 0.5f, min.y + (height - size.y) * 0.5f), IM_COL32(20, 20, 20, 255), label.c_str());
			x += w;
		}
		ImGui::Dummy(ImVec2(width, height));

		const int percent = static_cast<int>(std::lround(stretch * 100.0));
		ImGui::Text("%zu segments, %.2f m natural, stretched to %d%%", spelling.size(), total, percent);
		const std::vector<double> natural = XPlane::GetFacadeNaturalLengths(asset, m_Floor, m_Wall, 400.0, 400);
		if (!natural.empty())
		{
			const auto above = std::ranges::lower_bound(natural, static_cast<double>(m_Length));
			const double shorter = above == natural.begin() ? natural.front() : *(above - 1);
			const double longer = above == natural.end() ? natural.back() : *above;
			ImGui::TextDisabled("Nearest lengths without stretching: %.2f m and %.2f m", shorter, longer);
		}
	}

	void FacadeInspectorPanel::DrawSegments(const XPlane::FacadeAsset &asset)
	{
		if (!asset.IsType2() || asset.floors.empty() || !EditorUI::SectionHeader("SEGMENTS", false))
			return;
		const XPlane::FacadeAsset::Floor &floor = asset.floors[static_cast<size_t>(std::clamp(m_Floor, 0, static_cast<int>(asset.floors.size()) - 1))];
		constexpr ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY;
		const float rows = static_cast<float>(std::min<size_t>(floor.segments.size(), 12)) + 1.5f;
		if (!ImGui::BeginTable("##Segments", 5, flags, ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * rows)))
			return;
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("#");
		ImGui::TableSetupColumn("Width");
		ImGui::TableSetupColumn("Triangles");
		ImGui::TableSetupColumn("Curved");
		ImGui::TableSetupColumn("Attached objects", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableHeadersRow();
		ImGuiListClipper clipper;
		clipper.Begin(static_cast<int>(floor.segments.size()));
		while (clipper.Step())
		{
			for (int s = clipper.DisplayStart; s < clipper.DisplayEnd; ++s)
			{
				const XPlane::FacadeAsset::Segment &segment = floor.segments[static_cast<size_t>(s)];
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text("%d", s);
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(Meters(segment.width).c_str());
				ImGui::TableNextColumn();
				ImGui::Text("%zu", CountTriangles(segment));
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(static_cast<size_t>(s) < floor.curvedSegments.size() ? "yes" : "-");
				ImGui::TableNextColumn();
				std::string objects;
				for (const XPlane::FacadeAsset::Attachment &attachment : segment.attachments)
				{
					const std::string name = attachment.object >= 0 && static_cast<size_t>(attachment.object) < asset.objects.size() ? asset.objects[static_cast<size_t>(attachment.object)] : "?";
					objects += (objects.empty() ? "" : ", ") + name.substr(name.find_last_of('/') == std::string::npos ? 0 : name.find_last_of('/') + 1);
				}
				ImGui::TextUnformatted(objects.empty() ? "-" : objects.c_str());
			}
		}
		ImGui::EndTable();
	}
}

// -------------------------------------------------------

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
 * facade_editor_panel.cpp
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#include "facade_editor_panel.h"
#include <Editor/core/editor_commands.h>
#include <Editor/core/editor_project.h>
#include <Editor/core/editor_selection.h>
#include <Editor/core/editor_theme.h>
#include <Editor/core/editor_tools.h>
#include <Editor/core/editor_ui.h>
#include <Editor/core/facade_authoring.h>
#include <Editor/core/facade_tool.h>
#include <algorithm>
#include <cmath>
#include <map>
#include <sstream>
#include <SceneryEditorX/filesystem/vfs.h>
#include <SceneryEditorX/renderer/font/font_awesome.h>
#include <SceneryEditorX/renderer/ui/source/imgui/misc/cpp/imgui_stdlib.h>
#include <SceneryEditorX/renderer/ui/ui_impl.h>
#include <SceneryEditorX/xplane/xp_facade.h>
#include <SceneryEditorX/xplane/xp_texture_loader.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	using XPlane::FacadeAsset;
	using XPlane::FacadePanelRow;

	namespace
	{
		constexpr FacadePanelRow ACROSS[] = { FacadePanelRow::Left, FacadePanelRow::Center, FacadePanelRow::Right };
		constexpr FacadePanelRow UP[] = { FacadePanelRow::Bottom, FacadePanelRow::Middle, FacadePanelRow::Top };
		constexpr float EDGE_GRAB = 5.0f;	// Pixels either side of a panel edge that still grab it

		/* @brief Row colors: blue for the first row (left / bottom), green for the repeated one, orange for the last. */
		ImU32 RowColor(const FacadePanelRow row, const float alpha)
		{
			const int kind = static_cast<int>(row) % 3;
			const ImVec4 colors[] = { { 0.122f, 0.541f, 0.839f, alpha }, { 0.369f, 0.729f, 0.490f, alpha }, { 0.929f, 0.62f, 0.259f, alpha } };
			return EditorTheme::ToU32(colors[kind]);
		}

		using CanvasTexture = FacadeEditorPanel::CanvasTexture;

		/* @brief The texture drawn in a scrolling child; texture coordinates (s right, t up) to screen and back. */
		struct Canvas
		{
			ImVec2 min, max;
			bool hovered = false;
			bool active = false;
			bool activated = false;

			[[nodiscard]] ImVec2 ToScreen(const float s, const float t) const { return { min.x + s * (max.x - min.x), max.y - t * (max.y - min.y) }; }
			[[nodiscard]] float ToS(const float x) const { return (x - min.x) / std::max(1.0f, max.x - min.x); }
			[[nodiscard]] float ToT(const float y) const { return (max.y - y) / std::max(1.0f, max.y - min.y); }
		};

		/* @brief Starts a canvas (call EndCanvas after drawing on it). Ctrl + wheel zooms. */
		Canvas BeginCanvas(const char *id, const CanvasTexture &texture, float &zoom)
		{
			const float dpi = ImGui::GetStyle().FontScaleDpi;
			const float aspect = texture.width > 0 && texture.height > 0 ? static_cast<float>(texture.width) / static_cast<float>(texture.height) : 1.0f;
			const float maxHeight = 360.0f * dpi;
			float width = std::max(64.0f, ImGui::GetContentRegionAvail().x - 4.0f);
			float height = width / aspect;
			if (height > maxHeight)
			{
				height = maxHeight;
				width = height * aspect;
			}
			const ImVec2 size(width * zoom, height * zoom);
			ImGui::BeginChild(id, ImVec2(0.0f, std::min(size.y, maxHeight) + ImGui::GetStyle().ScrollbarSize + 6.0f), ImGuiChildFlags_Borders,
				ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
			Canvas canvas;
			canvas.min = ImGui::GetCursorScreenPos();
			canvas.max = ImVec2(canvas.min.x + size.x, canvas.min.y + size.y);
			ImGui::InvisibleButton("##Surface", size);
			canvas.hovered = ImGui::IsItemHovered();
			canvas.active = ImGui::IsItemActive();
			canvas.activated = ImGui::IsItemActivated();

			ImDrawList *draw = ImGui::GetWindowDrawList();
			if (const ImTextureID textureId = texture.image ? UI::GetTextureID(texture.image.Get()) : ImTextureID_Invalid; textureId != ImTextureID_Invalid)
				draw->AddImage(textureId, canvas.min, canvas.max, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));	// Row 0 is t = 0 (the bottom)
			else
			{
				draw->AddRectFilled(canvas.min, canvas.max, EditorTheme::ToU32(EditorTheme::Palette::Frame));
				draw->AddText(ImVec2(canvas.min.x + 8.0f, canvas.min.y + 8.0f), EditorTheme::ToU32(EditorTheme::Palette::TextDisabled), "No texture (choose one above)");
			}
			draw->AddRect(canvas.min, canvas.max, EditorTheme::ToU32(EditorTheme::Palette::Border));

			// Ctrl + wheel zooms about the cursor
			const ImGuiIO &io = ImGui::GetIO();
			if (canvas.hovered && io.KeyCtrl && io.MouseWheel != 0.0f)
				zoom = std::clamp(zoom * (io.MouseWheel > 0.0f ? 1.25f : 0.8f), 1.0f, 16.0f);
			return canvas;
		}

		void EndCanvas()
		{
			ImGui::EndChild();
		}

		/* @brief A texture coordinate on a whole pixel (when snapping and the size is known). */
		float Snap(const float value, const float size, const bool snap)
		{
			const float clamped = std::clamp(value, 0.0f, 1.0f);
			return snap && size > 0.0f ? XPlane::FacadeFromPixels(std::round(clamped * size), size) : clamped;
		}

		/* @brief An edge in pixels (fractions without a texture size) as an editable number; true when changed. */
		bool PixelField(const char *id, float &value, const float size)
		{
			if (size <= 0.0f)
				return ImGui::DragFloat(id, &value, 0.001f, 0.0f, 1.0f, "%.4f");
			float pixels = value * size;
			if (!ImGui::DragFloat(id, &pixels, 1.0f, 0.0f, size, "%.0f px"))
				return false;
			value = XPlane::FacadeFromPixels(std::round(std::clamp(pixels, 0.0f, size)), size);
			return true;
		}

		std::string Meters(const double meters)
		{
			char text[32];
			snprintf(text, sizeof(text), "%.2f m", meters);
			return text;
		}

		/**
		 * @brief Items laid out in rows that wrap in a narrow panel: Next() before each item puts it beside the previous
		 * one when it still fits, else it starts the next row.
		 */
		class FlowRow
		{
		public:
			FlowRow() : m_Right(ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x) {}

			void Next(const float width)
			{
				if (m_First)
				{
					m_First = false;
					return;
				}
				if (ImGui::GetItemRectMax().x + ImGui::GetStyle().ItemSpacing.x + width <= m_Right)
					ImGui::SameLine();
			}

			static float ButtonWidth(const std::string &label) { return ImGui::CalcTextSize(label.c_str(), nullptr, true).x + ImGui::GetStyle().FramePadding.x * 2.0f; }
			static float CheckboxWidth(const char *label) { return ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x + ImGui::CalcTextSize(label, nullptr, true).x; }

		private:
			float m_Right;
			bool m_First = true;
		};

		std::string JoinLengths(const std::vector<float> &lengths)
		{
			std::string text;
			for (const float length : lengths)
			{
				char value[24];
				snprintf(value, sizeof(value), "%s%g", text.empty() ? "" : " ", static_cast<double>(length));
				text += value;
			}
			return text;
		}
	}

	// -------------------------------------------------------

	FacadeEditorPanel::FacadeEditorPanel(EditorContext *context) : m_Context(context)
	{
		m_Title = "FACADE EDITOR";
	}

	const FacadeEditorPanel::CanvasTexture &FacadeEditorPanel::GetTexture(const std::string &reference)
	{
		static const CanvasTexture none;
		const std::filesystem::path file = FacadeAuthoring::ResolveTexture(reference);
		if (file.empty())
			return none;
		const auto [it, added] = m_Textures.try_emplace(VFS::ToUtf8(file));
		if (added)
		{
			XPlane::TextureInfo info;
			it->second.image = XPlane::TextureLoader::Load(file, XPlane::TextureUsage::Albedo, &info);
			it->second.width = static_cast<int>(info.width);
			it->second.height = static_cast<int>(info.height);
		}
		return it->second;
	}

	void FacadeEditorPanel::OnUIRender(bool &isOpen)
	{
		if (EditorUI::BeginPanel("FACADE EDITOR###FacadeEditor", &isOpen))
		{
			// The panel's own history while it has focus (Ctrl+Z anywhere else undoes the project; a text field being typed in undoes its text)
			if (FacadeAuthoring::GetKind() != FacadeAuthoring::Kind::None)
			{
				if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Z, ImGuiInputFlags_Repeat))
					UndoStep(false);
				if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Y, ImGuiInputFlags_Repeat) || ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z, ImGuiInputFlags_Repeat))
					UndoStep(true);
			}
			DrawHeader();
			switch (FacadeAuthoring::GetKind())
			{
				case FacadeAuthoring::Kind::Building:	DrawBuilding(); break;
				case FacadeAuthoring::Kind::Curb:		DrawCurb(); break;
				case FacadeAuthoring::Kind::Type2:		DrawType2(); break;
				default: break;
			}
		}
		EditorUI::EndPanel();
	}

	void FacadeEditorPanel::UndoStep(const bool redo)
	{
		if (redo ? FacadeAuthoring::Redo() : FacadeAuthoring::Undo())
		{
			m_Status = redo ? "Redone" : "Undone";
			m_DragCut = -1.0f;
			m_Dragging = false;
			m_ProfilePoint = -1;
		}
		else
			m_Status = redo ? "Nothing to redo" : "Nothing to undo";
	}

	void FacadeEditorPanel::DrawHeader()
	{
		const bool project = EditorProjects::GetDocument() != nullptr;
		ImGui::BeginDisabled(!project);
		FlowRow row;
		const std::string building = EditorUI::IconLabel(SEDX_ICON_PICTURE_O, "New building");
		row.Next(FlowRow::ButtonWidth(building));
		if (ImGui::Button(building.c_str()))
			EditorCommands::Run("facade.author.building", m_Context);
		EditorUI::ItemTooltip("A type 1 facade from a texture: mark its panels and floors on it");
		const std::string curb = EditorUI::IconLabel(SEDX_ICON_MAGIC, "New sidewalk / curb");
		row.Next(FlowRow::ButtonWidth(curb));
		if (ImGui::Button(curb.c_str()))
			EditorCommands::Run("facade.author.curb", m_Context);
		EditorUI::ItemTooltip("A type 2 ground facade from a cross-section (3D sidewalks, curbs, planters, low walls)");
		const std::string edit = EditorUI::IconLabel(SEDX_ICON_PENCIL, "Edit selected");
		row.Next(FlowRow::ButtonWidth(edit));
		ImGui::BeginDisabled(!EditorCommands::IsEnabled("facade.author.edit", m_Context));
		if (ImGui::Button(edit.c_str()))
			EditorCommands::Run("facade.author.edit", m_Context);
		ImGui::EndDisabled();
		EditorUI::ItemTooltip("The selected (or armed) facade: this project's own in place, a library or other pack's as a copy with its textures (and a type 2's objects)");
		const std::string open = EditorUI::IconLabel(SEDX_ICON_FOLDER_OPEN, "Open");
		row.Next(FlowRow::ButtonWidth(open));
		if (ImGui::Button(open.c_str()))
			ImGui::OpenPopup("##OpenFacade");
		EditorUI::ItemTooltip("Open a facade of this project's facades folder");
		if (ImGui::BeginPopup("##OpenFacade"))
		{
			const std::vector<std::string> mine = FacadeAuthoring::GetProjectFacades();
			if (mine.empty())
				ImGui::TextDisabled("The project's facades folder has no facades yet");
			for (const std::string &resource : mine)
			{
				ImGui::BeginDisabled(!FacadeAuthoring::CanEdit(resource));
				if (ImGui::Selectable(resource.c_str(), resource == FacadeAuthoring::GetResource()))
				{
					std::string error;
					m_Status = FacadeAuthoring::Edit(resource, &error) ? "Editing " + resource : error;
				}
				ImGui::EndDisabled();
			}
			ImGui::EndPopup();
		}
		ImGui::EndDisabled();

		if (FacadeAuthoring::GetKind() == FacadeAuthoring::Kind::None)
		{
			EditorUI::EmptyState(project ? "Start a building from a texture or a sidewalk / curb from a cross-section, or edit a facade (the X-Plane library's are copied into the project with their textures and objects)"
				: "Open a project: new facades are saved in its facades folder");
			return;
		}

		// Name, file, save and preview
		if (!EditorUI::BeginPropertyGrid("##FacadeFile", 90.0f))
			return;
		EditorUI::PropertyLabel("Name");
		std::string name = FacadeAuthoring::GetName();
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::InputText("##Name", &name, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsNoBlank) || ImGui::IsItemDeactivatedAfterEdit())
			FacadeAuthoring::SetName(name);
		EditorUI::ItemTooltip("File name: the facade is saved as facades/<name>.fac in the project folder");
		EditorUI::PropertyLabel("File");
		const std::string resource = FacadeAuthoring::GetResource();
		ImGui::TextWrapped("%s%s", resource.c_str(), FacadeAuthoring::IsDirty() ? "  (unsaved)" : "");
		EditorUI::ItemTooltip(VFS::ToUtf8(FacadeAuthoring::GetFilePath()).c_str());
		EditorUI::EndPropertyGrid();

		FlowRow actions;
		const std::string save = EditorUI::IconLabel(SEDX_ICON_FLOPPY_O, "Save");
		actions.Next(FlowRow::ButtonWidth(save));
		if (ImGui::Button(save.c_str()))
		{
			std::string error;
			m_Status = FacadeAuthoring::Save(&error) ? "Saved " + resource : error;
		}
		EditorUI::ItemTooltip("Write the .fac (and redraw every facade using it)");
		const std::string undo = std::string(SEDX_ICON_UNDO) + "##Undo", redo = std::string(SEDX_ICON_REPEAT) + "##Redo";
		actions.Next(FlowRow::ButtonWidth(undo) + FlowRow::ButtonWidth(redo) + ImGui::GetStyle().ItemSpacing.x);
		ImGui::BeginDisabled(!FacadeAuthoring::CanUndo());
		if (ImGui::Button(undo.c_str()))
			UndoStep(false);
		ImGui::EndDisabled();
		EditorUI::ItemTooltip("Undo the last edit (Ctrl+Z while this panel has focus; the project's own history is separate)");
		ImGui::SameLine();
		ImGui::BeginDisabled(!FacadeAuthoring::CanRedo());
		if (ImGui::Button(redo.c_str()))
			UndoStep(true);
		ImGui::EndDisabled();
		EditorUI::ItemTooltip("Redo (Ctrl+Y)");
		const bool saved = std::filesystem::exists(FacadeAuthoring::GetFilePath());
		const std::string revert = EditorUI::IconLabel(SEDX_ICON_REFRESH, "Revert");
		actions.Next(FlowRow::ButtonWidth(revert));
		ImGui::BeginDisabled(!saved);
		if (ImGui::Button(revert.c_str()))
		{
			std::string error;
			m_Status = FacadeAuthoring::Revert(&error) ? "Reverted to the saved file" : error;
		}
		ImGui::EndDisabled();
		const std::string &preview = FacadeAuthoring::GetPreviewItem();
		const std::string previewLabel = EditorUI::IconLabel(SEDX_ICON_EYE, preview.empty() ? "Place preview" : "Show preview");
		actions.Next(FlowRow::ButtonWidth(previewLabel));
		if (ImGui::Button(previewLabel.c_str()))
		{
			if (preview.empty())
				m_Status = FacadeAuthoring::PlacePreview(m_Context).empty() ? "Could not place a preview (see the console)" : "Preview placed where the viewport looks";
			else
			{
				EditorSelection::Select(m_Context, EditorSelection::Target::ForItem(preview));
				EditorTools::FrameSelection();
			}
		}
		EditorUI::ItemTooltip(preview.empty() ? "Save, then drop a footprint built with this facade where the viewport looks (one undo step)"
			: "Select the preview footprint and frame it in the viewport");
		actions.Next(FlowRow::CheckboxWidth("Live"));
		ImGui::Checkbox("Live", &FacadeAuthoring::LiveSave());
		EditorUI::ItemTooltip("Save and redraw shortly after every change, so footprints using the facade update in the viewport (once it has been saved)");
		const std::string draw = EditorUI::IconLabel(SEDX_ICON_BUILDING_O, "Draw with it");
		actions.Next(FlowRow::ButtonWidth(draw));
		ImGui::BeginDisabled(!saved);
		if (ImGui::Button(draw.c_str()))
		{
			FacadeTool::SetResource(resource);
			EditorTools::SetTool(EditorTools::Tool::Facade);
		}
		EditorUI::ItemTooltip("Arm it in the facade tool (B) and draw footprints with it");
		const std::string inspect = EditorUI::IconLabel(SEDX_ICON_INFO_CIRCLE, "Inspect");
		actions.Next(FlowRow::ButtonWidth(inspect));
		if (ImGui::Button(inspect.c_str()))
			FacadeTool::Inspect(resource);
		ImGui::EndDisabled();

		if (!m_Status.empty())
			ImGui::TextDisabled("%s", m_Status.c_str());
		for (const std::string &problem : FacadeAuthoring::GetProblems())
			ImGui::TextColored(EditorTheme::ToStyle(EditorTheme::Palette::Warning), "%s %s", SEDX_ICON_EXCLAMATION_TRIANGLE, problem.c_str());
	}

	bool FacadeEditorPanel::TextureRow(const char *label, std::string &reference, const bool optional, const char *tooltip)
	{
		bool changed = false;
		ImGui::PushID(label);
		EditorUI::PropertyLabel(label);
		const float dpi = ImGui::GetStyle().FontScaleDpi;
		const float buttons = (optional && !reference.empty() ? 2.0f : 1.0f) * 30.0f * dpi;
		ImGui::SetNextItemWidth(-buttons);
		std::string shown = reference.empty() ? std::string(optional ? "(none)" : "(choose one)") : reference;
		ImGui::InputText("##Path", &shown, ImGuiInputTextFlags_ReadOnly);
		if (tooltip)
			EditorUI::ItemTooltip(tooltip);
		ImGui::SameLine(0.0f, 2.0f * dpi);
		if (ImGui::Button(SEDX_ICON_FOLDER_OPEN))
		{
			const std::filesystem::path file = FacadeAuthoring::BrowseTexture(label);
			if (!file.empty())
			{
				std::string error;
				const std::string imported = FacadeAuthoring::ImportTexture(file, &error);
				if (!imported.empty())
				{
					reference = imported;
					changed = true;
				}
				else
					m_Status = error;
			}
		}
		EditorUI::ItemTooltip("Choose a texture (copied into the facades folder when it is outside the project)");
		if (optional && !reference.empty())
		{
			ImGui::SameLine(0.0f, 2.0f * dpi);
			if (ImGui::Button(SEDX_ICON_TIMES))
			{
				reference.clear();
				changed = true;
			}
			EditorUI::ItemTooltip("Clear");
		}
		ImGui::PopID();
		return changed;
	}

	// -------------------------------------------------------
	// Type 1 building

	void FacadeEditorPanel::DrawBuilding()
	{
		FacadeAsset &asset = FacadeAuthoring::GetAsset();
		bool edited = false;
		const float dpi = ImGui::GetStyle().FontScaleDpi;

		if (EditorUI::SectionHeader("TEXTURES") && EditorUI::BeginPropertyGrid("##Textures", 90.0f))
		{
			if (TextureRow("Wall", asset.wallTexture, false, "The texture the walls (and, without a roof texture, the roof) are cut from"))
			{
				const CanvasTexture &texture = GetTexture(asset.wallTexture);
				if (texture.width > 0)
				{
					for (FacadeAsset::Wall &wall : asset.walls)
					{
						wall.textureWidth = static_cast<float>(texture.width);
						wall.textureHeight = static_cast<float>(texture.height);
					}
					if (asset.roofTexture.empty())
					{
						asset.roofTextureWidth = static_cast<float>(texture.width);
						asset.roofTextureHeight = static_cast<float>(texture.height);
					}
				}
				edited = true;
			}
			edited |= TextureRow("Normal map", asset.wallTextureNormal, true, "TEXTURE_NORMAL: the walls' normal map (X-Plane 12 lighting)");
			if (TextureRow("Roof", asset.roofTexture, true, "A separate roof texture (SHADER_ROOF); none: the roof is cut from the wall texture"))
			{
				const CanvasTexture &texture = GetTexture(asset.roofTexture.empty() ? asset.wallTexture : asset.roofTexture);
				asset.roofTextureWidth = static_cast<float>(std::max(0, texture.width));
				asset.roofTextureHeight = static_cast<float>(std::max(0, texture.height));
				edited = true;
			}
			EditorUI::EndPropertyGrid();
		}

		// Wall types
		if (asset.walls.empty())
			asset.walls.push_back(XPlane::MakeType1Wall(asset));
		m_Wall = std::clamp(m_Wall, 0, static_cast<int>(asset.walls.size()) - 1);
		if (EditorUI::SectionHeader("WALL TYPES"))
		{
			const std::vector<std::string> names = XPlane::GetFacadeWallNames(asset);
			ImGui::SetNextItemWidth(160.0f * dpi);
			if (ImGui::BeginCombo("##Wall", names[static_cast<size_t>(m_Wall)].c_str()))
			{
				for (size_t w = 0; w < names.size(); ++w)
				{
					if (ImGui::Selectable((std::to_string(w + 1) + "  " + names[w]).c_str(), static_cast<int>(w) == m_Wall))
						m_Wall = static_cast<int>(w);
					EditorUI::ItemTooltip(XPlane::DescribeFacadeWall(asset, static_cast<int>(w)).c_str());
				}
				ImGui::EndCombo();
			}
			ImGui::SameLine();
			if (ImGui::Button(SEDX_ICON_PLUS))
			{
				asset.walls.push_back(XPlane::MakeType1Wall(asset));
				m_Wall = static_cast<int>(asset.walls.size()) - 1;
				edited = true;
			}
			EditorUI::ItemTooltip("Add a wall type (a copy of the last): narrow walls, gable ends, a front with a door... X-Plane picks one per wall by its width and heading rules");
			ImGui::SameLine();
			ImGui::BeginDisabled(asset.walls.size() < 2);
			if (ImGui::Button(SEDX_ICON_TRASH))
			{
				asset.walls.erase(asset.walls.begin() + m_Wall);
				m_Wall = std::max(0, m_Wall - 1);
				edited = true;
			}
			ImGui::EndDisabled();
			EditorUI::ItemTooltip("Remove this wall type");
			ImGui::SameLine();
			ImGui::BeginDisabled(m_Wall == 0);
			if (ImGui::Button(SEDX_ICON_ARROW_UP))
			{
				std::swap(asset.walls[static_cast<size_t>(m_Wall)], asset.walls[static_cast<size_t>(m_Wall - 1)]);
				--m_Wall;
				edited = true;
			}
			ImGui::EndDisabled();
			EditorUI::ItemTooltip("Move up: X-Plane takes the first wall type whose rules accept a wall");

			FacadeAsset::Wall &wall = asset.walls[static_cast<size_t>(m_Wall)];
			if (wall.filters.empty())
				wall.filters.push_back({ wall.minWidth, wall.maxWidth, 0.0f, 0.0f });
			FacadeAsset::WallFilter &rule = wall.filters.front();
			if (EditorUI::BeginPropertyGrid("##WallType", 110.0f))
			{
				EditorUI::PropertyLabel("Name");
				ImGui::SetNextItemWidth(-FLT_MIN);
				edited |= ImGui::InputText("##WallName", &wall.name);
				EditorUI::PropertyLabel("Widths");
				ImGui::SetNextItemWidth(-FLT_MIN);
				float widths[2] = { rule.minWidth, rule.maxWidth };
				if (ImGui::DragFloat2("##Widths", widths, 0.1f, 0.0f, 9999.0f, "%.1f m"))
				{
					rule.minWidth = wall.minWidth = std::max(0.0f, widths[0]);
					rule.maxWidth = wall.maxWidth = std::max(rule.minWidth, widths[1]);
					edited = true;
				}
				EditorUI::ItemTooltip("Walls this wide (meters) can get this wall type");
				EditorUI::PropertyLabel("Headings");
				ImGui::SetNextItemWidth(-FLT_MIN);
				float headings[2] = { rule.minHeading, rule.maxHeading };
				if (ImGui::DragFloat2("##Headings", headings, 1.0f, 0.0f, 360.0f, "%.0f deg"))
				{
					rule.minHeading = std::clamp(headings[0], 0.0f, 360.0f);
					rule.maxHeading = std::clamp(headings[1], 0.0f, 360.0f);
					edited = true;
				}
				EditorUI::ItemTooltip("Facing these degrees from the front wall, clockwise (equal: any heading). 270 - 90 is the front half");
				if (wall.filters.size() > 1)
				{
					EditorUI::PropertyLabel("More rules");
					ImGui::Text("%zu WALL_RULE rows (kept)", wall.filters.size() - 1);
				}

				EditorUI::PropertyLabel("Texture size");
				ImGui::SetNextItemWidth(-FLT_MIN);
				float meters[2] = { wall.scaleS, wall.scaleT };
				if (ImGui::DragFloat2("##Scale", meters, 0.05f, 0.1f, 1000.0f, "%.2f m"))
				{
					wall.scaleS = std::max(0.1f, meters[0]);
					wall.scaleT = std::max(0.1f, meters[1]);
					if (m_SquarePixels && wall.textureWidth > 0.0f && wall.textureHeight > 0.0f)
					{
						if (meters[0] != wall.scaleS || std::abs(meters[1] - wall.scaleT) < 1e-6f)
							wall.scaleT = wall.scaleS * wall.textureHeight / wall.textureWidth;
						else
							wall.scaleS = wall.scaleT * wall.textureWidth / wall.textureHeight;
					}
					edited = true;
				}
				EditorUI::ItemTooltip("SCALE: meters the whole texture covers across and up");
				EditorUI::PropertyLabel("");
				ImGui::Checkbox("Square pixels", &m_SquarePixels);
				ImGui::SameLine();
				ImGui::BeginDisabled(wall.textureWidth <= 0.0f);
				if (ImGui::Button("Apply to all walls"))
				{
					for (FacadeAsset::Wall &other : asset.walls)
					{
						other.scaleS = wall.scaleS;
						other.scaleT = wall.scaleT;
					}
					edited = true;
				}
				ImGui::EndDisabled();

				EditorUI::PropertyLabel("Roof slope");
				ImGui::SetNextItemWidth(100.0f * dpi);
				edited |= ImGui::DragFloat("##Slope", &wall.roofSlope, 0.5f, -89.0f, 89.0f, "%.1f deg");
				ImGui::SameLine();
				edited |= ImGui::Checkbox("Slant", &wall.slant);
				EditorUI::ItemTooltip("ROOF_SLOPE: the top floor leans in by this angle; Slant: the sloped row shortens instead of stretching");
				EditorUI::EndPropertyGrid();
			}

			// What the panels make
			double fixedWidth = 0.0, centerWidth = 0.0;
			for (const auto &panel : wall.leftPanels) fixedWidth += (panel.second - panel.first) * wall.scaleS;
			for (const auto &panel : wall.rightPanels) fixedWidth += (panel.second - panel.first) * wall.scaleS;
			for (const auto &panel : wall.centerPanels) centerWidth += (panel.second - panel.first) * wall.scaleS;
			ImGui::TextDisabled("Ends %s, repeating %s across; %s", Meters(fixedWidth).c_str(), Meters(centerWidth).c_str(), XPlane::DescribeFacadeHeights(XPlane::GetFacadeHeights(asset)).c_str());

			DrawWallCanvas(wall);
			DrawPanelRows(wall);
		}

		if (EditorUI::SectionHeader("ROOF"))
			DrawRoofCanvas();

		if (EditorUI::SectionHeader("BUILDING", false) && EditorUI::BeginPropertyGrid("##Building", 110.0f))
		{
			EditorUI::PropertyLabel("Footprint");
			int ring = asset.ring ? 0 : 1;
			if (ImGui::Combo("##Ring", &ring, "Closed (a building)\0Open (a wall run)\0"))
			{
				asset.ring = ring == 0;
				edited = true;
			}
			EditorUI::PropertyLabel("Floors");
			ImGui::SetNextItemWidth(-FLT_MIN);
			int floors[2] = { asset.floorsMin, asset.floorsMax };
			if (ImGui::DragInt2("##Floors", floors, 0.2f, 1, 999))
			{
				asset.floorsMin = std::clamp(floors[0], 1, 999);
				asset.floorsMax = std::clamp(floors[1], asset.floorsMin, 999);
				edited = true;
			}
			EditorUI::ItemTooltip("FLOORS_MIN / FLOORS_MAX: how many floors X-Plane may build");
			EditorUI::PropertyLabel("Options");
			edited |= ImGui::Checkbox("Graded", &asset.graded);
			EditorUI::ItemTooltip("Flat on its first wall instead of draped on the terrain");
			ImGui::SameLine();
			edited |= ImGui::Checkbox("Two-sided", &asset.twoSided);
			ImGui::SameLine();
			edited |= ImGui::Checkbox("Doubled", &asset.doubled);
			EditorUI::ItemTooltip("DOUBLED: walls built inside and out (fences)");
			EditorUI::PropertyLabel("Drawn to");
			ImGui::SetNextItemWidth(-FLT_MIN);
			edited |= ImGui::DragFloat("##Lod", &asset.lodFar, 50.0f, 100.0f, 100000.0f, "%.0f m");
			EditorUI::PropertyLabel("Hard surfaces");
			ImGui::SetNextItemWidth(110.0f * dpi);
			edited |= ImGui::InputTextWithHint("##HardWall", "walls", &asset.hardWall, ImGuiInputTextFlags_CharsNoBlank);
			ImGui::SameLine();
			ImGui::SetNextItemWidth(110.0f * dpi);
			edited |= ImGui::InputTextWithHint("##HardRoof", "roof", &asset.hardRoof, ImGuiInputTextFlags_CharsNoBlank);
			EditorUI::ItemTooltip("HARD_WALL / HARD_ROOF: concrete, asphalt, metal... what aircraft and vehicles feel; empty for none");
			EditorUI::EndPropertyGrid();
		}

		if (edited)
			FacadeAuthoring::MarkEdited();
	}

	void FacadeEditorPanel::DrawWallCanvas(FacadeAsset::Wall &wall)
	{
		FacadeAsset &asset = FacadeAuthoring::GetAsset();
		const float zoomWidth = 90.0f * ImGui::GetStyle().FontScaleDpi;
		FlowRow toolbar;
		toolbar.Next(FlowRow::CheckboxWidth("Panels across"));
		ImGui::RadioButton("Panels across", &m_Mode, 0);
		toolbar.Next(FlowRow::CheckboxWidth("Floors up"));
		ImGui::RadioButton("Floors up", &m_Mode, 1);
		toolbar.Next(FlowRow::CheckboxWidth("Snap to pixels"));
		ImGui::Checkbox("Snap to pixels", &m_Snap);
		EditorUI::ItemTooltip("Edges land on whole texture pixels (hold Alt to place them freely)");
		toolbar.Next(zoomWidth);
		ImGui::SetNextItemWidth(zoomWidth);
		ImGui::SliderFloat("##Zoom", &m_Zoom, 1.0f, 16.0f, "Zoom %.1fx", ImGuiSliderFlags_Logarithmic);
		EditorUI::ItemTooltip("Zoom (Ctrl + wheel over the texture)");
		if (m_Mode > 1)
			m_Mode = 0;
		const bool floors = m_Mode == 1;

		const CanvasTexture &texture = GetTexture(asset.wallTexture);
		const Canvas canvas = BeginCanvas("##WallCanvas", texture, m_Zoom);
		ImDrawList *draw = ImGui::GetWindowDrawList();
		const float size = floors ? wall.textureHeight : wall.textureWidth;
		const bool snap = m_Snap && !ImGui::GetIO().KeyAlt;

		// Both axes' panels: this mode's as colored bands, the other's as faint lines
		for (const bool vertical : { !floors, floors })
		{
			const bool current = vertical == floors;
			for (const FacadePanelRow row : vertical ? UP : ACROSS)
			{
				int index = 0;
				for (const auto &[a, b] : XPlane::GetFacadePanels(wall, row))
				{
					++index;
					const ImVec2 p0 = vertical ? canvas.ToScreen(0.0f, b) : canvas.ToScreen(a, 1.0f);
					const ImVec2 p1 = vertical ? canvas.ToScreen(1.0f, a) : canvas.ToScreen(b, 0.0f);
					if (!current)
					{
						draw->AddRect(p0, p1, EditorTheme::ToU32(ImVec4(1.0f, 1.0f, 1.0f, 0.18f)));
						continue;
					}
					draw->AddRectFilled(p0, p1, RowColor(row, 0.18f));
					draw->AddRect(p0, p1, RowColor(row, 0.9f), 0.0f, 0, 1.5f);
					const std::string label = std::string(1, XPlane::GetFacadePanelRowName(row)[0]) + std::to_string(index);
					const ImVec2 at = vertical ? ImVec2(p0.x + 4.0f, (p0.y + p1.y) * 0.5f - 7.0f) : ImVec2((p0.x + p1.x) * 0.5f - 7.0f, p0.y + 4.0f);
					draw->AddRectFilled(ImVec2(at.x - 2.0f, at.y - 1.0f), ImVec2(at.x + ImGui::CalcTextSize(label.c_str()).x + 2.0f, at.y + ImGui::GetTextLineHeight()), RowColor(row, 0.85f), 2.0f);
					draw->AddText(at, EditorTheme::ToU32(EditorTheme::Palette::Text), label.c_str());
				}
			}
		}

		// Edges: hovered ones highlight, dragging moves every panel sharing it
		const ImVec2 mouse = ImGui::GetMousePos();
		float hovered = -1.0f;
		for (const float cut : XPlane::GetFacadeTextureCuts(wall, floors))
		{
			const ImVec2 at = canvas.ToScreen(cut, cut);
			const float distance = floors ? std::abs(mouse.y - at.y) : std::abs(mouse.x - at.x);
			const bool grab = canvas.hovered && distance <= EDGE_GRAB && m_DragCut < 0.0f;
			if (grab)
				hovered = cut;
			const bool hot = grab || (m_DragCut >= 0.0f && std::abs(cut - m_DragCut) < 1e-6f);
			const ImU32 color = hot ? EditorTheme::ToU32(EditorTheme::Palette::AccentHovered) : EditorTheme::ToU32(ImVec4(1.0f, 1.0f, 1.0f, 0.75f));
			if (floors)
				draw->AddLine(ImVec2(canvas.min.x, at.y), ImVec2(canvas.max.x, at.y), color, hot ? 2.5f : 1.0f);
			else
				draw->AddLine(ImVec2(at.x, canvas.min.y), ImVec2(at.x, canvas.max.y), color, hot ? 2.5f : 1.0f);
		}
		if (hovered >= 0.0f || m_DragCut >= 0.0f)
			ImGui::SetMouseCursor(floors ? ImGuiMouseCursor_ResizeNS : ImGuiMouseCursor_ResizeEW);
		if (canvas.activated && hovered >= 0.0f)
			m_DragCut = hovered;
		if (m_DragCut >= 0.0f)
		{
			if (canvas.active)
			{
				const float target = Snap(floors ? canvas.ToT(mouse.y) : canvas.ToS(mouse.x), size, snap);
				const float moved = XPlane::MoveFacadeTextureCut(wall, floors, m_DragCut, target, size > 0.0f ? 1.0f / size : 1.0f / 4096.0f);
				if (moved != m_DragCut)
				{
					m_DragCut = moved;
					FacadeAuthoring::MarkEdited();
				}
				ImGui::SetTooltip(size > 0.0f ? "%.0f px" : "%.4f", static_cast<double>(size > 0.0f ? m_DragCut * size : m_DragCut));
			}
			else
				m_DragCut = -1.0f;
		}
		else if (canvas.hovered)
		{
			const float s = canvas.ToS(mouse.x), t = canvas.ToT(mouse.y);
			if (wall.textureWidth > 0.0f)
				ImGui::SetTooltip("%.0f, %.0f px", static_cast<double>(s * wall.textureWidth), static_cast<double>(t * wall.textureHeight));
		}
		EndCanvas();
		ImGui::TextDisabled("Drag an edge to move it (panels sharing it follow). Ctrl + wheel zooms.");
	}

	void FacadeEditorPanel::DrawPanelRows(FacadeAsset::Wall &wall)
	{
		const bool floors = m_Mode == 1;
		bool edited = false;
		const float size = floors ? wall.textureHeight : wall.textureWidth;
		const float dpi = ImGui::GetStyle().FontScaleDpi;
		constexpr ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit;
		if (!ImGui::BeginTable("##PanelRows", 4, flags))
			return;
		ImGui::TableSetupColumn("Panel");
		ImGui::TableSetupColumn("From", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("To", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("");
		ImGui::TableHeadersRow();
		for (const FacadePanelRow row : floors ? UP : ACROSS)
		{
			std::vector<std::pair<float, float>> &panels = XPlane::GetFacadePanels(wall, row);
			ImGui::PushID(static_cast<int>(row));
			for (size_t i = 0; i < panels.size(); ++i)
			{
				ImGui::PushID(static_cast<int>(i));
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(RowColor(row, 1.0f)), "%s %zu", XPlane::GetFacadePanelRowName(row), i + 1);
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(-FLT_MIN);
				edited |= PixelField("##From", panels[i].first, size);
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(-FLT_MIN);
				edited |= PixelField("##To", panels[i].second, size);
				ImGui::TableNextColumn();
				if (ImGui::SmallButton(SEDX_ICON_SCISSORS))
					edited |= XPlane::SplitFacadePanel(wall, row, i);
				EditorUI::ItemTooltip("Split in two");
				ImGui::SameLine(0.0f, 2.0f * dpi);
				if (ImGui::SmallButton(SEDX_ICON_TIMES))
				{
					panels.erase(panels.begin() + static_cast<std::ptrdiff_t>(i));
					edited = true;
					ImGui::PopID();
					break;
				}
				EditorUI::ItemTooltip("Remove");
				ImGui::PopID();
			}
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			if (ImGui::SmallButton((std::string(SEDX_ICON_PLUS) + " " + XPlane::GetFacadePanelRowName(row)).c_str()))
			{
				// After the last edge of the axis, a tenth of the texture wide
				const std::vector<float> cuts = XPlane::GetFacadeTextureCuts(wall, floors);
				const float from = cuts.empty() ? 0.0f : std::min(cuts.back(), 0.9f);
				panels.emplace_back(Snap(from, size, m_Snap), Snap(std::min(1.0f, from + 0.1f), size, m_Snap));
				edited = true;
			}
			EditorUI::ItemTooltip(floors ? "Add a floor of this row (bottom: once at the ground, middle: repeated, top: once under the roof)"
				: "Add a panel of this row (left / right: once at each end, center: repeated to fill the wall)");
			ImGui::PopID();
		}
		ImGui::EndTable();

		if (ImGui::Button("Snap all to pixels"))
		{
			XPlane::SnapFacadeWallToPixels(wall);
			edited = true;
		}
		EditorUI::ItemTooltip("Round every edge of this wall type to whole texture pixels");
		if (edited)
			FacadeAuthoring::MarkEdited();
	}

	void FacadeEditorPanel::DrawRoofCanvas()
	{
		FacadeAsset &asset = FacadeAuthoring::GetAsset();
		bool edited = false;
		const float dpi = ImGui::GetStyle().FontScaleDpi;
		int mode = !asset.hasRoof ? 0 : asset.roofScaled ? 2 : 1;
		ImGui::RadioButton("No roof", &mode, 0);
		ImGui::SameLine();
		ImGui::RadioButton("Stretched", &mode, 1);
		ImGui::SameLine();
		ImGui::RadioButton("Repeated", &mode, 2);
		EditorUI::ItemTooltip("Stretched: the rectangle covers the whole roof (ROOF corners). Repeated: it covers so many meters and repeats (ROOF_SCALE)");
		const std::pair<float, float> meters = XPlane::GetFacadeRoofMeters(asset);
		const float s1 = asset.roofUv.size() == 4 ? asset.roofUv[0].first : 0.0f, t1 = asset.roofUv.size() == 4 ? asset.roofUv[0].second : 0.0f;
		const float s2 = asset.roofUv.size() == 4 ? asset.roofUv[2].first : 1.0f, t2 = asset.roofUv.size() == 4 ? asset.roofUv[2].second : 1.0f;
		const int current = !asset.hasRoof ? 0 : asset.roofScaled ? 2 : 1;
		if (mode != current)
		{
			if (mode == 0)
			{
				asset.hasRoof = false;
				asset.roofUv.clear();
				asset.roofScaled = false;
			}
			else if (mode == 1)
				XPlane::SetFacadeRoofRectangle(asset, s1, t1, s2, t2);
			else
				XPlane::SetFacadeRoofScaled(asset, s1, t1, s2, t2, 10.0f, 10.0f);
			edited = true;
		}
		if (mode == 2)
		{
			ImGui::SetNextItemWidth(180.0f * dpi);
			float size[2] = { meters.first, meters.second };
			if (ImGui::DragFloat2("Meters", size, 0.1f, 0.1f, 1000.0f, "%.1f m"))
			{
				XPlane::SetFacadeRoofScaled(asset, s1, t1, s2, t2, size[0], size[1]);
				edited = true;
			}
			EditorUI::ItemTooltip("Meters of roof the rectangle covers before it repeats");
		}
		if (mode == 0)
		{
			if (edited)
				FacadeAuthoring::MarkEdited();
			return;
		}

		// Drag a rectangle on the roof texture
		const std::string &reference = asset.roofTexture.empty() ? asset.wallTexture : asset.roofTexture;
		const CanvasTexture &texture = GetTexture(reference);
		const Canvas canvas = BeginCanvas("##RoofCanvas", texture, m_Zoom);
		ImDrawList *draw = ImGui::GetWindowDrawList();
		const bool snap = m_Snap && !ImGui::GetIO().KeyAlt;
		const ImVec2 mouse = ImGui::GetMousePos();
		const float w = asset.roofTextureWidth, h = asset.roofTextureHeight;
		if (canvas.activated)
		{
			m_Dragging = true;
			m_DragStart[0] = Snap(canvas.ToS(mouse.x), w, snap);
			m_DragStart[1] = Snap(canvas.ToT(mouse.y), h, snap);
		}
		if (m_Dragging && canvas.active)
		{
			const float s = Snap(canvas.ToS(mouse.x), w, snap), t = Snap(canvas.ToT(mouse.y), h, snap);
			const float a = std::min(m_DragStart[0], s), b = std::max(m_DragStart[0], s), c = std::min(m_DragStart[1], t), d = std::max(m_DragStart[1], t);
			if (b > a && d > c)
			{
				if (asset.roofScaled)
					XPlane::SetFacadeRoofScaled(asset, a, c, b, d, meters.first, meters.second);
				else
					XPlane::SetFacadeRoofRectangle(asset, a, c, b, d);
				edited = true;
			}
		}
		else
			m_Dragging = false;
		if (asset.roofUv.size() == 4)
		{
			const ImVec2 p0 = canvas.ToScreen(asset.roofUv[0].first, asset.roofUv[2].second), p1 = canvas.ToScreen(asset.roofUv[2].first, asset.roofUv[0].second);
			draw->AddRectFilled(p0, p1, EditorTheme::ToU32(ImVec4(0.929f, 0.62f, 0.259f, 0.18f)));
			draw->AddRect(p0, p1, EditorTheme::ToU32(ImVec4(0.929f, 0.62f, 0.259f, 1.0f)), 0.0f, 0, 2.0f);
			if (asset.roofScaled)
			{
				const ImVec2 center = canvas.ToScreen(asset.roofCenter.first, asset.roofCenter.second);
				draw->AddCircle(center, 4.0f, EditorTheme::ToU32(ImVec4(0.929f, 0.62f, 0.259f, 1.0f)));
			}
		}
		EndCanvas();
		ImGui::TextDisabled("Drag a rectangle over the part of the texture the roof uses");
		if (edited)
			FacadeAuthoring::MarkEdited();
	}

	// -------------------------------------------------------
	// Type 2 sidewalks and curbs

	void FacadeEditorPanel::DrawCurb()
	{
		XPlane::CurbFacadeSpec &spec = FacadeAuthoring::GetCurbSpec();
		bool edited = false;
		const float dpi = ImGui::GetStyle().FontScaleDpi;

		if (EditorUI::SectionHeader("CROSS-SECTION"))
		{
			ImGui::SetNextItemWidth(200.0f * dpi);
			if (ImGui::BeginCombo("##Preset", "Start from a preset..."))
			{
				for (const XPlane::CurbProfile &preset : XPlane::GetCurbProfilePresets())
				{
					if (ImGui::Selectable(preset.name.c_str()))
					{
						spec.profile = preset.points;
						edited = true;
					}
				}
				ImGui::EndCombo();
			}
			ImGui::SameLine();
			edited |= ImGui::Checkbox("Closed (sidewalk)", &spec.ring);
			EditorUI::ItemTooltip("Closed: a sidewalk or planter around an area (its roof is the walking surface). Open: a curb or wall along a line");
			DrawProfile(spec);
			if (m_ProfilePoint >= 0)
				edited = true;
		}

		if (EditorUI::SectionHeader("TEXTURES") && EditorUI::BeginPropertyGrid("##CurbTextures", 100.0f))
		{
			edited |= TextureRow("Curb", spec.wallTexture, false, "The texture the curb's strip is cut from");
			edited |= TextureRow("Curb normal", spec.wallTextureNormal, true, "The curb's normal map");
			edited |= TextureRow("Surface", spec.roofTexture, true, "The walking surface (the roof); none: the curb texture");
			edited |= TextureRow("Surface normal", spec.roofTextureNormal, true, "The walking surface's normal map");
			EditorUI::PropertyLabel("Surface repeat");
			ImGui::SetNextItemWidth(-FLT_MIN);
			edited |= ImGui::DragFloat("##RoofScale", &spec.roofScale, 0.1f, 0.5f, 500.0f, "%.1f m");
			EditorUI::PropertyLabel("Hard surface");
			ImGui::SetNextItemWidth(-FLT_MIN);
			edited |= ImGui::InputTextWithHint("##HardRoof", "none", &spec.hardRoof, ImGuiInputTextFlags_CharsNoBlank);
			EditorUI::ItemTooltip("HARD_ROOF: concrete, asphalt... what aircraft and vehicles feel on the walking surface");
			EditorUI::PropertyLabel("Strip length");
			ImGui::SetNextItemWidth(-FLT_MIN);
			edited |= ImGui::DragFloat("##TextureLength", &spec.textureLength, 0.05f, 0.1f, 100.0f, "%.2f m");
			EditorUI::ItemTooltip("Meters of curb the strip covers along it before repeating");
			EditorUI::EndPropertyGrid();
		}
		if (!spec.wallTexture.empty())
			DrawStripCanvas(spec);

		if (EditorUI::SectionHeader("SEGMENTS") && EditorUI::BeginPropertyGrid("##Segments", 100.0f))
		{
			EditorUI::PropertyLabel("Wall name");
			ImGui::SetNextItemWidth(-FLT_MIN);
			edited |= ImGui::InputText("##WallName", &spec.wallName);
			EditorUI::PropertyLabel("Lengths");
			if (!m_LengthsEditing)
				m_LengthsText = JoinLengths(spec.segmentLengths);
			ImGui::SetNextItemWidth(-FLT_MIN);
			const bool entered = ImGui::InputText("##Lengths", &m_LengthsText, ImGuiInputTextFlags_EnterReturnsTrue);
			m_LengthsEditing = ImGui::IsItemActive();
			if (entered || ImGui::IsItemDeactivatedAfterEdit())
			{
				std::vector<float> lengths;
				std::istringstream stream(m_LengthsText);
				float length = 0.0f;
				while (stream >> length)
				{
					if (length >= 0.05f && length <= 100.0f)
						lengths.push_back(length);
				}
				if (!lengths.empty())
				{
					spec.segmentLengths = lengths;
					edited = true;
				}
				m_LengthsText = JoinLengths(spec.segmentLengths);
			}
			EditorUI::ItemTooltip("Segment lengths in meters, one segment (and curved variant) each");
			EditorUI::PropertyLabel("Spell up to");
			ImGui::SetNextItemWidth(-FLT_MIN);
			edited |= ImGui::DragFloat("##SpellingLength", &spec.spellingLength, 0.25f, 0.5f, 200.0f, "%.2f m");
			EditorUI::ItemTooltip("Spellings fill every multiple of the shortest segment up to this length; longer walls repeat them");
			EditorUI::PropertyLabel("Curve pieces");
			ImGui::SetNextItemWidth(-FLT_MIN);
			edited |= ImGui::DragFloat("##CurvedPiece", &spec.curvedPiece, 0.01f, 0.1f, 5.0f, "%.2f m");
			EditorUI::ItemTooltip("SEGMENT_CURVED variants bend in pieces this long (the 3D sidewalk article's 0.5 - 0.8 m)");
			EditorUI::PropertyLabel("Drawn to");
			ImGui::SetNextItemWidth(-FLT_MIN);
			edited |= ImGui::DragFloat("##Lod", &spec.lodFar, 50.0f, 100.0f, 50000.0f, "%.0f m");
			EditorUI::EndPropertyGrid();

			const FacadeAsset &asset = FacadeAuthoring::GetAsset();
			if (!asset.floors.empty() && !asset.floors[0].walls.empty())
			{
				size_t triangles = 0;
				for (const FacadeAsset::Segment &segment : asset.floors[0].segments)
					for (const FacadeAsset::Mesh &mesh : segment.meshes)
						triangles += mesh.indices.size() / 3;
				ImGui::TextDisabled("%zu segments (+ curved), %zu spellings, %zu triangles straight; roof at %s", asset.floors[0].segments.size(),
					asset.floors[0].walls[0].spellings.size(), triangles, Meters(asset.floors[0].height).c_str());
			}
		}

		if (edited)
			FacadeAuthoring::MarkEdited();
	}

	void FacadeEditorPanel::DrawProfile(XPlane::CurbFacadeSpec &spec)
	{
		const float dpi = ImGui::GetStyle().FontScaleDpi;
		std::vector<std::pair<float, float>> &points = spec.profile;
		if (points.size() < 2)
			points = XPlane::GetCurbProfilePresets().front().points;

		// The view: meters to pixels at one scale, a margin round the points, out to the right, up up
		float minX = 0.0f, maxX = 0.2f, minY = -0.1f, maxY = 0.2f;
		for (const auto &[x, y] : points)
		{
			minX = std::min(minX, x);
			maxX = std::max(maxX, x);
			minY = std::min(minY, y);
			maxY = std::max(maxY, y);
		}
		const float margin = 0.1f * std::max(maxX - minX, maxY - minY) + 0.05f;
		minX -= margin;
		maxX += margin;
		minY -= margin;
		maxY += margin;
		const ImVec2 size(std::max(120.0f, ImGui::GetContentRegionAvail().x), 170.0f * dpi);
		const float scale = std::min(size.x / (maxX - minX), size.y / (maxY - minY));
		const ImVec2 origin = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton("##Profile", size);
		const bool hovered = ImGui::IsItemHovered(), active = ImGui::IsItemActive(), activated = ImGui::IsItemActivated();
		const ImVec2 center(origin.x + size.x * 0.5f, origin.y + size.y * 0.5f);
		const float midX = 0.5f * (minX + maxX), midY = 0.5f * (minY + maxY);
		const auto toScreen = [&](const float x, const float y) { return ImVec2(center.x + (x - midX) * scale, center.y - (y - midY) * scale); };
		const auto toMeters = [&](const ImVec2 &p) { return std::pair<float, float>{ midX + (p.x - center.x) / scale, midY - (p.y - center.y) / scale }; };

		ImDrawList *draw = ImGui::GetWindowDrawList();
		draw->PushClipRect(origin, ImVec2(origin.x + size.x, origin.y + size.y), true);
		draw->AddRectFilled(origin, ImVec2(origin.x + size.x, origin.y + size.y), EditorTheme::ToU32(EditorTheme::Palette::Frame));
		// Ground (y 0) and the walking surface (the first point's height), the footprint's edge (out 0)
		const ImU32 guide = EditorTheme::ToU32(ImVec4(1.0f, 1.0f, 1.0f, 0.25f));
		draw->AddLine(toScreen(minX, 0.0f), toScreen(maxX, 0.0f), EditorTheme::ToU32(ImVec4(0.47f, 0.4f, 0.3f, 0.9f)), 1.5f);
		draw->AddLine(toScreen(0.0f, minY), toScreen(0.0f, maxY), guide);
		draw->AddLine(toScreen(minX, points.front().second), toScreen(0.0f, points.front().second), EditorTheme::ToU32(EditorTheme::Palette::Accent), 2.0f);
		draw->AddText(ImVec2(origin.x + 4.0f, origin.y + 2.0f), EditorTheme::ToU32(EditorTheme::Palette::TextDisabled), "walking surface  |  out ->");
		for (size_t i = 0; i + 1 < points.size(); ++i)
			draw->AddLine(toScreen(points[i].first, points[i].second), toScreen(points[i + 1].first, points[i + 1].second), EditorTheme::ToU32(EditorTheme::Palette::Text), 2.0f);

		const ImVec2 mouse = ImGui::GetMousePos();
		int nearest = -1;
		for (size_t i = 0; i < points.size(); ++i)
		{
			const ImVec2 at = toScreen(points[i].first, points[i].second);
			const bool hot = (hovered && std::hypot(mouse.x - at.x, mouse.y - at.y) < 7.0f * dpi) || m_ProfilePoint == static_cast<int>(i);
			if (hot && nearest < 0)
				nearest = static_cast<int>(i);
			draw->AddCircleFilled(at, hot ? 5.0f * dpi : 3.5f * dpi, hot ? EditorTheme::ToU32(EditorTheme::Palette::AccentHovered) : EditorTheme::ToU32(EditorTheme::Palette::Warning));
		}
		draw->PopClipRect();

		if (activated && nearest >= 0)
			m_ProfilePoint = nearest;
		if (m_ProfilePoint >= 0 && active && static_cast<size_t>(m_ProfilePoint) < points.size())
		{
			// Half-centimeter steps unless Alt is held
			auto [x, y] = toMeters(mouse);
			if (!ImGui::GetIO().KeyAlt)
			{
				x = std::round(x * 200.0f) / 200.0f;
				y = std::round(y * 200.0f) / 200.0f;
			}
			points[static_cast<size_t>(m_ProfilePoint)] = { x, y };
			ImGui::SetTooltip("out %.3f m, up %.3f m", static_cast<double>(x), static_cast<double>(y));
		}
		else
			m_ProfilePoint = -1;

		// The points as numbers
		constexpr ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit;
		if (ImGui::BeginTable("##ProfilePoints", 3, flags))
		{
			ImGui::TableSetupColumn("Out", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Up", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("");
			ImGui::TableHeadersRow();
			bool changed = false;
			for (size_t i = 0; i < points.size(); ++i)
			{
				ImGui::PushID(static_cast<int>(i));
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(-FLT_MIN);
				changed |= ImGui::DragFloat("##Out", &points[i].first, 0.005f, -5.0f, 5.0f, "%.3f m");
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(-FLT_MIN);
				changed |= ImGui::DragFloat("##Up", &points[i].second, 0.005f, -2.0f, 10.0f, "%.3f m");
				ImGui::TableNextColumn();
				if (ImGui::SmallButton(SEDX_ICON_PLUS))
				{
					const auto next = i + 1 < points.size() ? points[i + 1] : std::pair<float, float>{ points[i].first + 0.05f, points[i].second - 0.05f };
					points.insert(points.begin() + static_cast<std::ptrdiff_t>(i) + 1, { 0.5f * (points[i].first + next.first), 0.5f * (points[i].second + next.second) });
					changed = true;
				}
				EditorUI::ItemTooltip("Add a point after this one");
				ImGui::SameLine(0.0f, 2.0f * dpi);
				ImGui::BeginDisabled(points.size() <= 2);
				if (ImGui::SmallButton(SEDX_ICON_TIMES))
				{
					points.erase(points.begin() + static_cast<std::ptrdiff_t>(i));
					changed = true;
					ImGui::EndDisabled();
					ImGui::PopID();
					break;
				}
				ImGui::EndDisabled();
				ImGui::PopID();
			}
			ImGui::EndTable();
			if (changed)
				FacadeAuthoring::MarkEdited();
		}
		ImGui::TextDisabled("From the walking surface's edge down to the ground (out is away from the area). Drag the points; Alt for free placement.");
	}

	void FacadeEditorPanel::DrawStripCanvas(XPlane::CurbFacadeSpec &spec)
	{
		const CanvasTexture &texture = GetTexture(spec.wallTexture);
		ImGui::SetNextItemWidth(90.0f * ImGui::GetStyle().FontScaleDpi);
		ImGui::SliderFloat("##StripZoom", &m_Zoom, 1.0f, 16.0f, "Zoom %.1fx", ImGuiSliderFlags_Logarithmic);
		ImGui::SameLine();
		ImGui::Checkbox("Snap to pixels", &m_Snap);
		const Canvas canvas = BeginCanvas("##StripCanvas", texture, m_Zoom);
		ImDrawList *draw = ImGui::GetWindowDrawList();
		const bool snap = m_Snap && !ImGui::GetIO().KeyAlt;
		const float w = static_cast<float>(std::max(0, texture.width)), h = static_cast<float>(std::max(0, texture.height));
		const ImVec2 mouse = ImGui::GetMousePos();
		bool edited = false;
		if (canvas.activated)
		{
			m_Dragging = true;
			m_DragStart[0] = Snap(canvas.ToS(mouse.x), w, snap);
			m_DragStart[1] = Snap(canvas.ToT(mouse.y), h, snap);
		}
		if (m_Dragging && canvas.active)
		{
			const float s = Snap(canvas.ToS(mouse.x), w, snap), t = Snap(canvas.ToT(mouse.y), h, snap);
			if (std::abs(s - m_DragStart[0]) > 1e-4f && std::abs(t - m_DragStart[1]) > 1e-4f)
			{
				spec.textureS1 = std::min(m_DragStart[0], s);
				spec.textureS2 = std::max(m_DragStart[0], s);
				spec.textureT1 = std::min(m_DragStart[1], t);
				spec.textureT2 = std::max(m_DragStart[1], t);
				edited = true;
			}
		}
		else
			m_Dragging = false;
		const ImVec2 p0 = canvas.ToScreen(spec.textureS1, spec.textureT2), p1 = canvas.ToScreen(spec.textureS2, spec.textureT1);
		draw->AddRectFilled(p0, p1, EditorTheme::ToU32(ImVec4(0.122f, 0.541f, 0.839f, 0.2f)));
		draw->AddRect(p0, p1, EditorTheme::ToU32(EditorTheme::Palette::AccentHovered), 0.0f, 0, 2.0f);
		draw->AddText(ImVec2(p0.x + 3.0f, p0.y + 1.0f), EditorTheme::ToU32(EditorTheme::Palette::Text), "top edge");
		EndCanvas();
		ImGui::TextDisabled("Drag the curb's strip: along the curb left to right, its top edge at the top");
		if (edited)
			FacadeAuthoring::MarkEdited();
	}

	// -------------------------------------------------------
	// Type 2 facades as written (copies of the library's, other packs')

	void FacadeEditorPanel::DrawType2()
	{
		FacadeAsset &asset = FacadeAuthoring::GetAsset();
		bool edited = false;

		if (EditorUI::SectionHeader("TEXTURES") && EditorUI::BeginPropertyGrid("##Type2Textures", 100.0f))
		{
			edited |= TextureRow("Wall", asset.wallTexture, false, "SHADER_WALL's texture: what the segment meshes are cut from");
			edited |= TextureRow("Wall normal", asset.wallTextureNormal, true, "The walls' normal map");
			edited |= TextureRow("Roof", asset.roofTexture, true, "SHADER_ROOF's texture; none: the roof is cut from the wall texture");
			edited |= TextureRow("Roof normal", asset.roofTextureNormal, true, "The roof's normal map");
			// Rows the editor doesn't model (TEXTURE_LIT, WEATHER, DECAL_LIB...), written back as they are
			if (!asset.wallShaderRows.empty() || !asset.roofShaderRows.empty())
			{
				EditorUI::PropertyLabel("Kept rows");
				ImGui::BeginGroup();
				for (const std::string &row : asset.wallShaderRows)
					ImGui::TextDisabled("%s", row.c_str());
				for (const std::string &row : asset.roofShaderRows)
					ImGui::TextDisabled("roof: %s", row.c_str());
				ImGui::EndGroup();
				EditorUI::ItemTooltip("Shader rows written back as they are (the textures they name were copied with the facade)");
			}
			EditorUI::EndPropertyGrid();
		}

		if (EditorUI::SectionHeader("ROOF") && EditorUI::BeginPropertyGrid("##Type2Roof", 100.0f))
		{
			EditorUI::PropertyLabel("Texture repeat");
			ImGui::SetNextItemWidth(-FLT_MIN);
			float scale[2] = { asset.roofScaleS, asset.roofScaleT };
			if (ImGui::DragFloat2("##RoofScale", scale, 0.1f, 0.1f, 1000.0f, "%.1f m"))
			{
				asset.roofScaleS = std::max(0.1f, scale[0]);
				asset.roofScaleT = std::max(0.1f, scale[1]);
				edited = true;
			}
			EditorUI::ItemTooltip("ROOF_SCALE: meters of roof one roof texture covers, along the front wall and in from it");
			for (size_t f = 0; f < asset.floors.size(); ++f)
			{
				FacadeAsset::Floor &floor = asset.floors[f];
				ImGui::PushID(static_cast<int>(f));
				const std::string label = asset.floors.size() > 1 ? "Hard roof " + (floor.name.empty() ? std::to_string(f + 1) : floor.name) : std::string("Hard roof");
				EditorUI::PropertyLabel(label.c_str());
				ImGui::SetNextItemWidth(-FLT_MIN);
				edited |= ImGui::InputTextWithHint("##HardRoof", "none", &floor.hardRoof, ImGuiInputTextFlags_CharsNoBlank);
				EditorUI::ItemTooltip("HARD_ROOF: concrete, asphalt... what aircraft and vehicles feel on this floor's roofs; empty for none");
				ImGui::PopID();
			}
			EditorUI::EndPropertyGrid();
		}

		if (EditorUI::SectionHeader("WALL TYPES"))
		{
			for (size_t f = 0; f < asset.floors.size(); ++f)
			{
				FacadeAsset::Floor &floor = asset.floors[f];
				ImGui::PushID(static_cast<int>(f));
				if (asset.floors.size() > 1)
					ImGui::SeparatorText(("Floor " + (floor.name.empty() ? std::to_string(f + 1) : floor.name)).c_str());
				for (size_t w = 0; w < floor.walls.size(); ++w)
				{
					FacadeAsset::WallRule &wall = floor.walls[w];
					if (wall.filters.empty())
						wall.filters.push_back({ wall.minWidth, wall.maxWidth, wall.minHeading, wall.maxHeading });
					FacadeAsset::WallFilter &rule = wall.filters.front();
					ImGui::PushID(static_cast<int>(w));
					if (EditorUI::BeginPropertyGrid("##Wall", 100.0f))
					{
						EditorUI::PropertyLabel("Name");
						ImGui::SetNextItemWidth(-FLT_MIN);
						edited |= ImGui::InputTextWithHint("##Name", ("Wall " + std::to_string(w + 1)).c_str(), &wall.name);
						EditorUI::ItemTooltip("What the facade tool lists the wall type as");
						EditorUI::PropertyLabel("Widths");
						ImGui::SetNextItemWidth(-FLT_MIN);
						float widths[2] = { rule.minWidth, rule.maxWidth };
						if (ImGui::DragFloat2("##Widths", widths, 0.1f, 0.0f, 9999.0f, "%.1f m"))
						{
							rule.minWidth = wall.minWidth = std::max(0.0f, widths[0]);
							rule.maxWidth = wall.maxWidth = std::max(rule.minWidth, widths[1]);
							edited = true;
						}
						EditorUI::ItemTooltip("Walls this wide (meters) can get this wall type");
						EditorUI::PropertyLabel("Headings");
						ImGui::SetNextItemWidth(-FLT_MIN);
						float headings[2] = { rule.minHeading, rule.maxHeading };
						if (ImGui::DragFloat2("##Headings", headings, 1.0f, 0.0f, 360.0f, "%.0f deg"))
						{
							rule.minHeading = wall.minHeading = std::clamp(headings[0], 0.0f, 360.0f);
							rule.maxHeading = wall.maxHeading = std::clamp(headings[1], 0.0f, 360.0f);
							edited = true;
						}
						EditorUI::ItemTooltip("Facing these degrees from the front wall, clockwise (equal: any heading). 270 - 90 is the front half");
						if (wall.filters.size() > 1)
						{
							EditorUI::PropertyLabel("More rules");
							ImGui::Text("%zu WALL_RULE rows (kept)", wall.filters.size() - 1);
						}
						EditorUI::PropertyLabel("Spellings");
						std::string spellings;
						for (const std::vector<int> &spelling : wall.spellings)
						{
							spellings += spellings.empty() ? "" : "  |  ";
							for (size_t i = 0; i < spelling.size(); ++i)
								spellings += (i ? " " : "") + std::to_string(spelling[i]);
						}
						ImGui::TextWrapped("%s", spellings.empty() ? "(none)" : spellings.c_str());
						EditorUI::ItemTooltip("The segment sequences X-Plane fills a wall with (read-only)");
						EditorUI::EndPropertyGrid();
					}
					ImGui::PopID();
				}
				ImGui::PopID();
			}
		}

		if (EditorUI::SectionHeader("SEGMENTS", false))
		{
			ImGui::TextWrapped("Segments are kept as written (model them in a 3D tool); their objects were copied with the facade.");
			for (size_t f = 0; f < asset.floors.size(); ++f)
			{
				const FacadeAsset::Floor &floor = asset.floors[f];
				size_t triangles = 0;
				for (const FacadeAsset::Segment &segment : floor.segments)
					for (const FacadeAsset::Mesh &mesh : segment.meshes)
						triangles += mesh.indices.size() / 3;
				std::string roofs;
				for (const FacadeAsset::Roof &roof : floor.roofs)
					roofs += (roofs.empty() ? "" : ", ") + Meters(roof.height);
				ImGui::BulletText("%s: %zu segments (%zu curved), %zu triangles; roofs at %s", floor.name.empty() ? ("Floor " + std::to_string(f + 1)).c_str() : floor.name.c_str(),
					floor.segments.size(), floor.curvedSegments.size(), triangles, roofs.empty() ? "none" : roofs.c_str());
			}
			const std::filesystem::path folder = FacadeAuthoring::GetFilePath().parent_path();
			for (size_t i = 0; i < asset.objects.size(); ++i)
			{
				const bool local = !folder.empty() && std::filesystem::exists(folder / VFS::FromUtf8(asset.objects[i]));
				ImGui::BulletText("OBJ %zu  %s%s", i, asset.objects[i].c_str(), local ? "" : "  (library)");
			}
		}

		if (edited)
			FacadeAuthoring::MarkEdited();
	}
}

// -------------------------------------------------------

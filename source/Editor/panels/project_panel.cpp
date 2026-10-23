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
 * project_panel.cpp
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#include "project_panel.h"
#include <portable-file-dialogs.h>
#include <Editor/core/editor_project.h>
#include <Editor/core/editor_selection.h>
#include <Editor/core/project_dialogs.h>
#include <Editor/core/editor_theme.h>
#include <Editor/core/editor_ui.h>
#include <SceneryEditorX/filesystem/vfs.h>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/renderer/font/font_awesome.h>
#include <SceneryEditorX/renderer/ui/source/imgui/misc/cpp/imgui_stdlib.h>
#include <SceneryEditorX/xplane/xp_text.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace fs = std::filesystem;
	using namespace ProjectFile;
	using Section = ProjectDocument::Section;

	namespace
	{
		ImVec4 Color(const ImVec4 &palette)
		{
			return EditorTheme::ToStyle(palette);
		}

		const char *KindIcon(const ItemKind kind)
		{
			switch (kind)
			{
				case ItemKind::Group:				return SEDX_ICON_FOLDER;
				case ItemKind::Airport:				return SEDX_ICON_PLANE;
				case ItemKind::Object:				return SEDX_ICON_CUBE;
				case ItemKind::Polygon:				return SEDX_ICON_MAP;
				case ItemKind::Orthophoto:			return SEDX_ICON_PICTURE_O;
				case ItemKind::Line:				return SEDX_ICON_PENCIL;
				case ItemKind::String:				return SEDX_ICON_LINK;
				case ItemKind::Forest:				return SEDX_ICON_TREE;
				case ItemKind::Facade:				return SEDX_ICON_BUILDING;
				case ItemKind::Autogen:				return SEDX_ICON_ROAD;
				case ItemKind::ExclusionZone:
				case ItemKind::ExclusionPolygon:	return SEDX_ICON_BAN;
				case ItemKind::Raw:					return SEDX_ICON_CODE;
				case ItemKind::RoadNode:			return SEDX_ICON_DOT_CIRCLE_O;
				case ItemKind::Road:				return SEDX_ICON_ROAD;
			}
			return SEDX_ICON_CUBE;
		}

		/* @brief Text input that records an edit when the user finishes typing. */
		void EditText(const char *label, std::string &value, const Section section, const ImGuiInputTextFlags flags = 0)
		{
			EditorUI::PropertyLabel(label);
			ImGui::PushID(label);
			ImGui::InputText("##Value", &value, flags);
			if (ImGui::IsItemDeactivatedAfterEdit())
				EditorProjects::MarkEdited(section);
			ImGui::PopID();
		}

		using VFS::ToUtf8;

		const char *ExportKindName(const XPlane::ExportKind kind)
		{
			return XPlane::GetExportKindName(kind);
		}
	}

	// -------------------------------------------------------

	ProjectPanel::ProjectPanel(EditorContext *context) : m_Context(context)
	{
		m_Title = "PROJECT";
	}

	void ProjectPanel::OnUIRender(bool &isOpen)
	{
		if (EditorUI::BeginPanel("PROJECT###Project", &isOpen))
		{
			DrawToolbar();
			ProjectDocument *document = EditorProjects::GetDocument();
			if (!document)
				EditorUI::EmptyState("No project open.\nFile > New Project (Ctrl+N) for scenery, an airport or a library,\nor open a .edX / .sex project or a WorldEditor earth.wed.xml.");
			else if (ImGui::BeginChild("##ProjectBody"))
			{
				SceneryProject &project = document->GetProject();
				DrawInfo(project);
				DrawAirports(project);
				DrawHierarchy(project);
				DrawLibraries(project);
				DrawLibraryExports(project);
				DrawValidation();
				ImGui::EndChild();
			}
			else
				ImGui::EndChild();

			if (!m_PendingRemove.empty())
			{
				EditorProjects::RemoveItem(m_PendingRemove, m_Context);
				if (m_SelectedItem == m_PendingRemove)
					m_SelectedItem.clear();
				m_PendingRemove.clear();
			}
		}
		EditorUI::EndPanel();
	}

	void ProjectPanel::DrawToolbar()
	{
		ProjectDocument *document = EditorProjects::GetDocument();
		if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_FILE, "New").c_str()))
			EditorProjects::NewProject(m_Context);
		EditorUI::ItemTooltip("New project (Ctrl+N): scenery anywhere, an airport or an asset library");
		ImGui::SameLine();
		if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_FOLDER_OPEN, "Open").c_str()))
			EditorProjects::BrowseOpen(m_Context);
		ImGui::BeginDisabled(!document);
		ImGui::SameLine();
		if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_FLOPPY_O, "Save").c_str()))
			EditorProjects::Save();
		EditorUI::ItemTooltip("Save the project (Ctrl+S); its airport's edits go in with it");
		ImGui::SameLine();
		if (ImGui::Button("Save As..."))
			EditorProjects::SaveAs();
		ImGui::SameLine();
		if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_UPLOAD, "Export").c_str()))
			ProjectDialogs::OpenExport();
		EditorUI::ItemTooltip("Export the X-Plane scenery pack (Ctrl+E): apt.dat, DSF tiles and the pack's own art");
		ImGui::SameLine();
		if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_TIMES, "Close").c_str()))
			EditorProjects::Close(m_Context);
		ImGui::SameLine();
		if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_FOLDER, "Reveal").c_str()))
			EditorProjects::RevealFolder();
		ImGui::EndDisabled();

		if (document)
		{
			ImGui::TextUnformatted(EditorProjects::GetTitle().c_str());
			ImGui::SameLine();
			ImGui::TextDisabled("%s", document->HasFile() ? GetProjectFormatName(document->GetFormat()) : "not saved yet");
			if (document->HasFile())
				EditorUI::ItemTooltip(ToUtf8(document->GetPath()).c_str());
			if (document->WasLoadedFromSnapshot())
			{
				ImGui::SameLine();
				ImGui::TextDisabled("(cached)");
				EditorUI::ItemTooltip("Opened from the project cache: the file hasn't changed since it was last read");
			}
		}
		ImGui::Separator();
	}

	void ProjectPanel::DrawInfo(SceneryProject &project)
	{
		if (!EditorUI::SectionHeader("PROJECT"))
			return;
		if (EditorUI::BeginPropertyGrid("##ProjectInfo"))
		{
			EditText("Name", project.info.name, Section::Info);
			EditorUI::PropertyLabel("Kind");
			int kind = static_cast<int>(project.info.kind);
			if (ImGui::Combo("##Kind", &kind, "Scenery pack\0Asset library\0"))
			{
				project.info.kind = static_cast<ProjectKind>(kind);
				if (project.info.kind == ProjectKind::Library && !project.library)
				{
					project.library.emplace();
					project.library->version = 800;
					EditorProjects::MarkEdited(Section::Library);
				}
				EditorProjects::MarkEdited(Section::Info);
			}
			EditorUI::ItemTooltip("An asset library authors a library.txt (see LIBRARY below)");
			EditText("Author", project.info.author, Section::Info);
			EditText("Description", project.info.description, Section::Info);
			EditText("X-Plane", project.info.xplaneVersion, Section::Info);
			EditorUI::ItemTooltip("The X-Plane version the scenery targets (\"12.1.4-r3\")");
			EditText("Pack folder", project.info.packageName, Section::Info);
			EditorUI::ItemTooltip("The scenery pack folder name the project exports to");

			EditorUI::PropertyLabel("Datum");
			bool datum = project.datum.valid;
			if (ImGui::Checkbox("##HasDatum", &datum))
			{
				project.datum.valid = datum;
				if (datum)
					GetProjectCenter(project, project.datum.latitude, project.datum.longitude);
				EditorProjects::MarkEdited(Section::Info);
			}
			EditorUI::ItemTooltip("The project's reference point (WGS84). Unset: the first airport's datum is used");
			if (project.datum.valid)
			{
				ImGui::SameLine();
				ImGui::SetNextItemWidth(-FLT_MIN);
				double values[2] = { project.datum.latitude, project.datum.longitude };
				ImGui::InputScalarN("##Datum", ImGuiDataType_Double, values, 2, nullptr, nullptr, "%.8f");
				if (ImGui::IsItemDeactivatedAfterEdit())
				{
					project.datum.latitude = std::clamp(values[0], -90.0, 90.0);
					project.datum.longitude = std::clamp(values[1], -180.0, 180.0);
					EditorProjects::MarkEdited(Section::Info);
				}
				EditorUI::PropertyLabel("Elevation (m)");
				ImGui::InputDouble("##Elevation", &project.datum.elevation, 0.0, 0.0, "%.2f");
				if (ImGui::IsItemDeactivatedAfterEdit())
					EditorProjects::MarkEdited(Section::Info);
			}

			EditorUI::PropertyLabel("Created");
			ImGui::TextDisabled("%s", project.info.created.c_str());
			EditorUI::PropertyLabel("Edited");
			ImGui::TextDisabled("%s", project.info.modified.c_str());
			EditorUI::ItemTooltip("Moves only when a save changes the content, so unchanged saves leave git clean");
			EditorUI::EndPropertyGrid();
		}
	}

	void ProjectPanel::DrawAirports(SceneryProject &project)
	{
		if (!EditorUI::SectionHeader("AIRPORTS"))
			return;
		if (ImGui::SmallButton(EditorUI::IconLabel(SEDX_ICON_PLUS, "New airport").c_str()))
			ProjectDialogs::OpenAddAirport();
		EditorUI::ItemTooltip("Add a new airport to the project: ICAO code, name and reference point");
		ImGui::SameLine();
		if (ImGui::SmallButton(EditorUI::IconLabel(SEDX_ICON_DOWNLOAD, "From apt.dat").c_str()))
			EditorProjects::ImportAptDat(m_Context);
		EditorUI::ItemTooltip("Add the airports of an apt.dat file");
		if (project.airports.empty())
			ImGui::TextDisabled("None: a project doesn't need an airport. Add one when the scenery has one.");
		const std::string &shown = EditorProjects::GetShownAirport();
		for (const ProjectAirport &airport : project.airports)
		{
			ImGui::PushID(airport.airport.id.c_str());
			const bool isShown = airport.airport.id == shown;
			const std::string label = airport.airport.id + "  " + airport.airport.name;
			if (ImGui::Selectable(label.c_str(), isShown) && !isShown)
				EditorProjects::ShowAirport(airport.airport.id, m_Context);
			EditorUI::ItemTooltip(isShown ? "Shown in the airport editor (AIRPORTS > Airport)\nRight-click to remove it from the project" : "Show in the airport editor\nRight-click to remove it from the project");
			if (ImGui::BeginPopupContextItem("##AirportMenu"))
			{
				if (ImGui::MenuItem("Remove from project"))
				{
					// Its hierarchy item: removing it removes the airport (undo puts it back with the item)
					project.ForEachItem([&](const SceneryItem &item, int)
					{
						if (item.kind == ItemKind::Airport && item.airportId == airport.airport.id)
						{
							m_PendingRemove = item.id;
							return false;
						}
						return true;
					});
				}
				ImGui::EndPopup();
			}
			ImGui::SameLine(ImGui::GetContentRegionAvail().x - 80.0f * ImGui::GetStyle().FontScaleDpi);
			ImGui::TextDisabled("%zu elements", airport.airport.GetElementCount());
			ImGui::PopID();
		}
	}

	void ProjectPanel::DrawItem(SceneryItem &item)
	{
		ImGui::PushID(item.id.c_str());
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
		if (!item.IsContainer() || item.children.empty())
			flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
		if (m_SelectedItem == item.id)
			flags |= ImGuiTreeNodeFlags_Selected;

		std::string label = EditorUI::IconLabel(KindIcon(item.kind), item.name.empty() ? GetItemKindName(item.kind) : item.name.c_str());
		if (item.hidden)
			label += "  (hidden)";
		ImGui::PushStyleColor(ImGuiCol_Text, item.hidden ? Color(EditorTheme::Palette::TextDisabled) : ImGui::GetStyleColorVec4(ImGuiCol_Text));
		const bool open = ImGui::TreeNodeEx("##Item", flags, "%s", label.c_str());
		ImGui::PopStyleColor();
		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
		{
			m_SelectedItem = item.id;
			if (const uint64_t entity = EditorProjects::GetPlacedEntity(item.id); entity != 0 && m_Context)
				m_Context->selectedEntity = entity;
			else if (item.kind == ItemKind::Airport)
				EditorProjects::ShowAirport(item.airportId, m_Context);
			else if (item.kind == ItemKind::RoadNode)
				EditorSelection::Select(m_Context, EditorSelection::Target::ForItem(item.id));	// A junction draws nothing: select the item itself (the Nodes tool moves it)
		}
		if (ImGui::IsItemHovered() && !item.resource.empty())
			EditorUI::ItemTooltip(item.resource.c_str());

		if (ImGui::BeginPopupContextItem("##ItemMenu"))
		{
			if (ImGui::MenuItem(item.hidden ? "Show" : "Hide"))
			{
				item.hidden = !item.hidden;
				EditorProjects::MarkEdited(Section::Items);
			}
			EditorUI::ItemTooltip("Hidden items stay in the project but aren't exported to X-Plane (and aren't drawn)");
			if (ImGui::MenuItem(item.locked ? "Unlock" : "Lock"))
			{
				item.locked = !item.locked;
				EditorProjects::MarkEdited(Section::Items);
			}
			ImGui::Separator();
			if (ImGui::MenuItem(EditorUI::IconLabel(SEDX_ICON_TRASH, "Remove from project").c_str()))
				m_PendingRemove = item.id;
			ImGui::EndPopup();
		}

		if (open && item.IsContainer() && !item.children.empty())
		{
			for (SceneryItem &child : item.children)
				DrawItem(child);
			ImGui::TreePop();
		}
		ImGui::PopID();
	}

	void ProjectPanel::DrawHierarchy(SceneryProject &project)
	{
		const std::string header = "HIERARCHY (" + std::to_string(project.CountItems()) + ")";
		if (!EditorUI::SectionHeader(header.c_str()))
			return;

		// Keep the tree's selection in step with the scene's
		if (m_Context && m_Context->selectedEntity != 0)
		{
			if (std::string item = EditorProjects::FindItemForEntity(m_Context->selectedEntity); !item.empty())
				m_SelectedItem = std::move(item);
		}

		if (project.items.empty())
			ImGui::TextDisabled("Empty. Place objects from the X-PLANE LIBRARY panel: they join the project.");
		for (SceneryItem &item : project.items)
			DrawItem(item);

		if (const SceneryItem *selected = m_SelectedItem.empty() ? nullptr : project.FindItem(m_SelectedItem))
		{
			ImGui::Spacing();
			ImGui::TextDisabled("%s  %s", GetItemKindName(selected->kind), selected->id.c_str());
			if (selected->kind == ItemKind::Object)
				ImGui::TextDisabled("%.8f, %.8f  heading %.2f", selected->latitude, selected->longitude, selected->heading);
		}
	}

	void ProjectPanel::DrawLibraries(SceneryProject &project)
	{
		const std::string header = "LIBRARIES (" + std::to_string(project.libraries.size()) + ")";
		if (!EditorUI::SectionHeader(header.c_str(), false))
			return;
		if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_BOOK, "Find Used Libraries").c_str()))
		{
			const size_t assigned = EditorProjects::AssignLibraries();
			EDITOR_INFO_TAG("Project", "{} items linked to their library", assigned);
		}
		EditorUI::ItemTooltip("Record which X-Plane library each placed asset comes from (\"associated-library\")");

		int remove = -1;
		if (!project.libraries.empty() && ImGui::BeginTable("##Libraries", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp))
		{
			ImGui::TableSetupColumn("Library");
			ImGui::TableSetupColumn("Id", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("Exports", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("##Remove", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableHeadersRow();
			for (size_t i = 0; i < project.libraries.size(); ++i)
			{
				const LibraryReference &library = project.libraries[i];
				ImGui::PushID(static_cast<int>(i));
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(library.name.c_str());
				EditorUI::ItemTooltip(library.localPath.c_str());
				ImGui::TableNextColumn();
				ImGui::TextDisabled("%s", library.shortId.c_str());
				ImGui::TableNextColumn();
				ImGui::Text("%u", library.entryCount);
				ImGui::TableNextColumn();
				if (ImGui::SmallButton(SEDX_ICON_TRASH))
					remove = static_cast<int>(i);
				ImGui::PopID();
			}
			ImGui::EndTable();
		}
		if (remove >= 0)
		{
			project.libraries.erase(project.libraries.begin() + remove);
			EditorProjects::MarkEdited(Section::Libraries);
		}
	}

	void ProjectPanel::DrawLibraryExports(SceneryProject &project)
	{
		if (!project.library)
			return;
		XPlane::LibraryFile &library = *project.library;
		const std::string header = "LIBRARY (" + std::to_string(library.exports.size()) + " exports)";
		if (!EditorUI::SectionHeader(header.c_str()))
			return;

		if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_PLUS, "Add Files...").c_str()))
		{
			// Export art files of the project folder under a virtual path derived from the pack name
			const fs::path folder = EditorProjects::GetProjectFolder();
			const std::vector<std::string> files = pfd::open_file("Export files from the library", ToUtf8(folder),
				{ "X-Plane art", "*.obj *.fac *.for *.lin *.pol *.str *.agp *.agb *.ags *.net *.ter *.bch *.dcl", "All files", "*" }, pfd::opt::multiselect).result();
			const std::string prefix = "lib/" + (project.info.packageName.empty() ? project.info.name : project.info.packageName) + "/";
			for (const std::string &file : files)
			{
				std::error_code ec;
				const fs::path path(std::u8string(file.begin(), file.end()));
				const fs::path relative = folder.empty() ? path.filename() : fs::relative(path, folder, ec);
				XPlane::LibraryExport entry;
				entry.realPath = ToUtf8(fs::path(relative.generic_u8string()));
				entry.virtualPath = prefix + entry.realPath;
				for (char &c : entry.virtualPath)
					c = c == ' ' ? '_' : c;		// Virtual paths can't hold spaces
				library.exports.push_back(std::move(entry));
			}
			if (!files.empty())
				EditorProjects::MarkEdited(Section::Library);
		}
		EditorUI::ItemTooltip("Export art from the project folder (real paths stay relative to it)");
		ImGui::SameLine();
		if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_PLUS, "Add Export").c_str()))
		{
			library.exports.emplace_back();
			library.exports.back().virtualPath = "lib/new/asset.obj";
			library.exports.back().realPath = "objects/asset.obj";
			EditorProjects::MarkEdited(Section::Library);
		}
		ImGui::SameLine();
		if (ImGui::Button("Export library.txt..."))
			EditorProjects::ExportLibrary();
		EditorUI::SearchFilter("##ExportFilter", m_ExportFilter);

		const fs::path folder = EditorProjects::GetProjectFolder();
		int remove = -1;
		constexpr ImGuiTableFlags TABLE = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp;
		const float height = std::min(14.0f, static_cast<float>(library.exports.size()) + 1.5f) * ImGui::GetFrameHeightWithSpacing();
		if (!library.exports.empty() && ImGui::BeginTable("##Exports", 5, TABLE, ImVec2(0.0f, height)))
		{
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableSetupColumn("Virtual path", ImGuiTableColumnFlags_WidthStretch, 3.0f);
			ImGui::TableSetupColumn("File", ImGuiTableColumnFlags_WidthStretch, 3.0f);
			ImGui::TableSetupColumn("Kind", ImGuiTableColumnFlags_WidthStretch, 1.2f);
			ImGui::TableSetupColumn("Region", ImGuiTableColumnFlags_WidthStretch, 1.0f);
			ImGui::TableSetupColumn("##Remove", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableHeadersRow();
			for (size_t i = 0; i < library.exports.size(); ++i)
			{
				XPlane::LibraryExport &entry = library.exports[i];
				if (!m_ExportFilter.PassFilter(entry.virtualPath.c_str()) && !m_ExportFilter.PassFilter(entry.realPath.c_str()))
					continue;
				ImGui::PushID(static_cast<int>(i));
				ImGui::TableNextRow();

				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::InputText("##Virtual", &entry.virtualPath, ImGuiInputTextFlags_CharsNoBlank);
				if (ImGui::IsItemDeactivatedAfterEdit())
					EditorProjects::MarkEdited(Section::Library);

				ImGui::TableNextColumn();
				std::error_code ec;
				const bool missing = !folder.empty() && !fs::exists(folder / XPlane::ToNativeRelativePath(entry.realPath), ec);
				if (missing)
					ImGui::PushStyleColor(ImGuiCol_Text, Color(EditorTheme::Palette::Error));
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::InputText("##File", &entry.realPath);
				if (ImGui::IsItemDeactivatedAfterEdit())
					EditorProjects::MarkEdited(Section::Library);
				if (missing)
				{
					ImGui::PopStyleColor();
					EditorUI::ItemTooltip("This file isn't in the project folder");
				}

				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(-FLT_MIN);
				if (ImGui::BeginCombo("##Kind", entry.ratio != 1.0f ? "EXPORT_RATIO" : ExportKindName(entry.kind)))
				{
					for (const XPlane::ExportKind kind : { XPlane::ExportKind::Export, XPlane::ExportKind::Extend, XPlane::ExportKind::Exclude, XPlane::ExportKind::Backup })
					{
						if (ImGui::Selectable(ExportKindName(kind), entry.kind == kind))
						{
							entry.kind = kind;
							EditorProjects::MarkEdited(Section::Library);
						}
					}
					ImGui::EndCombo();
				}

				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::InputText("##Region", &entry.region, ImGuiInputTextFlags_CharsNoBlank);
				if (ImGui::IsItemDeactivatedAfterEdit())
					EditorProjects::MarkEdited(Section::Library);

				ImGui::TableNextColumn();
				if (ImGui::SmallButton(SEDX_ICON_TRASH))
					remove = static_cast<int>(i);
				ImGui::PopID();
			}
			ImGui::EndTable();
		}
		if (remove >= 0)
		{
			library.exports.erase(library.exports.begin() + remove);
			EditorProjects::MarkEdited(Section::Library);
		}
	}

	void ProjectPanel::DrawValidation()
	{
		const std::vector<std::string> &problems = EditorProjects::GetValidationProblems();
		const std::string header = "VALIDATION (" + std::to_string(problems.size()) + ")";
		if (!EditorUI::SectionHeader(header.c_str(), !problems.empty()))
			return;
		if (problems.empty())
		{
			ImGui::TextColored(Color(EditorTheme::Palette::Accent), "%s  No problems found", SEDX_ICON_CHECK);
			return;
		}
		for (size_t i = 0; i < problems.size() && i < 200; ++i)
			ImGui::TextColored(Color(EditorTheme::Palette::Warning), "%s  %s", SEDX_ICON_EXCLAMATION_TRIANGLE, problems[i].c_str());
	}
}

// -------------------------------------------------------

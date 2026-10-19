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
 * project_dialogs.cpp
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#include "project_dialogs.h"
#include "editor_project.h"
#include "editor_theme.h"
#include "editor_ui.h"
#include <algorithm>
#include <cmath>
#include <optional>
#include <portable-file-dialogs.h>
#include <SceneryEditorX/core/application/application.h>
#include <SceneryEditorX/core/platform/platform_context.h>
#include <SceneryEditorX/filesystem/file_system.h>
#include <SceneryEditorX/filesystem/vfs.h>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/project/scenery_pack.h>
#include <SceneryEditorX/renderer/font/font_awesome.h>
#include <SceneryEditorX/renderer/ui/source/imgui/misc/cpp/imgui_stdlib.h>
#include <SceneryEditorX/xplane/xp_airport_document.h>
#include <SceneryEditorX/xplane/xp_library_manager.h>

// -------------------------------------------------------

namespace SceneryEditorX::ProjectDialogs
{
	namespace fs = std::filesystem;
	using namespace ProjectFile;

	namespace
	{
		constexpr const char *LOG_TAG = "Project";
		constexpr const char *NEW_MODAL = "New Airport Project";
		constexpr const char *EXPORT_MODAL = "Export Scenery Pack";
		constexpr const char *LOCATION_FILE = "user:/new_project_location.txt";	// The folder the last project was created in
		constexpr double METERS_PER_FOOT = 0.3048;

		constexpr XPlane::AirportType AIRPORT_TYPES[] = { XPlane::AirportType::Land, XPlane::AirportType::Seaplane, XPlane::AirportType::Heliport };
		constexpr const char *AIRPORT_TYPE_NAMES[] = { "Land airport", "Seaplane base", "Heliport" };

		/* @brief The New Airport Project form, as typed. */
		struct NewForm
		{
			std::string icao;
			std::string name;
			std::string country;
			std::string state;
			std::string city;
			std::string latitude;
			std::string longitude;
			double elevation = 0.0;			// In the unit shown
			bool meters = false;
			int type = 0;
			bool starterLayout = true;
			std::string projectName;		// Empty: "<ICAO> - <Name>"
			std::string location;
			std::string author;
			bool attempted = false;			// Create was pressed: every problem shows, not only those of fields typed in
			std::string error;
		};

		struct ExportForm
		{
			std::string folder;
			std::string project;			// The project file the folder was chosen for
			ExportOptions options;
			bool registerPack = true;
			std::optional<ExportReport> report;
			std::string error;
		};

		struct State
		{
			bool openNew = false;
			bool openExport = false;
			NewForm form;
			ExportForm exporter;

			std::string showOnStartup;		// --show-dialog new|export (screenshots without input)
			int showFrames = 0;
			bool exportOnStartup = false;	// --export-scenery
			std::string startupExportFolder;
			int startupExportFrames = 0;

			fs::path mountedXPlane;
			fs::path mountedProject;
		};

		State &S()
		{
			static State state;
			return state;
		}

		ImVec4 Color(const ImVec4 &palette)
		{
			return EditorTheme::ToStyle(palette);
		}

		fs::path XPlaneRoot()
		{
			return XPlane::LibraryManager::GetInstallPath();
		}

		/* @brief Where the form suggests creating the project: the last folder used, else Custom Scenery, else Documents. */
		std::string DefaultLocation()
		{
			std::string saved;
			std::error_code ec;
			if (VFS::Get().Read(LOCATION_FILE, saved))
			{
				while (!saved.empty() && (saved.back() == '\n' || saved.back() == '\r' || saved.back() == ' '))
					saved.pop_back();
				if (!saved.empty() && fs::is_directory(VFS::FromUtf8(saved), ec))
					return saved;
			}
			if (const fs::path root = XPlaneRoot(); !root.empty())
				return VFS::ToUtf8(VFS::GetCustomSceneryFolder(root));
			const fs::path documents = VFS::GetFolder(VFS::Folder::Documents);
			return VFS::ToUtf8(documents.empty() ? fs::current_path(ec) : documents);
		}

		/* @brief The form as the spec CreateAirportProject takes, and every problem with it (coordinates first). */
		NewAirportProject ToSpec(const NewForm &form, std::vector<FieldProblem> &problems)
		{
			NewAirportProject spec;
			spec.icao = form.icao;
			spec.name = form.name;
			spec.country = form.country;
			spec.state = form.state;
			spec.city = form.city;
			spec.elevationFeet = form.meters ? form.elevation / METERS_PER_FOOT : form.elevation;
			spec.type = AIRPORT_TYPES[std::clamp(form.type, 0, 2)];
			spec.starterLayout = form.starterLayout;
			spec.projectName = form.projectName;
			spec.location = VFS::FromUtf8(form.location);
			spec.author = form.author;

			problems.clear();
			std::string reason;
			if (!ParseCoordinate(form.latitude, true, spec.latitude, &reason))
				problems.push_back({ "latitude", reason });
			if (!ParseCoordinate(form.longitude, false, spec.longitude, &reason))
				problems.push_back({ "longitude", reason });
			const bool coordinatesBad = !problems.empty();
			for (FieldProblem &problem : ValidateNewAirportProject(spec))
			{
				const bool coordinate = problem.field == "latitude" || problem.field == "longitude";
				if (!(coordinate && coordinatesBad))
					problems.push_back(std::move(problem));
			}
			return spec;
		}

		/* @brief The first problem of a field, shown under it once the field has something in it (or Create was pressed). */
		void DrawProblem(const std::vector<FieldProblem> &problems, const char *field, const bool touched)
		{
			if (!touched)
				return;
			for (const FieldProblem &problem : problems)
			{
				if (problem.field != field)
					continue;
				ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x);
				ImGui::TextColored(Color(EditorTheme::Palette::Error), "%s", problem.message.c_str());
				ImGui::PopTextWrapPos();
				return;
			}
		}

		/* @brief Lets through what a decimal-degree coordinate can hold: digits, sign, point, spaces and N S E W. */
		int CoordinateCharFilter(ImGuiInputTextCallbackData *data)
		{
			const ImWchar c = data->EventChar;
			if ((c >= '0' && c <= '9') || c == '.' || c == '-' || c == '+' || c == ' ')
				return 0;
			switch (c)
			{
				case 'N': case 'S': case 'E': case 'W':
				case 'n': case 's': case 'e': case 'w':
					return 0;
				default:
					return 1;
			}
		}

		/* @brief Exports the open project; logs and keeps the report. */
		bool RunExport(const fs::path &folder, ExportOptions options, const bool registerPack, ExportReport &report, std::string &error)
		{
			ProjectDocument *document = EditorProjects::GetDocument();
			if (!document)
			{
				error = "No project is open";
				return false;
			}
			EditorProjects::NotifySceneEdited();	// What was moved in the scene belongs in the export

			const fs::path root = XPlaneRoot();
			if (!root.empty())
				options.protectedFolders = { root / "Global Scenery", root / "Resources" };
			if (const std::shared_ptr<const XPlane::LibraryIndex> index = XPlane::LibraryManager::GetIndex(); index && !index->GetAssets().empty())
				options.isLibraryPath = [index](const std::string_view path) { return index->Find(path) != nullptr; };
			if (!ExportSceneryPack(document->GetProject(), EditorProjects::GetProjectFolder(), folder, options, report, &error))
			{
				EDITOR_ERROR_TAG(LOG_TAG, "Export to {} failed: {}", VFS::ToUtf8(folder), error);
				return false;
			}

			// Listed in scenery_packs.ini so X-Plane (and the airport index) order it first
			const fs::path customScenery = root.empty() ? fs::path() : VFS::GetCustomSceneryFolder(root);
			if (registerPack && !customScenery.empty() && VFS::IsInside(folder.parent_path(), customScenery) && VFS::IsInside(customScenery, folder.parent_path()))
			{
				std::string reason;
				if (!XPlane::RegisterSceneryPack(root, folder, &reason))
					report.warnings.push_back("Not listed in scenery_packs.ini: " + reason);
			}

			EDITOR_INFO_TAG(LOG_TAG, "Exported {} to {}: {} airports, {} DSF tiles ({} objects, {} polygons), {} art files copied, {} files written",
				document->GetProject().info.name, VFS::ToUtf8(folder), report.airports, report.tiles, report.objects, report.polygons, report.assets, report.written.size());
			for (const std::string &warning : report.warnings)
				EDITOR_WARN_TAG(LOG_TAG, "Export: {}", warning);
			if (!report.problems.empty())
				EDITOR_WARN_TAG(LOG_TAG, "Export: the project has {} problems X-Plane may reject (see the PROJECT panel)", report.problems.size());
			return true;
		}

		/* @brief The project's own folder for a new project, else the folder its last export went to. */
		void PrepareExportFolder()
		{
			ExportForm &exporter = S().exporter;
			ProjectDocument *document = EditorProjects::GetDocument();
			const std::string project = document && document->HasFile() ? VFS::ToUtf8(document->GetPath()) : std::string();
			if (exporter.folder.empty() || exporter.project != project)
			{
				exporter.folder = VFS::ToUtf8(EditorProjects::GetProjectFolder());
				exporter.project = project;
			}
		}

		/* @brief "ICAO;Name;Country;lat;lon;elevation ft[;folder]" from the command line. */
		std::optional<NewAirportProject> ParseCommandLineSpec(const std::string &text, std::string &error)
		{
			std::vector<std::string> parts;
			size_t start = 0;
			while (start <= text.size())
			{
				const size_t end = std::min(text.find(';', start), text.size());
				parts.push_back(text.substr(start, end - start));
				start = end + 1;
			}
			if (parts.size() < 6)
			{
				error = "--new-airport-project wants \"ICAO;Name;Country;latitude;longitude;elevation ft[;folder]\"";
				return std::nullopt;
			}
			NewAirportProject spec;
			spec.icao = parts[0];
			spec.name = parts[1];
			spec.country = parts[2];
			if (!ParseCoordinate(parts[3], true, spec.latitude, &error) || !ParseCoordinate(parts[4], false, spec.longitude, &error))
				return std::nullopt;
			spec.elevationFeet = std::atof(parts[5].c_str());
			spec.location = VFS::FromUtf8(parts.size() > 6 && !parts[6].empty() ? parts[6] : DefaultLocation());
			return spec;
		}

		void DrawNewProject(EditorContext *context)
		{
			State &s = S();
			NewForm &form = s.form;
			const float dpi = ImGui::GetStyle().FontScaleDpi;

			ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
			ImGui::SetNextWindowSizeConstraints(ImVec2(560.0f * dpi, 0.0f), ImVec2(720.0f * dpi, FLT_MAX));
			if (!ImGui::BeginPopupModal(NEW_MODAL, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
				return;

			std::vector<FieldProblem> problems;
			NewAirportProject spec = ToSpec(form, problems);
			const auto touched = [&form](const std::string &text) { return form.attempted || !text.empty(); };
			const float labelWidth = 170.0f;

			ImGui::SeparatorText("Airport");
			if (EditorUI::BeginPropertyGrid("##newAirport", labelWidth))
			{
				EditorUI::PropertyLabel("ICAO code");
				ImGui::SetNextItemWidth(-FLT_MIN);
				if (ImGui::InputTextWithHint("##icao", "KSEA", &form.icao, ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank) && form.icao.size() > 7)
					form.icao.resize(7);
				EditorUI::ItemTooltip("The airport's identifier in apt.dat: its ICAO code, or 3 to 7 letters and digits when it has none");
				DrawProblem(problems, "icao", touched(form.icao));

				EditorUI::PropertyLabel("Airport name");
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::InputTextWithHint("##name", "Seattle Tacoma Intl", &form.name);
				DrawProblem(problems, "name", touched(form.name));

				EditorUI::PropertyLabel("Type");
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::Combo("##type", &form.type, AIRPORT_TYPE_NAMES, IM_ARRAYSIZE(AIRPORT_TYPE_NAMES));

				EditorUI::PropertyLabel("Starter layout");
				ImGui::Checkbox(form.type == 2 ? "A helipad, windsock and beacon##starter" : (form.type == 1 ? "A water runway, windsock and beacon##starter" : "A runway, windsock and beacon##starter"), &form.starterLayout);
				EditorUI::ItemTooltip("Something to start shaping: X-Plane needs at least one runway, water runway or helipad");
				EditorUI::EndPropertyGrid();
			}

			ImGui::SeparatorText("Reference point");
			if (EditorUI::BeginPropertyGrid("##newLocation", labelWidth))
			{
				EditorUI::PropertyLabel("Latitude");
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::InputTextWithHint("##latitude", "47.44898333", &form.latitude, ImGuiInputTextFlags_CallbackCharFilter, CoordinateCharFilter);
				DrawProblem(problems, "latitude", touched(form.latitude));

				EditorUI::PropertyLabel("Longitude");
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::InputTextWithHint("##longitude", "-122.30916667", &form.longitude, ImGuiInputTextFlags_CallbackCharFilter, CoordinateCharFilter);
				DrawProblem(problems, "longitude", touched(form.longitude));

				EditorUI::PropertyLabel("Elevation (MSL)");
				ImGui::SetNextItemWidth(160.0f * dpi);
				ImGui::InputDouble("##elevation", &form.elevation, form.meters ? 1.0 : 10.0, form.meters ? 10.0 : 100.0, "%.0f");
				ImGui::SameLine();
				if (ImGui::RadioButton("ft", !form.meters) && form.meters)
				{
					form.meters = false;
					form.elevation = std::round(form.elevation / METERS_PER_FOOT);
				}
				ImGui::SameLine();
				if (ImGui::RadioButton("m", form.meters) && !form.meters)
				{
					form.meters = true;
					form.elevation = std::round(form.elevation * METERS_PER_FOOT);
				}
				DrawProblem(problems, "elevation", true);
				EditorUI::EndPropertyGrid();
			}
			ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 540.0f * dpi);
			ImGui::TextColored(Color(EditorTheme::Palette::TextLabel), "Decimal degrees with up to 8 places. North and east are positive, south and west negative: 50 degrees 30 minutes south is -50.50000000. Headings in the project are true, not magnetic.");
			ImGui::PopTextWrapPos();

			ImGui::SeparatorText("Region");
			if (EditorUI::BeginPropertyGrid("##newRegion", labelWidth))
			{
				EditorUI::PropertyLabel("Country");
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::InputTextWithHint("##country", "USA United States", &form.country);
				DrawProblem(problems, "country", touched(form.country));

				EditorUI::PropertyLabel("State / county");
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::InputTextWithHint("##state", "Optional: state, province or county", &form.state);
				DrawProblem(problems, "state", true);

				EditorUI::PropertyLabel("City");
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::InputTextWithHint("##city", "Optional", &form.city);
				DrawProblem(problems, "city", true);
				EditorUI::EndPropertyGrid();
			}

			ImGui::SeparatorText("Project");
			const std::string defaultName = MakeDefaultProjectName(form.icao, form.name);
			if (EditorUI::BeginPropertyGrid("##newProject", labelWidth))
			{
				EditorUI::PropertyLabel("Project name");
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::InputTextWithHint("##projectName", defaultName.c_str(), &form.projectName);
				EditorUI::ItemTooltip("The scenery pack's folder and the project file are named after it");
				DrawProblem(problems, "projectName", true);

				EditorUI::PropertyLabel("Create in");
				const float browseWidth = ImGui::CalcTextSize("Browse...").x + ImGui::GetStyle().FramePadding.x * 2.0f;
				ImGui::SetNextItemWidth(-(browseWidth + ImGui::GetStyle().ItemSpacing.x));
				ImGui::InputText("##location", &form.location);
				ImGui::SameLine();
				if (ImGui::Button("Browse...##location"))
				{
					const std::string chosen = pfd::select_folder("Choose the folder the project is created in", form.location).result();
					if (!chosen.empty())
						form.location = chosen;
				}
				if (const fs::path root = XPlaneRoot(); !root.empty())
				{
					if (ImGui::SmallButton("Custom Scenery"))
						form.location = VFS::ToUtf8(VFS::GetCustomSceneryFolder(root));
					EditorUI::ItemTooltip("X-Plane 12's Custom Scenery folder: the pack loads in X-Plane where it's made");
				}
				DrawProblem(problems, "location", true);

				EditorUI::PropertyLabel("Author");
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::InputTextWithHint("##author", "Optional", &form.author);
				EditorUI::EndPropertyGrid();
			}

			if (!form.location.empty())
			{
				const std::string folder = VFS::ToUtf8(GetNewProjectFolder(spec));
				ImGui::TextColored(Color(EditorTheme::Palette::TextLabel), "%s  %s", SEDX_ICON_FOLDER_O, folder.c_str());
				ImGui::TextColored(Color(EditorTheme::Palette::TextDisabled), "Earth nav data, objects and textures folders, and %s.edX inside it",
					(form.projectName.empty() ? defaultName : form.projectName).c_str());
			}
			if (!form.error.empty())
				ImGui::TextColored(Color(EditorTheme::Palette::Error), "%s  %s", SEDX_ICON_EXCLAMATION_TRIANGLE, form.error.c_str());

			ImGui::Spacing();
			const float buttonWidth = 130.0f * dpi;
			if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_PLUS, "Create").c_str(), ImVec2(buttonWidth, 0.0f)))
			{
				form.attempted = true;
				form.error.clear();
				fs::path projectFile;
				if (problems.empty())
				{
					if (CreateAirportProject(spec, projectFile, &form.error))
					{
						VFS::Get().Write(LOCATION_FILE, form.location);
						EDITOR_INFO_TAG(LOG_TAG, "Created the airport project {}", VFS::ToUtf8(projectFile));
						ImGui::CloseCurrentPopup();
						EditorProjects::Open(projectFile, context, true);
						const NewForm previous = form;
						form = NewForm();
						form.location = previous.location;
						form.author = previous.author;
					}
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0.0f)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
				ImGui::CloseCurrentPopup();
			if (form.attempted && !problems.empty())
			{
				ImGui::SameLine();
				ImGui::TextColored(Color(EditorTheme::Palette::Warning), "%s  %zu %s to fix", SEDX_ICON_EXCLAMATION_TRIANGLE, problems.size(), problems.size() == 1 ? "field" : "fields");
			}
			ImGui::EndPopup();
		}

		void DrawExport()
		{
			State &s = S();
			ExportForm &exporter = s.exporter;
			const float dpi = ImGui::GetStyle().FontScaleDpi;

			ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
			ImGui::SetNextWindowSizeConstraints(ImVec2(560.0f * dpi, 0.0f), ImVec2(760.0f * dpi, FLT_MAX));
			if (!ImGui::BeginPopupModal(EXPORT_MODAL, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
				return;

			ProjectDocument *document = EditorProjects::GetDocument();
			if (!document)
			{
				ImGui::CloseCurrentPopup();
				ImGui::EndPopup();
				return;
			}
			const SceneryProject &project = document->GetProject();
			ImGui::Text("%s  Export %s as an X-Plane 12 scenery pack", SEDX_ICON_PLANE, project.info.name.c_str());
			ImGui::Spacing();

			// Where: the project's own folder by default (it is laid out as a pack)
			const float browseWidth = ImGui::CalcTextSize("Browse...").x + ImGui::GetStyle().FramePadding.x * 2.0f;
			ImGui::SetNextItemWidth(-(browseWidth + ImGui::GetStyle().ItemSpacing.x));
			ImGui::InputText("##exportFolder", &exporter.folder);
			ImGui::SameLine();
			if (ImGui::Button("Browse...##export"))
			{
				const std::string chosen = pfd::select_folder("Choose the scenery pack folder to export to", exporter.folder).result();
				if (!chosen.empty())
					exporter.folder = chosen;
			}
			const fs::path folder = VFS::FromUtf8(exporter.folder);
			const fs::path projectFolder = EditorProjects::GetProjectFolder();
			const fs::path root = XPlaneRoot();
			const fs::path customScenery = root.empty() ? fs::path() : VFS::GetCustomSceneryFolder(root);
			const bool sameFolder = !projectFolder.empty() && VFS::IsInside(folder, projectFolder) && VFS::IsInside(projectFolder, folder);
			const bool inCustomScenery = !customScenery.empty() && VFS::IsInside(folder.parent_path(), customScenery) && VFS::IsInside(customScenery, folder.parent_path());
			const char *where = sameFolder ? (inCustomScenery ? "The project's own folder, in Custom Scenery: X-Plane loads it from here" : "The project's own folder")
				: (inCustomScenery ? "A pack in Custom Scenery" : "Outside Custom Scenery: move the folder there to fly it");
			ImGui::TextColored(Color(EditorTheme::Palette::TextLabel), "%s", where);

			ImGui::SeparatorText("Write");
			ImGui::Checkbox("Airports  (Earth nav data/apt.dat)", &exporter.options.aptDat);
			ImGui::Checkbox("Overlay placements  (Earth nav data DSF tiles)", &exporter.options.dsf);
			ImGui::BeginDisabled(sameFolder);
			ImGui::Checkbox(sameFolder ? "Copy the pack's own objects and textures  (they're here already)###copyAssets" : "Copy the pack's own objects and textures###copyAssets", &exporter.options.copyAssets);
			ImGui::EndDisabled();
			EditorUI::ItemTooltip("Art the project places from its own folder (objects/..., textures/...), with the textures it names. Library art stays in its library.");
			if (project.library)
				ImGui::Checkbox("library.txt", &exporter.options.library);
			ImGui::Checkbox("earth.wed.xml  (open the pack in WorldEditor)", &exporter.options.wed);
			ImGui::Checkbox("Include hidden items", &exporter.options.includeHidden);
			ImGui::Checkbox("Remove DSF tiles the project no longer places anything in", &exporter.options.removeStaleTiles);
			ImGui::BeginDisabled(!inCustomScenery);
			ImGui::Checkbox("List the pack in scenery_packs.ini", &exporter.registerPack);
			ImGui::EndDisabled();

			const std::vector<std::string> &problems = EditorProjects::GetValidationProblems();
			if (problems.empty())
				ImGui::TextColored(Color(EditorTheme::Palette::Accent), "%s  No problems found", SEDX_ICON_CHECK);
			else if (ImGui::TreeNode("##problems", "%s  %zu problems X-Plane may reject", SEDX_ICON_EXCLAMATION_TRIANGLE, problems.size()))
			{
				for (size_t i = 0; i < problems.size() && i < 50; ++i)
					ImGui::TextColored(Color(EditorTheme::Palette::Warning), "%s", problems[i].c_str());
				ImGui::TreePop();
			}

			if (exporter.report)
			{
				const ExportReport &report = *exporter.report;
				ImGui::SeparatorText("Exported");
				ImGui::TextColored(Color(EditorTheme::Palette::Accent), "%s  %zu airports, %zu DSF tiles (%zu objects, %zu polygons), %zu art files copied, %zu files written",
					SEDX_ICON_CHECK, report.airports, report.tiles, report.objects, report.polygons, report.assets, report.written.size());
				if (!report.removed.empty())
					ImGui::Text("Removed %zu stale DSF tiles", report.removed.size());
				if (!report.warnings.empty() && ImGui::BeginChild("##exportWarnings", ImVec2(0.0f, std::min(160.0f, 22.0f * static_cast<float>(report.warnings.size()) + 8.0f) * dpi), ImGuiChildFlags_Borders))
				{
					for (const std::string &warning : report.warnings)
						ImGui::TextColored(Color(EditorTheme::Palette::Warning), "%s  %s", SEDX_ICON_EXCLAMATION_TRIANGLE, warning.c_str());
				}
				if (!report.warnings.empty())
					ImGui::EndChild();
			}
			if (!exporter.error.empty())
				ImGui::TextColored(Color(EditorTheme::Palette::Error), "%s  %s", SEDX_ICON_EXCLAMATION_TRIANGLE, exporter.error.c_str());

			ImGui::Spacing();
			const float buttonWidth = 130.0f * dpi;
			ImGui::BeginDisabled(exporter.folder.empty());
			if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_UPLOAD, "Export").c_str(), ImVec2(buttonWidth, 0.0f)))
			{
				exporter.error.clear();
				ExportReport report;
				if (RunExport(folder, exporter.options, exporter.registerPack && inCustomScenery, report, exporter.error))
					exporter.report = std::move(report);
				else
					exporter.report.reset();
			}
			ImGui::EndDisabled();
			ImGui::SameLine();
			if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_FOLDER_OPEN, "Show Folder").c_str(), ImVec2(buttonWidth, 0.0f)))
				IO::FileSystem::OpenDirectoryInExplorer(folder);
			ImGui::SameLine();
			if (ImGui::Button("Close", ImVec2(buttonWidth, 0.0f)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
				ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}

		/* @brief The mounts follow the X-Plane install and the open project. */
		void UpdateMounts()
		{
			State &s = S();
			VFS::FileSystem &vfs = VFS::Get();
			const fs::path root = XPlaneRoot();
			if (root != s.mountedXPlane)
			{
				s.mountedXPlane = root;
				if (root.empty())
				{
					vfs.Unmount(VFS::MOUNT_XPLANE);
					vfs.Unmount(VFS::MOUNT_SCENERY);
				}
				else
				{
					vfs.Mount(VFS::MOUNT_XPLANE, root, true);
					vfs.Mount(VFS::MOUNT_SCENERY, VFS::GetCustomSceneryFolder(root));
					EDITOR_INFO_TAG(LOG_TAG, "VFS: xplane:/ -> {} (read-only), scenery:/ -> {}", VFS::ToUtf8(root), VFS::ToUtf8(VFS::GetCustomSceneryFolder(root)));
				}
			}

			const fs::path project = EditorProjects::GetDocument() ? EditorProjects::GetProjectFolder() : fs::path();
			if (project != s.mountedProject)
			{
				s.mountedProject = project;
				if (project.empty())
					vfs.Unmount(VFS::MOUNT_PROJECT);
				else
					vfs.Mount(VFS::MOUNT_PROJECT, project);
			}
		}
	}

	void Init()
	{
		VFS::FileSystem &vfs = VFS::Get();
		if (const fs::path config = VFS::GetAppFolder(VFS::Folder::Config); !config.empty())
			vfs.Mount(VFS::MOUNT_USER, config);
		if (const fs::path cache = VFS::GetAppFolder(VFS::Folder::Cache); !cache.empty())
			vfs.Mount(VFS::MOUNT_CACHE, cache / "cache");
		S().form.location = DefaultLocation();

		const PlatformContext *platform = Application::Get().GetPlatformContext();
		if (!platform)
			return;
		const std::vector<std::string> &args = platform->GetCommandLineArgs();
		for (size_t i = 0; i < args.size(); ++i)
		{
			if (args[i] == "--new-airport-project" && i + 1 < args.size())
			{
				std::string error;
				const std::optional<NewAirportProject> spec = ParseCommandLineSpec(args[++i], error);
				if (!spec)
				{
					EDITOR_ERROR_TAG(LOG_TAG, "{}", error);
					continue;
				}
				fs::path projectFile;
				if (CreateAirportProject(*spec, projectFile, &error))
					EDITOR_INFO_TAG(LOG_TAG, "Created the airport project {}", VFS::ToUtf8(projectFile));
				else
				{
					// Run again: open what the first run made
					const fs::path folder = GetNewProjectFolder(*spec);
					fs::path name = folder.filename();
					name += EDX_EXTENSION;
					projectFile = folder / name;
					std::error_code ec;
					if (!fs::is_regular_file(projectFile, ec))
					{
						EDITOR_ERROR_TAG(LOG_TAG, "Could not create the airport project: {}", error);
						continue;
					}
					EDITOR_INFO_TAG(LOG_TAG, "{} exists already: opening it", VFS::ToUtf8(projectFile));
				}
				EditorProjects::OpenWhenReady(projectFile);
			}
			else if (args[i] == "--show-dialog" && i + 1 < args.size())
				S().showOnStartup = args[++i];
			else if (args[i] == "--export-scenery")
			{
				S().exportOnStartup = true;
				if (i + 1 < args.size() && !args[i + 1].starts_with("--"))
					S().startupExportFolder = args[++i];
			}
		}
	}

	void Tick(EditorContext *context)
	{
		(void)context;
		UpdateMounts();

		// --show-dialog: once the editor is up (the export dialog once a project is open)
		State &s = S();
		if (!s.showOnStartup.empty() && ++s.showFrames > 30)
		{
			if (s.showOnStartup == "new")
			{
				OpenNewProject();
				s.showOnStartup.clear();
			}
			else if (s.showOnStartup == "export" && EditorProjects::GetDocument())
			{
				OpenExport();
				s.showOnStartup.clear();
			}
		}

		// --export-scenery: once the project is open and its scene has settled
		if (!s.exportOnStartup || !EditorProjects::GetDocument())
			return;
		if (++s.startupExportFrames < 120)
			return;
		s.exportOnStartup = false;
		ExportReport report;
		std::string error;
		const fs::path folder = s.startupExportFolder.empty() ? EditorProjects::GetProjectFolder() : VFS::FromUtf8(s.startupExportFolder);
		RunExport(folder, ExportOptions(), false, report, error);
	}

	void OpenNewProject()
	{
		State &s = S();
		if (s.form.location.empty())
			s.form.location = DefaultLocation();
		s.form.attempted = false;
		s.form.error.clear();
		s.openNew = true;
	}

	void OpenExport()
	{
		if (!EditorProjects::GetDocument())
			return;
		State &s = S();
		PrepareExportFolder();
		s.exporter.report.reset();
		s.exporter.error.clear();
		s.openExport = true;
	}

	bool QuickExport()
	{
		if (!EditorProjects::GetDocument())
			return false;
		State &s = S();
		PrepareExportFolder();
		ExportReport report;
		std::string error;
		const fs::path folder = VFS::FromUtf8(s.exporter.folder);
		const fs::path root = XPlaneRoot();
		const fs::path customScenery = root.empty() ? fs::path() : VFS::GetCustomSceneryFolder(root);
		const bool inCustomScenery = !customScenery.empty() && VFS::IsInside(folder.parent_path(), customScenery) && VFS::IsInside(customScenery, folder.parent_path());
		if (!RunExport(folder, s.exporter.options, s.exporter.registerPack && inCustomScenery, report, error))
			return false;
		s.exporter.report = std::move(report);
		return true;
	}

	void Draw(EditorContext *context)
	{
		State &s = S();
		if (s.openNew)
		{
			ImGui::OpenPopup(NEW_MODAL);
			s.openNew = false;
		}
		if (s.openExport)
		{
			ImGui::OpenPopup(EXPORT_MODAL);
			s.openExport = false;
		}
		DrawNewProject(context);
		DrawExport();
	}
}

// -------------------------------------------------------

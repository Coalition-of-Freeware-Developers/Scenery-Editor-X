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
 * xplane_library_panel.cpp
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#include "xplane_library_panel.h"
#include <Editor/core/editor_theme.h>
#include <Editor/core/editor_tools.h>
#include <Editor/core/editor_ui.h>
#include <Editor/core/editor_xplane.h>
#include <Editor/core/facade_tool.h>
#include <SceneryEditorX/filesystem/file_system.h>
#include <SceneryEditorX/renderer/font/font_awesome.h>
#include <SceneryEditorX/renderer/ui/resources/fonts.h>
#include <SceneryEditorX/renderer/ui/source/imgui/misc/cpp/imgui_stdlib.h>
#include <SceneryEditorX/xplane/xp_library_manager.h>
#include <SceneryEditorX/xplane/xp_object.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		using namespace XPlane;

		ImVec4 Color(const ImVec4 &srgb)
		{
			return EditorTheme::ToStyle(srgb);
		}

		/* @brief Path of a file relative to its package, for display. */
		std::string RelativeToPackage(const std::filesystem::path &file, const LibraryPackage &package)
		{
			std::error_code ec;
			const std::filesystem::path relative = std::filesystem::relative(file, package.path, ec);
			return ec || relative.empty() ? file.string() : relative.generic_string();
		}

		void StatusIcon(const bool ok, const char *tooltip)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, Color(ok ? EditorTheme::Palette::Accent : EditorTheme::Palette::Error));
			ImGui::TextUnformatted(ok ? SEDX_ICON_CHECK_CIRCLE : SEDX_ICON_EXCLAMATION_TRIANGLE);
			ImGui::PopStyleColor();
			if (tooltip && *tooltip)
				EditorUI::ItemTooltip(tooltip);
		}

		const char *TypeIcon(const XPlane::AssetType type)
		{
			switch (type)
			{
				case XPlane::AssetType::Object:			return SEDX_ICON_CUBE;
				case XPlane::AssetType::Facade:			return SEDX_ICON_BUILDING_O;
				case XPlane::AssetType::Forest:			return SEDX_ICON_TREE;
				case XPlane::AssetType::Line:
				case XPlane::AssetType::String:			return SEDX_ICON_ROAD;
				case XPlane::AssetType::Network:		return SEDX_ICON_ROAD;
				case XPlane::AssetType::Polygon:
				case XPlane::AssetType::Terrain:
				case XPlane::AssetType::Beach:
				case XPlane::AssetType::Decal:			return SEDX_ICON_MAP_O;
				case XPlane::AssetType::AutogenPoint:
				case XPlane::AssetType::AutogenBlock:
				case XPlane::AssetType::AutogenString:	return SEDX_ICON_CUBES;
				default:						return SEDX_ICON_FILE_O;
			}
		}
	}

	XPlaneLibraryPanel::XPlaneLibraryPanel(EditorContext *context) : m_Context(context)
	{
		m_Title = "X-PLANE LIBRARY";
	}

	void XPlaneLibraryPanel::OnUIRender(bool &isOpen)
	{
		if (EditorUI::BeginPanel("X-PLANE LIBRARY###XPlaneLibrary", &isOpen))
		{
			DrawInstallBar();

			const std::shared_ptr<const LibraryIndex> index = LibraryManager::GetIndex();
			if (ImGui::BeginTabBar("##XPlaneTabs"))
			{
				if (ImGui::BeginTabItem("Assets", nullptr, m_SelectTab ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None))
				{
					DrawAssetsTab(*index);
					ImGui::EndTabItem();
				}
				m_SelectTab = false;

				if (ImGui::BeginTabItem("Libraries"))
				{
					DrawLibrariesTab(*index);
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Simulation"))
				{
					if (ImGui::BeginChild("##Simulation"))
					{
						EditorXPlane::DrawSimulationControls();
						ImGui::Spacing();
						ImGui::SeparatorText("Datarefs");
						EditorXPlane::DrawDatarefTable("##AllDatarefs", nullptr);
					}
					ImGui::EndChild();
					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}
		}
		EditorUI::EndPanel();
	}

	// -------------------------------------------------------

	void XPlaneLibraryPanel::DrawInstallBar()
	{
		const InstallInfo &install = LibraryManager::GetInstallInfo();

		ImGui::AlignTextToFramePadding();
		StatusIcon(install.IsValid(), install.IsValid() ? "X-Plane 12 folder found" : install.GetProblem().c_str());
		ImGui::SameLine();
		ImGui::TextUnformatted(install.IsValid() ? (install.version.empty() ? "X-Plane 12" : ("X-Plane " + install.version).c_str()) : "X-Plane 12");
		ImGui::SameLine();

		// Editable path: Enter applies it.
		static std::string s_PathEdit;
		static bool s_EditingPath = false;
		const float buttons = ImGui::CalcTextSize("Browse...Detect Rescan").x + ImGui::GetStyle().FramePadding.x * 6.0f + ImGui::GetStyle().ItemSpacing.x * 3.0f + 60.0f;
		ImGui::SetNextItemWidth(std::max(120.0f, ImGui::GetContentRegionAvail().x - buttons));
		if (!s_EditingPath)
			s_PathEdit = install.root.string();
		if (ImGui::InputTextWithHint("##InstallPath", "X-Plane 12 folder", &s_PathEdit, ImGuiInputTextFlags_EnterReturnsTrue))
		{
			if (!LibraryManager::SetInstallPath(s_PathEdit))
				s_PathEdit = install.root.string();
		}
		s_EditingPath = ImGui::IsItemActive();
		EditorUI::ItemTooltip(install.IsValid() ? install.root.string().c_str() : "Type or browse to your X-Plane 12 folder, then press Enter");

		ImGui::SameLine();
		if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_FOLDER_OPEN, "Browse...").c_str()))
			EditorXPlane::BrowseForInstall();

		ImGui::SameLine();
		static std::vector<std::filesystem::path> s_Candidates;
		if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_MAGIC, "Detect").c_str()))
		{
			s_Candidates = FindInstalls();
			ImGui::OpenPopup("##DetectedInstalls");
		}
		EditorUI::ItemTooltip("Find X-Plane 12 installs (x-plane_install_12.txt and Steam libraries)");

		if (ImGui::BeginPopup("##DetectedInstalls"))
		{
			if (s_Candidates.empty())
				ImGui::TextDisabled("No X-Plane 12 installation found");
			for (const std::filesystem::path &candidate : s_Candidates)
			{
				const InstallInfo info = InspectInstall(candidate);
				ImGui::BeginDisabled(!info.IsValid());
				const std::string label = candidate.string() + (info.version.empty() ? std::string() : "  (" + info.version + ")");
				if (ImGui::Selectable(label.c_str(), candidate == install.root))
					LibraryManager::SetInstallPath(candidate);
				ImGui::EndDisabled();
			}
			ImGui::EndPopup();
		}

		ImGui::SameLine();
		const bool scanning = LibraryManager::GetScanState() == ScanState::Scanning;
		ImGui::BeginDisabled(scanning);
		if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_REFRESH, "Rescan").c_str()))
			LibraryManager::Rescan();
		ImGui::EndDisabled();

		// Scan status
		if (scanning)
		{
			static const char *spinner = "|/-\\";
			ImGui::TextColored(Color(EditorTheme::Palette::Warning), "%c Scanning libraries...", spinner[static_cast<int>(ImGui::GetTime() * 8.0) & 3]);
		}
		else
		{
			ImGui::TextDisabled("%s  (%.0f ms)", LibraryManager::GetScanMessage().c_str(), LibraryManager::GetLastScanMilliseconds());
		}
	}

	// -------------------------------------------------------

	void XPlaneLibraryPanel::RefreshFilter(const LibraryIndex &index)
	{
		const std::string key = std::string(m_Filter.InputBuf) + '|' + std::to_string(m_TypeFilter) + '|' + std::to_string(m_PackageFilter) + '|' +
								std::to_string(m_ShowHidden) + '|' + std::to_string(m_OnlyMissing);
		const uint64_t generation = LibraryManager::GetGeneration();
		if (key == m_FilterKey && generation == m_FilterGeneration)
			return;

		m_FilterKey = key;
		m_FilterGeneration = generation;
		m_Filtered.clear();

		const std::vector<LibraryAsset> &assets = index.GetAssets();
		for (uint32_t i = 0; i < static_cast<uint32_t>(assets.size()); ++i)
		{
			const LibraryAsset &asset = assets[i];
			if (m_TypeFilter >= 0 && asset.type != static_cast<XPlane::AssetType>(m_TypeFilter))
				continue;
			if (!m_ShowHidden && (asset.IsPrivate() || asset.IsDeprecated()))
				continue;
			if (m_PackageFilter >= 0 && std::ranges::none_of(asset.providers, [this](const AssetProvider &p) { return static_cast<int>(p.package) == m_PackageFilter; }))
				continue;
			if (m_OnlyMissing)
			{
				const AssetProvider *resolved = asset.GetResolved();
				if (!resolved || resolved->fileExists)
					continue;
			}
			if (!m_Filter.PassFilter(asset.virtualPath.c_str()))
				continue;
			m_Filtered.push_back(i);
		}
	}

	void XPlaneLibraryPanel::DrawAssetsTab(const LibraryIndex &index)
	{
		const std::vector<LibraryPackage> &packages = index.GetPackages();
		if (m_PackageFilter >= static_cast<int>(packages.size()))
			m_PackageFilter = -1;

		// Filters
		EditorUI::SearchFilter("##AssetSearch", m_Filter, ImGui::GetContentRegionAvail().x * 0.35f);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(130.0f * ImGui::GetStyle().FontScaleDpi);
		if (ImGui::BeginCombo("##Type", m_TypeFilter < 0 ? "All types" : GetAssetTypeName(static_cast<XPlane::AssetType>(m_TypeFilter))))
		{
			if (ImGui::Selectable("All types", m_TypeFilter < 0))
				m_TypeFilter = -1;
			for (int type = 0; type < static_cast<int>(XPlane::AssetType::Count); ++type)
			{
				const std::string label = std::string(TypeIcon(static_cast<XPlane::AssetType>(type))) + "  " + GetAssetTypeName(static_cast<XPlane::AssetType>(type));
				if (ImGui::Selectable(label.c_str(), m_TypeFilter == type))
					m_TypeFilter = type;
			}
			ImGui::EndCombo();
		}

		ImGui::SameLine();
		ImGui::SetNextItemWidth(200.0f * ImGui::GetStyle().FontScaleDpi);
		if (ImGui::BeginCombo("##Library", m_PackageFilter < 0 ? "All libraries" : packages[m_PackageFilter].name.c_str()))
		{
			if (ImGui::Selectable("All libraries", m_PackageFilter < 0))
				m_PackageFilter = -1;
			for (int i = 0; i < static_cast<int>(packages.size()); ++i)
			{
				if (!packages[i].hasLibrary || !packages[i].tracked)
					continue;
				ImGui::PushID(i);
				if (ImGui::Selectable(packages[i].name.c_str(), m_PackageFilter == i))
					m_PackageFilter = i;
				ImGui::PopID();
			}
			ImGui::EndCombo();
		}

		ImGui::SameLine();
		ImGui::Checkbox("Private / deprecated", &m_ShowHidden);
		ImGui::SameLine();
		ImGui::Checkbox("Missing files", &m_OnlyMissing);

		RefreshFilter(index);
		ImGui::SameLine();
		ImGui::TextDisabled("%zu / %zu", m_Filtered.size(), index.GetAssets().size());

		// Objects placed from here load in the background.
		EditorXPlane::DrawObjectLoadProgress(-FLT_MIN);

		if (index.GetAssets().empty())
		{
			EditorUI::EmptyState(LibraryManager::GetInstallInfo().IsValid() ? "No library assets yet (scanning, or no tracked libraries)."
																			  : "Set the X-Plane 12 folder above to browse its libraries.");
			return;
		}

		// List (left) and details (right)
		const float dpi = ImGui::GetStyle().FontScaleDpi;
		const float listWidth = std::max(260.0f * dpi, ImGui::GetContentRegionAvail().x * 0.58f);
		if (ImGui::BeginChild("##AssetList", ImVec2(listWidth, 0.0f), ImGuiChildFlags_ResizeX))
		{
			constexpr ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV;
			if (ImGui::BeginTable("##Assets", 3, flags))
			{
				ImGui::TableSetupScrollFreeze(0, 1);
				ImGui::TableSetupColumn("Virtual path", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("Library", ImGuiTableColumnFlags_WidthFixed, 150.0f * dpi);
				ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 24.0f * dpi);
				ImGui::TableHeadersRow();

				const std::vector<LibraryAsset> &assets = index.GetAssets();
				ImGuiListClipper clipper;
				clipper.Begin(static_cast<int>(m_Filtered.size()));
				while (clipper.Step())
				{
					for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
					{
						const LibraryAsset &asset = assets[m_Filtered[row]];
						const AssetProvider *resolved = asset.GetResolved();
						const bool missing = resolved && !resolved->fileExists;

						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::PushID(row);

						if (missing)
							ImGui::PushStyleColor(ImGuiCol_Text, Color(EditorTheme::Palette::Error));
						const std::string label = std::string(TypeIcon(asset.type)) + "  " + asset.virtualPath;
						const bool selected = m_SelectedAsset == asset.virtualPath;
						if (ImGui::Selectable(label.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick))
						{
							m_SelectedAsset = asset.virtualPath;
							if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && asset.type == XPlane::AssetType::Object)
								EditorXPlane::PlaceLibraryAsset(asset.virtualPath, m_Context);
							// A facade is armed in the facade tool, ready to draw
							if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && asset.type == XPlane::AssetType::Facade)
							{
								FacadeTool::SetResource(asset.virtualPath);
								EditorTools::SetTool(EditorTools::Tool::Facade);
							}
						}
						if (missing)
							ImGui::PopStyleColor();

						const bool draggable = asset.type == XPlane::AssetType::Object || asset.type == XPlane::AssetType::Facade;
						if (draggable && ImGui::BeginDragDropSource())
						{
							ImGui::SetDragDropPayload(EditorXPlane::ASSET_PAYLOAD, asset.virtualPath.c_str(), asset.virtualPath.size() + 1);
							ImGui::Text("%s  %s", asset.type == XPlane::AssetType::Facade ? SEDX_ICON_BUILDING_O : SEDX_ICON_CUBE, asset.virtualPath.c_str());
							ImGui::EndDragDropSource();
						}

						ImGui::TableNextColumn();
						ImGui::TextDisabled("%s", resolved ? packages[resolved->package].name.c_str() : "-");

						ImGui::TableNextColumn();
						ImGui::TextDisabled("%zu", asset.providers.size());

						ImGui::PopID();
					}
				}
				ImGui::EndTable();
			}
		}
		ImGui::EndChild();

		ImGui::SameLine();
		if (ImGui::BeginChild("##AssetDetails", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders))
		{
			const LibraryAsset *selected = m_SelectedAsset.empty() ? nullptr : index.Find(m_SelectedAsset);
			if (selected)
				DrawAssetDetails(index, *selected);
			else
				EditorUI::EmptyState("Select an asset to see where it comes from.\nDouble-click or drag an object into the viewport to place it.\n"
									 "Double-click a facade to draw with it, or drag it onto the ground.");
		}
		ImGui::EndChild();
	}

	void XPlaneLibraryPanel::DrawAssetDetails(const LibraryIndex &index, const LibraryAsset &asset)
	{
		const std::vector<LibraryPackage> &packages = index.GetPackages();
		const AssetProvider *resolved = asset.GetResolved();

		UI::Fonts::PushFont("Bold");
		ImGui::TextWrapped("%s", asset.virtualPath.c_str());
		UI::Fonts::PopFont();
		ImGui::TextDisabled("%s  %s%s%s", TypeIcon(asset.type), GetAssetTypeName(asset.type), asset.IsPrivate() ? "  |  private" : "",
			asset.IsDeprecated() ? "  |  deprecated" : "");

		// Actions
		const bool canPlace = asset.type == XPlane::AssetType::Object && resolved && resolved->fileExists;
		if (asset.type == XPlane::AssetType::Facade)
		{
			// Facades are drawn into the project with the facade tool
			ImGui::BeginDisabled(!resolved || !resolved->fileExists);
			if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_PENCIL, "Draw with it").c_str()))
			{
				FacadeTool::SetResource(asset.virtualPath);
				EditorTools::SetTool(EditorTools::Tool::Facade);
			}
			EditorUI::ItemTooltip("Arm the facade tool with it (B): draw a polygon, rectangle, circle or line in the viewport.\n"
								  "Or drag it onto the ground for a default footprint");
			ImGui::SameLine();
			if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_INFO_CIRCLE, "Inspect").c_str()))
				FacadeTool::Inspect(asset.virtualPath);
			EditorUI::ItemTooltip("Floors, walls and their rules, spellings and segments in the FACADES inspector");
			ImGui::EndDisabled();
		}
		else
		{
			ImGui::BeginDisabled(!canPlace);
			if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_PLUS, "Place in scene").c_str()))
				EditorXPlane::PlaceLibraryAsset(asset.virtualPath, m_Context);
			ImGui::EndDisabled();
			if (asset.type != XPlane::AssetType::Object)
				EditorUI::ItemTooltip("Only objects (.obj) and facades (.fac) can be placed for now");
			else
				EditorUI::ItemTooltip("Places the object right away; it appears when it has loaded in the background");
		}

		ImGui::SameLine();
		if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_LINK, "Copy path").c_str()))
			ImGui::SetClipboardText(asset.virtualPath.c_str());
		EditorUI::ItemTooltip("Copy the virtual path (for DSF / WED placements)");

		ImGui::SameLine();
		ImGui::BeginDisabled(!resolved);
		if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_FOLDER_OPEN_O, "Show file").c_str()) && resolved)
			IO::FileSystem::OpenExternally(resolved->realPath.parent_path());
		ImGui::EndDisabled();

		if (canPlace)
		{
			const XPlane::ObjectLoadState state = XPlane::ObjectAsset::GetLoadState(resolved->realPath);
			if (state == XPlane::ObjectLoadState::Queued || state == XPlane::ObjectLoadState::Decoding || state == XPlane::ObjectLoadState::Uploading)
				ImGui::TextColored(Color(EditorTheme::Palette::Warning), "%s %s...", SEDX_ICON_HOURGLASS_HALF, XPlane::GetObjectLoadStateName(state));
			else if (state == XPlane::ObjectLoadState::Ready)
				ImGui::TextDisabled("%s Loaded", SEDX_ICON_CHECK);
		}

		// Where it comes from
		ImGui::SeparatorText("Exported by");
		constexpr ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit;
		if (ImGui::BeginTable("##Providers", 4, flags))
		{
			ImGui::TableSetupColumn("Library", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Export");
			ImGui::TableSetupColumn("Region / season");
			ImGui::TableSetupColumn("File");
			ImGui::TableHeadersRow();

			for (size_t i = 0; i < asset.providers.size(); ++i)
			{
				const AssetProvider &provider = asset.providers[i];
				const LibraryPackage &package = packages[provider.package];
				ImGui::TableNextRow();
				ImGui::PushID(static_cast<int>(i));

				ImGui::TableNextColumn();
				const bool isResolved = resolved == &provider;
				if (isResolved)
					ImGui::PushStyleColor(ImGuiCol_Text, Color(EditorTheme::Palette::AccentHovered));
				ImGui::TextUnformatted(package.name.c_str());
				if (isResolved)
					ImGui::PopStyleColor();
				EditorUI::ItemTooltip((isResolved ? "Used by X-Plane (highest priority)\n" + RelativeToPackage(provider.realPath, package) : RelativeToPackage(provider.realPath, package)).c_str());

				ImGui::TableNextColumn();
				if (provider.ratio != 1.0f)
					ImGui::Text("%s x%.2g", GetExportKindName(provider.kind), provider.ratio);
				else
					ImGui::TextUnformatted(GetExportKindName(provider.kind));

				ImGui::TableNextColumn();
				ImGui::TextDisabled("%s / %s", provider.region.empty() ? "world" : provider.region.c_str(), FormatSeasons(provider.seasons).c_str());

				ImGui::TableNextColumn();
				StatusIcon(provider.fileExists, provider.fileExists ? provider.realPath.string().c_str() : ("Missing: " + provider.realPath.string()).c_str());

				ImGui::PopID();
			}
			ImGui::EndTable();
		}

		// Object preview: parsed on selection (CPU only, nothing is uploaded until the object is placed)
		if (asset.type != XPlane::AssetType::Object || !resolved || !resolved->fileExists)
			return;

		const std::string file = resolved->realPath.string();
		if (m_PreviewPath != file)
		{
			m_PreviewPath = file;
			m_Preview = std::make_unique<Obj8Data>();
			m_PreviewError.clear();
			if (!LoadObj8(resolved->realPath, *m_Preview, &m_PreviewError))
				m_Preview.reset();
		}

		ImGui::SeparatorText("Object");
		if (!m_Preview)
		{
			ImGui::TextColored(Color(EditorTheme::Palette::Error), "%s", m_PreviewError.c_str());
			return;
		}

		const Obj8Data &data = *m_Preview;
		if (EditorUI::BeginPropertyGrid("##ObjPreview", 110.0f))
		{
			EditorUI::PropertyLabel("Geometry");
			ImGui::Text("%u triangles, %zu vertices, %zu batches", data.GetTriangleCount(), data.vertices.size(), data.batches.size());

			EditorUI::PropertyLabel("Texture");
			ImGui::TextWrapped("%s", data.texture.empty() ? "(none: gray proxy)" : data.texture.c_str());
			if (!data.textureNormal.empty())
			{
				EditorUI::PropertyLabel("Normal map");
				ImGui::TextWrapped("%s%s", data.textureNormal.c_str(), data.normalMetalness ? "  (metalness)" : "");
			}
			if (!data.textureLit.empty())
			{
				EditorUI::PropertyLabel("Lit texture");
				ImGui::TextWrapped("%s", data.textureLit.c_str());
			}

			EditorUI::PropertyLabel("LODs");
			if (data.lods.empty())
				ImGui::TextUnformatted("single");
			else
			{
				std::string text = data.HasAdditiveLods() ? "additive: " : "selective: ";
				for (size_t i = 0; i < data.lods.size(); ++i)
					text += (i ? ", " : "") + std::to_string(static_cast<int>(data.lods[i].nearDistance)) + "-" + std::to_string(static_cast<int>(data.lods[i].farDistance)) + " m";
				ImGui::TextWrapped("%s", text.c_str());
			}

			EditorUI::PropertyLabel("Animation");
			if (data.IsAnimated())
				ImGui::Text("%zu groups, %zu hide/show rules, %zu datarefs", data.nodes.size(), data.visibility.size(), data.datarefs.size());
			else
				ImGui::TextUnformatted("static");

			if (!data.lights.empty() || data.manipulatorCount > 0)
			{
				EditorUI::PropertyLabel("Other");
				ImGui::Text("%zu lights, %u manipulators", data.lights.size(), data.manipulatorCount);
			}

			if (!data.warnings.empty())
			{
				EditorUI::PropertyLabel("Warnings");
				ImGui::TextColored(Color(EditorTheme::Palette::Warning), "%zu", data.warnings.size());
				if (ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					for (const std::string &warning : data.warnings)
						ImGui::TextUnformatted(warning.c_str());
					ImGui::EndTooltip();
				}
			}

			EditorUI::EndPropertyGrid();
		}

		if (!data.datarefs.empty() && ImGui::TreeNode("Datarefs"))
		{
			for (const Obj8DatarefUsage &usage : data.datarefs)
				ImGui::BulletText("%s  [%.3g .. %.3g]%s", usage.dataref.ToString().c_str(), usage.minValue, usage.maxValue, usage.loop > 0.0f ? "  loop" : "");
			ImGui::TreePop();
		}
	}

	// -------------------------------------------------------

	void XPlaneLibraryPanel::DrawLibrariesTab(const LibraryIndex &index)
	{
		const std::vector<LibraryPackage> &packages = index.GetPackages();
		const float dpi = ImGui::GetStyle().FontScaleDpi;

		if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_PLUS, "Track folder...").c_str()))
			EditorXPlane::BrowseForTrackedFolder();
		EditorUI::ItemTooltip("Track a library folder outside X-Plane (for example a library you are building)");

		ImGui::SameLine();
		EditorUI::SearchFilter("##LibrarySearch", m_LibraryFilter, ImGui::GetContentRegionAvail().x * 0.3f);
		ImGui::SameLine();
		ImGui::Checkbox("All packages", &m_ShowAllPackages);
		EditorUI::ItemTooltip("Also list scenery packages without a library.txt");
		ImGui::SameLine();
		bool checkFiles = LibraryManager::GetCheckFiles();
		if (ImGui::Checkbox("Check files", &checkFiles))
			LibraryManager::SetCheckFiles(checkFiles);
		EditorUI::ItemTooltip("Verify that every exported file exists (flags broken libraries; slower scans)");

		ImGui::SameLine();
		uint32_t libraries = 0, tracked = 0;
		for (const LibraryPackage &package : packages)
		{
			libraries += package.hasLibrary ? 1 : 0;
			tracked += package.hasLibrary && package.tracked ? 1 : 0;
		}
		ImGui::TextDisabled("%u tracked / %u libraries, %u exports, %u missing", tracked, libraries, index.GetExportCount(), index.GetMissingFileCount());

		constexpr ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit;
		if (!ImGui::BeginTable("##Libraries", 8, flags))
			return;

		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("Track", ImGuiTableColumnFlags_WidthFixed, 40.0f * dpi);
		ImGui::TableSetupColumn("Library", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Source", ImGuiTableColumnFlags_WidthFixed, 110.0f * dpi);
		ImGui::TableSetupColumn("In sim", ImGuiTableColumnFlags_WidthFixed, 60.0f * dpi);
		ImGui::TableSetupColumn("Assets", ImGuiTableColumnFlags_WidthFixed, 60.0f * dpi);
		ImGui::TableSetupColumn("Objects", ImGuiTableColumnFlags_WidthFixed, 60.0f * dpi);
		ImGui::TableSetupColumn("Missing", ImGuiTableColumnFlags_WidthFixed, 60.0f * dpi);
		ImGui::TableSetupColumn("Issues", ImGuiTableColumnFlags_WidthFixed, 50.0f * dpi);
		ImGui::TableHeadersRow();

		for (int i = 0; i < static_cast<int>(packages.size()); ++i)
		{
			const LibraryPackage &package = packages[i];
			if (!package.hasLibrary && !m_ShowAllPackages)
				continue;
			if (!m_LibraryFilter.PassFilter(package.name.c_str()))
				continue;

			ImGui::TableNextRow();
			ImGui::PushID(i);

			ImGui::TableNextColumn();
			ImGui::BeginDisabled(!package.hasLibrary);
			bool isTracked = package.tracked;
			if (ImGui::Checkbox("##Track", &isTracked))
				LibraryManager::SetPackageTracked(package.path, isTracked);
			ImGui::EndDisabled();

			ImGui::TableNextColumn();
			if (!package.tracked)
				ImGui::PushStyleColor(ImGuiCol_Text, Color(EditorTheme::Palette::TextDisabled));
			if (ImGui::Selectable(package.name.c_str(), m_PackageFilter == i, ImGuiSelectableFlags_AllowDoubleClick))
			{
				if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && package.tracked)
				{
					m_PackageFilter = i;
					m_TypeFilter = -1;
					m_SelectTab = true;
				}
			}
			if (!package.tracked)
				ImGui::PopStyleColor();
			EditorUI::ItemTooltip((package.path.string() + "\nDouble-click to browse its assets").c_str());

			if (ImGui::BeginPopupContextItem("##LibraryMenu"))
			{
				if (ImGui::MenuItem("Browse assets", nullptr, false, package.tracked && package.hasLibrary))
				{
					m_PackageFilter = i;
					m_TypeFilter = -1;
					m_SelectTab = true;
				}
				if (ImGui::MenuItem("Open folder"))
					IO::FileSystem::OpenExternally(package.path);
				if (package.source == PackageSource::Tracked && ImGui::MenuItem("Stop tracking folder"))
					LibraryManager::RemoveTrackedFolder(package.path);
				ImGui::EndPopup();
			}

			ImGui::TableNextColumn();
			ImGui::TextDisabled("%s", GetPackageSourceName(package.source));

			ImGui::TableNextColumn();
			if (package.source == PackageSource::CustomScenery || package.source == PackageSource::GlobalScenery)
				ImGui::TextDisabled("%s", package.enabledInSim ? "yes" : "no");
			else
				ImGui::TextDisabled("-");

			ImGui::TableNextColumn();
			if (package.tracked && package.hasLibrary)
				ImGui::Text("%u", package.assetCount);

			ImGui::TableNextColumn();
			if (package.tracked && package.hasLibrary)
				ImGui::Text("%u", package.typeCounts[static_cast<size_t>(XPlane::AssetType::Object)]);

			ImGui::TableNextColumn();
			if (package.missingFileCount > 0)
				ImGui::TextColored(Color(EditorTheme::Palette::Error), "%u", package.missingFileCount);

			ImGui::TableNextColumn();
			const size_t issues = package.warnings.size() + (package.error.empty() ? 0 : 1);
			if (issues > 0)
			{
				ImGui::TextColored(Color(package.error.empty() ? EditorTheme::Palette::Warning : EditorTheme::Palette::Error), "%s %zu", SEDX_ICON_EXCLAMATION_TRIANGLE, issues);
				if (ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					if (!package.error.empty())
						ImGui::TextUnformatted(package.error.c_str());
					for (const std::string &warning : package.warnings)
						ImGui::TextUnformatted(warning.c_str());
					ImGui::EndTooltip();
				}
			}

			ImGui::PopID();
		}

		ImGui::EndTable();
	}

}

// -------------------------------------------------------

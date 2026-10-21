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
#include <optional>
#include <Editor/core/asset_thumbnails.h>

#include <Editor/core/editor_project.h>
#include <Editor/core/editor_theme.h>
#include <Editor/core/editor_tools.h>
#include <Editor/core/editor_ui.h>
#include <Editor/core/editor_xplane.h>
#include <Editor/core/facade_tool.h>
#include <SceneryEditorX/core/application/application.h>
#include <SceneryEditorX/filesystem/file_system.h>
#include <SceneryEditorX/renderer/font/font_awesome.h>
#include <SceneryEditorX/renderer/ui/resources/fonts.h>
#include <SceneryEditorX/renderer/ui/source/imgui/imgui_internal.h>
#include <SceneryEditorX/renderer/ui/source/imgui/misc/cpp/imgui_stdlib.h>
#include <SceneryEditorX/project/project_document.h>
#include <SceneryEditorX/xplane/xp_library_manager.h>
#include <SceneryEditorX/xplane/xp_object.h>
#include <SceneryEditorX/xplane/xp_thumbnail.h>

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

		/* @brief How saved filters name the asset types: their file extensions (AssetType order). */
		constexpr const char *TYPE_IDS[] = { "obj", "fac", "for", "lin", "pol", "str", "agp", "agb", "ags", "net", "ter", "bch", "dcl", "other" };
		static_assert(std::size(TYPE_IDS) == static_cast<size_t>(XPlane::AssetType::Count), "a type id per AssetType");

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

		// --library-search <text>: start with a search; --library-grid: start in the thumbnail grid (screenshots and checks without typing)
		if (const PlatformContext *platform = Application::Get().GetPlatformContext())
		{
			const std::vector<std::string> &args = platform->GetCommandLineArgs();
			for (size_t i = 0; i < args.size(); ++i)
			{
				if (args[i] == "--library-search" && i + 1 < args.size())
				{
					std::snprintf(m_Filter.InputBuf, sizeof(m_Filter.InputBuf), "%s", args[i + 1].c_str());
					m_Filter.Build();
				}
				else if (args[i] == "--library-grid")
					m_GridView = true;
			}
		}
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
		StatusIcon(install.IsValid(), install.IsValid() ? "X-Plane folder found" : install.GetProblem().c_str());
		ImGui::SameLine();
		ImGui::TextUnformatted(install.IsValid() && !install.version.empty() ? ("X-Plane " + install.version).c_str() : "X-Plane");
		ImGui::SameLine();

		// Editable path: Enter applies it.
		static std::string s_PathEdit;
		static bool s_EditingPath = false;
		const float buttons = ImGui::CalcTextSize("Browse...Detect Rescan").x + ImGui::GetStyle().FramePadding.x * 6.0f + ImGui::GetStyle().ItemSpacing.x * 3.0f + 60.0f;
		ImGui::SetNextItemWidth(std::max(120.0f, ImGui::GetContentRegionAvail().x - buttons));
		if (!s_EditingPath)
			s_PathEdit = install.root.string();
		if (ImGui::InputTextWithHint("##InstallPath", "X-Plane folder", &s_PathEdit, ImGuiInputTextFlags_EnterReturnsTrue))
		{
			if (!LibraryManager::SetInstallPath(s_PathEdit))
				s_PathEdit = install.root.string();
		}
		s_EditingPath = ImGui::IsItemActive();
		EditorUI::ItemTooltip(install.IsValid() ? install.root.string().c_str() : "Type or browse to your X-Plane 11 or 12 folder, then press Enter");

		ImGui::SameLine();
		if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_FOLDER_OPEN, "Browse...").c_str()))
			EditorXPlane::BrowseForInstall();

		ImGui::SameLine();
		static std::vector<InstallCandidate> s_Candidates;
		if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_MAGIC, "Detect").c_str()))
		{
			s_Candidates = FindInstallCandidates();
			ImGui::OpenPopup("##DetectedInstalls");
		}
		EditorUI::ItemTooltip("Find X-Plane 11 and 12 installs: X-Plane's own install lists, Steam libraries, the registry and the usual folders");

		if (ImGui::BeginPopup("##DetectedInstalls"))
		{
			if (s_Candidates.empty())
				ImGui::TextDisabled("No X-Plane installation found");
			for (const InstallCandidate &candidate : s_Candidates)
			{
				const InstallInfo info = InspectInstall(candidate.root);
				const std::string version = !info.version.empty() ? info.version : candidate.majorVersion > 0 ? std::to_string(candidate.majorVersion) : "?";
				const std::string label = candidate.root.string() + "  (X-Plane " + version + ", " + GetInstallSourceName(candidate.source) + ")";
				ImGui::BeginDisabled(!info.IsValid());
				if (ImGui::Selectable(label.c_str(), candidate.root == install.root))
					LibraryManager::SetInstallPath(candidate.root);
				ImGui::EndDisabled();
				if (!info.IsValid())
					EditorUI::ItemTooltip(info.GetProblem().c_str());
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
		// A new search opens the libraries on their matches again
		if (m_LastSearch != m_Filter.InputBuf)
		{
			m_LastSearch = m_Filter.InputBuf;
			m_GroupOpen.clear();
		}

		const std::string key = GetFilterKey();
		const uint64_t generation = LibraryManager::GetGeneration();
		if (key != m_FilterKey || generation != m_FilterGeneration)
		{
			m_FilterKey = key;
			m_FilterGeneration = generation;
			m_Groups.clear();
			m_MatchCount = 0;

			// One group per package, in priority order; assets no library resolves last
			const std::vector<LibraryPackage> &packages = index.GetPackages();
			std::vector<int> groupOf(packages.size() + 1, -1);
			const auto groupFor = [&](const int package) -> Group &
			{
				const size_t slot = package < 0 ? packages.size() : static_cast<size_t>(package);
				if (groupOf[slot] < 0)
				{
					groupOf[slot] = static_cast<int>(m_Groups.size());
					m_Groups.push_back({ package, 0, {} });
				}
				return m_Groups[static_cast<size_t>(groupOf[slot])];
			};

			const std::vector<LibraryAsset> &assets = index.GetAssets();
			for (uint32_t i = 0; i < static_cast<uint32_t>(assets.size()); ++i)
			{
				const LibraryAsset &asset = assets[i];
				const AssetProvider *resolved = asset.GetResolved();
				Group &group = groupFor(resolved ? static_cast<int>(resolved->package) : -1);
				++group.total;

				if ((m_TypeMask & (1u << static_cast<uint32_t>(asset.type))) == 0)
					continue;
				if (!m_ShowHidden && (asset.IsPrivate() || asset.IsDeprecated()))
					continue;
				if (!m_Libraries.empty() && std::ranges::none_of(asset.providers, [&](const AssetProvider &p)
					{ return p.package < packages.size() && std::ranges::find(m_Libraries, packages[p.package].name) != m_Libraries.end(); }))
					continue;
				if (m_OnlyMissing && (!resolved || resolved->fileExists))
					continue;
				if (!m_Filter.PassFilter(asset.virtualPath.c_str()))
					continue;
				group.assets.push_back(i);
				++m_MatchCount;
			}

			std::erase_if(m_Groups, [](const Group &group) { return group.assets.empty(); });
			std::ranges::stable_sort(m_Groups, [&](const Group &a, const Group &b)
			{
				if ((a.package < 0) != (b.package < 0))
					return b.package < 0;
				return a.package >= 0 && b.package >= 0 && packages[static_cast<size_t>(a.package)].priority < packages[static_cast<size_t>(b.package)].priority;
			});
			m_RowsKey.clear();
		}

		// The rows as drawn: a header per library, its assets while it is open (all of them, flat, without grouping)
		std::string rowsKey = m_FilterKey + '|' + std::to_string(m_GroupByLibrary) + '|' + std::to_string(IsFiltering());
		for (const auto &[name, open] : m_GroupOpen)
			rowsKey += name + (open ? "+" : "-");
		if (rowsKey == m_RowsKey)
			return;
		m_RowsKey = rowsKey;
		m_Rows.clear();
		for (uint32_t g = 0; g < static_cast<uint32_t>(m_Groups.size()); ++g)
		{
			const Group &group = m_Groups[g];
			if (m_GroupByLibrary)
				m_Rows.push_back({ g, HEADER });
			if (m_GroupByLibrary && !IsGroupOpen(index, group))
				continue;
			for (const uint32_t asset : group.assets)
				m_Rows.push_back({ g, asset });
		}
		if (!m_GroupByLibrary)
		{
			const std::vector<LibraryAsset> &assets = index.GetAssets();
			std::ranges::sort(m_Rows, [&](const Row &a, const Row &b) { return assets[a.asset].virtualPath < assets[b.asset].virtualPath; });
		}
	}

	std::string XPlaneLibraryPanel::GetFilterKey() const
	{
		std::string libraries;
		for (const std::string &name : m_Libraries)
			libraries += name + '\x1f';
		return std::string(m_Filter.InputBuf) + '|' + std::to_string(m_TypeMask) + '|' + libraries + '|' + std::to_string(m_ShowHidden) + '|' + std::to_string(m_OnlyMissing);
	}

	bool XPlaneLibraryPanel::IsFiltering() const
	{
		return m_Filter.IsActive() || m_TypeMask != ~0u || !m_Libraries.empty() || m_ShowHidden || m_OnlyMissing;
	}

	bool XPlaneLibraryPanel::IsGroupOpen(const LibraryIndex &index, const Group &group) const
	{
		// Opened or closed by hand, else open while searching or filtering: the list then shows only what matched
		const std::string name = group.package >= 0 ? index.GetPackages()[static_cast<size_t>(group.package)].name : std::string();
		if (const auto it = m_GroupOpen.find(name); it != m_GroupOpen.end())
			return it->second;
		return IsFiltering();
	}

	void XPlaneLibraryPanel::SetGroupsOpen(const LibraryIndex &index, const bool open)
	{
		for (const Group &group : m_Groups)
			m_GroupOpen[group.package >= 0 ? index.GetPackages()[static_cast<size_t>(group.package)].name : std::string()] = open;
	}

	void XPlaneLibraryPanel::ClearFilters()
	{
		m_Filter.Clear();
		m_TypeMask = ~0u;
		m_Libraries.clear();
		m_ShowHidden = false;
		m_OnlyMissing = false;
		m_AppliedFilter.clear();
	}

	void XPlaneLibraryPanel::BrowseLibrary(const LibraryIndex &index, const int package)
	{
		if (package < 0 || static_cast<size_t>(package) >= index.GetPackages().size())
			return;
		ClearFilters();
		const std::string &name = index.GetPackages()[static_cast<size_t>(package)].name;
		m_Libraries = { name };
		m_GroupOpen[name] = true;
		m_SelectTab = true;
	}

	void XPlaneLibraryPanel::DrawAssetsTab(const LibraryIndex &index)
	{
		DrawFilterBar(index);
		RefreshFilter(index);

		// Objects placed from here load in the background.
		EditorXPlane::DrawObjectLoadProgress(-FLT_MIN);

		if (index.GetAssets().empty())
		{
			EditorUI::EmptyState(LibraryManager::GetInstallInfo().IsValid() ? "No library assets yet (scanning, or no tracked libraries)."
																			  : "Set the X-Plane folder above to browse its libraries.");
			return;
		}

		// List (left) and details (right)
		const float dpi = ImGui::GetStyle().FontScaleDpi;
		const float listWidth = std::max(260.0f * dpi, ImGui::GetContentRegionAvail().x * 0.58f);
		if (ImGui::BeginChild("##AssetList", ImVec2(listWidth, 0.0f), ImGuiChildFlags_ResizeX))
		{
			if (m_GridView)
				DrawAssetGrid(index);
			else
				DrawAssetList(index);
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

	void XPlaneLibraryPanel::DrawFilterBar(const LibraryIndex &index)
	{
		const float dpi = ImGui::GetStyle().FontScaleDpi;
		const float buttonSize = ImGui::GetFrameHeight();
		const float spacing = ImGui::GetStyle().ItemSpacing.x;

		// Search, then the filter drop-down and the list buttons on the right
		const char *countFormat = "%zu of %zu";
		char count[64];
		snprintf(count, sizeof(count), countFormat, m_MatchCount, index.GetAssets().size());
		const float right = (buttonSize + spacing) * 5.0f + ImGui::CalcTextSize(count).x + spacing;
		EditorUI::SearchFilter("##AssetSearch", m_Filter, std::max(120.0f * dpi, ImGui::GetContentRegionAvail().x - right));
		EditorUI::ItemTooltip("Search virtual paths: words narrow it down, -word leaves out (\"hangar -small\")");

		int active = 0;
		active += m_TypeMask != ~0u ? 1 : 0;
		active += m_Libraries.empty() ? 0 : 1;
		active += m_ShowHidden ? 1 : 0;
		active += m_OnlyMissing ? 1 : 0;
		ImGui::SameLine();
		const std::string filterTip = active > 0 ? std::to_string(active) + " filter" + (active == 1 ? "" : "s") + " on\nClick to change them or use a saved filter"
												 : std::string("Filters: asset types, libraries, hidden and missing assets, and the filters saved in the project");
		if (EditorUI::StripButton(SEDX_ICON_FILTER, filterTip.c_str(), active > 0 || m_OpenFilterPopup, buttonSize, EditorUI::StripStyle::Tinted))
			ImGui::OpenPopup("##AssetFilters");
		if (active > 0)
		{
			// A count badge on the button's corner
			const ImVec2 max = ImGui::GetItemRectMax();
			const ImVec2 center(max.x - 3.0f * dpi, ImGui::GetItemRectMin().y + 3.0f * dpi);
			ImGui::GetWindowDrawList()->AddCircleFilled(center, 6.0f * dpi, EditorTheme::ToU32(EditorTheme::Palette::Accent));
			const std::string digits = std::to_string(active);
			ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 0.75f);
			const ImVec2 size = ImGui::CalcTextSize(digits.c_str());
			ImGui::GetWindowDrawList()->AddText(ImVec2(center.x - size.x * 0.5f, center.y - size.y * 0.5f), IM_COL32_WHITE, digits.c_str());
			ImGui::PopFont();
		}
		DrawFilterPopup(index);

		ImGui::SameLine();
		if (EditorUI::StripButton(m_GridView ? SEDX_ICON_TH_LARGE : SEDX_ICON_TH_LIST, m_GridView ? "Thumbnail grid\nClick for the list (Ctrl + wheel sizes the tiles)" :
			"List with thumbnails\nClick for the thumbnail grid", m_GridView, buttonSize))
			m_GridView = !m_GridView;
		ImGui::SameLine();
		if (EditorUI::StripButton(SEDX_ICON_LIST, m_GroupByLibrary ? "Grouped by library\nClick for one list" : "One list\nClick to group by library", m_GroupByLibrary, buttonSize))
			m_GroupByLibrary = !m_GroupByLibrary;
		ImGui::SameLine();
		ImGui::BeginDisabled(!m_GroupByLibrary);
		if (EditorUI::StripButton(SEDX_ICON_PLUS_SQUARE_O, "Open every library", false, buttonSize))
			SetGroupsOpen(index, true);
		ImGui::SameLine();
		if (EditorUI::StripButton(SEDX_ICON_MINUS_SQUARE_O, "Close every library", false, buttonSize))
			SetGroupsOpen(index, false);
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::TextDisabled("%s", count);

		// What narrows the list, as chips: a click takes one off
		if (!IsFiltering() && m_AppliedFilter.empty())
			return;
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f * dpi, 1.0f * dpi));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f * dpi);
		bool first = true;
		const auto chip = [&](const std::string &label, const char *tooltip) -> bool
		{
			if (!first)
				ImGui::SameLine(0.0f, 4.0f * dpi);
			first = false;
			const bool clicked = ImGui::SmallButton((std::string(SEDX_ICON_TIMES) + "  " + label).c_str());
			EditorUI::ItemTooltip(tooltip);
			return clicked;
		};
		if (!m_AppliedFilter.empty())
		{
			if (chip(std::string(SEDX_ICON_STAR) + "  " + m_AppliedFilter, "The saved filter applied last\nClick to clear every filter"))
				ClearFilters();
		}
		if (m_Filter.IsActive() && chip("\"" + std::string(m_Filter.InputBuf) + "\"", "Clear the search"))
			m_Filter.Clear();
		if (m_TypeMask != ~0u)
		{
			std::string types;
			int shown = 0;
			for (int t = 0; t < static_cast<int>(XPlane::AssetType::Count); ++t)
			{
				if ((m_TypeMask & (1u << t)) == 0)
					continue;
				if (shown++ < 3)
					types += (types.empty() ? "" : ", ") + std::string(GetAssetTypeName(static_cast<XPlane::AssetType>(t)));
			}
			if (shown > 3)
				types += " +" + std::to_string(shown - 3);
			if (chip(shown == 0 ? std::string("No types") : types, "Show every asset type"))
				m_TypeMask = ~0u;
		}
		if (!m_Libraries.empty())
		{
			const std::string label = m_Libraries.size() == 1 ? m_Libraries.front() : m_Libraries.front() + " +" + std::to_string(m_Libraries.size() - 1);
			if (chip(label, "Show every library"))
				m_Libraries.clear();
		}
		if (m_ShowHidden && chip("Private / deprecated", "Hide private and deprecated assets"))
			m_ShowHidden = false;
		if (m_OnlyMissing && chip("Missing files", "Show assets whose files exist too"))
			m_OnlyMissing = false;
		ImGui::PopStyleVar(2);

		// Once the filters change, the saved filter no longer describes them
		if (!m_AppliedFilter.empty() && GetFilterKey() != m_AppliedKey)
			m_AppliedFilter.clear();
	}

	void XPlaneLibraryPanel::DrawFilterPopup(const LibraryIndex &index)
	{
		m_OpenFilterPopup = ImGui::IsPopupOpen("##AssetFilters");
		if (!ImGui::BeginPopup("##AssetFilters"))
			return;
		const float dpi = ImGui::GetStyle().FontScaleDpi;
		const std::vector<LibraryPackage> &packages = index.GetPackages();

		// Asset types, three to a row
		ImGui::SeparatorText("Asset types");
		if (ImGui::SmallButton("All##types"))
			m_TypeMask = ~0u;
		ImGui::SameLine();
		if (ImGui::SmallButton("None##types"))
			m_TypeMask = 0;
		ImGui::SameLine();
		if (ImGui::SmallButton("Objects + facades"))
			m_TypeMask = (1u << static_cast<uint32_t>(XPlane::AssetType::Object)) | (1u << static_cast<uint32_t>(XPlane::AssetType::Facade));
		if (ImGui::BeginTable("##Types", 3, ImGuiTableFlags_SizingStretchSame))
		{
			for (int t = 0; t < static_cast<int>(XPlane::AssetType::Count); ++t)
			{
				ImGui::TableNextColumn();
				const XPlane::AssetType type = static_cast<XPlane::AssetType>(t);
				bool on = (m_TypeMask & (1u << t)) != 0;
				const std::string label = std::string(TypeIcon(type)) + "  " + GetAssetTypeName(type) + "##type" + std::to_string(t);
				if (ImGui::Checkbox(label.c_str(), &on))
					m_TypeMask = on ? (m_TypeMask | (1u << t)) : (m_TypeMask & ~(1u << t));
			}
			ImGui::EndTable();
		}

		// Libraries: none ticked shows them all
		ImGui::SeparatorText("Libraries");
		EditorUI::SearchFilter("##PopupLibrarySearch", m_PopupLibraryFilter, 260.0f * dpi);
		ImGui::SameLine();
		if (ImGui::SmallButton("All##libraries"))
			m_Libraries.clear();
		if (ImGui::BeginChild("##PopupLibraries", ImVec2(380.0f * dpi, 170.0f * dpi), ImGuiChildFlags_Borders))
		{
			for (int i = 0; i < static_cast<int>(packages.size()); ++i)
			{
				const LibraryPackage &package = packages[i];
				if (!package.hasLibrary || !package.tracked || !m_PopupLibraryFilter.PassFilter(package.name.c_str()))
					continue;
				ImGui::PushID(i);
				const auto it = std::ranges::find(m_Libraries, package.name);
				bool on = it != m_Libraries.end();
				if (ImGui::Checkbox(package.name.c_str(), &on))
				{
					if (on)
						m_Libraries.push_back(package.name);
					else
						m_Libraries.erase(it);
				}
				ImGui::SameLine();
				ImGui::TextDisabled("%u", package.assetCount);
				ImGui::PopID();
			}
		}
		ImGui::EndChild();
		ImGui::TextDisabled(m_Libraries.empty() ? "Every library" : "%zu selected", m_Libraries.size());

		ImGui::SeparatorText("Show");
		ImGui::Checkbox("Private and deprecated assets", &m_ShowHidden);
		ImGui::Checkbox("Only assets whose files are missing", &m_OnlyMissing);

		DrawSavedFilters(index);

		ImGui::Separator();
		ImGui::BeginDisabled(!IsFiltering());
		if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_TIMES, "Clear all filters").c_str()))
			ClearFilters();
		ImGui::EndDisabled();
		ImGui::EndPopup();
	}

	void XPlaneLibraryPanel::DrawSavedFilters(const LibraryIndex &index)
	{
		ImGui::SeparatorText("Saved in the project");
		ProjectFile::ProjectDocument *document = EditorProjects::GetDocument();
		if (!document)
		{
			ImGui::TextDisabled("Open or create a project to keep filters with it");
			return;
		}
		const float dpi = ImGui::GetStyle().FontScaleDpi;
		const std::vector<ProjectFile::LibraryFilter> &saved = document->GetProject().libraryFilters;
		if (saved.empty())
			ImGui::TextDisabled("None yet: set the filters above, name them and save");

		std::optional<std::vector<ProjectFile::LibraryFilter>> changed;
		std::string label;
		for (size_t i = 0; i < saved.size(); ++i)
		{
			const ProjectFile::LibraryFilter &filter = saved[i];
			ImGui::PushID(static_cast<int>(i));
			if (m_RenameFrom == filter.name)
			{
				ImGui::SetNextItemWidth(220.0f * dpi);
				if (ImGui::InputText("##Rename", &m_RenameTo, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
				{
					if (!m_RenameTo.empty() && std::ranges::none_of(saved, [&](const ProjectFile::LibraryFilter &f) { return f.name == m_RenameTo; }))
					{
						changed = saved;
						(*changed)[i].name = m_RenameTo;
						label = "Rename library filter " + filter.name;
					}
					m_RenameFrom.clear();
				}
				if (ImGui::IsItemDeactivated() && !ImGui::IsItemDeactivatedAfterEdit())
					m_RenameFrom.clear();
			}
			else
			{
				const bool current = m_AppliedFilter == filter.name;
				if (ImGui::Selectable((std::string(SEDX_ICON_STAR) + "  " + filter.name).c_str(), current, ImGuiSelectableFlags_DontClosePopups))
				{
					// Apply it: types and libraries by name (libraries this install lacks are kept, they match nothing)
					m_Filter.Clear();
					std::snprintf(m_Filter.InputBuf, sizeof(m_Filter.InputBuf), "%s", filter.search.c_str());
					m_Filter.Build();
					m_TypeMask = filter.types.empty() ? ~0u : 0u;
					for (const std::string &type : filter.types)
					{
						for (size_t t = 0; t < std::size(TYPE_IDS); ++t)
							if (type == TYPE_IDS[t])
								m_TypeMask |= 1u << t;
					}
					m_Libraries = filter.libraries;
					m_ShowHidden = filter.showHidden;
					m_OnlyMissing = filter.onlyMissing;
					m_GroupOpen.clear();
					m_AppliedFilter = filter.name;
					m_AppliedKey = GetFilterKey();
				}
				EditorUI::ItemTooltip("Click to apply. Right-click to rename, update or delete");
				if (ImGui::BeginPopupContextItem("##SavedFilterMenu"))
				{
					if (ImGui::MenuItem("Rename"))
					{
						m_RenameFrom = filter.name;
						m_RenameTo = filter.name;
					}
					if (ImGui::MenuItem("Update to the current filters"))
					{
						changed = saved;
						ProjectFile::LibraryFilter updated;
						updated.name = filter.name;
						updated.search = m_Filter.InputBuf;
						for (size_t t = 0; t < std::size(TYPE_IDS); ++t)
							if (m_TypeMask != ~0u && (m_TypeMask & (1u << t)) != 0)
								updated.types.push_back(TYPE_IDS[t]);
						updated.libraries = m_Libraries;
						updated.showHidden = m_ShowHidden;
						updated.onlyMissing = m_OnlyMissing;
						(*changed)[i] = updated;
						label = "Update library filter " + filter.name;
					}
					if (ImGui::MenuItem("Delete"))
					{
						changed = saved;
						changed->erase(changed->begin() + static_cast<std::ptrdiff_t>(i));
						label = "Delete library filter " + filter.name;
					}
					ImGui::EndPopup();
				}
			}
			ImGui::PopID();
		}

		// Save the current filters under a name (an existing name is updated)
		ImGui::SetNextItemWidth(220.0f * dpi);
		const bool enter = ImGui::InputTextWithHint("##SaveFilterName", "Name these filters", &m_SaveName, ImGuiInputTextFlags_EnterReturnsTrue);
		ImGui::SameLine();
		const bool exists = std::ranges::any_of(saved, [&](const ProjectFile::LibraryFilter &f) { return f.name == m_SaveName; });
		ImGui::BeginDisabled(m_SaveName.empty());
		if ((ImGui::Button(EditorUI::IconLabel(SEDX_ICON_FLOPPY_O, exists ? "Update" : "Save").c_str()) || enter) && !m_SaveName.empty())
		{
			ProjectFile::LibraryFilter filter;
			filter.name = m_SaveName;
			filter.search = m_Filter.InputBuf;
			for (size_t t = 0; t < std::size(TYPE_IDS); ++t)
				if (m_TypeMask != ~0u && (m_TypeMask & (1u << t)) != 0)
					filter.types.push_back(TYPE_IDS[t]);
			filter.libraries = m_Libraries;
			filter.showHidden = m_ShowHidden;
			filter.onlyMissing = m_OnlyMissing;
			changed = saved;
			const auto it = std::ranges::find_if(*changed, [&](const ProjectFile::LibraryFilter &f) { return f.name == filter.name; });
			if (it != changed->end())
				*it = filter;
			else
				changed->push_back(filter);
			label = (exists ? "Update library filter " : "Save library filter ") + filter.name;
			m_AppliedFilter = filter.name;
			m_AppliedKey = GetFilterKey();
			m_SaveName.clear();
		}
		ImGui::EndDisabled();
		EditorUI::ItemTooltip("Saved with the project (.edX / .sex), so everyone working on it gets the same filters");

		if (changed)
			EditorProjects::SetLibraryFilters(std::move(*changed), label);
		(void)index;
	}

	void XPlaneLibraryPanel::DrawAssetList(const LibraryIndex &index)
	{
		const float dpi = ImGui::GetStyle().FontScaleDpi;
		const std::vector<LibraryPackage> &packages = index.GetPackages();
		const std::vector<LibraryAsset> &assets = index.GetAssets();
		if (m_Rows.empty())
		{
			EditorUI::EmptyState(IsFiltering() ? "Nothing matches the search and filters." : "No assets.");
			return;
		}

		constexpr ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV;
		if (!ImGui::BeginTable(m_GroupByLibrary ? "##AssetsGrouped" : "##AssetsFlat", m_GroupByLibrary ? 2 : 3, flags))
			return;
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn(m_GroupByLibrary ? "Library / virtual path" : "Virtual path", ImGuiTableColumnFlags_WidthStretch);
		if (!m_GroupByLibrary)
			ImGui::TableSetupColumn("Library", ImGuiTableColumnFlags_WidthFixed, 150.0f * dpi);
		ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 56.0f * dpi);
		ImGui::TableHeadersRow();

		std::optional<std::pair<std::string, bool>> toggled;
		const float thumbnail = std::round(2.0f * ImGui::GetTextLineHeight());
		const float rowHeight = thumbnail + 2.0f * dpi;
		ImGuiListClipper clipper;
		clipper.Begin(static_cast<int>(m_Rows.size()), rowHeight + ImGui::GetStyle().CellPadding.y * 2.0f);
		while (clipper.Step())
		{
			for (int r = clipper.DisplayStart; r < clipper.DisplayEnd; ++r)
			{
				const Row &row = m_Rows[static_cast<size_t>(r)];
				const Group &group = m_Groups[row.group];
				ImGui::TableNextRow(ImGuiTableRowFlags_None, rowHeight);
				ImGui::TableNextColumn();
				ImGui::PushID(r);

				if (row.asset == HEADER)
				{
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + std::max(0.0f, (rowHeight - ImGui::GetFrameHeight()) * 0.5f));
					DrawGroupHeader(index, group, toggled);
					ImGui::TableNextColumn();
					if (IsFiltering())
						ImGui::TextDisabled("%zu / %u", group.assets.size(), group.total);
					else
						ImGui::TextDisabled("%u", group.total);
					ImGui::PopID();
					continue;
				}

				const LibraryAsset &asset = assets[row.asset];
				const AssetProvider *resolved = asset.GetResolved();
				const bool missing = resolved && !resolved->fileExists;
				if (m_GroupByLibrary)
					ImGui::Indent(ImGui::GetStyle().IndentSpacing);
				const ImVec2 start = ImGui::GetCursorScreenPos();
				const bool selected = m_SelectedAsset == asset.virtualPath;
				const bool pressed = ImGui::Selectable("##Asset", selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick, ImVec2(0.0f, rowHeight));
				HandleAssetItem(asset, pressed);

				// The thumbnail (the type's icon while it draws, or when there is none), then the path
				ImDrawList *draw = ImGui::GetWindowDrawList();
				const ImVec2 thumbMin(start.x, start.y + (rowHeight - thumbnail) * 0.5f);
				const ImVec2 thumbMax(thumbMin.x + thumbnail, thumbMin.y + thumbnail);
				if (!AssetThumbnails::Draw(resolved && !missing ? resolved->realPath : std::filesystem::path(), thumbMin, thumbMax))
				{
					const ImVec2 iconSize = ImGui::CalcTextSize(TypeIcon(asset.type));
					draw->AddText(ImVec2(thumbMin.x + (thumbnail - iconSize.x) * 0.5f, thumbMin.y + (thumbnail - iconSize.y) * 0.5f),
						EditorTheme::ToU32(EditorTheme::Palette::TextDisabled), TypeIcon(asset.type));
				}
				draw->AddText(ImVec2(thumbMax.x + ImGui::GetStyle().ItemSpacing.x, start.y + (rowHeight - ImGui::GetTextLineHeight()) * 0.5f),
					EditorTheme::ToU32(missing ? EditorTheme::Palette::Error : EditorTheme::Palette::Text), asset.virtualPath.c_str());
				if (m_GroupByLibrary)
					ImGui::Unindent(ImGui::GetStyle().IndentSpacing);

				if (!m_GroupByLibrary)
				{
					ImGui::TableNextColumn();
					ImGui::TextDisabled("%s", resolved ? packages[resolved->package].name.c_str() : "-");
				}
				ImGui::TableNextColumn();
				ImGui::TextDisabled("%zu", asset.providers.size());
				ImGui::PopID();
			}
		}
		ImGui::EndTable();

		if (toggled)
			m_GroupOpen[toggled->first] = toggled->second;
	}

	void XPlaneLibraryPanel::DrawGroupHeader(const LibraryIndex &index, const Group &group, std::optional<std::pair<std::string, bool>> &toggled)
	{
		const std::vector<LibraryPackage> &packages = index.GetPackages();
		const std::string name = group.package >= 0 ? packages[static_cast<size_t>(group.package)].name : std::string();
		const bool open = IsGroupOpen(index, group);
		ImGui::SetNextItemOpen(open, ImGuiCond_Always);
		UI::Fonts::PushFont("Bold");
		const bool nowOpen = ImGui::TreeNodeEx("##Library", ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_FramePadding,
			"%s  %s", SEDX_ICON_BOOK, name.empty() ? "Not resolved by any library" : name.c_str());
		UI::Fonts::PopFont();
		if (nowOpen != open)
			toggled = std::make_pair(name, nowOpen);
		if (group.package >= 0)
			EditorUI::ItemTooltip(packages[static_cast<size_t>(group.package)].path.string().c_str());
	}

	void XPlaneLibraryPanel::HandleAssetItem(const LibraryAsset &asset, const bool pressed)
	{
		if (pressed)
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

		const bool draggable = asset.type == XPlane::AssetType::Object || asset.type == XPlane::AssetType::Facade;
		if (draggable && ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload(EditorXPlane::ASSET_PAYLOAD, asset.virtualPath.c_str(), asset.virtualPath.size() + 1);
			const AssetProvider *resolved = asset.GetResolved();
			AssetThumbnails::Thumbnail thumbnail;
			if (resolved && resolved->fileExists && AssetThumbnails::Get(resolved->realPath, thumbnail) == AssetThumbnails::State::Ready)
			{
				const float side = 64.0f * ImGui::GetStyle().FontScaleDpi;
				ImGui::Image(ImTextureRef(thumbnail.texture), ImVec2(side, side), thumbnail.uv0, thumbnail.uv1);
				ImGui::SameLine();
			}
			ImGui::Text("%s  %s", asset.type == XPlane::AssetType::Facade ? SEDX_ICON_BUILDING_O : SEDX_ICON_CUBE, asset.virtualPath.c_str());
			ImGui::EndDragDropSource();
		}
	}

	void XPlaneLibraryPanel::DrawAssetGrid(const LibraryIndex &index)
	{
		const std::vector<LibraryAsset> &assets = index.GetAssets();
		if (m_Rows.empty())
		{
			EditorUI::EmptyState(IsFiltering() ? "Nothing matches the search and filters." : "No assets.");
			return;
		}

		// Ctrl + wheel sizes the tiles
		const ImGuiIO &io = ImGui::GetIO();
		if (ImGui::IsWindowHovered() && io.KeyCtrl && io.MouseWheel != 0.0f)
		{
			m_TileSize = std::clamp(m_TileSize * (io.MouseWheel > 0.0f ? 1.15f : 0.87f), 56.0f, 192.0f);
			ImGui::SetScrollY(ImGui::GetScrollY());	// Keep the wheel from scrolling as well
		}

		const ImGuiStyle &style = ImGui::GetStyle();
		const float dpi = style.FontScaleDpi;
		const float tile = std::round(m_TileSize * dpi);
		const float labelHeight = ImGui::GetTextLineHeightWithSpacing() + 2.0f * dpi;
		const int columns = std::max(1, static_cast<int>((ImGui::GetContentRegionAvail().x + style.ItemSpacing.x) / (tile + style.ItemSpacing.x)));

		// Lines: library headers, and rows of tiles between them; only the visible ones are drawn
		struct Line
		{
			size_t first = 0;
			size_t count = 0;
			bool header = false;
		};
		std::vector<Line> lines;
		for (size_t r = 0; r < m_Rows.size();)
		{
			if (m_Rows[r].asset == HEADER)
			{
				lines.push_back({ r, 1, true });
				++r;
				continue;
			}
			Line line{ r, 0, false };
			while (r < m_Rows.size() && m_Rows[r].asset != HEADER && line.count < static_cast<size_t>(columns))
			{
				++line.count;
				++r;
			}
			lines.push_back(line);
		}

		std::optional<std::pair<std::string, bool>> toggled;
		const float headerHeight = ImGui::GetFrameHeightWithSpacing();
		const float tileHeight = tile + labelHeight + style.ItemSpacing.y;
		const float scrollTop = ImGui::GetScrollY();
		const float scrollBottom = scrollTop + ImGui::GetWindowHeight();
		float y = ImGui::GetCursorPosY();
		for (const Line &line : lines)
		{
			const float height = line.header ? headerHeight : tileHeight;
			if (y + height < scrollTop || y > scrollBottom)
			{
				y += height;
				continue;
			}
			ImGui::SetCursorPosY(y);
			if (line.header)
			{
				ImGui::PushID(static_cast<int>(line.first));
				DrawGroupHeader(index, m_Groups[m_Rows[line.first].group], toggled);
				ImGui::PopID();
				y += height;
				continue;
			}

			for (size_t i = 0; i < line.count; ++i)
			{
				const LibraryAsset &asset = assets[m_Rows[line.first + i].asset];
				const AssetProvider *resolved = asset.GetResolved();
				const bool missing = resolved && !resolved->fileExists;
				if (i > 0)
					ImGui::SameLine();
				ImGui::PushID(static_cast<int>(line.first + i));
				const ImVec2 min = ImGui::GetCursorScreenPos();
				const ImVec2 max(min.x + tile, min.y + tile);
				ImGui::InvisibleButton("##Tile", ImVec2(tile, tile + labelHeight));
				const bool hovered = ImGui::IsItemHovered();
				const bool selected = m_SelectedAsset == asset.virtualPath;
				HandleAssetItem(asset, ImGui::IsItemClicked(ImGuiMouseButton_Left));

				ImDrawList *draw = ImGui::GetWindowDrawList();
				draw->AddRectFilled(min, max, EditorTheme::ToU32(hovered ? EditorTheme::Palette::FrameHovered : EditorTheme::Palette::Frame), 3.0f * dpi);
				const float inset = 3.0f * dpi;
				if (!AssetThumbnails::Draw(missing || !resolved ? std::filesystem::path() : resolved->realPath, ImVec2(min.x + inset, min.y + inset), ImVec2(max.x - inset, max.y - inset)))
				{
					ImGui::PushFont(nullptr, style.FontSizeBase * std::max(1.0f, m_TileSize / 40.0f));
					const ImVec2 iconSize = ImGui::CalcTextSize(TypeIcon(asset.type));
					draw->AddText(ImVec2(min.x + (tile - iconSize.x) * 0.5f, min.y + (tile - iconSize.y) * 0.5f), EditorTheme::ToU32(EditorTheme::Palette::TextDisabled), TypeIcon(asset.type));
					ImGui::PopFont();
				}
				if (selected)
					draw->AddRect(min, max, EditorTheme::ToU32(EditorTheme::Palette::Accent), 3.0f * dpi, 0, 2.0f * dpi);

				// The file name under the tile
				const size_t slash = asset.virtualPath.find_last_of('/');
				const char *name = asset.virtualPath.c_str() + (slash == std::string::npos ? 0 : slash + 1);
				const ImVec2 labelMin(min.x, max.y + 2.0f * dpi);
				const ImVec2 labelMax(max.x, labelMin.y + ImGui::GetTextLineHeight());
				ImGui::PushStyleColor(ImGuiCol_Text, Color(missing ? EditorTheme::Palette::Error : EditorTheme::Palette::Text));
				ImGui::RenderTextEllipsis(draw, labelMin, labelMax, labelMax.x, name, nullptr, nullptr);
				ImGui::PopStyleColor();
				if (hovered)
					ImGui::SetItemTooltip("%s", asset.virtualPath.c_str());
				ImGui::PopID();
			}
			y += height;
		}
		ImGui::SetCursorPosY(y);
		ImGui::Dummy(ImVec2(1.0f, 1.0f));

		if (toggled)
			m_GroupOpen[toggled->first] = toggled->second;
	}

	void XPlaneLibraryPanel::DrawAssetPreview(const LibraryAsset &asset)
	{
		const AssetProvider *resolved = asset.GetResolved();
		if (!resolved || !resolved->fileExists || !XPlane::CanMakeThumbnail(resolved->realPath.string()))
			return;
		const float dpi = ImGui::GetStyle().FontScaleDpi;
		const float side = std::min(ImGui::GetContentRegionAvail().x, 256.0f * dpi);
		if (side < 32.0f * dpi)
			return;
		const ImVec2 min = ImGui::GetCursorScreenPos();
		const ImVec2 max(min.x + side, min.y + side);
		ImDrawList *draw = ImGui::GetWindowDrawList();
		draw->AddRectFilled(min, max, EditorTheme::ToU32(EditorTheme::Palette::Frame), 4.0f * dpi);

		AssetThumbnails::Thumbnail thumbnail;
		const AssetThumbnails::State state = AssetThumbnails::Get(resolved->realPath, thumbnail, AssetThumbnails::Size::Large);
		if (state == AssetThumbnails::State::Ready)
			draw->AddImage(thumbnail.texture, min, max, thumbnail.uv0, thumbnail.uv1);
		else
		{
			const std::string text = state == AssetThumbnails::State::Failed ? AssetThumbnails::GetError(resolved->realPath, AssetThumbnails::Size::Large) : "Drawing the preview...";
			const ImVec2 size = ImGui::CalcTextSize(text.c_str(), nullptr, false, side - 16.0f * dpi);
			draw->AddText(nullptr, 0.0f, ImVec2(min.x + (side - size.x) * 0.5f, min.y + (side - size.y) * 0.5f), EditorTheme::ToU32(EditorTheme::Palette::TextDisabled),
				text.c_str(), nullptr, side - 16.0f * dpi);
		}
		ImGui::Dummy(ImVec2(side, side));
	}

	void XPlaneLibraryPanel::DrawAssetDetails(const LibraryIndex &index, const LibraryAsset &asset)
	{
		const std::vector<LibraryPackage> &packages = index.GetPackages();
		const AssetProvider *resolved = asset.GetResolved();

		DrawAssetPreview(asset);

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
			const bool browsing = m_Libraries.size() == 1 && m_Libraries.front() == package.name;
			if (ImGui::Selectable(package.name.c_str(), browsing, ImGuiSelectableFlags_AllowDoubleClick))
			{
				if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && package.tracked)
					BrowseLibrary(index, i);
			}
			if (!package.tracked)
				ImGui::PopStyleColor();
			EditorUI::ItemTooltip((package.path.string() + "\nDouble-click to browse its assets").c_str());

			if (ImGui::BeginPopupContextItem("##LibraryMenu"))
			{
				if (ImGui::MenuItem("Browse assets", nullptr, false, package.tracked && package.hasLibrary))
					BrowseLibrary(index, i);
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

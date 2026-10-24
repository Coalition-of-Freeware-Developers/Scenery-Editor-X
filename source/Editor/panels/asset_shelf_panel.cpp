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
 * asset_shelf_panel.cpp
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#include "asset_shelf_panel.h"
#include <algorithm>
#include <cctype>
#include <Editor/core/editor_theme.h>
#include <Editor/core/editor_ui.h>
#include <SceneryEditorX/filesystem/vfs.h>
#include <SceneryEditorX/renderer/font/font_awesome.h>
#include <SceneryEditorX/renderer/ui/source/imgui/imgui_internal.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		/* Cap on recursive search hits so a broad query on a large library stays responsive. */
		constexpr size_t MAX_SEARCH_RESULTS = 2000;

		std::string Lower(std::string text)
		{
			std::ranges::transform(text, text.begin(), [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });
			return text;
		}

		AssetKind ClassifyFile(const std::filesystem::path &path)
		{
			const std::string ext = Lower(VFS::ToUtf8(path.extension()));
			if (ext == ".fbx" || ext == ".gltf" || ext == ".glb" || ext == ".dae" || ext == ".obj" || ext == ".blend")
				return AssetKind::Model;
			if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".dds" || ext == ".ktx" || ext == ".ktx2" ||
				ext == ".hdr" || ext == ".exr" || ext == ".tga" || ext == ".bmp")
				return AssetKind::Texture;
			if (ext == ".slang" || ext == ".glsl" || ext == ".hlsl" || ext == ".spv" || ext == ".vert" || ext == ".frag" || ext == ".comp")
				return AssetKind::Shader;
			if (ext == ".ttf" || ext == ".otf")
				return AssetKind::Font;
			if (ext == ".fac" || ext == ".for" || ext == ".pol" || ext == ".lin" || ext == ".str" || ext == ".ter" ||
				ext == ".agp" || ext == ".dsf" || ext == ".net" || ext == ".apt" || ext == ".ags" || ext == ".agb")
				return AssetKind::XPlane;
			return AssetKind::Other;
		}

		const char *KindIcon(const AssetKind kind)
		{
			switch (kind)
			{
				case AssetKind::Folder:		return SEDX_ICON_FOLDER;
				case AssetKind::Model:		return SEDX_ICON_CUBE;
				case AssetKind::Texture:	return SEDX_ICON_FILE_IMAGE_O;
				case AssetKind::Shader:		return SEDX_ICON_FILE_CODE_O;
				case AssetKind::Font:		return SEDX_ICON_FONT;
				case AssetKind::XPlane:		return SEDX_ICON_GLOBE;
				default:					return SEDX_ICON_FILE_O;
			}
		}

		struct KindFilter
		{
			int kind;
			const char *tooltip;
		};

		constexpr KindFilter s_KindFilters[] =
		{
			{ -1,									"All files" },
			{ static_cast<int>(AssetKind::Model),	"Models" },
			{ static_cast<int>(AssetKind::Texture),	"Textures" },
			{ static_cast<int>(AssetKind::Shader),	"Shaders" },
			{ static_cast<int>(AssetKind::Font),	"Fonts" },
			{ static_cast<int>(AssetKind::XPlane),	"X-Plane art assets" },
		};

		const char *FilterIcon(const int kind)
		{
			return kind < 0 ? SEDX_ICON_TH : KindIcon(static_cast<AssetKind>(kind));
		}

		/* Sorts folders first, then by name (case-insensitive). */
		template<typename Entry>
		void SortEntries(std::vector<Entry> &entries)
		{
			std::ranges::sort(entries, [](const auto &a, const auto &b)
			{
				if ((a.kind == AssetKind::Folder) != (b.kind == AssetKind::Folder))
					return a.kind == AssetKind::Folder;
				return Lower(a.name) < Lower(b.name);
			});
		}
	}

	// -------------------------------------------------------

	AssetShelfPanel::AssetShelfPanel(std::filesystem::path root) : m_Root(std::move(root))
	{
		m_Title = "ASSETS";
		Navigate(m_Root);
	}

	void AssetShelfPanel::OnUIRender(bool &isOpen)
	{
		if (EditorUI::BeginPanel("ASSETS###Assets", &isOpen))
		{
			DrawToolbar();
			ImGui::Separator();

			const float dpi = ImGui::GetStyle().FontScaleDpi;
			if (m_ShowTree)
			{
				ImGui::BeginChild("##AssetTree", ImVec2(200.0f * dpi, 0.0f), ImGuiChildFlags_ResizeX);
				DrawFolderTree(m_Root);
				ImGui::EndChild();
				ImGui::SameLine();
			}

			ImGui::BeginChild("##AssetGrid");
			DrawGrid();
			ImGui::EndChild();
		}
		EditorUI::EndPanel();
	}

	void AssetShelfPanel::DrawToolbar()
	{
		const float dpi = ImGui::GetStyle().FontScaleDpi;
		const float buttonSize = ImGui::GetFrameHeight();

		if (EditorUI::StripButton(SEDX_ICON_BARS, "Show or hide the library tree", m_ShowTree, buttonSize))
			m_ShowTree = !m_ShowTree;

		ImGui::SameLine();
		ImGui::BeginDisabled(m_CurrentFolder == m_Root);
		if (EditorUI::StripButton(SEDX_ICON_LEVEL_UP, "Parent folder", false, buttonSize))
			Navigate(m_CurrentFolder.parent_path());
		ImGui::EndDisabled();

		// Breadcrumb relative to the library root
		ImGui::SameLine();
		std::error_code ec;
		const std::filesystem::path relative = std::filesystem::relative(m_CurrentFolder, m_Root, ec);
		const std::string crumb = (ec || relative == ".") ? VFS::ToUtf8(m_Root.filename()) : VFS::ToGenericUtf8(m_Root.filename() / relative);
		ImGui::AlignTextToFramePadding();
		ImGui::TextDisabled("%s", crumb.c_str());

		// Right-aligned: search, kind filters, tile size, refresh
		const float filtersWidth = (buttonSize + ImGui::GetStyle().ItemSpacing.x) * (static_cast<float>(std::size(s_KindFilters)) + 1.0f);
		const float searchWidth = 220.0f * dpi;
		const float sliderWidth = 90.0f * dpi;
		const float rightBlock = searchWidth + filtersWidth + sliderWidth + ImGui::GetStyle().ItemSpacing.x * 3.0f;
		const float startX = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - rightBlock;

		ImGui::SameLine(std::max(startX, ImGui::GetCursorPosX()));
		ImGui::SetNextItemWidth(searchWidth);
		ImGui::InputTextWithHint("##AssetSearch", "Search", m_Search, sizeof(m_Search));
		if (m_LastSearch != m_Search)
		{
			m_LastSearch = m_Search;
			RebuildSearch();
		}

		for (const KindFilter &filter : s_KindFilters)
		{
			ImGui::SameLine();
			ImGui::PushID(filter.kind);
			if (EditorUI::StripButton(FilterIcon(filter.kind), filter.tooltip, m_KindFilter == filter.kind, buttonSize))
				m_KindFilter = filter.kind;
			ImGui::PopID();
		}

		ImGui::SameLine();
		ImGui::SetNextItemWidth(sliderWidth);
		ImGui::SliderFloat("##TileSize", &m_TileSize, 48.0f, 128.0f, "");
		EditorUI::ItemTooltip("Tile size");

		ImGui::SameLine();
		if (EditorUI::StripButton(SEDX_ICON_REFRESH, "Rescan the folder", false, buttonSize))
			Refresh();
	}

	void AssetShelfPanel::DrawFolderTree(const std::filesystem::path &folder)
	{
		const std::vector<std::filesystem::path> &children = GetSubfolders(folder);

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
		if (children.empty())
			flags |= ImGuiTreeNodeFlags_Leaf;
		if (folder == m_CurrentFolder)
			flags |= ImGuiTreeNodeFlags_Selected;
		if (folder == m_Root)
			flags |= ImGuiTreeNodeFlags_DefaultOpen;

		// UTF-8 like all ImGui text: path::string() throws on Windows for a folder outside the ANSI code page (C:\Users\<name>)
		const std::string label = folder == m_Root ? "All libraries" : VFS::ToUtf8(folder.filename());
		const bool open = ImGui::TreeNodeEx(VFS::ToUtf8(folder).c_str(), flags, "%s", EditorUI::IconLabel(SEDX_ICON_FOLDER, label.c_str()).c_str());
		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
			Navigate(folder);

		if (open)
		{
			// Copy: navigating from a child click can refresh the cache this reference points into.
			const std::vector<std::filesystem::path> subfolders = children;
			for (const std::filesystem::path &child : subfolders)
				DrawFolderTree(child);
			ImGui::TreePop();
		}
	}

	void AssetShelfPanel::DrawGrid()
	{
		using namespace EditorTheme;

		const bool searching = m_Search[0] != '\0';
		const std::vector<AssetEntry> &entries = searching ? m_SearchResults : m_Entries;

		const ImGuiStyle &style = ImGui::GetStyle();
		const float dpi = style.FontScaleDpi;
		const float tile = m_TileSize * dpi;
		const float labelHeight = ImGui::GetTextLineHeightWithSpacing() + 2.0f * dpi;
		const float cellWidth = tile + style.ItemSpacing.x;
		const int columns = std::max(1, static_cast<int>(ImGui::GetContentRegionAvail().x / cellWidth));

		std::filesystem::path navigateTo;
		int column = 0;
		size_t shown = 0;
		for (const AssetEntry &entry : entries)
		{
			if (!PassesKindFilter(entry.kind))
				continue;

			if (column > 0)
				ImGui::SameLine();

			ImGui::PushID(VFS::ToUtf8(entry.path).c_str());
			ImGui::BeginGroup();

			const ImVec2 tileMin = ImGui::GetCursorScreenPos();
			const ImVec2 tileMax(tileMin.x + tile, tileMin.y + tile);
			ImGui::InvisibleButton("##Tile", ImVec2(tile, tile + labelHeight));
			const bool hovered = ImGui::IsItemHovered();
			const bool selected = entry.path == m_SelectedPath;

			if (ImGui::IsItemClicked())
				m_SelectedPath = entry.path;
			if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && entry.kind == AssetKind::Folder)
				navigateTo = entry.path;

			ImDrawList *drawList = ImGui::GetWindowDrawList();
			drawList->AddRectFilled(tileMin, tileMax, ToU32(hovered ? Palette::FrameHovered : Palette::Separator), 2.0f * dpi);
			if (selected)
				drawList->AddRect(tileMin, tileMax, ToU32(Palette::Accent), 2.0f * dpi, 0, 2.0f * dpi);

			// Large glyph in the middle of the tile (fonts are rasterized on demand at this size)
			ImGui::PushFont(nullptr, style.FontSizeBase * (m_TileSize / 36.0f));
			const char *icon = KindIcon(entry.kind);
			const ImVec2 iconSize = ImGui::CalcTextSize(icon);
			drawList->AddText(ImVec2(tileMin.x + (tile - iconSize.x) * 0.5f, tileMin.y + (tile - iconSize.y) * 0.5f),
				ToU32(entry.kind == AssetKind::Folder ? Palette::Warning : Palette::TextLabel), icon);
			ImGui::PopFont();

			// Name under the tile, clipped with an ellipsis
			const ImVec2 labelMin(tileMin.x, tileMax.y + 2.0f * dpi);
			const ImVec2 labelMax(tileMax.x, labelMin.y + ImGui::GetTextLineHeight());
			ImGui::RenderTextEllipsis(drawList, labelMin, labelMax, labelMax.x, entry.name.c_str(), nullptr, nullptr);

			if (hovered)
				ImGui::SetItemTooltip("%s", VFS::ToGenericUtf8(entry.path).c_str());

			ImGui::EndGroup();
			ImGui::PopID();

			column = (column + 1) % columns;
			++shown;
		}

		if (shown == 0)
			EditorUI::EmptyState(searching ? "No assets match the search" : "This folder is empty");

		if (!navigateTo.empty())
			Navigate(navigateTo);
	}

	void AssetShelfPanel::Navigate(const std::filesystem::path &folder)
	{
		m_CurrentFolder = folder;
		m_Entries.clear();

		std::error_code ec;
		for (std::filesystem::directory_iterator it(folder, std::filesystem::directory_options::skip_permission_denied, ec), end; !ec && it != end; it.increment(ec))
		{
			const bool isFolder = it->is_directory(ec);
			m_Entries.push_back({ it->path(), VFS::ToUtf8(it->path().filename()), isFolder ? AssetKind::Folder : ClassifyFile(it->path()) });
		}

		if (ec)
			EDITOR_WARN_TAG("Assets", "Could not list '{}': {}", VFS::ToUtf8(folder), ec.message());

		SortEntries(m_Entries);
	}

	void AssetShelfPanel::Refresh()
	{
		m_SubfolderCache.clear();
		Navigate(std::filesystem::exists(m_CurrentFolder) ? m_CurrentFolder : m_Root);
		RebuildSearch();
	}

	void AssetShelfPanel::RebuildSearch()
	{
		m_SearchResults.clear();
		if (m_Search[0] == '\0')
			return;

		const std::string query = Lower(m_Search);
		std::error_code ec;
		for (std::filesystem::recursive_directory_iterator it(m_Root, std::filesystem::directory_options::skip_permission_denied, ec), end;
			 !ec && it != end && m_SearchResults.size() < MAX_SEARCH_RESULTS; it.increment(ec))
		{
			const std::string name = VFS::ToUtf8(it->path().filename());
			if (Lower(name).find(query) == std::string::npos)
				continue;

			const bool isFolder = it->is_directory(ec);
			m_SearchResults.push_back({ it->path(), name, isFolder ? AssetKind::Folder : ClassifyFile(it->path()) });
		}

		SortEntries(m_SearchResults);
	}

	const std::vector<std::filesystem::path> &AssetShelfPanel::GetSubfolders(const std::filesystem::path &folder)
	{
		if (const auto it = m_SubfolderCache.find(folder); it != m_SubfolderCache.end())
			return it->second;

		std::vector<std::filesystem::path> subfolders;
		std::error_code ec;
		for (std::filesystem::directory_iterator it(folder, std::filesystem::directory_options::skip_permission_denied, ec), end; !ec && it != end; it.increment(ec))
		{
			if (it->is_directory(ec))
				subfolders.push_back(it->path());
		}

		std::ranges::sort(subfolders);
		return m_SubfolderCache.emplace(folder, std::move(subfolders)).first->second;
	}

	bool AssetShelfPanel::PassesKindFilter(const AssetKind kind) const
	{
		return m_KindFilter < 0 || kind == AssetKind::Folder || static_cast<int>(kind) == m_KindFilter;
	}

}

// -------------------------------------------------------

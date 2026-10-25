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
 * xplane_library_panel.h
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <Editor/core/editor_context.h>
#include <SceneryEditorX/renderer/ui/editor_panel.h>
#include <SceneryEditorX/xplane/xp_library.h>
#include <SceneryEditorX/xplane/xp_obj8.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @class XPlaneLibraryPanel
	 * @brief "X-PLANE LIBRARY" panel: the X-Plane 12 folder, the library packages it provides (with tracking
	 * toggles), a searchable browser of every exported asset, and the simulation controls that drive animation.
	 * Objects can be placed with a double-click, the Place button, or by dragging them onto the viewport.
	 */
	class XPlaneLibraryPanel : public UI::EditorPanel
	{
	public:
		explicit XPlaneLibraryPanel(EditorContext *context);

		void OnUIRender(bool &isOpen) override;

	private:
		/* @brief One line of the asset list: a library's header, or an asset under it. */
		struct Row
		{
			uint32_t group = 0;				// Index into m_Groups
			uint32_t asset = HEADER;		// Index into the index's assets, HEADER for the library's own line
		};
		static constexpr uint32_t HEADER = ~0u;

		/* @brief The assets one library provides that pass the search and filters. */
		struct Group
		{
			int package = -1;				// Package index, -1 for assets no library resolves
			uint32_t total = 0;				// Assets it provides at all (before filtering)
			std::vector<uint32_t> assets;
		};

		void DrawInstallBar();
		void DrawAssetsTab(const XPlane::LibraryIndex &index);
		void DrawFilterBar(const XPlane::LibraryIndex &index);
		void DrawFilterPopup(const XPlane::LibraryIndex &index);
		void DrawSavedFilters(const XPlane::LibraryIndex &index);
		void DrawAssetList(const XPlane::LibraryIndex &index);
		void DrawAssetGrid(const XPlane::LibraryIndex &index);
		void DrawAssetDetails(const XPlane::LibraryIndex &index, const XPlane::LibraryAsset &asset);
		void DrawAssetPreview(const XPlane::LibraryAsset &asset);
		/* @brief A library's line: click to open or close it (both views). */
		void DrawGroupHeader(const XPlane::LibraryIndex &index, const Group &group, std::optional<std::pair<std::string, bool>> &toggled);
		/* @brief After an asset's item: selects it when pressed, a double-click places an object or arms a facade, dragging drops it. */
		void HandleAssetItem(const XPlane::LibraryAsset &asset, bool pressed);
		void DrawLibrariesTab(const XPlane::LibraryIndex &index);
		void RefreshFilter(const XPlane::LibraryIndex &index);

		/* @brief True when the search or any filter narrows the list (libraries then open on their matches). */
		[[nodiscard]] bool IsFiltering() const;
		/* @brief The search and filters as one string (changes whenever any of them does). */
		[[nodiscard]] std::string GetFilterKey() const;
		[[nodiscard]] bool IsGroupOpen(const XPlane::LibraryIndex &index, const Group &group) const;
		void SetGroupsOpen(const XPlane::LibraryIndex &index, bool open);
		void ClearFilters();
		/* @brief Shows only one library (the Libraries tab's "Browse assets"). */
		void BrowseLibrary(const XPlane::LibraryIndex &index, int package);

		EditorContext *m_Context = nullptr;

		// Asset browser
		ImGuiTextFilter m_Filter;
		uint32_t m_TypeMask = ~0u;											// Bit per AssetType that passes
		std::vector<std::string> m_Libraries;								// Package names that pass; empty: every library
		bool m_ShowHidden = false;											// Private and deprecated assets
		bool m_OnlyMissing = false;
		bool m_GroupByLibrary = true;
		bool m_GridView = false;											// Tiles with thumbnails instead of rows
		float m_TileSize = 96.0f;
		std::string m_AppliedFilter;										// The saved filter applied last (until the filters change)
		std::string m_AppliedKey;
		std::vector<Group> m_Groups;										// Libraries with matches, in priority order
		std::vector<Row> m_Rows;											// The list as drawn (collapsed libraries keep only their header)
		size_t m_MatchCount = 0;
		std::map<std::string, bool> m_GroupOpen;							// Libraries opened or closed by hand (by name)
		std::string m_FilterKey;											// Inputs m_Groups was built for
		std::string m_RowsKey;
		uint64_t m_FilterGeneration = ~0ull;
		std::string m_LastSearch;
		std::string m_SelectedAsset;										// Virtual path
		bool m_SelectTab = false;											// Jump to the Assets tab next frame
		bool m_OpenFilterPopup = false;
		ImGuiTextFilter m_PopupLibraryFilter;
		std::string m_SaveName;												// "Save as" in the filter drop-down
		std::string m_RenameFrom;											// A saved filter being renamed
		std::string m_RenameTo;

		// Selected object preview (parsed on selection, CPU only)
		std::string m_PreviewPath;
		std::unique_ptr<XPlane::Obj8Data> m_Preview;
		std::string m_PreviewError;

		// Libraries table
		ImGuiTextFilter m_LibraryFilter;
		bool m_ShowAllPackages = false;
	};

}

// -------------------------------------------------------

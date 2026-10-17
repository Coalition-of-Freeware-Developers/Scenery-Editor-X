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
 * asset_shelf_panel.h
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <filesystem>
#include <map>
#include <string>
#include <vector>
#include <SceneryEditorX/renderer/ui/editor_panel.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @enum AssetKind
	 * @brief Coarse file category used by the shelf's filter buttons and tile icons.
	 */
	enum class AssetKind : uint8_t
	{
		Folder,
		Model,
		Texture,
		Shader,
		Font,
		XPlane,		// X-Plane scenery art assets (.fac, .for, .pol, .lin, .str, .ter, .agp, .dsf, ...)
		Other
	};

	/**
	 * @class AssetShelfPanel
	 * @brief "ASSETS" panel: Substance-style shelf for browsing files on disk.
	 *
	 * Left: collapsible folder tree of the library root. Top: search, kind filters, tile size.
	 * Body: tile grid of the current folder, or of every match under the root while searching.
	 * Directory listings are cached and only re-read on navigation or Refresh.
	 */
	class AssetShelfPanel : public UI::EditorPanel
	{
	public:
		/**
		 * @param root Library root to browse (e.g. the engine resources directory).
		 */
		explicit AssetShelfPanel(std::filesystem::path root);

		void OnUIRender(bool &isOpen) override;

	private:
		struct AssetEntry
		{
			std::filesystem::path path;
			std::string name;
			AssetKind kind = AssetKind::Other;
		};

		void DrawToolbar();
		void DrawFolderTree(const std::filesystem::path &folder);
		void DrawGrid();
		void Navigate(const std::filesystem::path &folder);
		void Refresh();
		void RebuildSearch();
		const std::vector<std::filesystem::path> &GetSubfolders(const std::filesystem::path &folder);
		[[nodiscard]] bool PassesKindFilter(AssetKind kind) const;

		std::filesystem::path m_Root;
		std::filesystem::path m_CurrentFolder;
		std::vector<AssetEntry> m_Entries;			// Current folder listing
		std::vector<AssetEntry> m_SearchResults;	// Recursive matches while a search is active
		std::map<std::filesystem::path, std::vector<std::filesystem::path>> m_SubfolderCache;
		std::filesystem::path m_SelectedPath;

		char m_Search[128] = {};
		std::string m_LastSearch;
		int m_KindFilter = -1;		// -1 = all, otherwise an AssetKind value
		float m_TileSize = 72.0f;
		bool m_ShowTree = true;
	};

}

// -------------------------------------------------------

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
#include <memory>
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
		void DrawInstallBar();
		void DrawAssetsTab(const XPlane::LibraryIndex &index);
		void DrawAssetDetails(const XPlane::LibraryIndex &index, const XPlane::LibraryAsset &asset);
		void DrawLibrariesTab(const XPlane::LibraryIndex &index);
		void RefreshFilter(const XPlane::LibraryIndex &index);

		EditorContext *m_Context = nullptr;

		// Asset browser
		ImGuiTextFilter m_Filter;
		int m_TypeFilter = static_cast<int>(XPlane::AssetType::Object);	// AssetType, or -1 for all
		int m_PackageFilter = -1;											// Package index, or -1 for all
		bool m_ShowHidden = false;											// Private and deprecated assets
		bool m_OnlyMissing = false;
		std::vector<uint32_t> m_Filtered;									// Indices into the index's assets
		std::string m_FilterKey;											// Inputs m_Filtered was built for
		uint64_t m_FilterGeneration = ~0ull;
		std::string m_SelectedAsset;										// Virtual path
		bool m_SelectTab = false;											// Jump to the Assets tab next frame

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

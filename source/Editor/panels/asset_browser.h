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
 * asset_browser.h
 * -------------------------------------------------------
 * Created: 18/03/2026
 * -------------------------------------------------------
 */
#pragma once
#include <SceneryEditorX/renderer/ui/editor_panel.h>

// ---------------------------------------------------------

namespace SceneryEditorX
{
	class EditorLayer;

	/**
	 * @class AssetBrowser
	 * @brief A panel that allows users to browse and manage their assets within the editor. 
	 * It provides functionality for importing, organizing, and previewing various types of assets such as meshes, textures, materials, and more.
	 * The AssetBrowser is an essential tool for efficiently managing the resources used in a project.
	 */
	class AssetBrowser : public UI::EditorPanel
	{
	public:
		/**
		 * @brief Constructs an AssetBrowser panel.
		 * @param editor The editor layer that owns this panel.
		 */
		AssetBrowser(EditorLayer *editor);

		/**
		 * @brief Called every tick when the panel is visible.
		 */
		void OnTickVisible() override;

		/**
		 * @brief Shows the mesh import dialog.
		 * @param filePath The path of the mesh file to import.
		 */
		static void ShowMeshImportDialog(const std::string &filePath);

		/**
		 * @brief Renders the UI for the AssetBrowser panel.
		 * @param isOpen A reference to a boolean that indicates whether the panel is open.
		 */
		void OnUIRender(bool &isOpen) override;

	private:
		/**
		 * @brief Called when a path is clicked in the AssetBrowser.
		 * @param path The path that was clicked.
		 */
		static void OnPathClicked(const std::string &path);

	};
	
}

// ---------------------------------------------------------

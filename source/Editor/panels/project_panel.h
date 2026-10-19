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
 * project_panel.h
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <string>
#include <Editor/core/editor_context.h>
#include <SceneryEditorX/project/scenery_project.h>
#include <SceneryEditorX/renderer/ui/editor_panel.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @class ProjectPanel
	 * @brief "PROJECT" panel: the open .edX / .sex project. Its header (name, author, target X-Plane version, pack),
	 * datum, airports (pick the one the airport editor shows), the scene hierarchy, the libraries it uses, the
	 * library.txt it authors (library projects) and what validation found.
	 */
	class ProjectPanel : public UI::EditorPanel
	{
	public:
		explicit ProjectPanel(EditorContext *context);

		void OnUIRender(bool &isOpen) override;

	private:
		void DrawToolbar();
		void DrawInfo(ProjectFile::SceneryProject &project);
		void DrawAirports(ProjectFile::SceneryProject &project);
		void DrawHierarchy(ProjectFile::SceneryProject &project);
		void DrawItem(ProjectFile::SceneryItem &item);
		void DrawLibraries(ProjectFile::SceneryProject &project);
		void DrawLibraryExports(ProjectFile::SceneryProject &project);
		void DrawValidation();

		EditorContext *m_Context = nullptr;
		std::string m_SelectedItem;
		std::string m_PendingRemove;				// Removed after the tree is drawn (it holds references into the hierarchy)
		ImGuiTextFilter m_ExportFilter;
	};
}

// -------------------------------------------------------

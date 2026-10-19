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
 * outliner_panel.h
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <Editor/core/editor_context.h>
#include <SceneryEditorX/renderer/ui/editor_panel.h>
#include <string>
#include <vector>
#include <entt/entity/fwd.hpp>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @class OutlinerPanel
	 * @brief "OUTLINER" panel: flat list of the active scene's entities (Substance's "LAYERS" slot).
	 * Click selects (drives the Properties panel); right-click deletes; the toolbar adds empty entities.
	 */
	class OutlinerPanel : public UI::EditorPanel
	{
	public:
		explicit OutlinerPanel(EditorContext *context);

		void OnUIRender(bool &isOpen) override;

	private:
		/* @brief Rebuilds the root list (and the search matches) when the scene structure or the filter changed. */
		void RefreshLists();

		EditorContext *m_Context = nullptr;
		ImGuiTextFilter m_Filter;

		// Walking every entity each frame to find the roots was a large part of a frame with an airport loaded.
		std::vector<entt::entity> m_Roots;
		std::vector<entt::entity> m_Matches;	// Search results (the tree flattened)
		uint64_t m_ListVersion = 0;				// Scene::GetStructureVersion() the lists were built at
		std::string m_MatchFilter;				// Filter text m_Matches was built for
	};

}

// -------------------------------------------------------

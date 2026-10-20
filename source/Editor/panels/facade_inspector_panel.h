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
 * facade_inspector_panel.h
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <Editor/core/editor_context.h>
#include <string>
#include <SceneryEditorX/renderer/ui/editor_panel.h>
#include <SceneryEditorX/xplane/xp_assets.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @class FacadeInspectorPanel
	 * @brief "FACADES" panel: what a .fac holds and how X-Plane builds with it. Floors and their roofs, wall types with
	 * their width / heading rules, spellings and the lengths they fill without stretching, a spelling preview for any
	 * wall length, segments (and their curved variants and attached objects), objects and textures. Follows the
	 * selected facade (or the armed one) unless a facade was sent to it (Inspect).
	 */
	class FacadeInspectorPanel : public UI::EditorPanel
	{
	public:
		explicit FacadeInspectorPanel(EditorContext *context);

		void OnUIRender(bool &isOpen) override;

	private:
		void DrawSummary(const std::string &resource, const XPlane::FacadeAsset &asset);
		void DrawFloors(const XPlane::FacadeAsset &asset);
		void DrawWalls(const XPlane::FacadeAsset &asset);
		void DrawSpellingPreview(const XPlane::FacadeAsset &asset);
		void DrawSegments(const XPlane::FacadeAsset &asset);

		EditorContext *m_Context = nullptr;
		bool m_Follow = true;			// Show the selected / armed facade; off: the one sent with Inspect
		int m_Floor = 0;				// Type 2 floor the walls and segments are listed for
		int m_Wall = 0;					// Wall type of the spelling preview
		float m_Length = 20.0f;			// Wall length of the spelling preview, meters
	};
}

// -------------------------------------------------------

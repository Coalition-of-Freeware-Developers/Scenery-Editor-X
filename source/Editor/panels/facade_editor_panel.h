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
 * facade_editor_panel.h
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <Editor/core/editor_context.h>
#include <map>
#include <string>
#include <SceneryEditorX/renderer/ui/editor_panel.h>
#include <SceneryEditorX/renderer/vulkan/image_resource.h>
#include <SceneryEditorX/xplane/xp_facade_authoring.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @class FacadeEditorPanel
	 * @brief "FACADE EDITOR": makes new facades. A type 1 building from a texture: its panels across (left / center /
	 * right) and floors up (bottom / middle / top) marked by dragging their edges on the texture, meters per texture,
	 * the roof's texture rectangle, the wall types' width / heading rules. A type 2 sidewalk / curb from a cross-section
	 * drawn in a profile view, its texture strip and segments. Place preview drops a footprint using it in the viewport
	 * and Live preview redraws it on every change; the .fac is saved in the project's facades folder.
	 */
	class FacadeEditorPanel : public UI::EditorPanel
	{
	public:
		explicit FacadeEditorPanel(EditorContext *context);

		void OnUIRender(bool &isOpen) override;

		/* @brief A texture drawn on the canvases. */
		struct CanvasTexture
		{
			Ref<ImageResource> image;
			int width = 0;
			int height = 0;
		};

	private:
		/* @brief A facade texture for the canvases, loaded once per file (held by the panel, so released before the device goes). */
		const CanvasTexture &GetTexture(const std::string &reference);

		void DrawHeader();
		void DrawBuilding();
		void DrawWallCanvas(XPlane::FacadeAsset::Wall &wall);
		void DrawPanelRows(XPlane::FacadeAsset::Wall &wall);
		void DrawRoofCanvas();
		void DrawCurb();
		void DrawProfile(XPlane::CurbFacadeSpec &spec);
		void DrawStripCanvas(XPlane::CurbFacadeSpec &spec);

		/* @brief A texture row: path, Choose... (imported into the project), Clear when optional. True when it changed. */
		bool TextureRow(const char *label, std::string &reference, bool optional, const char *tooltip);

		EditorContext *m_Context = nullptr;
		std::map<std::string, CanvasTexture> m_Textures;	// By resolved file
		int m_Wall = 0;					// Type 1 wall type being edited
		int m_Mode = 0;					// Canvas: 0 panels across, 1 floors up, 2 roof
		float m_Zoom = 1.0f;
		bool m_Snap = true;				// Edges snap to whole texture pixels
		bool m_SquarePixels = true;		// SCALE keeps the texture's aspect
		float m_DragCut = -1.0f;		// The panel edge being dragged (texture coordinate), -1 when none
		bool m_Dragging = false;		// A roof / strip rectangle is being drawn
		float m_DragStart[2] = { 0.0f, 0.0f };
		int m_ProfilePoint = -1;		// The cross-section point being dragged
		std::string m_LengthsText;		// The curb's segment lengths as typed
		bool m_LengthsEditing = false;	// The lengths field has the keyboard (it isn't refreshed from the spec)
		std::string m_Status;			// Last save / error message
	};
}

// -------------------------------------------------------

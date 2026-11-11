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
 * editor_tools.h
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "viewport_view.h"
#include <cstdint>
#include <string>
#include <SceneryEditorX/scene/view_framing.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct EditorContext;
}

namespace SceneryEditorX::EditorTools
{
	/**
	 * The viewport's editing tools, after the tool strip of Substance Painter and the usual 3D editors:
	 *
	 *   Q  Select  click picks what is under the cursor (Shift adds, Ctrl toggles), a drag boxes several
	 *   W  Move    \
	 *   E  Rotate   > the gizmo on the selection; clicking elsewhere still selects
	 *   R  Scale   /
	 *   V  Nodes   bezier nodes and handles of the selected pavement, lines, boundaries, runways and project shapes
	 *   B  Facade  draw facades with the armed one; pick and type the selected facades' walls (FacadeTool)
	 *
	 * Picking reads the g-buffer's pick ids back from the GPU (ScenePicking), so whatever a pixel shows can be clicked,
	 * at any scene size. The camera keeps Alt + left drag (orbit), the middle button (pan), the right button (look,
	 * with WASD / Q E to fly) and the wheel. F frames the selection (from the viewport or any panel listing it, in every
	 * keymap: EditorCamera::HandleShortcuts), Del deletes it, Esc cancels a drag or deselects,
	 * X switches the gizmo between world and local axes, Ctrl inverts snapping while dragging, Ctrl+D duplicates.
	 * A quick right-click (no drag) opens the quad menu for what is under the cursor (ViewportMenus).
	 */

	enum class Tool : uint8_t
	{
		Select,
		Move,
		Rotate,
		Scale,
		Nodes,
		Facade,
		Count
	};

	/* @brief The key that picks a tool ("Q" ... "B"). */
	const char *GetToolKey(Tool tool);

	Tool GetTool();
	void SetTool(Tool tool);
	const char *GetToolName(Tool tool);
	const char *GetToolIcon(Tool tool);
	const char *GetToolTooltip(Tool tool);

	/**
	 * @brief Viewport, inside its window after the image is drawn and before its input item: the gizmo and node handles.
	 * @return True when they take the mouse this frame (the viewport then skips its input item, which would block them).
	 */
	bool BeginViewport(EditorContext *context, const ViewportView &view, bool imageHovered);

	/**
	 * @brief Viewport, after its input item: picking (click, marquee), the hovered highlight and the overlays.
	 * @param canvasHovered The image's input item is hovered.
	 * @param renderWidth, renderHeight Size of the render target the pick ids are read from.
	 */
	void EndViewport(EditorContext *context, const ViewportView &view, bool canvasHovered, float renderWidth, float renderHeight);

	/* @brief True while a tool drags (gizmo, nodes, a marquee): the camera stays put. */
	bool IsBusy();

	/* @brief Tool keys while the viewport is hovered or focused; Del and Esc anywhere text isn't being typed. */
	void HandleShortcuts(EditorContext *context);

	/**
	 * @brief What Frame Selection (F) fits in view: the picked nodes while the tools work on nodes (the node tool, or
	 * move / rotate / scale with nodes picked), else everything selected, whatever it is and whether or not it can be
	 * edited: models and X-Plane objects (their meshes), apt.dat rows, project items, the outliner's groups, the
	 * project's folders (SelectionBounds). At least SelectionBounds::MINIMUM_SIZE each way.
	 * @return False when nothing selected has a place in the scene.
	 */
	bool GetFrameBox(ViewFraming::Box &box);

	/* @brief Moves the main viewport's camera to see the selection (GetFrameBox; gliding there). False when nothing is selected. */
	bool FrameSelection();

	/* @brief Where the middle of the viewport (as drawn last frame) meets the ground, in scene meters; false when it looks above the horizon. */
	bool GetViewCenterOnGround(float position[3]);

	/* @brief Deletes the selected nodes (node tool) or the selection: apt.dat rows and project items undoably. */
	bool DeleteSelection(EditorContext *context);

	/* @brief Esc: cancels a drag, else clears the node selection, else the selection. */
	void Escape(EditorContext *context);

	/* @brief The help / status line for the viewport's overlay. */
	std::string GetHint();
}

// -------------------------------------------------------

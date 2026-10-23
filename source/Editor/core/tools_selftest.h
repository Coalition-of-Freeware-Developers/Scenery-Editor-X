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
 * tools_selftest.h
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "viewport_view.h"

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct EditorContext;
}

namespace SceneryEditorX::ToolsSelfTest
{
	/**
	 * --tools-selftest: once the scene has settled, drives the viewport tools the way a user would and checks what
	 * they did, logging PASS / FAIL lines (tag "SelfTest"): GPU picking (the pick ids under sampled pixels belong to
	 * objects drawn there), moving / turning project items, objects and airport rows through the gizmo's code path,
	 * node edits, delete, and undo / redo of each (restoring the data bit for bit where the data is geographic).
	 * It ends with something selected, so a frame capture shows the outline. For verification runs without input.
	 */

	void Enable();
	bool IsEnabled();

	/**
	 * @brief --tools-selftest-showcase: after the checks, frames an airport pavement edited as a B-spline (control
	 * polygon) beside a bezier shape with a selected smooth node (handles), holds that for a while so a frame capture
	 * shows the node tool's drawing, then undoes it all.
	 */
	void EnableShowcase();

	/**
	 * @brief --facade-showcase: the facade tests' facades stay selected with the facade tool's wall labels, the quad
	 * menu open on the sidewalk for 1200 frames, then the hotbox for 1200 (for RenderDoc captures); then it is all undone.
	 */
	void EnableFacadeShowcase();

	/**
	 * @brief --gizmo-selftest: only the gizmo checks, driven with a scripted mouse the way a user drags: hover a handle
	 * (it lights up), press, drag, release, then compare what moved with what the cursor's path says it should be (an
	 * object along X, turned on the Y ring, a drag cancelled with Esc, a polygon scaled along X, two pavement nodes
	 * moved and turned by W / E after picking them with V), undoing each. --gizmo-showcase then holds each gizmo
	 * (hovered or mid-drag) for a few seconds, logging "Gizmo showcase: <name>" as each one starts, for screenshots.
	 */
	void EnableGizmoTests(bool showcase);

	/* @brief Once per frame from the viewport, after the tools. */
	void Tick(EditorContext *context, const ViewportView &view, float renderWidth, float renderHeight);
}

// -------------------------------------------------------

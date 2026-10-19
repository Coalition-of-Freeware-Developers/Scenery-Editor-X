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
 * transform_tool.h
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "viewport_view.h"
#include <cstdint>
#include <string>

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct EditorContext;
}

namespace SceneryEditorX::TransformTool
{
	/**
	 * The move / rotate / scale gizmo for the selection, whatever it holds: scene entities (full transforms), X-Plane
	 * objects (position and heading; up and down only when they aren't on the ground), apt.dat rows (points, runways,
	 * helipads, pavement, lines, boundaries) and project items (objects, polygons, lines, forests, facades ...).
	 *
	 * While dragging, the selection's entities move as a preview (cheap: no mesh is rebuilt). Releasing writes the edit
	 * into what it belongs to (the airport's rows, the project's items, the scene's transforms), rebuilds only the rows
	 * / items that changed and records one undo step. Esc during a drag puts everything back.
	 */

	enum class Operation : uint8_t
	{
		Translate,
		Rotate,
		Scale
	};

	struct Settings
	{
		bool localSpace = false;	// Gizmo axes follow the active object's heading
		bool pivotActive = false;	// Turn / scale about the active object instead of the selection's center
		bool snap = false;			// Ctrl while dragging inverts it
		float snapMove = 1.0f;		// Meters
		float snapAngle = 15.0f;	// Degrees
		float snapScale = 0.1f;
	};

	Settings &GetSettings();

	/* @brief What the selection allows (an object can't be scaled, ground placements stay on the ground ...). */
	struct Capabilities
	{
		uint32_t editable = 0;		// Targets the gizmo moves
		uint32_t readOnly = 0;		// Targets it leaves alone (the scenery pack's DSF art)
		bool translateY = true;
		bool rotateFree = true;		// Any axis; otherwise only about the vertical
		bool scale = true;
		bool scaleY = true;
		std::string note;			// Why something is left out, for the viewport's hint line
	};

	Capabilities GetCapabilities();

	/**
	 * @brief Draws the gizmo over the viewport image and applies drags. Call inside the viewport window, before the
	 * image's input item (ImGuizmo only activates when no ImGui item is hovered).
	 * @param allowInput False while another tool owns the mouse.
	 * @return True while the gizmo is hovered or dragged: the viewport then leaves the mouse to it.
	 */
	bool OnViewportGui(EditorContext *context, const ViewportView &view, Operation operation, bool allowInput);

	/* @brief True while a drag is in progress. */
	bool IsUsing();

	/* @brief Puts a drag in progress back where it started (Esc). */
	void Cancel();

	/* @brief A line about the drag in progress ("Move 3: 12.40 m"), empty when idle. */
	const std::string &GetStatus();

	/* @brief Center of the selection in the scene (meters), false when nothing editable is selected. */
	bool GetSelectionCenter(Point3 &center, double &radius);

	/**
	 * @brief Moves / turns / scales the selection about its pivot without the gizmo (numeric edits, nudges, tests):
	 * the same preview, write-back and undo step as a drag.
	 * @param east, north Meters. @param turnDegrees Counter-clockwise seen from above. @param scale On the ground plane.
	 */
	bool TransformSelection(double east, double north, double turnDegrees, double scale);
}

// -------------------------------------------------------

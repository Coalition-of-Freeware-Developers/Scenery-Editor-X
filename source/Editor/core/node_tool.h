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
 * node_tool.h
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "viewport_view.h"
#include <cstddef>
#include <string>

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct EditorContext;
}

namespace SceneryEditorX::NodeTool
{
	/**
	 * Node editing for the selected shapes: apt.dat pavement, painted lines / light strings and boundaries, runway
	 * ends, and the project's polygons, orthophotos, lines, strings, forests, facades and exclusions.
	 *
	 *  - Click a node to select it (Shift adds, Ctrl toggles), drag it (or a marquee of them) across the ground.
	 *    Dragged nodes snap onto other nodes they come close to, and to the grid when snapping is on.
	 *  - Selected nodes show their bezier handles: drag one to bend the curve (Shift breaks the symmetry).
	 *  - Click an edge to select its two nodes; double-click it to insert a node there (the curve keeps its shape).
	 *  - Two or more selected nodes get a gizmo to move, turn and scale them together.
	 *  - Delete, Smooth, Corner and Reverse act on the selected nodes (the toolbar, or Del).
	 * While dragging, the edited outline is drawn over the scene; releasing writes it into the airport / project,
	 * rebuilds just that row / item and records one undo step.
	 */

	/**
	 * @brief Draws the selected shapes' nodes and handles the mouse on them. Call inside the viewport window, before
	 * the image's input item.
	 * @return True when the tool takes the mouse this frame (a node, handle, edge or its gizmo is hovered or dragged).
	 */
	bool OnViewportGui(EditorContext *context, const ViewportView &view, bool allowInput);

	/* @brief True while nodes, a handle or a marquee are being dragged. */
	bool IsDragging();

	/* @brief Puts a drag in progress back (Esc). */
	void Cancel();

	/* @brief Whether the selection holds anything with nodes. */
	bool HasShapes();

	size_t GetSelectedNodeCount();
	void ClearNodeSelection();
	void SelectAllNodes();

	/* @brief Marquee: selects the nodes drawn inside a screen rectangle (adding to the selection or replacing it). False without shapes. */
	bool SelectNodesInRect(const ImVec2 &min, const ImVec2 &max, bool add);

	/* @brief Edits on the selected nodes (each one undo step). False when nothing could be done. */
	bool TranslateSelectedNodes(double east, double north);
	bool DeleteSelectedNodes();
	bool SmoothSelectedNodes();
	bool CornerSelectedNodes();
	bool ReverseSelectedShapes();

	/* @brief A line about the drag in progress or the node selection, empty when idle. */
	const std::string &GetStatus();
}

// -------------------------------------------------------

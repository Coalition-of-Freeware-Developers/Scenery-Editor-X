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
 * selection_bounds.h
 * -------------------------------------------------------
 * Created: 10/11/2026
 * -------------------------------------------------------
 */
#pragma once
#include <cstdint>
#include <vector>
#include <Editor/core/editor_selection.h>
#include <SceneryEditorX/scene/view_framing.h>

// -------------------------------------------------------

namespace SceneryEditorX::SelectionBounds
{
	/**
	 * Where things are in the scene, for Frame Selection (F): the box each kind of selection target covers, whether
	 * or not the tools may change it (the pack's DSF art, locked items, the airport's groups and the project's folders
	 * frame like anything else). Boxes are what is drawn (meshes through their world transforms, the instances of
	 * instanced ones), plus what a target is made of when it draws little or nothing itself: an apt.dat row's footprint,
	 * a project item's rings or position, a folder's items. Main thread only; walks the scene, so call it on demand
	 * (a key press), not every frame. One call walks at most 100,000 entities and 200,000 instances, however many
	 * targets it frames: past that the box holds what was measured (and a warning says so).
	 */

	/* @brief Smallest box a framing uses (meters each way): a point, a node or a flat drape still frames from a little way off. */
	constexpr double MINIMUM_SIZE = 4.0;

	/**
	 * @brief Grows a box by everything an entity and the entities below it draw (an imported model, an X-Plane
	 * object, an outliner group). Entities that draw nothing (a light, an empty group) count where they are, when
	 * nothing below the entity draws.
	 * @return False when the entity is gone or nothing of it could be framed.
	 */
	bool AddEntity(uint64_t entity, ViewFraming::Box &box);

	/**
	 * @brief Grows a box by one selection target: a scene entity with everything below it (the airport's root frames
	 * as the airport, its outliers left out), an apt.dat row with its parts and light objects, or a project item (what
	 * it draws, its rings or position, the items of a folder or an airport).
	 * @return False when the target has no place in the scene.
	 */
	bool AddTarget(const EditorSelection::Target &target, ViewFraming::Box &box);

	/* @brief Grows a box by every target of the selection. False when none of them could be framed. */
	bool AddSelection(ViewFraming::Box &box);

	/* @brief Grows a box by points on the ground (scene meters, x east / z south), at the terrain's height. */
	bool AddGroundPoints(const std::vector<XPlane::Point2> &points, ViewFraming::Box &box);

	/* @brief Grows a box by everything the scene draws (Frame All without an airport). False when it draws nothing. */
	bool AddScene(ViewFraming::Box &box);
}

// -------------------------------------------------------

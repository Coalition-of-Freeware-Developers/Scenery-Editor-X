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
 * viewport_gizmo.h
 * -------------------------------------------------------
 * Created: 22/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "viewport_view.h"
#include <cstdint>
#include <string>

// -------------------------------------------------------

namespace SceneryEditorX::ViewportGizmo
{
	/**
	 * The move / rotate / scale gizmo drawn over the viewport at the selection's pivot, looking and handling the way
	 * modeling packages and game engines draw theirs: red, green and blue for X (east), Y (up) and Z (south).
	 *
	 *  - Move: an arrow per axis, a square per plane (on the side facing the camera) and a center circle that slides
	 *    across the ground (across the view in side views).
	 *  - Rotate: a ring per axis (its near half), and an outer ring that turns about the view direction. While
	 *    turning, a pie shows the angle so far.
	 *  - Scale: a box at the end of each axis, plane squares for two axes, and a center box for all of them.
	 *
	 * The hovered handle lights up yellow. It keeps its size on screen wherever it is, axes facing the camera fade
	 * out, and while dragging the others dim and a label at the cursor reads the amount. One gizmo is shown at a time:
	 * the tools call Manipulate() every frame they show it, then Draw() (after their own overlay, so it sits on top).
	 */

	enum class Mode : uint8_t
	{
		Translate,
		Rotate,
		Scale
	};

	/* @brief The handles a gizmo shows: what the selection allows. */
	enum Handle : uint32_t
	{
		HandleNone	= 0,
		AxisX		= 1u << 0,
		AxisY		= 1u << 1,
		AxisZ		= 1u << 2,
		PlaneYZ		= 1u << 3,	// Its normal is X
		PlaneXZ		= 1u << 4,	// Its normal is Y: the ground
		PlaneXY		= 1u << 5,	// Its normal is Z
		Center		= 1u << 6,	// Move: across the ground (the view plane in side views); scale: every axis at once
		ViewRing	= 1u << 7,	// Rotate: about the view direction

		Axes		= AxisX | AxisY | AxisZ,
		Planes		= PlaneYZ | PlaneXZ | PlaneXY,
		Everything	= Axes | Planes | Center,
		Ground		= AxisX | AxisZ | PlaneXZ | Center	// Moves and scales that keep things on the ground
	};

	/* @brief Where the gizmo sits and which way its axes point (orthonormal, Y up: local axes only turn about it). */
	struct Frame
	{
		Point3 pivot;
		Point3 axes[3] = { { 1.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0 }, { 0.0, 0.0, 1.0 } };

		/* @brief Axes turned about the vertical (radians, counter-clockwise seen from above). */
		static Frame Turned(const Point3 &pivot, double yaw);
	};

	struct Options
	{
		Mode mode = Mode::Translate;
		uint32_t handles = 0;
		bool uniformGround = false;		// Scale's center box scales X and Z only (things that stay flat on the ground)
		bool snap = false;
		double snapMove = 1.0;			// Meters
		double snapAngle = 15.0;		// Degrees
		double snapScale = 0.1;			// Factor steps
	};

	/* @brief A transform in the row-vector convention the tools use (p' = p * M), double precision. */
	struct Matrix
	{
		double m[4][4] = { { 1.0, 0.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0, 0.0 }, { 0.0, 0.0, 0.0, 1.0 } };
	};

	struct Result
	{
		bool hovered = false;	// A handle is under the cursor: the viewport leaves the mouse to the gizmo
		bool active = false;	// A drag is in progress (its delta is below)
		bool started = false;	// The drag began this frame
		bool finished = false;	// The drag ended this frame with the button released: write the delta
		Matrix delta;			// The drag so far as a world transform about the start's pivot
	};

	/**
	 * @brief Hit-tests and drags the gizmo. Call inside the viewport window every frame it shows, before the image's
	 * input item, then Draw(). While dragging, the frame passed in is ignored: the drag works from where it started.
	 * @param owner Who shows it (a drag doesn't carry over to another owner, or past a frame it wasn't shown in).
	 * @param allowInput False while something else owns the mouse (a drag in progress continues regardless).
	 */
	Result Manipulate(uint32_t owner, const ViewportView &view, const Frame &frame, const Options &options, bool allowInput);

	/* @brief Draws the gizmo Manipulate() handled this frame (nothing when it wasn't called, or is off screen). */
	void Draw(const ViewportView &view);

	/* @brief True while a drag is in progress. */
	bool IsActive();

	/* @brief Ends a drag in progress without a result; the mouse is ignored until the button is released. */
	void Cancel();

	// For tests that drive the gizmo with the mouse

	/* @brief Where a handle sits on screen (its grip point) as last handled; false when it isn't shown or grabbable. */
	bool GetHandleScreen(Handle handle, ImVec2 &position);

	/* @brief The handle under the cursor as last handled (the one dragged while a drag is in progress). */
	Handle GetHovered();

	/* @brief The frame the gizmo was last handled at (the start's while dragging); false before the first. */
	bool GetFrame(Frame &frame);
}

// -------------------------------------------------------

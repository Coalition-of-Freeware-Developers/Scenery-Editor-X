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
 * view_framing.h
 * -------------------------------------------------------
 * Created: 10/11/2026
 * -------------------------------------------------------
 */
#pragma once

// -------------------------------------------------------

namespace SceneryEditorX::ViewFraming
{
	/**
	 * Fitting what is selected into a view (Frame Selection, F): the box a selection covers in the scene, and how far
	 * back a camera looking along a direction has to sit for that box to fill the view. Scene meters, double precision
	 * (airport-scale coordinates). Standard library only.
	 */

	struct Point
	{
		double x = 0.0;
		double y = 0.0;
		double z = 0.0;
	};

	/* @brief Coordinates further out than this (meters from the scene origin) are bad data, never framed. */
	constexpr double MAX_COORDINATE = 1.0e7;

	/* @brief The share of the view the framed box fills in its tighter direction (a margin around it). */
	constexpr double DEFAULT_FILL = 0.9;

	/* @brief How far in front of the eye the nearest corner of a framed box stays, in meters. */
	constexpr double DEFAULT_NEAR_MARGIN = 0.5;

	/* @brief True for a point that can be framed: finite, and within MAX_COORDINATE of the origin on every axis. */
	[[nodiscard]] bool IsUsable(const Point &point);

	/**
	 * @struct Box
	 * @brief An axis-aligned box in scene meters, empty until something is added to it. What can't be framed is left
	 * out as it comes in (non-finite values, far-out ones, boxes whose min is above their max), so one bad bound in a
	 * selection doesn't throw the camera to the edge of the world.
	 */
	struct Box
	{
		Point min;
		Point max;
		bool valid = false;

		/* @brief Grows the box to hold a point. False (and the box unchanged) when the point can't be framed. */
		bool Add(const Point &point);

		/* @brief Grows the box to hold another box given by its corners. False when either corner is unusable or min > max. */
		bool Add(const Point &boxMin, const Point &boxMax);

		/* @brief Grows the box to hold another one (an empty one changes nothing). */
		bool Add(const Box &box);

		[[nodiscard]] bool IsValid() const { return valid; }
		[[nodiscard]] Point GetCenter() const;
		[[nodiscard]] Point GetSize() const;

		/* @brief Half the diagonal: the radius of the sphere around the box. */
		[[nodiscard]] double GetRadius() const;

		/**
		 * @brief Widens each axis shorter than `minimumSize` meters to that size about the box's center, so a point, a
		 * node or a flat drape frames as something the camera can stand back from.
		 */
		void Inflate(double minimumSize);
	};

	/**
	 * @struct Lens
	 * @brief A camera's direction and field of view: unit, mutually perpendicular axes (forward into the view, right
	 * and up on the screen) and the tangents of half the horizontal and vertical fields of view. Orthographic views show
	 * distance * tanHalfY meters from the middle to the top edge (and distance * tanHalfX to the side), the editor
	 * rig's convention, so the same distance frames the same in either projection.
	 */
	struct Lens
	{
		Point forward{ 0.0, 0.0, 1.0 };
		Point right{ -1.0, 0.0, 0.0 };
		Point up{ 0.0, 1.0, 0.0 };
		double tanHalfX = 0.5773502691896258;	// 60 degrees horizontally
		double tanHalfY = 0.3247595264191645;	// 16:9
		bool orthographic = false;
	};

	/**
	 * @brief How far behind the box's center (along lens.forward) the eye has to be for every corner of the box to be
	 * in view, the box filling `fill` of the view in its tighter direction, and the nearest corner at least `nearMargin`
	 * in front of the eye. Exact for the box's corners (the view's frustum is convex, so the whole box is then in view):
	 * a long runway seen end-on frames closer than its bounding sphere would. 0 for an empty box.
	 */
	[[nodiscard]] double FitDistance(const Box &box, const Lens &lens, double fill = DEFAULT_FILL, double nearMargin = DEFAULT_NEAR_MARGIN);

	/* @brief The same for a sphere around a center (the camera at this distance sees the whole sphere). */
	[[nodiscard]] double FitSphereDistance(double radius, const Lens &lens, double fill = DEFAULT_FILL, double nearMargin = DEFAULT_NEAR_MARGIN);

	/**
	 * @brief Whether a point is inside the view of an eye at `distance` behind `pivot` (looking along lens.forward),
	 * within `fill` of the view and at least `nearMargin` in front of the eye. For checking a framing.
	 */
	[[nodiscard]] bool IsInView(const Point &point, const Point &pivot, double distance, const Lens &lens, double fill = 1.0, double nearMargin = 0.0);
}

// -------------------------------------------------------

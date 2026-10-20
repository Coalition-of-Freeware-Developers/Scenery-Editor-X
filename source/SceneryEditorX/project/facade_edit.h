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
 * facade_edit.h
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "spline_edit.h"
#include <SceneryEditorX/xplane/xp_facade.h>
#include <string>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::FacadeEdit
{
	/**
	 * Facade footprint tools on a Splines::Chain in local meters (+X east, +Z south): what the editor's facade tool,
	 * PROPERTIES and context menus call to build, fix and rebuild facades. A footprint runs counter-clockwise seen
	 * from above so its walls face out; its first segment is the facade's front, and each node carries the wall type
	 * of the segment starting at it. Nodes an edit doesn't move keep their source coordinates (see spline_edit.h), so
	 * a git diff shows only what changed. Standard library only.
	 */

	using XPlane::Point2;

	/* @brief One wall of a footprint: the segment from node `index` to the next. */
	struct WallInfo
	{
		uint32_t index = 0;
		Point2 start;
		Point2 end;
		double length = 0.0;		// Meters along the curve
		double heading = 0.0;		// True heading the wall faces (outward, from its chord), degrees
		bool curved = false;
		int wallType = 0;
	};

	std::vector<WallInfo> GetWalls(const Splines::Chain &chain);

	/* @brief The walls as the facade layout reads them (XPlane::LayoutFacade). */
	std::vector<XPlane::FacadeSideInput> ToSideInputs(const std::vector<WallInfo> &walls);

	/* @brief Signed area of the flattened outline, square meters: positive when it runs counter-clockwise seen from above. */
	double GetSignedArea(const Splines::Chain &chain);

	/* @brief Sum of the wall lengths, meters. */
	double GetPerimeter(const Splines::Chain &chain);

	/**
	 * @brief Replaces every curved wall with straight pieces about pieceLength long, spaced evenly along the curve; the
	 * new nodes are corners carrying their wall's type. This is the recipe the 3D sidewalk article gives for curved
	 * facades (X-Plane stretches curb gaps toward curve vertices and WorldEditor doesn't preview curved walls): pieces
	 * of 0.5 - 0.8 m, handles made linear.
	 * @param added Receives the number of nodes added.
	 * @return False when the chain has no curved walls.
	 */
	bool SubdivideCurves(Splines::Chain &chain, double pieceLength = 0.65, size_t *added = nullptr);

	/**
	 * @brief Squares a building's corners: walls within toleranceDegrees of the footprint's main axes (found from
	 * every wall, longer ones counting more) are turned onto them and each corner moves to where its walls meet.
	 * Walls further off keep their direction. Curved footprints are refused (subdivide or straighten them first).
	 * @return False when nothing moved (why says why when there was a reason).
	 */
	bool SquareCorners(Splines::Chain &chain, double toleranceDegrees = 15.0, std::string *why = nullptr);

	/* @brief Makes a wall the front (segment 0) of a closed footprint by starting the chain at its node. False for open chains or wall 0. */
	bool SetFrontWall(Splines::Chain &chain, uint32_t wall);

	/* @brief Reverses a closed footprint running clockwise (its walls faced in). True when it was reversed. */
	bool MakeCounterClockwise(Splines::Chain &chain);

	/* @brief Sets the wall type of walls (all of them when the list is empty). True when one changed. */
	bool SetWallTypes(Splines::Chain &chain, const std::vector<uint32_t> &walls, int type);

	/**
	 * @brief A new footprint parallel to a chain at a distance: outward (away from the inside of a closed footprint,
	 * to the right of travel for an open one) when positive. Curves are flattened (pieces of about curveStep meters);
	 * corners are mitered, at most 4 x the distance out. Each node keeps the wall type of the wall it came from.
	 */
	Splines::Chain Offset(const Splines::Chain &chain, double distance, double curveStep = 1.0);

	/**
	 * @brief A rectangle with a->b as its front wall and `depth` meters deep: to the left of a->b when positive (the
	 * front faces right, away from the inside), to the right when negative (then the front runs b->a). Counter-clockwise.
	 */
	Splines::Chain MakeRectangle(const Point2 &a, const Point2 &b, double depth);

	/* @brief A regular polygon around a center (at least 3 sides), counter-clockwise, its first wall facing south. */
	Splines::Chain MakeCircle(const Point2 &center, double radius, int sides);

	/* @brief A footprint through points (corner nodes); a closed one is made counter-clockwise. */
	Splines::Chain MakeChain(const std::vector<Point2> &points, bool closed);

	/* @brief Sides a circle needs so its walls are about `wallLength` long (at least 6, at most 128). */
	int GetCircleSides(double radius, double wallLength);

	/**
	 * @brief A wall type carried to a facade with other wall names: the new facade's wall of the same name when it has
	 * one, else the same number (clamped to its walls).
	 */
	int RemapWallType(int type, const std::vector<std::string> &from, const std::vector<std::string> &to);
}

// -------------------------------------------------------

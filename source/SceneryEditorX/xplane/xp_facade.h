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
 * xp_facade.h
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "xp_apt_geometry.h"
#include "xp_assets.h"
#include <cstdint>
#include <string>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	/**
	 * How X-Plane builds a facade along its footprint, shared by the overlay builder (drawing) and the editor's facade
	 * tools (wall labels, the inspector, fitting walls to doors): which floor a height picks, which wall each side gets
	 * and which segment spelling fills it, after the facade spec and WorldEditor's preview (WED_FacadePreview.cpp).
	 * Standard library only.
	 *
	 * Facade frame: a footprint runs counter-clockwise seen from above, so each wall faces out to the right of travel.
	 * Side 0 is the facade's front: wall headings are measured from it.
	 */

	/* @brief How a facade is drawn, as the editor's tools see it (a guess from the asset: X-Plane doesn't record a purpose). */
	enum class FacadeFamily : uint8_t
	{
		Building,	// Closed footprint with a roof
		Ground,		// Closed and under a meter high: 3D sidewalks, curbs, planters (the roof is the walking surface)
		Enclosure,	// Closed without a roof: walls or fences around an area
		Line		// RING 0, an open run of walls: fences, hangars built along their length, hangar doors
	};

	const char *GetFacadeFamilyName(FacadeFamily family);
	FacadeFamily GetFacadeFamily(const FacadeAsset &asset);

	/* @brief The wall type names (type 2: the first floor's walls, as WorldEditor lists them); "Wall N" for a row without a name. */
	std::vector<std::string> GetFacadeWallNames(const FacadeAsset &asset);

	/* @brief Number of wall types the facade has (type 2: its first floor's). */
	int GetFacadeWallCount(const FacadeAsset &asset);

	/* @brief A wall type's width / heading conditions for display: "any width", "12 - 500 m, facing 135 - 225 deg"... (type 2: of a floor) */
	std::string DescribeFacadeWall(const FacadeAsset &asset, int wall, int floor = 0);

	/* @brief The heights a facade can be built at. */
	struct FacadeHeights
	{
		std::vector<float> choices;		// Type 2: every floor's height (top roof), sorted, unique: what a height snaps to
		float minimum = 0.0f;			// Meters
		float maximum = 0.0f;
		float floorHeight = 0.0f;		// Type 1: one repeated (middle) floor, 0 when the walls have none
		bool fixed = false;				// One height only
	};

	FacadeHeights GetFacadeHeights(const FacadeAsset &asset);

	/* @brief "0.15 m (fixed)", "4 - 30 m, 12 heights", "3.5 m floors, 7 - 96 m" */
	std::string DescribeFacadeHeights(const FacadeHeights &heights);

	/* @brief The height a facade is built at for an asked height: type 2 the chosen floor's roof, type 1 the height itself (at least a floor). */
	double SnapFacadeHeight(const FacadeAsset &asset, double height);

	/* @brief The type 2 floor X-Plane builds for a height: the one whose top roof is closest to it. -1 without floors. */
	int ChooseFacadeFloor(const FacadeAsset &asset, double height);

	/* @brief The roof of a floor closest to a height (the floor's own height when it has no roof). */
	double GetFacadeRoofHeight(const FacadeAsset::Floor &floor, double height);

	/* @brief One side of a footprint, as the layout needs it. */
	struct FacadeSideInput
	{
		double length = 0.0;			// Meters along the wall (along its curve when it is curved)
		double heading = 0.0;			// True heading the wall faces (outward), degrees
		int wallType = 0;				// The wall the DSF asks for (used with pickWalls)
	};

	struct FacadeWallLayout
	{
		int wall = 0;					// Wall type built
		bool chosen = false;			// Picked by X-Plane's width / heading rules (no wall given)
		bool fits = true;				// The wall's width / heading rules accept this side
		std::vector<int> spelling;		// Type 2 segment indices, start to end
		double total = 0.0;				// Their natural width, meters
		double stretch = 1.0;			// Side length / total (1 = the segments fit exactly)
	};

	struct FacadeLayout
	{
		int floor = -1;					// Type 2 floor, -1 for type 1
		double roofHeight = 0.0;		// Meters: type 2 the floor's roof, type 1 the asked height
		std::vector<FacadeWallLayout> walls;
	};

	/**
	 * @brief The wall type a side gets: with pickWalls the requested one (clamped to the facade's), else the first whose
	 * WALL / WALL_RULE width and heading conditions accept the side (X-Plane picks one of those at random), else 0.
	 * @param relativeHeading The side's heading relative to the front (side 0), degrees clockwise.
	 * @param fits Receives whether the wall's conditions accept the side.
	 */
	int ChooseFacadeWall(const FacadeAsset &asset, int floor, double length, double relativeHeading, int requested, bool pickWalls, bool *fits = nullptr);

	/**
	 * @brief The segments a type 2 wall of a length is built from, after WorldEditor's UTL_pick_spelling: while the wall
	 * is longer than what is chosen so far plus the longest spelling, a middling spelling is added; then the spelling
	 * bringing the total closest to the length (if any does). The wall is then stretched to fit exactly.
	 * @param total Receives the spelling's natural width.
	 */
	std::vector<int> PickFacadeSpelling(const FacadeAsset::Floor &floor, const FacadeAsset::WallRule &wall, double length, double &total);

	/* @brief Lays out every side of a footprint (relative headings are measured from side 0). */
	FacadeLayout LayoutFacade(const FacadeAsset &asset, const std::vector<FacadeSideInput> &sides, bool pickWalls, double height);

	/**
	 * @brief Wall lengths a type 2 wall type is built at without stretching (spelling widths and the totals the spelling
	 * choice reaches from them), sorted, up to maxLength. Hangar doors and other fixed-size walls look right at these.
	 * Empty for type 1 facades (their panels stretch).
	 */
	std::vector<double> GetFacadeNaturalLengths(const FacadeAsset &asset, int floor, int wall, double maxLength = 200.0, size_t limit = 64);

	/* @brief The natural length closest to a length, or the length itself when there are none. */
	double SnapFacadeLength(const std::vector<double> &natural, double length);

	/* @brief The outward normal of a wall running along a direction (to the right of travel). */
	inline Point2 GetFacadeOutward(const Point2 &direction) { return { -direction.z, direction.x }; }

	/**
	 * @class FacadePath
	 * @brief A wall's base line in local meters: two points when straight, the flattened curve when curved. Template
	 * geometry is bent along it: a template point `along` meters down the wall and `out` meters outward lands at
	 * Evaluate(along) + outward * out.
	 */
	class FacadePath
	{
	public:
		FacadePath() = default;
		explicit FacadePath(std::vector<Point2> points);

		[[nodiscard]] double GetLength() const { return m_Distances.empty() ? 0.0 : m_Distances.back(); }
		[[nodiscard]] const std::vector<Point2> &GetPoints() const { return m_Points; }
		[[nodiscard]] bool IsCurved() const { return m_Points.size() > 2; }

		/**
		 * @brief Position and unit direction at a distance along the path. Past either end it continues straight; at a
		 * kink between pieces the direction is the average of both, so bent walls don't tear open.
		 */
		void Evaluate(double distance, Point2 &position, Point2 &direction) const;

	private:
		std::vector<Point2> m_Points;
		std::vector<double> m_Distances;	// Running length at each point
	};
}

// -------------------------------------------------------

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
 * xp_autogen.h
 * -------------------------------------------------------
 * Created: 22/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "xp_apt_geometry.h"
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	/**
	 * X-Plane 10+ autogen (https://developer.x-plane.com/article/x-plane-10-autogen-scenery-file-format/ and the
	 * "Autogen String (.ags)" / "Autogen Block (.agb)" specifications): points (.agp), strings (.ags) and blocks (.agb),
	 * and their layout over a DSF placement as ground tiles, objects, facades and trees in a local frame (x east,
	 * z south). Standard library only.
	 *
	 * The specifications leave the geometry partly open. These are ASSUMPTIONS, read off X-Plane 12's own autogen and
	 * to be checked against X-Plane (plan Phase 16):
	 *   - ROTATION turns a tile's texture n x 90 degrees counter-clockwise; afterwards the tile's bottom edge is its front.
	 *     A string's tiles stand with their front on the polygon side and reach into the polygon (left of a
	 *     counter-clockwise side), ANCHOR_PT on the side line; a point's texture up faces the DSF heading (an unrotated
	 *     point at heading 0 lies north up; the spec says its bottom edge faces north, but WorldEditor draws it north up
	 *     and so does X-Plane 12: X-Coder's KDEN concourses); a block's grid has its bottom on the first polygon side.
	 *   - Annotation headings are clockwise from the texture's up direction (house fronts in the shipped tiles face the
	 *     street with that reading).
	 *   - Strings: corners first (3-tile corners joined at their pivot points, the middle tile turned half the corner's
	 *     turn), then each side filled with spellings left to right (priority groups in file order, a random fitting
	 *     spelling within the first group that has one), then a cap flush with the side's end. A tile is skipped when a
	 *     stop point falls outside the polygon or inside a tile placed before, or it covers a stop point placed before
	 *     (X-Plane slides it instead). SPELLING_SET variants are picked like an AG block's.
	 *   - Blocks: the spelling whose EDGE_BEST extent is closest to the block's width (depth for rows) is used; slop
	 *     columns / rows take up the difference between their minimum (0) and maximum (width - overlap); a compressed
	 *     slop tile drops the annotations it no longer shows. FLIP_180 isn't supported (no shipped block uses it).
	 */

	enum class AutogenType : uint8_t
	{
		Point,		// .agp AG_POINT
		String,		// .ags AG_STRING
		Block		// .agb AG_BLOCK
	};

	/* @brief What a tile (or a block's grid) puts on the ground, in texture pixels. */
	struct AutogenAnnotations
	{
		struct Object
		{
			enum class Mode : uint8_t
			{
				Draped,		// OBJ_DRAPED: on the ground under its origin
				Graded,		// OBJ_GRADED / OBJ_DELTA: on the ground at the tile's ground point (plus delta)
				Scraper		// OBJ_SCRAPER: graded, raised by the element's height past min in steps
			};

			int object = 0;					// Index into AutogenAsset::objects
			float s = 0.0f, t = 0.0f;		// Pixels
			float heading = 0.0f;			// Degrees clockwise from the texture's up
			Mode mode = Mode::Draped;
			float delta = 0.0f;				// OBJ_DELTA meters
			float scraperMin = 0.0f, scraperMax = 0.0f, scraperStep = 0.0f;
			int lo = 1, hi = 1;				// Object detail levels it appears between (absent: always)
			bool hasGroundFormula = false;	// AG block GROUND_PT_FORMULA
			float groundFormula[4] = { 0.0f, 0.0f, 0.0f, 0.0f };	// x ratio, x delta, y ratio, y delta
		};

		struct Facade
		{
			int facade = 0;					// Index into AutogenAsset::facades
			float height = 0.0f;			// Meters
			std::vector<float> points;		// s, t pairs (clockwise on the texture)
			std::vector<int> walls;			// FAC_WALLS: a wall type per point; empty for FAC
		};

		struct Tree
		{
			float s = 0.0f, t = 0.0f;
			float heading = 0.0f;
			float width = 0.0f;				// Meters
			int layer = 0;
		};

		struct TreeLine
		{
			float s1 = 0.0f, t1 = 0.0f, s2 = 0.0f, t2 = 0.0f;
			int layer = 0;
		};

		std::vector<Object> objects;
		std::vector<Facade> facades;
		std::vector<Tree> trees;
		std::vector<TreeLine> treeLines;

		[[nodiscard]] bool IsEmpty() const { return objects.empty() && facades.empty() && trees.empty() && treeLines.empty(); }
	};

	/* @brief A tile of a point or string: a rectangle of the texture with what stands on it. */
	struct AutogenTile
	{
		int id = 0;
		float left = 0.0f, bottom = 0.0f, right = 0.0f, top = 0.0f;	// Pixels
		int rotation = 0;					// Counter-clockwise quarter turns
		float metersPerPixelS = 1.0f;		// From TEXTURE_SCALE and TEXTURE_WIDTH / TEXTURE_HEIGHT in effect at the TILE
		float metersPerPixelT = 1.0f;
		bool hasAnchor = false;
		float anchor[2] = { 0.0f, 0.0f };
		bool hasGround = false;
		float ground[2] = { 0.0f, 0.0f };
		std::vector<float> crop;			// s, t pairs (clockwise); empty: the rectangle
		std::vector<float> stops;			// STOP_PT s, t pairs
		bool hasLeadPivot = false, hasTrailPivot = false;
		float leadPivot[2] = { 0.0f, 0.0f };
		float trailPivot[2] = { 0.0f, 0.0f };
		AutogenAnnotations annotations;

		[[nodiscard]] float GetAnchorS() const { return hasAnchor ? anchor[0] : 0.5f * (left + right); }
		[[nodiscard]] float GetAnchorT() const { return hasAnchor ? anchor[1] : 0.5f * (bottom + top); }
	};

	/* @brief A string's corner rule (CORNER, CORNER_LEN, ATOMIC_SPELLING_CORNER, CORNER_PAIR). */
	struct AutogenCorner
	{
		float angleMin = 0.0f, angleMax = 0.0f;		// Degrees of turn walking the outside, right turns positive
		float len1Min = 0.0f, len1Max = 1e9f;		// Previous / next segment lengths (CORNER_LEN)
		float len2Min = 0.0f, len2Max = 1e9f;
		bool pair = false;							// CORNER_PAIR: two consecutive corners
		float angle2Min = 0.0f, angle2Max = 0.0f;	// Its second corner
		float pairLenMin = 0.0f, pairLenMax = 1e9f;	// The side between them
		int group = 0;								// Previous corner placed must be in it (0: any)
		bool atomic = false;
		std::vector<int> tiles;						// 1 or 3 ids: preceding, middle, trailing
		int order[3] = { 1, 0, 2 };					// CORNER_ORDER (placement order: middle, preceding, trailing)
		int priority = 0;							// Priority group, tried in file order
	};

	/* @brief A string's side spelling (SPELLING, ATOMIC_SPELLING). */
	struct AutogenSpelling
	{
		std::vector<int> tiles;
		bool atomic = false;
		float lenMin = 0.0f, lenMax = 1e9f;			// Side lengths it's used for (ATOMIC_SPELLING)
		int group = 0;								// Previous tile placed must be in it (0: any)
		int priority = 0;
	};

	/* @brief A grid of an AG block: its cuts (pixels), slop rows / columns, edges and annotations. */
	struct AutogenGrid
	{
		std::vector<float> cutsH;					// Column lines (s)
		std::vector<float> cutsV;					// Row lines (t)
		std::vector<float> slopH;					// Per column: overlap of a slop column, -1 when it isn't one
		std::vector<float> slopV;
		std::vector<float> antislopH;				// Per column: pixels it reaches back over the previous one
		std::vector<float> antislopV;
		float edgeMin[4] = { 0, 0, 0, 0 };			// left bottom right top (pixels)
		float edgeBest[4] = { 0, 0, 0, 0 };
		float edgeMax[4] = { 0, 0, 0, 0 };
		bool hasEdges = false;
		float metersPerPixelS = 1.0f;
		float metersPerPixelT = 1.0f;
		AutogenAnnotations annotations;				// In grid pixels
	};

	/* @brief A spelling set (AG blocks, and strings that have them). */
	struct AutogenSpellingSet
	{
		int param = -1;								// Block code it serves (-1: any)
		float heightMin = 0.0f, heightMax = 1e9f;
		float widthMin = 0.0f, widthMax = 1e9f;
		float depthMin = 0.0f, depthMax = 1e9f;
		float frequency = 1.0f;
		int grid = 0;								// Blocks: USE_GRID
		std::vector<std::vector<int>> spellingsS;	// Blocks: column lists
		std::vector<std::vector<int>> spellingsT;	// Blocks: row lists
		std::vector<AutogenCorner> corners;			// Strings
		std::vector<AutogenSpelling> spellings;
		std::vector<int> caps;
	};

	struct AutogenAsset
	{
		AutogenType type = AutogenType::Point;
		std::string texture;						// Ground tiles' albedo, relative to the file
		float textureWidth = 1.0f;					// TEXTURE_SCALE pixels (what UVs divide by)
		float textureHeight = 1.0f;
		bool hideTiles = false;						// HIDE_TILES / HIDE_TILE
		std::string layerGroup = "footprints";
		int layerOffset = 0;
		std::string vegetation;						// VEGETATION .for
		int fillLayer = -1;							// FILL_LAYER (strings)
		float creaseLeft = -30.0f;					// CREASE_ANGLE: turns at or past these are corners (left negative)
		float creaseRight = 30.0f;
		bool shareY = false;
		bool crop = false;							// Blocks: CROP 1
		bool flip180 = false;
		std::vector<std::string> objects;			// OBJECT rows
		std::vector<std::string> facades;			// FACADE rows
		std::vector<AutogenTile> tiles;				// Points and strings
		std::vector<std::pair<int, std::vector<int>>> groups;	// GROUP id -> tile ids
		std::vector<AutogenGrid> grids;				// Blocks
		std::vector<AutogenSpellingSet> spellingSets;	// Strings without SPELLING_SET get one

		[[nodiscard]] const AutogenTile *FindTile(int id) const;
		[[nodiscard]] bool IsInGroup(int group, int tileId) const;
	};

	bool ParseAutogenAsset(std::string_view text, AutogenAsset &out, std::string *error = nullptr);
	bool LoadAutogenAsset(const std::filesystem::path &path, AutogenAsset &out, std::string *error = nullptr);

	// -------------------------------------------------------
	// Layout

	/* @brief What an autogen placement puts down, in the local frame (x east, z south). */
	struct AutogenLayout
	{
		struct Ground
		{
			std::vector<Point2> ring;				// Counter-clockwise from above
			std::vector<std::pair<float, float>> uvs;	// Per ring point, X-Plane UVs (pixels / TEXTURE_SCALE)
		};

		struct Object
		{
			int object = 0;
			Point2 at;
			double heading = 0.0;					// True degrees
			bool graded = false;					// On the ground under `ground` rather than under itself
			Point2 ground;
			float raise = 0.0f;						// Meters above that ground (OBJ_DELTA, scrapers)
		};

		struct Facade
		{
			int facade = 0;
			float height = 0.0f;
			std::vector<Point2> points;				// As authored, turned onto the ground
			std::vector<int> walls;
		};

		struct Tree
		{
			Point2 at;
			double heading = 0.0;
			float width = 0.0f;
			int layer = 0;
		};

		struct TreeLine
		{
			Point2 from, to;
			int layer = 0;
		};

		/* @brief FILL_LAYER: trees over the polygon (even-odd over its contours) outside the placed tiles. */
		struct Fill
		{
			std::vector<std::vector<Point2>> contours;
			std::vector<std::vector<Point2>> footprints;
			int layer = 0;
		};

		std::vector<Ground> ground;
		std::vector<Object> objects;
		std::vector<Facade> facades;
		std::vector<Tree> trees;
		std::vector<TreeLine> treeLines;
		std::vector<Fill> fills;
		uint32_t tiles = 0;							// Tiles placed
		uint32_t skipped = 0;						// Tiles left out (stop points, no room)
	};

	struct AutogenLayoutOptions
	{
		/**
		 * X-Plane's object detail level 1..6 (reno/draw_objs_06): an annotation with lo / hi appears always at or above
		 * hi, never below lo, and with chance (level - lo + 1) / (hi - lo + 1) between. 5 is X-Plane 12's default
		 * ("high" 3-d detail); 0 places none.
		 */
		int objectLevel = 5;
		uint32_t seed = 1;
	};

	/* @brief An .agp at a point and true heading: one tile (picked at random when there are several). */
	void LayoutAutogenPoint(const AutogenAsset &asset, Point2 at, double headingDegrees, const AutogenLayoutOptions &options, AutogenLayout &out);

	/* @brief How far an .agp's tiles reach from their anchor, meters (the farthest corner of any tile). */
	double GetAutogenReach(const AutogenAsset &asset);

	/**
	 * @brief An .ags over a DSF polygon: its contours as polylines (a closed one repeats its first point), the first
	 * `spawning` of them carrying tiles, `height` the element's height (meters, the parameter's high byte x 4).
	 */
	void LayoutAutogenString(const AutogenAsset &asset, const std::vector<std::vector<Point2>> &contours, int spawning, float height,
							 const AutogenLayoutOptions &options, AutogenLayout &out);

	/* @brief An .agb over a 4-point counter-clockwise DSF polygon, with its block code (low byte) and height. */
	void LayoutAutogenBlock(const AutogenAsset &asset, const std::vector<Point2> &quad, int blockCode, float height,
							const AutogenLayoutOptions &options, AutogenLayout &out);

	/* @brief Turn of a corner walking a counter-clockwise contour, X-Plane's sign (left turns negative), degrees. */
	[[nodiscard]] double GetAutogenTurn(const Point2 &incoming, const Point2 &outgoing);
}

// -------------------------------------------------------

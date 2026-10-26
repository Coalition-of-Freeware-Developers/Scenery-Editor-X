/**
 * -------------------------------------------------------
 * Scenery Editor X - Tests
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * AutogenTests.cpp
 * -------------------------------------------------------
 * X-Plane autogen (xp_autogen.*): .agp / .ags / .agb parsing, their layout over DSF placements (tile orientation,
 * string sides and corners, stop points, block grids and slop, object levels) and the overlay builder's use of it.
 * -------------------------------------------------------
 */
#include <catch2/catch_all.hpp>
#include <SceneryEditorX/filesystem/vfs.h>
#include <SceneryEditorX/xplane/xp_autogen.h>
#include <SceneryEditorX/xplane/xp_overlay.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

using namespace SceneryEditorX::XPlane;
namespace fs = std::filesystem;
using Catch::Approx;

namespace
{
	// 100 x 100 px tiles at 1 m a pixel; the house sits near the texture's bottom (the street side unrotated)
	const char *AGP_TEXT =
		"A\n1000\nAG_POINT\n\n"
		"TEXTURE ground.png\n"
		"TEXTURE_SCALE 100 100\n"
		"TEXTURE_WIDTH 100\n"
		"OBJECT house.obj\n"
		"TILE 0 0 100 100\n"
		"ANCHOR_PT 50 50\n"
		"OBJ_DRAPED 50 10 0 0\n"
		"OBJ_GRADED 20 20 90 0 2 4\n"
		"TREE 80 80 0 6 1\n"
		"GROUND_PT 50 50\n";

	// A string with 20 m lots: the street (and anchor) at the bottom, a house facing it, stop points inside
	const char *AGS_TEXT =
		"A\n1000\nAG_STRING\n\n"
		"TEXTURE lots.png\n"
		"TEXTURE_SCALE 200 100\n"
		"TEXTURE_WIDTH 200\n"
		"VEGETATION trees.for\n"
		"FILL_LAYER 0\n"
		"CREASE_ANGLE -30 30\n"
		"OBJECT house.obj\n"
		"FACADE fence.fac\n"
		"TILE_ID 7\n"
		"TILE 0 0 20 40\n"
		"ROTATION 0\n"
		"ANCHOR_PT 10 1\n"
		"OBJ_DRAPED 10 10 180 0\n"
		"FAC 0 1.0 1 39 19 39\n"
		"STOP_PT 10 5\n"
		"STOP_PT 10 35\n"
		"TILE_ID 8\n"
		"TILE 20 0 30 40\n"
		"ANCHOR_PT 25 1\n"
		"STOP_PT 25 5\n"
		"TILE_ID 9\n"
		"TILE 30 0 70 40\n"
		"ANCHOR_PT 50 1\n"
		"OBJ_DRAPED 50 10 180 0\n"
		"STOP_PT 50 5\n"
		"CORNER -100 -80 9\n"
		"ATOMIC_SPELLING 0 1000 0 7 7 7\n"
		"PRIORITY_MARKER\n"
		"SPELLING 7\n"
		"CAPS 8\n";

	// A block: 3 columns (edge, 20 m repeating, edge), 3 rows; the middle column is slop on the right half
	const char *AGB_TEXT =
		"A\n1000\nAG_BLOCK\n\n"
		"TEXTURE block.png\n"
		"TEXTURE_SCALE 100 100\n"
		"TEXTURE_WIDTH 100\n"
		"CROP 1\n"
		"OVERLAP_H 2\n"
		"OBJECT shop.obj\n"
		"SUB_GRID\n"
		"CUT_H 0\n"
		"CUT_H 10\n"
		"CUT_H 30\n"
		"CUT_H 40\n"
		"SLOP_H 4\n"
		"CUT_H 50\n"
		"CUT_V 0\n"
		"CUT_V 10\n"
		"CUT_V 40\n"
		"CUT_V 50\n"
		"END_CUTS\n"
		"EDGE_BEST 2 2 48 48\n"
		"OBJ_DRAPED 20 20 0 0\n"
		"OBJ_GRADED 5 5 0 0\n"
		"GROUND_PT_FORMULA 0.5 0 0.5 0\n"
		"SPELLING_SET 3\n"
		"USE_GRID 0\n"
		"DEPTH_RANGE 0 1000\n"
		"SPELLING_S 0 1 3\n"
		"SPELLING_S 0 1 1 3\n"
		"SPELLING_S 0 1 1 1 2 3\n"
		"DEFINE_SPELLING_T rows 0 1 2\n"
		"USE_SPELLING_T rows\n";

	const char *FOR_TEXT =
		"A\n800\nFOREST\n\n"
		"TEXTURE trees.png\n"
		"SCALE_X 1024\nSCALE_Y 1024\n"
		"SPACING 20 20\nRANDOM 4 4\n"
		"TREE 0 0 256 512 128 50 8 12 2 0\n"
		"TREE 256 0 256 512 128 50 8 12 2 1\n";

	const char *FAC_TEXT =
		"A\n800\nFACADE\n\n"
		"TEXTURE fence.png\n"
		"RING 0\n"
		"WALL 0 1000 0 0\n"
		"SCALE 10 10\n"
		"LEFT 0 0.5\n"
		"CENTER 0.5 1\n"
		"RIGHT 0.5 1\n"
		"BOTTOM 0 0.5\n"
		"TOP 0.5 1\n";

	/* @brief A counter-clockwise rectangle (x east, z south) as a closed polyline. */
	std::vector<Point2> Rectangle(const double x0, const double z0, const double x1, const double z1)
	{
		// Counter-clockwise seen from above: south-west, south-east, north-east, north-west (z grows south)
		return { { x0, z1 }, { x1, z1 }, { x1, z0 }, { x0, z0 }, { x0, z1 } };
	}

	bool Inside(const std::vector<Point2> &ring, const Point2 &p)
	{
		bool inside = false;
		for (size_t i = 0, j = ring.size() - 1; i < ring.size(); j = i++)
		{
			if ((ring[i].z > p.z) != (ring[j].z > p.z) && p.x < (ring[j].x - ring[i].x) * (p.z - ring[i].z) / (ring[j].z - ring[i].z) + ring[i].x)
				inside = !inside;
		}
		return inside;
	}

	struct TempDir
	{
		fs::path path;
		TempDir()
		{
			path = fs::temp_directory_path() / ("sedx_autogen_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
			fs::create_directories(path);
		}
		~TempDir()
		{
			std::error_code ec;
			fs::remove_all(path, ec);
		}
		fs::path Write(const std::string &name, const std::string &text) const
		{
			const fs::path file = path / name;
			fs::create_directories(file.parent_path());
			std::ofstream(file, std::ios::binary) << text;
			return file;
		}
	};

	fs::path FindXPlane()
	{
		for (const fs::path &root : SceneryEditorX::VFS::FindXPlaneInstalls(12))
		{
			if (fs::exists(root / "Resources" / "default scenery"))
				return root;
		}
		return {};
	}
}

// -------------------------------------------------------

TEST_CASE("Autogen files parse", "[autogen][assets]")
{
	SECTION("Points")
	{
		AutogenAsset asset;
		std::string error;
		REQUIRE(ParseAutogenAsset(AGP_TEXT, asset, &error));
		CHECK(asset.type == AutogenType::Point);
		CHECK(asset.texture == "ground.png");
		CHECK(asset.textureWidth == 100.0f);
		REQUIRE(asset.tiles.size() == 1);
		const AutogenTile &tile = asset.tiles[0];
		CHECK(tile.metersPerPixelS == Approx(1.0f));
		CHECK(tile.metersPerPixelT == Approx(1.0f));
		CHECK(tile.hasAnchor);
		CHECK(tile.hasGround);
		REQUIRE(tile.annotations.objects.size() == 2);
		CHECK(tile.annotations.objects[0].mode == AutogenAnnotations::Object::Mode::Draped);
		CHECK(tile.annotations.objects[1].mode == AutogenAnnotations::Object::Mode::Graded);
		CHECK(tile.annotations.objects[1].lo == 2);
		CHECK(tile.annotations.objects[1].hi == 4);
		REQUIRE(tile.annotations.trees.size() == 1);
		CHECK(tile.annotations.trees[0].width == 6.0f);
		CHECK(tile.annotations.trees[0].layer == 1);
	}
	SECTION("Strings")
	{
		AutogenAsset asset;
		REQUIRE(ParseAutogenAsset(AGS_TEXT, asset));
		CHECK(asset.type == AutogenType::String);
		CHECK(asset.fillLayer == 0);
		CHECK(asset.vegetation == "trees.for");
		CHECK(asset.creaseLeft == -30.0f);
		REQUIRE(asset.tiles.size() == 3);
		CHECK(asset.tiles[0].id == 7);
		CHECK(asset.FindTile(9) == &asset.tiles[2]);
		CHECK(asset.tiles[0].stops.size() == 4);
		REQUIRE(asset.tiles[0].annotations.facades.size() == 1);
		CHECK(asset.tiles[0].annotations.facades[0].points.size() == 4);
		REQUIRE(asset.spellingSets.size() == 1);
		const AutogenSpellingSet &set = asset.spellingSets[0];
		REQUIRE(set.corners.size() == 1);
		CHECK(set.corners[0].angleMin == -100.0f);
		CHECK(set.corners[0].tiles == std::vector<int>{ 9 });
		REQUIRE(set.spellings.size() == 2);
		CHECK(set.spellings[0].atomic);
		CHECK(set.spellings[0].priority == 0);
		CHECK(set.spellings[1].priority == 1);
		CHECK(set.caps == std::vector<int>{ 8 });
	}
	SECTION("Blocks")
	{
		AutogenAsset asset;
		REQUIRE(ParseAutogenAsset(AGB_TEXT, asset));
		CHECK(asset.type == AutogenType::Block);
		CHECK(asset.crop);
		REQUIRE(asset.grids.size() == 1);
		const AutogenGrid &grid = asset.grids[0];
		CHECK(grid.cutsH.size() == 5);
		CHECK(grid.cutsV.size() == 4);
		REQUIRE(grid.slopH.size() == 4);
		CHECK(grid.slopH[3] == 4.0f);
		CHECK(grid.slopH[1] < 0.0f);
		CHECK(grid.edgeBest[2] == 48.0f);
		REQUIRE(grid.annotations.objects.size() == 2);
		CHECK(grid.annotations.objects[1].hasGroundFormula);
		REQUIRE(asset.spellingSets.size() == 1);
		CHECK(asset.spellingSets[0].param == 3);
		CHECK(asset.spellingSets[0].spellingsS.size() == 3);
		REQUIRE(asset.spellingSets[0].spellingsT.size() == 1);
		CHECK(asset.spellingSets[0].spellingsT[0] == std::vector<int>{ 0, 1, 2 });
	}
	SECTION("Not autogen")
	{
		AutogenAsset asset;
		std::string error;
		CHECK_FALSE(ParseAutogenAsset("A\n800\nFOREST\n\nTEXTURE a.png\n", asset, &error));
		CHECK_FALSE(ParseAutogenAsset("A\n1000\nAG_POINT\n\nTEXTURE a.png\n", asset, &error));	// No TILE
	}
}

TEST_CASE("Autogen points face their heading", "[autogen]")
{
	AutogenAsset asset;
	REQUIRE(ParseAutogenAsset(AGP_TEXT, asset));
	AutogenLayoutOptions options;
	options.objectLevel = 6;

	SECTION("Unrotated: north up at heading 0")
	{
		AutogenLayout layout;
		LayoutAutogenPoint(asset, { 0.0, 0.0 }, 0.0, options, layout);
		REQUIRE(layout.objects.size() == 2);
		// (50, 10) is 40 px below the anchor: 40 m south; the texture's up points north
		CHECK(layout.objects[0].at.x == Approx(0.0).margin(1e-6));
		CHECK(layout.objects[0].at.z == Approx(40.0));
		CHECK(layout.objects[0].heading == Approx(0.0).margin(1e-9));
		// s runs east: the tree at (80, 80) stands 30 m east, 30 m north
		REQUIRE(layout.trees.size() == 1);
		CHECK(layout.trees[0].at.x == Approx(30.0));
		CHECK(layout.trees[0].at.z == Approx(-30.0));
		REQUIRE(layout.ground.size() == 1);
		CHECK(layout.ground[0].ring.size() == 4);
		CHECK(SignedArea(layout.ground[0].ring) > 0.0);
		// The graded object grades on the tile's ground point
		CHECK(layout.objects[1].graded);
		CHECK(layout.objects[1].ground.x == Approx(0.0).margin(1e-6));
		CHECK(layout.objects[1].ground.z == Approx(0.0).margin(1e-6));
	}
	SECTION("Reach: the farthest tile corner from the anchor")
	{
		CHECK(GetAutogenReach(asset) == Approx(std::hypot(50.0, 50.0)));
		AutogenAsset offCenter = asset;
		offCenter.tiles[0].anchor[0] = 10.0f;	// 90 px to the far side, 50 up or down
		CHECK(GetAutogenReach(offCenter) == Approx(std::hypot(90.0, 50.0)));
	}
	SECTION("ROTATION 2 at heading 90: the texture's up to the west")
	{
		AutogenAsset rotated = asset;
		rotated.tiles[0].rotation = 2;
		AutogenLayout layout;
		LayoutAutogenPoint(rotated, { 100.0, 100.0 }, 90.0, options, layout);
		REQUIRE(layout.objects.size() == 2);
		CHECK(layout.objects[0].at.x == Approx(140.0));	// 40 m east: the texture's bottom faces the heading
		CHECK(layout.objects[0].at.z == Approx(100.0));
		CHECK(layout.objects[0].heading == Approx(270.0));
		CHECK(layout.objects[1].heading == Approx(0.0).margin(1e-9));	// 90 clockwise of the tile's up
		REQUIRE(layout.trees.size() == 1);
		CHECK(layout.trees[0].width == 6.0f);
	}
	SECTION("Object detail levels")
	{
		// lo 2 hi 4: never below 2, always at 4 and above, sometimes between
		AutogenLayoutOptions low = options;
		low.objectLevel = 1;
		AutogenLayout none;
		LayoutAutogenPoint(asset, { 0.0, 0.0 }, 0.0, low, none);
		CHECK(none.objects.size() == 1);
		size_t seen = 0;
		for (uint32_t seed = 0; seed < 200; ++seed)
		{
			AutogenLayoutOptions mid = options;
			mid.objectLevel = 3;
			mid.seed = seed;
			AutogenLayout layout;
			LayoutAutogenPoint(asset, { 0.0, 0.0 }, 0.0, mid, layout);
			seen += layout.objects.size() - 1;
		}
		CHECK(seen > 80);	// About 2 in 3
		CHECK(seen < 190);
		AutogenLayoutOptions off = options;
		off.objectLevel = 0;
		AutogenLayout nothing;
		LayoutAutogenPoint(asset, { 0.0, 0.0 }, 0.0, off, nothing);
		CHECK(nothing.objects.empty());
	}
}

TEST_CASE("Autogen strings line their sides", "[autogen]")
{
	AutogenAsset asset;
	REQUIRE(ParseAutogenAsset(AGS_TEXT, asset));
	AutogenLayoutOptions options;
	options.objectLevel = 6;
	const std::vector<Point2> block = Rectangle(0.0, 0.0, 200.0, 100.0);

	SECTION("Tiles stand inside, houses face the street")
	{
		AutogenAsset sidesOnly = asset;
		sidesOnly.spellingSets[0].corners.clear();
		AutogenLayout layout;
		LayoutAutogenString(sidesOnly, { block }, 1, 4.0f, options, layout);
		CHECK(layout.tiles > 20);
		// Every house is inside the block, 9 m in from a side, facing out to it
		REQUIRE(layout.objects.size() > 10);
		for (const AutogenLayout::Object &object : layout.objects)
		{
			CHECK(Inside(block, object.at));
			const double toSide = std::min({ object.at.x, 200.0 - object.at.x, object.at.z, 100.0 - object.at.z });
			CHECK(toSide == Approx(9.0).margin(0.01));
			const Point2 facing = HeadingToDirection(object.heading);
			const Point2 out{ object.at.x < 9.5 ? -1.0 : object.at.x > 190.5 ? 1.0 : 0.0, object.at.z < 9.5 ? -1.0 : object.at.z > 90.5 ? 1.0 : 0.0 };
			CHECK(facing.x * out.x + facing.z * out.z > 0.99);
		}
		// Ground tiles overhang the street by the anchor's 1 m at most
		for (const AutogenLayout::Ground &ground : layout.ground)
		{
			CHECK(SignedArea(ground.ring) > 0.0);
			for (const Point2 &p : ground.ring)
			{
				CHECK(p.x >= -1.01);
				CHECK(p.x <= 201.01);
				CHECK(p.z >= -1.01);
				CHECK(p.z <= 101.01);
			}
		}
		// The fences, and the rest filled with trees
		CHECK(layout.facades.size() > 10);
		REQUIRE(layout.fills.size() == 1);
		CHECK(layout.fills[0].footprints.size() == layout.tiles);
	}
	SECTION("Corners at the turns the rules allow")
	{
		AutogenLayout layout;
		LayoutAutogenString(asset, { block }, 1, 4.0f, options, layout);
		// The four -90 degree corners get tile 9 on the bisector: its house sits diagonally in from each corner
		const Point2 corners[4] = { { 0.0, 100.0 }, { 200.0, 100.0 }, { 200.0, 0.0 }, { 0.0, 0.0 } };
		for (const Point2 &corner : corners)
		{
			const auto near = std::ranges::find_if(layout.objects, [&corner](const AutogenLayout::Object &o)
			{
				return std::abs(std::abs(o.at.x - corner.x) - std::abs(o.at.z - corner.z)) < 0.01 && std::hypot(o.at.x - corner.x, o.at.z - corner.z) < 15.0;
			});
			CHECK(near != layout.objects.end());
		}
		CHECK(GetAutogenTurn({ 1.0, 0.0 }, { 0.0, -1.0 }) == Approx(-90.0));	// East then north: a left turn
		CHECK(GetAutogenTurn({ 1.0, 0.0 }, { 0.0, 1.0 }) == Approx(90.0));
	}
	SECTION("Contours: only the spawning ones carry tiles; holes stay clear")
	{
		// The north side has no houses: a 3-side polyline spawns, the north side only closes the area
		const std::vector<Point2> threeSides = { { 0.0, 0.0 }, { 0.0, 100.0 }, { 200.0, 100.0 }, { 200.0, 0.0 } };
		const std::vector<Point2> north = { { 200.0, 0.0 }, { 0.0, 0.0 } };
		AutogenLayout layout;
		LayoutAutogenString(asset, { threeSides, north }, 1, 4.0f, options, layout);
		REQUIRE_FALSE(layout.objects.empty());
		for (const AutogenLayout::Object &object : layout.objects)
			CHECK_FALSE((object.at.z < 15.0 && object.at.x > 25.0 && object.at.x < 175.0));	// Nothing along the north side
		// A lake in the middle: no stop point (and so no house) in it
		const std::vector<Point2> lake = { { 20.0, 20.0 }, { 20.0, 80.0 }, { 180.0, 80.0 }, { 180.0, 20.0 }, { 20.0, 20.0 } };
		AutogenLayout withLake;
		LayoutAutogenString(asset, { block, lake }, 1, 4.0f, options, withLake);
		CHECK(withLake.tiles < layout.tiles + 30);
		for (const AutogenLayout::Object &object : withLake.objects)
			CHECK_FALSE((object.at.x > 20.0 && object.at.x < 180.0 && object.at.z > 20.0 && object.at.z < 80.0));
		// No spawning contours: just the fill
		AutogenLayout fillOnly;
		LayoutAutogenString(asset, { block }, 0, 4.0f, options, fillOnly);
		CHECK(fillOnly.tiles == 0);
		CHECK(fillOnly.fills.size() == 1);
	}
	SECTION("Atomic spellings come whole")
	{
		// 3 x 20 m only fits sides of 60 m and more; on a 50 m side the single-tile backup fills it
		const std::vector<Point2> small = Rectangle(0.0, 0.0, 50.0, 50.0);
		AutogenAsset noCorners = asset;
		noCorners.spellingSets[0].corners.clear();
		AutogenLayout layout;
		LayoutAutogenString(noCorners, { small }, 1, 4.0f, options, layout);
		CHECK(layout.tiles > 0);
		CHECK(layout.tiles <= 12);
	}
}

TEST_CASE("Autogen blocks fill their grid", "[autogen]")
{
	AutogenAsset asset;
	REQUIRE(ParseAutogenAsset(AGB_TEXT, asset));
	AutogenLayoutOptions options;
	options.objectLevel = 6;
	// 46 m along the first side (the EDGE_BEST width of "0 1 3"), 46 m deep
	const std::vector<Point2> quad = { { 0.0, 46.0 }, { 46.0, 46.0 }, { 46.0, 0.0 }, { 0.0, 0.0 } };
	AutogenLayout layout;
	LayoutAutogenBlock(asset, quad, 3, 8.0f, options, layout);
	REQUIRE(layout.tiles > 0);
	// One object per placed copy of column 1 (two OBJ_DRAPED rows in row 1 only: one per copy)
	size_t draped = 0;
	for (const AutogenLayout::Object &object : layout.objects)
	{
		if (!object.graded)
		{
			++draped;
			CHECK(Inside(quad, object.at));
			CHECK(object.heading == Approx(0.0).margin(1e-6));	// The texture's up: across the first side, north here
		}
		else
		{
			// GROUND_PT_FORMULA 0.5 0 0.5 0: the middle of the block
			CHECK(object.ground.x == Approx(23.0));
			CHECK(object.ground.z == Approx(23.0));
		}
	}
	CHECK(draped >= 1);
	// Cropped ground stays in the block
	for (const AutogenLayout::Ground &ground : layout.ground)
	{
		for (const Point2 &p : ground.ring)
		{
			CHECK(p.x >= -1e-6);
			CHECK(p.x <= 46.0 + 1e-6);
			CHECK(p.z >= -1e-6);
			CHECK(p.z <= 46.0 + 1e-6);
		}
	}

	// A wider block takes more copies of the repeating column
	AutogenLayout wide;
	const std::vector<Point2> wideQuad = { { 0.0, 46.0 }, { 86.0, 46.0 }, { 86.0, 0.0 }, { 0.0, 0.0 } };
	LayoutAutogenBlock(asset, wideQuad, 3, 8.0f, options, wide);
	CHECK(wide.tiles > layout.tiles);
}

TEST_CASE("The overlay builder places autogen", "[autogen][overlay]")
{
	TempDir temp;
	temp.Write("ag/lots.ags", AGS_TEXT);
	temp.Write("ag/house.obj", "I\n800\nOBJ\n\nPOINT_COUNTS 0 0 0 0\n");
	temp.Write("ag/trees.for", FOR_TEXT);
	temp.Write("ag/fence.fac", FAC_TEXT);
	const fs::path package = temp.path;
	const AssetResolver resolver = [&package](const std::string_view path) -> fs::path
	{
		const fs::path candidate{ std::string(path) };
		if (candidate.is_absolute())
			return fs::exists(candidate) ? candidate : fs::path();
		const fs::path local = package / candidate;
		return fs::exists(local) ? local : fs::path();
	};
	const GeoProjection projection(45.6, -122.6);
	OverlayBuilder builder(resolver, projection);

	DsfFile dsf;
	dsf.polygonDefinitions = { "ag/lots.ags" };
	DsfPolygonPlacement polygon;
	polygon.parameter = 1 + 256 * 2;	// One spawning contour, 8 m tall
	std::vector<DsfPoint> winding;
	for (const Point2 &corner : Rectangle(0.0, 0.0, 200.0, 100.0))
	{
		DsfPoint p;
		p.planes = 2;
		projection.ToGeo(corner, p.v[1], p.v[0]);
		winding.push_back(p);
	}
	polygon.windings = { winding };
	dsf.polygons.push_back(polygon);

	OverlayBuilder::Options options;
	options.autogenObjectLevel = 6;
	OverlayBuild out;
	builder.Build(dsf, options, out);
	CHECK(out.warnings.empty());
	for (const std::string &warning : out.warnings)
		UNSCOPED_INFO(warning);
	std::map<OverlayKind, size_t> kinds;
	for (const OverlayMesh &mesh : out.meshes)
		++kinds[mesh.kind];
	CHECK(kinds[OverlayKind::Autogen] == 1);	// The ground tiles
	CHECK(kinds[OverlayKind::Forest] == 1);		// The fill's trees
	CHECK(kinds[OverlayKind::Facade] >= 8);		// The fences
	REQUIRE_FALSE(out.objects.empty());
	for (const OverlayObject &object : out.objects)
	{
		CHECK(object.source == OverlayObject::Source::Autogen);
		CHECK(object.name == "house.obj");
	}

	// Switched off: nothing
	OverlayBuilder::Options off;
	off.drawAutogen = false;
	OverlayBuild none;
	builder.Build(dsf, off, none);
	CHECK(none.meshes.empty());
	CHECK(none.objects.empty());
}

TEST_CASE("Every X-Plane 12 default autogen parses and lays out", "[autogen][xplane-install][library-sweep]")
{
	const fs::path xplane = FindXPlane();
	if (xplane.empty())
	{
		WARN("No X-Plane 12 install found (x-plane_install_12.txt): skipped");
		return;
	}
	size_t points = 0, strings = 0, blocks = 0, stringTiles = 0, blockTiles = 0;
	AutogenLayoutOptions options;
	for (const auto &entry : fs::recursive_directory_iterator(xplane / "Resources" / "default scenery"))
	{
		const std::string extension = entry.path().extension().string();
		if (extension != ".agp" && extension != ".ags" && extension != ".agb")
			continue;
		AutogenAsset asset;
		std::string error;
		INFO(entry.path().string());
		REQUIRE(LoadAutogenAsset(entry.path(), asset, &error));
		AutogenLayout layout;
		switch (asset.type)
		{
			case AutogenType::Point:
				++points;
				LayoutAutogenPoint(asset, { 0.0, 0.0 }, 30.0, options, layout);
				CHECK(layout.tiles == 1);
				break;
			case AutogenType::String:
				++strings;
				LayoutAutogenString(asset, { Rectangle(0.0, 0.0, 300.0, 200.0) }, 1, 12.0f, options, layout);
				stringTiles += layout.tiles;
				break;
			case AutogenType::Block:
				++blocks;
				LayoutAutogenBlock(asset, { { 0.0, 120.0 }, { 160.0, 120.0 }, { 160.0, 0.0 }, { 0.0, 0.0 } }, 7, 12.0f, options, layout);
				blockTiles += layout.tiles;
				break;
		}
	}
	CHECK(points > 10);
	CHECK(strings > 30);
	CHECK(blocks > 10);
	CHECK(stringTiles > strings * 10);
	CHECK(blockTiles > blocks * 10);
}

// -------------------------------------------------------

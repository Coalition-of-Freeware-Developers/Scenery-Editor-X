/**
 * -------------------------------------------------------
 * Scenery Editor X - Tests
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * FacadeTests.cpp
 * -------------------------------------------------------
 * Facades: the newer .fac directives (WALL_RULE, SEGMENT_CURVED, roofs and roof objects, scrapers), the layout X-Plane
 * builds a facade with (floor, wall and spelling choice, after WorldEditor's preview), natural wall lengths, and the
 * overlay builder drawing curved walls and wall rules.
 * -------------------------------------------------------
 */
#include <catch2/catch_all.hpp>
#include <SceneryEditorX/xplane/xp_assets.h>
#include <SceneryEditorX/xplane/xp_dsf.h>
#include <SceneryEditorX/xplane/xp_facade.h>
#include <SceneryEditorX/xplane/xp_overlay.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace SceneryEditorX::XPlane;
namespace fs = std::filesystem;

namespace
{
	/* @brief A segment mesh: a quad `width` meters along -z, `height` up, its texture s from s0 to s1. */
	std::string Quad(const double width, const double height, const double s0, const double s1, const int cuts = 1)
	{
		const auto f = [](const double v) { return std::to_string(v); };
		return "MESH 0 100 " + std::to_string(cuts) + " 4 6\n"
			"VERTEX 0 0 0 0 0 1 " + f(s0) + " 0\n"
			"VERTEX 0 0 " + f(-width) + " 0 0 1 " + f(s1) + " 0\n"
			"VERTEX 0 " + f(height) + " " + f(-width) + " 0 0 1 " + f(s1) + " 1\n"
			"VERTEX 0 " + f(height) + " 0 0 0 1 " + f(s0) + " 1\n"
			"IDX 0 1 2 0 2 3\n";
	}

	/**
	 * Two floors: "low" (0.15 m, a sidewalk-like slab with three wall types and curved segment variants) and "high"
	 * (roofs at 6 and 12 m). Segment 0 is 1 m wide (texture s 0 - 0.5), segment 1 is 2 m (s 0.5 - 1).
	 */
	std::string RulesFacade()
	{
		return std::string("A\n1000\nFACADE\n\n"
			"SHADER_WALL\nTEXTURE wall.png\n"
			"RING 1\n"
			"LAYER_GROUP objects 2\n"
			"FACADE_SCRAPER 100 300 4 2\n"
			"FACADE_SCRAPER_MODEL base.obj - 0 0\n"
			"FACADE_SCRAPER_PAD 0 10 0 0 pad.obj 2 6\n"
			"OBJ grate.obj\n"
			"ROOF_SCALE 8 8\n"
			"FLOOR low\n"
			"ROOF_HEIGHT 0.15\n"
			"TWO_SIDED_ROOF\n"
			"ROOF_OBJ_HEADING 0 10 5 90 2 6\n"
			"HARD_ROOF concrete\n"
			"SEGMENT 0\n") + Quad(1.0, 0.15, 0.0, 0.5) +
			"ATTACH_GRADED 0 0 0 -0.5 0 3 5\n"
			"SEGMENT 1\n" + Quad(2.0, 0.15, 0.5, 1.0) +
			"SEGMENT_CURVED 0\n" + Quad(1.0, 0.15, 0.0, 0.5, 4) +
			"SEGMENT_CURVED 1\n" + Quad(2.0, 0.15, 0.5, 1.0, 4) +
			"WALL 0 10 270 90 front_short\n"
			"SPELLING 0\n"
			"SPELLING 0 0\n"
			"WALL 10 999 0 0 long\n"
			"WALL_RULE 0 999 90 180\n"
			"SPELLING 1\n"
			"SPELLING 1 1\n"
			"WALL 0 999 0 0\n"
			"SPELLING 0 1\n"
			"FLOOR high\n"
			"ROOF_HEIGHT 6 12\n"
			"SEGMENT 0\n" + Quad(4.0, 12.0, 0.0, 1.0) +
			"WALL 0 999 0 0 tall\n"
			"SPELLING 0\n";
	}

	/* @brief One wall type, spellings 4 m (0), 6 m (0 1) and 8 m (0 0): segment 0 is 4 m, segment 1 is 2 m. */
	const char *SPELLING_FACADE =
		"A\n1000\nFACADE\n\n"
		"TEXTURE wall.png\n"
		"FLOOR f\n"
		"ROOF_HEIGHT 3\n"
		"SEGMENT 0\n"
		"MESH 0 100 1 2 0\n"
		"VERTEX 0 0 0 1 0 0 0 0\n"
		"VERTEX 0 3 -4 1 0 0 1 1\n"
		"SEGMENT 1\n"
		"MESH 0 100 1 2 0\n"
		"VERTEX 0 0 0 1 0 0 0 0\n"
		"VERTEX 0 3 -2 1 0 0 1 1\n"
		"WALL 0 999 0 0 wall\n"
		"SPELLING 0\n"
		"SPELLING 0 1\n"
		"SPELLING 0 0\n";

	FacadeAsset Parse(const std::string &text)
	{
		FacadeAsset asset;
		std::string error;
		REQUIRE(ParseFacadeAsset(text, asset, &error));
		return asset;
	}

	struct TempDir
	{
		fs::path path;
		TempDir()
		{
			path = fs::temp_directory_path() / ("sedx_facade_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
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
}

// -------------------------------------------------------

TEST_CASE("Facade parser reads the newer directives", "[assets][facades]")
{
	const FacadeAsset fac = Parse(RulesFacade());
	CHECK(fac.layerGroup == "objects");
	CHECK(fac.layerOffset == 2);
	REQUIRE(fac.scrapers.size() == 1);
	CHECK(fac.scrapers[0].minAgl == Catch::Approx(100.0f));
	CHECK(fac.scrapers[0].floors == 2);
	CHECK(fac.scrapers[0].models == 1);
	CHECK(fac.scrapers[0].pads == 1);

	REQUIRE(fac.floors.size() == 2);
	const FacadeAsset::Floor &low = fac.floors[0];
	CHECK(low.height == Catch::Approx(0.15f));
	CHECK(low.hardRoof == "concrete");
	REQUIRE(low.roofs.size() == 1);
	CHECK(low.roofs[0].twoSided);
	REQUIRE(low.roofs[0].objects.size() == 1);
	CHECK(low.roofs[0].objects[0].heading == Catch::Approx(90.0f));
	CHECK(low.roofs[0].objects[0].showLo == 2);
	CHECK(low.roofs[0].objects[0].showHi == 6);

	// SEGMENT_CURVED keeps its own meshes (with their cuts) and widths, beside the straight ones
	REQUIRE(low.segments.size() == 2);
	REQUIRE(low.curvedSegments.size() == 2);
	CHECK(low.curvedSegments[1].width == Catch::Approx(2.0f));
	CHECK(low.curvedSegments[0].meshes[0].cuts == 4);
	CHECK(low.segments[0].meshes[0].cuts == 1);
	REQUIRE(low.segments[0].attachments.size() == 1);
	CHECK_FALSE(low.segments[0].attachments[0].draped);
	CHECK(low.segments[0].attachments[0].showLo == 3);
	CHECK(low.segments[0].attachments[0].showHi == 5);

	// WALL keeps its own filter, WALL_RULE adds alternatives
	REQUIRE(low.walls.size() == 3);
	REQUIRE(low.walls[0].filters.size() == 1);
	CHECK(low.walls[0].filters[0].minHeading == Catch::Approx(270.0f));
	REQUIRE(low.walls[1].filters.size() == 2);
	CHECK(low.walls[1].filters[1].maxHeading == Catch::Approx(180.0f));

	// A floor's height is its top roof
	CHECK(fac.floors[1].height == Catch::Approx(12.0f));
	CHECK(fac.floors[1].roofs.size() == 2);
}

TEST_CASE("Facade wall filters accept widths and headings, wrapping past north", "[facades]")
{
	FacadeAsset::WallFilter any;
	CHECK(any.Accepts(5.0, 123.0));
	FacadeAsset::WallFilter front{ 0.0f, 10.0f, 270.0f, 90.0f };
	CHECK(front.Accepts(5.0, 0.0));
	CHECK(front.Accepts(5.0, 300.0));
	CHECK(front.Accepts(5.0, -30.0));
	CHECK_FALSE(front.Accepts(5.0, 120.0));
	CHECK_FALSE(front.Accepts(12.0, 0.0));
	FacadeAsset::WallFilter back{ 0.0f, 999.0f, 90.0f, 180.0f };
	CHECK(back.Accepts(5.0, 90.0));
	CHECK_FALSE(back.Accepts(5.0, 180.0));	// The range is half open
}

TEST_CASE("Facade wall names, families and heights", "[facades]")
{
	const FacadeAsset fac = Parse(RulesFacade());
	CHECK(GetFacadeWallNames(fac) == std::vector<std::string>{ "front_short", "long", "Wall 3" });
	CHECK(GetFacadeWallCount(fac) == 3);
	CHECK(DescribeFacadeWall(fac, 0) == "up to 10 m, facing 270 - 90 deg from the front");
	CHECK(DescribeFacadeWall(fac, 1) == "10 m and wider or any width, facing 90 - 180 deg from the front");
	CHECK(DescribeFacadeWall(fac, 2) == "any width");

	const FacadeHeights heights = GetFacadeHeights(fac);
	CHECK(heights.choices == std::vector<float>{ 0.15f, 12.0f });
	CHECK_FALSE(heights.fixed);
	CHECK(DescribeFacadeHeights(heights) == "0.15 - 12 m, 2 heights");
	CHECK(GetFacadeFamily(fac) == FacadeFamily::Building);

	// A floor is chosen by its top roof; type 2 heights snap to it
	CHECK(ChooseFacadeFloor(fac, 5.0) == 0);
	CHECK(ChooseFacadeFloor(fac, 8.0) == 1);
	CHECK(SnapFacadeHeight(fac, 9.0) == Catch::Approx(12.0));
	CHECK(GetFacadeRoofHeight(fac.floors[1], 7.0) == Catch::Approx(6.0));

	// Sidewalk-like: one low floor is a ground facade; an open one a line
	FacadeAsset sidewalk = fac;
	sidewalk.floors.pop_back();
	CHECK(GetFacadeFamily(sidewalk) == FacadeFamily::Ground);
	CHECK(GetFacadeHeights(sidewalk).fixed);
	CHECK(DescribeFacadeHeights(GetFacadeHeights(sidewalk)) == "0.15 m (fixed)");
	sidewalk.ring = false;
	CHECK(GetFacadeFamily(sidewalk) == FacadeFamily::Line);

	// Type 1: floors of the middle rows (3 m here), bottom and top once
	const FacadeAsset type1 = Parse("A\n800\nFACADE\n\nTEXTURE w.png\nRING 1\nROOF 0 0\nROOF 1 0\nROOF 1 1\nROOF 0 1\n"
		"WALL 0 100\nSCALE 10 12\nLEFT 0 0.5\nRIGHT 0.5 1\nBOTTOM 0 0.25\nMIDDLE 0.25 0.5\nTOP 0.5 1\nFLOORS_MAX 5\n");
	const FacadeHeights floors = GetFacadeHeights(type1);
	CHECK(floors.floorHeight == Catch::Approx(3.0f));
	CHECK(floors.minimum == Catch::Approx(9.0f));		// bottom 3 + top 6
	CHECK(floors.maximum == Catch::Approx(18.0f));		// 5 floors: 2 fixed + 3 middle
	CHECK(DescribeFacadeHeights(floors) == "3 m floors, 9 - 18 m");
	CHECK(GetFacadeWallNames(type1) == std::vector<std::string>{ "Wall 1" });
}

TEST_CASE("Facade wall choice: picked walls, else the first whose rules accept the side", "[facades]")
{
	const FacadeAsset fac = Parse(RulesFacade());
	bool fits = false;
	CHECK(ChooseFacadeWall(fac, 0, 5.0, 0.0, 0, false, &fits) == 0);
	CHECK(fits);
	CHECK(ChooseFacadeWall(fac, 0, 20.0, 0.0, 0, false) == 1);		// Too long for the short one
	CHECK(ChooseFacadeWall(fac, 0, 5.0, 120.0, 0, false) == 1);	// Its WALL_RULE takes the side walls
	CHECK(ChooseFacadeWall(fac, 0, 5.0, 200.0, 0, false) == 2);	// Only the unnamed one is left
	CHECK(ChooseFacadeWall(fac, 0, 20.0, 0.0, 0, true, &fits) == 0);	// Picked walls win, even when they don't fit
	CHECK_FALSE(fits);
	CHECK(ChooseFacadeWall(fac, 0, 5.0, 0.0, 9, true) == 2);		// Clamped to the facade's walls
	CHECK(ChooseFacadeWall(fac, 1, 5.0, 0.0, 0, false) == 0);

	// Headings are relative to the front (side 0): a square's back wall faces 180 from it
	const std::vector<FacadeSideInput> square = { { 5.0, 180.0, 0 }, { 5.0, 90.0, 0 }, { 5.0, 0.0, 0 }, { 5.0, 270.0, 0 } };
	const FacadeLayout layout = LayoutFacade(fac, square, false, 0.15);
	CHECK(layout.floor == 0);
	CHECK(layout.roofHeight == Catch::Approx(0.15));
	REQUIRE(layout.walls.size() == 4);
	CHECK(layout.walls[0].wall == 0);	// front
	CHECK(layout.walls[1].wall == 0);	// 270 relative: inside the front wall's 270 - 90 range
	CHECK(layout.walls[2].wall == 2);	// 180 relative: neither range
	CHECK(layout.walls[3].wall == 1);	// 90 relative: the side rule
	CHECK(layout.walls[0].chosen);
}

TEST_CASE("Facade spelling choice follows WorldEditor's preview", "[facades]")
{
	const FacadeAsset fac = Parse(SPELLING_FACADE);
	const FacadeAsset::Floor &floor = fac.floors[0];
	const FacadeAsset::WallRule &wall = floor.walls[0];
	double total = 0.0;

	CHECK(PickFacadeSpelling(floor, wall, 8.0, total) == std::vector<int>{ 0, 0 });
	CHECK(total == Catch::Approx(8.0));
	CHECK(PickFacadeSpelling(floor, wall, 6.2, total) == std::vector<int>{ 0, 1 });
	CHECK(total == Catch::Approx(6.0));
	// A tie (7 m: 6 or 8) keeps the longer, the first one met from the long end
	PickFacadeSpelling(floor, wall, 7.0, total);
	CHECK(total == Catch::Approx(8.0));
	// Too short for anything: the shortest
	CHECK(PickFacadeSpelling(floor, wall, 1.0, total) == std::vector<int>{ 0 });
	CHECK(total == Catch::Approx(4.0));
	// Long walls: middling spellings (6 m) until the longest no longer fits, then the closest
	CHECK(PickFacadeSpelling(floor, wall, 20.0, total) == std::vector<int>{ 0, 1, 0, 1, 0, 0 });
	CHECK(total == Catch::Approx(20.0));

	const FacadeLayout layout = LayoutFacade(fac, { { 10.0, 180.0, 0 } }, false, 3.0);
	REQUIRE(layout.walls.size() == 1);
	CHECK(layout.walls[0].total == Catch::Approx(10.0));
	CHECK(layout.walls[0].stretch == Catch::Approx(1.0));

	// Lengths built without stretching: 4, 6, 8, 10, 12, ... and never 5 or 7
	const std::vector<double> natural = GetFacadeNaturalLengths(fac, 0, 0, 30.0);
	REQUIRE(natural.size() >= 4);
	CHECK(natural[0] == Catch::Approx(4.0));
	CHECK(natural[1] == Catch::Approx(6.0));
	CHECK(natural[2] == Catch::Approx(8.0));
	CHECK(std::ranges::none_of(natural, [](const double l) { return std::abs(l - 5.0) < 1e-6 || std::abs(l - 7.0) < 1e-6; }));
	CHECK(std::ranges::is_sorted(natural));
	CHECK(SnapFacadeLength(natural, 6.4) == Catch::Approx(6.0));
	CHECK(SnapFacadeLength({}, 6.4) == Catch::Approx(6.4));
}

TEST_CASE("Facade paths bend walls along curves without tearing", "[facades]")
{
	const FacadePath straight({ { 0.0, 0.0 }, { 10.0, 0.0 } });
	CHECK_FALSE(straight.IsCurved());
	CHECK(straight.GetLength() == Catch::Approx(10.0));
	Point2 position, direction;
	straight.Evaluate(4.0, position, direction);
	CHECK(position.x == Catch::Approx(4.0));
	CHECK(direction.x == Catch::Approx(1.0));
	straight.Evaluate(12.0, position, direction);	// Straight on past the end
	CHECK(position.x == Catch::Approx(12.0));
	straight.Evaluate(-1.0, position, direction);
	CHECK(position.x == Catch::Approx(-1.0));

	// A right angle: at the corner the direction is halfway between the two pieces
	const FacadePath bent({ { 0.0, 0.0 }, { 10.0, 0.0 }, { 10.0, 10.0 } });
	CHECK(bent.IsCurved());
	CHECK(bent.GetLength() == Catch::Approx(20.0));
	bent.Evaluate(10.0, position, direction);
	CHECK(position.x == Catch::Approx(10.0));
	CHECK(direction.x == Catch::Approx(std::sqrt(0.5)));
	CHECK(direction.z == Catch::Approx(std::sqrt(0.5)));
	bent.Evaluate(15.0, position, direction);
	CHECK(position.z == Catch::Approx(5.0));

	// Outward is to the right of travel: heading east, the wall faces south (+z)
	const Point2 outward = GetFacadeOutward({ 1.0, 0.0 });
	CHECK(outward.x == Catch::Approx(0.0).margin(1e-12));
	CHECK(outward.z == Catch::Approx(1.0));
}

TEST_CASE("Overlay builder: wall rules, curved walls and the front-aligned roof", "[overlay][facades]")
{
	TempDir temp;
	temp.Write("facades/rules.fac", RulesFacade());
	temp.Write("facades/grate.obj", "I\n800\nOBJ\n");
	const fs::path package = temp.path;
	const AssetResolver resolver = [&](const std::string_view path) -> fs::path
	{
		const fs::path local = package / fs::path(std::string(path));
		return fs::exists(local) ? local : fs::path();
	};
	const GeoProjection projection(45.6, -122.6);
	OverlayBuilder builder(resolver, projection);

	const auto point = [&](const double east, const double north, const uint8_t planes, const double wall = 0.0) -> DsfPoint
	{
		DsfPoint p;
		p.planes = planes;
		projection.ToGeo(Point2{ east, -north }, p.v[1], p.v[0]);
		p.v[2] = wall;
		return p;
	};

	DsfFile dsf;
	dsf.polygonDefinitions = { "facades/rules.fac" };

	// No wall types: a 4 x 20 m slab, counter-clockwise from its south-west corner. Its front (south, 4 m) gets the
	// short front wall (texture s 0 - 0.5); the long sides take wall 1 (s 0.5 - 1) by width; the back gets wall 2.
	DsfPolygonPlacement plain;
	plain.definition = 0;
	plain.parameter = 0;
	plain.planes = 2;
	plain.windings = { { point(0, 0, 2), point(4, 0, 2), point(4, 20, 2), point(0, 20, 2) } };
	dsf.polygons.push_back(plain);

	// Picked walls with a curved front: 5 planes (lon, lat, wall, control lon / lat). Wall 1 everywhere.
	DsfPolygonPlacement curved;
	curved.definition = 0;
	curved.parameter = 0;
	curved.planes = 5;
	DsfPoint a = point(100, 0, 5, 1.0), b = point(120, 0, 5, 1.0), c = point(120, 20, 5, 1.0), d = point(100, 20, 5, 1.0);
	for (DsfPoint *p : { &a, &b, &c, &d })
	{
		p->v[3] = p->v[0];
		p->v[4] = p->v[1];
	}
	// The front bows south: its first node's control leads out south-east, the next node's mirrored control comes in from the south-west
	const DsfPoint aControl = point(105, -8, 2), bControl = point(125, 8, 2);
	a.v[3] = aControl.v[0];
	a.v[4] = aControl.v[1];
	b.v[3] = bControl.v[0];
	b.v[4] = bControl.v[1];
	curved.windings = { { a, b, c, d } };
	dsf.polygons.push_back(curved);

	OverlayBuild out;
	builder.Build(dsf, OverlayBuilder::Options(), out);
	CHECK(out.warnings.empty());

	const OverlayMesh *plainWalls = nullptr, *curvedWalls = nullptr, *plainRoof = nullptr;
	for (const OverlayMesh &mesh : out.meshes)
	{
		if (mesh.kind != OverlayKind::Facade)
			continue;
		const bool roof = mesh.name.find("roof") != std::string::npos;
		if (mesh.placement == 0)
			(roof ? plainRoof : plainWalls) = &mesh;
		else if (!roof)
			curvedWalls = &mesh;
	}
	REQUIRE(plainWalls);
	REQUIRE(curvedWalls);
	REQUIRE(plainRoof);

	// Plain: which texture half each side's vertices use tells which wall built it
	bool frontUsesShort = false, sideUsesLong = false;
	for (const OverlayVertex &vertex : plainWalls->vertices)
	{
		const double north = -vertex.z;
		if (std::abs(north) < 0.01 && vertex.x > 0.01 && vertex.x < 3.99)
			frontUsesShort |= vertex.u <= 0.5f + 1e-4f;
		if (std::abs(vertex.x - 4.0) < 0.01 && north > 0.01 && north < 19.99)
			sideUsesLong |= vertex.u >= 0.5f - 1e-4f;
	}
	CHECK(frontUsesShort);
	CHECK(sideUsesLong);
	float top = 0.0f;
	for (const OverlayVertex &vertex : plainWalls->vertices)
		top = std::max(top, vertex.y);
	CHECK(top == Catch::Approx(0.15f).margin(1e-4f));

	// Curved: every vertex uses wall 1's half, and the front's vertices bow south of the straight line between its nodes
	double southmost = 0.0;
	for (const OverlayVertex &vertex : curvedWalls->vertices)
	{
		CHECK(vertex.u >= 0.5f - 1e-4f);
		if (vertex.x > 101.0 && vertex.x < 119.0 && vertex.z > 0.0)
			southmost = std::max(southmost, static_cast<double>(vertex.z));
	}
	CHECK(southmost > 2.0);

	// The roof's texture runs along the front: along the south edge, t stays the same
	const OverlayVertex *sw = nullptr, *se = nullptr;
	for (const OverlayVertex &vertex : plainRoof->vertices)
	{
		if (std::abs(vertex.z) < 0.01 && std::abs(vertex.x) < 0.01)
			sw = &vertex;
		if (std::abs(vertex.z) < 0.01 && std::abs(vertex.x - 4.0) < 0.01)
			se = &vertex;
	}
	REQUIRE(sw);
	REQUIRE(se);
	CHECK(sw->v == Catch::Approx(se->v).margin(1e-5));
	CHECK(se->u - sw->u == Catch::Approx(0.5).margin(1e-4));	// 4 m of an 8 m ROOF_SCALE

	// The attached grate (segment 0 of the front) is placed
	CHECK(std::ranges::any_of(out.objects, [](const OverlayObject &object) { return object.name == "grate.obj" && object.placement == 0; }));
}

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
 * FacadeEditTests.cpp
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#include <catch2/catch_all.hpp>
#include <SceneryEditorX/project/facade_edit.h>
#include <algorithm>
#include <cmath>

using namespace SceneryEditorX;
using namespace SceneryEditorX::FacadeEdit;
using Catch::Approx;

// -------------------------------------------------------

namespace
{
	const XPlane::GeoProjection PROJECTION(44.6010278, -123.9396111);

	std::vector<int> WallTypes(const Splines::Chain &chain)
	{
		std::vector<int> types;
		for (const Splines::Node &node : chain.nodes)
			types.push_back(node.attributes.wallType);
		return types;
	}

	/* @brief A 20 m wide footprint whose front (south) wall bows 6 m out: a cubic with handles pulled south. */
	Splines::Chain CurvedFront()
	{
		Splines::Chain chain = MakeChain({ { 0.0, 0.0 }, { 20.0, 0.0 }, { 20.0, -10.0 }, { 0.0, -10.0 } }, true);
		chain.nodes[0].hasOut = true;
		chain.nodes[0].handleOut = { 5.0, 8.0 };
		chain.nodes[0].split = true;
		chain.nodes[1].hasIn = true;
		chain.nodes[1].handleIn = { 15.0, 8.0 };
		chain.nodes[1].split = true;
		for (size_t i = 0; i < chain.nodes.size(); ++i)
			chain.nodes[i].attributes.wallType = static_cast<int>(i) + 1;
		return chain;
	}
}

// -------------------------------------------------------

TEST_CASE("Facade footprints: rectangles and circles run counter-clockwise from their front", "[facade_edit]")
{
	// Drawn west to east with the inside to the north: the first wall faces south
	const Splines::Chain north = MakeRectangle({ 0.0, 0.0 }, { 10.0, 0.0 }, 5.0);
	REQUIRE(north.nodes.size() == 4);
	CHECK(north.closed);
	CHECK(GetSignedArea(north) == Approx(50.0));
	std::vector<WallInfo> walls = GetWalls(north);
	REQUIRE(walls.size() == 4);
	CHECK(walls[0].start.x == Approx(0.0));
	CHECK(walls[0].end.x == Approx(10.0));
	CHECK(walls[0].heading == Approx(180.0));
	CHECK(walls[1].heading == Approx(90.0));
	CHECK(walls[1].length == Approx(5.0));
	CHECK(GetPerimeter(north) == Approx(30.0));

	// The inside to the south: the front still is the first edge drawn, now run east to west and facing north
	const Splines::Chain south = MakeRectangle({ 0.0, 0.0 }, { 10.0, 0.0 }, -5.0);
	CHECK(GetSignedArea(south) == Approx(50.0));
	walls = GetWalls(south);
	CHECK(walls[0].start.x == Approx(10.0));
	CHECK(walls[0].end.x == Approx(0.0));
	CHECK(walls[0].heading == Approx(0.0).margin(1e-9));

	const Splines::Chain circle = MakeCircle({ 0.0, 0.0 }, 10.0, 12);
	REQUIRE(circle.nodes.size() == 12);
	CHECK(GetSignedArea(circle) == Approx(0.5 * 12 * 100.0 * std::sin(2.0 * 3.14159265358979 / 12)));
	CHECK(GetWalls(circle)[0].heading == Approx(180.0));
	CHECK(GetCircleSides(10.0, 5.0) == 13);
	CHECK(GetCircleSides(0.5, 5.0) == 6);

	// A clockwise list becomes counter-clockwise; open chains keep their order
	const Splines::Chain fixed = MakeChain({ { 0.0, 0.0 }, { 0.0, -5.0 }, { 10.0, -5.0 }, { 10.0, 0.0 } }, true);
	CHECK(GetSignedArea(fixed) > 0.0);
	const Splines::Chain line = MakeChain({ { 0.0, 0.0 }, { 0.0, -5.0 } }, false);
	CHECK(line.nodes[1].position.z == Approx(-5.0));
	CHECK(GetSignedArea(line) == 0.0);
	CHECK(GetWalls(line).size() == 1);
}

TEST_CASE("Facade footprints: curved walls are cut into even straight pieces (the 3D sidewalk recipe)", "[facade_edit]")
{
	Splines::Chain chain = CurvedFront();
	const double curveLength = GetWalls(chain)[0].length;
	REQUIRE(GetWalls(chain)[0].curved);
	REQUIRE(curveLength > 20.5);
	const Splines::Bezier original = Splines::GetSegment(chain, 0);

	size_t added = 0;
	REQUIRE(SubdivideCurves(chain, 0.65, &added));
	const size_t pieces = static_cast<size_t>(std::lround(curveLength / 0.65));
	CHECK(added == pieces - 1);
	REQUIRE(chain.nodes.size() == 4 + added);

	// Every wall is straight now; the new ones are as long as each other and carry the curved wall's type
	const std::vector<WallInfo> walls = GetWalls(chain);
	for (const WallInfo &wall : walls)
		CHECK_FALSE(wall.curved);
	for (size_t i = 0; i < pieces; ++i)
	{
		CHECK(walls[i].length == Approx(curveLength / static_cast<double>(pieces)).epsilon(0.02));
		CHECK(walls[i].wallType == 1);
	}
	CHECK(walls[pieces].wallType == 2);	// The next original wall keeps its own

	// The new nodes sit on the original curve
	for (size_t i = 1; i < pieces; ++i)
	{
		const XPlane::Point2 &p = chain.nodes[i].position;
		double nearest = 1e9;
		for (int step = 0; step <= 2000; ++step)
		{
			const XPlane::Point2 q = original.Evaluate(step / 2000.0);
			nearest = std::min(nearest, std::hypot(q.x - p.x, q.z - p.z));
		}
		CHECK(nearest < 0.02);
	}
	CHECK_FALSE(chain.nodes[0].HasCurve());
	CHECK_FALSE(SubdivideCurves(chain));	// Nothing curved is left
}

TEST_CASE("Facade footprints: subdividing keeps the other nodes' coordinates bit for bit", "[facade_edit]")
{
	// Through the project's rings: only the curve's own nodes change
	ProjectFile::GeoRing ring;
	for (const auto &[east, north] : std::vector<std::pair<double, double>>{ { 0.0, 0.0 }, { 20.0, 0.0 }, { 20.0, 10.0 }, { 0.0, 10.0 } })
	{
		ProjectFile::GeoNode node;
		PROJECTION.ToGeo(XPlane::Point2{ east, -north }, node.latitude, node.longitude);
		ring.push_back(node);
	}
	ring[0].hiLatitude = -0.00007;	// About 8 m south
	ring[0].hiLongitude = 0.00006;
	ring[0].split = true;
	ring[1].loLatitude = -0.00007;
	ring[1].loLongitude = -0.00006;
	ring[1].split = true;
	ring[2].wallType = 3;

	Splines::Shape shape = Splines::FromGeoRings({ ring }, true, PROJECTION);
	REQUIRE(SubdivideCurves(shape.chains[0], 1.0));
	const std::vector<ProjectFile::GeoRing> rings = Splines::ToGeoRings(shape, PROJECTION);
	REQUIRE(rings.size() == 1);
	const ProjectFile::GeoRing &out = rings[0];
	REQUIRE(out.size() > ring.size());
	CHECK(out.front().latitude == ring[0].latitude);
	CHECK(out.front().longitude == ring[0].longitude);
	CHECK_FALSE(out.front().HasBezier());
	const ProjectFile::GeoNode &last = out.back();
	CHECK(last.latitude == ring[3].latitude);
	CHECK(last.longitude == ring[3].longitude);
	CHECK(std::ranges::count_if(out, [](const ProjectFile::GeoNode &node) { return node.wallType == 3; }) == 1);
}

TEST_CASE("Facade footprints: squaring corners", "[facade_edit]")
{
	// A quad drawn by hand: corners a little off square
	Splines::Chain chain = MakeChain({ { 0.0, 0.0 }, { 20.0, 0.4 }, { 19.6, -10.0 }, { -0.3, -10.2 } }, true);
	for (size_t i = 0; i < chain.nodes.size(); ++i)
		chain.nodes[i].attributes.wallType = static_cast<int>(i);
	std::string why;
	REQUIRE(SquareCorners(chain, 15.0, &why));
	const std::vector<WallInfo> walls = GetWalls(chain);
	for (size_t i = 0; i < walls.size(); ++i)
	{
		const WallInfo &a = walls[i], &b = walls[(i + 1) % walls.size()];
		const double ax = a.end.x - a.start.x, az = a.end.z - a.start.z, bx = b.end.x - b.start.x, bz = b.end.z - b.start.z;
		CHECK((ax * bx + az * bz) / (a.length * b.length) == Approx(0.0).margin(1e-9));
	}
	CHECK(WallTypes(chain) == std::vector<int>{ 0, 1, 2, 3 });
	CHECK(std::abs(GetSignedArea(chain) - 200.0) < 10.0);

	// Square already: nothing to do; curved walls: refused
	CHECK_FALSE(SquareCorners(chain, 15.0, &why));
	CHECK(why == "Already square");
	Splines::Chain curved = CurvedFront();
	CHECK_FALSE(SquareCorners(curved, 15.0, &why));
	CHECK(why.find("curved") != std::string::npos);

	// A wall far off the axes keeps its direction (a diagonal corner cut)
	Splines::Chain cut = MakeChain({ { 0.0, 0.0 }, { 20.0, 0.0 }, { 20.0, -6.0 }, { 14.0, -12.0 }, { 0.0, -12.2 } }, true);
	REQUIRE(SquareCorners(cut, 10.0));
	const std::vector<WallInfo> cutWalls = GetWalls(cut);
	const double diagonal = std::atan2(cutWalls[2].end.z - cutWalls[2].start.z, cutWalls[2].end.x - cutWalls[2].start.x);
	CHECK(std::abs(std::remainder(diagonal, 3.14159265358979 / 2.0)) > 0.5);
}

TEST_CASE("Facade footprints: front wall, direction, wall types and offsets", "[facade_edit]")
{
	Splines::Chain chain = MakeRectangle({ 0.0, 0.0 }, { 10.0, 0.0 }, 10.0);
	CHECK(SetWallTypes(chain, { 1, 3 }, 2));
	CHECK(WallTypes(chain) == std::vector<int>{ 0, 2, 0, 2 });
	CHECK_FALSE(SetWallTypes(chain, { 1 }, 2));
	CHECK(SetWallTypes(chain, {}, 1));
	CHECK(WallTypes(chain) == std::vector<int>{ 1, 1, 1, 1 });
	chain.nodes[2].attributes.wallType = 5;

	// The front moves to wall 2: the chain starts there, each wall keeping its type
	REQUIRE(SetFrontWall(chain, 2));
	CHECK(WallTypes(chain) == std::vector<int>{ 5, 1, 1, 1 });
	CHECK(GetWalls(chain)[0].heading == Approx(0.0).margin(1e-9));
	CHECK_FALSE(SetFrontWall(chain, 0));
	CHECK_FALSE(SetFrontWall(chain, 9));

	// A clockwise footprint is reversed; types stay with their walls
	Splines::Chain clockwise;
	clockwise.closed = true;
	for (const XPlane::Point2 &p : std::vector<XPlane::Point2>{ { 0.0, 0.0 }, { 0.0, -10.0 }, { 10.0, -10.0 }, { 10.0, 0.0 } })
	{
		Splines::Node node;
		node.position = p;
		clockwise.nodes.push_back(node);
	}
	clockwise.nodes[0].attributes.wallType = 7;	// The west wall (0,0) -> (0,-10)
	REQUIRE(GetSignedArea(clockwise) < 0.0);
	REQUIRE(MakeCounterClockwise(clockwise));
	CHECK(GetSignedArea(clockwise) > 0.0);
	for (const WallInfo &wall : GetWalls(clockwise))
	{
		if (wall.wallType == 7)
		{
			CHECK(wall.start.x == Approx(0.0));
			CHECK(wall.end.x == Approx(0.0));
			CHECK(wall.heading == Approx(270.0));	// Now facing out (west)
		}
	}
	CHECK_FALSE(MakeCounterClockwise(clockwise));

	// Offsets: out grows the square on every side, in shrinks it; wall types come along
	Splines::Chain square = MakeRectangle({ 0.0, 0.0 }, { 10.0, 0.0 }, 10.0);
	square.nodes[1].attributes.wallType = 4;
	const Splines::Chain out = Offset(square, 2.0);
	REQUIRE(out.nodes.size() == 4);
	CHECK(GetSignedArea(out) == Approx(196.0));
	CHECK(out.nodes[0].position.x == Approx(-2.0));
	CHECK(out.nodes[0].position.z == Approx(2.0));
	CHECK(WallTypes(out) == std::vector<int>{ 0, 4, 0, 0 });
	CHECK(GetSignedArea(Offset(square, -2.0)) == Approx(36.0));

	// An open chain moves to the right of travel
	const Splines::Chain fence = MakeChain({ { 0.0, 0.0 }, { 10.0, 0.0 } }, false);
	const Splines::Chain moved = Offset(fence, 1.5);
	REQUIRE(moved.nodes.size() == 2);
	CHECK_FALSE(moved.closed);
	CHECK(moved.nodes[0].position.z == Approx(1.5));
	CHECK(moved.nodes[1].position.z == Approx(1.5));
}

TEST_CASE("Facade wall types move to another facade by name", "[facade_edit]")
{
	const std::vector<std::string> from = { "natural_curb", "white_curb", "no_curb" };
	const std::vector<std::string> to = { "white_curb", "no_curb", "red_curb", "natural_curb" };
	CHECK(RemapWallType(0, from, to) == 3);
	CHECK(RemapWallType(2, from, to) == 1);
	CHECK(RemapWallType(9, from, to) == 3);		// Unknown: clamped
	CHECK(RemapWallType(1, from, { "a", "b" }) == 1);
	CHECK(RemapWallType(3, from, {}) == 0);

	// The walls as the layout reads them
	const std::vector<XPlane::FacadeSideInput> sides = ToSideInputs(GetWalls(MakeRectangle({ 0.0, 0.0 }, { 4.0, 0.0 }, 2.0)));
	REQUIRE(sides.size() == 4);
	CHECK(sides[0].length == Approx(4.0));
	CHECK(sides[0].heading == Approx(180.0));
	CHECK(sides[1].length == Approx(2.0));
}

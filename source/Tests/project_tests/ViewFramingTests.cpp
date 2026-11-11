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
 * ViewFramingTests.cpp
 * -------------------------------------------------------
 * Created: 10/11/2026
 * -------------------------------------------------------
 */
#include <catch2/catch_all.hpp>
#include <SceneryEditorX/scene/view_framing.h>
#include <cmath>
#include <limits>
#include <numbers>
#include <random>
#include <vector>

using namespace SceneryEditorX::ViewFraming;
using Catch::Approx;

// -------------------------------------------------------

namespace
{
	/* @brief A lens the way the editor's camera rig builds one: yaw / pitch in radians, +Y up, -Z north. */
	Lens RigLens(const double yaw, const double pitch, const double fovHorizontalDegrees, const double aspect, const bool orthographic = false)
	{
		Lens lens;
		const double cosPitch = std::cos(pitch);
		lens.forward = { cosPitch * std::sin(yaw), std::sin(pitch), cosPitch * std::cos(yaw) };
		lens.right = { -std::cos(yaw), 0.0, std::sin(yaw) };
		const Point &r = lens.right, &f = lens.forward;
		lens.up = { r.y * f.z - r.z * f.y, r.z * f.x - r.x * f.z, r.x * f.y - r.y * f.x };
		lens.tanHalfX = std::tan(fovHorizontalDegrees * std::numbers::pi / 360.0);
		lens.tanHalfY = lens.tanHalfX / aspect;
		lens.orthographic = orthographic;
		return lens;
	}

	std::vector<Point> Corners(const Box &box)
	{
		std::vector<Point> corners;
		for (int corner = 0; corner < 8; ++corner)
			corners.push_back({ (corner & 1) ? box.max.x : box.min.x, (corner & 2) ? box.max.y : box.min.y, (corner & 4) ? box.max.z : box.min.z });
		return corners;
	}

	bool AllInView(const Box &box, const double distance, const Lens &lens, const double fill, const double nearMargin)
	{
		for (const Point &corner : Corners(box))
			if (!IsInView(corner, box.GetCenter(), distance, lens, fill, nearMargin))
				return false;
		return true;
	}

	Box MakeBox(const Point &min, const Point &max)
	{
		Box box;
		box.Add(min, max);
		return box;
	}
}

// -------------------------------------------------------

TEST_CASE("View framing: boxes grow from what can be framed and leave the rest out", "[view-framing]")
{
	Box box;
	CHECK_FALSE(box.IsValid());
	CHECK(box.GetRadius() == 0.0);

	CHECK(box.Add(Point{ 10.0, 2.0, -5.0 }));
	REQUIRE(box.IsValid());
	CHECK(box.GetSize().x == 0.0);
	CHECK(box.Add(Point{ -10.0, 6.0, 15.0 }));
	CHECK(box.min.x == -10.0);
	CHECK(box.max.z == 15.0);
	CHECK(box.GetCenter().x == 0.0);
	CHECK(box.GetCenter().y == 4.0);
	CHECK(box.GetCenter().z == 5.0);
	CHECK(box.GetRadius() == Approx(0.5 * std::sqrt(20.0 * 20.0 + 4.0 * 4.0 + 20.0 * 20.0)));

	// Bad data never moves the box: NaN, infinity, far out in the void, an empty box (min above max) from a mesh without vertices
	const double nan = std::numeric_limits<double>::quiet_NaN();
	const double inf = std::numeric_limits<double>::infinity();
	const Box before = box;
	CHECK_FALSE(box.Add(Point{ nan, 0.0, 0.0 }));
	CHECK_FALSE(box.Add(Point{ 0.0, inf, 0.0 }));
	CHECK_FALSE(box.Add(Point{ 0.0, 0.0, 2.0 * MAX_COORDINATE }));
	CHECK_FALSE(box.Add(Point{ inf, inf, inf }, Point{ -inf, -inf, -inf }));
	CHECK_FALSE(box.Add(Point{ 1.0, 1.0, 1.0 }, Point{ 0.0, 2.0, 2.0 }));
	CHECK_FALSE(box.Add(Box{}));
	CHECK(box.min.x == before.min.x);
	CHECK(box.max.y == before.max.y);
	CHECK(box.max.z == before.max.z);

	// A box of boxes holds every corner
	Box merged;
	CHECK(merged.Add(MakeBox({ 0.0, 0.0, 0.0 }, { 1.0, 1.0, 1.0 })));
	CHECK(merged.Add(MakeBox({ -3.0, 0.5, 4.0 }, { -2.0, 8.0, 5.0 })));
	CHECK(merged.min.x == -3.0);
	CHECK(merged.max.y == 8.0);
	CHECK(merged.max.z == 5.0);
	CHECK(IsUsable(Point{ MAX_COORDINATE, -MAX_COORDINATE, 0.0 }));
}

TEST_CASE("View framing: inflating widens short axes about the center and leaves long ones alone", "[view-framing]")
{
	Box point;
	point.Add(Point{ 100.0, 20.0, -40.0 });
	point.Inflate(4.0);
	CHECK(point.GetSize().x == Approx(4.0));
	CHECK(point.GetSize().y == Approx(4.0));
	CHECK(point.GetSize().z == Approx(4.0));
	CHECK(point.GetCenter().x == Approx(100.0));
	CHECK(point.GetCenter().y == Approx(20.0));
	CHECK(point.GetCenter().z == Approx(-40.0));

	// A flat apron: only its height grows
	Box apron = MakeBox({ -200.0, 3.0, -50.0 }, { 200.0, 3.0, 50.0 });
	apron.Inflate(4.0);
	CHECK(apron.GetSize().x == Approx(400.0));
	CHECK(apron.GetSize().y == Approx(4.0));
	CHECK(apron.GetSize().z == Approx(100.0));
	CHECK(apron.GetCenter().y == Approx(3.0));

	Box empty;
	empty.Inflate(10.0);
	CHECK_FALSE(empty.IsValid());
}

TEST_CASE("View framing: every corner of a framed box is in view, and no nearer camera sees them all", "[view-framing]")
{
	std::mt19937 random(20261110u);
	std::uniform_real_distribution<double> yaws(-std::numbers::pi, std::numbers::pi);
	std::uniform_real_distribution<double> pitches(-0.5 * std::numbers::pi, 0.25 * std::numbers::pi);
	std::uniform_real_distribution<double> positions(-3000.0, 3000.0);
	std::uniform_real_distribution<double> sizes(0.0, 400.0);

	struct LensSetup
	{
		double fov;
		double aspect;
		bool orthographic;
	};
	const LensSetup setups[] = {
		{ 60.0, 16.0 / 9.0, false },	// The editor's default
		{ 60.0, 4.0 / 3.0, false },
		{ 60.0, 0.5, false },			// A tall, narrow viewport: the width decides
		{ 120.0, 21.0 / 9.0, false },	// Wide: a sphere's tangent fit would be far too close here
		{ 15.0, 1.0, false },			// Narrow telephoto
		{ 60.0, 16.0 / 9.0, true },
		{ 60.0, 0.5, true },
	};

	int framed = 0;
	for (const LensSetup &setup : setups)
	{
		for (int i = 0; i < 400; ++i)
		{
			const Lens lens = RigLens(yaws(random), pitches(random), setup.fov, setup.aspect, setup.orthographic);
			const Point origin{ positions(random), positions(random) * 0.1, positions(random) };
			const Box box = MakeBox(origin, { origin.x + sizes(random), origin.y + sizes(random) * 0.25, origin.z + sizes(random) });
			const double distance = FitDistance(box, lens, 0.9, 0.5);
			INFO("fov " << setup.fov << " aspect " << setup.aspect << " orthographic " << setup.orthographic << " case " << i);
			REQUIRE(distance > 0.0);
			CHECK(AllInView(box, distance, lens, 0.9, 0.5));

			// Tight: a camera 2% nearer leaves a corner outside (the fit isn't just safe, it's the closest that works)
			CHECK_FALSE(AllInView(box, distance * 0.98, lens, 0.9, 0.5));

			// Never farther than fitting the box's bounding sphere
			CHECK(distance <= FitSphereDistance(box.GetRadius(), lens, 0.9, 0.5) * (1.0 + 1e-9));
			++framed;
		}
	}
	CHECK(framed == 7 * 400);
}

TEST_CASE("View framing: a long runway frames tighter than its bounding sphere, along it or across it", "[view-framing]")
{
	// 3000 m x 45 m, running east-west, seen from 20 degrees above the horizon
	const Box runway = MakeBox({ -1500.0, 0.0, -22.5 }, { 1500.0, 0.5, 22.5 });
	const double pitch = -0.35;
	const Lens alongIt = RigLens(std::numbers::pi * 0.5, pitch, 60.0, 16.0 / 9.0);	// Looking east, down the runway
	const Lens across = RigLens(0.0, pitch, 60.0, 16.0 / 9.0);						// Looking south, across it

	const double endOn = FitDistance(runway, alongIt);
	const double sideOn = FitDistance(runway, across);
	const double sphere = FitSphereDistance(runway.GetRadius(), across);
	CHECK(endOn < sphere * 0.7);
	CHECK(sideOn < sphere * 0.7);
	CHECK(AllInView(runway, endOn, alongIt, DEFAULT_FILL, DEFAULT_NEAR_MARGIN));
	CHECK(AllInView(runway, sideOn, across, DEFAULT_FILL, DEFAULT_NEAR_MARGIN));

	// Across, the length fills the width: 1500 m to each side, the camera behind the runway's near edge (its nearest
	// corner, 22.5 m and 0.25 m from the center, is that much less deep in the view)
	const double tanX = std::tan(std::numbers::pi / 6.0) * DEFAULT_FILL;
	const double nearest = std::abs(across.forward.z) * 22.5 + std::abs(across.forward.y) * 0.25;
	CHECK(sideOn == Approx(1500.0 / tanX + nearest).epsilon(1e-9));

	// End-on, the near end's lower corner under the camera decides: it has to stay above the bottom edge of the view
	// (1500 m back and 0.25 m down from the center: lower on the screen, a little deeper in the view)
	const double tanY = tanX / (16.0 / 9.0);
	const double below = 1500.0 * std::abs(alongIt.up.x) + 0.25 * std::abs(alongIt.up.y);
	const double depth = -1500.0 * std::abs(alongIt.forward.x) + 0.25 * std::abs(alongIt.forward.y);
	CHECK(endOn == Approx(below / tanY - depth).epsilon(1e-9));
}

TEST_CASE("View framing: the nearest corner stays in front of the eye", "[view-framing]")
{
	// Straight down onto a tall, thin mast: its top must not end up behind the camera
	const Box mast = MakeBox({ -1.0, 0.0, -1.0 }, { 1.0, 300.0, 1.0 });
	const Lens down = RigLens(0.0, -0.5 * std::numbers::pi, 60.0, 16.0 / 9.0);
	const double distance = FitDistance(mast, down, DEFAULT_FILL, 2.0);
	CHECK(distance >= 150.0 + 2.0 - 1e-9);
	CHECK(AllInView(mast, distance, down, DEFAULT_FILL, 2.0));

	// Orthographic the same: the view is as wide anywhere, but what is behind the eye isn't drawn
	Lens ortho = down;
	ortho.orthographic = true;
	const double orthoDistance = FitDistance(mast, ortho, DEFAULT_FILL, 2.0);
	CHECK(orthoDistance >= 152.0 - 1e-9);
	CHECK(AllInView(mast, orthoDistance, ortho, DEFAULT_FILL, 2.0));

	CHECK(FitDistance(Box{}, down) == 0.0);
}

TEST_CASE("View framing: a framed sphere is in view all the way round", "[view-framing]")
{
	for (const bool orthographic : { false, true })
	{
		for (const double fov : { 30.0, 60.0, 100.0, 140.0 })
		{
			const Lens lens = RigLens(0.7, -0.4, fov, 16.0 / 9.0, orthographic);
			const double radius = 250.0;
			const double distance = FitSphereDistance(radius, lens);
			INFO("fov " << fov << " orthographic " << orthographic);
			int outside = 0;
			for (int a = 0; a < 36; ++a)
			{
				for (int b = -8; b <= 8; ++b)
				{
					const double theta = a * std::numbers::pi / 18.0, phi = b * std::numbers::pi / 18.0;
					const Point point{ radius * std::cos(phi) * std::cos(theta), radius * std::sin(phi), radius * std::cos(phi) * std::sin(theta) };
					outside += IsInView(point, Point{}, distance, lens, DEFAULT_FILL, 0.0) ? 0 : 1;
				}
			}
			CHECK(outside == 0);
			// Wide lenses: the old tangent fit (radius / tan) would have put the camera inside a quarter of the sphere's view
			if (!orthographic && fov >= 100.0)
				CHECK(distance > radius / (std::min(lens.tanHalfX, lens.tanHalfY) * DEFAULT_FILL) * 1.1);
		}
	}
	CHECK(FitSphereDistance(0.0, RigLens(0.0, 0.0, 60.0, 1.0)) == Approx(DEFAULT_NEAR_MARGIN));
	CHECK(FitSphereDistance(std::numeric_limits<double>::quiet_NaN(), RigLens(0.0, 0.0, 60.0, 1.0)) == Approx(DEFAULT_NEAR_MARGIN));
}

TEST_CASE("View framing: a bad fill neither hides nor fits everything", "[view-framing]")
{
	// A point on the edge of the full view, straight ahead of an eye 100 m back: in view at fill 1, not at 0.5
	const Lens lens = RigLens(0.0, 0.0, 60.0, 1.0);
	const double edge = 100.0 * lens.tanHalfX;
	const Point onEdge{ -edge, 0.0, 0.0 };	// The rig's right is -X when looking south
	CHECK(IsInView(onEdge, Point{}, 100.0, lens, 1.0, 0.0));
	CHECK_FALSE(IsInView(onEdge, Point{}, 100.0, lens, 0.5, 0.0));

	// NaN or infinite fills check the whole view, as a fill of 1 does (a NaN used to fail every point)
	const double nan = std::numeric_limits<double>::quiet_NaN();
	const double inf = std::numeric_limits<double>::infinity();
	CHECK(IsInView(onEdge, Point{}, 100.0, lens, nan, 0.0));
	CHECK(IsInView(onEdge, Point{}, 100.0, lens, inf, 0.0));
	CHECK(IsInView(Point{}, Point{}, 100.0, lens, nan, 0.0));
	CHECK_FALSE(IsInView(Point{ -edge * 1.01, 0.0, 0.0 }, Point{}, 100.0, lens, nan, 0.0));

	// Fitting with a bad fill falls back on the default share of the view
	Box box;
	box.Add(Point{ -10.0, -10.0, -10.0 }, Point{ 10.0, 10.0, 10.0 });
	CHECK(FitDistance(box, lens, nan) == Approx(FitDistance(box, lens, DEFAULT_FILL)));
}

// -------------------------------------------------------

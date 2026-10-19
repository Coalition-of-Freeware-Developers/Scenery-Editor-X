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
 * SplineEditTests.cpp
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#include <catch2/catch_all.hpp>
#include <SceneryEditorX/project/spline_edit.h>
#include <cmath>

using namespace SceneryEditorX;
using namespace SceneryEditorX::Splines;
using Catch::Approx;

// -------------------------------------------------------

namespace
{
	const GeoProjection PROJECTION(44.6010278, -123.9396111);

	XPlane::AptNode AptPoint(const double latitude, const double longitude, const int line = 0, const int light = 0)
	{
		XPlane::AptNode node;
		node.latitude = latitude;
		node.longitude = longitude;
		node.lineType = line;
		node.lightType = light;
		return node;
	}

	XPlane::AptNode AptCurve(const double latitude, const double longitude, const double controlLatitude, const double controlLongitude, const int line = 0)
	{
		XPlane::AptNode node = AptPoint(latitude, longitude, line);
		node.hasControl = true;
		node.controlLatitude = controlLatitude;
		node.controlLongitude = controlLongitude;
		return node;
	}

	bool SameAptNode(const XPlane::AptNode &a, const XPlane::AptNode &b)
	{
		return a.latitude == b.latitude && a.longitude == b.longitude && a.hasControl == b.hasControl &&
			(!a.hasControl || (a.controlLatitude == b.controlLatitude && a.controlLongitude == b.controlLongitude)) &&
			a.lineType == b.lineType && a.lightType == b.lightType;
	}

	void RequireSameChains(const std::vector<XPlane::AptNodeChain> &a, const std::vector<XPlane::AptNodeChain> &b)
	{
		REQUIRE(a.size() == b.size());
		for (size_t c = 0; c < a.size(); ++c)
		{
			INFO("chain " << c);
			REQUIRE(a[c].closed == b[c].closed);
			REQUIRE(a[c].nodes.size() == b[c].nodes.size());
			for (size_t n = 0; n < a[c].nodes.size(); ++n)
			{
				INFO("node " << n);
				CHECK(SameAptNode(a[c].nodes[n], b[c].nodes[n]));
			}
		}
	}

	/* A taxiway outline with a curve, a split pair in the middle and a split pair closing the ring (incoming half last). */
	std::vector<XPlane::AptNodeChain> MakeAptPavement()
	{
		XPlane::AptNodeChain outline;
		outline.closed = true;
		outline.nodes = {
			AptCurve(44.6030000, -123.9410000, 44.6030500, -123.9409000, 51),	// split: outgoing half of node 0
			AptPoint(44.6032000, -123.9405000, 51),
			AptCurve(44.6029000, -123.9401000, 44.6028000, -123.9400500, 1),	// smooth curve
			AptCurve(44.6025000, -123.9403000, 44.6025200, -123.9404000, 1),	// split pair: incoming half...
			AptCurve(44.6025000, -123.9403000, 44.6024500, -123.9403500, 2),	// ...and outgoing half
			AptPoint(44.6024000, -123.9409000),
			AptCurve(44.6030000, -123.9410000, 44.6029800, -123.9411000, 0)	// incoming half of node 0 (closes the ring)
		};

		XPlane::AptNodeChain hole;
		hole.closed = true;
		hole.nodes = { AptPoint(44.6027000, -123.9407000), AptPoint(44.6028000, -123.9406000), AptPoint(44.6027000, -123.9405000) };
		return { outline, hole };
	}

	ProjectFile::GeoNode GeoPoint(const double latitude, const double longitude)
	{
		ProjectFile::GeoNode node;
		node.latitude = latitude;
		node.longitude = longitude;
		return node;
	}

	double Distance(const Point2 &a, const Point2 &b)
	{
		return std::hypot(a.x - b.x, a.z - b.z);
	}
}

// -------------------------------------------------------

TEST_CASE("Spline edit: apt.dat chains round trip bit for bit", "[splines][apt]")
{
	const std::vector<XPlane::AptNodeChain> source = MakeAptPavement();
	const Shape shape = FromAptChains(source, PROJECTION);

	// Co-located pairs are one split node each: 7 + 3 rows, 5 + 3 nodes
	REQUIRE(shape.chains.size() == 2);
	CHECK(shape.chains[0].nodes.size() == 5);
	CHECK(shape.chains[0].wrapsSplit);
	CHECK(shape.chains[0].nodes[0].split);
	CHECK(shape.chains[0].nodes[3].split);
	CHECK_FALSE(shape.chains[0].nodes[2].split);
	CHECK(shape.chains[0].nodes[2].hasIn);
	CHECK(shape.chains[1].nodes.size() == 3);

	// The split node keeps both halves' styles
	CHECK(shape.chains[0].nodes[3].attributes.lineTypeIn == 1);
	CHECK(shape.chains[0].nodes[3].attributes.lineType == 2);

	RequireSameChains(ToAptChains(shape, PROJECTION), source);
}

TEST_CASE("Spline edit: project rings round trip bit for bit", "[splines][project]")
{
	ProjectFile::GeoRing ring = { GeoPoint(44.60, -123.94), GeoPoint(44.601, -123.939), GeoPoint(44.6005, -123.938), GeoPoint(44.5995, -123.9385) };
	ring[1].hiLatitude = 0.00012345678901;		// unsplit curve: lo = -hi
	ring[1].hiLongitude = -0.00009876543211;
	ring[1].loLatitude = -ring[1].hiLatitude;
	ring[1].loLongitude = -ring[1].hiLongitude;
	ring[2].split = true;						// split with only an incoming handle
	ring[2].loLatitude = 0.00003;
	ring[2].loLongitude = 0.00007;
	ring[3].hasUv = true;
	ring[3].s = 0.25f;
	ring[3].t = 0.75f;
	ring[3].wallType = 2;
	ring[0].spawnTiles = false;

	const std::vector<ProjectFile::GeoRing> source = { ring };
	const Shape shape = FromGeoRings(source, true, PROJECTION);
	REQUIRE(shape.chains.size() == 1);
	CHECK(shape.chains[0].nodes[1].hasIn);
	CHECK(shape.chains[0].nodes[1].hasOut);
	CHECK(shape.chains[0].nodes[2].split);
	CHECK(shape.chains[0].nodes[2].hasIn);
	CHECK_FALSE(shape.chains[0].nodes[2].hasOut);

	const std::vector<ProjectFile::GeoRing> written = ToGeoRings(shape, PROJECTION);
	REQUIRE(written.size() == 1);
	REQUIRE(written[0].size() == ring.size());
	for (size_t i = 0; i < ring.size(); ++i)
	{
		INFO("node " << i);
		CHECK(written[0][i] == ring[i]);
	}
}

TEST_CASE("Spline edit: moving one node changes only that node", "[splines][apt]")
{
	const std::vector<XPlane::AptNodeChain> source = MakeAptPavement();
	Shape shape = FromAptChains(source, PROJECTION);
	const Point2 before = shape.chains[0].nodes[2].position;
	const Point2 handleBefore = shape.chains[0].nodes[2].handleOut;
	TransformNodes(shape, { { 0, 2 } }, Affine2::Translation(2.0, -1.0));

	// The handles travel with the node
	CHECK(shape.chains[0].nodes[2].position.x == Approx(before.x + 2.0));
	CHECK(shape.chains[0].nodes[2].handleOut.z == Approx(handleBefore.z - 1.0));

	const std::vector<XPlane::AptNodeChain> written = ToAptChains(shape, PROJECTION);
	REQUIRE(written[0].nodes.size() == source[0].nodes.size());
	for (size_t i = 0; i < source[0].nodes.size(); ++i)
	{
		INFO("row " << i);
		if (i == 2)
		{
			CHECK_FALSE(SameAptNode(written[0].nodes[i], source[0].nodes[i]));
			// 1 m south = -1 / meters per degree of latitude
			CHECK(written[0].nodes[i].latitude == Approx(source[0].nodes[i].latitude + 1.0 / PROJECTION.GetMetersPerDegreeLatitude()).epsilon(1e-12));
			CHECK(written[0].nodes[i].longitude == Approx(source[0].nodes[i].longitude + 2.0 / PROJECTION.GetMetersPerDegreeLongitude()).epsilon(1e-12));
		}
		else
			CHECK(SameAptNode(written[0].nodes[i], source[0].nodes[i]));
	}
	RequireSameChains({ written[1] }, { source[1] });
}

TEST_CASE("Spline edit: inserting a node keeps the curve", "[splines][edit]")
{
	Shape shape = FromAptChains(MakeAptPavement(), PROJECTION);
	const Chain original = shape.chains[0];

	// Segment 2 runs from the smooth curve node into the split node: curved on both ends
	const Bezier curve = GetSegment(original, 2);
	REQUIRE_FALSE(curve.IsStraight());
	constexpr double t = 0.3;
	NodeRef inserted;
	REQUIRE(InsertNode(shape, 0, 2, t, &inserted));
	CHECK(inserted.chain == 0);
	CHECK(inserted.node == 3);
	REQUIRE(shape.chains[0].nodes.size() == original.nodes.size() + 1);

	const Bezier left = GetSegment(shape.chains[0], 2);
	const Bezier right = GetSegment(shape.chains[0], 3);
	for (int i = 0; i <= 20; ++i)
	{
		const double u = i / 20.0;
		CHECK(Distance(left.Evaluate(u), curve.Evaluate(t * u)) < 1e-9);
		CHECK(Distance(right.Evaluate(u), curve.Evaluate(t + (1.0 - t) * u)) < 1e-9);
	}

	// The new node continues the segment's style; the shortened handle split the previously smooth node
	CHECK(shape.chains[0].nodes[3].attributes.lineType == original.nodes[2].attributes.lineType);
	CHECK(shape.chains[0].nodes[2].split);

	// Written out and read back, the curve is the same
	const Shape reread = FromAptChains(ToAptChains(shape, PROJECTION), PROJECTION);
	REQUIRE(reread.chains[0].nodes.size() == shape.chains[0].nodes.size());
	for (uint32_t s = 0; s < shape.chains[0].GetSegmentCount(); ++s)
	{
		const Bezier a = GetSegment(shape.chains[0], s);
		const Bezier b = GetSegment(reread.chains[0], s);
		for (int i = 0; i <= 8; ++i)
			CHECK(Distance(a.Evaluate(i / 8.0), b.Evaluate(i / 8.0)) < 1e-6);
	}

	// A straight segment gets a plain node in the middle; the closing segment of a ring inserts at the end
	Shape square = FromGeoRings({ { GeoPoint(44.60, -123.94), GeoPoint(44.60, -123.93), GeoPoint(44.61, -123.93), GeoPoint(44.61, -123.94) } }, true, PROJECTION);
	REQUIRE(InsertNode(square, 0, 3, 0.5, &inserted));
	CHECK(inserted.node == 4);
	const Node &middle = square.chains[0].nodes[4];
	CHECK_FALSE(middle.HasCurve());
	CHECK(middle.position.x == Approx((square.chains[0].nodes[3].position.x + square.chains[0].nodes[0].position.x) * 0.5));
	CHECK_FALSE(InsertNode(square, 0, 9, 0.5));
	CHECK_FALSE(InsertNode(square, 0, 0, 0.0));
}

TEST_CASE("Spline edit: handles mirror unless split", "[splines][edit]")
{
	Shape shape = FromGeoRings({ { GeoPoint(44.60, -123.94), GeoPoint(44.60, -123.93), GeoPoint(44.61, -123.93) } }, false, PROJECTION);
	const Point2 p = shape.chains[0].nodes[1].position;

	SetHandle(shape, { 0, 1 }, true, { p.x + 10.0, p.z }, false);
	const Node &node = shape.chains[0].nodes[1];
	CHECK(node.hasIn);
	CHECK(node.hasOut);
	CHECK_FALSE(node.split);
	CHECK(node.handleIn.x == Approx(p.x - 10.0));

	// Breaking the symmetry splits the node and leaves the other handle where it was
	SetHandle(shape, { 0, 1 }, false, { p.x - 3.0, p.z + 4.0 }, true);
	CHECK(node.split);
	CHECK(node.handleOut.x == Approx(p.x + 10.0));
	CHECK(node.handleIn.z == Approx(p.z + 4.0));

	// Unsplit nodes write lo = -hi; split ones keep both
	const std::vector<ProjectFile::GeoRing> rings = ToGeoRings(shape, PROJECTION);
	CHECK(rings[0][1].split);
	CHECK(rings[0][1].loLatitude != -rings[0][1].hiLatitude);

	// Dropping a split node's handle onto it removes just that handle
	SetHandle(shape, { 0, 1 }, false, p, false);
	CHECK_FALSE(node.hasIn);
	CHECK(node.hasOut);
	SetHandle(shape, { 0, 1 }, true, p, false);
	CHECK_FALSE(node.HasCurve());
	CHECK_FALSE(node.split);
}

TEST_CASE("Spline edit: deleting keeps chains valid", "[splines][edit]")
{
	Shape shape = FromAptChains(MakeAptPavement(), PROJECTION);
	REQUIRE(shape.chains[0].nodes.size() == 5);

	// A ring keeps three nodes (the last ones asked for go first); a hole whose every node goes is removed
	const size_t removed = DeleteNodes(shape, { { 0, 0 }, { 0, 1 }, { 0, 2 }, { 0, 3 }, { 1, 0 }, { 1, 1 }, { 1, 2 } });
	CHECK(removed == 2 + 3);
	CHECK(shape.chains.size() == 1);
	CHECK(shape.chains[0].nodes.size() == 3);
	CHECK(shape.chains[0].wrapsSplit);		// Node 0 stayed, still closing the ring with its incoming half
	CHECK(DeleteNodes(shape, { { 0, 0 } }) == 0);

	// An open chain keeps two
	Shape line = FromGeoRings({ { GeoPoint(44.60, -123.94), GeoPoint(44.60, -123.93), GeoPoint(44.61, -123.93) } }, false, PROJECTION);
	CHECK(DeleteNodes(line, { { 0, 0 }, { 0, 1 }, { 0, 2 } }) == 1);
	CHECK(line.chains[0].nodes.size() == 2);
	CHECK(DeleteNodes(line, { { 0, 7 } }) == 0);

	// Written out, the remaining rows are valid apt.dat
	const std::vector<XPlane::AptNodeChain> written = ToAptChains(shape, PROJECTION);
	CHECK(written[0].nodes.size() >= 3);
}

TEST_CASE("Spline edit: smooth and corner", "[splines][edit]")
{
	Shape shape = FromGeoRings({ { GeoPoint(44.60, -123.94), GeoPoint(44.60, -123.93), GeoPoint(44.61, -123.93), GeoPoint(44.61, -123.94) } }, true, PROJECTION);
	const Chain corners = shape.chains[0];
	SmoothNodes(shape, AllNodes(shape));
	for (size_t i = 0; i < shape.chains[0].nodes.size(); ++i)
	{
		const Node &node = shape.chains[0].nodes[i];
		CHECK(node.hasIn);
		CHECK(node.hasOut);
		CHECK_FALSE(node.split);
		// Mirror images: equal reach either side
		CHECK(Distance(node.handleIn, node.position) == Approx(Distance(node.handleOut, node.position)));
		// Tangent along the line through the neighbors: parallel to next - previous
		const size_t count = corners.nodes.size();
		const Point2 previous = corners.nodes[(i + count - 1) % count].position;
		const Point2 next = corners.nodes[(i + 1) % count].position;
		const double cross = (node.handleOut.x - node.position.x) * (next.z - previous.z) - (node.handleOut.z - node.position.z) * (next.x - previous.x);
		CHECK(std::abs(cross) < 1e-6 * Distance(next, previous) * Distance(node.handleOut, node.position));
		const double dot = (node.handleOut.x - node.position.x) * (next.x - previous.x) + (node.handleOut.z - node.position.z) * (next.z - previous.z);
		CHECK(dot > 0.0);
	}

	// Smooth curves stay inside a reasonable bound of the polygon (no loops)
	const XPlane::Bounds2 nodes = GetBounds(shape);
	const XPlane::Bounds2 handles = GetBounds(shape, true);
	CHECK(handles.maxX - handles.minX < (nodes.maxX - nodes.minX) * 1.5);

	CornerNodes(shape, { { 0, 1 } });
	CHECK_FALSE(shape.chains[0].nodes[1].HasCurve());
	CHECK(shape.chains[0].nodes[0].HasCurve());
}

TEST_CASE("Spline edit: transforms on the ground plane", "[splines][edit]")
{
	const Point2 pivot{ 10.0, 20.0 };
	const Affine2 quarter = Affine2::Rotation(std::acos(-1.0) * 0.5, pivot);

	// Counter-clockwise seen from above (+Y): east (+X) turns to north (-Z)
	const Point2 east = quarter.Apply({ pivot.x + 5.0, pivot.z });
	CHECK(east.x == Approx(pivot.x).margin(1e-12));
	CHECK(east.z == Approx(pivot.z - 5.0));

	const Affine2 twice = quarter.After(quarter);
	const Point2 back = twice.Apply({ pivot.x + 5.0, pivot.z });
	CHECK(back.x == Approx(pivot.x - 5.0));
	CHECK(back.z == Approx(pivot.z).margin(1e-12));

	const Affine2 scale = Affine2::Scale(2.0, 0.5, pivot);
	const Point2 scaled = scale.Apply({ pivot.x + 1.0, pivot.z + 4.0 });
	CHECK(scaled.x == Approx(pivot.x + 2.0));
	CHECK(scaled.z == Approx(pivot.z + 2.0));
	CHECK(Affine2().IsIdentity());
	CHECK_FALSE(quarter.IsIdentity());

	// Scaling keeps unsplit handles mirrored (affine maps keep midpoints)
	Shape shape = FromGeoRings({ { GeoPoint(44.60, -123.94), GeoPoint(44.60, -123.93), GeoPoint(44.61, -123.93) } }, false, PROJECTION);
	SmoothNodes(shape, AllNodes(shape));
	TransformShape(shape, Affine2::Scale(3.0, 0.25, GetCentroid(shape, {})));
	const Node &node = shape.chains[0].nodes[1];
	CHECK((node.handleIn.x + node.handleOut.x) * 0.5 == Approx(node.position.x));
	CHECK((node.handleIn.z + node.handleOut.z) * 0.5 == Approx(node.position.z));
}

TEST_CASE("Spline edit: reversing keeps each segment's style", "[splines][apt]")
{
	XPlane::AptNodeChain chain;
	chain.closed = false;
	chain.nodes = { AptPoint(44.600, -123.940, 1, 101), AptPoint(44.601, -123.939, 2, 0), AptCurve(44.602, -123.938, 44.6025, -123.9375) };
	Shape shape = FromAptChains({ chain }, PROJECTION);
	ReverseChain(shape.chains[0]);

	const Chain &reversed = shape.chains[0];
	REQUIRE(reversed.nodes.size() == 3);
	CHECK(reversed.nodes[0].attributes.lineType == 2);		// old segment 1 (node 1 -> 2), now node 0 -> 1
	CHECK(reversed.nodes[1].attributes.lineType == 1);		// old segment 0
	CHECK(reversed.nodes[1].attributes.lightType == 101);
	CHECK(reversed.nodes[2].attributes.lineType == 0);		// the open end starts nothing

	// The curve's handle now leads into the end node it left
	CHECK(reversed.nodes[0].hasOut);
	CHECK(reversed.nodes[0].hasIn);
	const Bezier before = GetSegment(FromAptChains({ chain }, PROJECTION).chains[0], 1);
	const Bezier after = GetSegment(reversed, 0);
	for (int i = 0; i <= 8; ++i)
		CHECK(Distance(before.Evaluate(1.0 - i / 8.0), after.Evaluate(i / 8.0)) < 1e-9);
}

TEST_CASE("Spline edit: sampling for drawing and hit tests", "[splines][edit]")
{
	const Shape shape = FromAptChains(MakeAptPavement(), PROJECTION);
	std::vector<Sample> samples;
	SampleChain(shape.chains[0], samples, 0.5);
	REQUIRE(samples.size() > shape.chains[0].nodes.size());
	// A closed chain ends where it started; segments are visited in order
	CHECK(Distance(samples.front().position, samples.back().position) < 1e-9);
	for (size_t i = 1; i < samples.size(); ++i)
		CHECK(samples[i].segment >= samples[i - 1].segment);
	CHECK(samples.back().segment == shape.chains[0].GetSegmentCount() - 1);

	SampleChain(shape.chains[1], samples, 0.5);
	CHECK(samples.size() == 4);		// Three straight segments and the closing point
}

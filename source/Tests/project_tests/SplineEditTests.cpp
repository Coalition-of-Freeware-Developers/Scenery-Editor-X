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

// -------------------------------------------------------
// Node types, bending, B-splines, extending / closing

namespace
{
	Point2 Add(const Point2 &a, const Point2 &b)
	{
		return { a.x + b.x, a.z + b.z };
	}

	Point2 Lerp(const Point2 &a, const Point2 &b, const double t)
	{
		return { a.x + (b.x - a.x) * t, a.z + (b.z - a.z) * t };
	}

	bool Same(const Point2 &a, const Point2 &b)
	{
		return a.x == b.x && a.z == b.z;
	}

	/* A zigzag of five nodes running east (all corners). */
	Shape MakeLine(const bool closed = false)
	{
		const ProjectFile::GeoRing ring = { GeoPoint(44.6000, -123.9400), GeoPoint(44.6003, -123.9390), GeoPoint(44.5999, -123.9380), GeoPoint(44.6004, -123.9371), GeoPoint(44.6001, -123.9360) };
		return FromGeoRings({ ring }, closed, PROJECTION);
	}

	/* First and second derivatives of a segment at its start (atEnd false) or end. */
	Point2 FirstDerivative(const Bezier &b, const bool atEnd)
	{
		if (atEnd)
			return { 3.0 * (b.p[3].x - b.p[2].x), 3.0 * (b.p[3].z - b.p[2].z) };
		return { 3.0 * (b.p[1].x - b.p[0].x), 3.0 * (b.p[1].z - b.p[0].z) };
	}

	Point2 SecondDerivative(const Bezier &b, const bool atEnd)
	{
		if (atEnd)
			return { 6.0 * (b.p[3].x - 2.0 * b.p[2].x + b.p[1].x), 6.0 * (b.p[3].z - 2.0 * b.p[2].z + b.p[1].z) };
		return { 6.0 * (b.p[0].x - 2.0 * b.p[1].x + b.p[2].x), 6.0 * (b.p[0].z - 2.0 * b.p[1].z + b.p[2].z) };
	}

	/* Every joint of the chain is C2: position, first and second derivatives carry on. */
	void RequireC2(const Chain &chain, const double tolerance)
	{
		const uint32_t segments = chain.GetSegmentCount();
		for (uint32_t s = 1; s < segments || (chain.closed && s == segments); ++s)
		{
			INFO("joint " << s);
			const Bezier before = GetSegment(chain, s - 1);
			const Bezier after = GetSegment(chain, s % segments);
			CHECK(Distance(before.p[3], after.p[0]) < tolerance);
			CHECK(Distance(FirstDerivative(before, true), FirstDerivative(after, false)) < tolerance);
			CHECK(Distance(SecondDerivative(before, true), SecondDerivative(after, false)) < tolerance);
		}
	}

	bool HandlesOpposite(const Node &node)
	{
		const double ix = node.handleIn.x - node.position.x, iz = node.handleIn.z - node.position.z;
		const double ox = node.handleOut.x - node.position.x, oz = node.handleOut.z - node.position.z;
		const double lengths = std::hypot(ix, iz) * std::hypot(ox, oz);
		return lengths > 0.0 && std::abs(ix * oz - iz * ox) < 1e-9 * lengths && ix * ox + iz * oz < 0.0;
	}
}

TEST_CASE("Spline edit: node types are read from the handles", "[splines][types]")
{
	Shape shape = MakeLine();
	Node &node = shape.chains[0].nodes[2];
	const Point2 p = node.position;
	CHECK(GetNodeType(node) == NodeType::Corner);

	SetHandle(shape, { 0, 2 }, true, { p.x + 6.0, p.z + 1.0 }, false);
	CHECK(GetNodeType(node) == NodeType::Symmetric);		// A corner grows mirror-image handles

	SetHandle(shape, { 0, 2 }, false, { p.x - 2.0, p.z + 3.0 }, true);
	CHECK(GetNodeType(node) == NodeType::Free);				// Broken on purpose
	CHECK(node.split);

	// Split handles that line up read as smooth (WorldEditor writes them as split pairs when their lengths differ)
	Node lined = node;
	lined.handleIn = { p.x - 3.0, p.z - 0.5 };
	lined.preferred = NodeType::Corner;
	CHECK(GetNodeType(lined) == NodeType::Smooth);
	lined.preferred = NodeType::Free;						// ...unless they were made free
	CHECK(GetNodeType(lined) == NodeType::Free);

	Node one = node;
	one.hasIn = false;
	CHECK(GetNodeType(one) == NodeType::Free);

	CHECK(std::string(GetNodeTypeName(NodeType::Smooth)) == "Smooth");
}

TEST_CASE("Spline edit: setting node types reshapes the handles", "[splines][types]")
{
	Shape shape = MakeLine();
	const Chain &chain = shape.chains[0];
	const Point2 previous = chain.nodes[1].position, p = chain.nodes[2].position, next = chain.nodes[3].position;

	// Smooth from a corner: along the line through the neighbors, a third of the way to each (lengths differ: split)
	SetNodeType(shape, { { 0, 2 } }, NodeType::Smooth);
	const Node &node = chain.nodes[2];
	REQUIRE(GetNodeType(node) == NodeType::Smooth);
	CHECK(HandlesOpposite(node));
	CHECK(Distance(node.handleIn, p) == Approx(Distance(previous, p) / 3.0));
	CHECK(Distance(node.handleOut, p) == Approx(Distance(next, p) / 3.0));
	const double cross = (node.handleOut.x - p.x) * (next.z - previous.z) - (node.handleOut.z - p.z) * (next.x - previous.x);
	CHECK(std::abs(cross) < 1e-9 * Distance(next, previous) * Distance(node.handleOut, p));

	// Dragging one handle of a smooth node turns the other, which keeps its length
	const double inLength = Distance(node.handleIn, p);
	SetHandle(shape, { 0, 2 }, true, { p.x + 4.0, p.z - 4.0 }, false);
	CHECK(GetNodeType(node) == NodeType::Smooth);
	CHECK(HandlesOpposite(node));
	CHECK(Distance(node.handleIn, p) == Approx(inLength));

	// Symmetric evens the lengths out; free keeps everything, but then a handle moves alone
	SetNodeType(shape, { { 0, 2 } }, NodeType::Symmetric);
	CHECK(GetNodeType(node) == NodeType::Symmetric);
	CHECK_FALSE(node.split);
	CHECK(Distance(node.handleIn, p) == Approx(Distance(node.handleOut, p)));
	const Point2 out = node.handleOut;
	SetNodeType(shape, { { 0, 2 } }, NodeType::Free);
	CHECK(GetNodeType(node) == NodeType::Free);
	CHECK(Same(node.handleOut, out));
	const Point2 in = node.handleIn;
	SetHandle(shape, { 0, 2 }, true, { p.x + 1.0, p.z + 5.0 }, false);
	CHECK(Same(node.handleIn, in));
	CHECK(node.split);

	// A smooth node with mirror-image handles stays one apt.dat row until its lengths differ
	SetNodeType(shape, { { 0, 2 } }, NodeType::Symmetric);
	SetNodeType(shape, { { 0, 2 } }, NodeType::Smooth);
	CHECK(GetNodeType(node) == NodeType::Smooth);
	CHECK_FALSE(node.split);

	// Corner removes the handles; an open chain's end gets one along its segment
	SetNodeType(shape, { { 0, 2 } }, NodeType::Corner);
	CHECK(GetNodeType(node) == NodeType::Corner);
	SetNodeType(shape, { { 0, 0 } }, NodeType::Smooth);
	CHECK(chain.nodes[0].hasOut);
	CHECK(Distance(chain.nodes[0].handleOut, Lerp(chain.nodes[0].position, chain.nodes[1].position, 1.0 / 3.0)) < 1e-9);
}

TEST_CASE("Spline edit: bending a segment passes the curve through the point", "[splines][bend]")
{
	Shape shape = MakeLine();
	const Chain before = shape.chains[0];

	// A straight segment between corners becomes a curve; its neighbors stay straight
	const Bezier straight = GetSegment(before, 1);
	REQUIRE(straight.IsStraight());
	for (const double t : { 0.1, 0.35, 0.5, 0.8 })
	{
		INFO("t " << t);
		Shape bent = shape;
		const Point2 target = Add(straight.Evaluate(t), { 3.0, -2.0 });
		REQUIRE(BendSegment(bent, 0, 1, t, target));
		CHECK(Distance(GetSegment(bent.chains[0], 1).Evaluate(t), target) < 1e-9);
		CHECK(GetSegment(bent.chains[0], 0).IsStraight());
		CHECK(GetSegment(bent.chains[0], 2).IsStraight());
		CHECK(GetNodeType(bent.chains[0].nodes[1]) == NodeType::Free);	// One handle
		// The nearer handle does more of the work (near an end, all of it)
		const Node &start = bent.chains[0].nodes[1];
		const Node &end = bent.chains[0].nodes[2];
		const double startReach = start.hasOut ? Distance(start.handleOut, start.position) : 0.0;
		const double endReach = end.hasIn ? Distance(end.handleIn, end.position) : 0.0;
		if (t < 0.4)
			CHECK(startReach > endReach);
		if (t > 0.6)
			CHECK(endReach > startReach);
	}

	// Smooth nodes stay smooth: the neighboring segments bend along
	SetNodeType(shape, AllNodes(shape), NodeType::Smooth);
	const Bezier curve = GetSegment(shape.chains[0], 2);
	const Point2 target = Add(curve.Evaluate(0.5), { -1.5, 2.5 });
	REQUIRE(BendSegment(shape, 0, 2, 0.5, target));
	CHECK(Distance(GetSegment(shape.chains[0], 2).Evaluate(0.5), target) < 1e-9);
	CHECK(GetNodeType(shape.chains[0].nodes[2]) == NodeType::Smooth);
	CHECK(GetNodeType(shape.chains[0].nodes[3]) == NodeType::Smooth);
	CHECK(HandlesOpposite(shape.chains[0].nodes[2]));

	CHECK_FALSE(BendSegment(shape, 0, 4, 0.5, target));		// An open chain of 5 has 4 segments
}

TEST_CASE("Spline edit: straightening segments", "[splines][edit]")
{
	Shape shape = MakeLine();
	SetNodeType(shape, AllNodes(shape), NodeType::Symmetric);
	CHECK(StraightenSegments(shape, { { 0, 1 }, { 0, 2 } }) == 1);
	CHECK(GetSegment(shape.chains[0], 1).IsStraight());
	CHECK_FALSE(GetSegment(shape.chains[0], 0).IsStraight());
	CHECK_FALSE(GetSegment(shape.chains[0], 2).IsStraight());
	CHECK(GetNodeType(shape.chains[0].nodes[1]) == NodeType::Free);	// Its incoming handle stays
	CHECK(StraightenSegments(shape, { { 0, 1 }, { 0, 2 } }) == 0);
	CHECK(StraightenSegments(shape, { { 0, 0 } }) == 0);
}

TEST_CASE("Spline edit: a chain becomes a B-spline and back", "[splines][bspline]")
{
	for (const bool closed : { false, true })
	{
		INFO((closed ? "closed" : "open"));
		Shape shape = MakeLine(closed);
		const Chain corners = shape.chains[0];
		Chain &chain = shape.chains[0];

		// The nodes become the control points: the curve is C2 and runs inside them
		REQUIRE(BeginBSpline(chain, BSplineFit::NodesAsControls));
		REQUIRE(chain.IsBSpline());
		REQUIRE(chain.controls.size() == corners.nodes.size());
		for (size_t i = 0; i < corners.nodes.size(); ++i)
			CHECK(Same(chain.controls[i], corners.nodes[i].position));
		RequireC2(chain, 1e-9);
		for (const Node &node : chain.nodes)
		{
			CHECK_FALSE(node.split);
			CHECK(GetNodeType(node) == NodeType::Symmetric);
		}
		if (!closed)
		{
			CHECK(Same(chain.nodes.front().position, corners.nodes.front().position));
			CHECK(Same(chain.nodes.back().position, corners.nodes.back().position));
		}

		// Written to a project and to apt.dat and read back, the control polygon comes back
		for (const bool apt : { false, true })
		{
			INFO((apt ? "apt.dat" : "project"));
			const Shape reread = apt ? FromAptChains(ToAptChains(shape, PROJECTION), PROJECTION) : FromGeoRings(ToGeoRings(shape, PROJECTION), closed, PROJECTION);
			REQUIRE(reread.chains[0].nodes.size() == chain.nodes.size());
			const std::optional<std::vector<Point2>> recovered = RecoverBSpline(reread.chains[0]);
			REQUIRE(recovered.has_value());
			for (size_t i = 0; i < recovered->size(); ++i)
				CHECK(Distance((*recovered)[i], chain.controls[i]) < 1e-6);
			Chain again = reread.chains[0];
			CHECK(BeginBSpline(again, BSplineFit::NodesAsControls, true));
		}

		// Plain beziers aren't B-splines; recovering only leaves them alone
		Chain plain = corners;
		CHECK_FALSE(RecoverBSpline(plain).has_value());
		CHECK_FALSE(BeginBSpline(plain, BSplineFit::NodesAsControls, true));
		CHECK_FALSE(plain.IsBSpline());

		// Ending keeps the curve as it is
		const Chain spline = chain;
		EndBSpline(chain);
		CHECK_FALSE(chain.IsBSpline());
		for (uint32_t s = 0; s < chain.GetSegmentCount(); ++s)
			CHECK(Distance(GetSegment(chain, s).Evaluate(0.3), GetSegment(spline, s).Evaluate(0.3)) == 0.0);
	}
}

TEST_CASE("Spline edit: an interpolating B-spline passes through the nodes", "[splines][bspline]")
{
	for (const bool closed : { false, true })
	{
		INFO((closed ? "closed" : "open"));
		Shape shape = MakeLine(closed);
		const Chain corners = shape.chains[0];
		Chain &chain = shape.chains[0];
		REQUIRE(BeginBSpline(chain, BSplineFit::ThroughNodes));
		for (size_t i = 0; i < corners.nodes.size(); ++i)
			CHECK(Distance(chain.nodes[i].position, corners.nodes[i].position) < 1e-6);
		RequireC2(chain, 1e-9);
		if (!closed)
		{
			// Natural ends: no bend at the first node
			const Point2 bend = SecondDerivative(GetSegment(chain, 0), false);
			CHECK(std::hypot(bend.x, bend.z) < 1e-9);
		}
	}
}

TEST_CASE("Spline edit: moving a B-spline control point changes the curve nearby only", "[splines][bspline]")
{
	// A longer line, so some nodes sit far from the moved control point
	ProjectFile::GeoRing ring;
	for (int i = 0; i < 9; ++i)
		ring.push_back(GeoPoint(44.6000 + 0.0002 * ((i * 7) % 3), -123.9400 + 0.0008 * i));
	Shape shape = FromGeoRings({ ring }, false, PROJECTION);
	REQUIRE(BeginBSpline(shape.chains[0]));
	const std::vector<ProjectFile::GeoRing> before = ToGeoRings(shape, PROJECTION);

	// Written out and read back, then edited: the nodes that don't depend on control 4 write back bit for bit
	Shape reread = FromGeoRings(before, false, PROJECTION);
	REQUIRE(BeginBSpline(reread.chains[0], BSplineFit::NodesAsControls, true));
	TransformNodes(reread, { { 0, 4 } }, Affine2::Translation(5.0, -3.0));
	CHECK(Distance(reread.chains[0].controls[4], Add(shape.chains[0].controls[4], { 5.0, -3.0 })) < 1e-6);
	RequireC2(reread.chains[0], 1e-6);
	const std::vector<ProjectFile::GeoRing> after = ToGeoRings(reread, PROJECTION);
	for (size_t i = 0; i < ring.size(); ++i)
	{
		INFO("node " << i);
		if (i >= 3 && i <= 5)
			CHECK_FALSE(after[0][i] == before[0][i]);
		else
			CHECK(after[0][i] == before[0][i]);
	}

	// Inserting and deleting control points keeps the nodes and control points paired
	NodeRef inserted;
	REQUIRE(InsertNode(reread, 0, 2, 0.5, &inserted));
	CHECK(inserted.node == 3);
	CHECK(reread.chains[0].controls.size() == reread.chains[0].nodes.size());
	CHECK(reread.chains[0].nodes.size() == ring.size() + 1);
	RequireC2(reread.chains[0], 1e-9);
	CHECK(DeleteNodes(reread, { { 0, 0 }, { 0, 3 } }) == 2);
	CHECK(reread.chains[0].controls.size() == reread.chains[0].nodes.size());
	CHECK(Same(reread.chains[0].nodes[0].position, reread.chains[0].controls[0]));
	RequireC2(reread.chains[0], 1e-9);

	// Handle edits don't apply to a B-spline
	const Chain spline = reread.chains[0];
	SetHandle(reread, { 0, 2 }, true, { 0.0, 0.0 }, false);
	SetNodeType(reread, { { 0, 2 } }, NodeType::Corner);
	CHECK(Same(reread.chains[0].nodes[2].handleOut, spline.nodes[2].handleOut));
}

TEST_CASE("Spline edit: dragging a B-spline's curve", "[splines][bspline]")
{
	for (const bool closed : { false, true })
	{
		INFO((closed ? "closed" : "open"));
		for (const uint32_t controls : { 2u, 3u, 6u })
		{
			if (closed && controls < 3)
				continue;
			for (uint32_t segment = 0; segment < (closed ? controls : controls - 1); ++segment)
			{
				double sum = 0.0;
				for (const auto &[index, weight] : GetBSplineWeights(controls, closed, segment, 0.37))
				{
					CHECK(index < controls);
					sum += weight;
				}
				CHECK(sum == Approx(1.0));
			}
		}

		Shape shape = MakeLine(closed);
		REQUIRE(BeginBSpline(shape.chains[0]));
		for (uint32_t segment = 0; segment < shape.chains[0].GetSegmentCount(); ++segment)
		{
			INFO("segment " << segment);
			Shape dragged = shape;
			const Point2 target = Add(GetSegment(dragged.chains[0], segment).Evaluate(0.6), { 2.0, 4.0 });
			REQUIRE(BendSegment(dragged, 0, segment, 0.6, target));
			CHECK(Distance(GetSegment(dragged.chains[0], segment).Evaluate(0.6), target) < 1e-9);
			RequireC2(dragged.chains[0], 1e-9);
		}
	}
}

TEST_CASE("Spline edit: extending, closing and opening chains", "[splines][chains]")
{
	XPlane::AptNodeChain line;
	line.closed = false;
	line.nodes = { AptPoint(44.600, -123.940, 1, 101), AptPoint(44.601, -123.939, 2, 0), AptPoint(44.602, -123.938) };
	Shape shape = FromAptChains({ line }, PROJECTION);
	Chain &chain = shape.chains[0];

	// Extending the end: the old last node starts a segment in the style of the one before it
	NodeRef added;
	const Point2 beyond = Add(chain.nodes.back().position, { 30.0, 5.0 });
	REQUIRE(AppendNode(shape, 0, false, beyond, &added));
	CHECK(added.node == 3);
	CHECK(Same(chain.nodes[3].position, beyond));
	CHECK(chain.nodes[2].attributes.lineType == 2);
	REQUIRE(AppendNode(shape, 0, true, Add(chain.nodes.front().position, { -30.0, 0.0 }), &added));
	CHECK(added.node == 0);
	CHECK(chain.nodes[0].attributes.lineType == 1);
	CHECK(chain.nodes.size() == 5);

	// Closing adds the segment back to the first node (one row per node, the ring ends with 113)
	REQUIRE(CloseChain(shape, 0));
	CHECK(chain.closed);
	CHECK_FALSE(CloseChain(shape, 0));
	CHECK_FALSE(AppendNode(shape, 0, false, beyond));
	std::vector<XPlane::AptNodeChain> written = ToAptChains(shape, PROJECTION);
	CHECK(written[0].closed);
	CHECK(written[0].nodes.size() == 5);

	// Opening at a node: it becomes both ends, and the curve keeps its shape
	SetNodeType(shape, { { 0, 2 } }, NodeType::Smooth);
	const Chain ring = chain;
	REQUIRE(OpenChain(shape, 0, 2));
	CHECK_FALSE(chain.closed);
	REQUIRE(chain.nodes.size() == ring.nodes.size() + 1);
	CHECK(Same(chain.nodes.front().position, ring.nodes[2].position));
	CHECK(Same(chain.nodes.back().position, ring.nodes[2].position));
	CHECK_FALSE(chain.nodes.front().hasIn);
	CHECK(chain.nodes.front().hasOut);
	CHECK(chain.nodes.back().hasIn);
	for (uint32_t s = 0; s < chain.GetSegmentCount(); ++s)
	{
		const Bezier a = GetSegment(chain, s);
		const Bezier b = GetSegment(ring, (s + 2) % ring.GetSegmentCount());
		for (int i = 0; i <= 4; ++i)
			CHECK(Distance(a.Evaluate(i / 4.0), b.Evaluate(i / 4.0)) < 1e-9);
	}
	// Its ends are single apt.dat rows (no split pairs)
	written = ToAptChains(shape, PROJECTION);
	CHECK_FALSE(written[0].closed);
	CHECK(written[0].nodes.size() == chain.nodes.size());

	// Closing two ends that sit on one spot merges them back into one node
	REQUIRE(CloseChain(shape, 0));
	CHECK(chain.nodes.size() == ring.nodes.size());
	CHECK(HandlesOpposite(chain.nodes[0]));

	// A two-node line can't close
	Shape pair = FromGeoRings({ { GeoPoint(44.60, -123.94), GeoPoint(44.60, -123.93) } }, false, PROJECTION);
	CHECK_FALSE(CloseChain(pair, 0));
}

TEST_CASE("Spline edit: lengths and the tightest bend", "[splines][measure]")
{
	// A 100 m square
	Chain square;
	square.closed = true;
	for (const Point2 &p : { Point2{ 0.0, 0.0 }, Point2{ 100.0, 0.0 }, Point2{ 100.0, 100.0 }, Point2{ 0.0, 100.0 } })
	{
		Node node;
		node.position = p;
		square.nodes.push_back(node);
	}
	CHECK(GetChainLength(square) == Approx(400.0));
	CHECK(std::isinf(GetMinimumRadius(square)));

	// A quarter circle of radius 50 as one bezier (handles 0.5523 r)
	constexpr double r = 50.0, k = 0.5522847498 * r;
	Chain arc;
	arc.closed = false;
	Node a, b;
	a.position = { r, 0.0 };
	a.handleOut = { r, k };
	a.hasOut = true;
	b.position = { 0.0, r };
	b.handleIn = { k, r };
	b.hasIn = true;
	arc.nodes = { a, b };
	CHECK(GetChainLength(arc) == Approx(std::acos(-1.0) * r / 2.0).epsilon(5e-4));
	uint32_t segment = 9;
	const double radius = GetMinimumRadius(arc, &segment);
	CHECK(radius == Approx(r).epsilon(0.02));
	CHECK(segment == 0);

	// A handle on one side only: the curvature spike at the bare node isn't a bend
	Chain gentle;
	gentle.closed = false;
	Node start, end;
	start.position = { 0.0, 0.0 };
	end.position = { 100.0, 0.0 };
	end.handleIn = { 60.0, 15.0 };
	end.hasIn = true;
	gentle.nodes = { start, end };
	CHECK(GetMinimumRadius(gentle) > 20.0);
}

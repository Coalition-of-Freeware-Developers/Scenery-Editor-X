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
 * facade_edit.cpp
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#include "facade_edit.h"
#include <algorithm>
#include <cmath>

// -------------------------------------------------------

namespace SceneryEditorX::FacadeEdit
{
	using Splines::Bezier;
	using Splines::Chain;
	using Splines::Node;

	namespace
	{
		constexpr double PI = 3.14159265358979323846;

		double Distance(const Point2 &a, const Point2 &b)
		{
			return std::hypot(b.x - a.x, b.z - a.z);
		}

		Point2 Direction(const Point2 &a, const Point2 &b)
		{
			const double length = Distance(a, b);
			return length > 1e-12 ? Point2{ (b.x - a.x) / length, (b.z - a.z) / length } : Point2{ 1.0, 0.0 };
		}

		Node CornerNode(const Point2 &position, const Splines::NodeAttributes &attributes)
		{
			Node node;
			node.position = position;
			node.attributes = attributes;
			return node;
		}

		/* @brief Arc length along a curve at evenly spaced parameters, to place points at even distances. */
		struct ArcTable
		{
			std::vector<double> length;		// At t = i / (size - 1)
		};

		ArcTable Measure(const Bezier &bezier, const int steps = 512)
		{
			ArcTable table;
			table.length.reserve(static_cast<size_t>(steps) + 1);
			Point2 previous = bezier.p[0];
			double total = 0.0;
			table.length.push_back(0.0);
			for (int i = 1; i <= steps; ++i)
			{
				const Point2 point = bezier.Evaluate(static_cast<double>(i) / steps);
				total += Distance(previous, point);
				table.length.push_back(total);
				previous = point;
			}
			return table;
		}

		double ParameterAt(const ArcTable &table, const double distance)
		{
			const auto it = std::lower_bound(table.length.begin(), table.length.end(), distance);
			if (it == table.length.begin())
				return 0.0;
			if (it == table.length.end())
				return 1.0;
			const size_t i = static_cast<size_t>(it - table.length.begin());
			const double span = table.length[i] - table.length[i - 1];
			const double fraction = span > 1e-12 ? (distance - table.length[i - 1]) / span : 0.0;
			return (static_cast<double>(i - 1) + fraction) / static_cast<double>(table.length.size() - 1);
		}

		/* @brief The flattened outline: points and the segment each starts (closed chains don't repeat their first point). */
		void Flatten(const Chain &chain, const double step, std::vector<Point2> &points, std::vector<uint32_t> &segments)
		{
			std::vector<Splines::Sample> samples;
			Splines::SampleChain(chain, samples, step);
			if (chain.closed && samples.size() > 1)
				samples.pop_back();
			for (const Splines::Sample &sample : samples)
			{
				if (!points.empty() && Distance(points.back(), sample.position) < 1e-6)
					continue;
				points.push_back(sample.position);
				segments.push_back(sample.segment);
			}
			if (chain.closed && points.size() > 1 && Distance(points.front(), points.back()) < 1e-6)
			{
				points.pop_back();
				segments.pop_back();
			}
		}

		/* @brief Where two lines (point + unit direction) cross; false when they are (nearly) parallel. */
		bool Intersect(const Point2 &p1, const Point2 &d1, const Point2 &p2, const Point2 &d2, Point2 &out)
		{
			const double cross = d1.x * d2.z - d1.z * d2.x;
			if (std::abs(cross) < 1e-6)
				return false;
			const double t = ((p2.x - p1.x) * d2.z - (p2.z - p1.z) * d2.x) / cross;
			out = { p1.x + d1.x * t, p1.z + d1.z * t };
			return true;
		}

		Point2 Project(const Point2 &point, const Point2 &linePoint, const Point2 &direction)
		{
			const double t = (point.x - linePoint.x) * direction.x + (point.z - linePoint.z) * direction.z;
			return { linePoint.x + direction.x * t, linePoint.z + direction.z * t };
		}
	}

	// -------------------------------------------------------

	std::vector<WallInfo> GetWalls(const Chain &chain)
	{
		std::vector<WallInfo> walls;
		const uint32_t segments = chain.nodes.size() < 2 ? 0 : chain.GetSegmentCount();
		walls.reserve(segments);
		for (uint32_t s = 0; s < segments; ++s)
		{
			const Bezier bezier = Splines::GetSegment(chain, s);
			WallInfo wall;
			wall.index = s;
			wall.start = bezier.p[0];
			wall.end = bezier.p[3];
			wall.curved = !bezier.IsStraight();
			wall.length = wall.curved ? Splines::GetSegmentLength(bezier) : Distance(wall.start, wall.end);
			wall.heading = XPlane::DirectionToHeading(XPlane::GetFacadeOutward(Direction(wall.start, wall.end)));
			wall.wallType = chain.nodes[s].attributes.wallType;
			walls.push_back(wall);
		}
		return walls;
	}

	std::vector<XPlane::FacadeSideInput> ToSideInputs(const std::vector<WallInfo> &walls)
	{
		std::vector<XPlane::FacadeSideInput> sides;
		sides.reserve(walls.size());
		for (const WallInfo &wall : walls)
			sides.push_back({ wall.length, wall.heading, wall.wallType });
		return sides;
	}

	double GetSignedArea(const Chain &chain)
	{
		if (!chain.closed)
			return 0.0;
		std::vector<Point2> points;
		std::vector<uint32_t> segments;
		Flatten(chain, 0.5, points, segments);
		return points.size() < 3 ? 0.0 : XPlane::SignedArea(points);
	}

	double GetPerimeter(const Chain &chain)
	{
		double length = 0.0;
		for (const WallInfo &wall : GetWalls(chain))
			length += wall.length;
		return length;
	}

	bool SubdivideCurves(Chain &chain, double pieceLength, size_t *added)
	{
		if (added)
			*added = 0;
		const size_t count = chain.nodes.size();
		if (count < 2)
			return false;
		pieceLength = std::max(0.05, pieceLength);
		const uint32_t segments = chain.GetSegmentCount();
		std::vector<bool> curved(segments, false);
		bool any = false;
		for (uint32_t s = 0; s < segments; ++s)
		{
			curved[s] = !Splines::GetSegment(chain, s).IsStraight();
			any |= curved[s];
		}
		if (!any)
			return false;

		std::vector<Node> nodes;
		size_t inserted = 0;
		for (size_t i = 0; i < count; ++i)
		{
			Node node = chain.nodes[i];
			const bool outCurved = i < segments && curved[i];
			const bool inCurved = i > 0 ? curved[i - 1] : (chain.closed && curved[segments - 1]);
			if (outCurved)
				node.hasOut = false;
			if (inCurved)
				node.hasIn = false;
			if (node.hasIn != node.hasOut)
				node.split = true;		// The handle left no longer mirrors the one removed
			else if (!node.hasIn)
				node.split = false;
			nodes.push_back(node);

			if (!outCurved)
				continue;
			// Even pieces along the curve, about pieceLength each
			const Bezier bezier = Splines::GetSegment(chain, static_cast<uint32_t>(i));
			const ArcTable table = Measure(bezier);
			const double length = table.length.back();
			const int pieces = std::max(1, static_cast<int>(std::lround(length / pieceLength)));
			for (int k = 1; k < pieces; ++k)
			{
				const double t = ParameterAt(table, length * k / pieces);
				nodes.push_back(CornerNode(bezier.Evaluate(t), chain.nodes[i].attributes));
				++inserted;
			}
		}
		chain.nodes = std::move(nodes);
		chain.controls.clear();
		chain.wrapsSplit = false;
		if (added)
			*added = inserted;
		return true;
	}

	bool SquareCorners(Chain &chain, double toleranceDegrees, std::string *why)
	{
		const auto fail = [why](const char *reason)
		{
			if (why)
				*why = reason;
			return false;
		};
		const size_t count = chain.nodes.size();
		if (count < 3)
			return fail("Needs at least three nodes");
		const uint32_t segments = chain.GetSegmentCount();
		for (uint32_t s = 0; s < segments; ++s)
		{
			if (!Splines::GetSegment(chain, s).IsStraight())
				return fail("The footprint has curved walls: subdivide or straighten them first");
		}

		// The main axes: wall directions folded onto a quarter turn (x4), averaged by length
		const auto pointOf = [&](const uint32_t node) { return chain.nodes[node % count].position; };
		double c = 0.0, s4 = 0.0;
		for (uint32_t s = 0; s < segments; ++s)
		{
			const Point2 a = pointOf(s), b = pointOf(s + 1);
			const double length = Distance(a, b);
			if (length < 1e-6)
				continue;
			const double angle = std::atan2(b.z - a.z, b.x - a.x);
			c += length * std::cos(angle * 4.0);
			s4 += length * std::sin(angle * 4.0);
		}
		if (std::abs(c) < 1e-9 && std::abs(s4) < 1e-9)
			return fail("No main direction to square to");
		const double axis = std::atan2(s4, c) / 4.0;
		const double tolerance = std::clamp(toleranceDegrees, 0.0, 45.0) * PI / 180.0;

		// Each wall as a line through its middle, turned onto the axes when it is close to them
		std::vector<Point2> middles(segments), directions(segments);
		bool turned = false;
		for (uint32_t s = 0; s < segments; ++s)
		{
			const Point2 a = pointOf(s), b = pointOf(s + 1);
			double angle = std::atan2(b.z - a.z, b.x - a.x);
			const double off = std::remainder(angle - axis, PI / 2.0);
			if (std::abs(off) <= tolerance && Distance(a, b) > 1e-6)
			{
				angle -= off;
				turned |= std::abs(off) > 1e-9;
			}
			middles[s] = { (a.x + b.x) * 0.5, (a.z + b.z) * 0.5 };
			directions[s] = { std::cos(angle), std::sin(angle) };
		}
		if (!turned)
			return fail("Already square");

		// Corners where their walls meet (parallel neighbors: the corner slides onto the wall)
		std::vector<Point2> positions(count);
		for (size_t i = 0; i < count; ++i)
		{
			const Point2 &old = chain.nodes[i].position;
			if (!chain.closed && i == 0)
				positions[i] = Project(old, middles[0], directions[0]);
			else if (!chain.closed && i == count - 1)
				positions[i] = Project(old, middles[segments - 1], directions[segments - 1]);
			else
			{
				const uint32_t in = static_cast<uint32_t>((i + segments - 1) % segments), out = static_cast<uint32_t>(i % segments);
				if (!Intersect(middles[in], directions[in], middles[out], directions[out], positions[i]))
				{
					const Point2 a = Project(old, middles[in], directions[in]), b = Project(old, middles[out], directions[out]);
					positions[i] = { (a.x + b.x) * 0.5, (a.z + b.z) * 0.5 };
				}
			}
		}

		bool moved = false;
		for (size_t i = 0; i < count; ++i)
		{
			if (Distance(positions[i], chain.nodes[i].position) > 1e-4)
			{
				chain.nodes[i].position = positions[i];
				moved = true;
			}
		}
		chain.controls.clear();
		return moved ? true : fail("Already square");
	}

	bool SetFrontWall(Chain &chain, const uint32_t wall)
	{
		if (!chain.closed || wall == 0 || wall >= chain.nodes.size())
			return false;
		std::rotate(chain.nodes.begin(), chain.nodes.begin() + wall, chain.nodes.end());
		if (chain.controls.size() > wall)
			std::rotate(chain.controls.begin(), chain.controls.begin() + wall, chain.controls.end());
		chain.wrapsSplit = false;
		return true;
	}

	bool MakeCounterClockwise(Chain &chain)
	{
		if (!chain.closed || chain.nodes.size() < 3 || GetSignedArea(chain) >= 0.0)
			return false;
		Splines::ReverseChain(chain);
		return true;
	}

	bool SetWallTypes(Chain &chain, const std::vector<uint32_t> &walls, const int type)
	{
		const uint32_t segments = chain.nodes.size() < 2 ? 0 : chain.GetSegmentCount();
		bool changed = false;
		const auto set = [&](const uint32_t wall)
		{
			if (wall < segments && chain.nodes[wall].attributes.wallType != type)
			{
				chain.nodes[wall].attributes.wallType = type;
				changed = true;
			}
		};
		if (walls.empty())
		{
			for (uint32_t s = 0; s < segments; ++s)
				set(s);
		}
		for (const uint32_t wall : walls)
			set(wall);
		return changed;
	}

	Chain Offset(const Chain &chain, const double distance, const double curveStep)
	{
		Chain out;
		out.closed = chain.closed;
		std::vector<Point2> points;
		std::vector<uint32_t> segments;
		Flatten(chain, std::max(0.1, curveStep), points, segments);
		const size_t count = points.size();
		if (count < 2 || (chain.closed && count < 3))
			return out;

		// Outward: right of travel on a counter-clockwise ring (and on an open chain); a clockwise ring's inside is there
		const double side = chain.closed && XPlane::SignedArea(points) < 0.0 ? -1.0 : 1.0;
		const size_t edges = chain.closed ? count : count - 1;
		std::vector<Point2> normals(edges);
		for (size_t e = 0; e < edges; ++e)
		{
			const Point2 outward = XPlane::GetFacadeOutward(Direction(points[e], points[(e + 1) % count]));
			normals[e] = { outward.x * side, outward.z * side };
		}

		const double limit = std::abs(distance) * 4.0;
		for (size_t i = 0; i < count; ++i)
		{
			Point2 position;
			const bool start = !chain.closed && i == 0, end = !chain.closed && i == count - 1;
			if (start || end)
			{
				const Point2 &normal = normals[start ? 0 : edges - 1];
				position = { points[i].x + normal.x * distance, points[i].z + normal.z * distance };
			}
			else
			{
				const Point2 &before = normals[(i + edges - 1) % edges], &after = normals[i % edges];
				Point2 miter{ before.x + after.x, before.z + after.z };
				const double length = std::hypot(miter.x, miter.z);
				miter = length > 1e-9 ? Point2{ miter.x / length, miter.z / length } : after;
				const double cosine = miter.x * after.x + miter.z * after.z;
				double reach = cosine > 1e-6 ? distance / cosine : distance;
				reach = std::clamp(reach, -limit, limit);
				position = { points[i].x + miter.x * reach, points[i].z + miter.z * reach };
			}
			const uint32_t segment = std::min<uint32_t>(segments[i], static_cast<uint32_t>(chain.nodes.size() - 1));
			out.nodes.push_back(CornerNode(position, chain.nodes[segment].attributes));
		}
		return out;
	}

	Chain MakeRectangle(const Point2 &a, const Point2 &b, const double depth)
	{
		const Point2 direction = Direction(a, b);
		const Point2 left{ direction.z, -direction.x };
		const Point2 a2{ a.x + left.x * depth, a.z + left.z * depth };
		const Point2 b2{ b.x + left.x * depth, b.z + left.z * depth };
		// Inside to the left of a->b: a, b, then back along the far side; inside to the right: the front runs b->a
		return depth >= 0.0 ? MakeChain({ a, b, b2, a2 }, true) : MakeChain({ b, a, a2, b2 }, true);
	}

	Chain MakeCircle(const Point2 &center, const double radius, int sides)
	{
		sides = std::max(3, sides);
		std::vector<Point2> points;
		points.reserve(static_cast<size_t>(sides));
		// Counter-clockwise (east / north), starting so the first wall's middle is due south
		const double step = 2.0 * PI / sides;
		for (int k = 0; k < sides; ++k)
		{
			const double angle = -PI / 2.0 - step / 2.0 + step * k;
			points.push_back({ center.x + radius * std::cos(angle), center.z - radius * std::sin(angle) });
		}
		return MakeChain(points, true);
	}

	Chain MakeChain(const std::vector<Point2> &points, const bool closed)
	{
		Chain chain;
		chain.closed = closed;
		std::vector<Point2> ordered = points;
		if (closed && ordered.size() >= 3 && XPlane::SignedArea(ordered) < 0.0)
			std::reverse(ordered.begin(), ordered.end());
		for (const Point2 &point : ordered)
			chain.nodes.push_back(CornerNode(point, {}));
		return chain;
	}

	int GetCircleSides(const double radius, const double wallLength)
	{
		const double perimeter = 2.0 * PI * std::max(0.0, radius);
		return std::clamp(static_cast<int>(std::lround(perimeter / std::max(0.1, wallLength))), 6, 128);
	}

	int RemapWallType(const int type, const std::vector<std::string> &from, const std::vector<std::string> &to)
	{
		if (to.empty())
			return 0;
		if (type >= 0 && static_cast<size_t>(type) < from.size())
		{
			const auto it = std::ranges::find(to, from[static_cast<size_t>(type)]);
			if (it != to.end())
				return static_cast<int>(it - to.begin());
		}
		return std::clamp(type, 0, static_cast<int>(to.size()) - 1);
	}
}

// -------------------------------------------------------

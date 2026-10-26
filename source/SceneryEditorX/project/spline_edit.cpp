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
 * spline_edit.cpp
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#include "spline_edit.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <set>

// -------------------------------------------------------

namespace SceneryEditorX::Splines
{
	namespace
	{
		constexpr double COLOCATED_METERS = 1e-4;	// apt.dat nodes this close are one (split) node
		constexpr double HANDLE_EPSILON = 1e-6;		// A handle this close to its node is no handle
		constexpr double ALIGN_SINE = 2e-3;			// Handles within ~0.1 degrees of opposite line up (rounding in files)
		constexpr double MERGE_METERS = 1e-3;		// Closing a chain whose ends sit this close merges them

		double Distance(const Point2 &a, const Point2 &b)
		{
			return std::hypot(a.x - b.x, a.z - b.z);
		}

		Point2 Lerp(const Point2 &a, const Point2 &b, const double t)
		{
			return { a.x + (b.x - a.x) * t, a.z + (b.z - a.z) * t };
		}

		Point2 Mirror(const Point2 &handle, const Point2 &about)
		{
			return { 2.0 * about.x - handle.x, 2.0 * about.z - handle.z };
		}

		bool Same(const Point2 &a, const Point2 &b)
		{
			return a.x == b.x && a.z == b.z;
		}

		Point2 Add(const Point2 &a, const Point2 &b)
		{
			return { a.x + b.x, a.z + b.z };
		}

		Point2 Sub(const Point2 &a, const Point2 &b)
		{
			return { a.x - b.x, a.z - b.z };
		}

		Point2 Scale(const Point2 &a, const double s)
		{
			return { a.x * s, a.z * s };
		}

		double Length(const Point2 &a)
		{
			return std::hypot(a.x, a.z);
		}

		/* @brief Unit vector along a (nothing when a is too short to have a direction). */
		std::optional<Point2> Direction(const Point2 &a)
		{
			const double length = Length(a);
			if (length < HANDLE_EPSILON)
				return std::nullopt;
			return Point2{ a.x / length, a.z / length };
		}

		/* @brief Both handles present and pointing (nearly) opposite ways. */
		bool HandlesLineUp(const Node &node)
		{
			if (!node.hasIn || !node.hasOut)
				return false;
			const Point2 in = Sub(node.handleIn, node.position);
			const Point2 out = Sub(node.handleOut, node.position);
			const double lengths = Length(in) * Length(out);
			if (lengths < HANDLE_EPSILON * HANDLE_EPSILON)
				return false;
			const double cross = in.x * out.z - in.z * out.x;
			const double dot = in.x * out.x + in.z * out.z;
			return dot < 0.0 && std::abs(cross) <= ALIGN_SINE * lengths;
		}

		/* @brief Lengths equal enough that the handles count as mirror images (relative, so it holds at any scale). */
		bool EqualLengths(const double a, const double b)
		{
			return std::abs(a - b) <= 1e-7 * std::max({ 1.0, a, b });
		}

		SourcePoint MakeSource(const Point2 &local, const double latitude, const double longitude)
		{
			SourcePoint source;
			source.local = local;
			source.latitude = latitude;
			source.longitude = longitude;
			source.valid = true;
			return source;
		}

		/* @brief Geographic position of a local point: the source's own coordinates when the point didn't move. */
		void ToGeo(const GeoProjection &projection, const Point2 &point, const SourcePoint &source, double &latitude, double &longitude)
		{
			if (source.valid && Same(source.local, point))
			{
				latitude = source.latitude;
				longitude = source.longitude;
				return;
			}
			projection.ToGeo(point, latitude, longitude);
		}

		void CopyAptAttributes(const XPlane::AptNode &node, NodeAttributes &attributes, const bool incoming)
		{
			if (incoming)
			{
				attributes.lineTypeIn = node.lineType;
				attributes.lightTypeIn = node.lightType;
			}
			else
			{
				attributes.lineType = node.lineType;
				attributes.lightType = node.lightType;
			}
		}

		uint32_t Previous(const Chain &chain, const uint32_t index, bool &valid)
		{
			const uint32_t count = static_cast<uint32_t>(chain.nodes.size());
			valid = index > 0 || (chain.closed && count > 2);
			return index > 0 ? index - 1 : count - 1;
		}

		uint32_t Next(const Chain &chain, const uint32_t index, bool &valid)
		{
			const uint32_t count = static_cast<uint32_t>(chain.nodes.size());
			valid = index + 1 < count || (chain.closed && count > 2);
			return index + 1 < count ? index + 1 : 0;
		}

		/* @brief After one handle of an unsplit node changed, keeps the node mirrored or marks it split when it can't be. */
		void Resymmetrize(Node &node)
		{
			if (node.split || !node.hasIn || !node.hasOut)
				return;
			const double in = Distance(node.handleIn, node.position);
			const double out = Distance(node.handleOut, node.position);
			const Point2 mirrored = Mirror(node.handleOut, node.position);
			if (!EqualLengths(in, out) || Distance(mirrored, node.handleIn) > 1e-7 * std::max(1.0, out))
				node.split = true;
		}

		/* @brief The split flag for a node's handles as they are: one handle, or two that aren't mirror images, are split. */
		void UpdateSplit(Node &node)
		{
			if (!node.hasIn && !node.hasOut)
			{
				node.split = false;
				return;
			}
			if (node.hasIn != node.hasOut)
			{
				node.split = true;
				return;
			}
			node.split = false;
			Resymmetrize(node);
		}

		/* @brief The node a new node on segment from -> to starts as: the segment's style, texture coordinates interpolated. */
		Node MakeInsertedNode(const Node &start, const Node &end, const double t)
		{
			Node node;
			node.attributes = start.attributes;
			node.attributes.lineTypeIn = start.attributes.lineType;
			node.attributes.lightTypeIn = start.attributes.lightType;
			if (start.attributes.hasUv && end.attributes.hasUv)
			{
				const float f = static_cast<float>(t);
				node.attributes.s = start.attributes.s + (end.attributes.s - start.attributes.s) * f;
				node.attributes.t = start.attributes.t + (end.attributes.t - start.attributes.t) * f;
				node.attributes.sLo = node.attributes.sHi = node.attributes.s;
				node.attributes.tLo = node.attributes.tHi = node.attributes.t;
			}
			return node;
		}

		/* @brief A node about to start a segment of its own (the end of a line being extended or closed) carries on the style of the one before it. */
		void ContinueStyle(Node &node, const Node &before)
		{
			if (node.attributes.lineType == 0 && node.attributes.lightType == 0)
			{
				node.attributes.lineType = before.attributes.lineType;
				node.attributes.lightType = before.attributes.lightType;
			}
		}

		// -------------------------------------------------------
		// B-spline helpers

		/* @brief Control point i of a B-spline, with the mirrored points beyond an open chain's ends. */
		Point2 ControlAt(const std::vector<Point2> &controls, const bool closed, const int64_t i)
		{
			const int64_t count = static_cast<int64_t>(controls.size());
			if (closed)
				return controls[static_cast<size_t>(((i % count) + count) % count)];
			if (i < 0)
				return Mirror(controls[1], controls[0]);
			if (i >= count)
				return Mirror(controls[count - 2], controls[count - 1]);
			return controls[static_cast<size_t>(i)];
		}

		struct SplineNode
		{
			Point2 position;
			Point2 handleIn;
			Point2 handleOut;
			bool hasHandles = false;

			bool operator==(const SplineNode &other) const
			{
				return Same(position, other.position) && Same(handleIn, other.handleIn) && Same(handleOut, other.handleOut) && hasHandles == other.hasHandles;
			}
		};

		/* @brief Bezier node i of the B-spline a control polygon defines. */
		SplineNode ComputeSplineNode(const std::vector<Point2> &controls, const bool closed, const uint32_t i)
		{
			const uint32_t count = static_cast<uint32_t>(controls.size());
			const Point2 previous = ControlAt(controls, closed, static_cast<int64_t>(i) - 1);
			const Point2 current = controls[i];
			const Point2 next = ControlAt(controls, closed, static_cast<int64_t>(i) + 1);

			SplineNode node;
			if (!closed && i == 0)
			{
				// The curve starts on the first control point, heading for the second
				node.position = current;
				node.handleOut = { (2.0 * current.x + next.x) / 3.0, (2.0 * current.z + next.z) / 3.0 };
				node.handleIn = Mirror(node.handleOut, node.position);
			}
			else if (!closed && i + 1 == count)
			{
				node.position = current;
				node.handleIn = { (previous.x + 2.0 * current.x) / 3.0, (previous.z + 2.0 * current.z) / 3.0 };
				node.handleOut = Mirror(node.handleIn, node.position);
			}
			else
			{
				node.position = { (previous.x + 4.0 * current.x + next.x) / 6.0, (previous.z + 4.0 * current.z + next.z) / 6.0 };
				node.handleOut = { (2.0 * current.x + next.x) / 3.0, (2.0 * current.z + next.z) / 3.0 };
				node.handleIn = Mirror(node.handleOut, node.position);
			}
			node.hasHandles = Distance(node.handleOut, node.position) >= HANDLE_EPSILON;
			return node;
		}
	}

	const char *GetNodeTypeName(const NodeType type)
	{
		switch (type)
		{
			case NodeType::Corner:		return "Corner";
			case NodeType::Free:		return "Free";
			case NodeType::Smooth:		return "Smooth";
			case NodeType::Symmetric:	return "Symmetric";
		}
		return "";
	}

	// -------------------------------------------------------

	uint32_t Chain::GetSegmentCount() const
	{
		const uint32_t count = static_cast<uint32_t>(nodes.size());
		if (count < 2)
			return 0;
		return closed ? count : count - 1;
	}

	size_t Shape::CountNodes() const
	{
		size_t count = 0;
		for (const Chain &chain : chains)
			count += chain.nodes.size();
		return count;
	}

	bool Shape::IsValid(const uint32_t chain, const uint32_t node) const
	{
		return chain < chains.size() && node < chains[chain].nodes.size();
	}

	Point2 Bezier::Evaluate(const double t) const
	{
		const double u = 1.0 - t;
		const double b0 = u * u * u, b1 = 3.0 * u * u * t, b2 = 3.0 * u * t * t, b3 = t * t * t;
		return { b0 * p[0].x + b1 * p[1].x + b2 * p[2].x + b3 * p[3].x, b0 * p[0].z + b1 * p[1].z + b2 * p[2].z + b3 * p[3].z };
	}

	bool Bezier::IsStraight() const
	{
		return Same(p[0], p[1]) && Same(p[2], p[3]);
	}

	Affine2 Affine2::Translation(const double x, const double z)
	{
		Affine2 m;
		m.tx = x;
		m.tz = z;
		return m;
	}

	Affine2 Affine2::Rotation(const double radians, const Point2 &pivot)
	{
		// Counter-clockwise seen from above (+Y) in X-Z: x' = x cos + z sin, z' = -x sin + z cos
		const double c = std::cos(radians), s = std::sin(radians);
		Affine2 m;
		m.a = c;
		m.b = s;
		m.c = -s;
		m.d = c;
		m.tx = pivot.x - (m.a * pivot.x + m.b * pivot.z);
		m.tz = pivot.z - (m.c * pivot.x + m.d * pivot.z);
		return m;
	}

	Affine2 Affine2::Scale(const double sx, const double sz, const Point2 &pivot)
	{
		Affine2 m;
		m.a = sx;
		m.d = sz;
		m.tx = pivot.x - sx * pivot.x;
		m.tz = pivot.z - sz * pivot.z;
		return m;
	}

	Affine2 Affine2::After(const Affine2 &other) const
	{
		Affine2 m;
		m.a = a * other.a + b * other.c;
		m.b = a * other.b + b * other.d;
		m.c = c * other.a + d * other.c;
		m.d = c * other.b + d * other.d;
		m.tx = a * other.tx + b * other.tz + tx;
		m.tz = c * other.tx + d * other.tz + tz;
		return m;
	}

	bool Affine2::IsIdentity() const
	{
		return a == 1.0 && b == 0.0 && c == 0.0 && d == 1.0 && tx == 0.0 && tz == 0.0;
	}

	// -------------------------------------------------------
	// Conversions

	Shape FromAptChains(const std::vector<XPlane::AptNodeChain> &chains, const GeoProjection &projection)
	{
		Shape shape;
		shape.chains.reserve(chains.size());
		for (const XPlane::AptNodeChain &source : chains)
		{
			Chain &chain = shape.chains.emplace_back();
			chain.closed = source.closed;
			const size_t count = source.nodes.size();
			if (count == 0)
				continue;

			std::vector<Point2> local(count);
			for (size_t i = 0; i < count; ++i)
				local[i] = projection.ToLocal(source.nodes[i].latitude, source.nodes[i].longitude);

			// Runs of co-located nodes (how split bezier nodes are written)
			struct Run
			{
				size_t first = 0;
				size_t last = 0;
			};
			std::vector<Run> runs;
			for (size_t i = 0; i < count; ++i)
			{
				if (!runs.empty() && Distance(local[runs.back().last], local[i]) < COLOCATED_METERS)
					runs.back().last = i;
				else
					runs.push_back({ i, i });
			}

			// A ring that ends on its first point again: the last run is the first node's incoming half
			const bool wraps = chain.closed && runs.size() > 2 && Distance(local[runs.back().first], local[runs.front().first]) < COLOCATED_METERS;
			chain.wrapsSplit = wraps;
			const size_t used = wraps ? runs.size() - 1 : runs.size();

			chain.nodes.reserve(used);
			for (size_t r = 0; r < used; ++r)
			{
				const size_t first = (wraps && r == 0) ? runs.back().first : runs[r].first;
				const size_t last = runs[r].last;
				const XPlane::AptNode &in = source.nodes[first];
				const XPlane::AptNode &out = source.nodes[last];

				Node &node = chain.nodes.emplace_back();
				node.position = local[last];
				node.sourcePosition = MakeSource(local[last], out.latitude, out.longitude);
				CopyAptAttributes(out, node.attributes, false);

				if (first == last)
				{
					CopyAptAttributes(out, node.attributes, true);
					if (out.hasControl)
					{
						const Point2 control = projection.ToLocal(out.controlLatitude, out.controlLongitude);
						node.hasIn = node.hasOut = true;
						node.handleOut = control;
						node.handleIn = Mirror(control, node.position);
						node.sourceOut = MakeSource(control, out.controlLatitude, out.controlLongitude);
					}
					continue;
				}

				// Split node: the first node's mirrored control leads in, the last node's control leads out
				node.split = true;
				CopyAptAttributes(in, node.attributes, true);
				node.sourcePositionIn = MakeSource(local[first], in.latitude, in.longitude);
				if (in.hasControl)
				{
					const Point2 control = projection.ToLocal(in.controlLatitude, in.controlLongitude);
					node.hasIn = true;
					node.handleIn = Mirror(control, local[first]);
					node.sourceIn = MakeSource(node.handleIn, in.controlLatitude, in.controlLongitude);
				}
				if (out.hasControl)
				{
					const Point2 control = projection.ToLocal(out.controlLatitude, out.controlLongitude);
					node.hasOut = true;
					node.handleOut = control;
					node.sourceOut = MakeSource(control, out.controlLatitude, out.controlLongitude);
				}
			}
		}
		return shape;
	}

	std::vector<XPlane::AptNodeChain> ToAptChains(const Shape &shape, const GeoProjection &projection)
	{
		std::vector<XPlane::AptNodeChain> chains;
		chains.reserve(shape.chains.size());
		for (const Chain &chain : shape.chains)
		{
			XPlane::AptNodeChain &out = chains.emplace_back();
			out.closed = chain.closed;
			out.nodes.reserve(chain.nodes.size() + 4);
			XPlane::AptNode tail;
			bool hasTail = false;

			for (size_t i = 0; i < chain.nodes.size(); ++i)
			{
				const Node &node = chain.nodes[i];
				XPlane::AptNode main;
				ToGeo(projection, node.position, node.sourcePosition, main.latitude, main.longitude);
				main.lineType = node.attributes.lineType;
				main.lightType = node.attributes.lightType;

				if (!node.split)
				{
					if (node.hasOut || node.hasIn)
					{
						main.hasControl = true;
						if (node.hasOut)
							ToGeo(projection, node.handleOut, node.sourceOut, main.controlLatitude, main.controlLongitude);
						else
							projection.ToGeo(Mirror(node.handleIn, node.position), main.controlLatitude, main.controlLongitude);
					}
					out.nodes.push_back(main);
					continue;
				}

				// Split: the incoming half first (its mirrored control is the incoming handle), then the outgoing half
				XPlane::AptNode incoming;
				const bool unmoved = node.sourcePosition.valid && Same(node.sourcePosition.local, node.position);
				if (unmoved && node.sourcePositionIn.valid)
				{
					incoming.latitude = node.sourcePositionIn.latitude;
					incoming.longitude = node.sourcePositionIn.longitude;
				}
				else
				{
					incoming.latitude = main.latitude;
					incoming.longitude = main.longitude;
				}
				incoming.lineType = node.attributes.lineTypeIn;
				incoming.lightType = node.attributes.lightTypeIn;
				if (node.hasIn)
				{
					incoming.hasControl = true;
					if (node.sourceIn.valid && Same(node.sourceIn.local, node.handleIn))
					{
						incoming.controlLatitude = node.sourceIn.latitude;
						incoming.controlLongitude = node.sourceIn.longitude;
					}
					else
						projection.ToGeo(Mirror(node.handleIn, node.position), incoming.controlLatitude, incoming.controlLongitude);
				}
				if (node.hasOut)
				{
					main.hasControl = true;
					ToGeo(projection, node.handleOut, node.sourceOut, main.controlLatitude, main.controlLongitude);
				}

				if (i == 0 && chain.closed && chain.wrapsSplit)
				{
					tail = incoming;
					hasTail = true;
				}
				else
					out.nodes.push_back(incoming);
				out.nodes.push_back(main);
			}
			if (hasTail)
				out.nodes.push_back(tail);
		}
		return chains;
	}

	Shape FromGeoRings(const std::vector<ProjectFile::GeoRing> &rings, const bool closed, const GeoProjection &projection)
	{
		Shape shape;
		shape.chains.reserve(rings.size());
		for (const ProjectFile::GeoRing &ring : rings)
		{
			Chain &chain = shape.chains.emplace_back();
			chain.closed = closed;
			chain.nodes.reserve(ring.size());
			for (const ProjectFile::GeoNode &geo : ring)
			{
				Node &node = chain.nodes.emplace_back();
				node.position = projection.ToLocal(geo.latitude, geo.longitude);
				node.sourcePosition = MakeSource(node.position, geo.latitude, geo.longitude);
				node.split = geo.split;
				if (geo.HasLo())
				{
					node.hasIn = true;
					node.handleIn = projection.ToLocal(geo.latitude + geo.loLatitude, geo.longitude + geo.loLongitude);
					node.sourceIn = MakeSource(node.handleIn, geo.loLatitude, geo.loLongitude);	// Offsets, as stored
				}
				if (geo.HasHi())
				{
					node.hasOut = true;
					node.handleOut = projection.ToLocal(geo.latitude + geo.hiLatitude, geo.longitude + geo.hiLongitude);
					node.sourceOut = MakeSource(node.handleOut, geo.hiLatitude, geo.hiLongitude);
				}
				NodeAttributes &attributes = node.attributes;
				attributes.hasUv = geo.hasUv;
				attributes.s = geo.s;
				attributes.t = geo.t;
				attributes.sLo = geo.sLo;
				attributes.tLo = geo.tLo;
				attributes.sHi = geo.sHi;
				attributes.tHi = geo.tHi;
				attributes.wallType = geo.wallType;
				attributes.spawnTiles = geo.spawnTiles;
			}
		}
		return shape;
	}

	std::vector<ProjectFile::GeoRing> ToGeoRings(const Shape &shape, const GeoProjection &projection)
	{
		std::vector<ProjectFile::GeoRing> rings;
		rings.reserve(shape.chains.size());
		for (const Chain &chain : shape.chains)
		{
			ProjectFile::GeoRing &ring = rings.emplace_back();
			ring.reserve(chain.nodes.size());
			for (const Node &node : chain.nodes)
			{
				ProjectFile::GeoNode &geo = ring.emplace_back();
				ToGeo(projection, node.position, node.sourcePosition, geo.latitude, geo.longitude);
				const bool unmoved = node.sourcePosition.valid && Same(node.sourcePosition.local, node.position);

				// Handle offsets: as stored when neither the node nor the handle moved
				const auto offset = [&](const Point2 &handle, const SourcePoint &source, double &latitude, double &longitude)
				{
					if (unmoved && source.valid && Same(source.local, handle))
					{
						latitude = source.latitude;
						longitude = source.longitude;
						return;
					}
					double handleLatitude = 0.0, handleLongitude = 0.0;
					projection.ToGeo(handle, handleLatitude, handleLongitude);
					latitude = handleLatitude - geo.latitude;
					longitude = handleLongitude - geo.longitude;
				};

				geo.split = node.split;
				if (node.hasOut)
					offset(node.handleOut, node.sourceOut, geo.hiLatitude, geo.hiLongitude);
				if (!node.split && node.hasOut)
				{
					// Unsplit nodes keep lo = -hi exactly
					geo.loLatitude = -geo.hiLatitude;
					geo.loLongitude = -geo.hiLongitude;
				}
				else if (node.hasIn)
					offset(node.handleIn, node.sourceIn, geo.loLatitude, geo.loLongitude);

				const NodeAttributes &attributes = node.attributes;
				geo.hasUv = attributes.hasUv;
				geo.s = attributes.s;
				geo.t = attributes.t;
				geo.sLo = attributes.sLo;
				geo.tLo = attributes.tLo;
				geo.sHi = attributes.sHi;
				geo.tHi = attributes.tHi;
				geo.wallType = attributes.wallType;
				geo.spawnTiles = attributes.spawnTiles;
			}
		}
		return rings;
	}

	// -------------------------------------------------------
	// Queries

	Bezier GetSegment(const Chain &chain, const uint32_t segment)
	{
		Bezier bezier;
		const uint32_t count = static_cast<uint32_t>(chain.nodes.size());
		if (count == 0)
			return bezier;
		const Node &from = chain.nodes[segment % count];
		const Node &to = chain.nodes[(segment + 1) % count];
		bezier.p[0] = from.position;
		bezier.p[1] = from.hasOut ? from.handleOut : from.position;
		bezier.p[2] = to.hasIn ? to.handleIn : to.position;
		bezier.p[3] = to.position;
		return bezier;
	}

	void SampleChain(const Chain &chain, std::vector<Sample> &out, const double maxSegmentLength)
	{
		out.clear();
		const uint32_t segments = chain.GetSegmentCount();
		if (chain.nodes.empty())
			return;
		if (segments == 0)
		{
			out.push_back({ chain.nodes[0].position, 0, 0.0 });
			return;
		}

		for (uint32_t s = 0; s < segments; ++s)
		{
			const Bezier bezier = GetSegment(chain, s);
			if (bezier.IsStraight())
			{
				out.push_back({ bezier.p[0], s, 0.0 });
				continue;
			}
			const double estimate = Distance(bezier.p[0], bezier.p[1]) + Distance(bezier.p[1], bezier.p[2]) + Distance(bezier.p[2], bezier.p[3]);
			const int steps = std::clamp(static_cast<int>(std::ceil(estimate / std::max(0.05, maxSegmentLength))), 4, 64);
			for (int i = 0; i < steps; ++i)
			{
				const double t = static_cast<double>(i) / steps;
				out.push_back({ bezier.Evaluate(t), s, t });
			}
		}
		// The end: the last node of an open chain, the first point again for a closed one
		const Bezier last = GetSegment(chain, segments - 1);
		out.push_back({ last.p[3], segments - 1, 1.0 });
	}

	XPlane::Bounds2 GetBounds(const Shape &shape, const bool includeHandles)
	{
		XPlane::Bounds2 bounds;
		for (const Chain &chain : shape.chains)
		{
			for (const Node &node : chain.nodes)
			{
				bounds.Add(node.position);
				if (includeHandles && node.hasIn)
					bounds.Add(node.handleIn);
				if (includeHandles && node.hasOut)
					bounds.Add(node.handleOut);
			}
		}
		return bounds;
	}

	Point2 GetCentroid(const Shape &shape, const std::vector<NodeRef> &nodes)
	{
		double x = 0.0, z = 0.0;
		size_t count = 0;
		const auto add = [&](const Point2 &p) { x += p.x; z += p.z; ++count; };
		if (nodes.empty())
		{
			for (const Chain &chain : shape.chains)
				for (const Node &node : chain.nodes)
					add(node.position);
		}
		else
		{
			for (const NodeRef &ref : nodes)
				if (shape.IsValid(ref.chain, ref.node))
					add(shape.chains[ref.chain].nodes[ref.node].position);
		}
		return count ? Point2{ x / static_cast<double>(count), z / static_cast<double>(count) } : Point2{};
	}

	std::vector<NodeRef> AllNodes(const Shape &shape)
	{
		std::vector<NodeRef> nodes;
		nodes.reserve(shape.CountNodes());
		for (uint32_t c = 0; c < shape.chains.size(); ++c)
			for (uint32_t n = 0; n < shape.chains[c].nodes.size(); ++n)
				nodes.push_back({ c, n });
		return nodes;
	}

	NodeType GetNodeType(const Node &node)
	{
		if (!node.hasIn && !node.hasOut)
			return NodeType::Corner;
		if (!node.hasIn || !node.hasOut)
			return NodeType::Free;
		if (!node.split)
			return node.preferred == NodeType::Smooth || node.preferred == NodeType::Free ? node.preferred : NodeType::Symmetric;
		return node.preferred != NodeType::Free && HandlesLineUp(node) ? NodeType::Smooth : NodeType::Free;
	}

	double GetSegmentLength(const Bezier &bezier)
	{
		if (bezier.IsStraight())
			return Distance(bezier.p[0], bezier.p[3]);

		// Gauss-Legendre (5 points) on 8 pieces: far below a millimeter for any curve scenery uses
		constexpr double nodes[5] = { -0.9061798459386640, -0.5384693101056831, 0.0, 0.5384693101056831, 0.9061798459386640 };
		constexpr double weights[5] = { 0.2369268850561891, 0.4786286704993665, 0.5688888888888889, 0.4786286704993665, 0.2369268850561891 };
		const Point2 d0 = Sub(bezier.p[1], bezier.p[0]), d1 = Sub(bezier.p[2], bezier.p[1]), d2 = Sub(bezier.p[3], bezier.p[2]);
		constexpr int pieces = 8;
		double length = 0.0;
		for (int piece = 0; piece < pieces; ++piece)
		{
			const double a = static_cast<double>(piece) / pieces, b = static_cast<double>(piece + 1) / pieces;
			for (int i = 0; i < 5; ++i)
			{
				const double t = 0.5 * (a + b) + 0.5 * (b - a) * nodes[i];
				const double u = 1.0 - t;
				const double x = 3.0 * (u * u * d0.x + 2.0 * u * t * d1.x + t * t * d2.x);
				const double z = 3.0 * (u * u * d0.z + 2.0 * u * t * d1.z + t * t * d2.z);
				length += 0.5 * (b - a) * weights[i] * std::hypot(x, z);
			}
		}
		return length;
	}

	double GetChainLength(const Chain &chain)
	{
		double length = 0.0;
		for (uint32_t s = 0; s < chain.GetSegmentCount(); ++s)
			length += GetSegmentLength(GetSegment(chain, s));
		return length;
	}

	double GetMinimumRadius(const Chain &chain, uint32_t *segment, double *t)
	{
		// The circle through three points of the curve about half a meter apart (along it), rather than the derivatives:
		// a handle of no length makes a bezier's curvature spike at its node and short segments hook within
		// centimeters, though nothing visibly bends there. Segments under a meter are corners, not bends.
		constexpr double spacing = 0.5;
		double largest = 0.0;	// Curvature, 1 / meters
		std::vector<Point2> points;
		std::vector<double> parameters;
		for (uint32_t s = 0; s < chain.GetSegmentCount(); ++s)
		{
			const Bezier bezier = GetSegment(chain, s);
			if (bezier.IsStraight())
				continue;
			const int dense = std::clamp(static_cast<int>(std::ceil(GetSegmentLength(bezier) / (spacing * 0.2))), 8, 1024);
			points.assign(1, bezier.p[0]);
			parameters.assign(1, 0.0);
			for (int i = 1; i <= dense; ++i)
			{
				const double u = static_cast<double>(i) / dense;
				const Point2 p = bezier.Evaluate(u);
				if (Distance(p, points.back()) >= spacing)
				{
					points.push_back(p);
					parameters.push_back(u);
				}
			}
			for (size_t i = 1; i + 1 < points.size(); ++i)
			{
				const Point2 &a = points[i - 1], &b = points[i], &c = points[i + 1];
				const double ab = Distance(a, b), bc = Distance(b, c), ca = Distance(c, a);
				if (ab < 1e-4 || bc < 1e-4 || ca < 1e-4)
					continue;
				const double cross = (b.x - a.x) * (c.z - a.z) - (b.z - a.z) * (c.x - a.x);
				const double curvature = 2.0 * std::abs(cross) / (ab * bc * ca);
				if (curvature > largest)
				{
					largest = curvature;
					if (segment)
						*segment = s;
					if (t)
						*t = parameters[i];
				}
			}
		}
		return largest > 0.0 ? 1.0 / largest : std::numeric_limits<double>::infinity();
	}

	// -------------------------------------------------------
	// Edits

	void TransformNodes(Shape &shape, const std::vector<NodeRef> &nodes, const Affine2 &transform)
	{
		std::map<uint32_t, std::vector<Point2>> splines;	// B-spline chains touched: their control points before
		for (const NodeRef &ref : nodes)
		{
			if (!shape.IsValid(ref.chain, ref.node))
				continue;
			Chain &chain = shape.chains[ref.chain];
			if (chain.IsBSpline())
			{
				splines.try_emplace(ref.chain, chain.controls);
				chain.controls[ref.node] = transform.Apply(chain.controls[ref.node]);
				continue;
			}
			Node &node = chain.nodes[ref.node];
			node.position = transform.Apply(node.position);
			if (node.hasIn)
				node.handleIn = transform.Apply(node.handleIn);
			if (node.hasOut)
				node.handleOut = transform.Apply(node.handleOut);
		}
		for (const auto &[chain, previous] : splines)
			ApplyBSpline(shape.chains[chain], &previous);
	}

	void TransformShape(Shape &shape, const Affine2 &transform)
	{
		TransformNodes(shape, AllNodes(shape), transform);
	}

	void SetHandle(Shape &shape, const NodeRef &ref, const bool outgoing, const Point2 &position, const bool breakSymmetry)
	{
		if (!shape.IsValid(ref.chain, ref.node) || shape.chains[ref.chain].IsBSpline())
			return;
		Node &node = shape.chains[ref.chain].nodes[ref.node];
		const NodeType type = GetNodeType(node);

		if (Distance(position, node.position) < HANDLE_EPSILON)
		{
			// Dropped onto its node: the handle goes (a symmetric node loses both, they mirror each other)
			const bool both = type == NodeType::Symmetric;
			if (outgoing || both)
				node.hasOut = false;
			if (!outgoing || both)
				node.hasIn = false;
			node.preferred = NodeType::Corner;
			UpdateSplit(node);
			return;
		}

		Point2 &handle = outgoing ? node.handleOut : node.handleIn;
		Point2 &other = outgoing ? node.handleIn : node.handleOut;
		bool &hasHandle = outgoing ? node.hasOut : node.hasIn;
		bool &hasOther = outgoing ? node.hasIn : node.hasOut;
		handle = position;
		hasHandle = true;
		if (breakSymmetry)
		{
			node.split = true;
			node.preferred = NodeType::Free;
			return;
		}

		switch (type)
		{
			case NodeType::Corner:
			case NodeType::Symmetric:
				other = Mirror(position, node.position);
				hasOther = true;
				node.split = false;
				break;
			case NodeType::Smooth:
			{
				// Turn the other handle to point the opposite way; it keeps its length
				const double length = Distance(other, node.position);
				const double reach = Distance(position, node.position);
				if (EqualLengths(length, reach))
				{
					other = Mirror(position, node.position);
					node.split = false;
				}
				else
				{
					const Point2 away = Scale(Sub(node.position, position), length / reach);
					other = Add(node.position, away);
					node.split = true;
				}
				node.preferred = NodeType::Smooth;
				break;
			}
			case NodeType::Free:
				node.split = true;
				break;
		}
	}

	void SetNodeType(Shape &shape, const std::vector<NodeRef> &nodes, const NodeType type)
	{
		for (const NodeRef &ref : nodes)
		{
			if (!shape.IsValid(ref.chain, ref.node) || shape.chains[ref.chain].IsBSpline())
				continue;
			const Chain &chain = shape.chains[ref.chain];
			Node &node = shape.chains[ref.chain].nodes[ref.node];
			const Point2 p = node.position;

			if (type == NodeType::Corner)
			{
				node.hasIn = node.hasOut = false;
				node.split = false;
				node.preferred = NodeType::Corner;
				continue;
			}
			if (type == NodeType::Free)
			{
				// Nothing moves; mirror-image handles stay stored as such (no split pair in apt.dat) until one is dragged
				UpdateSplit(node);
				node.preferred = NodeType::Free;
				continue;
			}

			// Smooth / symmetric: a direction through the node and a reach on either side
			bool hasPrevious = false, hasNext = false;
			const Point2 previous = chain.nodes[Previous(chain, ref.node, hasPrevious)].position;
			const Point2 next = chain.nodes[Next(chain, ref.node, hasNext)].position;
			const double toPrevious = hasPrevious ? Distance(p, previous) : 0.0;
			const double toNext = hasNext ? Distance(p, next) : 0.0;

			std::optional<Point2> direction;
			if (node.hasIn && node.hasOut)
				direction = Direction(Sub(node.handleOut, node.handleIn));
			if (!direction && node.hasOut)
				direction = Direction(Sub(node.handleOut, p));
			if (!direction && node.hasIn)
				direction = Direction(Sub(p, node.handleIn));
			if (!direction && hasPrevious && hasNext)
				direction = Direction(Sub(next, previous));
			if (!direction && hasNext)
				direction = Direction(Sub(next, p));
			if (!direction && hasPrevious)
				direction = Direction(Sub(p, previous));
			if (!direction)
				continue;

			double reachIn = node.hasIn ? Distance(node.handleIn, p) : 0.0;
			double reachOut = node.hasOut ? Distance(node.handleOut, p) : 0.0;
			if (type == NodeType::Symmetric)
			{
				double reach = node.hasIn && node.hasOut ? 0.5 * (reachIn + reachOut) : std::max(reachIn, reachOut);
				if (reach < HANDLE_EPSILON)
				{
					// No handles yet: a third of the way to the nearer neighbor (as Smooth Nodes does)
					reach = (hasPrevious && hasNext ? std::min(toPrevious, toNext) : std::max(toPrevious, toNext)) / 3.0;
				}
				if (reach < HANDLE_EPSILON)
					continue;
				node.handleOut = Add(p, Scale(*direction, reach));
				node.handleIn = Mirror(node.handleOut, p);
				node.hasIn = node.hasOut = true;
				node.split = false;
				node.preferred = NodeType::Symmetric;
				continue;
			}

			// Smooth: missing handles reach a third of the way to their neighbor (or as far as the other handle)
			if (reachIn < HANDLE_EPSILON && hasPrevious)
				reachIn = toPrevious / 3.0;
			if (reachOut < HANDLE_EPSILON && hasNext)
				reachOut = toNext / 3.0;
			if (reachIn < HANDLE_EPSILON)
				reachIn = reachOut;
			if (reachOut < HANDLE_EPSILON)
				reachOut = reachIn;
			if (reachIn < HANDLE_EPSILON || reachOut < HANDLE_EPSILON)
				continue;
			node.handleOut = Add(p, Scale(*direction, reachOut));
			node.hasIn = node.hasOut = true;
			if (EqualLengths(reachIn, reachOut))
			{
				node.handleIn = Mirror(node.handleOut, p);
				node.split = false;
			}
			else
			{
				node.handleIn = Sub(p, Scale(*direction, reachIn));
				node.split = true;
			}
			node.preferred = NodeType::Smooth;
		}
	}

	namespace
	{
		/* @brief A bent segment's handle: a corner node grows it on this side only; any other node keeps its type. */
		void PlaceBendHandle(Shape &shape, const NodeRef &ref, const bool outgoing, const Point2 &position)
		{
			Node &node = shape.chains[ref.chain].nodes[ref.node];
			const NodeType type = GetNodeType(node);
			if (type == NodeType::Corner || (type == NodeType::Free && !(outgoing ? node.hasOut : node.hasIn)))
			{
				if (Distance(position, node.position) < HANDLE_EPSILON)
					return;
				(outgoing ? node.handleOut : node.handleIn) = position;
				(outgoing ? node.hasOut : node.hasIn) = true;
				UpdateSplit(node);
				return;
			}
			SetHandle(shape, ref, outgoing, position, false);
		}
	}

	bool BendSegment(Shape &shape, const uint32_t chainIndex, const uint32_t segment, double t, const Point2 &target)
	{
		if (chainIndex >= shape.chains.size())
			return false;
		Chain &chain = shape.chains[chainIndex];
		if (segment >= chain.GetSegmentCount())
			return false;
		t = std::clamp(t, 0.05, 0.95);
		const Bezier bezier = GetSegment(chain, segment);
		const Point2 delta = Sub(target, bezier.Evaluate(t));

		if (chain.IsBSpline())
		{
			// The least movement of the control points that takes the curve point there
			const std::vector<std::pair<uint32_t, double>> weights = GetBSplineWeights(static_cast<uint32_t>(chain.controls.size()), chain.closed, segment, t);
			double sum = 0.0;
			for (const auto &[index, weight] : weights)
				sum += weight * weight;
			if (sum < 1e-12)
				return false;
			const std::vector<Point2> previous = chain.controls;
			for (const auto &[index, weight] : weights)
				chain.controls[index] = Add(chain.controls[index], Scale(delta, weight / sum));
			ApplyBSpline(chain, &previous);
			return true;
		}

		// The curve point moves by 3t(1-t)^2 dP1 + 3t^2(1-t) dP2: share the move between the handles, the nearer one
		// doing more (all of it within a sixth of either end)
		const double share = std::clamp((t - 1.0 / 6.0) / (2.0 / 3.0), 0.0, 1.0);
		const double w = share * share * (3.0 - 2.0 * share);
		const double u = 1.0 - t;
		const Point2 p1 = Add(bezier.p[1], Scale(delta, (1.0 - w) / (3.0 * t * u * u)));
		const Point2 p2 = Add(bezier.p[2], Scale(delta, w / (3.0 * t * t * u)));
		const uint32_t count = static_cast<uint32_t>(chain.nodes.size());
		PlaceBendHandle(shape, { chainIndex, segment }, true, p1);
		PlaceBendHandle(shape, { chainIndex, (segment + 1) % count }, false, p2);
		return true;
	}

	size_t StraightenSegments(Shape &shape, const std::vector<NodeRef> &nodes)
	{
		const std::set<NodeRef> listed(nodes.begin(), nodes.end());
		size_t straightened = 0;
		for (uint32_t c = 0; c < shape.chains.size(); ++c)
		{
			Chain &chain = shape.chains[c];
			if (chain.IsBSpline())
				continue;
			const uint32_t count = static_cast<uint32_t>(chain.nodes.size());
			for (uint32_t s = 0; s < chain.GetSegmentCount(); ++s)
			{
				const uint32_t to = (s + 1) % count;
				if (!listed.contains({ c, s }) || !listed.contains({ c, to }))
					continue;
				Node &from = chain.nodes[s];
				Node &end = chain.nodes[to];
				if (!from.hasOut && !end.hasIn)
					continue;
				from.hasOut = false;
				end.hasIn = false;
				from.preferred = end.preferred = NodeType::Corner;
				UpdateSplit(from);
				UpdateSplit(end);
				++straightened;
			}
		}
		return straightened;
	}

	bool InsertNode(Shape &shape, const uint32_t chainIndex, const uint32_t segment, const double t, NodeRef *inserted)
	{
		if (chainIndex >= shape.chains.size() || t <= 0.0 || t >= 1.0)
			return false;
		Chain &chain = shape.chains[chainIndex];
		const uint32_t count = static_cast<uint32_t>(chain.nodes.size());
		if (segment >= chain.GetSegmentCount())
			return false;

		const uint32_t from = segment;
		const uint32_t to = (segment + 1) % count;
		const Bezier bezier = GetSegment(chain, segment);

		// Attributes continue the segment's (its style starts at `from`); texture coordinates are interpolated
		Node node = MakeInsertedNode(chain.nodes[from], chain.nodes[to], t);

		if (chain.IsBSpline())
		{
			// A control point on the control polygon's edge under the segment
			const uint32_t at = to == 0 ? count : to;
			chain.nodes.insert(chain.nodes.begin() + at, node);
			chain.controls.insert(chain.controls.begin() + at, Lerp(chain.controls[from], chain.controls[to], std::clamp(t, 0.1, 0.9)));
			ApplyBSpline(chain);
			if (inserted)
				*inserted = { chainIndex, at };
			return true;
		}

		if (bezier.IsStraight())
			node.position = Lerp(bezier.p[0], bezier.p[3], t);
		else
		{
			// de Casteljau: the two halves are exactly the original curve
			const Point2 p01 = Lerp(bezier.p[0], bezier.p[1], t);
			const Point2 p12 = Lerp(bezier.p[1], bezier.p[2], t);
			const Point2 p23 = Lerp(bezier.p[2], bezier.p[3], t);
			const Point2 p012 = Lerp(p01, p12, t);
			const Point2 p123 = Lerp(p12, p23, t);
			node.position = Lerp(p012, p123, t);
			node.handleIn = p012;
			node.handleOut = p123;
			node.hasIn = Distance(p012, node.position) >= HANDLE_EPSILON;
			node.hasOut = Distance(p123, node.position) >= HANDLE_EPSILON;
			node.split = node.hasIn != node.hasOut;
			if (node.hasIn && node.hasOut)
				Resymmetrize(node);

			Node &startNode = chain.nodes[from];
			Node &endNode = chain.nodes[to];
			if (startNode.hasOut)
			{
				startNode.handleOut = p01;
				Resymmetrize(startNode);
			}
			if (endNode.hasIn)
			{
				endNode.handleIn = p23;
				Resymmetrize(endNode);
			}
		}

		const uint32_t at = to == 0 ? count : to;	// The closing segment of a ring inserts at the end
		chain.nodes.insert(chain.nodes.begin() + at, node);
		if (inserted)
			*inserted = { chainIndex, at };
		return true;
	}

	size_t DeleteNodes(Shape &shape, std::vector<NodeRef> nodes)
	{
		std::sort(nodes.begin(), nodes.end());
		nodes.erase(std::unique(nodes.begin(), nodes.end()), nodes.end());

		std::map<uint32_t, std::vector<uint32_t>> byChain;
		for (const NodeRef &ref : nodes)
			if (shape.IsValid(ref.chain, ref.node))
				byChain[ref.chain].push_back(ref.node);

		size_t removed = 0;
		// Back to front, so removing a whole hole doesn't shift the chains still to visit
		for (auto it = byChain.rbegin(); it != byChain.rend(); ++it)
		{
			const uint32_t chainIndex = it->first;
			std::vector<uint32_t> &indices = it->second;
			Chain &chain = shape.chains[chainIndex];
			if (chainIndex > 0 && indices.size() == chain.nodes.size())
			{
				removed += chain.nodes.size();
				shape.chains.erase(shape.chains.begin() + chainIndex);
				continue;
			}

			std::sort(indices.rbegin(), indices.rend());
			size_t erased = 0;
			for (const uint32_t index : indices)
			{
				if (chain.nodes.size() <= chain.GetMinimumNodes())
					break;
				if (index == 0)
					chain.wrapsSplit = false;
				chain.nodes.erase(chain.nodes.begin() + index);
				if (chain.IsBSpline())
					chain.controls.erase(chain.controls.begin() + index);
				++erased;
			}
			if (erased > 0 && chain.IsBSpline())
				ApplyBSpline(chain);
			removed += erased;
		}
		return removed;
	}

	void SmoothNodes(Shape &shape, const std::vector<NodeRef> &nodes)
	{
		// Tangents come from the neighbors' positions as they were, not as smoothed earlier in this call
		const Shape before = shape;
		for (const NodeRef &ref : nodes)
		{
			if (!before.IsValid(ref.chain, ref.node) || before.chains[ref.chain].IsBSpline())
				continue;
			const Chain &chain = before.chains[ref.chain];
			Node &node = shape.chains[ref.chain].nodes[ref.node];
			bool hasPrevious = false, hasNext = false;
			const Point2 previous = chain.nodes[Previous(chain, ref.node, hasPrevious)].position;
			const Point2 next = chain.nodes[Next(chain, ref.node, hasNext)].position;
			const Point2 p = node.position;

			Point2 out;
			if (hasPrevious && hasNext)
			{
				const Point2 direction = { next.x - previous.x, next.z - previous.z };
				const double length = std::hypot(direction.x, direction.z);
				const double reach = std::min(Distance(p, next), Distance(p, previous)) / 3.0;
				if (length < HANDLE_EPSILON || reach < HANDLE_EPSILON)
					continue;
				out = { p.x + direction.x / length * reach, p.z + direction.z / length * reach };
			}
			else if (hasNext)
				out = Lerp(p, next, 1.0 / 3.0);
			else if (hasPrevious)
				out = Mirror(Lerp(p, previous, 1.0 / 3.0), p);
			else
				continue;

			node.handleOut = out;
			node.handleIn = Mirror(out, p);
			node.hasIn = node.hasOut = true;
			node.split = false;
			node.preferred = NodeType::Symmetric;
		}
	}

	void CornerNodes(Shape &shape, const std::vector<NodeRef> &nodes)
	{
		SetNodeType(shape, nodes, NodeType::Corner);
	}

	void ReverseChain(Chain &chain)
	{
		const size_t count = chain.nodes.size();
		if (count < 2)
			return;

		// A segment's style is carried by the node it starts at: after reversing, segment (i+1 -> i) starts at i+1
		std::vector<NodeAttributes> segmentStyle(count);
		for (size_t i = 0; i < count; ++i)
			segmentStyle[i] = chain.nodes[i].attributes;

		std::reverse(chain.nodes.begin(), chain.nodes.end());
		std::reverse(chain.controls.begin(), chain.controls.end());
		for (size_t i = 0; i < count; ++i)
		{
			Node &node = chain.nodes[i];
			std::swap(node.handleIn, node.handleOut);
			std::swap(node.hasIn, node.hasOut);
			std::swap(node.sourceIn, node.sourceOut);
			// Reversed handles are no longer what the source stored for either side
			node.sourceIn.valid = node.sourceOut.valid = false;

			// New node i is old node (count - 1 - i); its outgoing segment was old segment (count - 2 - i)
			const size_t oldIndex = count - 1 - i;
			const bool hasOldSegment = oldIndex > 0 || chain.closed;
			const NodeAttributes &style = segmentStyle[oldIndex > 0 ? oldIndex - 1 : count - 1];
			node.attributes.lineType = hasOldSegment ? style.lineType : 0;
			node.attributes.lightType = hasOldSegment ? style.lightType : 0;
			node.attributes.lineTypeIn = node.attributes.lineType;
			node.attributes.lightTypeIn = node.attributes.lightType;
			node.attributes.wallType = hasOldSegment ? style.wallType : 0;
			node.attributes.spawnTiles = hasOldSegment ? style.spawnTiles : true;
		}
		chain.wrapsSplit = false;
	}

	bool AppendNode(Shape &shape, const uint32_t chainIndex, const bool atStart, const Point2 &position, NodeRef *added)
	{
		if (chainIndex >= shape.chains.size())
			return false;
		Chain &chain = shape.chains[chainIndex];
		if (chain.closed || chain.nodes.empty())
			return false;

		Node node;
		node.position = position;
		uint32_t at = 0;
		if (atStart)
		{
			// The new first node starts the segment into the old one: it continues the old first segment's style
			node.attributes = chain.nodes.front().attributes;
			chain.nodes.insert(chain.nodes.begin(), node);
			if (chain.IsBSpline())
				chain.controls.insert(chain.controls.begin(), position);
		}
		else
		{
			// The old last node now starts a segment: it carries on the style of the one before it
			Node &last = chain.nodes.back();
			if (chain.nodes.size() >= 2)
				ContinueStyle(last, chain.nodes[chain.nodes.size() - 2]);
			node.attributes = last.attributes;
			at = static_cast<uint32_t>(chain.nodes.size());
			chain.nodes.push_back(node);
			if (chain.IsBSpline())
				chain.controls.push_back(position);
		}
		if (chain.IsBSpline())
			ApplyBSpline(chain);
		if (added)
			*added = { chainIndex, at };
		return true;
	}

	bool CloseChain(Shape &shape, const uint32_t chainIndex)
	{
		if (chainIndex >= shape.chains.size())
			return false;
		Chain &chain = shape.chains[chainIndex];
		if (chain.closed)
			return false;
		const size_t count = chain.nodes.size();
		const bool merge = count >= 2 && Distance(chain.nodes.front().position, chain.nodes.back().position) < MERGE_METERS;
		if (count - (merge ? 1 : 0) < 3)
			return false;

		if (merge)
		{
			// The last node lies on the first: one node, the first, taking the last one's incoming handle
			Node &first = chain.nodes.front();
			const Node &last = chain.nodes.back();
			first.handleIn = last.hasIn ? Add(first.position, Sub(last.handleIn, last.position)) : first.position;
			first.hasIn = last.hasIn;
			first.preferred = NodeType::Corner;
			UpdateSplit(first);
			chain.nodes.pop_back();
			if (chain.IsBSpline())
				chain.controls.pop_back();
		}
		else
			ContinueStyle(chain.nodes.back(), chain.nodes[count - 2]);

		chain.closed = true;
		chain.wrapsSplit = false;
		if (chain.IsBSpline())
			ApplyBSpline(chain);
		return true;
	}

	bool OpenChain(Shape &shape, const uint32_t chainIndex, const uint32_t nodeIndex)
	{
		if (!shape.IsValid(chainIndex, nodeIndex))
			return false;
		Chain &chain = shape.chains[chainIndex];
		if (!chain.closed)
			return false;

		std::rotate(chain.nodes.begin(), chain.nodes.begin() + nodeIndex, chain.nodes.end());
		if (chain.IsBSpline())
		{
			std::rotate(chain.controls.begin(), chain.controls.begin() + nodeIndex, chain.controls.end());
			chain.controls.push_back(chain.controls.front());
		}

		// The node becomes both ends: the first keeps its outgoing handle, its copy at the end the incoming one
		Node end = chain.nodes.front();
		Node &start = chain.nodes.front();
		start.hasIn = false;
		end.hasOut = false;
		for (Node *node : { &start, &end })
		{
			node->split = false;	// An end node's single handle needs no split (written as one apt.dat row)
			node->preferred = NodeType::Corner;
		}
		end.sourcePositionIn = {};
		chain.nodes.push_back(end);

		chain.closed = false;
		chain.wrapsSplit = false;
		if (chain.IsBSpline())
			ApplyBSpline(chain);
		return true;
	}

	// -------------------------------------------------------
	// B-splines

	bool BeginBSpline(Chain &chain, const BSplineFit fit, const bool recoverOnly, const double tolerance)
	{
		if (chain.IsBSpline())
			return true;
		if (chain.nodes.size() < chain.GetMinimumNodes())
			return false;
		if (std::optional<std::vector<Point2>> recovered = RecoverBSpline(chain, tolerance))
		{
			chain.controls = std::move(*recovered);
			return true;
		}
		if (recoverOnly)
			return false;

		if (fit == BSplineFit::ThroughNodes)
			chain.controls = InterpolateBSpline(chain);
		else
		{
			chain.controls.clear();
			for (const Node &node : chain.nodes)
				chain.controls.push_back(node.position);
		}
		ApplyBSpline(chain);
		return true;
	}

	void EndBSpline(Chain &chain)
	{
		chain.controls.clear();
	}

	void ApplyBSpline(Chain &chain, const std::vector<Point2> *previous)
	{
		const uint32_t count = static_cast<uint32_t>(chain.controls.size());
		if (count < chain.GetMinimumNodes())
			return;
		if (chain.nodes.size() != count)
			chain.nodes.resize(count, chain.nodes.empty() ? Node{} : chain.nodes.back());
		const bool comparable = previous && previous->size() == count;

		for (uint32_t i = 0; i < count; ++i)
		{
			const SplineNode now = ComputeSplineNode(chain.controls, chain.closed, i);
			if (comparable && ComputeSplineNode(*previous, chain.closed, i) == now)
				continue;	// Its control points didn't move: it keeps its coordinates exactly (unchanged in the file)

			Node &node = chain.nodes[i];
			node.position = now.position;
			node.hasIn = node.hasOut = now.hasHandles;
			node.handleIn = now.hasHandles ? now.handleIn : now.position;
			node.handleOut = now.hasHandles ? now.handleOut : now.position;
			node.split = false;
			node.preferred = NodeType::Corner;
		}
		chain.wrapsSplit = false;
	}

	std::optional<std::vector<Point2>> RecoverBSpline(const Chain &chain, const double tolerance)
	{
		const uint32_t count = static_cast<uint32_t>(chain.nodes.size());
		if (count < chain.GetMinimumNodes() || count < 2)
			return std::nullopt;

		// Each segment's handles give its two control points: B[i] = 2 P1 - P2, B[i+1] = 2 P2 - P1
		std::vector<Point2> sums(count);
		std::vector<int> estimates(count, 0);
		for (uint32_t s = 0; s < chain.GetSegmentCount(); ++s)
		{
			const Bezier bezier = GetSegment(chain, s);
			const uint32_t to = (s + 1) % count;
			sums[s] = Add(sums[s], Sub(Scale(bezier.p[1], 2.0), bezier.p[2]));
			sums[to] = Add(sums[to], Sub(Scale(bezier.p[2], 2.0), bezier.p[1]));
			++estimates[s];
			++estimates[to];
		}
		std::vector<Point2> controls(count);
		for (uint32_t i = 0; i < count; ++i)
			controls[i] = Scale(sums[i], 1.0 / std::max(1, estimates[i]));
		if (!chain.closed)
		{
			// Open chains end on their first and last control points
			controls.front() = chain.nodes.front().position;
			controls.back() = chain.nodes.back().position;
		}

		// Only a B-spline when the chain those control points make is this one
		Chain check;
		check.closed = chain.closed;
		check.controls = controls;
		check.nodes.resize(count);
		ApplyBSpline(check);
		for (uint32_t i = 0; i < count; ++i)
		{
			if (Distance(check.nodes[i].position, chain.nodes[i].position) > tolerance)
				return std::nullopt;
		}
		for (uint32_t s = 0; s < chain.GetSegmentCount(); ++s)
		{
			const Bezier expected = GetSegment(check, s);
			const Bezier actual = GetSegment(chain, s);
			if (Distance(expected.p[1], actual.p[1]) > tolerance || Distance(expected.p[2], actual.p[2]) > tolerance)
				return std::nullopt;
		}
		return controls;
	}

	std::vector<Point2> InterpolateBSpline(const Chain &chain)
	{
		const uint32_t count = static_cast<uint32_t>(chain.nodes.size());
		std::vector<Point2> controls(count);
		for (uint32_t i = 0; i < count; ++i)
			controls[i] = chain.nodes[i].position;
		if (count < chain.GetMinimumNodes() || count < 3)
			return controls;

		// Node i = (B[i-1] + 4 B[i] + B[i+1]) / 6, open ends B = node. Strictly diagonally dominant, so Gauss-Seidel
		// converges (at least 4x per sweep)
		double scale = 0.0;
		for (const Node &node : chain.nodes)
			scale = std::max({ scale, std::abs(node.position.x), std::abs(node.position.z) });
		const double settled = 1e-12 * std::max(1.0, scale);
		const uint32_t first = chain.closed ? 0 : 1;
		const uint32_t end = chain.closed ? count : count - 1;
		for (int sweep = 0; sweep < 200; ++sweep)
		{
			double change = 0.0;
			for (uint32_t i = first; i < end; ++i)
			{
				const Point2 &previous = controls[(i + count - 1) % count];
				const Point2 &next = controls[(i + 1) % count];
				const Point2 &node = chain.nodes[i].position;
				const Point2 updated{ (6.0 * node.x - previous.x - next.x) / 4.0, (6.0 * node.z - previous.z - next.z) / 4.0 };
				change = std::max(change, Distance(updated, controls[i]));
				controls[i] = updated;
			}
			if (change <= settled)
				break;
		}
		return controls;
	}

	std::vector<std::pair<uint32_t, double>> GetBSplineWeights(const uint32_t controls, const bool closed, const uint32_t segment, const double t)
	{
		std::map<uint32_t, double> weights;
		if (controls < (closed ? 3u : 2u))
			return {};
		const int64_t count = controls;

		// Control point j's share (the mirrored points beyond an open chain's ends are 2 B[end] - B[next to it])
		const auto add = [&](const int64_t j, const double w)
		{
			if (closed)
				weights[static_cast<uint32_t>(((j % count) + count) % count)] += w;
			else if (j < 0)
			{
				weights[0] += 2.0 * w;
				weights[1] -= w;
			}
			else if (j >= count)
			{
				weights[static_cast<uint32_t>(count - 1)] += 2.0 * w;
				weights[static_cast<uint32_t>(count - 2)] -= w;
			}
			else
				weights[static_cast<uint32_t>(j)] += w;
		};

		// The segment's bezier: P0 = (B[i-1] + 4B[i] + B[i+1]) / 6, P1 = (2B[i] + B[i+1]) / 3, P2 = (B[i] + 2B[i+1]) / 3,
		// P3 = (B[i] + 4B[i+1] + B[i+2]) / 6, weighted by the Bernstein polynomials
		const int64_t i = segment;
		const double u = 1.0 - t;
		const double b0 = u * u * u, b1 = 3.0 * u * u * t, b2 = 3.0 * u * t * t, b3 = t * t * t;
		add(i - 1, b0 / 6.0);
		add(i, b0 * 4.0 / 6.0 + b1 * 2.0 / 3.0 + b2 / 3.0 + b3 / 6.0);
		add(i + 1, b0 / 6.0 + b1 / 3.0 + b2 * 2.0 / 3.0 + b3 * 4.0 / 6.0);
		add(i + 2, b3 / 6.0);

		std::vector<std::pair<uint32_t, double>> result;
		for (const auto &[index, weight] : weights)
			if (weight != 0.0)
				result.emplace_back(index, weight);
		return result;
	}
}

// -------------------------------------------------------

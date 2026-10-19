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
#include <map>

// -------------------------------------------------------

namespace SceneryEditorX::Splines
{
	namespace
	{
		constexpr double COLOCATED_METERS = 1e-4;	// apt.dat nodes this close are one (split) node
		constexpr double HANDLE_EPSILON = 1e-6;		// A handle this close to its node is no handle

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

	// -------------------------------------------------------
	// Edits

	void TransformNodes(Shape &shape, const std::vector<NodeRef> &nodes, const Affine2 &transform)
	{
		for (const NodeRef &ref : nodes)
		{
			if (!shape.IsValid(ref.chain, ref.node))
				continue;
			Node &node = shape.chains[ref.chain].nodes[ref.node];
			node.position = transform.Apply(node.position);
			if (node.hasIn)
				node.handleIn = transform.Apply(node.handleIn);
			if (node.hasOut)
				node.handleOut = transform.Apply(node.handleOut);
		}
	}

	void TransformShape(Shape &shape, const Affine2 &transform)
	{
		TransformNodes(shape, AllNodes(shape), transform);
	}

	void SetHandle(Shape &shape, const NodeRef &ref, const bool outgoing, const Point2 &position, const bool breakSymmetry)
	{
		if (!shape.IsValid(ref.chain, ref.node))
			return;
		Node &node = shape.chains[ref.chain].nodes[ref.node];

		if (Distance(position, node.position) < HANDLE_EPSILON)
		{
			// Dropped onto its node: the handle goes (an unsplit node loses both, they mirror each other)
			if (outgoing || !node.split)
				node.hasOut = false;
			if (!outgoing || !node.split)
				node.hasIn = false;
			if (!node.hasIn && !node.hasOut)
				node.split = false;
			return;
		}

		(outgoing ? node.handleOut : node.handleIn) = position;
		(outgoing ? node.hasOut : node.hasIn) = true;
		if (breakSymmetry)
		{
			node.split = true;
			return;
		}
		if (!node.split)
		{
			(outgoing ? node.handleIn : node.handleOut) = Mirror(position, node.position);
			(outgoing ? node.hasIn : node.hasOut) = true;
		}
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

		Node node;
		const Node &start = chain.nodes[from];
		const Node &end = chain.nodes[to];

		// Attributes continue the segment's (its style starts at `from`); texture coordinates are interpolated
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
			for (const uint32_t index : indices)
			{
				if (chain.nodes.size() <= chain.GetMinimumNodes())
					break;
				if (index == 0)
					chain.wrapsSplit = false;
				chain.nodes.erase(chain.nodes.begin() + index);
				++removed;
			}
		}
		return removed;
	}

	void SmoothNodes(Shape &shape, const std::vector<NodeRef> &nodes)
	{
		// Tangents come from the neighbors' positions as they were, not as smoothed earlier in this call
		const Shape before = shape;
		for (const NodeRef &ref : nodes)
		{
			if (!before.IsValid(ref.chain, ref.node))
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
		}
	}

	void CornerNodes(Shape &shape, const std::vector<NodeRef> &nodes)
	{
		for (const NodeRef &ref : nodes)
		{
			if (!shape.IsValid(ref.chain, ref.node))
				continue;
			Node &node = shape.chains[ref.chain].nodes[ref.node];
			node.hasIn = node.hasOut = false;
			node.split = false;
		}
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
}

// -------------------------------------------------------

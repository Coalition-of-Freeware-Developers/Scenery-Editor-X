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
 * xp_apt_geometry.cpp
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#include "xp_apt_geometry.h"
#include <algorithm>
#include <cmath>
#include <deque>
#include <limits>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	namespace
	{
		constexpr double kPi = 3.14159265358979323846;
		constexpr double kDegToRad = kPi / 180.0;
		constexpr double kRadToDeg = 180.0 / kPi;

		double Distance(const Point2 &a, const Point2 &b)
		{
			return std::hypot(b.x - a.x, b.z - a.z);
		}

		Point2 CubicBezier(const Point2 &p0, const Point2 &p1, const Point2 &p2, const Point2 &p3, const double t)
		{
			const double u = 1.0 - t;
			const double b0 = u * u * u;
			const double b1 = 3.0 * u * u * t;
			const double b2 = 3.0 * u * t * t;
			const double b3 = t * t * t;
			return { b0 * p0.x + b1 * p1.x + b2 * p2.x + b3 * p3.x, b0 * p0.z + b1 * p1.z + b2 * p2.z + b3 * p3.z };
		}

		/* @brief Appends the points of one segment (excluding its start, which the previous segment produced). */
		void FlattenSegment(const AptNode &from, const AptNode &to, const GeoProjection &projection, const double maxSegmentLength, std::vector<Point2> &out)
		{
			const Point2 p0 = projection.ToLocal(from.latitude, from.longitude);
			const Point2 p3 = projection.ToLocal(to.latitude, to.longitude);
			// Co-located nodes (how split bezier nodes are written) join their curves at a corner: no curve between them,
			// else their controls would draw a loop out and back.
			if ((!from.hasControl && !to.hasControl) || Distance(p0, p3) < 1e-4)
			{
				out.push_back(p3);
				return;
			}

			// The stored control point leads out of a node; the one leading into the next node mirrors its control.
			const Point2 p1 = from.hasControl ? projection.ToLocal(from.controlLatitude, from.controlLongitude) : p0;
			Point2 p2 = p3;
			if (to.hasControl)
			{
				const Point2 control = projection.ToLocal(to.controlLatitude, to.controlLongitude);
				p2 = { 2.0 * p3.x - control.x, 2.0 * p3.z - control.z };
			}

			const double estimate = Distance(p0, p1) + Distance(p1, p2) + Distance(p2, p3);
			// Clamped as a double: a far-off control point makes the estimate too big for an int (NaN: the fewest pieces)
			const double pieces = std::ceil(estimate / std::max(0.1, maxSegmentLength));
			const int segments = std::isnan(pieces) ? 4 : static_cast<int>(std::clamp(pieces, 4.0, 48.0));
			for (int i = 1; i <= segments; ++i)
				out.push_back(CubicBezier(p0, p1, p2, p3, static_cast<double>(i) / segments));
		}
	}

	// -------------------------------------------------------

	void Bounds2::Add(const Point2 &point)
	{
		if (!valid)
		{
			minX = maxX = point.x;
			minZ = maxZ = point.z;
			valid = true;
			return;
		}
		minX = std::min(minX, point.x);
		maxX = std::max(maxX, point.x);
		minZ = std::min(minZ, point.z);
		maxZ = std::max(maxZ, point.z);
	}

	void Bounds2::Add(const Bounds2 &other)
	{
		if (!other.valid)
			return;
		Add(Point2{ other.minX, other.minZ });
		Add(Point2{ other.maxX, other.maxZ });
	}

	double Bounds2::GetRadius() const
	{
		return valid ? 0.5 * std::hypot(maxX - minX, maxZ - minZ) : 0.0;
	}

	// -------------------------------------------------------

	GeoProjection::GeoProjection(const double datumLatitude, const double datumLongitude) : m_Latitude(datumLatitude), m_Longitude(datumLongitude)
	{
		// WGS84 meridian / parallel arc lengths per degree at the datum latitude.
		const double phi = datumLatitude * kDegToRad;
		m_MetersPerDegreeLatitude = 111132.954 - 559.822 * std::cos(2.0 * phi) + 1.175 * std::cos(4.0 * phi);
		m_MetersPerDegreeLongitude = 111412.84 * std::cos(phi) - 93.5 * std::cos(3.0 * phi) + 0.118 * std::cos(5.0 * phi);
	}

	Point2 GeoProjection::ToLocal(const double latitude, const double longitude) const
	{
		double dLon = longitude - m_Longitude;
		if (dLon > 180.0) dLon -= 360.0;
		if (dLon < -180.0) dLon += 360.0;
		return { dLon * m_MetersPerDegreeLongitude, -(latitude - m_Latitude) * m_MetersPerDegreeLatitude };
	}

	void GeoProjection::ToGeo(const Point2 &local, double &latitude, double &longitude) const
	{
		latitude = m_Latitude - local.z / m_MetersPerDegreeLatitude;
		longitude = m_Longitude + (m_MetersPerDegreeLongitude > 1.0 ? local.x / m_MetersPerDegreeLongitude : 0.0);
		if (longitude > 180.0) longitude -= 360.0;
		if (longitude < -180.0) longitude += 360.0;
	}

	// -------------------------------------------------------

	void FlattenChain(const AptNodeChain &chain, const GeoProjection &projection, std::vector<Point2> &out, const double maxSegmentLength)
	{
		out.clear();
		const size_t count = chain.nodes.size();
		if (count == 0)
			return;

		out.push_back(projection.ToLocal(chain.nodes[0].latitude, chain.nodes[0].longitude));
		for (size_t i = 0; i + 1 < count; ++i)
			FlattenSegment(chain.nodes[i], chain.nodes[i + 1], projection, maxSegmentLength, out);

		if (chain.closed && count > 1)
		{
			// The closing segment ends at the first point, which is already there.
			FlattenSegment(chain.nodes[count - 1], chain.nodes[0], projection, maxSegmentLength, out);
			if (!out.empty())
				out.pop_back();
		}

		// Drop consecutive duplicates (split bezier nodes sit on top of each other).
		std::vector<Point2> cleaned;
		cleaned.reserve(out.size());
		for (const Point2 &point : out)
		{
			if (!cleaned.empty() && Distance(cleaned.back(), point) < 1e-4)
				continue;
			cleaned.push_back(point);
		}
		if (chain.closed && cleaned.size() > 1 && Distance(cleaned.front(), cleaned.back()) < 1e-4)
			cleaned.pop_back();
		out.swap(cleaned);
	}

	double SignedArea(const std::vector<Point2> &polygon)
	{
		// Shoelace in (x, -z) so counter-clockwise seen from above (+Y) is positive.
		double sum = 0.0;
		const size_t count = polygon.size();
		for (size_t i = 0, j = count - 1; i < count; j = i++)
			sum += (polygon[j].x * -polygon[i].z) - (polygon[i].x * -polygon[j].z);
		return 0.5 * sum;
	}

	Bounds2 GetBounds(const std::vector<Point2> &points)
	{
		Bounds2 bounds;
		for (const Point2 &point : points)
			bounds.Add(point);
		return bounds;
	}

	// -------------------------------------------------------
	// Ear clipping (after mapbox/earcut, without the z-order hashing: airport polygons are small enough)
	// -------------------------------------------------------

	namespace
	{
		struct EarNode
		{
			uint32_t index = 0;
			double x = 0.0;
			double y = 0.0;
			EarNode *prev = nullptr;
			EarNode *next = nullptr;
			bool steiner = false;
		};

		class Earcut
		{
		public:
			bool Run(const std::vector<Point2> &points, const std::vector<size_t> &ringStarts, std::vector<uint32_t> &triangles)
			{
				m_Points = &points;
				const size_t outerEnd = ringStarts.size() > 1 ? ringStarts[1] : points.size();
				EarNode *outer = LinkedList(0, outerEnd, true);
				if (!outer || outer->next == outer->prev)
					return false;

				if (ringStarts.size() > 1)
					outer = EliminateHoles(ringStarts, outer);

				EarcutLinked(outer, triangles, 0);
				return !triangles.empty();
			}

		private:
			EarNode *NewNode(const uint32_t index, const double x, const double y)
			{
				m_Nodes.push_back(EarNode{ index, x, y });
				return &m_Nodes.back();
			}

			EarNode *InsertNode(const uint32_t index, const double x, const double y, EarNode *last)
			{
				EarNode *node = NewNode(index, x, y);
				if (!last)
				{
					node->prev = node;
					node->next = node;
				}
				else
				{
					node->next = last->next;
					node->prev = last;
					last->next->prev = node;
					last->next = node;
				}
				return node;
			}

			static void RemoveNode(EarNode *node)
			{
				node->next->prev = node->prev;
				node->prev->next = node->next;
			}

			double X(const size_t i) const { return (*m_Points)[i].x; }
			double Y(const size_t i) const { return (*m_Points)[i].z; }

			EarNode *LinkedList(const size_t start, const size_t end, const bool clockwise)
			{
				double sum = 0.0;
				for (size_t i = start, j = end - 1; i < end; j = i++)
					sum += (X(j) - X(i)) * (Y(i) + Y(j));

				EarNode *last = nullptr;
				if (clockwise == (sum > 0.0))
				{
					for (size_t i = start; i < end; ++i)
						last = InsertNode(static_cast<uint32_t>(i), X(i), Y(i), last);
				}
				else
				{
					for (size_t i = end; i > start; --i)
						last = InsertNode(static_cast<uint32_t>(i - 1), X(i - 1), Y(i - 1), last);
				}

				if (last && Equals(last, last->next))
				{
					RemoveNode(last);
					last = last->next;
				}
				return last;
			}

			static bool Equals(const EarNode *a, const EarNode *b) { return a->x == b->x && a->y == b->y; }

			static double Area(const EarNode *p, const EarNode *q, const EarNode *r)
			{
				return (q->y - p->y) * (r->x - q->x) - (q->x - p->x) * (r->y - q->y);
			}

			static EarNode *FilterPoints(EarNode *start, EarNode *end = nullptr)
			{
				if (!start)
					return start;
				if (!end)
					end = start;

				EarNode *p = start;
				bool again;
				do
				{
					again = false;
					if (!p->steiner && (Equals(p, p->next) || Area(p->prev, p, p->next) == 0.0))
					{
						RemoveNode(p);
						p = end = p->prev;
						if (p == p->next)
							break;
						again = true;
					}
					else
					{
						p = p->next;
					}
				} while (again || p != end);
				return end;
			}

			static bool PointInTriangle(const double ax, const double ay, const double bx, const double by, const double cx, const double cy, const double px, const double py)
			{
				return (cx - px) * (ay - py) >= (ax - px) * (cy - py) && (ax - px) * (by - py) >= (bx - px) * (ay - py) && (bx - px) * (cy - py) >= (cx - px) * (by - py);
			}

			static bool IsEar(const EarNode *ear)
			{
				const EarNode *a = ear->prev;
				const EarNode *b = ear;
				const EarNode *c = ear->next;
				if (Area(a, b, c) >= 0.0)
					return false;

				const EarNode *p = ear->next->next;
				while (p != ear->prev)
				{
					if (PointInTriangle(a->x, a->y, b->x, b->y, c->x, c->y, p->x, p->y) && Area(p->prev, p, p->next) >= 0.0)
						return false;
					p = p->next;
				}
				return true;
			}

			void EarcutLinked(EarNode *ear, std::vector<uint32_t> &triangles, const int pass)
			{
				if (!ear)
					return;

				EarNode *stop = ear;
				while (ear->prev != ear->next)
				{
					EarNode *prev = ear->prev;
					EarNode *next = ear->next;

					if (IsEar(ear))
					{
						triangles.push_back(prev->index);
						triangles.push_back(ear->index);
						triangles.push_back(next->index);
						RemoveNode(ear);
						ear = next->next;
						stop = next->next;
						continue;
					}

					ear = next;
					if (ear == stop)
					{
						if (pass == 0)
							EarcutLinked(FilterPoints(ear), triangles, 1);
						else if (pass == 1)
						{
							ear = CureLocalIntersections(FilterPoints(ear), triangles);
							EarcutLinked(ear, triangles, 2);
						}
						else if (pass == 2)
							SplitEarcut(ear, triangles);
						break;
					}
				}
			}

			static int Sign(const double value) { return value > 0.0 ? 1 : (value < 0.0 ? -1 : 0); }

			static bool OnSegment(const EarNode *p, const EarNode *q, const EarNode *r)
			{
				return q->x <= std::max(p->x, r->x) && q->x >= std::min(p->x, r->x) && q->y <= std::max(p->y, r->y) && q->y >= std::min(p->y, r->y);
			}

			static bool Intersects(const EarNode *p1, const EarNode *q1, const EarNode *p2, const EarNode *q2)
			{
				const int o1 = Sign(Area(p1, q1, p2));
				const int o2 = Sign(Area(p1, q1, q2));
				const int o3 = Sign(Area(p2, q2, p1));
				const int o4 = Sign(Area(p2, q2, q1));
				if (o1 != o2 && o3 != o4)
					return true;
				if (o1 == 0 && OnSegment(p1, p2, q1)) return true;
				if (o2 == 0 && OnSegment(p1, q2, q1)) return true;
				if (o3 == 0 && OnSegment(p2, p1, q2)) return true;
				if (o4 == 0 && OnSegment(p2, q1, q2)) return true;
				return false;
			}

			static bool LocallyInside(const EarNode *a, const EarNode *b)
			{
				return Area(a->prev, a, a->next) < 0.0 ? Area(a, b, a->next) >= 0.0 && Area(a, a->prev, b) >= 0.0
													  : Area(a, b, a->prev) < 0.0 || Area(a, a->next, b) < 0.0;
			}

			static bool MiddleInside(const EarNode *a, const EarNode *b)
			{
				const EarNode *p = a;
				bool inside = false;
				const double px = (a->x + b->x) * 0.5;
				const double py = (a->y + b->y) * 0.5;
				do
				{
					if ((p->y > py) != (p->next->y > py) && p->next->y != p->y && px < (p->next->x - p->x) * (py - p->y) / (p->next->y - p->y) + p->x)
						inside = !inside;
					p = p->next;
				} while (p != a);
				return inside;
			}

			static bool IntersectsPolygon(const EarNode *a, const EarNode *b)
			{
				const EarNode *p = a;
				do
				{
					if (p->index != a->index && p->next->index != a->index && p->index != b->index && p->next->index != b->index && Intersects(p, p->next, a, b))
						return true;
					p = p->next;
				} while (p != a);
				return false;
			}

			static bool IsValidDiagonal(const EarNode *a, const EarNode *b)
			{
				return a->next->index != b->index && a->prev->index != b->index && !IntersectsPolygon(a, b) &&
					   ((LocallyInside(a, b) && LocallyInside(b, a) && MiddleInside(a, b) && (Area(a->prev, a, b->prev) != 0.0 || Area(a, b->prev, b) != 0.0)) ||
						(Equals(a, b) && Area(a->prev, a, a->next) > 0.0 && Area(b->prev, b, b->next) > 0.0));
			}

			EarNode *SplitPolygon(EarNode *a, EarNode *b)
			{
				EarNode *a2 = NewNode(a->index, a->x, a->y);
				EarNode *b2 = NewNode(b->index, b->x, b->y);
				EarNode *an = a->next;
				EarNode *bp = b->prev;

				a->next = b;
				b->prev = a;
				a2->next = an;
				an->prev = a2;
				b2->next = a2;
				a2->prev = b2;
				bp->next = b2;
				b2->prev = bp;
				return b2;
			}

			EarNode *CureLocalIntersections(EarNode *start, std::vector<uint32_t> &triangles)
			{
				EarNode *p = start;
				do
				{
					EarNode *a = p->prev;
					EarNode *b = p->next->next;
					if (!Equals(a, b) && Intersects(a, p, p->next, b) && LocallyInside(a, b) && LocallyInside(b, a))
					{
						triangles.push_back(a->index);
						triangles.push_back(p->index);
						triangles.push_back(b->index);
						RemoveNode(p);
						RemoveNode(p->next);
						p = start = b;
					}
					p = p->next;
				} while (p != start);
				return FilterPoints(p);
			}

			void SplitEarcut(EarNode *start, std::vector<uint32_t> &triangles)
			{
				EarNode *a = start;
				do
				{
					EarNode *b = a->next->next;
					while (b != a->prev)
					{
						if (a->index != b->index && IsValidDiagonal(a, b))
						{
							EarNode *c = SplitPolygon(a, b);
							a = FilterPoints(a, a->next);
							c = FilterPoints(c, c->next);
							EarcutLinked(a, triangles, 0);
							EarcutLinked(c, triangles, 0);
							return;
						}
						b = b->next;
					}
					a = a->next;
				} while (a != start);
			}

			static EarNode *GetLeftmost(EarNode *start)
			{
				EarNode *p = start;
				EarNode *leftmost = start;
				do
				{
					if (p->x < leftmost->x || (p->x == leftmost->x && p->y < leftmost->y))
						leftmost = p;
					p = p->next;
				} while (p != start);
				return leftmost;
			}

			static bool SectorContainsSector(const EarNode *m, const EarNode *p)
			{
				return Area(m->prev, m, p->prev) < 0.0 && Area(p->next, m, m->next) < 0.0;
			}

			static EarNode *FindHoleBridge(EarNode *hole, EarNode *outerNode)
			{
				EarNode *p = outerNode;
				const double hx = hole->x;
				const double hy = hole->y;
				double qx = -std::numeric_limits<double>::infinity();
				EarNode *m = nullptr;

				// Find a segment intersected by a ray from the hole's leftmost point to the left.
				do
				{
					if (hy <= p->y && hy >= p->next->y && p->next->y != p->y)
					{
						const double x = p->x + (hy - p->y) * (p->next->x - p->x) / (p->next->y - p->y);
						if (x <= hx && x > qx)
						{
							qx = x;
							m = p->x < p->next->x ? p : p->next;
							if (x == hx)
								return m;
						}
					}
					p = p->next;
				} while (p != outerNode);

				if (!m)
					return nullptr;

				// Look for points inside the triangle of the hole point, the segment intersection and the endpoint;
				// the one with the smallest angle is the bridge.
				const EarNode *stop = m;
				const double mx = m->x;
				const double my = m->y;
				double tanMin = std::numeric_limits<double>::infinity();
				p = m;
				do
				{
					if (hx >= p->x && p->x >= mx && hx != p->x && PointInTriangle(hy < my ? hx : qx, hy, mx, my, hy < my ? qx : hx, hy, p->x, p->y))
					{
						const double tan = std::abs(hy - p->y) / (hx - p->x);
						if (LocallyInside(p, hole) && (tan < tanMin || (tan == tanMin && (p->x > m->x || (p->x == m->x && SectorContainsSector(m, p))))))
						{
							m = p;
							tanMin = tan;
						}
					}
					p = p->next;
				} while (p != stop);

				return m;
			}

			void EliminateHole(EarNode *hole, EarNode *outerNode)
			{
				EarNode *bridge = FindHoleBridge(hole, outerNode);
				if (!bridge)
					return;
				EarNode *bridgeReverse = SplitPolygon(bridge, hole);
				FilterPoints(bridgeReverse, bridgeReverse->next);
				FilterPoints(bridge, bridge->next);
			}

			EarNode *EliminateHoles(const std::vector<size_t> &ringStarts, EarNode *outerNode)
			{
				std::vector<EarNode *> queue;
				for (size_t ring = 1; ring < ringStarts.size(); ++ring)
				{
					const size_t start = ringStarts[ring];
					const size_t end = ring + 1 < ringStarts.size() ? ringStarts[ring + 1] : m_Points->size();
					if (end - start < 3)
						continue;
					EarNode *list = LinkedList(start, end, false);
					if (!list)
						continue;
					if (list == list->next)
						list->steiner = true;
					queue.push_back(GetLeftmost(list));
				}

				std::ranges::sort(queue, [](const EarNode *a, const EarNode *b) { return a->x < b->x; });
				for (EarNode *hole : queue)
					EliminateHole(hole, outerNode);
				return outerNode;
			}

			const std::vector<Point2> *m_Points = nullptr;
			std::deque<EarNode> m_Nodes;	// Stable addresses
		};
	}

	bool TriangulatePolygon(const std::vector<std::vector<Point2>> &rings, std::vector<Point2> &outPoints, std::vector<uint32_t> &outIndices)
	{
		outPoints.clear();
		outIndices.clear();
		if (rings.empty() || rings[0].size() < 3)
			return false;

		std::vector<size_t> ringStarts;
		for (const std::vector<Point2> &ring : rings)
		{
			if (ring.size() < 3)
				continue;
			ringStarts.push_back(outPoints.size());
			outPoints.insert(outPoints.end(), ring.begin(), ring.end());
		}
		if (ringStarts.empty())
			return false;

		Earcut earcut;
		if (!earcut.Run(outPoints, ringStarts, outIndices))
			return false;

		// Wind every triangle counter-clockwise seen from above (+Y), whatever the input winding was.
		for (size_t i = 0; i + 2 < outIndices.size(); i += 3)
		{
			const Point2 &a = outPoints[outIndices[i]];
			const Point2 &b = outPoints[outIndices[i + 1]];
			const Point2 &c = outPoints[outIndices[i + 2]];
			const double normalY = (b.z - a.z) * (c.x - a.x) - (b.x - a.x) * (c.z - a.z);
			if (normalY < 0.0)
				std::swap(outIndices[i + 1], outIndices[i + 2]);
		}
		return true;
	}

	// -------------------------------------------------------

	void BuildPolylineStrip(const std::vector<Point2> &path, const bool closed, const double width, std::vector<Point2> &outPoints, std::vector<uint32_t> &outIndices)
	{
		outPoints.clear();
		outIndices.clear();
		const size_t count = path.size();
		if (count < 2)
			return;

		const double half = 0.5 * width;
		const auto direction = [&](const size_t from, const size_t to) -> Point2
		{
			const double dx = path[to].x - path[from].x;
			const double dz = path[to].z - path[from].z;
			const double length = std::hypot(dx, dz);
			return length > 1e-9 ? Point2{ dx / length, dz / length } : Point2{ 1.0, 0.0 };
		};

		// Offset each point along the miter of its two neighbouring segments (clamped so sharp corners don't spike).
		for (size_t i = 0; i < count; ++i)
		{
			Point2 in, out;
			if (closed)
			{
				in = direction((i + count - 1) % count, i);
				out = direction(i, (i + 1) % count);
			}
			else
			{
				in = i > 0 ? direction(i - 1, i) : direction(0, 1);
				out = i + 1 < count ? direction(i, i + 1) : direction(count - 2, count - 1);
			}

			Point2 miter = { in.x + out.x, in.z + out.z };
			const double miterLength = std::hypot(miter.x, miter.z);
			if (miterLength < 1e-6)
				miter = in;
			else
				miter = { miter.x / miterLength, miter.z / miterLength };

			// Perpendicular (rotate the tangent by 90 degrees in the ground plane).
			const Point2 normal = { -miter.z, miter.x };
			const double cosHalfAngle = normal.x * -in.z + normal.z * in.x;
			const double scale = half / std::clamp(std::abs(cosHalfAngle), 0.35, 1.0);

			outPoints.push_back({ path[i].x + normal.x * scale, path[i].z + normal.z * scale });
			outPoints.push_back({ path[i].x - normal.x * scale, path[i].z - normal.z * scale });
		}

		const size_t segments = closed ? count : count - 1;
		for (size_t i = 0; i < segments; ++i)
		{
			const uint32_t a = static_cast<uint32_t>(i * 2);
			const uint32_t b = static_cast<uint32_t>(((i + 1) % count) * 2);
			outIndices.insert(outIndices.end(), { a, b, a + 1, a + 1, b, b + 1 });
		}

		for (size_t i = 0; i + 2 < outIndices.size(); i += 3)
		{
			const Point2 &a = outPoints[outIndices[i]];
			const Point2 &b = outPoints[outIndices[i + 1]];
			const Point2 &c = outPoints[outIndices[i + 2]];
			const double normalY = (b.z - a.z) * (c.x - a.x) - (b.x - a.x) * (c.z - a.z);
			if (normalY < 0.0)
				std::swap(outIndices[i + 1], outIndices[i + 2]);
		}
	}

	// -------------------------------------------------------

	Point2 HeadingToDirection(const double headingDegrees)
	{
		const double radians = headingDegrees * kDegToRad;
		return { std::sin(radians), -std::cos(radians) };
	}

	double DirectionToHeading(const Point2 &direction)
	{
		double heading = std::atan2(direction.x, -direction.z) * kRadToDeg;
		if (heading < 0.0)
			heading += 360.0;
		return heading;
	}

	namespace
	{
		void RectangleCorners(const Point2 &start, const Point2 &end, const double width, Point2 corners[4])
		{
			double dx = end.x - start.x;
			double dz = end.z - start.z;
			const double length = std::hypot(dx, dz);
			if (length < 1e-9)
			{
				dx = 0.0;
				dz = -1.0;
			}
			else
			{
				dx /= length;
				dz /= length;
			}
			const double half = 0.5 * width;
			const Point2 side = { -dz * half, dx * half };	// Left of the direction of travel, seen from above
			corners[0] = { start.x + side.x, start.z + side.z };
			corners[1] = { start.x - side.x, start.z - side.z };
			corners[2] = { end.x - side.x, end.z - side.z };
			corners[3] = { end.x + side.x, end.z + side.z };
		}
	}

	void GetRunwayCorners(const AptRunway &runway, const GeoProjection &projection, Point2 corners[4], const bool includeBlastPads)
	{
		Point2 start = projection.ToLocal(runway.ends[0].latitude, runway.ends[0].longitude);
		Point2 end = projection.ToLocal(runway.ends[1].latitude, runway.ends[1].longitude);
		if (includeBlastPads)
		{
			double dx = end.x - start.x;
			double dz = end.z - start.z;
			const double length = std::hypot(dx, dz);
			if (length > 1e-9)
			{
				dx /= length;
				dz /= length;
				start = { start.x - dx * runway.ends[0].blastPad, start.z - dz * runway.ends[0].blastPad };
				end = { end.x + dx * runway.ends[1].blastPad, end.z + dz * runway.ends[1].blastPad };
			}
		}
		RectangleCorners(start, end, runway.width, corners);
	}

	void GetWaterRunwayCorners(const AptWaterRunway &runway, const GeoProjection &projection, Point2 corners[4])
	{
		RectangleCorners(projection.ToLocal(runway.ends[0].latitude, runway.ends[0].longitude), projection.ToLocal(runway.ends[1].latitude, runway.ends[1].longitude), runway.width, corners);
	}

	void GetHelipadCorners(const AptHelipad &helipad, const GeoProjection &projection, Point2 corners[4])
	{
		const Point2 center = projection.ToLocal(helipad.latitude, helipad.longitude);
		const Point2 direction = HeadingToDirection(helipad.heading);
		const double half = 0.5 * helipad.length;
		const Point2 start = { center.x - direction.x * half, center.z - direction.z * half };
		const Point2 end = { center.x + direction.x * half, center.z + direction.z * half };
		RectangleCorners(start, end, helipad.width, corners);
	}
}

// -------------------------------------------------------

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
 * xp_overlay.cpp
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#include "xp_overlay.h"
#include "xp_facade.h"
#include "xp_text.h"
#include <algorithm>
#include <cmath>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	namespace fs = std::filesystem;

	namespace
	{
		constexpr double kPi = 3.14159265358979323846;
		constexpr double kDegToRad = kPi / 180.0;
		constexpr float RUNWAY_MARKINGS_LIFT = 0.008f;	// Runway markings above their runway (below the apt.dat lines, 0.03 up)

		// Draw heights above the ground for draped overlay groups (LAYER_GROUP), so stacked layers don't fight
		constexpr float LAYER_BASE_HEIGHT = 0.4f;
		constexpr float LAYER_STEP = 0.012f;
		constexpr float LAYER_OFFSET_STEP = 0.002f;

		std::string Lower(std::string_view text)
		{
			return ToLower(text);
		}

		bool EndsWith(const std::string &text, const char *suffix)
		{
			const std::string lower = Lower(text);
			return lower.ends_with(suffix);
		}

		/* @brief Texture path relative to the asset file. */
		fs::path TexturePath(const fs::path &assetFile, const std::string &texture)
		{
			if (texture.empty())
				return {};
			return assetFile.parent_path() / fs::path(texture).lexically_normal();
		}

		/* @brief Object path next to the asset, or a library path for the resolver. */
		std::string ObjectReference(const fs::path &assetFile, const std::string &object)
		{
			if (object.starts_with("lib/"))
				return object;
			return (assetFile.parent_path() / fs::path(object)).lexically_normal().generic_string();
		}

		/* @brief Deterministic pseudo-random in [0, 1). */
		float Random01(uint32_t &seed)
		{
			seed = seed * 1664525u + 1013904223u;
			return static_cast<float>((seed >> 8) & 0xFFFFFF) / 16777216.0f;
		}

		void AddVertex(OverlayMesh &mesh, const double x, const double y, const double z, const double u, const double v, const float nx = 0.0f, const float ny = 1.0f, const float nz = 0.0f)
		{
			OverlayVertex vertex;
			vertex.x = static_cast<float>(x);
			vertex.y = static_cast<float>(y);
			vertex.z = static_cast<float>(z);
			vertex.u = static_cast<float>(u);
			vertex.v = static_cast<float>(v);
			vertex.nx = nx;
			vertex.ny = ny;
			vertex.nz = nz;
			mesh.vertices.push_back(vertex);
			mesh.bounds.Add(Point2{ x, z });
		}

		/* @brief Adds a quad (a, b, c, d in order) with per-corner UVs; wound so the given normal is the front. */
		void AddQuad(OverlayMesh &mesh, const double (&corners)[4][3], const double (&uvs)[4][2], const float normal[3])
		{
			const uint32_t base = static_cast<uint32_t>(mesh.vertices.size());
			for (int i = 0; i < 4; ++i)
				AddVertex(mesh, corners[i][0], corners[i][1], corners[i][2], uvs[i][0], uvs[i][1], normal[0], normal[1], normal[2]);

			// Orient against the normal
			const double ax = corners[1][0] - corners[0][0], ay = corners[1][1] - corners[0][1], az = corners[1][2] - corners[0][2];
			const double bx = corners[2][0] - corners[0][0], by = corners[2][1] - corners[0][1], bz = corners[2][2] - corners[0][2];
			const double cx = ay * bz - az * by, cy = az * bx - ax * bz, cz = ax * by - ay * bx;
			const bool flip = cx * normal[0] + cy * normal[1] + cz * normal[2] < 0.0;
			if (!flip)
				mesh.indices.insert(mesh.indices.end(), { base, base + 1, base + 2, base, base + 2, base + 3 });
			else
				mesh.indices.insert(mesh.indices.end(), { base, base + 2, base + 1, base, base + 3, base + 2 });
		}

		/* @brief Local point of a DSF point (lon / lat planes 0, 1). */
		Point2 LocalOf(const GeoProjection &projection, const DsfPoint &point)
		{
			return projection.ToLocal(point.v[1], point.v[0]);
		}

		/**
		 * @brief The UV given with the input point at a triangulated point. Triangulation may reorder points, reverse
		 * a ring's winding or duplicate points to bridge holes, so UVs are matched by position, never by index.
		 */
		std::pair<double, double> UvAt(const Point2 &point, const std::vector<Point2> &source, const std::vector<std::pair<double, double>> &uvs)
		{
			size_t best = 0;
			double bestDistance = 1e300;
			for (size_t i = 0; i < source.size() && i < uvs.size(); ++i)
			{
				const double dx = source[i].x - point.x, dz = source[i].z - point.z;
				const double distance = dx * dx + dz * dz;
				if (distance < bestDistance)
				{
					bestDistance = distance;
					best = i;
				}
			}
			return best < uvs.size() ? uvs[best] : std::pair<double, double>(0.0, 0.0);
		}

		/**
		 * @brief Flattens a closed UV mapped winding with curves (8 planes: lon, lat, control lon, control lat, s, t,
		 * control s, control t). Texture coordinates follow the same curves as the positions, with the bezier convention
		 * of FlattenWinding: a point's control leads out of it, the next point's mirrored control leads in.
		 */
		void FlattenUvWinding(const std::vector<DsfPoint> &winding, const GeoProjection &projection, const double maxSegmentLength, std::vector<Point2> &points, std::vector<std::pair<double, double>> &uvs)
		{
			points.clear();
			uvs.clear();
			const size_t count = winding.size();
			if (count == 0)
				return;

			const auto curved = [](const DsfPoint &point) { return point.planes >= 8 && (std::abs(point.v[2] - point.v[0]) > 1e-9 || std::abs(point.v[3] - point.v[1]) > 1e-9); };
			const auto cubic = [](const double a, const double b, const double c, const double d, const double t)
			{
				const double u = 1.0 - t;
				return u * u * u * a + 3.0 * u * u * t * b + 3.0 * u * t * t * c + t * t * t * d;
			};

			points.push_back(LocalOf(projection, winding[0]));
			uvs.emplace_back(winding[0].v[4], winding[0].v[5]);
			for (size_t i = 0; i < count; ++i)
			{
				const DsfPoint &from = winding[i];
				const DsfPoint &to = winding[(i + 1) % count];
				const Point2 p0 = LocalOf(projection, from), p3 = LocalOf(projection, to);
				const double s0 = from.v[4], t0 = from.v[5], s3 = to.v[4], t3 = to.v[5];
				const bool fromCurved = curved(from), toCurved = curved(to);
				if (std::hypot(p3.x - p0.x, p3.z - p0.z) < 1e-4 || (!fromCurved && !toCurved))
				{
					points.push_back(p3);
					uvs.emplace_back(s3, t3);
					continue;
				}

				const Point2 p1 = fromCurved ? projection.ToLocal(from.v[3], from.v[2]) : p0;
				const double s1 = fromCurved ? from.v[6] : s0, t1 = fromCurved ? from.v[7] : t0;
				Point2 p2 = p3;
				double s2 = s3, t2 = t3;
				if (toCurved)
				{
					const Point2 control = projection.ToLocal(to.v[3], to.v[2]);
					p2 = { 2.0 * p3.x - control.x, 2.0 * p3.z - control.z };
					s2 = 2.0 * s3 - to.v[6];
					t2 = 2.0 * t3 - to.v[7];
				}
				const double estimate = std::hypot(p1.x - p0.x, p1.z - p0.z) + std::hypot(p2.x - p1.x, p2.z - p1.z) + std::hypot(p3.x - p2.x, p3.z - p2.z);
				const int segments = std::clamp(static_cast<int>(std::ceil(estimate / std::max(0.1, maxSegmentLength))), 4, 48);
				for (int k = 1; k <= segments; ++k)
				{
					const double t = static_cast<double>(k) / segments;
					points.push_back({ cubic(p0.x, p1.x, p2.x, p3.x, t), cubic(p0.z, p1.z, p2.z, p3.z, t) });
					uvs.emplace_back(cubic(s0, s1, s2, s3, t), cubic(t0, t1, t2, t3, t));
				}
			}

			// The last point closes back onto the first; co-located points (split nodes) collapse into one
			points.pop_back();
			uvs.pop_back();
			std::vector<Point2> keptPoints;
			std::vector<std::pair<double, double>> keptUvs;
			for (size_t i = 0; i < points.size(); ++i)
			{
				if (!keptPoints.empty() && std::hypot(points[i].x - keptPoints.back().x, points[i].z - keptPoints.back().z) < 1e-4)
					continue;
				keptPoints.push_back(points[i]);
				keptUvs.push_back(uvs[i]);
			}
			if (keptPoints.size() > 1 && std::hypot(keptPoints.front().x - keptPoints.back().x, keptPoints.front().z - keptPoints.back().z) < 1e-4)
			{
				keptPoints.pop_back();
				keptUvs.pop_back();
			}
			points.swap(keptPoints);
			uvs.swap(keptUvs);
		}

		std::string BaseName(const std::string &path)
		{
			const size_t slash = path.find_last_of("/\\");
			return slash == std::string::npos ? path : path.substr(slash + 1);
		}

		/**
		 * @brief A flat polygon at height y repeating the texture rectangle s1 t1 - s2 t2 (a type 1 ROOF_SCALE roof): the
		 * rectangle covers metersS along the heading's s axis by metersT across, the polygon's center takes the texture
		 * point centerS / centerT, and the triangles are cut along the repeats so every piece maps onto the rectangle
		 * (a sampler can't repeat part of a texture). Axes as AppendTexturedPolygon's.
		 */
		void AppendRepeatedRectangle(OverlayMesh &mesh, const std::vector<std::vector<Point2>> &rings, const float y, const float s1, const float t1, const float s2,
			const float t2, const double metersS, const double metersT, const float centerS, const float centerT, const double headingDegrees)
		{
			std::vector<Point2> points;
			std::vector<uint32_t> indices;
			if (!TriangulatePolygon(rings, points, indices) || s2 <= s1 || t2 <= t1)
				return;

			// Each point in repeats of the rectangle: meters along the texture axes from the polygon's center, over the rectangle's meters
			const double angle = headingDegrees * kDegToRad;
			const double c = std::cos(angle), s = std::sin(angle);
			std::vector<std::pair<double, double>> meters;
			meters.reserve(points.size());
			double minU = 1e300, maxU = -1e300, minV = 1e300, maxV = -1e300;
			for (const Point2 &point : points)
			{
				const double east = point.x, north = -point.z;
				const double u = east * c - north * s, v = east * s + north * c;
				meters.emplace_back(u, v);
				minU = std::min(minU, u);
				maxU = std::max(maxU, u);
				minV = std::min(minV, v);
				maxV = std::max(maxV, v);
			}
			const double sizeU = std::max(0.01, metersS), sizeV = std::max(0.01, metersT);
			const double offsetU = (centerS - s1) / (s2 - s1) - 0.5 * (minU + maxU) / sizeU;
			const double offsetV = (centerT - t1) / (t2 - t1) - 0.5 * (minV + maxV) / sizeV;

			struct Corner
			{
				double a, b;	// Repeats along s and t
				double x, z;	// World position
			};
			const auto clip = [](std::vector<Corner> &polygon, const bool alongA, const double limit, const bool keepAbove)
			{
				std::vector<Corner> out;
				for (size_t i = 0; i < polygon.size(); ++i)
				{
					const Corner &p = polygon[i], &q = polygon[(i + 1) % polygon.size()];
					const double dp = (alongA ? p.a : p.b) - limit, dq = (alongA ? q.a : q.b) - limit;
					const bool inP = keepAbove ? dp >= 0.0 : dp <= 0.0, inQ = keepAbove ? dq >= 0.0 : dq <= 0.0;
					if (inP)
						out.push_back(p);
					if (inP != inQ)
					{
						const double f = dp / (dp - dq);
						out.push_back({ p.a + (q.a - p.a) * f, p.b + (q.b - p.b) * f, p.x + (q.x - p.x) * f, p.z + (q.z - p.z) * f });
					}
				}
				polygon.swap(out);
			};

			const float spanS = s2 - s1, spanT = t2 - t1;
			for (size_t i = 0; i + 2 < indices.size(); i += 3)
			{
				Corner triangle[3];
				double minA = 1e300, maxA = -1e300, minB = 1e300, maxB = -1e300;
				for (int k = 0; k < 3; ++k)
				{
					const uint32_t index = indices[i + k];
					triangle[k] = { meters[index].first / sizeU + offsetU, meters[index].second / sizeV + offsetV, points[index].x, points[index].z };
					minA = std::min(minA, triangle[k].a);
					maxA = std::max(maxA, triangle[k].a);
					minB = std::min(minB, triangle[k].b);
					maxB = std::max(maxB, triangle[k].b);
				}
				// One piece per repeat the triangle covers (a roof needing more than 4096 per triangle keeps one piece, its texture clamped)
				const double firstA = std::floor(minA), firstB = std::floor(minB);
				const double columns = std::max(1.0, std::ceil(maxA) - firstA), rows = std::max(1.0, std::ceil(maxB) - firstB);
				const bool cut = columns * rows <= 4096.0;
				for (double column = 0.0; column < (cut ? columns : 1.0); column += 1.0)
				{
					for (double row = 0.0; row < (cut ? rows : 1.0); row += 1.0)
					{
						const double a0 = firstA + column, b0 = firstB + row;
						std::vector<Corner> piece(std::begin(triangle), std::end(triangle));
						if (cut)
						{
							clip(piece, true, a0, true);
							clip(piece, true, a0 + 1.0, false);
							clip(piece, false, b0, true);
							clip(piece, false, b0 + 1.0, false);
						}
						if (piece.size() < 3)
							continue;
						const uint32_t base = static_cast<uint32_t>(mesh.vertices.size());
						for (const Corner &corner : piece)
						{
							const double fa = std::clamp(corner.a - a0, 0.0, 1.0), fb = std::clamp(corner.b - b0, 0.0, 1.0);
							AddVertex(mesh, corner.x, y, corner.z, s1 + spanS * fa, t1 + spanT * fb);
						}
						for (uint32_t k = 1; k + 1 < piece.size(); ++k)
						{
							mesh.indices.push_back(base);
							mesh.indices.push_back(base + k);
							mesh.indices.push_back(base + k + 1);
						}
					}
				}
			}
		}
	}

	// -------------------------------------------------------

	const char *GetOverlayKindName(const OverlayKind kind)
	{
		switch (kind)
		{
			case OverlayKind::DrapedPolygon:	return "Draped polygon";
			case OverlayKind::Line:				return "Line";
			case OverlayKind::Forest:			return "Forest";
			case OverlayKind::Facade:			return "Facade";
			case OverlayKind::Autogen:			return "Autogen";
			case OverlayKind::Pavement:			return "Pavement";
			case OverlayKind::Runway:			return "Runway";
			case OverlayKind::Helipad:			return "Helipad";
			case OverlayKind::PaintedLine:		return "Painted line";
			case OverlayKind::PavementLine:		return "Pavement edge line";
			case OverlayKind::Road:				return "Road";
		}
		return "Overlay";
	}

	// -------------------------------------------------------

	void AppendTexturedPolygon(OverlayMesh &mesh, const std::vector<std::vector<Point2>> &rings, const float y, const float scaleS, const float scaleT, const double headingDegrees, const Point2 origin)
	{
		std::vector<Point2> points;
		std::vector<uint32_t> indices;
		if (!TriangulatePolygon(rings, points, indices))
			return;

		// UVs: meters along the texture axes (rotated by the heading) over the scale
		const double angle = headingDegrees * kDegToRad;
		const double c = std::cos(angle), s = std::sin(angle);
		const double invS = scaleS > 0.001f ? 1.0 / scaleS : 0.001;
		const double invT = scaleT > 0.001f ? 1.0 / scaleT : 0.001;
		mesh.tangentX = static_cast<float>(c);	// u grows east x cos - north x sin: (cos, sin) in x / z
		mesh.tangentZ = static_cast<float>(s);
		const uint32_t base = static_cast<uint32_t>(mesh.vertices.size());
		for (const Point2 &point : points)
		{
			// Texture t runs north (-z); a heading turns the texture clockwise
			const double east = point.x - origin.x, north = origin.z - point.z;
			const double u = (east * c - north * s) * invS;
			const double v = (east * s + north * c) * invT;
			AddVertex(mesh, point.x, y, point.z, u, v);
		}
		for (const uint32_t index : indices)
			mesh.indices.push_back(base + index);
	}

	namespace
	{
		/**
		 * Marking atlas layout, in pixels of a 4096 x 2048 atlas with the origin top-left (AEP's Runway_Markings_Black.png;
		 * X-Plane 12's 2048 x 1024 RWY_markings_*.dds have the same layout at half size). Rectangles are the paint; the
		 * quads take a border around it (the atlas's dark outline, channel G, reaches 6 px past the paint).
		 */
		struct AtlasRect
		{
			float x0, y0, x1, y1;
		};

		constexpr float MARKING_ATLAS_WIDTH = 4096.0f;
		constexpr float MARKING_ATLAS_HEIGHT = 2048.0f;
		constexpr float MARKING_BORDER = 8.0f;
		constexpr AtlasRect MARKING_THRESHOLD_BAR = { 2050.0f, 596.0f, 2110.0f, 2028.0f };	// One of eight long bars
		constexpr AtlasRect MARKING_TOUCHDOWN_BAR = { 322.0f, 1316.0f, 382.0f, 2028.0f };	// One of the short bars
		constexpr AtlasRect MARKING_AIMING_POINT = { 2994.0f, 596.0f, 3278.0f, 2028.0f };	// The wide block
		constexpr AtlasRect MARKING_CENTERLINE = { 98.0f, 8.0f, 126.0f, 1220.0f };			// A stripe the length of a dash
		constexpr AtlasRect MARKING_EDGE_STRIPE = { 4034.0f, 0.0f, 4062.0f, 2048.0f };		// Full height: repeats along v
		constexpr float MARKING_GLYPH_FIRST_CENTER = 296.0f;	// "1"; then 2..9, 0, L, R, C every 288 px
		constexpr float MARKING_GLYPH_PITCH = 288.0f;
		constexpr AtlasRect MARKING_GLYPH_PAINT = { -76.0f, 54.0f, 76.0f, 522.0f };			// Around the glyph's center x
		constexpr AtlasRect MARKING_GLYPH_QUAD = { -100.0f, 30.0f, 100.0f, 550.0f };		// Covers the widest glyph (4) and the tails (6, 9)

		// FAA AC 150/5340-1 dimensions, meters
		constexpr double THRESHOLD_SETBACK = 6.1;			// Threshold to the bars
		constexpr double THRESHOLD_BAR_LENGTH = 45.72;
		constexpr double THRESHOLD_BAR_WIDTH = 1.75;		// Bars and the gaps between them; the center gap is two
		constexpr double DESIGNATOR_GAP = 12.2;				// Bars to designator, designator to centerline
		constexpr double GLYPH_LENGTH = 18.29;
		constexpr double GLYPH_WIDTH = 6.1;
		constexpr double GLYPH_SPACING = 9.0;				// Between the centers of two digits
		constexpr double LETTER_GAP = 6.1;					// Letter (nearer the threshold) to number
		constexpr double CENTERLINE_DASH = 36.58;
		constexpr double CENTERLINE_GAP = 24.38;
		constexpr double AIMING_POINT_START = 306.0;
		constexpr double AIMING_POINT_LENGTH = 45.72;
		constexpr double TOUCHDOWN_BAR_LENGTH = 22.86;
		constexpr double TOUCHDOWN_BAR_WIDTH = 1.83;
		constexpr double TOUCHDOWN_BAR_GAP = 1.52;
		constexpr double EDGE_STRIPE_WIDTH = 0.91;
		constexpr double EDGE_STRIPE_REPEAT = 60.0;			// Meters per repeat of the edge stripe's texture
		constexpr double MARKED_LENGTH_MIN = 1200.0;		// Aiming points and touchdown zones only on runways this long

		/* @brief 0 none, 1 visual, 2 non-precision, 3 precision (FAA, UK and EASA codes alike). */
		int MarkingClass(const int code)
		{
			switch (code)
			{
				case 1:					return 1;
				case 2: case 4: case 6:	return 2;
				case 3: case 5: case 7:	return 3;
				default:				return 0;
			}
		}

		int GlyphIndex(const char glyph)
		{
			if (glyph >= '1' && glyph <= '9')
				return glyph - '1';
			switch (glyph)
			{
				case '0':	return 9;
				case 'L':	return 10;
				case 'R':	return 11;
				case 'C':	return 12;
				default:	return -1;
			}
		}

		/* @brief A runway end's frame: its end point, the direction into the runway and the right of it. */
		struct EndFrame
		{
			Point2 origin;
			Point2 forward;
			Point2 right;

			Point2 At(const double along, const double lateral) const
			{
				return { origin.x + forward.x * along + right.x * lateral, origin.z + forward.z * along + right.z * lateral };
			}
		};

		/**
		 * @brief One marking quad: paint of paintLength x paintWidth meters centered at (along, lateral), the sprite's
		 * border around it. repeat > 0: the texture repeats that many times along (the sprite must span the atlas height).
		 */
		void AppendMarkingQuad(OverlayMesh &mesh, const EndFrame &frame, const float y, const double along, const double lateral,
			const double paintLength, const double paintWidth, const RunwayMarkingSprite &sprite, const double repeat = 0.0)
		{
			const double length = repeat > 0.0 ? paintLength : paintLength / std::max(0.01f, sprite.paintLength);
			const double width = paintWidth / std::max(0.01f, sprite.paintWidth);
			const double v1 = repeat > 0.0 ? sprite.v0 + repeat : sprite.v1;
			const Point2 corners[4] =
			{
				frame.At(along - 0.5 * length, lateral - 0.5 * width),
				frame.At(along - 0.5 * length, lateral + 0.5 * width),
				frame.At(along + 0.5 * length, lateral + 0.5 * width),
				frame.At(along + 0.5 * length, lateral - 0.5 * width)
			};
			const double uvs[4][2] = { { sprite.u0, sprite.v0 }, { sprite.u1, sprite.v0 }, { sprite.u1, v1 }, { sprite.u0, v1 } };
			const uint32_t base = static_cast<uint32_t>(mesh.vertices.size());
			for (int i = 0; i < 4; ++i)
				AddVertex(mesh, corners[i].x, y, corners[i].z, uvs[i][0], uvs[i][1]);

			// Counter-clockwise from above, like the rest of the overlay geometry
			uint32_t indices[6] = { 0, 1, 2, 0, 2, 3 };
			for (int i = 0; i < 6; i += 3)
			{
				const Point2 &a = corners[indices[i]], &b = corners[indices[i + 1]], &c = corners[indices[i + 2]];
				if ((b.z - a.z) * (c.x - a.x) - (b.x - a.x) * (c.z - a.z) < 0.0)
					std::swap(indices[i + 1], indices[i + 2]);
			}
			for (const uint32_t index : indices)
				mesh.indices.push_back(base + index);
		}
	}

	RunwayMarkingSprite GetRunwayMarkingSprite(const RunwayMarking marking, const char glyph)
	{
		AtlasRect paint{};
		AtlasRect quad{};
		const auto bordered = [](const AtlasRect &r) { return AtlasRect{ r.x0 - MARKING_BORDER, r.y0 - MARKING_BORDER, r.x1 + MARKING_BORDER, r.y1 + MARKING_BORDER }; };
		switch (marking)
		{
			case RunwayMarking::ThresholdBar:	paint = MARKING_THRESHOLD_BAR; quad = bordered(paint); break;
			case RunwayMarking::TouchdownBar:	paint = MARKING_TOUCHDOWN_BAR; quad = bordered(paint); break;
			case RunwayMarking::AimingPoint:	paint = MARKING_AIMING_POINT; quad = bordered(paint); break;
			case RunwayMarking::Centerline:		paint = MARKING_CENTERLINE; quad = bordered(paint); break;
			case RunwayMarking::EdgeStripe:
				paint = MARKING_EDGE_STRIPE;
				quad = { paint.x0 - MARKING_BORDER, 0.0f, paint.x1 + MARKING_BORDER, MARKING_ATLAS_HEIGHT };
				break;
			case RunwayMarking::Glyph:
			{
				const float center = MARKING_GLYPH_FIRST_CENTER + MARKING_GLYPH_PITCH * static_cast<float>(std::max(0, GlyphIndex(glyph)));
				paint = { center + MARKING_GLYPH_PAINT.x0, MARKING_GLYPH_PAINT.y0, center + MARKING_GLYPH_PAINT.x1, MARKING_GLYPH_PAINT.y1 };
				quad = { center + MARKING_GLYPH_QUAD.x0, MARKING_GLYPH_QUAD.y0, center + MARKING_GLYPH_QUAD.x1, MARKING_GLYPH_QUAD.y1 };
				break;
			}
		}

		// X-Plane UVs: v up, so the atlas's top row is v = 1
		RunwayMarkingSprite sprite;
		sprite.u0 = quad.x0 / MARKING_ATLAS_WIDTH;
		sprite.u1 = quad.x1 / MARKING_ATLAS_WIDTH;
		sprite.v0 = 1.0f - quad.y1 / MARKING_ATLAS_HEIGHT;
		sprite.v1 = 1.0f - quad.y0 / MARKING_ATLAS_HEIGHT;
		sprite.paintWidth = (paint.x1 - paint.x0) / (quad.x1 - quad.x0);
		sprite.paintLength = (paint.y1 - paint.y0) / (quad.y1 - quad.y0);
		return sprite;
	}

	bool AppendRunwayMarkings(OverlayMesh &mesh, const AptRunway &runway, const GeoProjection &projection, const float y)
	{
		const Point2 points[2] = { projection.ToLocal(runway.ends[0].latitude, runway.ends[0].longitude), projection.ToLocal(runway.ends[1].latitude, runway.ends[1].longitude) };
		const double length = std::hypot(points[1].x - points[0].x, points[1].z - points[0].z);
		const int classes[2] = { MarkingClass(runway.ends[0].marking), MarkingClass(runway.ends[1].marking) };
		const int strongest = std::max(classes[0], classes[1]);
		if (length < 1.0 || strongest == 0 || runway.width < 1.0f)
			return false;
		const size_t before = mesh.indices.size();
		const double halfWidth = 0.5 * runway.width;

		EndFrame frames[2];
		double clearOfDesignator[2] = { 0.0, 0.0 };	// From each end point: where its centerline may begin
		for (int e = 0; e < 2; ++e)
		{
			const Point2 &from = points[e], &to = points[1 - e];
			EndFrame &frame = frames[e];
			frame.forward = { (to.x - from.x) / length, (to.z - from.z) / length };
			frame.right = { -frame.forward.z, frame.forward.x };	// Heading north (0, -1): east (1, 0)
			frame.origin = from;

			const int cls = classes[e];
			if (cls == 0)
				continue;
			const double threshold = std::clamp(static_cast<double>(runway.ends[e].displacedThreshold), 0.0, 0.5 * length);
			double along = threshold + THRESHOLD_SETBACK;

			// Threshold bars: 4 / 6 / 8 / 12 / 16 by width, a bar's width apart, twice that across the centerline
			if (cls >= 2)
			{
				const int bars = runway.width < 20.0f ? 4 : runway.width < 26.0f ? 6 : runway.width < 38.0f ? 8 : runway.width < 53.0f ? 12 : 16;
				const RunwayMarkingSprite sprite = GetRunwayMarkingSprite(RunwayMarking::ThresholdBar);
				for (int i = 0; i < bars / 2; ++i)
				{
					const double lateral = THRESHOLD_BAR_WIDTH * (1.5 + 2.0 * i);
					if (lateral + 0.5 * THRESHOLD_BAR_WIDTH > halfWidth)
						break;
					AppendMarkingQuad(mesh, frame, y, along + 0.5 * THRESHOLD_BAR_LENGTH, -lateral, THRESHOLD_BAR_LENGTH, THRESHOLD_BAR_WIDTH, sprite);
					AppendMarkingQuad(mesh, frame, y, along + 0.5 * THRESHOLD_BAR_LENGTH, lateral, THRESHOLD_BAR_LENGTH, THRESHOLD_BAR_WIDTH, sprite);
				}
				along += THRESHOLD_BAR_LENGTH + DESIGNATOR_GAP;
			}

			// Designator: the letter nearer the threshold, the number beyond it, both read from the approach
			const std::string &designator = runway.ends[e].designator;
			std::string number;
			char letter = 0;
			for (const char c : designator)
			{
				if (c >= '0' && c <= '9')
					number += c;
				else if (c == 'L' || c == 'R' || c == 'C')
					letter = c;
			}
			if (letter != 0)
			{
				AppendMarkingQuad(mesh, frame, y, along + 0.5 * GLYPH_LENGTH, 0.0, GLYPH_LENGTH, GLYPH_WIDTH, GetRunwayMarkingSprite(RunwayMarking::Glyph, letter));
				along += GLYPH_LENGTH + LETTER_GAP;
			}
			if (!number.empty())
			{
				const double first = -0.5 * GLYPH_SPACING * static_cast<double>(number.size() - 1);
				for (size_t i = 0; i < number.size(); ++i)
					AppendMarkingQuad(mesh, frame, y, along + 0.5 * GLYPH_LENGTH, first + GLYPH_SPACING * static_cast<double>(i), GLYPH_LENGTH, GLYPH_WIDTH, GetRunwayMarkingSprite(RunwayMarking::Glyph, number[i]));
				along += GLYPH_LENGTH;
			}
			clearOfDesignator[e] = along + DESIGNATOR_GAP;

			// Aiming points and (precision) touchdown zone bars, inner edges ~24% of the width apart
			if (cls >= 2 && length >= MARKED_LENGTH_MIN)
			{
				const double inner = std::clamp(0.24 * runway.width, 3.0, 11.0);
				const double aimWidth = runway.width >= 45.0f ? 9.14 : runway.width >= 30.0f ? 6.1 : 3.0;
				if (inner + aimWidth <= halfWidth)
				{
					const RunwayMarkingSprite aim = GetRunwayMarkingSprite(RunwayMarking::AimingPoint);
					const double at = threshold + AIMING_POINT_START + 0.5 * AIMING_POINT_LENGTH;
					AppendMarkingQuad(mesh, frame, y, at, -(inner + 0.5 * aimWidth), AIMING_POINT_LENGTH, aimWidth, aim);
					AppendMarkingQuad(mesh, frame, y, at, inner + 0.5 * aimWidth, AIMING_POINT_LENGTH, aimWidth, aim);
				}
				if (cls == 3)
				{
					static constexpr std::pair<double, int> TOUCHDOWN[] = { { 152.4, 3 }, { 457.2, 2 }, { 609.6, 2 }, { 762.0, 1 }, { 914.4, 1 } };
					const RunwayMarkingSprite bar = GetRunwayMarkingSprite(RunwayMarking::TouchdownBar);
					for (const auto &[start, count] : TOUCHDOWN)
					{
						if (threshold + start + TOUCHDOWN_BAR_LENGTH > 0.5 * length - 30.0)
							break;
						for (int k = 0; k < count; ++k)
						{
							const double lateral = inner + 0.5 * TOUCHDOWN_BAR_WIDTH + k * (TOUCHDOWN_BAR_WIDTH + TOUCHDOWN_BAR_GAP);
							if (lateral + 0.5 * TOUCHDOWN_BAR_WIDTH > halfWidth)
								break;
							const double at = threshold + start + 0.5 * TOUCHDOWN_BAR_LENGTH;
							AppendMarkingQuad(mesh, frame, y, at, -lateral, TOUCHDOWN_BAR_LENGTH, TOUCHDOWN_BAR_WIDTH, bar);
							AppendMarkingQuad(mesh, frame, y, at, lateral, TOUCHDOWN_BAR_LENGTH, TOUCHDOWN_BAR_WIDTH, bar);
						}
					}
				}
			}
		}

		// Centerline dashes between the two designators (measured from end 0)
		{
			const double width = strongest == 3 ? 0.91 : strongest == 2 ? 0.46 : 0.3;
			const RunwayMarkingSprite sprite = GetRunwayMarkingSprite(RunwayMarking::Centerline);
			const double last = length - clearOfDesignator[1];
			for (double at = clearOfDesignator[0]; at + CENTERLINE_DASH <= last; at += CENTERLINE_DASH + CENTERLINE_GAP)
				AppendMarkingQuad(mesh, frames[0], y, at + 0.5 * CENTERLINE_DASH, 0.0, CENTERLINE_DASH, width, sprite);
		}

		// Edge stripes along the full length (non-precision and precision runways)
		if (strongest >= 2 && halfWidth > 4.0 * EDGE_STRIPE_WIDTH)
		{
			const RunwayMarkingSprite sprite = GetRunwayMarkingSprite(RunwayMarking::EdgeStripe);
			const double lateral = halfWidth - EDGE_STRIPE_WIDTH;
			const double repeat = std::max(1.0, std::round(length / EDGE_STRIPE_REPEAT));
			AppendMarkingQuad(mesh, frames[0], y, 0.5 * length, -lateral, length, EDGE_STRIPE_WIDTH, sprite, repeat);
			AppendMarkingQuad(mesh, frames[0], y, 0.5 * length, lateral, length, EDGE_STRIPE_WIDTH, sprite, repeat);
		}

		return mesh.indices.size() > before;
	}

	void AppendTexturedRibbon(OverlayMesh &mesh, const std::vector<Point2> &path, const bool closed, const float y, const float width, const float s1, const float s2, const float scaleT, const bool mirror)
	{
		std::vector<Point2> points;
		std::vector<uint32_t> indices;
		BuildPolylineStrip(path, closed, width, points, indices);
		if (points.size() < 4)
			return;

		// Distance along the path per pair of strip points
		const size_t count = path.size();
		std::vector<double> distance(count, 0.0);
		for (size_t i = 1; i < count; ++i)
			distance[i] = distance[i - 1] + std::hypot(path[i].x - path[i - 1].x, path[i].z - path[i - 1].z);
		const double invT = scaleT > 0.001f ? 1.0 / scaleT : 1.0;

		const uint32_t base = static_cast<uint32_t>(mesh.vertices.size());
		for (size_t i = 0; i < count; ++i)
		{
			double v = distance[i] * invT;
			if (mirror)
			{
				// Alternate direction each repeat so the texture never shows a seam
				const double whole = std::floor(v);
				const double frac = v - whole;
				v = (static_cast<long long>(whole) % 2 == 0) ? frac : 1.0 - frac;
			}
			AddVertex(mesh, points[i * 2].x, y, points[i * 2].z, s1, v);
			AddVertex(mesh, points[i * 2 + 1].x, y, points[i * 2 + 1].z, s2, v);
		}
		for (const uint32_t index : indices)
			mesh.indices.push_back(base + index);
	}

	void AppendOffsetRibbon(OverlayMesh &mesh, const std::vector<Point2> &path, const float y, float left, float right, float sLeft, float sRight, const float scaleT)
	{
		const size_t count = path.size();
		if (count < 2 || left == right)
			return;
		if (left > right)
		{
			std::swap(left, right);
			std::swap(sLeft, sRight);
		}
		const auto direction = [&](const size_t from, const size_t to) -> Point2
		{
			const double dx = path[to].x - path[from].x, dz = path[to].z - path[from].z;
			const double length = std::hypot(dx, dz);
			return length > 1e-9 ? Point2{ dx / length, dz / length } : Point2{ 1.0, 0.0 };
		};

		const double invT = scaleT > 0.001f ? 1.0 / scaleT : 1.0;
		const uint32_t base = static_cast<uint32_t>(mesh.vertices.size());
		double distance = 0.0;
		for (size_t i = 0; i < count; ++i)
		{
			if (i > 0)
				distance += std::hypot(path[i].x - path[i - 1].x, path[i].z - path[i - 1].z);
			const Point2 in = i > 0 ? direction(i - 1, i) : direction(0, 1);
			const Point2 out = i + 1 < count ? direction(i, i + 1) : direction(count - 2, count - 1);
			Point2 miter{ in.x + out.x, in.z + out.z };
			const double miterLength = std::hypot(miter.x, miter.z);
			miter = miterLength < 1e-6 ? in : Point2{ miter.x / miterLength, miter.z / miterLength };

			// Right of travel (+X east with -Z north: heading east, right is +Z), stretched at corners (clamped)
			const Point2 normal{ -miter.z, miter.x };
			const double scale = 1.0 / std::clamp(std::abs(normal.x * -in.z + normal.z * in.x), 0.35, 1.0);
			const double v = distance * invT;
			AddVertex(mesh, path[i].x + normal.x * right * scale, y, path[i].z + normal.z * right * scale, sRight, v);
			AddVertex(mesh, path[i].x + normal.x * left * scale, y, path[i].z + normal.z * left * scale, sLeft, v);
		}
		for (size_t i = 0; i + 1 < count; ++i)
		{
			const uint32_t a = base + static_cast<uint32_t>(i * 2);
			const uint32_t b = a + 2;
			mesh.indices.insert(mesh.indices.end(), { a, b, a + 1, a + 1, b, b + 1 });
		}
	}

	void AppendLinePaint(OverlayMesh &mesh, const LineAsset &asset, const std::vector<Point2> &path, const bool closed, const float y)
	{
		const size_t count = path.size();
		if (count < 2 || asset.segments.empty())
			return;

		// Stations along the path: where it is, its miter (right of travel, stretched at corners) and the distance
		// travelled. A closed line comes back to its first point.
		struct Station
		{
			Point2 position;
			Point2 miter;
			double distance = 0.0;
		};
		const auto direction = [&](const size_t from, const size_t to) -> Point2
		{
			const double dx = path[to].x - path[from].x, dz = path[to].z - path[from].z;
			const double length = std::hypot(dx, dz);
			return length > 1e-9 ? Point2{ dx / length, dz / length } : Point2{ 1.0, 0.0 };
		};
		std::vector<Station> stations;
		const size_t stationCount = closed ? count + 1 : count;
		stations.reserve(stationCount);
		for (size_t s = 0; s < stationCount; ++s)
		{
			const size_t i = s % count;
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
			Point2 miter{ in.x + out.x, in.z + out.z };
			const double miterLength = std::hypot(miter.x, miter.z);
			miter = miterLength < 1e-6 ? in : Point2{ miter.x / miterLength, miter.z / miterLength };
			const Point2 normal{ -miter.z, miter.x };
			const double scale = 1.0 / std::clamp(std::abs(normal.x * -in.z + normal.z * in.x), 0.35, 1.0);

			Station station;
			station.position = path[i];
			station.miter = { normal.x * scale, normal.z * scale };
			if (!stations.empty())
				station.distance = stations.back().distance + std::hypot(path[i].x - stations.back().position.x, path[i].z - stations.back().position.z);
			stations.push_back(station);
		}
		const double total = stations.back().distance;
		if (total <= 1e-6)
			return;

		// A point part way along the path: a station where one is, else on its segment with the segment's own normal
		const auto stationAt = [&](const double distance) -> Station
		{
			if (distance <= 0.0)
				return stations.front();
			if (distance >= total)
				return stations.back();
			size_t k = 0;
			while (k + 2 < stations.size() && stations[k + 1].distance <= distance)
				++k;
			const Station &a = stations[k], &b = stations[k + 1];
			if (std::abs(distance - a.distance) < 1e-9)
				return a;
			if (std::abs(distance - b.distance) < 1e-9)
				return b;
			const double length = b.distance - a.distance;
			const double f = length > 1e-12 ? (distance - a.distance) / length : 0.0;
			const Point2 along = length > 1e-12 ? Point2{ (b.position.x - a.position.x) / length, (b.position.z - a.position.z) / length } : Point2{ 1.0, 0.0 };
			Station station;
			station.position = { a.position.x + (b.position.x - a.position.x) * f, a.position.z + (b.position.z - a.position.z) * f };
			station.miter = { -along.z, along.x };
			station.distance = distance;
			return station;
		};

		// Rows paint in layer order: later rows over earlier ones (drapes are drawn in order within a mesh)
		std::vector<size_t> rows(asset.segments.size());
		for (size_t i = 0; i < rows.size(); ++i)
			rows[i] = i;
		std::stable_sort(rows.begin(), rows.end(), [&](const size_t a, const size_t b) { return asset.segments[a].layer < asset.segments[b].layer; });

		const double scaleS = asset.scaleS;
		const double scaleT = asset.scaleT > 0.001f ? asset.scaleT : 1.0;
		for (const size_t row : rows)
		{
			const LineAsset::Segment &segment = asset.segments[row];

			// The path runs through the row's center pixel: the s1 edge lies right of travel, the s2 edge left
			const double right = (segment.sm - segment.s1) * scaleS;
			const double left = -(segment.s2 - segment.sm) * scaleS;
			if (right - left <= 1e-6)
				continue;

			// Caps (open lines only), each the length its texture rows cover; shrunk together when the line is shorter
			const LineAsset::Cap *startCap = !closed && row < asset.startCaps.size() ? &asset.startCaps[row] : nullptr;
			const LineAsset::Cap *endCap = !closed && row < asset.endCaps.size() ? &asset.endCaps[row] : nullptr;
			double startLength = startCap ? std::max(0.0, static_cast<double>(startCap->t2 - startCap->t1)) * scaleT : 0.0;
			double endLength = endCap ? std::max(0.0, static_cast<double>(endCap->t2 - endCap->t1)) * scaleT : 0.0;
			double capScale = 1.0;
			if (startLength + endLength > total)
			{
				capScale = total / (startLength + endLength);
				startLength *= capScale;
				endLength *= capScale;
			}
			const double bodyStart = startLength, bodyEnd = total - endLength;

			// ALIGN n: the body ends on a whole 1/n of a texture repeat (stretched evenly, where WorldEditor stretches its last piece)
			double alignScale = 1.0;
			if (asset.align > 0 && bodyEnd - bodyStart > 1e-6)
			{
				const double repeats = (bodyEnd - bodyStart) / scaleT;
				const double rounded = std::max(1.0, std::round(repeats * asset.align)) / asset.align;
				alignScale = rounded / repeats;
			}

			// Cuts: the caps' ends and, with MIRROR, every repeat (the texture runs back and forth)
			struct Span
			{
				double from, to;
				int kind;	// 0 start cap, 1 body, 2 end cap
			};
			std::vector<Span> spans;
			if (startLength > 1e-6)
				spans.push_back({ 0.0, startLength, 0 });
			if (bodyEnd - bodyStart > 1e-6)
			{
				const double repeat = scaleT / alignScale;
				if (asset.mirror && (bodyEnd - bodyStart) / repeat < 4096.0)
				{
					for (double from = bodyStart; from < bodyEnd - 1e-6; from += repeat)
						spans.push_back({ from, std::min(from + repeat, bodyEnd), 1 });
				}
				else
					spans.push_back({ bodyStart, bodyEnd, 1 });
			}
			if (endLength > 1e-6)
				spans.push_back({ bodyEnd, total, 2 });

			for (const Span &span : spans)
			{
				const LineAsset::Cap *cap = span.kind == 0 ? startCap : span.kind == 2 ? endCap : nullptr;
				const float sRight = cap ? cap->s1 : segment.s1;
				const float sLeft = cap ? cap->s2 : segment.s2;
				const auto textureV = [&](const double distance) -> double
				{
					if (cap)
						return cap->t1 + (distance - span.from) / (scaleT * capScale);
					const double v = (distance - bodyStart) / scaleT * alignScale;
					if (!asset.mirror)
						return v;
					// Alternate direction each repeat; a span covers one repeat, so its end is that repeat's far edge
					const double whole = std::floor((span.from - bodyStart) / scaleT * alignScale + 1e-9);
					const double frac = std::clamp(v - whole, 0.0, 1.0);
					return static_cast<long long>(whole) % 2 == 0 ? frac : 1.0 - frac;
				};

				std::vector<Station> strip;
				strip.push_back(stationAt(span.from));
				for (const Station &station : stations)
				{
					if (station.distance > span.from + 1e-9 && station.distance < span.to - 1e-9)
						strip.push_back(station);
				}
				strip.push_back(stationAt(span.to));

				const uint32_t base = static_cast<uint32_t>(mesh.vertices.size());
				for (const Station &station : strip)
				{
					const double v = textureV(station.distance);
					AddVertex(mesh, station.position.x + station.miter.x * right, y, station.position.z + station.miter.z * right, sRight, v);
					AddVertex(mesh, station.position.x + station.miter.x * left, y, station.position.z + station.miter.z * left, sLeft, v);
				}
				for (size_t i = 0; i + 1 < strip.size(); ++i)
				{
					const uint32_t a = base + static_cast<uint32_t>(i * 2);
					const uint32_t b = a + 2;
					mesh.indices.insert(mesh.indices.end(), { a, b, a + 1, a + 1, b, b + 1 });
				}
			}
		}
	}

	void ResamplePath(const std::vector<Point2> &path, const bool closed, const double spacing, const double startOffset, std::vector<Point2> &outPoints, std::vector<double> &outHeadings)
	{
		outPoints.clear();
		outHeadings.clear();
		if (path.size() < 2 || spacing <= 0.01)
			return;

		// Each segment places the points in [0, length) so a corner isn't placed twice; an open path ends on its last point.
		const size_t segments = closed ? path.size() : path.size() - 1;
		double carry = startOffset;
		double lastHeading = 0.0;
		for (size_t i = 0; i < segments; ++i)
		{
			const Point2 &a = path[i];
			const Point2 &b = path[(i + 1) % path.size()];
			const double dx = b.x - a.x, dz = b.z - a.z;
			const double length = std::hypot(dx, dz);
			if (length < 1e-6)
				continue;
			const double heading = DirectionToHeading(Point2{ dx / length, dz / length });
			lastHeading = heading;
			double t = carry;
			while (t < length - 1e-6)
			{
				outPoints.push_back({ a.x + dx * (t / length), a.z + dz * (t / length) });
				outHeadings.push_back(heading);
				t += spacing;
			}
			carry = t - length;
		}
		if (!closed)
		{
			const Point2 &end = path.back();
			if (outPoints.empty() || std::hypot(end.x - outPoints.back().x, end.z - outPoints.back().z) > 1e-6)
			{
				outPoints.push_back(end);
				outHeadings.push_back(lastHeading);
			}
		}
	}

	// -------------------------------------------------------

	OverlayBuilder::OverlayBuilder(AssetResolver resolver, GeoProjection projection) : m_Resolver(std::move(resolver)), m_Projection(projection)
	{
	}

	void OverlayBuilder::Warn(OverlayBuild &out, const std::string &message)
	{
		if (m_Warned.emplace(message, true).second)
			out.warnings.push_back(message);
	}

	fs::path OverlayBuilder::ResolveFile(const std::string_view path)
	{
		const std::string key(path);
		auto it = m_Resolved.find(key);
		if (it == m_Resolved.end())
			it = m_Resolved.emplace(key, m_Resolver ? m_Resolver(path) : fs::path()).first;
		return it->second;
	}

	float OverlayBuilder::Height(const Options &options, const std::string_view group, const int offset) const
	{
		const std::string_view name = group.empty() ? std::string_view("terrain") : group;
		if (options.layerHeight)
			return options.layerHeight(name, offset);
		return LAYER_BASE_HEIGHT + LAYER_STEP * static_cast<float>(GetLayerGroupOrder(name)) + LAYER_OFFSET_STEP * static_cast<float>(std::clamp(offset, -5, 5));
	}

	template<typename Asset>
	const Asset *OverlayBuilder::GetCached(std::map<std::string, Cached<Asset>> &cache, const std::string_view path, fs::path *file, OverlayBuild &out,
		bool (*load)(const fs::path &, Asset &, std::string *), const char *what)
	{
		const std::string key(path);
		auto it = cache.find(key);
		if (it == cache.end())
		{
			Cached<Asset> entry;
			entry.file = ResolveFile(path);
			if (entry.file.empty())
				Warn(out, std::string(what) + " not found: " + key);
			else
			{
				auto asset = std::make_unique<Asset>();
				std::string error;
				if (load(entry.file, *asset, &error))
					entry.asset = std::move(asset);
				else
					Warn(out, std::string(what) + " " + key + ": " + error);
			}
			it = cache.emplace(key, std::move(entry)).first;
		}
		if (file)
			*file = it->second.file;
		return it->second.asset.get();
	}

	const PolygonAsset *OverlayBuilder::GetPolygon(const std::string_view path, fs::path *file, OverlayBuild &out) { return GetCached(m_Polygons, path, file, out, &LoadPolygonAsset, "Draped polygon"); }
	const LineAsset *OverlayBuilder::GetLine(const std::string_view path, fs::path *file, OverlayBuild &out) { return GetCached(m_Lines, path, file, out, &LoadLineAsset, "Line"); }
	const ObjectStringAsset *OverlayBuilder::GetString(const std::string_view path, fs::path *file, OverlayBuild &out) { return GetCached(m_Strings, path, file, out, &LoadObjectStringAsset, "Object string"); }
	const ForestAsset *OverlayBuilder::GetForest(const std::string_view path, fs::path *file, OverlayBuild &out) { return GetCached(m_Forests, path, file, out, &LoadForestAsset, "Forest"); }
	const FacadeAsset *OverlayBuilder::GetFacade(const std::string_view path, fs::path *file, OverlayBuild &out) { return GetCached(m_Facades, path, file, out, &LoadFacadeAsset, "Facade"); }
	const AutogenPointAsset *OverlayBuilder::GetAutogen(const std::string_view path, fs::path *file, OverlayBuild &out) { return GetCached(m_Autogens, path, file, out, &LoadAutogenPointAsset, "Autogen point"); }
	const RoadNetworkAsset *OverlayBuilder::GetRoadNetwork(const std::string_view path, fs::path *file, OverlayBuild &out) { return GetCached(m_Roads, path, file, out, &LoadRoadNetworkAsset, "Road network"); }
	const DecalLibraryAsset *OverlayBuilder::GetDecalLibrary(const std::string_view path, fs::path *file, OverlayBuild &out) { return GetCached(m_DecalLibraries, path, file, out, &LoadDecalLibraryAsset, "Decal library"); }

	void OverlayBuilder::GetPolygonUvScale(const PolygonAsset &asset, float &scaleS, float &scaleT)
	{
		// RUNWAY_TILE: SCALE is the whole atlas (X-Plane 12's 4 x 8 runway atlases have square tiles, SCALE 22 44 = 5.5 m
		// slabs, matching the joints in X-Plane's own frames). TEXTURE_TILE: SCALE is one tile.
		const bool perTile = asset.tiled && !asset.runwayTile;
		scaleS = asset.scaleS * (perTile ? static_cast<float>(asset.tilesX) : 1.0f);
		scaleT = asset.scaleT * (perTile ? static_cast<float>(asset.tilesY) : 1.0f);
	}

	void OverlayBuilder::ApplyPolygonShading(OverlayMesh &mesh, const PolygonAsset &asset, const fs::path &assetFile, OverlayBuild &out)
	{
		GetPolygonUvScale(asset, mesh.metersPerU, mesh.metersPerV);
		if (asset.runwayTile && (asset.tilesX > 1 || asset.tilesY > 1))
		{
			mesh.shading = OverlayShading::Tiled;
			mesh.tilesX = asset.tilesX;
			mesh.tilesY = asset.tilesY;
			if (!asset.tileMap.empty() && asset.tileMapWidth > 0 && asset.tileMapHeight > 0)
			{
				mesh.tileMap = TexturePath(assetFile, asset.tileMap);
				mesh.tileMapWidth = asset.tileMapWidth;
				mesh.tileMapHeight = asset.tileMapHeight;
			}
		}

		// Decals: the .pol's own rows, then its DECAL_LIB library's (paths relative to the file that names them)
		const auto add = [&mesh](const PolygonDecal &decal, const fs::path &file)
		{
			OverlayDecal resolved;
			resolved.normal = decal.normal;
			resolved.scaleS = decal.scaleS;
			resolved.scaleT = decal.scaleT;
			resolved.strength = decal.strength;
			resolved.gain = decal.gain;
			resolved.texture = TexturePath(file, decal.texture);
			if (resolved.strength > 0.0f)
				mesh.decals.push_back(std::move(resolved));
		};
		for (const PolygonDecal &decal : asset.decals)
			add(decal, assetFile);
		if (!asset.decal.empty())
		{
			fs::path libraryFile;
			if (const DecalLibraryAsset *library = GetDecalLibrary(asset.decal, &libraryFile, out))
			{
				for (const PolygonDecal &decal : library->decals)
					add(decal, libraryFile);
			}
		}
	}

	void OverlayBuilder::FlattenRoad(const DsfRoadSegment &segment, std::vector<Point2> &out, const Options &options) const
	{
		out.clear();
		const double step = std::max(0.25, options.bezierSegment);
		Point2 from;
		std::vector<Point2> controls;
		bool started = false;
		for (const DsfRoadPoint &point : segment.points)
		{
			const Point2 local = m_Projection.ToLocal(point.latitude, point.longitude);
			if (point.control)
			{
				controls.push_back(local);
				continue;
			}
			if (!started)
			{
				out.push_back(local);
				from = local;
				started = true;
				controls.clear();
				continue;
			}

			// One control point: quadratic; two (or more: the first and last count): cubic
			if (!controls.empty())
			{
				const Point2 c1 = controls.size() == 1 ? Point2{ from.x + (controls[0].x - from.x) * 2.0 / 3.0, from.z + (controls[0].z - from.z) * 2.0 / 3.0 } : controls.front();
				const Point2 c2 = controls.size() == 1 ? Point2{ local.x + (controls[0].x - local.x) * 2.0 / 3.0, local.z + (controls[0].z - local.z) * 2.0 / 3.0 } : controls.back();
				const double estimate = std::hypot(c1.x - from.x, c1.z - from.z) + std::hypot(c2.x - c1.x, c2.z - c1.z) + std::hypot(local.x - c2.x, local.z - c2.z);
				const int steps = std::clamp(static_cast<int>(std::ceil(estimate / step)), 2, 256);
				for (int i = 1; i < steps; ++i)
				{
					const double t = static_cast<double>(i) / steps, u = 1.0 - t;
					const double b0 = u * u * u, b1 = 3.0 * u * u * t, b2 = 3.0 * u * t * t, b3 = t * t * t;
					out.push_back({ b0 * from.x + b1 * c1.x + b2 * c2.x + b3 * local.x, b0 * from.z + b1 * c1.z + b2 * c2.z + b3 * local.z });
				}
			}
			out.push_back(local);
			from = local;
			controls.clear();
		}
	}

	bool OverlayBuilder::InArea(const Options &options, const Point2 &point) const
	{
		if (!options.area.valid)
			return true;
		return point.x >= options.area.minX - options.areaMargin && point.x <= options.area.maxX + options.areaMargin &&
			   point.z >= options.area.minZ - options.areaMargin && point.z <= options.area.maxZ + options.areaMargin;
	}

	bool OverlayBuilder::PolygonInArea(const Options &options, const std::vector<std::vector<Point2>> &rings) const
	{
		if (!options.area.valid)
			return true;
		for (const std::vector<Point2> &ring : rings)
		{
			for (const Point2 &point : ring)
			{
				if (InArea(options, point))
					return true;
			}
		}
		return false;
	}

	void OverlayBuilder::FlattenWinding(const std::vector<DsfPoint> &winding, const bool closed, const bool bezier, std::vector<Point2> &out, const Options &options, const size_t controlPlane) const
	{
		// Reuse the apt.dat bezier convention: a point's control leads out of it, the next point's mirrored control leads in.
		AptNodeChain chain;
		chain.closed = closed;
		for (const DsfPoint &point : winding)
		{
			AptNode node;
			node.latitude = point.v[1];
			node.longitude = point.v[0];
			if (bezier && point.planes >= controlPlane + 2)
			{
				node.controlLongitude = point.v[controlPlane];
				node.controlLatitude = point.v[controlPlane + 1];
				node.hasControl = std::abs(node.controlLongitude - node.longitude) > 1e-9 || std::abs(node.controlLatitude - node.latitude) > 1e-9;
			}
			chain.nodes.push_back(node);
		}
		FlattenChain(chain, m_Projection, out, options.bezierSegment);
	}

	// -------------------------------------------------------

	void OverlayBuilder::Build(const DsfFile &dsf, const Options &options, OverlayBuild &out)
	{
		for (uint32_t i = 0; i < dsf.objects.size(); ++i)
		{
			const DsfObjectPlacement &placement = dsf.objects[i];
			if (placement.definition >= dsf.objectDefinitions.size())
				continue;
			const Point2 local = m_Projection.ToLocal(placement.latitude, placement.longitude);
			if (!InArea(options, local))
			{
				++out.culled;
				continue;
			}

			std::string resource = dsf.objectDefinitions[placement.definition];
			for (char &c : resource)
			{
				if (c == '\\')
					c = '/';
			}

			if (EndsWith(resource, ".agp"))
			{
				BuildAutogen(placement, i, resource, options, out);
				continue;
			}
			if (!options.placeObjects)
				continue;
			if (out.objects.size() >= static_cast<size_t>(options.maxObjects))
			{
				Warn(out, "Object limit reached; the rest of the tile's objects were skipped");
				break;
			}

			const fs::path file = ResolveFile(resource);
			if (file.empty())
			{
				Warn(out, "Object not found: " + resource);
				continue;
			}

			OverlayObject object;
			object.file = file;
			object.resource = resource;
			object.name = BaseName(resource);
			object.x = local.x;
			object.z = local.z;
			// The scene seats them on the terrain: above the ground (AGL) or at their elevation (MSL)
			object.y = placement.mode == DsfObjectPlacement::Mode::Draped ? 0.0 : placement.elevation;
			object.msl = placement.mode == DsfObjectPlacement::Mode::Msl;
			object.heading = static_cast<float>(placement.heading);
			object.placement = i;
			out.objects.push_back(std::move(object));
		}

		for (uint32_t i = 0; i < dsf.polygons.size(); ++i)
		{
			const DsfPolygonPlacement &placement = dsf.polygons[i];
			if (placement.definition >= dsf.polygonDefinitions.size() || placement.windings.empty() || placement.windings[0].empty())
				continue;

			std::string resource = dsf.polygonDefinitions[placement.definition];
			for (char &c : resource)
			{
				if (c == '\\')
					c = '/';
			}

			if (EndsWith(resource, ".pol"))
			{
				if (options.drawPolygons)
					BuildDrapedPolygon(placement, i, resource, options, out);
			}
			else if (EndsWith(resource, ".lin"))
			{
				if (options.drawLines)
					BuildLine(placement, i, resource, options, out);
			}
			else if (EndsWith(resource, ".for"))
			{
				if (options.drawForests)
					BuildForest(placement, i, resource, options, out);
			}
			else if (EndsWith(resource, ".fac"))
			{
				if (options.drawFacades)
					BuildFacade(placement, i, resource, options, out);
			}
			else if (EndsWith(resource, ".str"))
			{
				if (options.placeStrings)
					BuildString(placement, i, resource, options, out);
			}
			else
				++out.unsupported;	// .ags / .agb autogen blocks and anything newer
		}

		for (uint32_t i = 0; i < dsf.roads.size() && options.drawRoads; ++i)
		{
			const DsfRoadSegment &segment = dsf.roads[i];
			if (segment.definition >= dsf.networkDefinitions.size() || segment.points.size() < 2)
				continue;
			std::string resource = dsf.networkDefinitions[segment.definition];
			std::replace(resource.begin(), resource.end(), '\\', '/');
			BuildRoad(segment, i, resource, options, out);
		}
	}

	void OverlayBuilder::BuildRoad(const DsfRoadSegment &segment, const uint32_t index, const std::string &resource, const Options &options, OverlayBuild &out)
	{
		std::vector<Point2> path;
		FlattenRoad(segment, path, options);
		if (path.size() < 2)
			return;
		if (!PolygonInArea(options, { path }))
		{
			++out.culled;
			return;
		}

		// The road type the subtype draws as: its flat strips, one mesh per texture; a type without strips (older
		// networks) as a band of its width in its map color
		fs::path file;
		const RoadNetworkAsset *network = GetRoadNetwork(resource, &file, out);
		const RoadNetworkAsset::RoadType *type = network ? network->FindRoadType(static_cast<int>(segment.subtype)) : nullptr;
		if (network && !type)
			Warn(out, resource + " has no road type " + std::to_string(segment.subtype) + ": drawn as a plain band");
		const float y = Height(options, "roads", 0);

		const auto makeMesh = [&]()
		{
			OverlayMesh mesh;
			mesh.kind = OverlayKind::Road;
			mesh.resource = resource;
			mesh.name = network && type ? network->GetSubtypeName(static_cast<int>(segment.subtype)) : BaseName(resource);
			mesh.placement = index;
			return mesh;
		};

		if (type && !type->strips.empty())
		{
			std::map<int, OverlayMesh> byTexture;
			for (const RoadNetworkAsset::Strip &strip : type->strips)
			{
				auto [it, added] = byTexture.try_emplace(strip.texture, makeMesh());
				OverlayMesh &mesh = it->second;
				if (added && strip.texture >= 0 && static_cast<size_t>(strip.texture) < network->textures.size())
				{
					mesh.texture = TexturePath(file, network->textures[static_cast<size_t>(strip.texture)]);
					mesh.cutout = true;
					mesh.cutoff = 0.3f;
				}
				AppendOffsetRibbon(mesh, path, y, strip.left, strip.right, strip.sLeft, strip.sRight, type->length);
			}
			for (auto &[texture, mesh] : byTexture)
				if (!mesh.indices.empty())
					out.meshes.push_back(std::move(mesh));
			return;
		}

		OverlayMesh mesh = makeMesh();
		const float half = 0.5f * (type && type->width > 0.0f ? type->width : 8.0f);
		if (type)
			for (int c = 0; c < 3; ++c)
				mesh.color[c] = type->color[c];
		AppendOffsetRibbon(mesh, path, y, -half, half, 0.0f, 1.0f, 100.0f);
		if (!mesh.indices.empty())
			out.meshes.push_back(std::move(mesh));
	}

	// -------------------------------------------------------

	void OverlayBuilder::BuildDrapedPolygon(const DsfPolygonPlacement &placement, const uint32_t index, const std::string &resource, const Options &options, OverlayBuild &out)
	{
		fs::path file;
		const PolygonAsset *asset = GetPolygon(resource, &file, out);
		if (!asset)
			return;

		const bool uvMapped = placement.parameter == 65535;
		const bool bezier = uvMapped ? placement.planes >= 8 : placement.planes >= 4;

		std::vector<std::vector<Point2>> rings;
		std::vector<std::vector<std::pair<double, double>>> ringUvs;	// Only for UV mapped polygons
		for (const std::vector<DsfPoint> &winding : placement.windings)
		{
			rings.emplace_back();
			if (uvMapped && bezier)
			{
				ringUvs.emplace_back();
				FlattenUvWinding(winding, m_Projection, options.bezierSegment, rings.back(), ringUvs.back());
			}
			else if (uvMapped)
			{
				ringUvs.emplace_back();
				for (const DsfPoint &point : winding)
				{
					rings.back().push_back(LocalOf(m_Projection, point));
					ringUvs.back().emplace_back(point.v[2], point.v[3]);
				}
				// Drop a repeated closing point
				if (rings.back().size() > 1)
				{
					const Point2 &first = rings.back().front(), &last = rings.back().back();
					if (std::hypot(first.x - last.x, first.z - last.z) < 1e-4)
					{
						rings.back().pop_back();
						ringUvs.back().pop_back();
					}
				}
			}
			else
				FlattenWinding(winding, true, bezier, rings.back(), options);
		}
		if (rings.empty() || rings[0].size() < 3)
			return;
		if (!PolygonInArea(options, rings))
		{
			++out.culled;
			return;
		}

		OverlayMesh mesh;
		mesh.kind = OverlayKind::DrapedPolygon;
		mesh.resource = resource;
		mesh.name = BaseName(resource);
		mesh.placement = index;
		mesh.texture = TexturePath(file, asset->texture);
		mesh.cutout = asset->noBlendCutoff >= 0.0f;
		mesh.cutoff = asset->noBlendCutoff >= 0.0f ? asset->noBlendCutoff : 0.5f;
		mesh.blended = !asset->noAlpha && !mesh.cutout;
		const float y = Height(options, asset->layerGroup, asset->layerOffset);

		if (uvMapped)
		{
			// Orthophoto: the UVs come with the points (or follow their curves); triangulate, then give each vertex the UV of its input point
			std::vector<Point2> points;
			std::vector<uint32_t> indices;
			if (!TriangulatePolygon(rings, points, indices))
				return;
			std::vector<Point2> source;
			std::vector<std::pair<double, double>> uvs;
			for (size_t r = 0; r < rings.size(); ++r)
			{
				source.insert(source.end(), rings[r].begin(), rings[r].end());
				uvs.insert(uvs.end(), ringUvs[r].begin(), ringUvs[r].end());
			}
			const uint32_t base = static_cast<uint32_t>(mesh.vertices.size());
			for (const Point2 &point : points)
			{
				const auto [u, v] = UvAt(point, source, uvs);
				AddVertex(mesh, point.x, y, point.z, u, v);
			}
			for (const uint32_t idx : indices)
				mesh.indices.push_back(base + idx);
		}
		else
		{
			// Projected: the parameter is the texture heading (in 1/128 degree steps above 360 for flattened polygons)
			double heading = placement.parameter;
			if (heading >= 360.0)
				heading = (1.0 / 128.0) * static_cast<int>(heading / 360.0) + std::fmod(heading, 360.0);
			float scaleS = 1.0f, scaleT = 1.0f;
			GetPolygonUvScale(*asset, scaleS, scaleT);
			AppendTexturedPolygon(mesh, rings, y, scaleS, scaleT, heading);
			ApplyPolygonShading(mesh, *asset, file, out);
		}

		if (!mesh.indices.empty())
			out.meshes.push_back(std::move(mesh));
	}

	void OverlayBuilder::BuildLine(const DsfPolygonPlacement &placement, const uint32_t index, const std::string &resource, const Options &options, OverlayBuild &out)
	{
		fs::path file;
		const LineAsset *asset = GetLine(resource, &file, out);
		if (!asset || asset->segments.empty())
			return;

		const bool closed = placement.parameter != 0;
		const bool bezier = placement.planes >= 4;
		std::vector<Point2> path;
		FlattenWinding(placement.windings[0], closed, bezier, path, options);
		if (path.size() < 2)
			return;
		if (!PolygonInArea(options, { path }))
		{
			++out.culled;
			return;
		}

		OverlayMesh mesh;
		mesh.kind = OverlayKind::Line;
		mesh.resource = resource;
		mesh.name = BaseName(resource);
		mesh.placement = index;
		mesh.texture = TexturePath(file, asset->texture);
		mesh.cutout = true;
		mesh.cutoff = 0.5f;
		AppendLinePaint(mesh, *asset, path, closed, Height(options, asset->layerGroup, asset->layerOffset));
		if (!mesh.indices.empty())
			out.meshes.push_back(std::move(mesh));
	}

	void OverlayBuilder::BuildForest(const DsfPolygonPlacement &placement, const uint32_t index, const std::string &resource, const Options &options, OverlayBuild &out)
	{
		fs::path file;
		const ForestAsset *asset = GetForest(resource, &file, out);
		if (!asset || asset->trees.empty())
			return;

		// Density 0..255 in the low byte; fill mode in the high byte: 0 fills the polygon, 1 lines its windings (open
		// chains), 2 puts one tree on each point. Only an area is a closed shape: filling or closing a chain or a set of
		// scattered trees would plant rows of trees across whatever lies between them (runways, aprons).
		const float density = static_cast<float>(placement.parameter % 256) / 255.0f;
		const int fillMode = placement.parameter / 256;
		const bool area = fillMode != 1 && fillMode != 2;

		std::vector<std::vector<Point2>> rings;
		for (const std::vector<DsfPoint> &winding : placement.windings)
		{
			rings.emplace_back();
			FlattenWinding(winding, area, false, rings.back(), options);
		}
		if (rings.empty() || rings[0].empty() || (area && rings[0].size() < 3))
			return;
		if (!PolygonInArea(options, rings))
		{
			++out.culled;
			return;
		}

		// Weighted tree choice
		float totalPercent = 0.0f;
		for (const ForestAsset::Tree &tree : asset->trees)
			totalPercent += std::max(0.0f, tree.percent);
		if (totalPercent <= 0.0f)
			return;

		const auto pickTree = [&](uint32_t &seed) -> const ForestAsset::Tree &
		{
			float roll = Random01(seed) * totalPercent;
			for (const ForestAsset::Tree &tree : asset->trees)
			{
				roll -= std::max(0.0f, tree.percent);
				if (roll <= 0.0f)
					return tree;
			}
			return asset->trees.back();
		};

		// Point-in-polygon (outline minus holes)
		const auto inside = [&rings](const Point2 &p) -> bool
		{
			bool in = false;
			for (size_t r = 0; r < rings.size(); ++r)
			{
				const std::vector<Point2> &ring = rings[r];
				bool inRing = false;
				for (size_t i = 0, j = ring.size() - 1; i < ring.size(); j = i++)
				{
					if ((ring[i].z > p.z) != (ring[j].z > p.z) && p.x < (ring[j].x - ring[i].x) * (p.z - ring[i].z) / (ring[j].z - ring[i].z) + ring[i].x)
						inRing = !inRing;
				}
				if (r == 0)
					in = inRing;
				else if (inRing)
					return false;
			}
			return in;
		};

		OverlayMesh mesh;
		mesh.kind = OverlayKind::Forest;
		mesh.resource = resource;
		mesh.name = BaseName(resource);
		mesh.placement = index;
		mesh.texture = TexturePath(file, asset->texture);
		mesh.cutout = true;
		mesh.cutoff = 0.5f;
		mesh.twoSided = true;

		uint32_t seed = m_Seed++ * 7919u + index;
		const float spacingX = std::max(1.0f, asset->spacingX);
		const float spacingY = std::max(1.0f, asset->spacingY);
		uint32_t trees = 0;

		const auto addTree = [&](const Point2 &at)
		{
			const ForestAsset::Tree &tree = pickTree(seed);
			const float height = tree.minHeight + (tree.maxHeight - tree.minHeight) * Random01(seed);
			const float aspect = std::abs(tree.width) / std::max(1.0f, tree.height);
			const float width = height * aspect;
			const float s1 = tree.s / asset->scaleX, s2 = (tree.s + tree.width) / asset->scaleX;
			const float t1 = tree.t / asset->scaleY, t2 = (tree.t + tree.height) / asset->scaleY;
			const float left = (tree.offset - tree.s) / std::max(1.0f, std::abs(tree.width));	// Fraction of the width left of the trunk
			const double yaw = Random01(seed) * kPi;
			const int quads = std::clamp(tree.quads, 1, 4);
			for (int q = 0; q < quads; ++q)
			{
				const double angle = yaw + kPi * q / quads;
				const double dx = std::cos(angle), dz = std::sin(angle);
				const double x0 = at.x - dx * width * left, z0 = at.z - dz * width * left;
				const double x1 = at.x + dx * width * (1.0f - left), z1 = at.z + dz * width * (1.0f - left);
				const double corners[4][3] = { { x0, 0.0, z0 }, { x1, 0.0, z1 }, { x1, height, z1 }, { x0, height, z0 } };
				const double uvs[4][2] = { { s1, t1 }, { s2, t1 }, { s2, t2 }, { s1, t2 } };
				const float normal[3] = { static_cast<float>(-dz), 0.0f, static_cast<float>(dx) };
				AddQuad(mesh, corners, uvs, normal);
			}
			++trees;
		};

		if (fillMode == 2)
		{
			// Single trees, one on each point
			for (const std::vector<Point2> &ring : rings)
			{
				for (const Point2 &point : ring)
				{
					if (trees >= options.forestTreeLimit)
						break;
					addTree(point);
				}
			}
		}
		else if (fillMode == 1)
		{
			// Rows of trees along each chain
			std::vector<Point2> points;
			std::vector<double> headings;
			for (const std::vector<Point2> &ring : rings)
			{
				ResamplePath(ring, false, spacingX * std::max(0.2f, 1.0f - density * 0.8f), 0.0, points, headings);
				for (const Point2 &point : points)
				{
					if (trees >= options.forestTreeLimit)
						break;
					addTree(point);
				}
			}
		}
		else
		{
			const Bounds2 bounds = GetBounds(rings[0]);
			if (!bounds.valid)
				return;
			for (double gz = bounds.minZ; gz <= bounds.maxZ && trees < options.forestTreeLimit; gz += spacingY)
			{
				for (double gx = bounds.minX; gx <= bounds.maxX && trees < options.forestTreeLimit; gx += spacingX)
				{
					if (Random01(seed) > density)
						continue;
					Point2 at{ gx + (Random01(seed) - 0.5f) * 2.0f * asset->randomX, gz + (Random01(seed) - 0.5f) * 2.0f * asset->randomY };
					if (!inside(at))
						continue;
					addTree(at);
				}
			}
		}

		if (!mesh.indices.empty())
			out.meshes.push_back(std::move(mesh));
	}

	void OverlayBuilder::BuildString(const DsfPolygonPlacement &placement, const uint32_t index, const std::string &resource, const Options &options, OverlayBuild &out)
	{
		fs::path file;
		const ObjectStringAsset *asset = GetString(resource, &file, out);
		if (!asset)
			return;

		const bool bezier = placement.planes >= 4;
		std::vector<Point2> path;
		FlattenWinding(placement.windings[0], false, bezier, path, options);
		if (path.size() < 2)
			return;
		if (!PolygonInArea(options, { path }))
		{
			++out.culled;
			return;
		}
		// Spacing in meters; X-Plane 12 writes spacings off the whole meter as 10000 + 1/128ths of a meter
		const double spacing = placement.parameter >= 10000 ? (placement.parameter - 10000) / 128.0 : static_cast<double>(placement.parameter);
		PlaceObjectString(*asset, file, resource, path, false, std::max(1.0, spacing), 0.0f, index, out);
	}

	void OverlayBuilder::PlaceObjectString(const ObjectStringAsset &asset, const fs::path &assetFile, const std::string_view resource, const std::vector<Point2> &path, const bool closed, const double spacing, const float y, const uint32_t placement, OverlayBuild &out, const OverlayObject::Source source)
	{
		if (asset.objects.empty())
			return;

		// Resolve the string's objects once
		std::vector<fs::path> files;
		for (const ObjectStringAsset::Entry &entry : asset.objects)
		{
			const std::string reference = ObjectReference(assetFile, entry.object);
			fs::path file = ResolveFile(reference);
			if (file.empty() && !entry.object.starts_with("lib/"))
				file = ResolveFile(entry.object);
			if (file.empty())
				Warn(out, "Object of " + std::string(resource) + " not found: " + entry.object);
			files.push_back(file);
		}

		std::vector<Point2> points;
		std::vector<double> headings;
		ResamplePath(path, closed, spacing, 0.0, points, headings);

		uint32_t seed = m_Seed++ * 48271u + placement;
		for (size_t i = 0; i < points.size(); ++i)
		{
			const size_t which = i % asset.objects.size();
			if (files[which].empty())
				continue;
			const ObjectStringAsset::Entry &entry = asset.objects[which];

			// Lateral offset to the right of travel
			const Point2 forward = HeadingToDirection(headings[i]);
			const Point2 right{ -forward.z, forward.x };
			OverlayObject object;
			object.source = source;
			object.file = files[which];
			object.resource = std::string(resource);
			object.name = BaseName(entry.object);
			object.x = points[i].x + right.x * asset.offset;
			object.z = points[i].z + right.z * asset.offset;
			object.y = y;
			const float rotation = entry.rotationMin + (entry.rotationMax - entry.rotationMin) * Random01(seed);
			object.heading = static_cast<float>(headings[i]) + rotation;
			object.placement = placement;
			out.objects.push_back(std::move(object));
			if (out.objects.size() > 200000)
				break;
		}
	}

	void OverlayBuilder::BuildAutogen(const DsfObjectPlacement &placement, const uint32_t index, const std::string &resource, const Options &options, OverlayBuild &out)
	{
		fs::path file;
		const AutogenPointAsset *asset = GetAutogen(resource, &file, out);
		if (!asset)
			return;

		const Point2 origin = m_Projection.ToLocal(placement.latitude, placement.longitude);
		const double heading = placement.heading * kDegToRad;
		const double c = std::cos(heading), s = std::sin(heading);
		// Tile x runs east and y north before rotation; rotate clockwise by the heading
		const auto place = [&](const float tx, const float ty) -> Point2
		{
			const double east = tx - asset->anchorX, north = ty - asset->anchorY;
			const double rx = east * c + north * s;
			const double rn = -east * s + north * c;
			return { origin.x + rx, origin.z - rn };
		};

		if (asset->HasTile() && options.drawPolygons)
		{
			OverlayMesh mesh;
			mesh.kind = OverlayKind::Autogen;
			mesh.resource = resource;
			mesh.name = BaseName(resource);
			mesh.placement = index;
			mesh.texture = TexturePath(file, asset->texture);
			mesh.cutout = true;
			mesh.cutoff = 0.5f;
			const float y = Height(options, "objects", -1);
			std::vector<Point2> ring;
			std::vector<std::pair<double, double>> uvs;
			for (size_t i = 0; i + 3 < asset->tile.size(); i += 4)
			{
				ring.push_back(place(asset->tile[i], asset->tile[i + 1]));
				uvs.emplace_back(asset->tile[i + 2], asset->tile[i + 3]);
			}
			std::vector<Point2> points;
			std::vector<uint32_t> indices;
			if (ring.size() >= 3 && TriangulatePolygon({ ring }, points, indices))
			{
				const uint32_t base = static_cast<uint32_t>(mesh.vertices.size());
				for (const Point2 &point : points)
				{
					const auto [u, v] = UvAt(point, ring, uvs);
					AddVertex(mesh, point.x, y, point.z, u, v);
				}
				for (const uint32_t idx : indices)
					mesh.indices.push_back(base + idx);
				out.meshes.push_back(std::move(mesh));
			}
		}

		if (!options.placeObjects)
			return;
		for (const AutogenPointAsset::Object &object : asset->placedObjects)
		{
			if (object.object < 0 || static_cast<size_t>(object.object) >= asset->objects.size())
				continue;
			const std::string &name = asset->objects[static_cast<size_t>(object.object)];
			const std::string reference = ObjectReference(file, name);
			fs::path objectFile = ResolveFile(reference);
			if (objectFile.empty() && !name.starts_with("lib/"))
				objectFile = ResolveFile(name);
			if (objectFile.empty())
			{
				Warn(out, "Object of " + resource + " not found: " + name);
				continue;
			}
			OverlayObject placed;
			placed.source = OverlayObject::Source::Autogen;
			placed.file = objectFile;
			placed.resource = resource;
			placed.name = BaseName(name);
			const Point2 at = place(object.x, object.y);
			placed.x = at.x;
			placed.z = at.z;
			placed.y = 0.0;
			placed.heading = static_cast<float>(placement.heading) + object.heading;
			placed.placement = index;
			out.objects.push_back(std::move(placed));
		}
	}

	// -------------------------------------------------------
	// Facades

	namespace
	{
		/* @brief One side of a facade footprint: from a DSF point to the next, flattened when it is a curve. */
		struct WallSide
		{
			FacadePath path;
			double length = 0.0;
			double heading = 0.0;	// Where the side faces (outward, from its chord), true degrees
			int wallType = 0;		// The DSF's wall for it (wall type planes only)
		};

		AptNode ToAptNode(const DsfPoint &point, const bool bezier, const size_t controlPlane)
		{
			AptNode node;
			node.latitude = point.v[1];
			node.longitude = point.v[0];
			if (bezier && point.planes >= controlPlane + 2)
			{
				node.controlLongitude = point.v[controlPlane];
				node.controlLatitude = point.v[controlPlane + 1];
				node.hasControl = std::abs(node.controlLongitude - node.longitude) > 1e-9 || std::abs(node.controlLatitude - node.latitude) > 1e-9;
			}
			return node;
		}
	}

	void OverlayBuilder::BuildFacade(const DsfPolygonPlacement &placement, const uint32_t index, const std::string &resource, const Options &options, OverlayBuild &out)
	{
		fs::path file;
		const FacadeAsset *asset = GetFacade(resource, &file, out);
		if (!asset || placement.windings.empty())
			return;

		// Planes after lon / lat: 3 wall type, 4 bezier control, 5 wall type then bezier control (WorldEditor's stacking).
		// Without wall types X-Plane picks each wall by its width and heading rules.
		const bool hasWallTypes = placement.planes == 3 || placement.planes == 5;
		const bool bezier = placement.planes >= 4;
		const size_t controlPlane = placement.planes == 5 ? 3 : 2;
		const std::vector<DsfPoint> &winding = placement.windings[0];	// Facades have no holes

		std::vector<Point2> footprint;
		FlattenWinding(winding, asset->ring, bezier, footprint, options, controlPlane);
		if (footprint.size() < 2)
			return;
		if (!PolygonInArea(options, { footprint }))
		{
			++out.culled;
			return;
		}

		// Each side on its own, so a curved side keeps its wall type and bends its wall instead of being cut into many
		std::vector<WallSide> sides;
		const size_t count = winding.size();
		const size_t sideCount = asset->ring ? count : (count > 0 ? count - 1 : 0);
		for (size_t i = 0; i < sideCount; ++i)
		{
			const DsfPoint &a = winding[i];
			const DsfPoint &b = winding[(i + 1) % count];
			AptNodeChain chain;
			chain.closed = false;
			chain.nodes = { ToAptNode(a, bezier, controlPlane), ToAptNode(b, bezier, controlPlane) };
			std::vector<Point2> points;
			FlattenChain(chain, m_Projection, points, options.bezierSegment);

			WallSide side;
			side.path = FacadePath(std::move(points));
			side.length = side.path.GetLength();
			if (side.length < 0.05)
				continue;	// Split bezier nodes are co-located points: no wall between them
			const Point2 &start = side.path.GetPoints().front(), &end = side.path.GetPoints().back();
			const double chord = std::max(1e-9, std::hypot(end.x - start.x, end.z - start.z));
			side.heading = DirectionToHeading(GetFacadeOutward({ (end.x - start.x) / chord, (end.z - start.z) / chord }));
			side.wallType = hasWallTypes ? static_cast<int>(a.v[2]) : 0;
			sides.push_back(std::move(side));
		}
		if (sides.empty())
			return;

		const double height = placement.parameter > 0 ? static_cast<double>(placement.parameter) : 3.0;
		std::vector<FacadeSideInput> inputs;
		inputs.reserve(sides.size());
		for (const WallSide &side : sides)
			inputs.push_back({ side.length, side.heading, side.wallType });
		const FacadeLayout layout = LayoutFacade(*asset, inputs, hasWallTypes, height);

		OverlayMesh walls;
		walls.kind = OverlayKind::Facade;
		walls.resource = resource;
		walls.name = BaseName(resource);
		walls.placement = index;
		walls.texture = TexturePath(file, asset->wallTexture);
		walls.cutout = asset->noBlendCutoff >= 0.0f || walls.texture.empty() == false;
		walls.cutoff = asset->noBlendCutoff >= 0.0f ? asset->noBlendCutoff : 0.5f;
		walls.twoSided = true;
		walls.color[0] = walls.color[1] = walls.color[2] = 0.75f;

		double roofHeight = height;
		if (asset->IsType2())
		{
			if (layout.floor < 0)
			{
				++out.unsupported;
				return;
			}
			const FacadeAsset::Floor &floor = asset->floors[static_cast<size_t>(layout.floor)];
			roofHeight = layout.roofHeight;
			// Curved sides bend the SEGMENT_CURVED variants when the facade has them
			const bool hasCurved = floor.curvedSegments.size() == floor.segments.size();

			for (size_t s = 0; s < sides.size(); ++s)
			{
				const WallSide &side = sides[s];
				const FacadeWallLayout &wall = layout.walls[s];
				if (wall.spelling.empty() || wall.total <= 0.0)
					continue;

				// Template space: x outward, y up, -z along the wall; each segment spans z in [-width, 0]
				double along = 0.0;
				for (const int segmentIndex : wall.spelling)
				{
					if (segmentIndex < 0 || static_cast<size_t>(segmentIndex) >= floor.segments.size())
						continue;
					const FacadeAsset::Segment &straight = floor.segments[static_cast<size_t>(segmentIndex)];
					const FacadeAsset::Segment &segment = side.path.IsCurved() && hasCurved ? floor.curvedSegments[static_cast<size_t>(segmentIndex)] : straight;
					// A curved variant fills the straight one's width
					const double scale = wall.stretch * (segment.width > 1e-6 && &segment != &straight ? straight.width / segment.width : 1.0);

					// NO_WALL_MESH: only the attached objects are drawn (barricades, fences of objects), as in WorldEditor
					static const std::vector<FacadeAsset::Mesh> noMeshes;
					for (const FacadeAsset::Mesh &m : asset->noWallMesh ? noMeshes : segment.meshes)
					{
						const uint32_t base = static_cast<uint32_t>(walls.vertices.size());
						const size_t vertexCount = m.positions.size() / 3;
						for (size_t v = 0; v < vertexCount; ++v)
						{
							const double lx = m.positions[v * 3], ly = m.positions[v * 3 + 1], lz = m.positions[v * 3 + 2];
							Point2 position, direction;
							side.path.Evaluate(along + (-lz) * scale, position, direction);
							const Point2 outward = GetFacadeOutward(direction);
							const float nx = v * 3 + 2 < m.normals.size() ? m.normals[v * 3] : 0.0f;
							const float ny = v * 3 + 2 < m.normals.size() ? m.normals[v * 3 + 1] : 1.0f;
							const float nz = v * 3 + 2 < m.normals.size() ? m.normals[v * 3 + 2] : 0.0f;
							// Rotate the normal like the position (x -> outward, -z -> along)
							const float wnx = static_cast<float>(outward.x * nx - direction.x * nz);
							const float wnz = static_cast<float>(outward.z * nx - direction.z * nz);
							AddVertex(walls, position.x + outward.x * lx, ly, position.z + outward.z * lx, v * 2 + 1 < m.uvs.size() ? m.uvs[v * 2] : 0.0,
								v * 2 + 1 < m.uvs.size() ? m.uvs[v * 2 + 1] : 0.0, wnx, ny, wnz);
						}
						for (size_t i = 0; i + 2 < m.indices.size(); i += 3)
						{
							// OBJ-style clockwise triangles become the engine's counter-clockwise
							if (m.indices[i] < vertexCount && m.indices[i + 1] < vertexCount && m.indices[i + 2] < vertexCount)
								walls.indices.insert(walls.indices.end(), { base + m.indices[i], base + m.indices[i + 2], base + m.indices[i + 1] });
						}
					}

					// Attached objects (jetway tunnels, light poles, door handles...)
					for (const FacadeAsset::Attachment &attachment : segment.attachments)
					{
						if (attachment.object < 0 || static_cast<size_t>(attachment.object) >= asset->objects.size() || !options.placeObjects)
							continue;
						const std::string &name = asset->objects[static_cast<size_t>(attachment.object)];
						const std::string reference = ObjectReference(file, name);
						fs::path objectFile = ResolveFile(reference);
						if (objectFile.empty() && !name.starts_with("lib/"))
							objectFile = ResolveFile(name);
						if (objectFile.empty())
						{
							Warn(out, "Object of " + resource + " not found: " + name);
							continue;
						}
						Point2 position, direction;
						side.path.Evaluate(along + (-attachment.z) * scale, position, direction);
						const Point2 outward = GetFacadeOutward(direction);
						OverlayObject object;
						object.source = OverlayObject::Source::Facade;
						object.file = objectFile;
						object.resource = resource;
						object.name = BaseName(name);
						object.x = position.x + outward.x * attachment.x;
						object.z = position.z + outward.z * attachment.x;
						object.y = attachment.y;
						// The OBJ sits in the segment's frame (x outward, -z along the wall): its -Z runs along the wall,
						// turned by the attachment's heading (WorldEditor's WED_FacadePreview does the same)
						object.heading = static_cast<float>(DirectionToHeading(direction)) + attachment.heading;
						object.placement = index;
						out.objects.push_back(std::move(object));
					}
					along += straight.width * wall.stretch;
				}
			}
		}
		else
		{
			// Type 1: texture panels. Left / center / right panels across (center repeated) and bottom / middle / top
			// floors up (middle repeated), stretched to fit the side exactly.
			if (asset->walls.empty())
				return;
			for (size_t s = 0; s < sides.size(); ++s)
			{
				const WallSide &side = sides[s];
				const FacadeAsset::Wall *wall = &asset->walls[static_cast<size_t>(std::clamp(layout.walls[s].wall, 0, static_cast<int>(asset->walls.size()) - 1))];

				struct Span
				{
					float t0, t1;	// Texture range
					double size;	// Meters
				};
				std::vector<Span> columns, rows;
				const auto panelWidth = [&](const std::pair<float, float> &panel) { return static_cast<double>(panel.second - panel.first) * wall->scaleS; };
				const auto floorHeight = [&](const std::pair<float, float> &panel) { return static_cast<double>(panel.second - panel.first) * wall->scaleT; };

				double fixedWidth = 0.0;
				for (const auto &panel : wall->leftPanels) fixedWidth += panelWidth(panel);
				for (const auto &panel : wall->rightPanels) fixedWidth += panelWidth(panel);
				double centerWidth = 0.0;
				for (const auto &panel : wall->centerPanels) centerWidth += panelWidth(panel);
				int centerRepeats = centerWidth > 0.01 ? std::max(0, static_cast<int>(std::lround((side.length - fixedWidth) / centerWidth))) : 0;
				if (wall->leftPanels.empty() && wall->rightPanels.empty() && centerRepeats == 0)
					centerRepeats = 1;
				for (const auto &panel : wall->leftPanels) columns.push_back({ panel.first, panel.second, panelWidth(panel) });
				for (int r = 0; r < centerRepeats; ++r)
					for (const auto &panel : wall->centerPanels) columns.push_back({ panel.first, panel.second, panelWidth(panel) });
				for (const auto &panel : wall->rightPanels) columns.push_back({ panel.first, panel.second, panelWidth(panel) });
				if (columns.empty())
					continue;

				double fixedHeight = 0.0;
				for (const auto &panel : wall->bottomFloors) fixedHeight += floorHeight(panel);
				for (const auto &panel : wall->topFloors) fixedHeight += floorHeight(panel);
				double middleHeight = 0.0;
				for (const auto &panel : wall->middleFloors) middleHeight += floorHeight(panel);
				int middleRepeats = middleHeight > 0.01 ? std::max(0, static_cast<int>(std::lround((height - fixedHeight) / middleHeight))) : 0;
				if (wall->bottomFloors.empty() && wall->topFloors.empty() && middleRepeats == 0)
					middleRepeats = 1;
				for (const auto &panel : wall->bottomFloors) rows.push_back({ panel.first, panel.second, floorHeight(panel) });
				for (int r = 0; r < middleRepeats; ++r)
					for (const auto &panel : wall->middleFloors) rows.push_back({ panel.first, panel.second, floorHeight(panel) });
				for (const auto &panel : wall->topFloors) rows.push_back({ panel.first, panel.second, floorHeight(panel) });
				if (rows.empty())
					continue;

				double totalWidth = 0.0, totalHeight = 0.0;
				for (const Span &c : columns) totalWidth += c.size;
				for (const Span &r : rows) totalHeight += r.size;
				const double stretchX = totalWidth > 0.01 ? side.length / totalWidth : 1.0;
				const double stretchY = totalHeight > 0.01 ? height / totalHeight : 1.0;

				double x = 0.0;
				for (const Span &column : columns)
				{
					const double w = column.size * stretchX;
					Point2 p0, p1, d0, d1, mid, dm;
					side.path.Evaluate(x, p0, d0);
					side.path.Evaluate(x + w, p1, d1);
					side.path.Evaluate(x + w * 0.5, mid, dm);
					const Point2 outward = GetFacadeOutward(dm);
					const float normal[3] = { static_cast<float>(outward.x), 0.0f, static_cast<float>(outward.z) };
					double yPos = 0.0;
					for (const Span &row : rows)
					{
						const double h = row.size * stretchY;
						const double corners[4][3] =
						{
							{ p0.x, yPos, p0.z },
							{ p1.x, yPos, p1.z },
							{ p1.x, yPos + h, p1.z },
							{ p0.x, yPos + h, p0.z }
						};
						const double uvs[4][2] = { { column.t0, row.t0 }, { column.t1, row.t0 }, { column.t1, row.t1 }, { column.t0, row.t1 } };
						AddQuad(walls, corners, uvs, normal);
						yPos += h;
					}
					x += w;
				}
				roofHeight = std::max(roofHeight, totalHeight * stretchY);
			}
		}

		if (!walls.indices.empty())
			out.meshes.push_back(std::move(walls));

		// Roof: the footprint at the roof height
		if (asset->ring && asset->hasRoof && !asset->noRoofMesh && footprint.size() >= 3)
		{
			OverlayMesh roof;
			roof.kind = OverlayKind::Facade;
			roof.resource = resource;
			roof.name = BaseName(resource) + " roof";
			roof.placement = index;
			roof.texture = TexturePath(file, asset->roofTexture.empty() ? asset->wallTexture : asset->roofTexture);
			roof.cutout = false;
			roof.twoSided = true;
			roof.color[0] = roof.color[1] = roof.color[2] = 0.45f;
			// The roof texture runs along the front wall (s) and in from it (t), as X-Plane and WorldEditor lay it out
			const auto frontHeading = [&]
			{
				const Point2 &start = sides.front().path.GetPoints().front(), &end = sides.front().path.GetPoints().back();
				const double chord = std::max(1e-9, std::hypot(end.x - start.x, end.z - start.z));
				return DirectionToHeading({ (end.x - start.x) / chord, (end.z - start.z) / chord });
			};
			if (asset->IsType2() || asset->roofUv.size() != 4)
			{
				AppendTexturedPolygon(roof, { footprint }, static_cast<float>(roofHeight), asset->roofScaleS > 0.01f ? asset->roofScaleS : 10.0f,
					asset->roofScaleT > 0.01f ? asset->roofScaleT : 10.0f, frontHeading() - 90.0);
			}
			else if (asset->roofScaled)
			{
				// Type 1 ROOF_SCALE: the texture rectangle repeats every so many meters (GetFacadeRoofMeters), centered on the roof
				const float s1 = asset->roofUv[0].first, t1 = asset->roofUv[0].second, s2 = asset->roofUv[2].first, t2 = asset->roofUv[2].second;
				AppendRepeatedRectangle(roof, { footprint }, static_cast<float>(roofHeight), s1, t1, s2, t2, static_cast<double>(asset->roofScaleS) * (s2 - s1),
					static_cast<double>(asset->roofScaleT) * (t2 - t1), asset->roofCenter.first, asset->roofCenter.second, frontHeading() - 90.0);
			}
			else
			{
				// Type 1 ROOF corners: map the footprint's bounding box onto the texture rectangle
				std::vector<Point2> points;
				std::vector<uint32_t> indices;
				if (TriangulatePolygon({ footprint }, points, indices))
				{
					const Bounds2 bounds = GetBounds(points);
					const double w = std::max(0.01, bounds.maxX - bounds.minX), h = std::max(0.01, bounds.maxZ - bounds.minZ);
					const float s1 = asset->roofUv[0].first, t1 = asset->roofUv[0].second, s2 = asset->roofUv[2].first, t2 = asset->roofUv[2].second;
					const uint32_t base = static_cast<uint32_t>(roof.vertices.size());
					for (const Point2 &point : points)
						AddVertex(roof, point.x, roofHeight, point.z, s1 + (s2 - s1) * (point.x - bounds.minX) / w, t1 + (t2 - t1) * (bounds.maxZ - point.z) / h);
					for (const uint32_t idx : indices)
						roof.indices.push_back(base + idx);
				}
			}
			if (!roof.indices.empty())
				out.meshes.push_back(std::move(roof));
		}
	}

	// -------------------------------------------------------
	// apt.dat surfaces through the library

	std::string OverlayBuilder::GetSurfaceLibraryPolygon(const int surface)
	{
		// The same table WorldEditor uses (WED_LibraryMgr::RescanSurfaces) for X-Plane 12's surface codes
		const char *name = nullptr;
		switch (surface)
		{
			case 1:		name = "asphalt/strips.pol"; break;
			case 24:	name = "asphalt/patched.pol"; break;
			case 25:	name = "asphalt/plain.pol"; break;
			case 26:	name = "asphalt/worn.pol"; break;
			case 20:	name = "asphalt_L/strips.pol"; break;
			case 21:	name = "asphalt_L/patched.pol"; break;
			case 22:	name = "asphalt_L/plain.pol"; break;
			case 23:	name = "asphalt_L/worn.pol"; break;
			case 27:	name = "asphalt_D/strips.pol"; break;
			case 28:	name = "asphalt_D/patched.pol"; break;
			case 29:	name = "asphalt_D/plain.pol"; break;
			case 30:	name = "asphalt_D/worn.pol"; break;
			case 31:	name = "asphalt_D2/strips.pol"; break;
			case 32:	name = "asphalt_D2/patched.pol"; break;
			case 33:	name = "asphalt_D2/plain.pol"; break;
			case 34:	name = "asphalt_D2/worn.pol"; break;
			case 35:	name = "asphalt_D3/strips.pol"; break;
			case 36:	name = "asphalt_D3/patched.pol"; break;
			case 37:	name = "asphalt_D3/plain.pol"; break;
			case 38:	name = "asphalt_D3/worn.pol"; break;
			case 2:		name = "concrete/new.pol"; break;
			case 53:	name = "concrete/dirty.pol"; break;
			case 54:	name = "concrete/worn.pol"; break;
			case 50:	name = "concrete_L/new.pol"; break;
			case 51:	name = "concrete_L/dirty.pol"; break;
			case 52:	name = "concrete_L/worn.pol"; break;
			case 55:	name = "concrete_D/new.pol"; break;
			case 56:	name = "concrete_D/dirty.pol"; break;
			case 57:	name = "concrete_D/worn.pol"; break;
			default:	return {};	// Grass, dirt, gravel, lakebed, water, snow and transparent: drawn with a color
		}
		return std::string("lib/airport/ground/pavement/") + name;
	}

	std::string OverlayBuilder::GetRunwayLibraryPolygon(const int surface, const OverlayKind kind)
	{
		const char *role = nullptr;
		switch (kind)
		{
			case OverlayKind::Runway:	role = "runway"; break;
			case OverlayKind::Pavement:	role = "taxiway"; break;
			case OverlayKind::Helipad:	role = "helipad"; break;
			default:					return {};
		}

		// The same family and variant as the WorldEditor table above: asphalt strips / patched / plain / worn are
		// <role> / _patch / _plain / _worn, concrete new / dirty / worn are <role> / _dirty / _worn
		const char *family = nullptr;
		const char *variant = nullptr;
		switch (surface)
		{
			case 1:		family = "asphalt";		variant = ""; break;
			case 24:	family = "asphalt";		variant = "_patch"; break;
			case 25:	family = "asphalt";		variant = "_plain"; break;
			case 26:	family = "asphalt";		variant = "_worn"; break;
			case 20:	family = "asphalt_L";	variant = ""; break;
			case 21:	family = "asphalt_L";	variant = "_patch"; break;
			case 22:	family = "asphalt_L";	variant = "_plain"; break;
			case 23:	family = "asphalt_L";	variant = "_worn"; break;
			case 27:	family = "asphalt_D";	variant = ""; break;
			case 28:	family = "asphalt_D";	variant = "_patch"; break;
			case 29:	family = "asphalt_D";	variant = "_plain"; break;
			case 30:	family = "asphalt_D";	variant = "_worn"; break;
			case 31:	family = "asphalt_D2";	variant = ""; break;
			case 32:	family = "asphalt_D2";	variant = "_patch"; break;
			case 33:	family = "asphalt_D2";	variant = "_plain"; break;
			case 34:	family = "asphalt_D2";	variant = "_worn"; break;
			case 35:	family = "asphalt_D3";	variant = ""; break;
			case 36:	family = "asphalt_D3";	variant = "_patch"; break;
			case 37:	family = "asphalt_D3";	variant = "_plain"; break;
			case 38:	family = "asphalt_D3";	variant = "_worn"; break;
			case 2:		family = "concrete";	variant = ""; break;
			case 53:	family = "concrete";	variant = "_dirty"; break;
			case 54:	family = "concrete";	variant = "_worn"; break;
			case 50:	family = "concrete_L";	variant = ""; break;
			case 51:	family = "concrete_L";	variant = "_dirty"; break;
			case 52:	family = "concrete_L";	variant = "_worn"; break;
			case 55:	family = "concrete_D";	variant = ""; break;
			case 56:	family = "concrete_D";	variant = "_dirty"; break;
			case 57:	family = "concrete_D";	variant = "_worn"; break;
			default:	return {};	// Grass, dirt, gravel, lakebed, water, snow and transparent: drawn with a color
		}
		return std::string("lib/airport/default_runways/") + family + "/" + role + variant + ".pol";
	}

	std::string OverlayBuilder::GetLineLibraryPath(const int lineType)
	{
		// lib/airport/lines/<code>_<name>.lin; the resolver finds the file by code prefix
		if (lineType <= 0 || lineType >= 100)
			return {};
		return "lib/airport/lines/" + std::to_string(lineType) + "_";
	}

	std::string OverlayBuilder::GetLightStringLibraryPath(const int lightType)
	{
		if (lightType < 101 || lightType > 108)
			return {};
		return "lib/airport/lights/fast/" + std::to_string(lightType) + "_";
	}

	double OverlayBuilder::GetLightStringSpacing(const int lightType)
	{
		switch (lightType)
		{
			case 101: case 105: case 107: case 108:	return 15.0;	// Centerline
			case 102: case 106:						return 20.0;	// Edge
			case 103: case 104:						return 1.5;		// Hold short bars
			default:								return 20.0;
		}
	}

	void OverlayBuilder::BuildAirportSurfaces(const AptAirport &airport, const std::vector<float> &pavementHeights, const float runwayHeight, const float lineHeight, const Options &options, OverlayBuild &out)
	{
		// Pavement and runways
		const auto surfaceMesh = [&](const int surface, const OverlayKind kind, const std::string &name, const uint32_t index, OverlayMesh &mesh, const PolygonAsset *&asset, fs::path &file) -> bool
		{
			// X-Plane 12's runway / taxiway art when the library has it, else WorldEditor's ground pavement (X-Plane 11)
			std::string library = GetRunwayLibraryPolygon(surface, kind);
			if (library.empty() || (!m_Polygons.contains(library) && ResolveFile(library).empty()))
				library = GetSurfaceLibraryPolygon(surface);
			if (library.empty())
				return false;
			asset = GetPolygon(library, &file, out);
			if (!asset)
				return false;
			mesh.kind = kind;
			mesh.resource = library;
			mesh.name = name;
			mesh.placement = index;
			mesh.texture = TexturePath(file, asset->texture);
			mesh.cutout = false;
			mesh.blended = false;
			return true;
		};

		const auto wanted = [&options](const OverlayKind kind, const uint32_t index) { return !options.elementFilter || options.elementFilter(kind, index); };

		for (uint32_t i = 0; i < airport.pavements.size(); ++i)
		{
			const AptPavement &pavement = airport.pavements[i];
			if (GetSurfaceFamily(pavement.surface) == SurfaceFamily::Transparent || !wanted(OverlayKind::Pavement, i))
				continue;
			OverlayMesh mesh;
			const PolygonAsset *asset = nullptr;
			fs::path file;
			if (!surfaceMesh(pavement.surface, OverlayKind::Pavement, pavement.description.empty() ? "Pavement " + std::to_string(i + 1) : pavement.description, i, mesh, asset, file))
				continue;

			std::vector<std::vector<Point2>> rings;
			for (const AptNodeChain &ring : pavement.rings)
			{
				if (!ring.closed || ring.nodes.size() < 3)
					continue;
				rings.emplace_back();
				FlattenChain(ring, m_Projection, rings.back(), options.bezierSegment);
				if (rings.back().size() < 3)
					rings.pop_back();
			}
			if (rings.empty())
				continue;
			const float y = i < pavementHeights.size() ? pavementHeights[i] : runwayHeight - 0.02f;
			float scaleS = 1.0f, scaleT = 1.0f;
			GetPolygonUvScale(*asset, scaleS, scaleT);
			AppendTexturedPolygon(mesh, rings, y, scaleS, scaleT, pavement.textureHeading);
			ApplyPolygonShading(mesh, *asset, file, out);
			if (!mesh.indices.empty())
				out.meshes.push_back(std::move(mesh));
		}

		for (uint32_t i = 0; i < airport.runways.size(); ++i)
		{
			const AptRunway &runway = airport.runways[i];
			if (GetSurfaceFamily(runway.surface) == SurfaceFamily::Transparent || !wanted(OverlayKind::Runway, i))
				continue;
			OverlayMesh mesh;
			const PolygonAsset *asset = nullptr;
			fs::path file;
			if (!surfaceMesh(runway.surface, OverlayKind::Runway, "Runway " + runway.GetName(), i, mesh, asset, file))
				continue;
			Point2 corners[4];
			GetRunwayCorners(runway, m_Projection, corners, false);
			// The texture starts at end 0's center, so slab joints line up with the centerline and the threshold
			float scaleS = 1.0f, scaleT = 1.0f;
			GetPolygonUvScale(*asset, scaleS, scaleT);
			AppendTexturedPolygon(mesh, { { corners[0], corners[1], corners[2], corners[3] } }, runwayHeight, scaleS, scaleT, runway.GetHeading(),
				m_Projection.ToLocal(runway.ends[0].latitude, runway.ends[0].longitude));
			ApplyPolygonShading(mesh, *asset, file, out);
			if (mesh.indices.empty())
				continue;

			// The painted markings, from the polygon's RUNWAY_MARKINGS atlas, just above the surface
			OverlayMesh markings;
			if (!asset->runwayMarkings.empty())
			{
				markings.kind = OverlayKind::Runway;
				markings.resource = mesh.resource;
				markings.name = mesh.name + " markings";
				markings.placement = i;
				markings.texture = TexturePath(file, asset->runwayMarkings);
				markings.shading = OverlayShading::Markings;
				std::copy(std::begin(asset->runwayMarkingsColor), std::end(asset->runwayMarkingsColor), std::begin(markings.color));
				markings.blended = true;
				markings.tangentX = mesh.tangentX;
				markings.tangentZ = mesh.tangentZ;
				AppendRunwayMarkings(markings, runway, m_Projection, runwayHeight + RUNWAY_MARKINGS_LIFT);
			}
			out.meshes.push_back(std::move(mesh));
			if (!markings.indices.empty())
				out.meshes.push_back(std::move(markings));
		}

		for (uint32_t i = 0; i < airport.helipads.size(); ++i)
		{
			const AptHelipad &helipad = airport.helipads[i];
			if (GetSurfaceFamily(helipad.surface) == SurfaceFamily::Transparent || !wanted(OverlayKind::Helipad, i))
				continue;
			OverlayMesh mesh;
			const PolygonAsset *asset = nullptr;
			fs::path file;
			if (!surfaceMesh(helipad.surface, OverlayKind::Helipad, "Helipad " + helipad.designator, i, mesh, asset, file))
				continue;
			Point2 corners[4];
			GetHelipadCorners(helipad, m_Projection, corners);
			float scaleS = 1.0f, scaleT = 1.0f;
			GetPolygonUvScale(*asset, scaleS, scaleT);
			AppendTexturedPolygon(mesh, { { corners[0], corners[1], corners[2], corners[3] } }, runwayHeight + 0.01f, scaleS, scaleT, helipad.heading);
			ApplyPolygonShading(mesh, *asset, file, out);
			if (!mesh.indices.empty())
				out.meshes.push_back(std::move(mesh));
		}

		// Painted lines and light strings: per chain, split where the style changes
		const auto buildChains = [&](const std::vector<AptNodeChain> &chains, const std::string &what, const uint32_t index, const OverlayKind lineKind)
		{
			for (const AptNodeChain &chain : chains)
			{
				if (chain.nodes.size() < 2)
					continue;
				const size_t segments = chain.closed ? chain.nodes.size() : chain.nodes.size() - 1;

				// Group consecutive segments with the same painted line type, then the same light type
				for (int pass = 0; pass < 2; ++pass)
				{
					size_t start = 0;
					while (start < segments)
					{
						const AptNode &first = chain.nodes[start];
						const int style = pass == 0 ? first.lineType : first.lightType;
						size_t end = start + 1;
						while (end < segments && (pass == 0 ? chain.nodes[end].lineType : chain.nodes[end].lightType) == style)
							++end;
						if (style == 0)
						{
							start = end;
							continue;
						}

						// The run's nodes, as an open chain (or the whole ring when it covers everything)
						AptNodeChain run;
						const bool wholeRing = chain.closed && start == 0 && end == segments;
						run.closed = wholeRing;
						for (size_t n = start; n <= end && (wholeRing ? n < segments : true); ++n)
							run.nodes.push_back(chain.nodes[n % chain.nodes.size()]);
						std::vector<Point2> path;
						FlattenChain(run, m_Projection, path, options.bezierSegment);
						if (path.size() >= 2)
						{
							if (pass == 0)
							{
								fs::path file;
								const std::string library = GetLineLibraryPath(style);
								const LineAsset *asset = library.empty() ? nullptr : GetLine(library, &file, out);
								if (asset && !asset->segments.empty())
								{
									OverlayMesh mesh;
									mesh.kind = lineKind;
									mesh.resource = library;
									mesh.name = what + ": " + GetLineTypeName(style);
									mesh.placement = index;
									mesh.texture = TexturePath(file, asset->texture);
									mesh.cutout = true;
									mesh.cutoff = 0.5f;
									AppendLinePaint(mesh, *asset, path, run.closed, lineHeight);
									if (!mesh.indices.empty())
										out.meshes.push_back(std::move(mesh));
								}
							}
							else if (options.placeStrings)
							{
								fs::path file;
								const std::string library = GetLightStringLibraryPath(style);
								const ObjectStringAsset *asset = library.empty() ? nullptr : GetString(library, &file, out);
								if (asset)
									PlaceObjectString(*asset, file, library, path, run.closed, GetLightStringSpacing(style), 0.0f, lineKind == OverlayKind::PaintedLine ? index : 0x80000000u | index, out, OverlayObject::Source::AirportLights);
							}
						}
						start = end;
					}
				}
			}
		};

		for (uint32_t i = 0; i < airport.linearFeatures.size(); ++i)
			if (wanted(OverlayKind::PaintedLine, i))
				buildChains(airport.linearFeatures[i].chains, airport.linearFeatures[i].description.empty() ? "Line " + std::to_string(i + 1) : airport.linearFeatures[i].description, i, OverlayKind::PaintedLine);
		for (uint32_t i = 0; i < airport.pavements.size(); ++i)
			if (wanted(OverlayKind::PavementLine, i))
				buildChains(airport.pavements[i].rings, "Pavement " + std::to_string(i + 1) + " edge", i, OverlayKind::PavementLine);
	}
}

// -------------------------------------------------------

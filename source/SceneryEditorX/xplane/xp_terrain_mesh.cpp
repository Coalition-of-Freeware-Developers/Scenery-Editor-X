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
 * xp_terrain_mesh.cpp
 * -------------------------------------------------------
 * Created: 22/10/2026
 * -------------------------------------------------------
 */
#include "xp_terrain_mesh.h"
#include "xp_terrain.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <unordered_map>

// Draping cuts every overlay triangle of an airport against the terrain: keep it optimized in MSVC debug builds too
#if defined(_MSC_VER) && !defined(__clang__) && !defined(NDEBUG)
#pragma optimize("gt", on)
#endif

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	namespace
	{
		constexpr uint32_t NONE = 0xFFFFFFFFu;
		constexpr double BARY_EPSILON = 1e-7;
		constexpr double MIN_PIECE_AREA = 1e-6;		// Square meters: slivers from cutting along an edge are dropped
		constexpr int MAX_CELLS_PER_AXIS = 1024;

		double Cross2(const double ax, const double az, const double bx, const double bz)
		{
			return ax * bz - az * bx;
		}

		/* @brief Barycentric coordinates of p in (a, b, c) on the ground plane; false for a degenerate triangle. */
		bool Barycentric(const double ax, const double az, const double bx, const double bz, const double cx, const double cz, const double px, const double pz, double out[3])
		{
			const double v0x = bx - ax, v0z = bz - az, v1x = cx - ax, v1z = cz - az, v2x = px - ax, v2z = pz - az;
			const double den = Cross2(v0x, v0z, v1x, v1z);
			if (std::abs(den) < 1e-12)
				return false;
			out[1] = Cross2(v2x, v2z, v1x, v1z) / den;
			out[2] = Cross2(v0x, v0z, v2x, v2z) / den;
			out[0] = 1.0 - out[1] - out[2];
			return true;
		}

		/* @brief Distance between two segments on the ground plane. */
		double SegmentDistance(const Point2 &p0, const Point2 &p1, const Point2 &q0, const Point2 &q1)
		{
			const auto pointToSegment = [](const Point2 &p, const Point2 &a, const Point2 &b)
			{
				const double dx = b.x - a.x, dz = b.z - a.z;
				const double length2 = dx * dx + dz * dz;
				double t = length2 > 0.0 ? ((p.x - a.x) * dx + (p.z - a.z) * dz) / length2 : 0.0;
				t = std::clamp(t, 0.0, 1.0);
				return std::hypot(p.x - (a.x + t * dx), p.z - (a.z + t * dz));
			};
			// Crossing segments are 0 apart
			const double d1 = Cross2(p1.x - p0.x, p1.z - p0.z, q0.x - p0.x, q0.z - p0.z);
			const double d2 = Cross2(p1.x - p0.x, p1.z - p0.z, q1.x - p0.x, q1.z - p0.z);
			const double d3 = Cross2(q1.x - q0.x, q1.z - q0.z, p0.x - q0.x, p0.z - q0.z);
			const double d4 = Cross2(q1.x - q0.x, q1.z - q0.z, p1.x - q0.x, p1.z - q0.z);
			if (((d1 > 0.0 && d2 < 0.0) || (d1 < 0.0 && d2 > 0.0)) && ((d3 > 0.0 && d4 < 0.0) || (d3 < 0.0 && d4 > 0.0)))
				return 0.0;
			return std::min(std::min(pointToSegment(p0, q0, q1), pointToSegment(p1, q0, q1)), std::min(pointToSegment(q0, p0, p1), pointToSegment(q1, p0, p1)));
		}

		bool PointInRing(const Point2 &p, const std::vector<Point2> &ring)
		{
			bool inside = false;
			for (size_t i = 0, j = ring.size() - 1; i < ring.size(); j = i++)
			{
				const Point2 &a = ring[i], &b = ring[j];
				if ((a.z > p.z) != (b.z > p.z) && p.x < (b.x - a.x) * (p.z - a.z) / (b.z - a.z) + a.x)
					inside = !inside;
			}
			return inside;
		}

		bool PointInTriangle(const Point2 &p, const Point2 &a, const Point2 &b, const Point2 &c)
		{
			double bary[3];
			return Barycentric(a.x, a.z, b.x, b.z, c.x, c.z, p.x, p.z, bary) && bary[0] >= 0.0 && bary[1] >= 0.0 && bary[2] >= 0.0;
		}

		void Normalize(float &x, float &y, float &z)
		{
			const float length = std::sqrt(x * x + y * y + z * z);
			if (length > 1e-12f)
			{
				x /= length;
				y /= length;
				z /= length;
			}
			else
			{
				x = 0.0f;
				y = 1.0f;
				z = 0.0f;
			}
		}

		/* @brief A point of a piece being cut: ground position and its barycentrics in the triangle being draped. */
		struct ClipPoint
		{
			double x = 0.0;
			double z = 0.0;
		};

		TerrainVertex LerpVertex(const TerrainVertex &a, const TerrainVertex &b, const double t)
		{
			const auto lerp = [t](const float u, const float v) { return static_cast<float>(u + (static_cast<double>(v) - u) * t); };
			TerrainVertex out;
			out.x = lerp(a.x, b.x);
			out.y = lerp(a.y, b.y);
			out.z = lerp(a.z, b.z);
			out.nx = lerp(a.nx, b.nx);
			out.ny = lerp(a.ny, b.ny);
			out.nz = lerp(a.nz, b.nz);
			Normalize(out.nx, out.ny, out.nz);
			out.s = lerp(a.s, b.s);
			out.t = lerp(a.t, b.t);
			return out;
		}

		/* @brief A fixed order of points, so a crossing is computed from an edge's ends the same way whichever triangle asks. */
		bool VertexBefore(const TerrainVertex &a, const TerrainVertex &b)
		{
			if (a.x != b.x)
				return a.x < b.x;
			if (a.z != b.z)
				return a.z < b.z;
			return a.y < b.y;
		}

		/**
		 * @brief Sutherland-Hodgman: the part of a convex polygon on one side of the line x = value (axisX) or z = value,
		 * keepAbove keeping the side of larger coordinates. Crossings land exactly on the line.
		 */
		void ClipPolygon(const std::vector<TerrainVertex> &in, std::vector<TerrainVertex> &out, const bool axisX, const double value, const bool keepAbove)
		{
			out.clear();
			const size_t n = in.size();
			const auto side = [axisX, value](const TerrainVertex &v) { return (axisX ? static_cast<double>(v.x) : static_cast<double>(v.z)) - value; };
			for (size_t i = 0; i < n; ++i)
			{
				const TerrainVertex &p = in[i];
				const TerrainVertex &q = in[(i + 1) % n];
				const double dp = side(p), dq = side(q);
				if (keepAbove ? dp >= 0.0 : dp <= 0.0)
					out.push_back(p);
				if ((dp > 0.0 && dq < 0.0) || (dp < 0.0 && dq > 0.0))
				{
					const bool swap = VertexBefore(q, p);
					const TerrainVertex &a = swap ? q : p, &b = swap ? p : q;
					const double da = swap ? dq : dp, db = swap ? dp : dq;
					TerrainVertex crossing = LerpVertex(a, b, da / (da - db));
					(axisX ? crossing.x : crossing.z) = static_cast<float>(value);
					out.push_back(crossing);
				}
			}
		}
	}

	double WaterFloorDepth(const double surfaceElevationM, const double bathymetryElevationM, const double depthRatio)
	{
		return std::clamp(depthRatio, 0.0, 1.0) * std::max(surfaceElevationM - bathymetryElevationM, 0.0);
	}

	uint32_t CutTerrainAlongGrid(TerrainMeshPart &part, const double cell)
	{
		if (cell <= 0.0 || part.indices.empty())
			return static_cast<uint32_t>(part.indices.size() / 3);

		std::vector<TerrainVertex> vertices;
		std::vector<uint32_t> indices;
		vertices.reserve(part.vertices.size() * 2);
		indices.reserve(part.indices.size() * 2);

		// Welding: a 1 cm bucket grid; points within 5 mm on the ground plane are the same vertex
		constexpr double BUCKET = 0.01;
		constexpr double WELD = 0.005;
		std::unordered_map<uint64_t, std::vector<uint32_t>> buckets;
		const auto key = [](const int64_t bx, const int64_t bz) { return (static_cast<uint64_t>(bx) << 32) ^ (static_cast<uint64_t>(bz) & 0xFFFFFFFFull); };
		const auto weld = [&](const TerrainVertex &v)
		{
			const int64_t bx = static_cast<int64_t>(std::floor(v.x / BUCKET)), bz = static_cast<int64_t>(std::floor(v.z / BUCKET));
			for (int64_t dx = -1; dx <= 1; ++dx)
			{
				for (int64_t dz = -1; dz <= 1; ++dz)
				{
					const auto it = buckets.find(key(bx + dx, bz + dz));
					if (it == buckets.end())
						continue;
					for (const uint32_t index : it->second)
					{
						const TerrainVertex &other = vertices[index];
						if (std::abs(other.x - v.x) <= WELD && std::abs(other.z - v.z) <= WELD)
							return index;
					}
				}
			}
			const uint32_t index = static_cast<uint32_t>(vertices.size());
			vertices.push_back(v);
			buckets[key(bx, bz)].push_back(index);
			return index;
		};

		std::vector<TerrainVertex> polygon, clipped;
		for (size_t t = 0; t + 2 < part.indices.size(); t += 3)
		{
			const TerrainVertex corners[3] = { part.vertices[part.indices[t]], part.vertices[part.indices[t + 1]], part.vertices[part.indices[t + 2]] };
			const double minX = std::min({ corners[0].x, corners[1].x, corners[2].x }), maxX = std::max({ corners[0].x, corners[1].x, corners[2].x });
			const double minZ = std::min({ corners[0].z, corners[1].z, corners[2].z }), maxZ = std::max({ corners[0].z, corners[1].z, corners[2].z });
			const int64_t i0 = static_cast<int64_t>(std::floor(minX / cell)), i1 = static_cast<int64_t>(std::floor(maxX / cell));
			const int64_t j0 = static_cast<int64_t>(std::floor(minZ / cell)), j1 = static_cast<int64_t>(std::floor(maxZ / cell));
			for (int64_t i = i0; i <= i1; ++i)
			{
				for (int64_t j = j0; j <= j1; ++j)
				{
					polygon.assign(corners, corners + 3);
					const double x0 = static_cast<double>(i) * cell, z0 = static_cast<double>(j) * cell;
					if (i > i0)
					{
						ClipPolygon(polygon, clipped, true, x0, true);
						polygon.swap(clipped);
					}
					if (i < i1 && polygon.size() >= 3)
					{
						ClipPolygon(polygon, clipped, true, x0 + cell, false);
						polygon.swap(clipped);
					}
					if (j > j0 && polygon.size() >= 3)
					{
						ClipPolygon(polygon, clipped, false, z0, true);
						polygon.swap(clipped);
					}
					if (j < j1 && polygon.size() >= 3)
					{
						ClipPolygon(polygon, clipped, false, z0 + cell, false);
						polygon.swap(clipped);
					}
					if (polygon.size() < 3)
						continue;

					// A fan of the convex piece, slivers and pieces welded flat left out
					const uint32_t first = weld(polygon[0]);
					uint32_t previous = weld(polygon[1]);
					for (size_t k = 2; k < polygon.size(); ++k)
					{
						const uint32_t current = weld(polygon[k]);
						const TerrainVertex &a = vertices[first], &b = vertices[previous], &c = vertices[current];
						const double area = 0.5 * Cross2(b.x - a.x, b.z - a.z, c.x - a.x, c.z - a.z);
						if (first != previous && previous != current && current != first && std::abs(area) > MIN_PIECE_AREA)
						{
							indices.push_back(first);
							indices.push_back(previous);
							indices.push_back(current);
						}
						previous = current;
					}
				}
			}
		}

		part.vertices.swap(vertices);
		part.indices.swap(indices);
		return static_cast<uint32_t>(part.indices.size() / 3);
	}

	// -------------------------------------------------------

	double GetTerrainCurvatureDrop(const double x, const double z)
	{
		return (x * x + z * z) / (2.0 * TERRAIN_EARTH_RADIUS);
	}

	bool TerrainArea::Overlaps(const Point2 &a, const Point2 &b, const Point2 &c) const
	{
		if (rings.empty())
			return true;
		const Point2 corners[3] = { a, b, c };
		const double triMinX = std::min({ a.x, b.x, c.x }) - margin, triMaxX = std::max({ a.x, b.x, c.x }) + margin;
		const double triMinZ = std::min({ a.z, b.z, c.z }) - margin, triMaxZ = std::max({ a.z, b.z, c.z }) + margin;
		for (const std::vector<Point2> &ring : rings)
		{
			if (ring.empty())
				continue;
			// Quick reject on the boxes
			double minX = ring[0].x, maxX = ring[0].x, minZ = ring[0].z, maxZ = ring[0].z;
			for (const Point2 &p : ring)
			{
				minX = std::min(minX, p.x);
				maxX = std::max(maxX, p.x);
				minZ = std::min(minZ, p.z);
				maxZ = std::max(maxZ, p.z);
			}
			if (maxX < triMinX || minX > triMaxX || maxZ < triMinZ || minZ > triMaxZ)
				continue;

			// Inside one another, or edges within the margin
			if (ring.size() >= 3)
			{
				for (const Point2 &corner : corners)
				{
					if (PointInRing(corner, ring))
						return true;
				}
			}
			if (PointInTriangle(ring[0], a, b, c))
				return true;
			for (size_t i = 0; i < ring.size(); ++i)
			{
				const Point2 &p0 = ring[i];
				const Point2 &p1 = ring[(i + 1) % ring.size()];
				for (int e = 0; e < 3; ++e)
				{
					if (SegmentDistance(p0, p1, corners[e], corners[(e + 1) % 3]) <= margin)
						return true;
				}
			}
		}
		return false;
	}

	void TerrainArea::GetGeoBox(const GeoProjection &projection, double &west, double &south, double &east, double &north) const
	{
		Bounds2 bounds;
		for (const std::vector<Point2> &ring : rings)
		{
			for (const Point2 &p : ring)
				bounds.Add(p);
		}
		if (!bounds.valid)
			bounds.Add(Point2{});
		double lat0, lon0, lat1, lon1;
		projection.ToGeo(Point2{ bounds.minX - margin, bounds.maxZ + margin }, lat0, lon0);	// South-west (z is south)
		projection.ToGeo(Point2{ bounds.maxX + margin, bounds.minZ - margin }, lat1, lon1);	// North-east
		west = std::min(lon0, lon1);
		east = std::max(lon0, lon1);
		south = std::min(lat0, lat1);
		north = std::max(lat0, lat1);
	}

	// -------------------------------------------------------

	uint32_t BuildTerrainMeshes(const DsfFile &dsf, const GeoProjection &projection, const TerrainBuildOptions &options, std::vector<TerrainMeshPart> &out)
	{
		const DsfRaster *elevation = dsf.FindRaster("elevation");
		const double metersPerDegreeLat = projection.GetMetersPerDegreeLatitude();
		const double metersPerDegreeLon = projection.GetMetersPerDegreeLongitude();
		const double postLon = elevation && elevation->width > 1 ? (dsf.east - dsf.west) / (elevation->width - 1) : 0.0;
		const double postLat = elevation && elevation->height > 1 ? (dsf.north - dsf.south) / (elevation->height - 1) : 0.0;

		const auto sample = [&](const double lon, const double lat)
		{
			return elevation ? static_cast<double>(elevation->Sample(lon, lat, dsf.west, dsf.south, dsf.east, dsf.north)) : options.datumElevation;
		};

		std::map<std::pair<std::string, bool>, size_t> partIndex;	// (definition, overlay) -> part in out
		const size_t firstPart = out.size();
		uint32_t appended = 0;
		std::vector<uint32_t> remap;
		std::vector<Point2> local;
		std::vector<uint8_t> needsNormal;

		for (size_t patchIndex = 0; patchIndex < dsf.terrainPatches.size(); ++patchIndex)
		{
			const DsfTerrainPatch &patch = dsf.terrainPatches[patchIndex];
			if (patch.definition >= dsf.terrainDefinitions.size() || patch.planes < 3)
				continue;
			const std::string &definition = dsf.terrainDefinitions[patch.definition];
			const bool water = IsDsfWaterTerrain(definition);
			const bool overlay = patch.IsOverlay() && !water;

			// The patch's points on the ground plane, for the area test
			const uint32_t pointCount = patch.GetPointCount();
			local.resize(pointCount);
			for (uint32_t i = 0; i < pointCount; ++i)
			{
				const double *point = patch.GetPoint(i);
				local[i] = projection.ToLocal(point[1], point[0]);
			}

			TerrainMeshPart *part = nullptr;
			remap.assign(pointCount, NONE);
			for (size_t t = 0; t + 2 < patch.indices.size(); t += 3)
			{
				uint32_t corner[3] = { patch.indices[t], patch.indices[t + 1], patch.indices[t + 2] };
				if (corner[0] >= pointCount || corner[1] >= pointCount || corner[2] >= pointCount)
					continue;
				if (!options.area.Overlaps(local[corner[0]], local[corner[1]], local[corner[2]]))
					continue;

				if (!part)
				{
					const auto [it, added] = partIndex.try_emplace({ definition, overlay }, out.size());
					if (added)
					{
						TerrainMeshPart &created = out.emplace_back();
						created.definition = definition;
						created.overlay = overlay;
						created.water = water;
						created.order = static_cast<uint32_t>(patchIndex);
					}
					part = &out[it->second];
				}

				for (uint32_t &index : corner)
				{
					if (remap[index] != NONE)
					{
						index = remap[index];
						continue;
					}
					const double *point = patch.GetPoint(index);
					const double lon = point[0], lat = point[1];
					double height = point[2];
					if (height <= DSF_ELEVATION_FROM_RASTER + 0.5)
						height = sample(lon, lat);

					TerrainVertex vertex;
					vertex.x = static_cast<float>(local[index].x);
					vertex.z = static_cast<float>(local[index].z);
					vertex.y = static_cast<float>(height - options.datumElevation - (options.earthCurvature ? GetTerrainCurvatureDrop(local[index].x, local[index].z) : 0.0));
					bool computed = true;
					if (water)
					{
						vertex.nx = 0.0f;
						vertex.ny = 1.0f;
						vertex.nz = 0.0f;
					}
					else if (patch.planes >= 5 && (std::abs(point[3]) > 1e-3 || std::abs(point[4]) > 1e-3))
					{
						// East and north ratios of the normal (z is south here)
						vertex.nx = static_cast<float>(point[3]);
						vertex.nz = static_cast<float>(-point[4]);
						vertex.ny = std::sqrt(std::max(0.0f, 1.0f - vertex.nx * vertex.nx - vertex.nz * vertex.nz));
					}
					else if (elevation && postLon > 0.0 && postLat > 0.0)
					{
						// The raster's slope, a post either side
						const double east = (sample(lon + postLon, lat) - sample(lon - postLon, lat)) / (2.0 * postLon * metersPerDegreeLon);
						const double north = (sample(lon, lat + postLat) - sample(lon, lat - postLat)) / (2.0 * postLat * metersPerDegreeLat);
						vertex.nx = static_cast<float>(-east);
						vertex.ny = 1.0f;
						vertex.nz = static_cast<float>(north);
						Normalize(vertex.nx, vertex.ny, vertex.nz);
					}
					else
						computed = false;
					if (patch.planes >= 7)
					{
						vertex.s = static_cast<float>(point[5]);
						vertex.t = static_cast<float>(point[6]);
					}

					remap[index] = static_cast<uint32_t>(part->vertices.size());
					part->vertices.push_back(vertex);
					needsNormal.resize(part->vertices.size(), 0);
					needsNormal.back() = computed ? 0 : 1;
					index = remap[index];
				}

				// Face up (counter-clockwise seen from above: the cross product's y is positive)
				const TerrainVertex &a = part->vertices[corner[0]], &b = part->vertices[corner[1]], &c = part->vertices[corner[2]];
				const float crossY = (b.z - a.z) * (c.x - a.x) - (b.x - a.x) * (c.z - a.z);
				if (crossY < 0.0f)
					std::swap(corner[1], corner[2]);
				part->indices.insert(part->indices.end(), corner, corner + 3);
				++appended;
			}

			// Normals from the mesh where neither the patch nor a raster gave one
			if (part && std::ranges::any_of(needsNormal, [](const uint8_t flag) { return flag != 0; }))
			{
				for (size_t i = 0; i < part->vertices.size() && i < needsNormal.size(); ++i)
				{
					if (needsNormal[i])
						part->vertices[i].nx = part->vertices[i].ny = part->vertices[i].nz = 0.0f;
				}
				for (size_t t = 0; t + 2 < part->indices.size(); t += 3)
				{
					TerrainVertex *v[3] = { &part->vertices[part->indices[t]], &part->vertices[part->indices[t + 1]], &part->vertices[part->indices[t + 2]] };
					const float ex = v[1]->x - v[0]->x, ey = v[1]->y - v[0]->y, ez = v[1]->z - v[0]->z;
					const float fx = v[2]->x - v[0]->x, fy = v[2]->y - v[0]->y, fz = v[2]->z - v[0]->z;
					// (b - a) x (c - a), which points up for counter-clockwise from above
					const float nx = ey * fz - ez * fy, ny = ez * fx - ex * fz, nz = ex * fy - ey * fx;
					for (int k = 0; k < 3; ++k)
					{
						const size_t index = part->indices[t + k];
						if (index < needsNormal.size() && needsNormal[index])
						{
							v[k]->nx += nx;
							v[k]->ny += ny;
							v[k]->nz += nz;
						}
					}
				}
				for (size_t i = 0; i < part->vertices.size() && i < needsNormal.size(); ++i)
				{
					if (needsNormal[i])
						Normalize(part->vertices[i].nx, part->vertices[i].ny, part->vertices[i].nz);
				}
			}
			needsNormal.clear();
		}

		// Water: cut for tessellation, then each vertex's floor depth from its depth ratio and the bathymetry under it
		const DsfRaster *bathymetry = dsf.FindRaster("sea_level");
		for (size_t p = firstPart; p < out.size(); ++p)
		{
			TerrainMeshPart &part = out[p];
			if (!part.water)
				continue;
			const uint32_t before = static_cast<uint32_t>(part.indices.size() / 3);
			const uint32_t after = CutTerrainAlongGrid(part, options.waterCellSize);
			appended = appended - before + after;
			for (TerrainVertex &vertex : part.vertices)
			{
				const double surface = vertex.y + options.datumElevation + (options.earthCurvature ? GetTerrainCurvatureDrop(vertex.x, vertex.z) : 0.0);
				double floor = surface - options.waterDepthWithoutBathymetryM;
				if (bathymetry)
				{
					double lat, lon;
					projection.ToGeo(Point2{ vertex.x, vertex.z }, lat, lon);
					floor = bathymetry->Sample(lon, lat, dsf.west, dsf.south, dsf.east, dsf.north);
				}
				vertex.t = static_cast<float>(WaterFloorDepth(surface, floor, vertex.t));
			}
		}
		return appended;
	}

	// -------------------------------------------------------

	void TerrainSurface::Clear()
	{
		m_Positions.clear();
		m_Normals.clear();
		m_Triangles.clear();
		m_TriangleBoxes.clear();
		m_Bounds = Bounds2();
		m_Cells.clear();
		m_CellTriangles.clear();
		m_Stamp.clear();
		m_CellsX = m_CellsZ = 0;
		m_Query = 0;
	}

	void TerrainSurface::Build(const std::vector<TerrainMeshPart> &parts)
	{
		Clear();
		for (const TerrainMeshPart &part : parts)
		{
			if (part.overlay)
				continue;
			const uint32_t base = static_cast<uint32_t>(m_Positions.size() / 3);
			for (const TerrainVertex &vertex : part.vertices)
			{
				m_Positions.insert(m_Positions.end(), { vertex.x, vertex.y, vertex.z });
				m_Normals.insert(m_Normals.end(), { vertex.nx, vertex.ny, vertex.nz });
				m_Bounds.Add(Point2{ vertex.x, vertex.z });
			}
			for (const uint32_t index : part.indices)
				m_Triangles.push_back(base + index);
		}
		const uint32_t triangles = GetTriangleCount();
		if (triangles == 0)
		{
			Clear();
			return;
		}

		m_TriangleBoxes.resize(static_cast<size_t>(triangles) * 4);
		for (uint32_t t = 0; t < triangles; ++t)
		{
			const float *a = &m_Positions[m_Triangles[t * 3] * 3];
			const float *b = &m_Positions[m_Triangles[t * 3 + 1] * 3];
			const float *c = &m_Positions[m_Triangles[t * 3 + 2] * 3];
			float *box = &m_TriangleBoxes[static_cast<size_t>(t) * 4];
			box[0] = std::min({ a[0], b[0], c[0] });
			box[1] = std::min({ a[2], b[2], c[2] });
			box[2] = std::max({ a[0], b[0], c[0] });
			box[3] = std::max({ a[2], b[2], c[2] });
		}

		// A grid of about two triangles a cell
		const double width = std::max(1.0, m_Bounds.maxX - m_Bounds.minX), depth = std::max(1.0, m_Bounds.maxZ - m_Bounds.minZ);
		m_CellSize = std::max({ 10.0, std::sqrt(width * depth / triangles * 2.0), std::max(width, depth) / MAX_CELLS_PER_AXIS });
		m_CellsX = std::max(1, static_cast<int>(std::ceil(width / m_CellSize)));
		m_CellsZ = std::max(1, static_cast<int>(std::ceil(depth / m_CellSize)));
		m_Cells.assign(static_cast<size_t>(m_CellsX) * m_CellsZ, Cell{});

		const auto cellRange = [this](const float *box, int &x0, int &z0, int &x1, int &z1)
		{
			x0 = std::clamp(static_cast<int>((box[0] - m_Bounds.minX) / m_CellSize), 0, m_CellsX - 1);
			z0 = std::clamp(static_cast<int>((box[1] - m_Bounds.minZ) / m_CellSize), 0, m_CellsZ - 1);
			x1 = std::clamp(static_cast<int>((box[2] - m_Bounds.minX) / m_CellSize), 0, m_CellsX - 1);
			z1 = std::clamp(static_cast<int>((box[3] - m_Bounds.minZ) / m_CellSize), 0, m_CellsZ - 1);
		};
		for (uint32_t t = 0; t < triangles; ++t)
		{
			int x0, z0, x1, z1;
			cellRange(&m_TriangleBoxes[static_cast<size_t>(t) * 4], x0, z0, x1, z1);
			for (int z = z0; z <= z1; ++z)
				for (int x = x0; x <= x1; ++x)
					++m_Cells[static_cast<size_t>(z) * m_CellsX + x].count;
		}
		uint32_t offset = 0;
		for (Cell &cell : m_Cells)
		{
			cell.first = offset;
			offset += cell.count;
			cell.count = 0;
		}
		m_CellTriangles.resize(offset);
		for (uint32_t t = 0; t < triangles; ++t)
		{
			int x0, z0, x1, z1;
			cellRange(&m_TriangleBoxes[static_cast<size_t>(t) * 4], x0, z0, x1, z1);
			for (int z = z0; z <= z1; ++z)
			{
				for (int x = x0; x <= x1; ++x)
				{
					Cell &cell = m_Cells[static_cast<size_t>(z) * m_CellsX + x];
					m_CellTriangles[cell.first + cell.count++] = t;
				}
			}
		}
		m_Stamp.assign(triangles, 0);
	}

	void TerrainSurface::Gather(const double minX, const double minZ, const double maxX, const double maxZ, std::vector<uint32_t> &out) const
	{
		out.clear();
		if (m_Cells.empty() || maxX < m_Bounds.minX || minX > m_Bounds.maxX || maxZ < m_Bounds.minZ || minZ > m_Bounds.maxZ)
			return;
		if (++m_Query == 0)
		{
			std::fill(m_Stamp.begin(), m_Stamp.end(), 0u);
			m_Query = 1;
		}
		const int x0 = std::clamp(static_cast<int>((minX - m_Bounds.minX) / m_CellSize), 0, m_CellsX - 1);
		const int z0 = std::clamp(static_cast<int>((minZ - m_Bounds.minZ) / m_CellSize), 0, m_CellsZ - 1);
		const int x1 = std::clamp(static_cast<int>((maxX - m_Bounds.minX) / m_CellSize), 0, m_CellsX - 1);
		const int z1 = std::clamp(static_cast<int>((maxZ - m_Bounds.minZ) / m_CellSize), 0, m_CellsZ - 1);
		for (int z = z0; z <= z1; ++z)
		{
			for (int x = x0; x <= x1; ++x)
			{
				const Cell &cell = m_Cells[static_cast<size_t>(z) * m_CellsX + x];
				for (uint32_t i = 0; i < cell.count; ++i)
				{
					const uint32_t t = m_CellTriangles[cell.first + i];
					if (m_Stamp[t] == m_Query)
						continue;
					m_Stamp[t] = m_Query;
					const float *box = &m_TriangleBoxes[static_cast<size_t>(t) * 4];
					if (box[2] < minX || box[0] > maxX || box[3] < minZ || box[1] > maxZ)
						continue;
					out.push_back(t);
				}
			}
		}
	}

	uint32_t TerrainSurface::Locate(const double x, const double z, double bary[3]) const
	{
		if (m_Cells.empty() || x < m_Bounds.minX || x > m_Bounds.maxX || z < m_Bounds.minZ || z > m_Bounds.maxZ)
			return NONE;
		const int cx = std::clamp(static_cast<int>((x - m_Bounds.minX) / m_CellSize), 0, m_CellsX - 1);
		const int cz = std::clamp(static_cast<int>((z - m_Bounds.minZ) / m_CellSize), 0, m_CellsZ - 1);
		const Cell &cell = m_Cells[static_cast<size_t>(cz) * m_CellsX + cx];
		uint32_t best = NONE;
		double bestScore = -BARY_EPSILON;
		for (uint32_t i = 0; i < cell.count; ++i)
		{
			const uint32_t t = m_CellTriangles[cell.first + i];
			const float *a = &m_Positions[m_Triangles[t * 3] * 3];
			const float *b = &m_Positions[m_Triangles[t * 3 + 1] * 3];
			const float *c = &m_Positions[m_Triangles[t * 3 + 2] * 3];
			double candidate[3];
			if (!Barycentric(a[0], a[2], b[0], b[2], c[0], c[2], x, z, candidate))
				continue;
			// On an edge two triangles qualify: keep the one the point is deepest in
			const double score = std::min({ candidate[0], candidate[1], candidate[2] });
			if (score >= bestScore)
			{
				bestScore = score;
				best = t;
				bary[0] = candidate[0];
				bary[1] = candidate[1];
				bary[2] = candidate[2];
				if (score > BARY_EPSILON)
					break;
			}
		}
		return best;
	}

	bool TerrainSurface::HeightAt(const double x, const double z, float &y, float normal[3]) const
	{
		double bary[3];
		const uint32_t t = Locate(x, z, bary);
		if (t == NONE)
			return false;
		y = 0.0f;
		float n[3] = { 0.0f, 0.0f, 0.0f };
		for (int k = 0; k < 3; ++k)
		{
			const uint32_t vertex = m_Triangles[t * 3 + k];
			y += static_cast<float>(bary[k]) * m_Positions[vertex * 3 + 1];
			for (int axis = 0; axis < 3; ++axis)
				n[axis] += static_cast<float>(bary[k]) * m_Normals[vertex * 3 + axis];
		}
		if (normal)
		{
			Normalize(n[0], n[1], n[2]);
			normal[0] = n[0];
			normal[1] = n[1];
			normal[2] = n[2];
		}
		return true;
	}

	float TerrainSurface::HeightOr(const double x, const double z, const float fallback) const
	{
		float y = fallback;
		return HeightAt(x, z, y) ? y : fallback;
	}

	bool TerrainSurface::Raycast(const double origin[3], const double direction[3], const double maxDistance, double &distance) const
	{
		bool hit = false;
		distance = maxDistance;
		const uint32_t triangles = GetTriangleCount();
		for (uint32_t t = 0; t < triangles; ++t)
		{
			// Moeller-Trumbore, both faces
			const float *a = &m_Positions[m_Triangles[t * 3] * 3];
			const float *b = &m_Positions[m_Triangles[t * 3 + 1] * 3];
			const float *c = &m_Positions[m_Triangles[t * 3 + 2] * 3];
			const double e1[3] = { b[0] - a[0], b[1] - a[1], b[2] - a[2] };
			const double e2[3] = { c[0] - a[0], c[1] - a[1], c[2] - a[2] };
			const double p[3] = { direction[1] * e2[2] - direction[2] * e2[1], direction[2] * e2[0] - direction[0] * e2[2], direction[0] * e2[1] - direction[1] * e2[0] };
			const double det = e1[0] * p[0] + e1[1] * p[1] + e1[2] * p[2];
			if (std::abs(det) < 1e-12)
				continue;
			const double inverse = 1.0 / det;
			const double s[3] = { origin[0] - a[0], origin[1] - a[1], origin[2] - a[2] };
			const double u = (s[0] * p[0] + s[1] * p[1] + s[2] * p[2]) * inverse;
			if (u < 0.0 || u > 1.0)
				continue;
			const double q[3] = { s[1] * e1[2] - s[2] * e1[1], s[2] * e1[0] - s[0] * e1[2], s[0] * e1[1] - s[1] * e1[0] };
			const double v = (direction[0] * q[0] + direction[1] * q[1] + direction[2] * q[2]) * inverse;
			if (v < 0.0 || u + v > 1.0)
				continue;
			const double along = (e2[0] * q[0] + e2[1] * q[1] + e2[2] * q[2]) * inverse;
			if (along >= 0.0 && along < distance)
			{
				distance = along;
				hit = true;
			}
		}
		return hit;
	}

	uint32_t TerrainSurface::Drape(std::vector<OverlayVertex> &vertices, std::vector<uint32_t> &indices, const bool clip) const
	{
		if (IsEmpty() || indices.size() < 3)
			return static_cast<uint32_t>(indices.size() / 3);

		std::vector<OverlayVertex> outVertices;
		std::vector<uint32_t> outIndices;
		outVertices.reserve(vertices.size() + vertices.size() / 2);
		outIndices.reserve(indices.size() + indices.size() / 2);

		std::vector<uint32_t> candidates;
		std::vector<ClipPoint> polygon, clipped;

		// A drape vertex lifted onto terrain triangle t at its ground position: its own y is the offset, up normals turn
		// with the terrain
		const auto onTerrain = [this](OverlayVertex vertex, const uint32_t t, const double *bary)
		{
			float height = 0.0f, n[3] = { 0.0f, 0.0f, 0.0f };
			for (int k = 0; k < 3; ++k)
			{
				const uint32_t index = m_Triangles[t * 3 + k];
				height += static_cast<float>(bary[k]) * m_Positions[index * 3 + 1];
				for (int axis = 0; axis < 3; ++axis)
					n[axis] += static_cast<float>(bary[k]) * m_Normals[index * 3 + axis];
			}
			vertex.y += height;
			if (vertex.ny > 0.9f)
			{
				Normalize(n[0], n[1], n[2]);
				vertex.nx = n[0];
				vertex.ny = n[1];
				vertex.nz = n[2];
			}
			return vertex;
		};

		for (size_t i = 0; i + 2 < indices.size(); i += 3)
		{
			const uint32_t ia = indices[i], ib = indices[i + 1], ic = indices[i + 2];
			if (ia >= vertices.size() || ib >= vertices.size() || ic >= vertices.size())
				continue;
			const OverlayVertex &A = vertices[ia], &B = vertices[ib], &C = vertices[ic];

			// Inside one terrain triangle (convex: its corners in it means all of it is): lift the corners
			double baryA[3], baryB[3], baryC[3];
			const uint32_t ta = Locate(A.x, A.z, baryA);
			if (ta != NONE)
			{
				const uint32_t tb = Locate(B.x, B.z, baryB);
				const uint32_t tc = tb == ta ? Locate(C.x, C.z, baryC) : NONE;
				if (tb == ta && tc == ta)
				{
					const uint32_t base = static_cast<uint32_t>(outVertices.size());
					outVertices.push_back(onTerrain(A, ta, baryA));
					outVertices.push_back(onTerrain(B, ta, baryB));
					outVertices.push_back(onTerrain(C, ta, baryC));
					outIndices.insert(outIndices.end(), { base, base + 1, base + 2 });
					continue;
				}
			}

			const double triArea = Cross2(B.x - A.x, B.z - A.z, C.x - A.x, C.z - A.z);
			if (std::abs(triArea) < 1e-9)
			{
				// Seen edge-on from above (a wall in a drape): lift each corner where it stands
				if (clip && ta == NONE)
					continue;
				const uint32_t base = static_cast<uint32_t>(outVertices.size());
				for (const OverlayVertex *vertex : { &A, &B, &C })
				{
					OverlayVertex lifted = *vertex;
					lifted.y += HeightOr(vertex->x, vertex->z, 0.0f);
					outVertices.push_back(lifted);
				}
				outIndices.insert(outIndices.end(), { base, base + 1, base + 2 });
				continue;
			}

			Gather(std::min({ A.x, B.x, C.x }), std::min({ A.z, B.z, C.z }), std::max({ A.x, B.x, C.x }), std::max({ A.z, B.z, C.z }), candidates);
			bool emitted = false;
			for (const uint32_t t : candidates)
			{
				const float *k0 = &m_Positions[m_Triangles[t * 3] * 3];
				const float *k1 = &m_Positions[m_Triangles[t * 3 + 1] * 3];
				const float *k2 = &m_Positions[m_Triangles[t * 3 + 2] * 3];
				const double kx[3] = { k0[0], k1[0], k2[0] }, kz[3] = { k0[2], k1[2], k2[2] };
				const double orientation = Cross2(kx[1] - kx[0], kz[1] - kz[0], kx[2] - kx[0], kz[2] - kz[0]) >= 0.0 ? 1.0 : -1.0;

				// Sutherland-Hodgman: the drape triangle against the terrain triangle's three edges
				polygon.assign({ { A.x, A.z }, { B.x, B.z }, { C.x, C.z } });
				for (int e = 0; e < 3 && polygon.size() >= 3; ++e)
				{
					const double ex = kx[(e + 1) % 3] - kx[e], ez = kz[(e + 1) % 3] - kz[e];
					clipped.clear();
					for (size_t p = 0; p < polygon.size(); ++p)
					{
						const ClipPoint &current = polygon[p];
						const ClipPoint &next = polygon[(p + 1) % polygon.size()];
						const double dc = Cross2(ex, ez, current.x - kx[e], current.z - kz[e]) * orientation;
						const double dn = Cross2(ex, ez, next.x - kx[e], next.z - kz[e]) * orientation;
						if (dc >= 0.0)
							clipped.push_back(current);
						if ((dc >= 0.0) != (dn >= 0.0))
						{
							const double f = dc / (dc - dn);
							clipped.push_back({ current.x + (next.x - current.x) * f, current.z + (next.z - current.z) * f });
						}
					}
					polygon.swap(clipped);
				}
				if (polygon.size() < 3)
					continue;
				double area = 0.0;
				for (size_t p = 1; p + 1 < polygon.size(); ++p)
					area += Cross2(polygon[p].x - polygon[0].x, polygon[p].z - polygon[0].z, polygon[p + 1].x - polygon[0].x, polygon[p + 1].z - polygon[0].z);
				if (std::abs(area) * 0.5 < MIN_PIECE_AREA)
					continue;

				// The piece's corners: attributes from the drape triangle, height from the terrain triangle
				const uint32_t base = static_cast<uint32_t>(outVertices.size());
				for (const ClipPoint &point : polygon)
				{
					double inDrape[3] = { 1.0, 0.0, 0.0 }, inTerrain[3] = { 1.0, 0.0, 0.0 };
					Barycentric(A.x, A.z, B.x, B.z, C.x, C.z, point.x, point.z, inDrape);
					Barycentric(kx[0], kz[0], kx[1], kz[1], kx[2], kz[2], point.x, point.z, inTerrain);
					OverlayVertex vertex;
					vertex.x = static_cast<float>(point.x);
					vertex.z = static_cast<float>(point.z);
					vertex.y = static_cast<float>(inDrape[0] * A.y + inDrape[1] * B.y + inDrape[2] * C.y);
					vertex.u = static_cast<float>(inDrape[0] * A.u + inDrape[1] * B.u + inDrape[2] * C.u);
					vertex.v = static_cast<float>(inDrape[0] * A.v + inDrape[1] * B.v + inDrape[2] * C.v);
					vertex.nx = static_cast<float>(inDrape[0] * A.nx + inDrape[1] * B.nx + inDrape[2] * C.nx);
					vertex.ny = static_cast<float>(inDrape[0] * A.ny + inDrape[1] * B.ny + inDrape[2] * C.ny);
					vertex.nz = static_cast<float>(inDrape[0] * A.nz + inDrape[1] * B.nz + inDrape[2] * C.nz);
					outVertices.push_back(onTerrain(vertex, t, inTerrain));
				}
				for (uint32_t p = 1; p + 1 < polygon.size(); ++p)
					outIndices.insert(outIndices.end(), { base, base + p, base + p + 1 });
				emitted = true;
			}

			if (!emitted && !clip)
			{
				// No terrain under it: as it was
				const uint32_t base = static_cast<uint32_t>(outVertices.size());
				outVertices.insert(outVertices.end(), { A, B, C });
				outIndices.insert(outIndices.end(), { base, base + 1, base + 2 });
			}
		}

		vertices.swap(outVertices);
		indices.swap(outIndices);
		return static_cast<uint32_t>(indices.size() / 3);
	}

	uint32_t TerrainSurface::Drape(OverlayMesh &mesh, const bool clip) const
	{
		return Drape(mesh.vertices, mesh.indices, clip);
	}

	void TerrainSurface::Lift(std::vector<OverlayVertex> &vertices) const
	{
		if (IsEmpty())
			return;
		for (OverlayVertex &vertex : vertices)
			vertex.y += HeightOr(vertex.x, vertex.z, 0.0f);
	}
}

#if defined(_MSC_VER) && !defined(__clang__) && !defined(NDEBUG)
#pragma optimize("", on)
#endif

// -------------------------------------------------------

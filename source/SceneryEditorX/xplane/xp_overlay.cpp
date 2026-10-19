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
		}
		return "Overlay";
	}

	// -------------------------------------------------------

	void AppendTexturedPolygon(OverlayMesh &mesh, const std::vector<std::vector<Point2>> &rings, const float y, const float scaleS, const float scaleT, const double headingDegrees)
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
		const uint32_t base = static_cast<uint32_t>(mesh.vertices.size());
		for (const Point2 &point : points)
		{
			// Texture t runs north (-z); a heading turns the texture clockwise
			const double east = point.x, north = -point.z;
			const double u = (east * c - north * s) * invS;
			const double v = (east * s + north * c) * invT;
			AddVertex(mesh, point.x, y, point.z, u, v);
		}
		for (const uint32_t index : indices)
			mesh.indices.push_back(base + index);
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
			object.y = placement.mode == DsfObjectPlacement::Mode::Agl ? placement.elevation : 0.0;	// MSL elevations need terrain we don't have: ground them
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

		out.unsupported += static_cast<uint32_t>(dsf.networks.size());
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
			AppendTexturedPolygon(mesh, rings, y, asset->scaleS * static_cast<float>(asset->tilesX), asset->scaleT * static_cast<float>(asset->tilesY), heading);
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
		const LineAsset::Segment &segment = asset->segments[0];
		const float y = Height(options, asset->layerGroup, asset->layerOffset);
		AppendTexturedRibbon(mesh, path, closed, y, asset->GetWidthMeters(0) + 2.0f / asset->textureWidth * asset->scaleS, segment.s1, segment.s2, asset->scaleT, asset->mirror);
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
		struct WallSide
		{
			Point2 a, b;
			double length = 0.0;
			Point2 direction;		// Unit, a -> b
			Point2 outward;			// Unit, to the right of travel (outside a counter-clockwise ring)
			int wallType = 0;
		};

		std::vector<WallSide> MakeSides(const std::vector<Point2> &ring, const std::vector<int> &wallTypes, const bool closed)
		{
			std::vector<WallSide> sides;
			const size_t count = ring.size();
			const size_t sideCount = closed ? count : (count > 0 ? count - 1 : 0);
			for (size_t i = 0; i < sideCount; ++i)
			{
				WallSide side;
				side.a = ring[i];
				side.b = ring[(i + 1) % count];
				const double dx = side.b.x - side.a.x, dz = side.b.z - side.a.z;
				side.length = std::hypot(dx, dz);
				if (side.length < 0.05)
					continue;
				side.direction = { dx / side.length, dz / side.length };
				side.outward = { -side.direction.z, side.direction.x };
				side.wallType = i < wallTypes.size() ? wallTypes[i] : 0;
				sides.push_back(side);
			}
			return sides;
		}

		/* @brief Picks the segment sequence whose total width is closest to the wall length (repeating the longest when the wall is longer than any). */
		std::vector<int> ChooseSpelling(const FacadeAsset::Floor &floor, const FacadeAsset::WallRule &rule, const double length, double &total)
		{
			std::vector<int> best;
			double bestTotal = 0.0;
			double bestError = 1e30;
			const auto widthOf = [&floor](const std::vector<int> &spelling)
			{
				double width = 0.0;
				for (const int s : spelling)
				{
					if (s >= 0 && static_cast<size_t>(s) < floor.segments.size())
						width += floor.segments[static_cast<size_t>(s)].width;
				}
				return width;
			};
			for (const std::vector<int> &spelling : rule.spellings)
			{
				const double width = widthOf(spelling);
				if (width <= 0.0)
					continue;
				const double error = std::abs(width - length);
				if (error < bestError)
				{
					bestError = error;
					best = spelling;
					bestTotal = width;
				}
			}
			if (best.empty())
			{
				total = 0.0;
				return best;
			}

			// Longer walls: repeat the chosen spelling until it's close
			while (bestTotal > 0.0 && length > bestTotal * 1.5)
			{
				std::vector<int> doubled = best;
				doubled.insert(doubled.end(), best.begin(), best.end());
				bestTotal *= 2.0;
				best = std::move(doubled);
				if (best.size() > 512)
					break;
			}
			total = bestTotal;
			return best;
		}
	}

	void OverlayBuilder::BuildFacade(const DsfPolygonPlacement &placement, const uint32_t index, const std::string &resource, const Options &options, OverlayBuild &out)
	{
		fs::path file;
		const FacadeAsset *asset = GetFacade(resource, &file, out);
		if (!asset)
			return;

		// Planes after lon / lat: 3 wall type, 4 bezier control, 5 wall type then bezier control (WorldEditor's stacking)
		const bool hasWallTypes = placement.planes == 3 || placement.planes == 5;
		const bool bezier = placement.planes >= 4;
		const size_t controlPlane = placement.planes == 5 ? 3 : 2;
		std::vector<std::vector<Point2>> rings;
		std::vector<std::vector<int>> wallTypes;
		for (const std::vector<DsfPoint> &winding : placement.windings)
		{
			rings.emplace_back();
			FlattenWinding(winding, asset->ring, bezier, rings.back(), options, controlPlane);
			wallTypes.emplace_back();
			if (hasWallTypes)
			{
				for (const DsfPoint &point : winding)
					wallTypes.back().push_back(static_cast<int>(point.v[2]));
			}
			if (rings.back().size() != winding.size())
				wallTypes.back().clear();	// Curves re-sampled the ring; wall types no longer line up
		}
		if (rings.empty() || rings[0].size() < 2)
			return;
		if (!PolygonInArea(options, rings))
		{
			++out.culled;
			return;
		}

		const double height = placement.parameter > 0 ? static_cast<double>(placement.parameter) : 3.0;
		const std::vector<WallSide> sides = MakeSides(rings[0], wallTypes[0], asset->ring);
		if (sides.empty())
			return;

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
		const FacadeAsset::Floor *floor = nullptr;

		if (asset->IsType2())
		{
			// Type 2: the floor whose roof heights come closest to the asked height, then segment meshes per wall
			double bestError = 1e30;
			for (const FacadeAsset::Floor &candidate : asset->floors)
			{
				if (candidate.segments.empty() || candidate.walls.empty())
					continue;
				double error = 0.0;
				if (!candidate.roofHeights.empty())
				{
					error = 1e30;
					for (const float h : candidate.roofHeights)
						error = std::min(error, std::abs(h - height));
				}
				if (error < bestError)
				{
					bestError = error;
					floor = &candidate;
				}
			}
			if (!floor)
			{
				++out.unsupported;
				return;
			}
			if (!floor->roofHeights.empty())
			{
				roofHeight = floor->roofHeights[0];
				for (const float h : floor->roofHeights)
				{
					if (std::abs(h - height) < std::abs(roofHeight - height))
						roofHeight = h;
				}
			}

			if (!asset->noWallMesh)
			{
				for (const WallSide &side : sides)
				{
					const FacadeAsset::WallRule &rule = floor->walls[static_cast<size_t>(std::clamp(side.wallType, 0, static_cast<int>(floor->walls.size()) - 1))];
					double total = 0.0;
					const std::vector<int> spelling = ChooseSpelling(*floor, rule, side.length, total);
					if (spelling.empty() || total <= 0.0)
						continue;
					const double stretch = side.length / total;

					// Template space: x outward, y up, -z along the wall; each segment spans z in [-width, 0]
					double along = 0.0;
					for (const int segmentIndex : spelling)
					{
						if (segmentIndex < 0 || static_cast<size_t>(segmentIndex) >= floor->segments.size())
							continue;
						const FacadeAsset::Segment &segment = floor->segments[static_cast<size_t>(segmentIndex)];
						for (const FacadeAsset::Mesh &m : segment.meshes)
						{
							const uint32_t base = static_cast<uint32_t>(walls.vertices.size());
							const size_t vertexCount = m.positions.size() / 3;
							for (size_t v = 0; v < vertexCount; ++v)
							{
								const double lx = m.positions[v * 3], ly = m.positions[v * 3 + 1], lz = m.positions[v * 3 + 2];
								const double d = along + (-lz) * stretch;	// Distance from the side's start
								const double wx = side.a.x + side.direction.x * d + side.outward.x * lx;
								const double wz = side.a.z + side.direction.z * d + side.outward.z * lx;
								const float nx = v * 3 + 2 < m.normals.size() ? m.normals[v * 3] : 0.0f;
								const float ny = v * 3 + 2 < m.normals.size() ? m.normals[v * 3 + 1] : 1.0f;
								const float nz = v * 3 + 2 < m.normals.size() ? m.normals[v * 3 + 2] : 0.0f;
								// Rotate the normal like the position (x -> outward, -z -> along)
								const float wnx = static_cast<float>(side.outward.x * nx - side.direction.x * nz);
								const float wnz = static_cast<float>(side.outward.z * nx - side.direction.z * nz);
								AddVertex(walls, wx, ly, wz, v * 2 + 1 < m.uvs.size() ? m.uvs[v * 2] : 0.0, v * 2 + 1 < m.uvs.size() ? m.uvs[v * 2 + 1] : 0.0, wnx, ny, wnz);
							}
							for (size_t i = 0; i + 2 < m.indices.size(); i += 3)
							{
								// OBJ-style clockwise triangles become the engine's counter-clockwise
								if (m.indices[i] < vertexCount && m.indices[i + 1] < vertexCount && m.indices[i + 2] < vertexCount)
									walls.indices.insert(walls.indices.end(), { base + m.indices[i], base + m.indices[i + 2], base + m.indices[i + 1] });
							}
						}

						// Attached objects (jetway tunnels, light poles...)
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
							const double d = along + (-attachment.z) * stretch;
							OverlayObject object;
							object.source = OverlayObject::Source::Facade;
							object.file = objectFile;
							object.resource = resource;
							object.name = BaseName(name);
							object.x = side.a.x + side.direction.x * d + side.outward.x * attachment.x;
							object.z = side.a.z + side.direction.z * d + side.outward.z * attachment.x;
							object.y = attachment.y;
							object.heading = static_cast<float>(DirectionToHeading(side.outward)) + attachment.heading;
							object.placement = index;
							out.objects.push_back(std::move(object));
						}
						along += segment.width * stretch;
					}
				}
			}
		}
		else
		{
			// Type 1: texture panels. Pick the wall whose width range fits, tile left / center / right panels across
			// and bottom / middle / top floors up, stretched to fit the side exactly.
			if (asset->walls.empty())
				return;
			for (const WallSide &side : sides)
			{
				const FacadeAsset::Wall *wall = &asset->walls[0];
				if (side.wallType > 0 && static_cast<size_t>(side.wallType) < asset->walls.size())
					wall = &asset->walls[static_cast<size_t>(side.wallType)];
				else
				{
					for (const FacadeAsset::Wall &candidate : asset->walls)
					{
						if (side.length >= candidate.minWidth && side.length <= candidate.maxWidth)
						{
							wall = &candidate;
							break;
						}
					}
				}

				// Horizontal panels: left once, center repeated, right once
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
				const float normal[3] = { static_cast<float>(side.outward.x), 0.0f, static_cast<float>(side.outward.z) };

				double x = 0.0;
				for (const Span &column : columns)
				{
					const double w = column.size * stretchX;
					double yPos = 0.0;
					for (const Span &row : rows)
					{
						const double h = row.size * stretchY;
						const double corners[4][3] =
						{
							{ side.a.x + side.direction.x * x, yPos, side.a.z + side.direction.z * x },
							{ side.a.x + side.direction.x * (x + w), yPos, side.a.z + side.direction.z * (x + w) },
							{ side.a.x + side.direction.x * (x + w), yPos + h, side.a.z + side.direction.z * (x + w) },
							{ side.a.x + side.direction.x * x, yPos + h, side.a.z + side.direction.z * x }
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
		if (asset->ring && asset->hasRoof && !asset->noRoofMesh && rings[0].size() >= 3)
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
			if (asset->IsType2() || asset->roofUv.size() != 4)
				AppendTexturedPolygon(roof, { rings[0] }, static_cast<float>(roofHeight), asset->roofScaleS > 0.01f ? asset->roofScaleS : 10.0f, asset->roofScaleT > 0.01f ? asset->roofScaleT : 10.0f, 0.0);
			else
			{
				// Type 1 ROOF corners: map the footprint's bounding box onto the texture rectangle
				std::vector<Point2> points;
				std::vector<uint32_t> indices;
				if (TriangulatePolygon({ rings[0] }, points, indices))
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
			const std::string library = GetSurfaceLibraryPolygon(surface);
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

		for (uint32_t i = 0; i < airport.pavements.size(); ++i)
		{
			const AptPavement &pavement = airport.pavements[i];
			if (GetSurfaceFamily(pavement.surface) == SurfaceFamily::Transparent)
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
			AppendTexturedPolygon(mesh, rings, y, asset->scaleS * static_cast<float>(asset->tilesX), asset->scaleT * static_cast<float>(asset->tilesY), pavement.textureHeading);
			if (!mesh.indices.empty())
				out.meshes.push_back(std::move(mesh));
		}

		for (uint32_t i = 0; i < airport.runways.size(); ++i)
		{
			const AptRunway &runway = airport.runways[i];
			if (GetSurfaceFamily(runway.surface) == SurfaceFamily::Transparent)
				continue;
			OverlayMesh mesh;
			const PolygonAsset *asset = nullptr;
			fs::path file;
			if (!surfaceMesh(runway.surface, OverlayKind::Runway, "Runway " + runway.GetName(), i, mesh, asset, file))
				continue;
			Point2 corners[4];
			GetRunwayCorners(runway, m_Projection, corners, false);
			AppendTexturedPolygon(mesh, { { corners[0], corners[1], corners[2], corners[3] } }, runwayHeight, asset->scaleS * static_cast<float>(asset->tilesX), asset->scaleT * static_cast<float>(asset->tilesY), runway.GetHeading());
			if (!mesh.indices.empty())
				out.meshes.push_back(std::move(mesh));
		}

		for (uint32_t i = 0; i < airport.helipads.size(); ++i)
		{
			const AptHelipad &helipad = airport.helipads[i];
			if (GetSurfaceFamily(helipad.surface) == SurfaceFamily::Transparent)
				continue;
			OverlayMesh mesh;
			const PolygonAsset *asset = nullptr;
			fs::path file;
			if (!surfaceMesh(helipad.surface, OverlayKind::Helipad, "Helipad " + helipad.designator, i, mesh, asset, file))
				continue;
			Point2 corners[4];
			GetHelipadCorners(helipad, m_Projection, corners);
			AppendTexturedPolygon(mesh, { { corners[0], corners[1], corners[2], corners[3] } }, runwayHeight + 0.01f, asset->scaleS * static_cast<float>(asset->tilesX), asset->scaleT * static_cast<float>(asset->tilesY), helipad.heading);
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
									const LineAsset::Segment &segment = asset->segments[0];
									AppendTexturedRibbon(mesh, path, run.closed, lineHeight, asset->GetWidthMeters(0) + 2.0f / asset->textureWidth * asset->scaleS, segment.s1, segment.s2, asset->scaleT, asset->mirror);
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
			buildChains(airport.linearFeatures[i].chains, airport.linearFeatures[i].description.empty() ? "Line " + std::to_string(i + 1) : airport.linearFeatures[i].description, i, OverlayKind::PaintedLine);
		for (uint32_t i = 0; i < airport.pavements.size(); ++i)
			buildChains(airport.pavements[i].rings, "Pavement " + std::to_string(i + 1) + " edge", i, OverlayKind::PavementLine);
	}
}

// -------------------------------------------------------

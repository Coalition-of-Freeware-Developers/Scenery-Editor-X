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
 * xp_airport_scene.cpp
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#include "xp_airport_scene.h"
#include "xp_dsf.h"
#include "xp_library_manager.h"
#include "xp_overlay.h"
#include "xp_scene.h"
#include "xp_texture_loader.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <map>
#include <memory>
#include <set>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/renderer/vulkan/enums.h>
#include <SceneryEditorX/scene/camera.h>
#include <SceneryEditorX/scene/components/component_sets.h>
#include <SceneryEditorX/scene/components/renderable.h>
#include <SceneryEditorX/scene/entity.h>
#include <SceneryEditorX/scene/material.h>
#include <SceneryEditorX/scene/mesh.h>
#include <SceneryEditorX/scene/scene.h>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	namespace
	{
		namespace fs = std::filesystem;

		constexpr const char *LOG_TAG = "XPlaneAirport";
		constexpr float kDegToRad = 3.14159265358979323846f / 180.0f;

		/* @brief The flat colors the airport is drawn with when no library texture applies. */
		enum class Paint : uint8_t
		{
			Asphalt,
			Concrete,
			Grass,
			Dirt,
			Gravel,
			Lakebed,
			Water,
			Snow,
			Transparent,
			RunwayAsphalt,
			RunwayConcrete,
			YellowLine,
			WhiteLine,
			LightGreen,
			LightBlue,
			LightAmber,
			LightRed,
			Boundary,
			Startup,
			Sign,
			Windsock,
			Beacon,
			Fixture,
			TaxiRoute,
			TaxiNode,
			Truck,
			Jetway,
			Viewpoint,
			Count
		};

		struct PaintSpec
		{
			const char *name;
			float r, g, b, a;
			float roughness;
		};

		constexpr PaintSpec PAINTS[static_cast<size_t>(Paint::Count)] =
		{
			{ "Asphalt",			0.17f, 0.17f, 0.18f, 1.0f, 0.95f },
			{ "Concrete",			0.48f, 0.47f, 0.45f, 1.0f, 0.90f },
			{ "Grass",				0.14f, 0.30f, 0.09f, 1.0f, 1.00f },
			{ "Dirt",				0.36f, 0.26f, 0.16f, 1.0f, 1.00f },
			{ "Gravel",				0.42f, 0.40f, 0.36f, 1.0f, 1.00f },
			{ "Dry lakebed",		0.58f, 0.50f, 0.36f, 1.0f, 1.00f },
			{ "Water",				0.05f, 0.16f, 0.32f, 1.0f, 0.20f },
			{ "Snow",				0.88f, 0.90f, 0.92f, 1.0f, 0.80f },
			{ "Transparent",		0.55f, 0.55f, 0.55f, 0.30f, 1.00f },
			{ "Runway asphalt",		0.12f, 0.12f, 0.13f, 1.0f, 0.90f },
			{ "Runway concrete",	0.55f, 0.54f, 0.52f, 1.0f, 0.85f },
			{ "Yellow line",		0.92f, 0.76f, 0.10f, 1.0f, 0.70f },
			{ "White line",			0.92f, 0.92f, 0.90f, 1.0f, 0.70f },
			{ "Green lights",		0.10f, 0.95f, 0.35f, 1.0f, 0.40f },
			{ "Blue lights",		0.20f, 0.35f, 1.00f, 1.0f, 0.40f },
			{ "Amber lights",		1.00f, 0.62f, 0.10f, 1.0f, 0.40f },
			{ "Red lights",			1.00f, 0.12f, 0.10f, 1.0f, 0.40f },
			{ "Boundary",			0.95f, 0.48f, 0.10f, 1.0f, 0.80f },
			{ "Startup location",	0.10f, 0.80f, 0.95f, 1.0f, 0.60f },
			{ "Sign",				0.90f, 0.25f, 0.90f, 1.0f, 0.60f },
			{ "Windsock",			1.00f, 0.50f, 0.00f, 1.0f, 0.60f },
			{ "Beacon",				0.85f, 1.00f, 0.85f, 1.0f, 0.40f },
			{ "Light fixture",		1.00f, 0.25f, 0.20f, 1.0f, 0.50f },
			{ "Taxi route",			0.28f, 0.34f, 0.46f, 1.0f, 0.80f },
			{ "Taxi node",			0.55f, 0.65f, 0.85f, 1.0f, 0.80f },
			{ "Ground service",		0.20f, 0.72f, 0.62f, 1.0f, 0.60f },
			{ "Jetway",				0.60f, 0.32f, 0.92f, 1.0f, 0.60f },
			{ "Viewpoint",			0.95f, 0.95f, 0.95f, 1.0f, 0.60f }
		};

		/**
		 * Heights above the ground plane, so stacked flat geometry doesn't fight for the same pixels. From the bottom:
		 * boundary, DSF polygons in the terrain / beaches / shoulders groups, apt.dat pavement (first row on top),
		 * DSF "taxiways", runways, DSF "runways", painted lines, DSF "markings" and the higher groups, taxi routes,
		 * markers. Reverse-Z depth keeps millimeters apart out to a few kilometers.
		 */
		constexpr float HEIGHT_BOUNDARY = 0.00f;
		constexpr float HEIGHT_PAVEMENT = 0.03f;
		constexpr float HEIGHT_RUNWAY_ABOVE_PAVEMENT = 0.02f;
		constexpr float HEIGHT_HELIPAD_EXTRA = 0.01f;
		constexpr float HEIGHT_LINES_EXTRA = 0.03f;
		constexpr float HEIGHT_LIGHTS_EXTRA = 0.04f;
		constexpr float HEIGHT_TAXI_EXTRA = 0.13f;
		constexpr float HEIGHT_MARKERS_EXTRA = 0.16f;

		constexpr double LINE_WIDTH = 0.6;		// Painted lines without a library texture, meters
		constexpr double LIGHT_WIDTH = 0.35;	// Light strings drawn as ribbons (no library)
		constexpr double BOUNDARY_WIDTH = 1.5;
		constexpr double TAXI_WIDTH = 0.5;
		constexpr double BEZIER_SEGMENT = 2.0;	// Longest straight piece of a curve, meters

		constexpr double OVERLAY_MARGIN_OWN_PACK = 8000.0;		// Meters around the airport kept from the pack's own DSF tile
		constexpr double OVERLAY_MARGIN_SHARED_TILE = 1500.0;	// ... from a tile shared with other airports (Global Airports)
		constexpr double OVERLAY_MAX_OBJECTS = 12000;			// DSF object placements per airport
		constexpr float BLEND_SPLIT_ALPHA = 0.9f;				// Alpha at or above this stays in the deferred passes of a blended material

		/* @brief Height of a DSF draped polygon's LAYER_GROUP within the stack above. */
		float OverlayLayerHeight(const float pavementTop, const float runwayHeight, const std::string_view group, const int offset)
		{
			const float nudge = 0.0008f * static_cast<float>(std::clamp(offset, -5, 5));
			switch (GetLayerGroupOrder(group))
			{
				case 0:		return 0.008f + nudge;									// terrain
				case 1:		return 0.014f + nudge;									// beaches
				case 2:		return 0.020f + nudge;									// shoulders
				case 3:		return pavementTop + 0.005f + nudge;					// taxiways: over the apt.dat pavement
				case 4:		return runwayHeight + 0.005f + nudge;					// runways
				case 5:		return runwayHeight + HEIGHT_LINES_EXTRA + 0.005f + nudge;	// markings: over the apt.dat lines
				case 6:		return runwayHeight + 0.050f + nudge;					// airports
				case 7:		return runwayHeight + 0.065f + nudge;					// roads
				case 8:		return runwayHeight + 0.080f + nudge;					// objects
				case 9:		return runwayHeight + 0.095f + nudge;					// light_objects
				default:	return runwayHeight + 0.110f + nudge;					// cars
			}
		}

		/* @brief One entity's worth of geometry. */
		struct Part
		{
			AirportElementKind kind = AirportElementKind::Pavement;
			uint32_t index = 0;
			uint32_t subIndex = 0;
			std::string name;
			Paint paint = Paint::Asphalt;

			// Library / DSF art: the material comes from the texture and flags instead of the paint
			bool textured = false;
			fs::path texture;						// Empty: drawn with color
			float color[4] = { 0.6f, 0.6f, 0.6f, 1.0f };
			bool cutout = false;
			float cutoff = 0.5f;
			bool blended = false;

			std::vector<Vertex_PosTexNorTan> vertices;
			std::vector<uint32_t> indices;
			Bounds2 bounds;					// Airport-space footprint (for placed parts: around the origin)
			uint32_t subMesh = 0;

			// Point features: geometry built at the origin pointing north; the entity transform places it
			bool placed = false;
			Point2 origin;
			double heading = 0.0;
		};

		struct Build
		{
			GeoProjection projection;
			std::vector<Part> parts;
			Bounds2 bounds;
			float pavementTop = HEIGHT_PAVEMENT;
			float runwayHeight = HEIGHT_PAVEMENT + HEIGHT_RUNWAY_ABOVE_PAVEMENT;
			std::vector<OverlayObject> objects;		// OBJ8 placements: DSF objects, strings, facade attachments, apt.dat light strings
			std::vector<std::string> warnings;
			uint32_t overlayMeshes = 0;
			uint32_t overlayCulled = 0;
			uint32_t overlayUnsupported = 0;
		};

		// -------------------------------------------------------

		Paint SurfacePaint(const int surface, const bool runway)
		{
			switch (GetSurfaceFamily(surface))
			{
				case SurfaceFamily::Asphalt:		return runway ? Paint::RunwayAsphalt : Paint::Asphalt;
				case SurfaceFamily::Concrete:		return runway ? Paint::RunwayConcrete : Paint::Concrete;
				case SurfaceFamily::Grass:			return Paint::Grass;
				case SurfaceFamily::Dirt:			return Paint::Dirt;
				case SurfaceFamily::Gravel:			return Paint::Gravel;
				case SurfaceFamily::Lakebed:		return Paint::Lakebed;
				case SurfaceFamily::Water:			return Paint::Water;
				case SurfaceFamily::Snow:			return Paint::Snow;
				case SurfaceFamily::Transparent:	return Paint::Transparent;
				default:							return runway ? Paint::RunwayAsphalt : Paint::Asphalt;
			}
		}

		Paint LinePaint(const int lineType)
		{
			// 20..22 are the white runway-style markings; everything else painted is yellow.
			return lineType >= 20 && lineType <= 22 ? Paint::WhiteLine : Paint::YellowLine;
		}

		Paint LightPaint(const int lightType)
		{
			switch (lightType)
			{
				case 101: case 107: case 108:	return Paint::LightGreen;
				case 102:						return Paint::LightBlue;
				case 103: case 104: case 105:	return Paint::LightAmber;
				case 106:						return Paint::LightRed;
				default:						return Paint::LightBlue;
			}
		}

		Vertex_PosTexNorTan MakeVertex(const Point2 &point, const float y)
		{
			return Vertex_PosTexNorTan(Vec3(static_cast<float>(point.x), y, static_cast<float>(point.z)),
				Vec2(static_cast<float>(point.x * 0.1), static_cast<float>(point.z * 0.1)), Vec3(0.0f, 1.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f));
		}

		/* @brief Appends indexed triangles (already wound counter-clockwise from above). */
		void EmitTriangles(Part &part, const std::vector<Point2> &points, const std::vector<uint32_t> &indices, const float y)
		{
			const uint32_t base = static_cast<uint32_t>(part.vertices.size());
			part.vertices.reserve(part.vertices.size() + points.size());
			for (const Point2 &point : points)
			{
				part.vertices.push_back(MakeVertex(point, y));
				part.bounds.Add(point);
			}
			part.indices.reserve(part.indices.size() + indices.size());
			for (const uint32_t index : indices)
				part.indices.push_back(base + index);
		}

		/* @brief Appends a convex quad given its corners in order. */
		void EmitQuad(Part &part, const Point2 corners[4], const float y)
		{
			std::vector<Point2> points(corners, corners + 4);
			std::vector<uint32_t> indices = { 0, 1, 2, 0, 2, 3 };
			for (size_t i = 0; i < indices.size(); i += 3)
			{
				const Point2 &a = points[indices[i]];
				const Point2 &b = points[indices[i + 1]];
				const Point2 &c = points[indices[i + 2]];
				if ((b.z - a.z) * (c.x - a.x) - (b.x - a.x) * (c.z - a.z) < 0.0)
					std::swap(indices[i + 1], indices[i + 2]);
			}
			EmitTriangles(part, points, indices, y);
		}

		/* @brief A marker at a point: an arrow head along the heading (arrow) or a square turned to it. */
		void EmitMarker(Part &part, const Point2 &center, const double headingDegrees, const double size, const float y, const bool arrow)
		{
			const Point2 forward = HeadingToDirection(headingDegrees);
			const Point2 right = { -forward.z, forward.x };
			if (arrow)
			{
				std::vector<Point2> points =
				{
					{ center.x + forward.x * size * 0.65, center.z + forward.z * size * 0.65 },
					{ center.x - forward.x * size * 0.35 - right.x * size * 0.45, center.z - forward.z * size * 0.35 - right.z * size * 0.45 },
					{ center.x - forward.x * size * 0.15, center.z - forward.z * size * 0.15 },
					{ center.x - forward.x * size * 0.35 + right.x * size * 0.45, center.z - forward.z * size * 0.35 + right.z * size * 0.45 }
				};
				std::vector<uint32_t> indices = { 0, 1, 2, 0, 2, 3 };
				for (size_t i = 0; i < indices.size(); i += 3)
				{
					const Point2 &a = points[indices[i]];
					const Point2 &b = points[indices[i + 1]];
					const Point2 &c = points[indices[i + 2]];
					if ((b.z - a.z) * (c.x - a.x) - (b.x - a.x) * (c.z - a.z) < 0.0)
						std::swap(indices[i + 1], indices[i + 2]);
				}
				EmitTriangles(part, points, indices, y);
			}
			else
			{
				const double half = size * 0.5;
				const Point2 corners[4] =
				{
					{ center.x - forward.x * half - right.x * half, center.z - forward.z * half - right.z * half },
					{ center.x - forward.x * half + right.x * half, center.z - forward.z * half + right.z * half },
					{ center.x + forward.x * half + right.x * half, center.z + forward.z * half + right.z * half },
					{ center.x + forward.x * half - right.x * half, center.z + forward.z * half - right.z * half }
				};
				EmitQuad(part, corners, y);
			}
		}

		/**
		 * @brief Builds a marker at the origin and records where its entity goes, so moving the feature later only moves
		 * the entity. The footprint is kept in airport space for framing.
		 */
		void PlaceMarker(Part &part, const Point2 &origin, const double heading, const double size, const float y, const bool arrow)
		{
			part.placed = true;
			part.origin = origin;
			part.heading = heading;
			EmitMarker(part, Point2{ 0.0, 0.0 }, 0.0, size, y, arrow);
			part.bounds = Bounds2();
			part.bounds.Add(Point2{ origin.x - size, origin.z - size });
			part.bounds.Add(Point2{ origin.x + size, origin.z + size });
		}

		/* @brief Copies an overlay mesh's material (texture and flags) onto a part. */
		void ApplyOverlayMaterial(Part &part, const OverlayMesh &mesh)
		{
			part.textured = true;
			part.texture = mesh.texture;
			std::copy(std::begin(mesh.color), std::end(mesh.color), std::begin(part.color));
			part.cutout = mesh.cutout;
			part.cutoff = mesh.cutoff;
			part.blended = mesh.blended;
		}

		/* @brief Appends an overlay mesh's textured geometry (positions, UVs and normals as built). */
		void AppendOverlayMesh(Part &part, const OverlayMesh &mesh)
		{
			const uint32_t base = static_cast<uint32_t>(part.vertices.size());
			part.vertices.reserve(part.vertices.size() + mesh.vertices.size());
			for (const OverlayVertex &vertex : mesh.vertices)
			{
				const Vec3 normal(vertex.nx, vertex.ny, vertex.nz);
				// A tangent perpendicular to the normal: east for ground, along the ground for walls
				const Vec3 tangent = std::abs(vertex.ny) > 0.9f ? Vec3(1.0f, 0.0f, 0.0f) : xMath::Normalize(xMath::Cross(Vec3(0.0f, 1.0f, 0.0f), normal));
				part.vertices.push_back(Vertex_PosTexNorTan(Vec3(vertex.x, vertex.y, vertex.z), Vec2(vertex.u, vertex.v), normal, tangent));
			}
			part.indices.reserve(part.indices.size() + mesh.indices.size());
			for (const uint32_t index : mesh.indices)
				part.indices.push_back(base + index);
			part.bounds.Add(mesh.bounds);
		}

		/* @brief Where a point feature sits and points, in airport space. False when kind / index name no such feature. */
		bool GetPlacement(const AptAirport &airport, const GeoProjection &projection, const AirportElementKind kind, const uint32_t index, Point2 &origin, double &heading)
		{
			const auto at = [&projection](const double latitude, const double longitude) { return projection.ToLocal(latitude, longitude); };
			switch (kind)
			{
				case AirportElementKind::Viewpoint:
					if (!airport.viewpoint) return false;
					origin = at(airport.viewpoint->latitude, airport.viewpoint->longitude);
					heading = 0.0;
					return true;
				case AirportElementKind::Beacon:
					if (!airport.beacon) return false;
					origin = at(airport.beacon->latitude, airport.beacon->longitude);
					heading = 45.0;
					return true;
				case AirportElementKind::Windsock:
					if (index >= airport.windsocks.size()) return false;
					origin = at(airport.windsocks[index].latitude, airport.windsocks[index].longitude);
					heading = 0.0;
					return true;
				case AirportElementKind::Sign:
					if (index >= airport.signs.size()) return false;
					origin = at(airport.signs[index].latitude, airport.signs[index].longitude);
					heading = airport.signs[index].heading;
					return true;
				case AirportElementKind::LightFixture:
					if (index >= airport.lightFixtures.size()) return false;
					origin = at(airport.lightFixtures[index].latitude, airport.lightFixtures[index].longitude);
					heading = airport.lightFixtures[index].heading;
					return true;
				case AirportElementKind::Startup:
					if (index >= airport.startups.size()) return false;
					origin = at(airport.startups[index].latitude, airport.startups[index].longitude);
					heading = airport.startups[index].heading;
					return true;
				case AirportElementKind::LegacyStartup:
					if (index >= airport.legacyStartups.size()) return false;
					origin = at(airport.legacyStartups[index].latitude, airport.legacyStartups[index].longitude);
					heading = airport.legacyStartups[index].heading;
					return true;
				case AirportElementKind::TruckParking:
					if (index >= airport.truckParkings.size()) return false;
					origin = at(airport.truckParkings[index].latitude, airport.truckParkings[index].longitude);
					heading = airport.truckParkings[index].heading;
					return true;
				case AirportElementKind::TruckDestination:
					if (index >= airport.truckDestinations.size()) return false;
					origin = at(airport.truckDestinations[index].latitude, airport.truckDestinations[index].longitude);
					heading = airport.truckDestinations[index].heading;
					return true;
				case AirportElementKind::Jetway:
					if (index >= airport.jetways.size()) return false;
					origin = at(airport.jetways[index].latitude, airport.jetways[index].longitude);
					heading = airport.jetways[index].heading;
					return true;
				default:
					return false;
			}
		}

		/* @brief Appends a colored ribbon along a chain, colored by its dominant style (the fallback without library art). */
		void EmitChainRibbons(std::vector<Part> &parts, const Part &prototype, const AptNodeChain &chain, const GeoProjection &projection, const bool lights, const float y)
		{
			int dominant = 0;
			std::array<int, 256> votes{};
			for (const AptNode &node : chain.nodes)
			{
				const int style = lights ? node.lightType : node.lineType;
				if (style > 0 && style < 256 && ++votes[static_cast<size_t>(style)] > votes[static_cast<size_t>(dominant)])
					dominant = style;
			}
			if (dominant == 0)
				return;

			std::vector<Point2> path;
			FlattenChain(chain, projection, path, BEZIER_SEGMENT);
			if (path.size() < 2)
				return;

			std::vector<Point2> points;
			std::vector<uint32_t> indices;
			BuildPolylineStrip(path, chain.closed, lights ? LIGHT_WIDTH : LINE_WIDTH, points, indices);

			Part &part = parts.emplace_back(prototype);
			part.paint = lights ? LightPaint(dominant) : LinePaint(dominant);
			part.subIndex = lights ? 1 : 0;
			EmitTriangles(part, points, indices, y);
		}

		std::string Describe(const std::string &what, const size_t index, const std::string &description)
		{
			std::string name = what + " " + std::to_string(index + 1);
			if (!description.empty())
				name += ": " + description;
			return name;
		}

		// -------------------------------------------------------

		/**
		 * @brief Finds the files behind asset paths: library virtual paths through the X-Plane library index, anything
		 * else relative to the scenery pack. A path ending in "_" is a prefix ("lib/airport/lines/4_"): the first
		 * library export starting with it is taken, which is how apt.dat line and light codes find their art.
		 */
		AssetResolver MakeResolver(const fs::path &package)
		{
			return [package](const std::string_view path) -> fs::path
			{
				const std::string key(path);
				if (key.empty())
					return {};
				std::error_code ec;

				if (key.back() == '_')
				{
					const std::shared_ptr<const LibraryIndex> index = LibraryManager::GetIndex();
					if (!index)
						return {};
					const std::vector<LibraryAsset> &assets = index->GetAssets();
					auto it = std::lower_bound(assets.begin(), assets.end(), key, [](const LibraryAsset &asset, const std::string &k) { return asset.virtualPath < k; });
					for (; it != assets.end() && it->virtualPath.starts_with(key); ++it)
					{
						if (fs::path file = index->Resolve(it->virtualPath); !file.empty())
							return file;
					}
					return {};
				}

				// Any virtual path can be a library export, not just "lib/...": third-party libraries export their own
				// roots (ZDP_Library/..., MisterX_Library/...). Like X-Plane, an export wins over a file in the pack.
				if (!fs::path(key).is_absolute())
				{
					if (fs::path file = LibraryManager::Resolve(key); !file.empty())
						return file;
				}

				const fs::path direct(key);
				if (direct.is_absolute())
					return fs::exists(direct, ec) ? direct : fs::path();
				if (!package.empty())
				{
					const fs::path local = (package / direct).lexically_normal();
					if (fs::exists(local, ec))
						return local;
				}
				return {};
			};
		}

		OverlayBuilder::Options MakeOverlayOptions(const Build &build, const double margin)
		{
			OverlayBuilder::Options options;
			options.area = build.bounds;
			options.areaMargin = margin;
			options.maxObjects = OVERLAY_MAX_OBJECTS;
			options.bezierSegment = BEZIER_SEGMENT;
			const float pavementTop = build.pavementTop, runwayHeight = build.runwayHeight;
			options.layerHeight = [pavementTop, runwayHeight](const std::string_view group, const int offset) { return OverlayLayerHeight(pavementTop, runwayHeight, group, offset); };
			return options;
		}

		// -------------------------------------------------------

		/**
		 * @brief Builds the apt.dat geometry. With a library available, pavement, runways and painted lines use the
		 * library's .pol / .lin art (the same WorldEditor shows) and light strings become the library's light objects;
		 * without one everything is drawn in flat colors.
		 */
		void BuildAirport(const AptAirport &airport, Build &out, OverlayBuilder *overlay)
		{
			double datumLat = 0.0, datumLon = 0.0;
			if (!airport.GetDatum(datumLat, datumLon))
			{
				out.projection = GeoProjection();
				return;
			}
			out.projection = GeoProjection(datumLat, datumLon);
			const GeoProjection &projection = out.projection;
			std::vector<Part> &parts = out.parts;

			const auto start_part = [&parts](const AirportElementKind kind, const uint32_t index, std::string name, const Paint paint) -> Part &
			{
				Part &part = parts.emplace_back();
				part.kind = kind;
				part.index = index;
				part.name = std::move(name);
				part.paint = paint;
				return part;
			};

			// Pavement: first in the file draws on top, so later polygons sit lower.
			const size_t pavementCount = airport.pavements.size();
			const float pavementStep = pavementCount > 1 ? std::min(0.003f, 0.3f / static_cast<float>(pavementCount)) : 0.0f;
			const float pavementTop = HEIGHT_PAVEMENT + pavementStep * static_cast<float>(pavementCount);
			const float runwayHeight = pavementTop + HEIGHT_RUNWAY_ABOVE_PAVEMENT;
			const float lineHeight = runwayHeight + HEIGHT_LINES_EXTRA;
			out.pavementTop = pavementTop;
			out.runwayHeight = runwayHeight;
			std::vector<float> pavementHeights(pavementCount);
			for (size_t i = 0; i < pavementCount; ++i)
				pavementHeights[i] = pavementTop - pavementStep * static_cast<float>(i + 1);

			// Library art for the apt.dat rows, keyed by what it draws
			OverlayBuild surfaces;
			std::map<std::pair<int, uint32_t>, std::vector<const OverlayMesh *>> textured;
			bool lightsAsObjects = false;
			if (overlay)
			{
				OverlayBuilder::Options options = MakeOverlayOptions(out, 0.0);
				options.area = Bounds2();	// Everything of the airport
				overlay->BuildAirportSurfaces(airport, pavementHeights, runwayHeight, lineHeight, options, surfaces);
				for (const OverlayMesh &mesh : surfaces.meshes)
					textured[{ static_cast<int>(mesh.kind), mesh.placement }].push_back(&mesh);
				lightsAsObjects = !surfaces.objects.empty() || overlay->GetString(OverlayBuilder::GetLightStringLibraryPath(101), nullptr, surfaces) != nullptr;
				for (OverlayObject &object : surfaces.objects)
					out.objects.push_back(std::move(object));
				out.warnings.insert(out.warnings.end(), surfaces.warnings.begin(), surfaces.warnings.end());
			}
			const auto texturedMeshes = [&textured](const OverlayKind kind, const uint32_t index) -> const std::vector<const OverlayMesh *> *
			{
				const auto it = textured.find({ static_cast<int>(kind), index });
				return it != textured.end() ? &it->second : nullptr;
			};

			for (size_t i = 0; i < pavementCount; ++i)
			{
				const AptPavement &pavement = airport.pavements[i];
				Part &part = start_part(AirportElementKind::Pavement, static_cast<uint32_t>(i), Describe("Pavement", i, pavement.description), SurfacePaint(pavement.surface, false));

				if (const auto *meshes = texturedMeshes(OverlayKind::Pavement, static_cast<uint32_t>(i)))
				{
					ApplyOverlayMaterial(part, *meshes->front());
					for (const OverlayMesh *mesh : *meshes)
						AppendOverlayMesh(part, *mesh);
				}
				else
				{
					std::vector<std::vector<Point2>> rings;
					for (const AptNodeChain &ring : pavement.rings)
					{
						if (!ring.closed || ring.nodes.size() < 3)
							continue;
						rings.emplace_back();
						FlattenChain(ring, projection, rings.back(), BEZIER_SEGMENT);
						if (rings.back().size() < 3)
							rings.pop_back();
					}
					std::vector<Point2> points;
					std::vector<uint32_t> indices;
					if (!rings.empty() && TriangulatePolygon(rings, points, indices))
						EmitTriangles(part, points, indices, pavementHeights[i]);
					else if (!rings.empty())
					{
						// Degenerate outline: keep its footprint so the entity can still be framed and selected.
						for (const Point2 &point : rings[0])
							part.bounds.Add(point);
					}
				}

				// Lines painted along the pavement edges, and their lights
				const std::string partName = part.name;
				if (const auto *lines = texturedMeshes(OverlayKind::PavementLine, static_cast<uint32_t>(i)))
				{
					for (const OverlayMesh *mesh : *lines)
					{
						Part &edge = start_part(AirportElementKind::Pavement, static_cast<uint32_t>(i), partName + " edge: " + mesh->name.substr(mesh->name.find(": ") == std::string::npos ? 0 : mesh->name.find(": ") + 2), Paint::YellowLine);
						ApplyOverlayMaterial(edge, *mesh);
						AppendOverlayMesh(edge, *mesh);
					}
				}
				else
				{
					for (const AptNodeChain &ring : pavement.rings)
					{
						Part prototype;
						prototype.kind = AirportElementKind::Pavement;
						prototype.index = static_cast<uint32_t>(i);
						prototype.name = partName + " edge";
						EmitChainRibbons(parts, prototype, ring, projection, false, lineHeight);
					}
				}
				if (!lightsAsObjects)
				{
					for (const AptNodeChain &ring : pavement.rings)
					{
						Part prototype;
						prototype.kind = AirportElementKind::Pavement;
						prototype.index = static_cast<uint32_t>(i);
						prototype.name = partName + " edge lights";
						EmitChainRibbons(parts, prototype, ring, projection, true, runwayHeight + HEIGHT_LIGHTS_EXTRA);
					}
				}
			}

			for (size_t i = 0; i < airport.runways.size(); ++i)
			{
				const AptRunway &runway = airport.runways[i];
				Part &part = start_part(AirportElementKind::Runway, static_cast<uint32_t>(i), "Runway " + runway.GetName(), SurfacePaint(runway.surface, true));
				if (const auto *meshes = texturedMeshes(OverlayKind::Runway, static_cast<uint32_t>(i)))
				{
					ApplyOverlayMaterial(part, *meshes->front());
					for (const OverlayMesh *mesh : *meshes)
						AppendOverlayMesh(part, *mesh);
				}
				else
				{
					Point2 corners[4];
					GetRunwayCorners(runway, projection, corners, false);
					EmitQuad(part, corners, runwayHeight);
				}

				// Blast pads, a shade lower so the runway proper reads on top
				if (runway.ends[0].blastPad > 0.0f || runway.ends[1].blastPad > 0.0f)
				{
					Point2 padded[4];
					GetRunwayCorners(runway, projection, padded, true);
					Part &pads = start_part(AirportElementKind::Runway, static_cast<uint32_t>(i), "Runway " + runway.GetName() + " blast pads", SurfacePaint(runway.surface, false));
					pads.subIndex = 1;
					EmitQuad(pads, padded, runwayHeight - 0.005f);
				}
			}

			for (size_t i = 0; i < airport.waterRunways.size(); ++i)
			{
				const AptWaterRunway &runway = airport.waterRunways[i];
				Part &part = start_part(AirportElementKind::WaterRunway, static_cast<uint32_t>(i), "Water runway " + runway.GetName(), Paint::Water);
				Point2 corners[4];
				GetWaterRunwayCorners(runway, projection, corners);
				EmitQuad(part, corners, HEIGHT_BOUNDARY + 0.005f);
			}

			for (size_t i = 0; i < airport.helipads.size(); ++i)
			{
				const AptHelipad &helipad = airport.helipads[i];
				Part &part = start_part(AirportElementKind::Helipad, static_cast<uint32_t>(i), "Helipad " + helipad.designator, SurfacePaint(helipad.surface, true));
				if (const auto *meshes = texturedMeshes(OverlayKind::Helipad, static_cast<uint32_t>(i)))
				{
					ApplyOverlayMaterial(part, *meshes->front());
					for (const OverlayMesh *mesh : *meshes)
						AppendOverlayMesh(part, *mesh);
				}
				else
				{
					Point2 corners[4];
					GetHelipadCorners(helipad, projection, corners);
					EmitQuad(part, corners, runwayHeight + HEIGHT_HELIPAD_EXTRA);
				}
			}

			for (size_t i = 0; i < airport.linearFeatures.size(); ++i)
			{
				const AptLinearFeature &feature = airport.linearFeatures[i];
				Part prototype;
				prototype.kind = AirportElementKind::LinearFeature;
				prototype.index = static_cast<uint32_t>(i);
				prototype.name = Describe("Line", i, feature.description);
				const size_t before = parts.size();

				if (const auto *lines = texturedMeshes(OverlayKind::PaintedLine, static_cast<uint32_t>(i)))
				{
					for (const OverlayMesh *mesh : *lines)
					{
						Part &part = start_part(AirportElementKind::LinearFeature, static_cast<uint32_t>(i), mesh->name, Paint::YellowLine);
						ApplyOverlayMaterial(part, *mesh);
						AppendOverlayMesh(part, *mesh);
					}
				}
				else
				{
					for (const AptNodeChain &chain : feature.chains)
						EmitChainRibbons(parts, prototype, chain, projection, false, lineHeight);
				}
				if (!lightsAsObjects)
				{
					for (const AptNodeChain &chain : feature.chains)
					{
						Part lightsPrototype = prototype;
						lightsPrototype.name += " lights";
						EmitChainRibbons(parts, lightsPrototype, chain, projection, true, runwayHeight + HEIGHT_LIGHTS_EXTRA);
					}
				}
				if (parts.size() == before)
				{
					// A feature with no drawn style (lights only as objects, or no style at all): keep a framable entity for it.
					Part &part = start_part(AirportElementKind::LinearFeature, static_cast<uint32_t>(i), prototype.name, Paint::YellowLine);
					for (const AptNodeChain &chain : feature.chains)
					{
						std::vector<Point2> path;
						FlattenChain(chain, projection, path, BEZIER_SEGMENT);
						for (const Point2 &point : path)
							part.bounds.Add(point);
					}
				}
			}

			for (size_t i = 0; i < airport.boundaries.size(); ++i)
			{
				const AptBoundary &boundary = airport.boundaries[i];
				Part &part = start_part(AirportElementKind::Boundary, static_cast<uint32_t>(i), airport.boundaries.size() > 1 ? Describe("Boundary", i, boundary.description) : std::string("Boundary"), Paint::Boundary);
				for (const AptNodeChain &ring : boundary.rings)
				{
					std::vector<Point2> path, points;
					std::vector<uint32_t> indices;
					FlattenChain(ring, projection, path, BEZIER_SEGMENT);
					BuildPolylineStrip(path, ring.closed, BOUNDARY_WIDTH, points, indices);
					EmitTriangles(part, points, indices, HEIGHT_BOUNDARY);
				}
			}

			const float markerHeight = runwayHeight + HEIGHT_MARKERS_EXTRA;
			Point2 origin;
			double heading = 0.0;
			if (airport.viewpoint && GetPlacement(airport, projection, AirportElementKind::Viewpoint, 0, origin, heading))
			{
				Part &part = start_part(AirportElementKind::Viewpoint, 0, "Viewpoint: " + airport.viewpoint->name, Paint::Viewpoint);
				PlaceMarker(part, origin, heading, 3.0, markerHeight, false);
			}
			if (airport.beacon && airport.beacon->type != 0 && GetPlacement(airport, projection, AirportElementKind::Beacon, 0, origin, heading))
			{
				Part &part = start_part(AirportElementKind::Beacon, 0, airport.beacon->name.empty() ? std::string("Beacon") : "Beacon: " + airport.beacon->name, Paint::Beacon);
				PlaceMarker(part, origin, heading, 2.5, markerHeight, false);
			}
			for (size_t i = 0; i < airport.windsocks.size(); ++i)
			{
				const AptWindsock &windsock = airport.windsocks[i];
				Part &part = start_part(AirportElementKind::Windsock, static_cast<uint32_t>(i), windsock.name.empty() ? Describe("Windsock", i, "") : "Windsock " + windsock.name, Paint::Windsock);
				GetPlacement(airport, projection, AirportElementKind::Windsock, static_cast<uint32_t>(i), origin, heading);
				PlaceMarker(part, origin, heading, 2.0, markerHeight, false);
			}
			for (size_t i = 0; i < airport.signs.size(); ++i)
			{
				const AptSign &sign = airport.signs[i];
				Part &part = start_part(AirportElementKind::Sign, static_cast<uint32_t>(i), "Sign " + sign.text, Paint::Sign);
				GetPlacement(airport, projection, AirportElementKind::Sign, static_cast<uint32_t>(i), origin, heading);
				PlaceMarker(part, origin, heading, 1.5 + 0.5 * std::clamp(sign.size, 1, 5), markerHeight, true);
			}
			for (size_t i = 0; i < airport.lightFixtures.size(); ++i)
			{
				const AptLightFixture &fixture = airport.lightFixtures[i];
				std::string name = GetLightFixtureName(fixture.type);
				if (!fixture.name.empty())
					name += ": " + fixture.name;
				Part &part = start_part(AirportElementKind::LightFixture, static_cast<uint32_t>(i), std::move(name), Paint::Fixture);
				GetPlacement(airport, projection, AirportElementKind::LightFixture, static_cast<uint32_t>(i), origin, heading);
				PlaceMarker(part, origin, heading, 3.0, markerHeight, true);
			}
			for (size_t i = 0; i < airport.startups.size(); ++i)
			{
				const AptStartup &startup = airport.startups[i];
				std::string name = GetStartupTypeName(startup.type);
				if (!startup.name.empty())
					name += " " + startup.name;
				Part &part = start_part(AirportElementKind::Startup, static_cast<uint32_t>(i), std::move(name), Paint::Startup);
				// Marker size follows the design group (A..F) when known
				double size = 6.0;
				if (startup.hasMetadata && startup.widthCode.size() == 1 && startup.widthCode[0] >= 'A' && startup.widthCode[0] <= 'F')
					size = 4.0 + 2.0 * (startup.widthCode[0] - 'A');
				GetPlacement(airport, projection, AirportElementKind::Startup, static_cast<uint32_t>(i), origin, heading);
				PlaceMarker(part, origin, heading, size, markerHeight, true);
			}
			for (size_t i = 0; i < airport.legacyStartups.size(); ++i)
			{
				const AptLegacyStartup &startup = airport.legacyStartups[i];
				Part &part = start_part(AirportElementKind::LegacyStartup, static_cast<uint32_t>(i), "Startup (legacy) " + startup.name, Paint::Startup);
				GetPlacement(airport, projection, AirportElementKind::LegacyStartup, static_cast<uint32_t>(i), origin, heading);
				PlaceMarker(part, origin, heading, 5.0, markerHeight, true);
			}

			if (airport.taxiNetwork && !airport.taxiNetwork->edges.empty())
			{
				const AptTaxiNetwork &network = *airport.taxiNetwork;
				std::vector<Point2> nodePositions(network.nodes.size());
				std::vector<int> idToIndex;
				int maxId = -1;
				for (const AptTaxiNode &node : network.nodes)
					maxId = std::max(maxId, node.id);
				idToIndex.assign(static_cast<size_t>(maxId + 1), -1);
				for (size_t i = 0; i < network.nodes.size(); ++i)
				{
					nodePositions[i] = projection.ToLocal(network.nodes[i].latitude, network.nodes[i].longitude);
					if (network.nodes[i].id >= 0)
						idToIndex[static_cast<size_t>(network.nodes[i].id)] = static_cast<int>(i);
				}

				Part &routes = start_part(AirportElementKind::TaxiRoutes, 0, "Taxi routes (" + std::to_string(network.edges.size()) + ")", Paint::TaxiRoute);
				for (const AptTaxiEdge &edge : network.edges)
				{
					if (edge.from < 0 || edge.to < 0 || static_cast<size_t>(edge.from) >= idToIndex.size() || static_cast<size_t>(edge.to) >= idToIndex.size())
						continue;
					const int a = idToIndex[static_cast<size_t>(edge.from)];
					const int b = idToIndex[static_cast<size_t>(edge.to)];
					if (a < 0 || b < 0)
						continue;
					std::vector<Point2> path = { nodePositions[static_cast<size_t>(a)], nodePositions[static_cast<size_t>(b)] };
					std::vector<Point2> points;
					std::vector<uint32_t> indices;
					BuildPolylineStrip(path, false, edge.groundVehicles ? TAXI_WIDTH * 0.6 : TAXI_WIDTH, points, indices);
					EmitTriangles(routes, points, indices, runwayHeight + HEIGHT_TAXI_EXTRA);
				}

				Part &nodes = start_part(AirportElementKind::TaxiNodes, 0, "Taxi nodes (" + std::to_string(network.nodes.size()) + ")", Paint::TaxiNode);
				for (const Point2 &position : nodePositions)
					EmitMarker(nodes, position, 45.0, 1.2, runwayHeight + HEIGHT_TAXI_EXTRA + 0.005f, false);
			}

			for (size_t i = 0; i < airport.truckParkings.size(); ++i)
			{
				const AptTruckParking &truck = airport.truckParkings[i];
				std::string name = GetTruckTypeName(truck.type);
				if (!truck.name.empty())
					name += ": " + truck.name;
				Part &part = start_part(AirportElementKind::TruckParking, static_cast<uint32_t>(i), std::move(name), Paint::Truck);
				GetPlacement(airport, projection, AirportElementKind::TruckParking, static_cast<uint32_t>(i), origin, heading);
				PlaceMarker(part, origin, heading, 3.0, markerHeight, true);
			}
			for (size_t i = 0; i < airport.truckDestinations.size(); ++i)
			{
				const AptTruckDestination &destination = airport.truckDestinations[i];
				Part &part = start_part(AirportElementKind::TruckDestination, static_cast<uint32_t>(i), destination.name.empty() ? Describe("Truck destination", i, destination.truckTypes) : "Truck destination: " + destination.name, Paint::Truck);
				GetPlacement(airport, projection, AirportElementKind::TruckDestination, static_cast<uint32_t>(i), origin, heading);
				PlaceMarker(part, origin, heading, 2.5, markerHeight, false);
			}
			for (size_t i = 0; i < airport.jetways.size(); ++i)
			{
				const AptJetway &jetway = airport.jetways[i];
				Part &part = start_part(AirportElementKind::Jetway, static_cast<uint32_t>(i), Describe("Jetway", i, ""), Paint::Jetway);
				GetPlacement(airport, projection, AirportElementKind::Jetway, static_cast<uint32_t>(i), origin, heading);

				// Built pointing north from the origin: the parked tunnel as a ribbon, the cabin turned relative to it.
				const double length = std::max(5.0, static_cast<double>(jetway.tunnelLength));
				part.placed = true;
				part.origin = origin;
				part.heading = heading;
				std::vector<Point2> path = { { 0.0, 0.0 }, { 0.0, -length } };
				std::vector<Point2> points;
				std::vector<uint32_t> indices;
				BuildPolylineStrip(path, false, 3.0, points, indices);
				EmitTriangles(part, points, indices, markerHeight);
				EmitMarker(part, path[1], static_cast<double>(jetway.cabinHeading) - heading, 4.0, markerHeight + 0.005f, true);
				part.bounds = Bounds2();
				part.bounds.Add(Point2{ origin.x - length, origin.z - length });
				part.bounds.Add(Point2{ origin.x + length, origin.z + length });
			}

			out.bounds = Bounds2();
			for (const Part &part : parts)
				out.bounds.Add(part.bounds);
		}

		/**
		 * @brief Adds the scenery pack's DSF overlay around the airport: draped polygons, lines, forests and facades as
		 * textured parts (merged per asset), objects and object strings as placements for the OBJ8 loader.
		 */
		void BuildOverlay(const AirportDocument &document, Build &out, OverlayBuilder &overlay)
		{
			const fs::path &package = document.GetPackageFolder();
			if (package.empty() || !out.bounds.valid)
				return;

			// Tiles under the airport (with a little slack for airports on a tile edge)
			double south = 0.0, west = 0.0, north = 0.0, east = 0.0;
			if (!document.airport.GetBounds(south, west, north, east))
			{
				if (!document.airport.GetDatum(south, west))
					return;
				north = south;
				east = west;
			}
			constexpr double slack = 0.02;
			std::set<std::pair<int, int>> tiles;
			for (int lat = static_cast<int>(std::floor(south - slack)); lat <= static_cast<int>(std::floor(north + slack)); ++lat)
			{
				for (int lon = static_cast<int>(std::floor(west - slack)); lon <= static_cast<int>(std::floor(east + slack)); ++lon)
					tiles.insert({ lat, lon });
			}

			// A tile of a shared pack (Global Airports) holds every airport of a 1x1 degree cell: keep what's near this one
			const bool shared = IsProtectedPackage(package, LibraryManager::GetInstallPath());
			const OverlayBuilder::Options options = MakeOverlayOptions(out, shared ? OVERLAY_MARGIN_SHARED_TILE : OVERLAY_MARGIN_OWN_PACK);

			OverlayBuild overlayBuild;
			for (const auto &[lat, lon] : tiles)
			{
				const fs::path tile = GetDsfTilePath(package, lat + 0.5, lon + 0.5);
				std::error_code ec;
				if (!fs::exists(tile, ec))
					continue;

				const auto start = std::chrono::steady_clock::now();
				DsfFile dsf;
				std::string error;
				if (!LoadDsf(tile, dsf, &error))
				{
					out.warnings.push_back("DSF " + tile.filename().string() + ": " + error);
					continue;
				}
				const size_t meshesBefore = overlayBuild.meshes.size(), objectsBefore = overlayBuild.objects.size();
				overlay.Build(dsf, options, overlayBuild);
				SEDX_CORE_INFO_TAG(LOG_TAG, "{}: {} objects, {} polygons in the tile; kept {} meshes and {} objects near the airport in {:.0f} ms", tile.filename().string(),
					dsf.objects.size(), dsf.polygons.size(), overlayBuild.meshes.size() - meshesBefore, overlayBuild.objects.size() - objectsBefore,
					std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count());
			}

			// Meshes of the same asset share an entity: one draw per texture, and the outliner stays readable
			std::map<std::string, size_t> merged;
			for (const OverlayMesh &mesh : overlayBuild.meshes)
			{
				AirportElementKind kind = AirportElementKind::DrapedPolygon;
				switch (mesh.kind)
				{
					case OverlayKind::Line:		kind = AirportElementKind::LineString; break;
					case OverlayKind::Forest:	kind = AirportElementKind::Forest; break;
					case OverlayKind::Facade:	kind = AirportElementKind::Facade; break;
					default:					break;
				}
				const std::string key = std::to_string(static_cast<int>(kind)) + "|" + mesh.resource + "|" + mesh.texture.generic_string() + "|" + (mesh.cutout ? "c" : "") + (mesh.blended ? "b" : "") + (mesh.name.ends_with(" roof") ? "r" : "");
				auto it = merged.find(key);
				if (it == merged.end())
				{
					Part &part = out.parts.emplace_back();
					part.kind = kind;
					part.index = mesh.placement;
					part.subIndex = 0;
					part.name = mesh.name;
					ApplyOverlayMaterial(part, mesh);
					it = merged.emplace(key, out.parts.size() - 1).first;
				}
				Part &part = out.parts[it->second];
				AppendOverlayMesh(part, mesh);
				++part.subIndex;	// Placements merged into the part
			}
			for (const auto &[key, index] : merged)
			{
				Part &part = out.parts[index];
				if (part.subIndex > 1)
					part.name += " (x" + std::to_string(part.subIndex) + ")";
			}

			out.objects.insert(out.objects.end(), std::make_move_iterator(overlayBuild.objects.begin()), std::make_move_iterator(overlayBuild.objects.end()));
			out.warnings.insert(out.warnings.end(), overlayBuild.warnings.begin(), overlayBuild.warnings.end());
			out.overlayMeshes += static_cast<uint32_t>(overlayBuild.meshes.size());
			out.overlayCulled += overlayBuild.culled;
			out.overlayUnsupported += overlayBuild.unsupported;
		}

		// -------------------------------------------------------

		const char *GroupName(const AirportElementKind kind)
		{
			switch (kind)
			{
				case AirportElementKind::Runway:
				case AirportElementKind::WaterRunway:		return "Runways";
				case AirportElementKind::Helipad:			return "Helipads";
				case AirportElementKind::Pavement:			return "Pavement";
				case AirportElementKind::LinearFeature:		return "Lines and lights";
				case AirportElementKind::Boundary:			return "Boundary";
				case AirportElementKind::Viewpoint:
				case AirportElementKind::Beacon:
				case AirportElementKind::Windsock:			return "Airport furniture";
				case AirportElementKind::Sign:				return "Signs";
				case AirportElementKind::LightFixture:		return "Approach lights";
				case AirportElementKind::Startup:
				case AirportElementKind::LegacyStartup:		return "Startup locations";
				case AirportElementKind::TaxiRoutes:
				case AirportElementKind::TaxiNodes:			return "Taxi network";
				case AirportElementKind::TruckParking:
				case AirportElementKind::TruckDestination:	return "Ground service";
				case AirportElementKind::Jetway:			return "Jetways";
				case AirportElementKind::DrapedPolygon:		return "Draped polygons";
				case AirportElementKind::LineString:		return "Line strings";
				case AirportElementKind::Forest:			return "Forests";
				case AirportElementKind::Facade:			return "Facades";
				case AirportElementKind::SceneryObject:		return "Objects";
				case AirportElementKind::AirportLight:		return "Airport lights";
				default:									return "Other";
			}
		}

		struct TextureEntry
		{
			Ref<ImageResource> image;
			TextureInfo info;
		};

		struct Loaded
		{
			uint64_t root = 0;
			std::array<uint64_t, static_cast<size_t>(AirportElementKind::Count)> groups{};
			std::vector<uint64_t> entities;			// Every part entity
			std::vector<std::pair<uint64_t, AirportElementKind>> objectRoots;	// Placed OBJ8 roots and the group kind they're under
			Ref<Mesh> mesh;
			std::array<Ref<MaterialAsset>, static_cast<size_t>(Paint::Count)> materials;
			std::map<std::string, Ref<MaterialAsset>> overlayMaterials;		// By texture + flags
			std::map<std::string, TextureEntry> textures;					// By file
			GeoProjection projection;
			Bounds2 bounds;
			uint64_t revision = 0;
			uint64_t geometryRevision = 0;
			uint64_t libraryGeneration = 0;
			SceneAirports::Stats stats;
			std::array<bool, static_cast<size_t>(AirportElementKind::Count)> kindVisible;

			Loaded() { kindVisible.fill(true); }
		};

		std::unique_ptr<Loaded> s_Loaded;
		GeoProjection s_NoProjection;

		entt::registry *GetRegistry()
		{
			Scene *scene = Scene::GetActive();
			return scene ? &scene->GetRegistry() : nullptr;
		}

		entt::entity FindHandle(entt::registry &registry, const uint64_t id)
		{
			if (id == 0)
				return entt::null;
			const Entity entity = Scene::TryGetEntityWithUUID(UUID(id));
			const entt::entity handle = entity ? static_cast<entt::entity>(entity) : entt::null;
			return registry.valid(handle) ? handle : entt::null;
		}

		uint64_t GetId(entt::registry &registry, const entt::entity handle)
		{
			const IDComponent *id = registry.try_get<IDComponent>(handle);
			return id ? static_cast<uint64_t>(id->pID) : 0;
		}

		void Attach(entt::registry &registry, const entt::entity child, const entt::entity parent)
		{
			const UUID parentId = registry.get<IDComponent>(parent).pID;
			const UUID childId = registry.get<IDComponent>(child).pID;
			registry.get<RelationshipComponent>(child).pParentHandle = parentId;
			registry.get<RelationshipComponent>(parent).pChildren.push_back(childId);
		}

		Ref<MaterialAsset> MakeMaterial(const Paint paint)
		{
			const PaintSpec &spec = PAINTS[static_cast<size_t>(paint)];
			Ref<MaterialAsset> material = CreateRef<MaterialAsset>();
			material->SetName(std::string("Airport ") + spec.name);
			material->SetProperty(MaterialClass::ColorR, spec.r);
			material->SetProperty(MaterialClass::ColorG, spec.g);
			material->SetProperty(MaterialClass::ColorB, spec.b);
			material->SetProperty(MaterialClass::ColorA, spec.a);
			material->SetProperty(MaterialClass::TextureTilingX, 1.0f);
			material->SetProperty(MaterialClass::TextureTilingY, 1.0f);
			material->SetProperty(MaterialClass::Roughness, spec.roughness);
			material->SetProperty(MaterialClass::Metalness, 0.0f);
			material->SetProperty(MaterialClass::Normal, 0.0f);
			material->SetProperty(MaterialProperty::CullMode, static_cast<float>(CullMode::None));	// Visible from below too
			if (spec.a < 1.0f)
				material->SetProperty(MaterialProperty::BlendMode, 1.0f);
			return material;
		}

		/* @brief The material for library / DSF art: its texture (loaded once per file) with the asset's alpha treatment. */
		Ref<MaterialAsset> GetOverlayMaterial(Loaded &loaded, const Part &part)
		{
			ImageResource *image = nullptr;
			bool hasAlpha = false;
			std::string key;
			if (!part.texture.empty())
			{
				key = part.texture.generic_string();
				auto it = loaded.textures.find(key);
				if (it == loaded.textures.end())
				{
					TextureEntry entry;
					const fs::path file = TextureLoader::ResolveTexturePath(part.texture.parent_path(), part.texture.filename().string());
					if (!file.empty())
						entry.image = TextureLoader::Load(file, TextureUsage::Albedo, &entry.info);
					if (!entry.image)
						SEDX_CORE_WARN_TAG(LOG_TAG, "Texture not found or unreadable: {}{}", key, entry.info.error.empty() ? std::string() : " (" + entry.info.error + ")");
					it = loaded.textures.emplace(key, std::move(entry)).first;
				}
				image = it->second.image.Get();
				hasAlpha = it->second.info.hasAlpha;
			}
			if (!image)
			{
				char color[64];
				std::snprintf(color, sizeof(color), "color:%.3f,%.3f,%.3f,%.3f", part.color[0], part.color[1], part.color[2], part.color[3]);
				key = color;
			}

			const bool cutout = image && hasAlpha && part.cutout;
			const bool blended = image && hasAlpha && part.blended && !cutout;
			if (cutout)
				key += "|cut" + std::to_string(part.cutoff);
			if (blended)
				key += "|blend";

			auto it = loaded.overlayMaterials.find(key);
			if (it != loaded.overlayMaterials.end())
				return it->second;

			Ref<MaterialAsset> material = CreateRef<MaterialAsset>();
			material->SetName(image ? "Airport " + part.texture.filename().string() : "Airport overlay color");
			const float tone = image ? 1.0f : 1.0f;
			material->SetProperty(MaterialClass::ColorR, image ? tone : part.color[0]);
			material->SetProperty(MaterialClass::ColorG, image ? tone : part.color[1]);
			material->SetProperty(MaterialClass::ColorB, image ? tone : part.color[2]);
			material->SetProperty(MaterialClass::ColorA, image ? 1.0f : part.color[3]);
			material->SetProperty(MaterialClass::TextureTilingX, 1.0f);
			material->SetProperty(MaterialClass::TextureTilingY, 1.0f);
			material->SetProperty(MaterialClass::Roughness, 0.9f);
			material->SetProperty(MaterialClass::Metalness, 0.0f);
			material->SetProperty(MaterialClass::Normal, 0.0f);
			material->SetProperty(MaterialProperty::CullMode, static_cast<float>(CullMode::None));
			// Like the OBJ8 materials: cutouts alpha-test at the asset's ratio; blended art keeps its opaque texels in the
			// deferred passes and blends the rest in the transparent pass.
			material->SetProperty(MaterialProperty::AlphaCutoff, blended ? BLEND_SPLIT_ALPHA : (cutout ? std::clamp(part.cutoff, 0.001f, 1.0f) : 0.0f));
			material->SetProperty(MaterialProperty::BlendMode, blended ? 1.0f : 0.0f);
			if (image)
				material->SetTexture(MaterialTextureType::Color, image);
			loaded.overlayMaterials.emplace(key, material);
			return material;
		}

		/* @brief The group entity for a kind (kinds sharing a group name share the entity), created under the root when missing. */
		entt::entity EnsureGroup(Loaded &loaded, entt::registry &registry, const entt::entity rootHandle, const AirportElementKind kind)
		{
			const size_t kindIndex = static_cast<size_t>(kind);
			if (loaded.groups[kindIndex] != 0)
			{
				if (const entt::entity handle = FindHandle(registry, loaded.groups[kindIndex]); handle != entt::null)
					return handle;
				loaded.groups[kindIndex] = 0;
			}

			const char *groupName = GroupName(kind);
			for (size_t k = 0; k < loaded.groups.size(); ++k)
			{
				if (loaded.groups[k] != 0 && std::string_view(GroupName(static_cast<AirportElementKind>(k))) == groupName)
				{
					if (const entt::entity handle = FindHandle(registry, loaded.groups[k]); handle != entt::null)
					{
						loaded.groups[kindIndex] = loaded.groups[k];
						return handle;
					}
				}
			}

			const Entity group = Scene::CreateEntity(groupName);
			const entt::entity groupHandle = static_cast<entt::entity>(group);
			if (!registry.valid(groupHandle))
				return entt::null;
			AirportElementComponent &component = registry.emplace<AirportElementComponent>(groupHandle);
			component.kind = AirportElementKind::Group;
			component.index = static_cast<uint32_t>(kindIndex);
			component.airportRoot = loaded.root;
			Attach(registry, groupHandle, rootHandle);
			loaded.groups[kindIndex] = GetId(registry, groupHandle);
			++loaded.stats.entities;
			return groupHandle;
		}

		/* @brief Places the OBJ8s (DSF objects, strings, facade attachments, light strings) under their groups. */
		void PlaceObjects(Loaded &loaded, entt::registry &registry, const entt::entity rootHandle, const Build &build)
		{
			if (build.objects.empty())
				return;
			const auto start = std::chrono::steady_clock::now();
			uint32_t placed = 0, failed = 0;
			std::string firstError;
			for (const OverlayObject &object : build.objects)
			{
				const AirportElementKind kind = object.source == OverlayObject::Source::AirportLights ? AirportElementKind::AirportLight : AirportElementKind::SceneryObject;
				const entt::entity groupHandle = EnsureGroup(loaded, registry, rootHandle, kind);
				if (groupHandle == entt::null)
					break;

				SceneObjects::PlaceOptions options;
				options.position[0] = static_cast<float>(object.x);
				options.position[1] = static_cast<float>(object.y);
				options.position[2] = static_cast<float>(object.z);
				options.headingDegrees = object.heading;
				options.virtualPath = object.resource.starts_with("lib/") ? object.resource : std::string();
				options.name = object.name;
				options.frameCamera = false;
				std::string error;
				const uint64_t id = SceneObjects::Place(object.file, options, &error);
				const entt::entity handle = FindHandle(registry, id);
				if (handle == entt::null)
				{
					++failed;
					if (firstError.empty())
						firstError = error;
					continue;
				}
				Attach(registry, handle, groupHandle);
				if (!loaded.kindVisible[static_cast<size_t>(kind)])
				{
					if (Entity *stored = Scene::GetStoredEntity(Entity(handle, Scene::GetActive())))
						stored->SetActive(false);
				}
				loaded.objectRoots.emplace_back(id, kind);
				++placed;
			}
			loaded.stats.objects += placed;
			loaded.stats.entities += placed;
			SEDX_CORE_INFO_TAG(LOG_TAG, "Placed {} objects ({} light fixtures) in {:.0f} ms{}", placed,
				std::count_if(loaded.objectRoots.begin(), loaded.objectRoots.end(), [](const auto &entry) { return entry.second == AirportElementKind::AirportLight; }),
				std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count(),
				failed ? "; " + std::to_string(failed) + " could not be placed: " + firstError : std::string());
		}

		/* @brief Builds the groups and part entities under the root, then places the objects. */
		void BuildEntities(Loaded &loaded, const AirportDocument &document)
		{
			entt::registry *registry = GetRegistry();
			const entt::entity rootHandle = registry ? FindHandle(*registry, loaded.root) : entt::null;
			if (!registry || rootHandle == entt::null)
				return;

			const auto start = std::chrono::steady_clock::now();
			loaded.libraryGeneration = LibraryManager::GetGeneration();
			Build build;
			double datumLat = 0.0, datumLon = 0.0;
			std::unique_ptr<OverlayBuilder> overlay;
			if (document.airport.GetDatum(datumLat, datumLon))
				overlay = std::make_unique<OverlayBuilder>(MakeResolver(document.GetPackageFolder()), GeoProjection(datumLat, datumLon));
			BuildAirport(document.airport, build, overlay.get());
			if (overlay)
				BuildOverlay(document, build, *overlay);
			loaded.projection = build.projection;
			loaded.bounds = build.bounds;
			loaded.groups.fill(0);
			loaded.entities.clear();
			loaded.objectRoots.clear();
			loaded.overlayMaterials.clear();
			loaded.textures.clear();
			loaded.stats = {};
			loaded.stats.overlayMeshes = build.overlayMeshes;
			loaded.stats.warnings = static_cast<uint32_t>(build.warnings.size());

			// One mesh, a sub-mesh per part
			loaded.mesh.Reset();
			bool anyGeometry = false;
			for (Part &part : build.parts)
			{
				if (part.indices.empty())
					continue;
				if (!loaded.mesh)
				{
					loaded.mesh = CreateRef<Mesh>();
					loaded.mesh->SetFlags(0);	// Keep the local meters as they are: no normalization or generated LODs
					loaded.mesh->SetResourceName("Airport " + document.airport.id);
				}
				loaded.mesh->AddGeometry(part.vertices, part.indices, false, &part.subMesh);
				loaded.stats.triangles += static_cast<uint32_t>(part.indices.size() / 3);
				anyGeometry = true;
			}
			if (anyGeometry)
				loaded.mesh->CreateGpuBuffers();

			// Groups, in part order, only for kinds that have parts
			for (const Part &part : build.parts)
				EnsureGroup(loaded, *registry, rootHandle, part.kind);

			for (const Part &part : build.parts)
			{
				const Entity entity = Scene::CreateEntity(part.name);
				const entt::entity handle = static_cast<entt::entity>(entity);
				if (!registry->valid(handle))
					continue;

				AirportElementComponent &component = registry->emplace<AirportElementComponent>(handle);
				component.kind = part.kind;
				component.index = part.index;
				component.subIndex = part.subIndex;
				component.airportRoot = loaded.root;
				component.triangles = static_cast<uint32_t>(part.indices.size() / 3);
				component.bounds = part.bounds;

				const entt::entity groupHandle = FindHandle(*registry, loaded.groups[static_cast<size_t>(part.kind)]);
				Attach(*registry, handle, groupHandle != entt::null ? groupHandle : rootHandle);

				if (part.placed)
				{
					// Heading is clockwise from north (-Z), like DSF placements; the entity's yaw is the opposite sense.
					TransformComponent &transform = registry->get<TransformComponent>(handle);
					transform.translation = Vec3(static_cast<float>(part.origin.x), 0.0f, static_cast<float>(part.origin.z));
					transform.SetRotationEuler(Vec3(0.0f, -static_cast<float>(part.heading) * kDegToRad, 0.0f));
				}

				// DSF art whose texture is missing (packs ship broken references) isn't drawn, like in X-Plane, rather
				// than covering the ground in flat gray. The entity stays so it can be found and framed.
				bool drawable = !part.indices.empty();
				Ref<MaterialAsset> overlayMaterial;
				if (drawable && part.textured)
				{
					overlayMaterial = GetOverlayMaterial(loaded, part);
					const bool textureOnly = part.kind == AirportElementKind::DrapedPolygon || part.kind == AirportElementKind::LineString || part.kind == AirportElementKind::Forest;
					if (textureOnly && !overlayMaterial->GetTexture(MaterialTextureType::Color))
						drawable = false;
				}

				if (drawable)
				{
					if (Entity *stored = Scene::GetStoredEntity(entity))
					{
						if (Renderable *renderable = stored->AddComponent<Renderable>())
						{
							Ref<MaterialAsset> material;
							if (part.textured)
								material = overlayMaterial;
							else
							{
								Ref<MaterialAsset> &paintMaterial = loaded.materials[static_cast<size_t>(part.paint)];
								if (!paintMaterial)
									paintMaterial = MakeMaterial(part.paint);
								material = paintMaterial;
							}
							renderable->SetMesh(loaded.mesh.Get(), part.subMesh);
							renderable->SetMaterial(material);
							renderable->SetFlag(CastsShadows, part.kind == AirportElementKind::Facade || part.kind == AirportElementKind::Forest);
						}
						if (!loaded.kindVisible[static_cast<size_t>(part.kind)])
							stored->SetActive(false);
					}
				}
				loaded.entities.push_back(GetId(*registry, handle));
				++loaded.stats.entities;
				++loaded.stats.parts;
			}

			PlaceObjects(loaded, *registry, rootHandle, build);

			loaded.revision = document.GetRevision();
			loaded.geometryRevision = document.GetGeometryRevision();
			loaded.stats.buildMilliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
			SEDX_CORE_INFO_TAG(LOG_TAG, "{} {}: {} parts ({} overlay meshes merged in), {} triangles, {} objects, {} entities, {} textures in {:.0f} ms", document.airport.id, document.airport.name,
				loaded.stats.parts, build.overlayMeshes, loaded.stats.triangles, loaded.stats.objects, loaded.stats.entities, loaded.textures.size(), loaded.stats.buildMilliseconds);
			if (build.overlayCulled || build.overlayUnsupported)
				SEDX_CORE_INFO_TAG(LOG_TAG, "Overlay: {} placements outside the airport skipped, {} of unsupported kinds (roads, autogen blocks)", build.overlayCulled, build.overlayUnsupported);
			constexpr size_t maxWarnings = 25;
			for (size_t i = 0; i < build.warnings.size() && i < maxWarnings; ++i)
				SEDX_CORE_WARN_TAG(LOG_TAG, "{}", build.warnings[i]);
			if (build.warnings.size() > maxWarnings)
				SEDX_CORE_WARN_TAG(LOG_TAG, "... and {} more asset warnings", build.warnings.size() - maxWarnings);
		}

		void DestroyChildren(Loaded &loaded)
		{
			entt::registry *registry = GetRegistry();
			if (!registry)
				return;
			const entt::entity rootHandle = FindHandle(*registry, loaded.root);
			if (rootHandle == entt::null)
				return;

			// Groups take their parts and objects with them.
			const std::vector<UUID> children = registry->get<RelationshipComponent>(rootHandle).pChildren;
			for (const UUID &child : children)
			{
				if (const Entity entity = Scene::TryGetEntityWithUUID(child))
					Scene::DestroyEntity(entity);
			}
			loaded.groups.fill(0);
			loaded.entities.clear();
			loaded.objectRoots.clear();
			loaded.mesh.Reset();
			loaded.overlayMaterials.clear();
			loaded.textures.clear();
		}

		/* @brief Three-quarter view from the south-east that fits a sphere; the sphere's center becomes the orbit pivot. */
		void FrameSphere(const Vec3 &center, const float radius, const float pitchDegrees)
		{
			entt::registry *registry = GetRegistry();
			if (!registry)
				return;
			auto view = registry->view<TransformComponent, CameraComponent>();
			for (const entt::entity handle : view)
			{
				if (!view.get<CameraComponent>(handle).isPrimary)
					continue;

				const float yaw = -35.0f * kDegToRad;
				const float pitch = pitchDegrees * kDegToRad;
				const Vec3 forward(std::cos(pitch) * std::sin(yaw), std::sin(pitch), std::cos(pitch) * std::cos(yaw));
				const float distance = std::max(radius, 5.0f) / std::sin(30.0f * kDegToRad) * 1.1f;
				const Vec3 eye = center - forward * distance;

				TransformComponent &transform = view.get<TransformComponent>(handle);
				if (Camera *camera = Scene::GetCamera())
					camera->LookAt(transform, eye, center);
				else
				{
					transform.translation = eye;
					transform.SetRotationEuler(Vec3(pitch, yaw, 0.0f));
				}
				break;
			}
		}

		Vec3 RootOffset(const Loaded &loaded)
		{
			entt::registry *registry = GetRegistry();
			const entt::entity rootHandle = registry ? FindHandle(*registry, loaded.root) : entt::null;
			if (rootHandle == entt::null)
				return Vec3(0.0f, 0.0f, 0.0f);
			return registry->get<TransformComponent>(rootHandle).translation;
		}
	}

	// -------------------------------------------------------

	const char *GetAirportElementKindName(const AirportElementKind kind)
	{
		switch (kind)
		{
			case AirportElementKind::Airport:			return "Airport";
			case AirportElementKind::Group:				return "Group";
			case AirportElementKind::Runway:			return "Runway";
			case AirportElementKind::WaterRunway:		return "Water runway";
			case AirportElementKind::Helipad:			return "Helipad";
			case AirportElementKind::Pavement:			return "Pavement";
			case AirportElementKind::LinearFeature:		return "Line";
			case AirportElementKind::Boundary:			return "Boundary";
			case AirportElementKind::Viewpoint:			return "Viewpoint";
			case AirportElementKind::Beacon:			return "Beacon";
			case AirportElementKind::Windsock:			return "Windsock";
			case AirportElementKind::Sign:				return "Sign";
			case AirportElementKind::LightFixture:		return "Light fixture";
			case AirportElementKind::Startup:			return "Startup location";
			case AirportElementKind::LegacyStartup:		return "Startup location (legacy)";
			case AirportElementKind::TaxiRoutes:		return "Taxi routes";
			case AirportElementKind::TaxiNodes:			return "Taxi nodes";
			case AirportElementKind::TruckParking:		return "Truck parking";
			case AirportElementKind::TruckDestination:	return "Truck destination";
			case AirportElementKind::Jetway:			return "Jetway";
			case AirportElementKind::DrapedPolygon:		return "Draped polygon";
			case AirportElementKind::LineString:		return "Line string";
			case AirportElementKind::Forest:			return "Forest";
			case AirportElementKind::Facade:			return "Facade";
			case AirportElementKind::SceneryObject:		return "Object";
			case AirportElementKind::AirportLight:		return "Airport light";
			default:									return "Unknown";
		}
	}

	bool SceneAirports::IsOverlayKind(const AirportElementKind kind)
	{
		switch (kind)
		{
			case AirportElementKind::DrapedPolygon:
			case AirportElementKind::LineString:
			case AirportElementKind::Forest:
			case AirportElementKind::Facade:
			case AirportElementKind::SceneryObject:
			case AirportElementKind::AirportLight:
				return true;
			default:
				return false;
		}
	}

	uint64_t SceneAirports::Load(const AirportDocument &document, const bool frameCamera)
	{
		Unload();
		entt::registry *registry = GetRegistry();
		if (!registry)
		{
			SEDX_CORE_WARN_TAG(LOG_TAG, "No active scene to load {} into", document.airport.id);
			return 0;
		}

		std::string rootName = "Airport " + document.airport.id;
		if (!document.airport.name.empty())
			rootName += " - " + document.airport.name;
		const Entity root = Scene::CreateEntity(rootName);
		const entt::entity rootHandle = static_cast<entt::entity>(root);
		if (!registry->valid(rootHandle))
			return 0;

		auto loaded = std::make_unique<Loaded>();
		loaded->root = GetId(*registry, rootHandle);
		AirportElementComponent &component = registry->emplace<AirportElementComponent>(rootHandle);
		component.kind = AirportElementKind::Airport;
		component.airportRoot = loaded->root;

		BuildEntities(*loaded, document);
		registry->get<AirportElementComponent>(rootHandle).bounds = loaded->bounds;
		s_Loaded = std::move(loaded);

		if (frameCamera)
			Frame();
		return s_Loaded->root;
	}

	void SceneAirports::Rebuild(const AirportDocument &document)
	{
		if (!s_Loaded)
			return;
		entt::registry *registry = GetRegistry();
		const entt::entity rootHandle = registry ? FindHandle(*registry, s_Loaded->root) : entt::null;
		if (rootHandle == entt::null)
		{
			// The user deleted the root: start over.
			s_Loaded.reset();
			return;
		}

		DestroyChildren(*s_Loaded);
		BuildEntities(*s_Loaded, document);
		if (AirportElementComponent *component = registry->try_get<AirportElementComponent>(rootHandle))
			component->bounds = s_Loaded->bounds;

		std::string rootName = "Airport " + document.airport.id;
		if (!document.airport.name.empty())
			rootName += " - " + document.airport.name;
		if (TagComponent *tag = registry->try_get<TagComponent>(rootHandle))
			tag->pTag = rootName;
	}

	void SceneAirports::Unload()
	{
		if (!s_Loaded)
			return;
		if (entt::registry *registry = GetRegistry())
		{
			if (const Entity root = Scene::TryGetEntityWithUUID(UUID(s_Loaded->root)); root && registry->valid(static_cast<entt::entity>(root)))
				Scene::DestroyEntity(root);
		}
		s_Loaded.reset();
	}

	bool SceneAirports::IsLoaded()
	{
		if (!s_Loaded)
			return false;
		entt::registry *registry = GetRegistry();
		if (!registry || FindHandle(*registry, s_Loaded->root) == entt::null)
		{
			s_Loaded.reset();
			return false;
		}
		return true;
	}

	uint64_t SceneAirports::GetRootId()
	{
		return IsLoaded() ? s_Loaded->root : 0;
	}

	uint64_t SceneAirports::GetBuiltRevision()
	{
		return IsLoaded() ? s_Loaded->revision : 0;
	}

	uint64_t SceneAirports::GetBuiltGeometryRevision()
	{
		return IsLoaded() ? s_Loaded->geometryRevision : 0;
	}

	uint64_t SceneAirports::GetBuiltLibraryGeneration()
	{
		return IsLoaded() ? s_Loaded->libraryGeneration : 0;
	}

	bool SceneAirports::IsPlacedKind(const AirportElementKind kind)
	{
		switch (kind)
		{
			case AirportElementKind::Viewpoint:
			case AirportElementKind::Beacon:
			case AirportElementKind::Windsock:
			case AirportElementKind::Sign:
			case AirportElementKind::LightFixture:
			case AirportElementKind::Startup:
			case AirportElementKind::LegacyStartup:
			case AirportElementKind::TruckParking:
			case AirportElementKind::TruckDestination:
			case AirportElementKind::Jetway:
				return true;
			default:
				return false;
		}
	}

	void SceneAirports::UpdatePlacements(const AirportDocument &document)
	{
		if (!IsLoaded())
			return;
		entt::registry *registry = GetRegistry();
		Bounds2 bounds = s_Loaded->bounds;
		for (const uint64_t id : s_Loaded->entities)
		{
			const entt::entity handle = FindHandle(*registry, id);
			if (handle == entt::null)
				continue;
			AirportElementComponent *component = registry->try_get<AirportElementComponent>(handle);
			if (!component || !IsPlacedKind(component->kind))
				continue;

			Point2 origin;
			double heading = 0.0;
			if (!GetPlacement(document.airport, s_Loaded->projection, component->kind, component->index, origin, heading))
				continue;

			TransformComponent &transform = registry->get<TransformComponent>(handle);
			transform.translation = Vec3(static_cast<float>(origin.x), 0.0f, static_cast<float>(origin.z));
			transform.SetRotationEuler(Vec3(0.0f, -static_cast<float>(heading) * kDegToRad, 0.0f));

			const double size = std::max(3.0, component->bounds.GetRadius());
			component->bounds = Bounds2();
			component->bounds.Add(Point2{ origin.x - size, origin.z - size });
			component->bounds.Add(Point2{ origin.x + size, origin.z + size });
			bounds.Add(component->bounds);
		}
		s_Loaded->bounds = bounds;
		s_Loaded->revision = document.GetRevision();
	}

	const AirportElementComponent *SceneAirports::FindElement(const uint64_t entityId)
	{
		if (!IsLoaded())
			return nullptr;
		entt::registry *registry = GetRegistry();
		const entt::entity handle = registry ? FindHandle(*registry, entityId) : entt::null;
		if (handle == entt::null)
			return nullptr;
		const AirportElementComponent *component = registry->try_get<AirportElementComponent>(handle);
		return component && component->airportRoot == s_Loaded->root ? component : nullptr;
	}

	uint64_t SceneAirports::FindEntity(const AirportElementKind kind, const uint32_t index, const uint32_t subIndex)
	{
		if (!IsLoaded())
			return 0;
		entt::registry *registry = GetRegistry();
		for (const uint64_t id : s_Loaded->entities)
		{
			const entt::entity handle = FindHandle(*registry, id);
			if (handle == entt::null)
				continue;
			const AirportElementComponent *component = registry->try_get<AirportElementComponent>(handle);
			if (component && component->kind == kind && component->index == index && component->subIndex == subIndex)
				return id;
		}
		return 0;
	}

	void SceneAirports::SetKindVisible(const AirportElementKind kind, const bool visible)
	{
		if (!IsLoaded())
			return;
		s_Loaded->kindVisible[static_cast<size_t>(kind)] = visible;
		entt::registry *registry = GetRegistry();
		for (const uint64_t id : s_Loaded->entities)
		{
			const entt::entity handle = FindHandle(*registry, id);
			if (handle == entt::null)
				continue;
			const AirportElementComponent *component = registry->try_get<AirportElementComponent>(handle);
			if (!component || component->kind != kind)
				continue;
			if (Entity *stored = Scene::GetStoredEntity(Entity(handle, Scene::GetActive())))
				stored->SetActive(visible);
		}
		for (const auto &[id, objectKind] : s_Loaded->objectRoots)
		{
			if (objectKind != kind)
				continue;
			const entt::entity handle = FindHandle(*registry, id);
			if (handle == entt::null)
				continue;
			if (Entity *stored = Scene::GetStoredEntity(Entity(handle, Scene::GetActive())))
				stored->SetActive(visible);
		}
	}

	bool SceneAirports::IsKindVisible(const AirportElementKind kind)
	{
		return !s_Loaded || s_Loaded->kindVisible[static_cast<size_t>(kind)];
	}

	void SceneAirports::Frame()
	{
		if (!IsLoaded() || !s_Loaded->bounds.valid)
			return;
		const Point2 center = s_Loaded->bounds.GetCenter();
		const Vec3 offset = RootOffset(*s_Loaded);
		FrameSphere(Vec3(static_cast<float>(center.x), 0.0f, static_cast<float>(center.z)) + offset, static_cast<float>(s_Loaded->bounds.GetRadius()), -50.0f);
	}

	void SceneAirports::FrameEntity(const uint64_t entityId)
	{
		const AirportElementComponent *component = FindElement(entityId);
		if (!component || !IsLoaded())
			return;
		if (!component->bounds.valid)
		{
			Frame();
			return;
		}
		const Point2 center = component->bounds.GetCenter();
		const Vec3 offset = RootOffset(*s_Loaded);
		FrameSphere(Vec3(static_cast<float>(center.x), 0.0f, static_cast<float>(center.z)) + offset, std::max(3.0f, static_cast<float>(component->bounds.GetRadius())), -35.0f);
	}

	const GeoProjection &SceneAirports::GetProjection()
	{
		return s_Loaded ? s_Loaded->projection : s_NoProjection;
	}

	SceneAirports::Stats SceneAirports::GetStats()
	{
		return s_Loaded ? s_Loaded->stats : Stats{};
	}

	void SceneAirports::Shutdown()
	{
		Unload();
	}
}

// -------------------------------------------------------

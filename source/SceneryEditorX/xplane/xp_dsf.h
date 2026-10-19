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
 * xp_dsf.h
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	/**
	 * X-Plane DSF scenery tiles (https://developer.x-plane.com/article/dsf-file-format-specification/), read far
	 * enough for overlays: the placements of objects, polygons (facades, forests, lines, strings, draped polygons)
	 * and road chains, with every definition name. Terrain patches are counted and skipped. Standard library only;
	 * 7-zip compressed tiles are detected and reported, not decoded.
	 */

	/* @brief A pool point: up to 8 planes (longitude, latitude, then type-specific values). */
	struct DsfPoint
	{
		std::array<double, 8> v{};
		uint8_t planes = 0;

		[[nodiscard]] double Longitude() const { return v[0]; }
		[[nodiscard]] double Latitude() const { return v[1]; }
	};

	/* @brief One OBJECT command. */
	struct DsfObjectPlacement
	{
		enum class Mode : uint8_t
		{
			Draped,		// On the ground (3 planes)
			Msl,		// Elevation is meters above sea level (4 planes)
			Agl			// Elevation is meters above the ground (4 planes)
		};

		uint32_t definition = 0;		// Index into DsfFile::objectDefinitions
		double longitude = 0.0;
		double latitude = 0.0;
		double heading = 0.0;			// True degrees
		double elevation = 0.0;
		Mode mode = Mode::Draped;
		int32_t filter = -1;			// Airport filter: index among the sim/filter/aptid properties, -1 for none
	};

	/* @brief One POLYGON / NESTED_POLYGON command: a facade, forest, line, string, draped polygon or autogen block. */
	struct DsfPolygonPlacement
	{
		uint32_t definition = 0;		// Index into DsfFile::polygonDefinitions
		uint16_t parameter = 0;			// Meaning depends on the asset: facade height, forest density, line closed, string spacing, pol heading / 65535 for UV mapped
		uint8_t planes = 2;				// Per point: 2 lon/lat, 3 + wall type, 4 + bezier control, 5..8 with UVs
		std::vector<std::vector<DsfPoint>> windings;	// windings[0] outline, the rest holes
		int32_t filter = -1;			// Airport filter: index among the sim/filter/aptid properties, -1 for none
	};

	/* @brief A point of a road segment: a vertex the road runs through, or a bezier control point of the curve between vertices. */
	struct DsfRoadPoint
	{
		double longitude = 0.0;
		double latitude = 0.0;
		bool control = false;			// Between two vertices: none = straight, one = quadratic, two = cubic curve
	};

	/**
	 * @brief A road, railway or power line segment between two junctions (network chains, decoded). Segments meet
	 * where they share a junction id. In the file each point is lon, lat, then the junction's level (or for shape
	 * points 0 = vertex, 1 = bezier control point), then the junction id (0 for shape points), in 32-bit pools.
	 */
	struct DsfRoadSegment
	{
		uint32_t definition = 0;		// Index into DsfFile::networkDefinitions
		uint32_t subtype = 0;			// Road type of the .net (0..255)
		uint32_t startJunction = 0;		// 1 and up
		uint32_t endJunction = 0;
		int startLevel = 0;				// 0 on the ground, 1 and up raised (bridges, overpasses)
		int endLevel = 0;
		std::vector<DsfRoadPoint> points;	// Start junction ... end junction, at least two vertices
		int32_t filter = -1;			// Airport filter: index among the sim/filter/aptid properties, -1 for none
	};

	struct DsfFile
	{
		std::vector<std::pair<std::string, std::string>> properties;	// PROP atom
		std::vector<std::string> terrainDefinitions;
		std::vector<std::string> objectDefinitions;
		std::vector<std::string> polygonDefinitions;
		std::vector<std::string> networkDefinitions;
		std::vector<std::string> rasterDefinitions;

		std::vector<DsfObjectPlacement> objects;
		std::vector<DsfPolygonPlacement> polygons;
		std::vector<DsfRoadSegment> roads;
		uint32_t terrainPatchCount = 0;
		uint32_t terrainTriangleCount = 0;

		double west = 0.0;				// sim/west ... sim/north
		double south = 0.0;
		double east = 0.0;
		double north = 0.0;

		/* @brief Value of a PROP key ("sim/overlay", "sim/require_object", ...), or empty. */
		[[nodiscard]] std::string_view GetProperty(std::string_view key) const;

		/* @brief sim/overlay 1: scenery drawn on top of the base mesh. */
		[[nodiscard]] bool IsOverlay() const;

		/* @brief Exclusion zones (sim/exclude_*), as "west/south/east/north" strings paired with their key. */
		[[nodiscard]] std::vector<std::pair<std::string, std::string>> GetExclusions() const;
	};

	/* @brief Parses a DSF held in memory. */
	bool ReadDsf(const uint8_t *data, size_t size, DsfFile &out, std::string *error = nullptr);

	/* @brief Reads a DSF file. A 7-zip compressed tile fails with an error naming the problem. */
	bool LoadDsf(const std::filesystem::path &path, DsfFile &out, std::string *error = nullptr);

	/* @brief True when the file starts with the 7-zip signature X-Plane uses for compressed tiles. */
	bool IsDsfCompressed(const std::filesystem::path &path);

	/**
	 * @brief Encodes a DSF tile the way X-Plane reads it: the properties as given (sim/west ... sim/north must be among
	 * them), the object, polygon and network definitions, 16-bit point pools (32-bit for road segments), the command
	 * stream and the MD5 footer. Overlay content only: objects, polygons and roads are written; terrain and rasters aren't.
	 *
	 * Points are quantized per pool. Pools are kept to DSF_POOL_SPAN degrees where the content allows, so positions
	 * land within a few centimeters (a polygon larger than that gets a pool of its own size); planes that hold only
	 * whole numbers from 0 to 65535 (wall types, whole headings, whole elevations) round trip exactly. Objects with an
	 * elevation are written MSL or AGL (the AGL comment), the rest draped.
	 * @return False (with the reason) when a polygon has more than 65535 points or 255 windings.
	 */
	bool WriteDsf(const DsfFile &dsf, std::vector<uint8_t> &out, std::string *error = nullptr);

	/* @brief Degrees a point pool spans at most (unless one polygon is larger): 0.02 degrees keeps points within ~3 cm. */
	inline constexpr double DSF_POOL_SPAN = 0.02;

	/**
	 * @brief Splits a DSF covering several 1x1 degree tiles into one DSF per tile, sorted south-west first: objects go
	 * to the tile under them, polygons to the tile of their first point (a warning names any reaching into another
	 * tile), exclusions (sim/exclude_*) to every tile they overlap. Definitions are renumbered per tile, the bounds
	 * properties set per tile; every other property is copied.
	 */
	std::vector<DsfFile> SplitDsfByTile(const DsfFile &dsf, std::vector<std::string> *warnings = nullptr);

	/* @brief MD5 digest (RFC 1321) of some bytes, as a DSF footer carries it. */
	std::array<uint8_t, 16> Md5(const uint8_t *data, size_t size);

	/* @brief "+44-124" style tile name for a coordinate (the 1x1 degree tile containing it). */
	std::string GetDsfTileName(double latitude, double longitude);

	/* @brief "+40-130" style 10x10 degree folder name. */
	std::string GetDsfTileFolderName(double latitude, double longitude);

	/* @brief "<package>/Earth nav data/+40-130/+44-124.dsf" for a coordinate. */
	std::filesystem::path GetDsfTilePath(const std::filesystem::path &packageFolder, double latitude, double longitude);
}

// -------------------------------------------------------

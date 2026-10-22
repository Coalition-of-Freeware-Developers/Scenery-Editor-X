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
	 * X-Plane DSF scenery tiles (https://developer.x-plane.com/article/dsf-file-format-specification/): the placements
	 * of objects, polygons (facades, forests, lines, strings, draped polygons) and road chains, with every definition
	 * name; on request (DsfReadOptions) the terrain patches of the base mesh and the raster layers (elevation,
	 * bathymetry, season ranges), limited to an area. 7-zip compressed tiles (the Global Scenery's) are unpacked on the
	 * way in (xp_7z.h). Standard library only.
	 */

	/* @brief What ReadDsf decodes besides the definitions and properties. */
	struct DsfReadOptions
	{
		bool overlays = true;				// Objects, polygons and roads
		bool overlaysOnlyInBaseMesh = false;	// Overlays only from a tile that isn't an overlay (sim/overlay 1): one read finds a pack's base mesh and its scenery, passing over overlay packs
		bool terrain = false;				// Terrain patches (off keeps overlay reads as fast as before)
		bool rasters = false;				// Raster layers
		std::vector<std::string> rasterNames;	// Rasters to keep (empty: all)
		/**
		 * Terrain triangles and overlay placements kept: those whose longitude / latitude box overlaps this one
		 * (degrees). An empty box (west >= east) keeps everything. Point pools wholly outside it aren't decoded at all.
		 */
		double areaWest = 0.0;
		double areaSouth = 0.0;
		double areaEast = 0.0;
		double areaNorth = 0.0;

		[[nodiscard]] bool HasArea() const { return areaWest < areaEast && areaSouth < areaNorth; }
	};

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

	/**
	 * @brief One terrain patch of the base mesh (TERRAIN_PATCH commands and the triangles after them), with the points
	 * its triangles use. Planes per point (DSF Usage in X-Plane): longitude, latitude, elevation (meters MSL; -32768
	 * means "sample the elevation raster"), the normal's east and north components (0, 0 when the raster provides
	 * them), then the .ter's texture coordinates: for an overlay patch the border's s, t; for a water patch (the
	 * "terrain_Water" definition) the fetch ratio (1 open sea, 0 pond) and the bathymetry ratio.
	 */
	struct DsfTerrainPatch
	{
		enum Flags : uint8_t
		{
			Physical = 1 << 0,		// Hard: part of the collision mesh
			Overlay = 1 << 1		// Drawn over the patches before it, faded by its .ter's BORDER_TEX
		};

		uint32_t definition = 0;		// Index into DsfFile::terrainDefinitions
		uint8_t flags = Physical;
		float lodStart = 0.0f;			// Meters (TERRAIN_PATCH_FLAGS_LOD)
		float lodEnd = -1.0f;			// -1: no limit
		uint8_t planes = 0;				// Values per point
		std::vector<double> points;		// planes values per point
		std::vector<uint32_t> indices;	// Triangles (strips and fans expanded), as wound in the file

		[[nodiscard]] uint32_t GetPointCount() const { return planes ? static_cast<uint32_t>(points.size() / planes) : 0u; }
		[[nodiscard]] const double *GetPoint(const uint32_t index) const { return points.data() + static_cast<size_t>(index) * planes; }
		[[nodiscard]] bool IsOverlay() const { return (flags & Overlay) != 0; }
	};

	/**
	 * @brief A raster layer (DEMS atom): a grid over the tile, row 0 the southern edge. Post-centric rasters sample the
	 * edges exactly (1201 x 1201 for a 3 arc-second DEM); area-centric ones sample cell centers.
	 */
	struct DsfRaster
	{
		std::string name;				// "elevation", "sea_level", "spr1" ... "win2", "soundscape"
		uint8_t version = 0;
		uint8_t bytesPerPixel = 0;
		uint16_t flags = 0;				// Bits 0-1: 0 float, 1 signed, 2 unsigned; bit 2: post-centric
		uint32_t width = 0;
		uint32_t height = 0;
		float scale = 1.0f;				// value = stored * scale + offset
		float offset = 0.0f;
		std::vector<float> values;		// width x height, row-major from the south-west corner

		[[nodiscard]] bool IsPostCentric() const { return (flags & 4) != 0; }
		[[nodiscard]] float At(uint32_t x, uint32_t y) const;

		/* @brief Bilinear value at a point of the tile bounds (clamped to the edges). */
		[[nodiscard]] float Sample(double longitude, double latitude, double west, double south, double east, double north) const;
	};

	/* @brief Elevation in a vertex that asks for the elevation raster's value instead. */
	inline constexpr double DSF_ELEVATION_FROM_RASTER = -32768.0;

	/**
	 * @brief One sim/exclude_* zone: X-Plane leaves out scenery of its kind from lower-priority DSFs under it. A value
	 * is "west/south/east/north"; X-Plane 12 polygon zones follow it with ";" and the polygon's longitude / latitude
	 * pairs (the box bounds them). A placement with any point inside is excluded (forests: tree by tree).
	 */
	struct DsfExclusion
	{
		std::string kind;				// What it excludes: obj, fac, for, bch, net, lin, pol, str
		double west = 0.0;
		double south = 0.0;
		double east = 0.0;
		double north = 0.0;
		std::vector<std::pair<double, double>> polygon;	// Longitude, latitude; empty for a box

		[[nodiscard]] bool Contains(double longitude, double latitude) const;
	};

	/* @brief Parses a sim/exclude_* property; false when the key or value isn't one. */
	bool ParseDsfExclusion(std::string_view key, std::string_view value, DsfExclusion &out);

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
		std::vector<DsfTerrainPatch> terrainPatches;	// DsfReadOptions::terrain only (those with triangles in its area)
		std::vector<DsfRaster> rasters;					// DsfReadOptions::rasters only
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

		/* @brief The exclusion zones, parsed (values that don't parse are skipped). */
		[[nodiscard]] std::vector<DsfExclusion> GetExclusionZones() const;

		/* @brief A raster layer by name, or nullptr. */
		[[nodiscard]] const DsfRaster *FindRaster(std::string_view name) const;
	};

	/* @brief Parses a DSF held in memory (overlay content only). A 7-zip compressed tile is unpacked first. */
	bool ReadDsf(const uint8_t *data, size_t size, DsfFile &out, std::string *error = nullptr);

	/* @brief Parses a DSF held in memory, decoding what the options ask for. */
	bool ReadDsf(const uint8_t *data, size_t size, DsfFile &out, const DsfReadOptions &options, std::string *error = nullptr);

	/* @brief Reads a DSF file (overlay content only), compressed or not. */
	bool LoadDsf(const std::filesystem::path &path, DsfFile &out, std::string *error = nullptr);

	/* @brief Reads a DSF file, compressed or not, decoding what the options ask for. */
	bool LoadDsf(const std::filesystem::path &path, DsfFile &out, const DsfReadOptions &options, std::string *error = nullptr);

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

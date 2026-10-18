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
	};

	/* @brief One POLYGON / NESTED_POLYGON command: a facade, forest, line, string, draped polygon or autogen block. */
	struct DsfPolygonPlacement
	{
		uint32_t definition = 0;		// Index into DsfFile::polygonDefinitions
		uint16_t parameter = 0;			// Meaning depends on the asset: facade height, forest density, line closed, string spacing, pol heading / 65535 for UV mapped
		uint8_t planes = 2;				// Per point: 2 lon/lat, 3 + wall type, 4 + bezier control, 5..8 with UVs
		std::vector<std::vector<DsfPoint>> windings;	// windings[0] outline, the rest holes
	};

	/* @brief A road / rail / power line chain (NETWORK_CHAIN). Recorded for completeness; not drawn yet. */
	struct DsfNetworkChain
	{
		uint32_t definition = 0;		// Index into DsfFile::networkDefinitions
		uint8_t subtype = 0;
		std::vector<DsfPoint> points;	// lon, lat, elevation, junction id, [bezier control...]
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
		std::vector<DsfNetworkChain> networks;
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

	/* @brief "+44-124" style tile name for a coordinate (the 1x1 degree tile containing it). */
	std::string GetDsfTileName(double latitude, double longitude);

	/* @brief "+40-130" style 10x10 degree folder name. */
	std::string GetDsfTileFolderName(double latitude, double longitude);

	/* @brief "<package>/Earth nav data/+40-130/+44-124.dsf" for a coordinate. */
	std::filesystem::path GetDsfTilePath(const std::filesystem::path &packageFolder, double latitude, double longitude);
}

// -------------------------------------------------------

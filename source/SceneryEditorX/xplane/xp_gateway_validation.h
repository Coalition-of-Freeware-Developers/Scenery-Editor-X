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
 * xp_gateway_validation.h
 * -------------------------------------------------------
 * Created: 24/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "xp_apt.h"
#include "xp_dsf.h"
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	class LibraryIndex;

	/**
	 * Checks an airport the way WorldEditor (WED 2.7) does for its "Airport Scenery Gateway" export target before it
	 * uploads: errors block the upload, warnings ask the artist to confirm. Ported from WED_Validate.cpp:
	 *
	 *  - name and airport ID rules
	 *  - an airport boundary when there are runways, without bezier curves or vertices under 30 m apart; runways,
	 *    taxiways and ramp starts inside it
	 *  - the extent: at most 7 nm across, nothing more than 0.6 nm past the boundary
	 *  - art only from X-Plane's default library (nothing private or deprecated), orthophotos only with automatic
	 *    subtexture selection, no set_MSL (set_AGL is a warning), no custom trucks, no transparent runways
	 *  - roads only from lib/g10/roads.net / roads_EU.net, layers 0..5, near the airport, not hugging a tile edge
	 *  - 1302 metadata: ISO-3166 alpha-3 country prefix, datum latitude / longitude as a pair near the airport, code
	 *    formats, region codes, transition altitude / level, no URLs
	 *  - plus everything ValidateAptAirport finds (structural problems X-Plane rejects)
	 *
	 * Not ported (see docs/scenery-gateway-integration.md): ATC flow / frequency rules, runway heading vs. name (needs a
	 * magnetic model), CIFP runway ends (runway_coordinates.txt), filling metadata from airport_metadata.csv.
	 *
	 * The overlay is the airport's DSF content (objects, polygons, roads of this airport only), whether it comes from a
	 * project (BuildProjectDsf) or a scenery pack's tiles. Standard library only.
	 */

	enum class GatewaySeverity : uint8_t
	{
		Error,		// Blocks the upload
		Warning		// Uploading needs the artist's confirmation
	};

	/* @brief One finding. */
	struct GatewayProblem
	{
		GatewaySeverity severity = GatewaySeverity::Error;
		std::string code;			// Stable short id ("name-too-long", "boundary-missing", ...): tests and filters
		std::string message;		// For the artist
	};

	/* @brief Everything ValidateGatewayAirport found. */
	struct GatewayValidation
	{
		std::vector<GatewayProblem> problems;

		[[nodiscard]] size_t CountErrors() const;
		[[nodiscard]] size_t CountWarnings() const;
		[[nodiscard]] bool HasErrors() const { return CountErrors() > 0; }

		/* @brief True when a problem with this code was found. */
		[[nodiscard]] bool Has(std::string_view code) const;

		void Add(GatewaySeverity severity, std::string code, std::string message);
	};

	/* @brief How X-Plane's library sees an art path. */
	struct GatewayResourceInfo
	{
		bool known = false;					// Some library (or the pack) exports it
		bool defaultLibrary = false;		// X-Plane's own library (Resources/default scenery) exports it
		bool privateOrDeprecated = false;	// No public export of it: PRIVATE or DEPRECATED (SEMI_DEPRECATED is public)
	};

	struct GatewayValidationOptions
	{
		/* @brief How the library sees an art path. Null: the art can't be checked, which is an error. */
		std::function<GatewayResourceInfo(std::string_view path)> describeResource;

		/* @brief Automatic subtexture boxes (#subtex) of an orthophoto's .pol; -1 when the .pol can't be read. Null: unknown. */
		std::function<int(std::string_view path)> countSubtextures;
	};

	/**
	 * @brief Checks an airport and its overlay for a Gateway upload.
	 * @param overlay The airport's DSF placements (definitions, objects, polygons, roads); its filters are ignored.
	 */
	GatewayValidation ValidateGatewayAirport(const AptAirport &airport, const DsfFile &overlay, const GatewayValidationOptions &options);

	/**
	 * @brief How a library index sees an art path: default when a package in Resources/default scenery exports it,
	 * private / deprecated when none of those exports is public. "::FLATTEN::.pol" is always default.
	 */
	GatewayResourceInfo DescribeGatewayResource(const LibraryIndex &index, std::string_view path);

	/* @brief True for an upper case ISO-3166-1 alpha-3 country code ("USA", "DEU"). */
	bool IsIso3166Alpha3(std::string_view code);

	/* @brief The 1302 keys WorldEditor uploads as additionalMetadata ("city", "country", "datum_lat", ...), sorted. */
	const std::vector<std::string_view> &GetGatewayMetadataKeys();

	/* @brief The roads the Gateway takes: "lib/g10/roads.net" and "lib/g10/roads_EU.net". */
	bool IsGatewayRoadNetwork(std::string_view path);

	inline constexpr double GATEWAY_MAX_SPAN_NM = 7.0;			// Widest airport, either direction
	inline constexpr double GATEWAY_BOUNDARY_MARGIN_NM = 0.6;	// Farthest anything may sit past the boundary
	inline constexpr double GATEWAY_MIN_BOUNDARY_SIDE_M = 30.0;	// Shortest boundary side
}

// -------------------------------------------------------

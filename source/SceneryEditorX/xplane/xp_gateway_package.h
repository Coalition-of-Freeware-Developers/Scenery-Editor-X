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
 * xp_gateway_package.h
 * -------------------------------------------------------
 * Created: 24/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "xp_apt.h"
#include "xp_dsf.h"
#include "xp_gateway_submission.h"
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	/**
	 * Packing an airport for a Scenery Gateway upload the way WorldEditor's Gateway export does (WED 2.7,
	 * WED_GatewayExport.cpp): the latest apt.dat as <ICAO>.dat, the airport's DSF content as DSF2Text in <ICAO>.txt, and
	 * <ICAO>_Scenery_Pack.zip with the installable pack (Earth nav data/apt.dat, the binary DSF tiles, README.txt naming
	 * the uploader with the GPL v2 notice, COPYING), all deflated into the masterZipBlob. The upload's type (2D / 3D) and
	 * features come from the content; gui_label is set to match, as WorldEditor does before it validates.
	 *
	 * The airport's content is an AptAirport plus a DsfFile of its placements, from a project (ProjectFile's
	 * CollectGatewayContent) or a scenery pack (LoadPackOverlay). Standard library plus zlib (deflate).
	 */

	/* @brief The Gateway's feature ids (the upload's "features"). */
	enum GatewayFeature : int
	{
		GatewayFeatureAtcFlow = 1,			// Rows 1000 + (ATC flows)
		GatewayFeatureTaxiRoutes = 2,		// Rows 1202 (aircraft taxi routes)
		GatewayFeatureTruckRoutes = 8,		// Rows 1206 (ground vehicle routes)
		GatewayFeatureRoads = 86			// DSF road networks
	};

	/* @brief What an airport has, for the upload's type and features and for the submit dialog. */
	struct GatewayContentSummary
	{
		bool is3D = false;					// Objects, facades, forests, object strings or jetways: things that stick up
		bool hasDsf = false;				// Anything for the DSF (placements or exclusions)
		bool atcFlows = false;
		bool taxiRoutes = false;
		bool truckRoutes = false;
		bool roads = false;

		size_t runways = 0, waterRunways = 0, helipads = 0, pavements = 0, lines = 0, startups = 0, jetways = 0;
		size_t objects = 0;					// .obj
		size_t autogenPoints = 0;			// .agp
		size_t facades = 0, forests = 0, strings = 0, linePlacements = 0, polygons = 0, orthophotos = 0, autogen = 0;
		size_t roadSegments = 0, exclusions = 0;

		/* @brief The upload's features: "1,2,86", "" for none. */
		[[nodiscard]] std::string GetFeatures() const;

		/* @brief "ATC flows", "Taxi routes", "Truck routes", "Roads" for the features present. */
		[[nodiscard]] std::vector<std::string> GetFeatureNames() const;
	};

	/* @brief Looks at an airport and its placements. */
	GatewayContentSummary SummarizeGatewayContent(const AptAirport &airport, const DsfFile &overlay);

	/* @brief Sets 1302 gui_label to "3D" / "2D" to match the content (WorldEditor's Enforce_MetaDataGuiLabel). */
	void ApplyGatewayGuiLabel(AptAirport &airport, bool is3D);

	/* @brief What goes into a package. */
	struct GatewayPackageInput
	{
		AptAirport airport;							// As uploaded (its gui_label is set to match the content)
		DsfFile overlay;							// The airport's placements and sim/exclude_* properties; the rest of its properties and its filters are replaced
		std::string uploader;						// README "Uploaded by" (the Gateway user name)
		std::string comments;						// The artist's comments (README and the upload)
		int parentId = 0;							// The submission this one replaces; 0: a first upload
		std::string validatedAgainst = "Unknown";	// X-Plane build (ReadXPlaneBuild)
		std::string creationAgent = "Scenery Editor X";
		int clientVersion = 0;
	};

	/* @brief A packed airport, ready for GatewayClient::UploadScenery. */
	struct GatewayPackage
	{
		GatewaySubmission submission;
		GatewayContentSummary summary;
		AptAirport airport;							// The airport as uploaded
		std::string aptDat;							// <ICAO>.dat
		std::string dsfText;						// <ICAO>.txt, empty without DSF content
		std::vector<ZipFile> packFiles;				// <ICAO>_Scenery_Pack/ contents, relative to that folder
		std::vector<std::string> warnings;			// Nothing failed, but worth knowing (polygons crossing tiles, ...)
	};

	/**
	 * @brief Packs an airport.
	 * @return False (with the reason) when the airport ID can't name the files, or a DSF tile can't be encoded.
	 */
	bool BuildGatewayPackage(const GatewayPackageInput &input, GatewayPackage &out, std::string *error = nullptr);

	/* @brief The pack's README.txt: airport, uploader, the artist's comments, how to install, the GPL v2 notice. */
	std::string MakeGatewayReadme(std::string_view airportName, std::string_view icao, std::string_view uploader, std::string_view comments);

	/* @brief The GNU General Public License version 2, the pack's COPYING file. */
	const std::string &GetGatewayLicenseText();

	/**
	 * @brief X-Plane's build as WorldEditor reads it for validatedAgainst: the word after "X-Plane " near the start of
	 * Log.txt ("Log.txt for X-Plane 12.4.4-b5-3a0f7e10 (build ..." -> "12.4.4-b5-3a0f7e10"). Empty when there is none.
	 */
	std::string ReadXPlaneBuild(std::string_view logText);

	/* @brief ReadXPlaneBuild on <X-Plane>/Log.txt (the last run's log). Empty when it can't be read. */
	std::string LoadXPlaneBuild(const std::filesystem::path &xplaneRoot);

	/**
	 * @brief Adds another DSF's placements (objects, polygons, roads) to a DSF, renumbering definitions and dropping
	 * filters. Road junction ids are offset so the two networks stay apart.
	 */
	void AppendDsfPlacements(DsfFile &into, const DsfFile &from);

	/* @brief AppendDsfPlacements plus the sim/exclude_* properties `into` doesn't have yet. */
	void AppendDsf(DsfFile &into, const DsfFile &from);

	/**
	 * @brief The placements of one DSF tile that belong to an airport: in a tile with sim/filter/aptid properties, those
	 * filtered to this airport's ID (and the exclusions that follow its aptid); in a tile without any, the placements
	 * within 0.6 nm of its boundary (or of its rows' extent) and every exclusion. Definitions keep their indices.
	 * @param leftOut Incremented by the placements that were left out.
	 */
	DsfFile SelectAirportPlacements(const DsfFile &tile, const AptAirport &airport, size_t *leftOut = nullptr);

	/**
	 * @brief The placements a scenery pack's DSF tiles (Earth nav data/...) hold for one airport (SelectAirportPlacements
	 * on every tile it reaches), merged into one DSF.
	 * @param warnings Receives tiles that couldn't be read and placements left out.
	 */
	DsfFile LoadPackOverlay(const std::filesystem::path &packFolder, const AptAirport &airport, std::vector<std::string> *warnings = nullptr);
}

// -------------------------------------------------------

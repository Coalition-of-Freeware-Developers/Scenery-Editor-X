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
 * project_gateway.h
 * -------------------------------------------------------
 * Created: 24/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "scenery_project.h"
#include <SceneryEditorX/xplane/xp_dsf.h>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::ProjectFile
{
	/**
	 * A project and the X-Plane Scenery Gateway: what an airport of the project uploads (its apt.dat rows and the items
	 * in its hierarchy, as a DSF), the submission each airport descends from on each server, and bringing a Gateway
	 * submission into a project. Standard library plus zlib (the submission's zip archives).
	 *
	 * Scenery ids: ProjectAirport::sceneryId is the public Gateway's (WorldEditor's scenery_id); ids from test servers
	 * live in ProjectAirport::testSceneryIds, so a test upload never becomes a real upload's parent.
	 */

	/* @brief An airport of the project as it would go to the Gateway. */
	struct GatewayContent
	{
		XPlane::AptAirport airport;
		XPlane::DsfFile overlay;					// The visible items in the airport's hierarchy (objects included)
		std::vector<std::string> warnings;			// Items the DSF conversion left out (too few nodes, wrong resource kind)
		std::vector<std::string> outside;			// Visible items outside every airport's hierarchy: not uploaded
		uint32_t hidden = 0;						// Hidden items in the airport's hierarchy: not uploaded
	};

	/**
	 * @brief Collects an airport's apt.dat rows and the items in its hierarchy (the Airport item's children), the way
	 * WorldEditor exports one airport to the Gateway. False when the project has no such airport or its item is hidden.
	 */
	bool CollectGatewayContent(const SceneryProject &project, std::string_view airportId, GatewayContent &out, std::string *error = nullptr);

	/**
	 * @brief The submission an airport descends from on a server (the next upload's parent), 0 when none.
	 * @param serverKey XPlane::GatewayServer::GetKey() of a test server; ignored for the public Gateway.
	 */
	int GetGatewaySceneryId(const ProjectAirport &airport, bool officialServer, std::string_view serverKey);

	/* @brief Records the submission an airport now descends from on a server (after an import or an accepted upload). */
	void SetGatewaySceneryId(ProjectAirport &airport, bool officialServer, std::string_view serverKey, int sceneryId);

	/* @brief What ImportGatewaySubmission did. */
	struct GatewayImportReport
	{
		std::string airportId;
		bool replaced = false;						// An airport with that ID was in the project: its rows and items were replaced
		size_t items = 0;							// Items made from the pack's DSF
		std::vector<std::string> warnings;
	};

	/**
	 * @brief Brings a Gateway submission (its masterZipBlob, decoded) into a project: the airport from <ICAO>.dat, and
	 * the placements of its pack's DSF tiles as items in the airport's hierarchy. An airport with the same ID is
	 * replaced (rows and items); otherwise the airport is added at the top level. The scenery id is recorded for the
	 * server it came from.
	 */
	bool ImportGatewaySubmission(SceneryProject &project, const std::vector<uint8_t> &masterZip, int sceneryId, bool officialServer, std::string_view serverKey,
		GatewayImportReport &report, std::string *error = nullptr);
}

// -------------------------------------------------------

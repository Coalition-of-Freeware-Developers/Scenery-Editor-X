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
 * xp_gateway_types.h
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	/**
	 * X-Plane Scenery Gateway API records, per https://gateway.x-plane.com/api (apiv1). The parsers take the raw
	 * JSON responses so they can be unit tested without the network; the client in xp_gateway.h fetches them.
	 */

	inline constexpr const char *GATEWAY_URL = "https://gateway.x-plane.com";
	inline constexpr const char *GATEWAY_API_URL = "https://gateway.x-plane.com/apiv1";

	/* @brief One row of GET /apiv1/airports. */
	struct GatewayAirport
	{
		std::string code;					// "KSEA", "5S4", "XEN0026"
		std::string name;
		double latitude = 0.0;
		double longitude = 0.0;
		int elevationFt = 0;
		bool hasElevation = false;
		int acceptedSceneryCount = 0;
		int approvedSceneryCount = 0;
		int recommendedSceneryId = 0;		// 0 when none
		std::string status;					// "Scenery Submitted", "Awaiting Scenery"
		int sceneryType = 0;				// 0 none; higher values have 2D / 3D submissions
		int submissionCount = 0;
		std::string checkedOutBy;			// Artist who locked the airport, empty when free
		bool deprecated = false;
		std::string deprecatedInFavorOf;

		[[nodiscard]] bool HasScenery() const { return recommendedSceneryId != 0 || approvedSceneryCount > 0 || acceptedSceneryCount > 0; }
	};

	/* @brief A submission listed under an airport, or the header of a downloaded pack. */
	struct GatewayScenery
	{
		int sceneryId = 0;
		int parentId = 0;					// Submission this one was based on, 0 for none
		int userId = 0;
		std::string userName;
		std::string dateUploaded;			// ISO 8601 ("2024-01-26T22:06:30.000Z")
		std::string dateAccepted;
		std::string dateApproved;
		std::string dateDeclined;
		std::string type;					// "2D" or "3D"
		std::string features;
		std::string artistComments;
		std::string moderatorComments;		// Semicolon separated
		std::string status;					// "Uploaded", "Accepted", "Approved", "Declined", "Unknown"

		[[nodiscard]] bool Is3D() const { return type == "3D"; }
		[[nodiscard]] bool IsApproved() const { return status == "Approved"; }
		[[nodiscard]] bool IsDeclined() const { return status == "Declined"; }

		/* @brief "2024-01-26" from an ISO date, or the raw text. */
		static std::string ShortDate(const std::string &isoDate);
	};

	/* @brief GET /apiv1/airport/{code}. */
	struct GatewayAirportDetails
	{
		GatewayAirport airport;
		std::vector<GatewayScenery> scenery;	// Oldest first, as the Gateway lists them

		/* @brief The recommended submission, or nullptr. */
		[[nodiscard]] const GatewayScenery *GetRecommended() const;
	};

	/* @brief GET /apiv1/scenery/{id}: a submission with its files. */
	struct GatewaySceneryPack
	{
		GatewayScenery info;
		std::string icao;
		std::string airportName;
		std::map<std::string, std::string> metadata;	// additionalMetadata: the 1302 rows
		std::vector<uint8_t> masterZip;					// Decoded masterZipBlob: <ICAO>.dat, <ICAO>.txt (DSF text) and <ICAO>_Scenery_Pack.zip
	};

	/* @brief GET /apiv1/stats. */
	struct GatewayStats
	{
		int airports = 0;
		int userSceneryPacks = 0;
		int sceneryPacks3D = 0;
		int artists = 0;
	};

	/* @brief GET /apiv1/releases. */
	struct GatewayRelease
	{
		std::string version;				// "12.1.0"
		std::string date;
	};

	// -------------------------------------------------------

	bool ParseGatewayAirports(std::string_view json, std::vector<GatewayAirport> &out, std::string *error = nullptr);
	bool ParseGatewayAirport(std::string_view json, GatewayAirportDetails &out, std::string *error = nullptr);
	bool ParseGatewayScenery(std::string_view json, GatewaySceneryPack &out, std::string *error = nullptr);
	bool ParseGatewayStats(std::string_view json, GatewayStats &out, std::string *error = nullptr);
	bool ParseGatewayReleases(std::string_view json, std::vector<GatewayRelease> &out, std::string *error = nullptr);

	/* @brief Standard base64 (with or without padding, whitespace ignored). False on a bad character. */
	bool DecodeBase64(std::string_view text, std::vector<uint8_t> &out);

	/* @brief Standard base64 with '=' padding and no line breaks (what the Gateway's masterZipBlob takes). */
	std::string EncodeBase64(const std::vector<uint8_t> &bytes);

	/* @brief Web page of an airport / submission on the Gateway. */
	std::string GetGatewayAirportPageUrl(std::string_view code);
	std::string GetGatewaySceneryPageUrl(int sceneryId);

	/* @brief "KSEA - Seattle Tacoma Intl" style folder name, with characters a file system rejects replaced. */
	std::string MakeGatewayPackageName(std::string_view code, std::string_view airportName);
}

// -------------------------------------------------------

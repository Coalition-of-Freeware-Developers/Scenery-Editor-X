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
 * xp_gateway_submission.h
 * -------------------------------------------------------
 * Created: 24/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "xp_gateway_server.h"
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	/**
	 * Uploading scenery to the Gateway, the way WorldEditor (WED 2.7) does it: PUT <api>/scenery with
	 * {"scenery": {userId, password, icao, aptName, ..., masterZipBlob}}. A new airport and an update differ only in
	 * parentId. Only HTTP 200 with an integer sceneryId counts as uploaded. Standard library + nlohmann/json only.
	 *
	 * Laminar's API docs warn that accounts uploading through the API without WED's validation may be banned, so
	 * uploads to the public Gateway stay off (CanUploadTo) until Scenery Editor X validates like WED does and Laminar
	 * agrees; test servers take them.
	 */

	/* @brief One upload: an airport's files plus what the Gateway lists about it. */
	struct GatewaySubmission
	{
		std::string icao;								// The apt.dat airport ID (header ident), not the icao_code metadata
		std::string airportName;
		std::string artistComments;
		int parentId = 0;								// The submission this one updates (the airport's scenery id); 0: first upload
		bool is3D = false;								// Has objects, facades, forests or object strings
		std::string features;							// Comma-separated Gateway feature ids ("1,2"), "" for none
		std::string validatedAgainst = "Unknown";		// The X-Plane build it was checked in ("12.1.3-r2-2f236")
		double latitude = 0.0;							// Center of the airport's bounds
		double longitude = 0.0;
		std::map<std::string, std::string> metadata;	// The 1302 rows (additionalMetadata)
		std::vector<uint8_t> masterZip;					// <ICAO>.dat, <ICAO>.txt (DSF text, when any), <ICAO>_Scenery_Pack.zip
		int clientVersion = 0;							// Sent when non-zero (WED sends its own version number)
	};

	/* @brief How an upload went. */
	struct GatewayUploadResult
	{
		enum class Outcome : uint8_t
		{
			Uploaded,		// HTTP 200 with a sceneryId: waiting for a moderator
			Unauthorized,	// HTTP 401/403: wrong user name or password, or an X-Plane build the Gateway doesn't take
			UnknownAirport,	// HTTP 404: the Gateway has no airport with that code
			Rejected,		// Another HTTP error
			Unconfirmed,	// HTTP 200 without a sceneryId (or not JSON): it may or may not have arrived
			NoAnswer		// No connection, timed out, cancelled
		};

		Outcome outcome = Outcome::NoAnswer;
		int sceneryId = 0;			// When Uploaded: the new submission (the next upload's parentId)
		long httpStatus = 0;
		std::string message;		// The server's text (printable characters only) or the transport error

		[[nodiscard]] bool Succeeded() const { return outcome == Outcome::Uploaded; }
	};

	const char *GetGatewayUploadOutcomeName(GatewayUploadResult::Outcome outcome);

	/**
	 * @brief False (with the reason) when an upload must not go to this server: the public Gateway takes uploads
	 * only when `liveUploadsAllowed` (off everywhere for now).
	 */
	bool CanUploadTo(const GatewayServer &server, bool liveUploadsAllowed, std::string *reason = nullptr);

	/* @brief The upload's HTTP request. The body holds the password: never log it. */
	GatewayHttpRequest MakeGatewayUploadRequest(const GatewayServer &server, const GatewaySubmission &submission, std::string_view userName, std::string_view password);

	/* @brief Reads the server's answer to an upload. */
	GatewayUploadResult ReadGatewayUploadResponse(const GatewayHttpResponse &response);

	// -------------------------------------------------------

	/* @brief A file for a zip archive. */
	struct ZipFile
	{
		std::string name;			// Forward slashes
		std::string data;
	};

	/* @brief A zip archive of the files, stored uncompressed (no zlib needed; any unzip reads it). */
	std::vector<uint8_t> WriteStoredZip(const std::vector<ZipFile> &files);

	/* @brief CRC-32 (the zip / PNG polynomial). */
	uint32_t Crc32(std::string_view data, uint32_t crc = 0);

	/**
	 * @brief The masterZipBlob archive WED builds: <ICAO>.dat, <ICAO>.txt when there's DSF text, and
	 * <ICAO>_Scenery_Pack.zip holding the installable pack under one <ICAO>_Scenery_Pack/ folder.
	 * @param packFiles The pack's files relative to its folder ("Earth nav data/apt.dat", ...).
	 */
	std::vector<uint8_t> MakeGatewayMasterZip(std::string_view icao, std::string_view aptDat, std::string_view dsfText, const std::vector<ZipFile> &packFiles);

	/**
	 * @brief A tiny made-up airport (one runway, boundary, 1302 rows) as a submission, for checking an account and
	 * the upload path against a test server. Never meant for the public Gateway.
	 */
	GatewaySubmission MakeGatewayTestSubmission(std::string_view icao = "XSDX");
}

// -------------------------------------------------------

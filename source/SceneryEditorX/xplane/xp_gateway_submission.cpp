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
 * xp_gateway_submission.cpp
 * -------------------------------------------------------
 * Created: 24/10/2026
 * -------------------------------------------------------
 */
#include "xp_gateway_submission.h"
#include "xp_gateway_types.h"
#include <array>
#include <cstdio>
#include <nlohmann/json.hpp>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	namespace
	{
		using Json = nlohmann::json;

		constexpr size_t MAX_MESSAGE_LENGTH = 400;

		/* @brief The server's text as one readable line: tags dropped, control characters and whitespace runs folded. */
		std::string CleanMessage(const std::string_view body)
		{
			std::string out;
			bool inTag = false;
			bool space = false;
			for (const char c : body)
			{
				if (c == '<') { inTag = true; continue; }
				if (c == '>' && inTag) { inTag = false; space = !out.empty(); continue; }
				if (inTag)
					continue;
				const unsigned char u = static_cast<unsigned char>(c);
				if (u < 32 || u == 127 || c == ' ')
				{
					space = !out.empty();
					continue;
				}
				if (space)
					out.push_back(' ');
				space = false;
				out.push_back(c);
				if (out.size() >= MAX_MESSAGE_LENGTH)
				{
					out += "...";
					break;
				}
			}
			return out;
		}

		/* @brief "message" / "error" from a JSON error body, else the cleaned text. */
		std::string ServerMessage(const std::string &body)
		{
			try
			{
				const Json root = Json::parse(body);
				for (const char *key : { "message", "error", "errors" })
				{
					const auto it = root.find(key);
					if (it == root.end())
						continue;
					if (it->is_string())
						return CleanMessage(it->get<std::string>());
					return CleanMessage(it->dump());
				}
			}
			catch (...)
			{
			}
			return CleanMessage(body);
		}

		std::string Format(const char *format, const double value)
		{
			char buffer[64];
			snprintf(buffer, sizeof(buffer), format, value);
			return buffer;
		}
	}

	// -------------------------------------------------------

	const char *GetGatewayUploadOutcomeName(const GatewayUploadResult::Outcome outcome)
	{
		switch (outcome)
		{
			case GatewayUploadResult::Outcome::Uploaded:		return "Uploaded";
			case GatewayUploadResult::Outcome::Unauthorized:	return "Not authorized";
			case GatewayUploadResult::Outcome::UnknownAirport:	return "Unknown airport";
			case GatewayUploadResult::Outcome::Rejected:		return "Rejected";
			case GatewayUploadResult::Outcome::Unconfirmed:		return "Not confirmed";
			case GatewayUploadResult::Outcome::NoAnswer:		return "No answer";
		}
		return "Unknown";
	}

	bool CanUploadTo(const GatewayServer &server, const bool liveUploadsAllowed, std::string *reason)
	{
		if (server.apiUrl.empty())
		{
			if (reason)
				*reason = "No Gateway server is set";
			return false;
		}
		if (server.official && !liveUploadsAllowed)
		{
			if (reason)
				*reason = "Uploading to the public X-Plane Scenery Gateway is switched off in this version of Scenery Editor X. "
					"Use a test server (Edit > Preferences > Scenery Gateway), or WorldEditor for real submissions.";
			return false;
		}
		return true;
	}

	GatewayHttpRequest MakeGatewayUploadRequest(const GatewayServer &server, const GatewaySubmission &submission, const std::string_view userName, const std::string_view password)
	{
		Json scenery;
		scenery["userId"] = std::string(userName);				// The artist's user name, despite the key
		scenery["password"] = std::string(password);
		scenery["icao"] = submission.icao;
		scenery["aptName"] = submission.airportName;
		scenery["artistComments"] = submission.artistComments;
		scenery["parentId"] = submission.parentId > 0 ? Json(submission.parentId) : Json(nullptr);
		scenery["type"] = submission.is3D ? "3D" : "2D";
		scenery["features"] = submission.features;
		scenery["validatedAgainst"] = submission.validatedAgainst.empty() ? std::string("Unknown") : submission.validatedAgainst;
		scenery["latitude"] = submission.latitude;
		scenery["longitude"] = submission.longitude;
		Json metadata = Json::object();
		for (const auto &[key, value] : submission.metadata)
			metadata[key] = value;
		scenery["additionalMetadata"] = std::move(metadata);
		if (submission.clientVersion != 0)
			scenery["clientVersion"] = submission.clientVersion;
		scenery["masterZipBlob"] = EncodeBase64(submission.masterZip);

		Json root;
		root["scenery"] = std::move(scenery);

		GatewayHttpRequest request;
		request.method = "PUT";
		request.url = server.apiUrl + "/scenery";
		request.headers.emplace_back("Content-Type", "application/json");
		request.headers.emplace_back("Accept", "application/json");
		request.body = root.dump();
		request.sendsCredentials = true;
		return request;
	}

	GatewayUploadResult ReadGatewayUploadResponse(const GatewayHttpResponse &response)
	{
		using Outcome = GatewayUploadResult::Outcome;
		GatewayUploadResult result;
		result.httpStatus = response.status;
		if (!response.completed)
		{
			result.outcome = Outcome::NoAnswer;
			result.message = response.error.empty() ? std::string("No answer from the server") : response.error;
			return result;
		}

		if (response.status == 200)
		{
			try
			{
				const Json root = Json::parse(response.body);
				const Json *object = &root;
				if (const auto it = root.find("scenery"); it != root.end() && it->is_object())
					object = &*it;
				const auto id = object->find("sceneryId");
				if (id != object->end() && id->is_number_integer() && id->get<long long>() > 0)
				{
					result.outcome = Outcome::Uploaded;
					result.sceneryId = static_cast<int>(id->get<long long>());
					result.message = "Uploaded as scenery " + std::to_string(result.sceneryId) + "; it shows on the Gateway once a moderator accepts it";
					return result;
				}
				result.outcome = Outcome::Unconfirmed;
				result.message = "The server answered without a scenery id: the upload may not have arrived";
			}
			catch (...)
			{
				result.outcome = Outcome::Unconfirmed;
				result.message = "The server's answer wasn't JSON: the upload may not have arrived";
			}
			return result;
		}

		const std::string text = ServerMessage(response.body);
		const std::string status = "HTTP " + std::to_string(response.status);
		if (response.status == 401 || response.status == 403)
			result.outcome = Outcome::Unauthorized;
		else if (response.status == 404)
			result.outcome = Outcome::UnknownAirport;
		else
			result.outcome = Outcome::Rejected;
		result.message = text.empty() ? status : text + " (" + status + ")";
		return result;
	}

	// -------------------------------------------------------

	uint32_t Crc32(const std::string_view data, uint32_t crc)
	{
		static const std::array<uint32_t, 256> table = []
		{
			std::array<uint32_t, 256> values{};
			for (uint32_t n = 0; n < 256; ++n)
			{
				uint32_t c = n;
				for (int k = 0; k < 8; ++k)
					c = (c & 1u) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
				values[n] = c;
			}
			return values;
		}();

		crc = ~crc;
		for (const char ch : data)
			crc = table[(crc ^ static_cast<uint8_t>(ch)) & 0xFFu] ^ (crc >> 8);
		return ~crc;
	}

	std::vector<uint8_t> WriteStoredZip(const std::vector<ZipFile> &files)
	{
		return WriteZipArchive(files, ZipMethod::Stored);
	}

	std::vector<uint8_t> MakeGatewayMasterZip(const std::string_view icao, const std::string_view aptDat, const std::string_view dsfText, const std::vector<ZipFile> &packFiles,
		const ZipMethod method)
	{
		const std::string id(icao);
		const std::string folder = id + "_Scenery_Pack/";
		std::vector<ZipFile> pack;
		pack.reserve(packFiles.size());
		for (const ZipFile &file : packFiles)
			pack.push_back({ folder + file.name, file.data });
		const std::vector<uint8_t> packZip = WriteZipArchive(pack, method);

		std::vector<ZipFile> master;
		master.push_back({ id + ".dat", std::string(aptDat) });
		if (!dsfText.empty())
			master.push_back({ id + ".txt", std::string(dsfText) });
		master.push_back({ id + "_Scenery_Pack.zip", std::string(packZip.begin(), packZip.end()) });
		return WriteZipArchive(master, method);
	}

	GatewaySubmission MakeGatewayTestSubmission(const std::string_view icao)
	{
		constexpr double LAT = 45.0;
		constexpr double LON = -100.0;
		constexpr double LAT_END = LAT + 0.009;
		const std::string id(icao);

		const auto f8 = [](const double value) { return Format("%.8f", value); };
		std::string apt;
		apt += "I\n1200 Scenery Editor X Gateway test upload - not real data\n\n";
		apt += "1 1500 0 0 " + id + " Scenery Editor X Test Field\n";
		apt += "1302 city Mockville\n1302 country Testland\n";
		apt += "1302 datum_lat " + f8(LAT) + "\n1302 datum_lon " + f8(LON) + "\n";
		apt += "1302 gui_label 2D\n1302 icao_code " + id + "\n";
		apt += "100 30.00 1 0 0.25 0 2 0 18 " + f8(LAT) + " " + f8(LON) + " 0 0 2 0 0 0 36 " + f8(LAT_END) + " " + f8(LON) + " 0 0 2 0 0 0\n";
		apt += "130 Airport Boundary\n";
		apt += "111 " + f8(LAT - 0.002) + " " + f8(LON - 0.002) + "\n";
		apt += "111 " + f8(LAT - 0.002) + " " + f8(LON + 0.002) + "\n";
		apt += "111 " + f8(LAT_END + 0.002) + " " + f8(LON + 0.002) + "\n";
		apt += "113 " + f8(LAT_END + 0.002) + " " + f8(LON - 0.002) + "\n";
		apt += "99\n";

		GatewaySubmission submission;
		submission.icao = id;
		submission.airportName = "Scenery Editor X Test Field";
		submission.artistComments = "Scenery Editor X test upload (made-up airport for checking the account and upload path)";
		submission.is3D = false;
		submission.latitude = (LAT + LAT_END) * 0.5;
		submission.longitude = LON;
		submission.metadata = { { "city", "Mockville" }, { "country", "Testland" }, { "datum_lat", f8(LAT) }, { "datum_lon", f8(LON) }, { "gui_label", "2D" }, { "icao_code", id } };
		submission.masterZip = MakeGatewayMasterZip(id, apt, std::string_view(), {
			{ "Earth nav data/apt.dat", apt },
			{ "README.txt", "Made-up airport uploaded by Scenery Editor X to check a Gateway test server.\n" },
		});
		return submission;
	}
}

// -------------------------------------------------------

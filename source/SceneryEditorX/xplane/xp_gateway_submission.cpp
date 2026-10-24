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

		void PutU16(std::vector<uint8_t> &out, const uint32_t value)
		{
			out.push_back(static_cast<uint8_t>(value & 0xFF));
			out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
		}

		void PutU32(std::vector<uint8_t> &out, const uint32_t value)
		{
			for (int shift = 0; shift < 32; shift += 8)
				out.push_back(static_cast<uint8_t>((value >> shift) & 0xFF));
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
		// 1980-01-01 00:00: fixed, so the same files give the same bytes
		constexpr uint32_t DOS_TIME = 0;
		constexpr uint32_t DOS_DATE = (0u << 9) | (1u << 5) | 1u;
		constexpr uint32_t UTF8_NAMES = 1u << 11;

		std::vector<uint8_t> out;
		std::vector<uint8_t> directory;
		for (const ZipFile &file : files)
		{
			const uint32_t crc = Crc32(file.data);
			const uint32_t size = static_cast<uint32_t>(file.data.size());
			const uint32_t offset = static_cast<uint32_t>(out.size());

			PutU32(out, 0x04034B50u);		// Local file header
			PutU16(out, 20);				// Version needed: 2.0
			PutU16(out, UTF8_NAMES);
			PutU16(out, 0);					// Stored
			PutU16(out, DOS_TIME);
			PutU16(out, DOS_DATE);
			PutU32(out, crc);
			PutU32(out, size);
			PutU32(out, size);
			PutU16(out, static_cast<uint32_t>(file.name.size()));
			PutU16(out, 0);
			out.insert(out.end(), file.name.begin(), file.name.end());
			out.insert(out.end(), file.data.begin(), file.data.end());

			PutU32(directory, 0x02014B50u);	// Central directory header
			PutU16(directory, (3u << 8) | 20u);	// Made by: Unix, 2.0
			PutU16(directory, 20);
			PutU16(directory, UTF8_NAMES);
			PutU16(directory, 0);
			PutU16(directory, DOS_TIME);
			PutU16(directory, DOS_DATE);
			PutU32(directory, crc);
			PutU32(directory, size);
			PutU32(directory, size);
			PutU16(directory, static_cast<uint32_t>(file.name.size()));
			PutU16(directory, 0);			// Extra
			PutU16(directory, 0);			// Comment
			PutU16(directory, 0);			// Disk
			PutU16(directory, 0);			// Internal attributes
			PutU32(directory, 0100644u << 16);	// External attributes: a regular file, rw-r--r--
			PutU32(directory, offset);
			directory.insert(directory.end(), file.name.begin(), file.name.end());
		}

		const uint32_t directoryOffset = static_cast<uint32_t>(out.size());
		out.insert(out.end(), directory.begin(), directory.end());
		PutU32(out, 0x06054B50u);			// End of central directory
		PutU16(out, 0);
		PutU16(out, 0);
		PutU16(out, static_cast<uint32_t>(files.size()));
		PutU16(out, static_cast<uint32_t>(files.size()));
		PutU32(out, static_cast<uint32_t>(directory.size()));
		PutU32(out, directoryOffset);
		PutU16(out, 0);
		return out;
	}

	std::vector<uint8_t> MakeGatewayMasterZip(const std::string_view icao, const std::string_view aptDat, const std::string_view dsfText, const std::vector<ZipFile> &packFiles)
	{
		const std::string id(icao);
		const std::string folder = id + "_Scenery_Pack/";
		std::vector<ZipFile> pack;
		pack.reserve(packFiles.size());
		for (const ZipFile &file : packFiles)
			pack.push_back({ folder + file.name, file.data });
		const std::vector<uint8_t> packZip = WriteStoredZip(pack);

		std::vector<ZipFile> master;
		master.push_back({ id + ".dat", std::string(aptDat) });
		if (!dsfText.empty())
			master.push_back({ id + ".txt", std::string(dsfText) });
		master.push_back({ id + "_Scenery_Pack.zip", std::string(packZip.begin(), packZip.end()) });
		return WriteStoredZip(master);
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

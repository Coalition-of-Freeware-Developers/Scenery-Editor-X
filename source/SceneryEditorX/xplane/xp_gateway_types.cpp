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
 * xp_gateway_types.cpp
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#include "xp_gateway_types.h"
#include <nlohmann/json.hpp>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	namespace
	{
		using Json = nlohmann::json;

		/* @brief Field lookup that tolerates the Gateway's mixed casing ("AirportCode" in lists, "icao" in details). */
		const Json *Field(const Json &object, std::initializer_list<const char *> names)
		{
			if (!object.is_object())
				return nullptr;
			for (const char *name : names)
			{
				const auto it = object.find(name);
				if (it != object.end() && !it->is_null())
					return &*it;
			}
			return nullptr;
		}

		std::string String(const Json &object, std::initializer_list<const char *> names)
		{
			const Json *value = Field(object, names);
			if (!value)
				return {};
			if (value->is_string())
				return value->get<std::string>();
			if (value->is_number_integer())
				return std::to_string(value->get<long long>());
			if (value->is_number())
				return std::to_string(value->get<double>());
			if (value->is_boolean())
				return value->get<bool>() ? "1" : "0";
			return {};
		}

		int Int(const Json &object, std::initializer_list<const char *> names, const int fallback = 0)
		{
			const Json *value = Field(object, names);
			if (!value)
				return fallback;
			if (value->is_number())
				return static_cast<int>(value->get<double>());
			if (value->is_string())
			{
				try { return std::stoi(value->get<std::string>()); }
				catch (...) { return fallback; }
			}
			if (value->is_boolean())
				return value->get<bool>() ? 1 : 0;
			return fallback;
		}

		double Double(const Json &object, std::initializer_list<const char *> names, const double fallback = 0.0)
		{
			const Json *value = Field(object, names);
			if (!value)
				return fallback;
			if (value->is_number())
				return value->get<double>();
			if (value->is_string())
			{
				try { return std::stod(value->get<std::string>()); }
				catch (...) { return fallback; }
			}
			return fallback;
		}

		bool Parse(const std::string_view text, Json &out, std::string *error)
		{
			try
			{
				out = Json::parse(text.begin(), text.end());
				return true;
			}
			catch (const std::exception &e)
			{
				if (error)
					*error = std::string("Invalid JSON from the Gateway: ") + e.what();
				return false;
			}
		}

		GatewayAirport ReadAirport(const Json &json)
		{
			GatewayAirport airport;
			airport.code = String(json, { "AirportCode", "icao", "airportCode" });
			airport.name = String(json, { "AirportName", "airportName" });
			airport.latitude = Double(json, { "Latitude", "latitude" });
			airport.longitude = Double(json, { "Longitude", "longitude" });
			airport.hasElevation = Field(json, { "Elevation", "elevation" }) != nullptr;
			airport.elevationFt = Int(json, { "Elevation", "elevation" });
			airport.acceptedSceneryCount = Int(json, { "AcceptedSceneryCount", "acceptedSceneryCount" });
			airport.approvedSceneryCount = Int(json, { "ApprovedSceneryCount", "approvedSceneryCount" });
			airport.recommendedSceneryId = Int(json, { "RecommendedSceneryId", "recommendedSceneryId" });
			airport.status = String(json, { "Status", "status" });
			airport.sceneryType = Int(json, { "SceneryType", "sceneryType" });
			airport.submissionCount = Int(json, { "SubmissionCount", "submissionCount" });
			airport.checkedOutBy = String(json, { "checkedOutBy", "CheckedOutBy" });
			airport.deprecated = Int(json, { "Deprecated", "deprecated" }) != 0;
			airport.deprecatedInFavorOf = String(json, { "DeprecatedInFavorOf", "deprecatedInFavorOf" });
			return airport;
		}

		GatewayScenery ReadScenery(const Json &json)
		{
			GatewayScenery scenery;
			scenery.sceneryId = Int(json, { "sceneryId", "SceneryId" });
			scenery.parentId = Int(json, { "parentId", "ParentId" });
			scenery.userId = Int(json, { "userId", "UserId" });
			scenery.userName = String(json, { "userName", "UserName" });
			scenery.dateUploaded = String(json, { "dateUploaded", "DateAdded", "DateUploaded" });
			scenery.dateAccepted = String(json, { "dateAccepted", "DateAccepted" });
			scenery.dateApproved = String(json, { "dateApproved", "DateApproved" });
			scenery.dateDeclined = String(json, { "dateDeclined", "DateDeclined" });
			scenery.type = String(json, { "type", "Type" });
			scenery.features = String(json, { "features", "Features" });
			scenery.artistComments = String(json, { "artistComments", "ArtistComments" });
			scenery.moderatorComments = String(json, { "moderatorComments", "ModeratorComments" });
			scenery.status = String(json, { "Status", "status" });
			if (scenery.status.empty())
			{
				// Downloaded packs carry dates only.
				if (!scenery.dateDeclined.empty()) scenery.status = "Declined";
				else if (!scenery.dateApproved.empty()) scenery.status = "Approved";
				else if (!scenery.dateAccepted.empty()) scenery.status = "Accepted";
				else if (!scenery.dateUploaded.empty()) scenery.status = "Uploaded";
				else scenery.status = "Unknown";
			}
			return scenery;
		}
	}

	// -------------------------------------------------------

	std::string GatewayScenery::ShortDate(const std::string &isoDate)
	{
		return isoDate.size() >= 10 ? isoDate.substr(0, 10) : isoDate;
	}

	const GatewayScenery *GatewayAirportDetails::GetRecommended() const
	{
		if (airport.recommendedSceneryId == 0)
			return nullptr;
		for (const GatewayScenery &entry : scenery)
		{
			if (entry.sceneryId == airport.recommendedSceneryId)
				return &entry;
		}
		return nullptr;
	}

	// -------------------------------------------------------

	bool ParseGatewayAirports(const std::string_view json, std::vector<GatewayAirport> &out, std::string *error)
	{
		out.clear();
		Json root;
		if (!Parse(json, root, error))
			return false;

		const Json *list = Field(root, { "airports" });
		if (!list)
			list = root.is_array() ? &root : nullptr;
		if (!list || !list->is_array())
		{
			if (error)
				*error = "Unexpected airport list from the Gateway (no 'airports' array)";
			return false;
		}

		out.reserve(list->size());
		for (const Json &entry : *list)
		{
			GatewayAirport airport = ReadAirport(entry);
			if (!airport.code.empty())
				out.push_back(std::move(airport));
		}
		return true;
	}

	bool ParseGatewayAirport(const std::string_view json, GatewayAirportDetails &out, std::string *error)
	{
		out = {};
		Json root;
		if (!Parse(json, root, error))
			return false;

		const Json *object = Field(root, { "airport" });
		if (!object)
			object = &root;
		if (!object->is_object() || !Field(*object, { "icao", "AirportCode", "airportCode" }))
		{
			if (error)
				*error = "Unexpected airport record from the Gateway";
			return false;
		}

		out.airport = ReadAirport(*object);
		if (const Json *scenery = Field(*object, { "scenery" }); scenery && scenery->is_array())
		{
			for (const Json &entry : *scenery)
				out.scenery.push_back(ReadScenery(entry));
		}
		return true;
	}

	bool ParseGatewayScenery(const std::string_view json, GatewaySceneryPack &out, std::string *error)
	{
		out = {};
		Json root;
		if (!Parse(json, root, error))
			return false;

		const Json *object = Field(root, { "scenery" });
		if (!object)
			object = &root;
		if (!object->is_object() || !Field(*object, { "sceneryId", "SceneryId" }))
		{
			if (error)
				*error = "Unexpected scenery record from the Gateway";
			return false;
		}

		out.info = ReadScenery(*object);
		out.icao = String(*object, { "icao", "Icao" });
		out.airportName = String(*object, { "aptName", "airportName", "AptName" });
		if (const Json *metadata = Field(*object, { "additionalMetadata" }); metadata && metadata->is_object())
		{
			for (const auto &[key, value] : metadata->items())
				out.metadata[key] = value.is_string() ? value.get<std::string>() : value.dump();
		}

		const std::string blob = String(*object, { "masterZipBlob", "MasterZipBlob" });
		if (blob.empty())
		{
			if (error)
				*error = "The scenery record has no masterZipBlob";
			return false;
		}
		if (!DecodeBase64(blob, out.masterZip))
		{
			if (error)
				*error = "The scenery pack's masterZipBlob isn't valid base64";
			return false;
		}
		return true;
	}

	bool ParseGatewayStats(const std::string_view json, GatewayStats &out, std::string *error)
	{
		out = {};
		Json root;
		if (!Parse(json, root, error))
			return false;
		const Json *object = Field(root, { "stats" });
		if (!object)
			object = &root;
		out.airports = Int(*object, { "airports" });
		out.userSceneryPacks = Int(*object, { "userSceneryPacks" });
		out.sceneryPacks3D = Int(*object, { "sceneryPacks3D" });
		out.artists = Int(*object, { "artists" });
		return out.airports > 0;
	}

	bool ParseGatewayReleases(const std::string_view json, std::vector<GatewayRelease> &out, std::string *error)
	{
		out.clear();
		Json root;
		if (!Parse(json, root, error))
			return false;
		const Json *list = Field(root, { "releases" });
		if (!list)
			list = root.is_array() ? &root : nullptr;
		if (!list || !list->is_array())
		{
			if (error)
				*error = "Unexpected release list from the Gateway";
			return false;
		}
		for (const Json &entry : *list)
		{
			GatewayRelease release;
			release.version = String(entry, { "Version", "version" });
			release.date = String(entry, { "Date", "date" });
			if (!release.version.empty())
				out.push_back(std::move(release));
		}
		return true;
	}

	// -------------------------------------------------------

	bool DecodeBase64(const std::string_view text, std::vector<uint8_t> &out)
	{
		out.clear();
		out.reserve(text.size() / 4 * 3);

		uint32_t accumulator = 0;
		int bits = 0;
		for (const char c : text)
		{
			int value;
			if (c >= 'A' && c <= 'Z') value = c - 'A';
			else if (c >= 'a' && c <= 'z') value = c - 'a' + 26;
			else if (c >= '0' && c <= '9') value = c - '0' + 52;
			else if (c == '+' || c == '-') value = 62;
			else if (c == '/' || c == '_') value = 63;
			else if (c == '=' || c == ' ' || c == '\n' || c == '\r' || c == '\t') continue;
			else return false;

			accumulator = (accumulator << 6) | static_cast<uint32_t>(value);
			bits += 6;
			if (bits >= 8)
			{
				bits -= 8;
				out.push_back(static_cast<uint8_t>((accumulator >> bits) & 0xFF));
			}
		}
		return true;
	}

	std::string EncodeBase64(const std::vector<uint8_t> &bytes)
	{
		static constexpr char ALPHABET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
		std::string out;
		out.reserve((bytes.size() + 2) / 3 * 4);
		size_t i = 0;
		for (; i + 2 < bytes.size(); i += 3)
		{
			const uint32_t triple = (static_cast<uint32_t>(bytes[i]) << 16) | (static_cast<uint32_t>(bytes[i + 1]) << 8) | bytes[i + 2];
			out.push_back(ALPHABET[(triple >> 18) & 63]);
			out.push_back(ALPHABET[(triple >> 12) & 63]);
			out.push_back(ALPHABET[(triple >> 6) & 63]);
			out.push_back(ALPHABET[triple & 63]);
		}
		if (i < bytes.size())
		{
			const bool two = i + 1 < bytes.size();
			const uint32_t triple = (static_cast<uint32_t>(bytes[i]) << 16) | (two ? static_cast<uint32_t>(bytes[i + 1]) << 8 : 0u);
			out.push_back(ALPHABET[(triple >> 18) & 63]);
			out.push_back(ALPHABET[(triple >> 12) & 63]);
			out.push_back(two ? ALPHABET[(triple >> 6) & 63] : '=');
			out.push_back('=');
		}
		return out;
	}

	std::string GetGatewayAirportPageUrl(const std::string_view code)
	{
		return std::string(GATEWAY_URL) + "/airports/" + std::string(code) + "/show";
	}

	std::string GetGatewaySceneryPageUrl(const int sceneryId)
	{
		return std::string(GATEWAY_URL) + "/scenery/page/" + std::to_string(sceneryId);
	}

	std::string MakeGatewayPackageName(const std::string_view code, const std::string_view airportName)
	{
		std::string name(code);
		if (!airportName.empty())
		{
			if (!name.empty())
				name += " - ";
			name += airportName;
		}
		for (char &c : name)
		{
			if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|' || static_cast<unsigned char>(c) < 32)
				c = '_';
		}
		// Windows rejects trailing dots and spaces; a name that was nothing but those gets a generic one.
		while (!name.empty() && (name.back() == ' ' || name.back() == '.' || name.back() == '-'))
			name.pop_back();
		while (!name.empty() && (name.front() == ' ' || name.front() == '.' || name.front() == '-'))
			name.erase(name.begin());
		return name.empty() ? std::string("Gateway Airport") : name;
	}
}

// -------------------------------------------------------

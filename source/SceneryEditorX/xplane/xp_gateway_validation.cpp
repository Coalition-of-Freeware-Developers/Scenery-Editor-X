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
 * xp_gateway_validation.cpp
 * -------------------------------------------------------
 * Created: 24/10/2026
 * -------------------------------------------------------
 */
#include "xp_gateway_validation.h"
#include "xp_library.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <map>
#include <numbers>
#include <set>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	namespace
	{
		constexpr double DEGREES_PER_NM = 1.0 / 60.0;
		constexpr double CONTROL_EPSILON = 1e-9;	// Degrees: a control point this close to its node is a corner

		/* @brief ISO-3166-1 alpha-3, as WorldEditor's list (WED_MetaDataDefaults.cpp) has them. Sorted. */
		constexpr std::array<std::string_view, 249> ISO3166_CODES = {
			"ABW", "AFG", "AGO", "AIA", "ALA", "ALB", "AND", "ARE", "ARG", "ARM", "ASM", "ATA", "ATF", "ATG", "AUS", "AUT", "AZE",
			"BDI", "BEL", "BEN", "BES", "BFA", "BGD", "BGR", "BHR", "BHS", "BIH", "BLM", "BLR", "BLZ", "BMU", "BOL", "BRA", "BRB",
			"BRN", "BTN", "BVT", "BWA", "CAF", "CAN", "CCK", "CHE", "CHL", "CHN", "CIV", "CMR", "COD", "COG", "COK", "COL", "COM",
			"CPV", "CRI", "CUB", "CUW", "CXR", "CYM", "CYP", "CZE", "DEU", "DJI", "DMA", "DNK", "DOM", "DZA", "ECU", "EGY", "ERI",
			"ESH", "ESP", "EST", "ETH", "FIN", "FJI", "FLK", "FRA", "FRO", "FSM", "GAB", "GBR", "GEO", "GGY", "GHA", "GIB", "GIN",
			"GLP", "GMB", "GNB", "GNQ", "GRC", "GRD", "GRL", "GTM", "GUF", "GUM", "GUY", "HKG", "HMD", "HND", "HRV", "HTI", "HUN",
			"IDN", "IMN", "IND", "IOT", "IRL", "IRN", "IRQ", "ISL", "ISR", "ITA", "JAM", "JEY", "JOR", "JPN", "KAZ", "KEN", "KGZ",
			"KHM", "KIR", "KNA", "KOR", "KWT", "LAO", "LBN", "LBR", "LBY", "LCA", "LIE", "LKA", "LSO", "LTU", "LUX", "LVA", "MAC",
			"MAF", "MAR", "MCO", "MDA", "MDG", "MDV", "MEX", "MHL", "MKD", "MLI", "MLT", "MMR", "MNE", "MNG", "MNP", "MOZ", "MRT",
			"MSR", "MTQ", "MUS", "MWI", "MYS", "MYT", "NAM", "NCL", "NER", "NFK", "NGA", "NIC", "NIU", "NLD", "NOR", "NPL", "NRU",
			"NZL", "OMN", "PAK", "PAN", "PCN", "PER", "PHL", "PLW", "PNG", "POL", "PRI", "PRK", "PRT", "PRY", "PSE", "PYF", "QAT",
			"REU", "ROU", "RUS", "RWA", "SAU", "SDN", "SEN", "SGP", "SGS", "SHN", "SJM", "SLB", "SLE", "SLV", "SMR", "SOM", "SPM",
			"SRB", "SSD", "STP", "SUR", "SVK", "SVN", "SWE", "SWZ", "SXM", "SYC", "SYR", "TCA", "TCD", "TGO", "THA", "TJK", "TKL",
			"TKM", "TLS", "TON", "TTO", "TUN", "TUR", "TUV", "TWN", "TZA", "UGA", "UKR", "UMI", "URY", "USA", "UZB", "VAT", "VCT",
			"VEN", "VGB", "VIR", "VNM", "VUT", "WLF", "WSM", "YEM", "ZAF", "ZMB", "ZWE" };

		/* @brief ICAO region codes WorldEditor accepts for region_code (WED_Validate.cpp). Sorted. */
		constexpr std::array<std::string_view, 251> REGION_CODES = {
			"A1", "AG", "AN", "AY", "BG", "BI", "BK", "CF", "CY", "DA", "DB", "DF", "DG", "DI", "DN", "DR", "DT", "DX", "EB", "ED",
			"EE", "EF", "EG", "EH", "EI", "EK", "EL", "EN", "EP", "ES", "ET", "EV", "EY", "FA", "FB", "FC", "FD", "FE", "FG", "FH",
			"FI", "FJ", "FK", "FL", "FM", "FN", "FO", "FP", "FQ", "FS", "FT", "FV", "FW", "FX", "FY", "FZ", "GA", "GB", "GC", "GE",
			"GF", "GG", "GL", "GM", "GO", "GQ", "GU", "GV", "HA", "HB", "HC", "HD", "HE", "HH", "HK", "HL", "HR", "HS", "HT", "HU",
			"K1", "K2", "K3", "K4", "K5", "K6", "K7", "KZ", "LA", "LB", "LC", "LD", "LE", "LF", "LG", "LH", "LI", "LJ", "LK", "LL",
			"LM", "LO", "LP", "LQ", "LR", "LS", "LT", "LU", "LW", "LX", "LY", "LZ", "MB", "MD", "MG", "MH", "MK", "MM", "MN", "MP",
			"MR", "MS", "MT", "MU", "MW", "MY", "MZ", "NC", "NF", "NG", "NI", "NL", "NS", "NT", "NV", "NW", "NZ", "OA", "OB", "OE",
			"OI", "OJ", "OK", "OL", "OM", "OO", "OP", "OR", "OS", "OT", "OY", "PA", "PC", "PG", "PH", "PK", "PL", "PM", "PT", "PW",
			"RC", "RJ", "RK", "RO", "RP", "S1", "SA", "SB", "SC", "SE", "SF", "SG", "SK", "SL", "SM", "SO", "SP", "SU", "SV", "SY",
			"TA", "TB", "TD", "TF", "TG", "TI", "TJ", "TK", "TL", "TN", "TQ", "TR", "TT", "TU", "TV", "TX", "UA", "UB", "UC", "UD",
			"UE", "UG", "UH", "UI", "UK", "UL", "UM", "UN", "UO", "UR", "US", "UT", "UU", "UW", "VA", "VC", "VD", "VE", "VG", "VH",
			"VI", "VL", "VM", "VN", "VO", "VQ", "VR", "VT", "VV", "VY", "WA", "WB", "WI", "WM", "WP", "WR", "WS", "YB", "YM", "ZB",
			"ZG", "ZH", "ZJ", "ZK", "ZL", "ZM", "ZP", "ZS", "ZU", "ZW", "ZY" };

		bool IsAsciiAlpha(const char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); }
		bool IsAsciiDigit(const char c) { return c >= '0' && c <= '9'; }
		bool IsAsciiAlnum(const char c) { return IsAsciiAlpha(c) || IsAsciiDigit(c); }
		bool IsAsciiSpace(const char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\f'; }

		std::string ToLowerAscii(std::string text)
		{
			for (char &c : text)
			{
				if (c >= 'A' && c <= 'Z')
					c = static_cast<char>(c - 'A' + 'a');
			}
			return text;
		}

		std::string ToUpperAscii(std::string text)
		{
			for (char &c : text)
			{
				if (c >= 'a' && c <= 'z')
					c = static_cast<char>(c - 'a' + 'A');
			}
			return text;
		}

		bool EndsWithNoCase(const std::string_view text, const std::string_view suffix)
		{
			return text.size() >= suffix.size() && ToLowerAscii(std::string(text.substr(text.size() - suffix.size()))) == suffix;
		}

		/* @brief Characters of UTF-8 text, 0 when it isn't valid UTF-8 (WorldEditor's strlen_utf8). */
		size_t CountUtf8(const std::string_view text)
		{
			size_t count = 0;
			for (size_t i = 0; i < text.size(); ++i, ++count)
			{
				const unsigned char c = static_cast<unsigned char>(text[i]);
				if (c & 0x80)
				{
					if ((c & 0xE0) == 0xC0) i += 1;
					else if ((c & 0xF0) == 0xE0) i += 2;
					else if ((c & 0xF8) == 0xF0) i += 3;
					else return 0;
				}
			}
			return count;
		}

		bool IsAllAlnum(const std::string_view text)
		{
			return std::ranges::all_of(text, IsAsciiAlnum);
		}

		bool HasDigit(const std::string_view text)
		{
			return std::ranges::any_of(text, IsAsciiDigit);
		}

		/* @brief The whole text reads as a number (strtod takes all of it), with no leading space. */
		bool IsNumber(const std::string_view text)
		{
			if (text.empty() || IsAsciiSpace(text.front()))
				return false;
			const std::string copy(text);
			char *end = nullptr;
			std::strtod(copy.c_str(), &end);
			return end && *end == 0;
		}

		/* @brief `word` appears in `text` as a word of its own (no letters right before or after it). */
		bool ContainsWord(const std::string_view text, const std::string_view word)
		{
			for (size_t at = text.find(word); at != std::string_view::npos; at = text.find(word, at + 1))
			{
				const bool before = at > 0 && IsAsciiAlpha(text[at - 1]);
				const bool after = at + word.size() < text.size() && IsAsciiAlpha(text[at + word.size()]);
				if (!before && !after)
					return true;
			}
			return false;
		}

		/* @brief A code of min..max ASCII letters (and digits when allowed); false with the reason. */
		bool CheckCode(const std::string_view code, const size_t minLength, const size_t maxLength, const bool digits, std::string &reason)
		{
			const std::string quoted = "'" + std::string(code) + "'";
			if (!IsAllAlnum(code))
			{
				reason = quoted + " contains characters other than the ASCII letters and digits";
				return false;
			}
			if (code.size() < minLength || code.size() > maxLength)
			{
				reason = quoted + " should be " + (minLength == maxLength ? std::to_string(minLength) : "between " + std::to_string(minLength) + " and " + std::to_string(maxLength)) + " characters long";
				return false;
			}
			if (!digits && HasDigit(code))
			{
				reason = quoted + " contains digits";
				return false;
			}
			return true;
		}

		/* @brief Up to three examples, then "and N more". */
		std::string Examples(const std::vector<std::string> &names)
		{
			std::string out;
			const size_t shown = std::min<size_t>(names.size(), 3);
			for (size_t i = 0; i < shown; ++i)
				out += (i ? ", " : "") + names[i];
			if (names.size() > shown)
				out += " and " + std::to_string(names.size() - shown) + " more";
			return out;
		}

		std::string Plural(const size_t count, const char *one, const char *many)
		{
			return std::to_string(count) + " " + (count == 1 ? one : many);
		}

		/* @brief Even-odd test against a ring's nodes (curves ignored), in longitude / latitude like WorldEditor's Polygon2::inside. */
		bool Inside(const AptNodeChain &ring, const double latitude, const double longitude)
		{
			bool inside = false;
			const std::vector<AptNode> &nodes = ring.nodes;
			for (size_t i = 0, j = nodes.empty() ? 0 : nodes.size() - 1; i < nodes.size(); j = i++)
			{
				const AptNode &a = nodes[i];
				const AptNode &b = nodes[j];
				if ((a.latitude > latitude) != (b.latitude > latitude) &&
					longitude < (b.longitude - a.longitude) * (latitude - a.latitude) / (b.latitude - a.latitude) + a.longitude)
					inside = !inside;
			}
			return inside;
		}

		/* @brief Latitude / longitude box. */
		struct Box
		{
			double south = 90.0, west = 180.0, north = -90.0, east = -180.0;
			bool valid = false;

			void Add(const double latitude, const double longitude)
			{
				valid = true;
				south = std::min(south, latitude);
				north = std::max(north, latitude);
				west = std::min(west, longitude);
				east = std::max(east, longitude);
			}

			void Add(const Box &other)
			{
				if (!other.valid)
					return;
				Add(other.south, other.west);
				Add(other.north, other.east);
			}

			[[nodiscard]] bool Contains(const double latitude, const double longitude) const
			{
				return valid && latitude >= south && latitude <= north && longitude >= west && longitude <= east;
			}

			[[nodiscard]] bool Contains(const Box &other) const
			{
				return !other.valid || (Contains(other.south, other.west) && Contains(other.north, other.east));
			}

			[[nodiscard]] double CenterLatitude() const { return 0.5 * (south + north); }

			/* @brief Grown by nautical miles on every side (longitude scaled for the latitude). */
			[[nodiscard]] Box Expanded(const double nm) const
			{
				Box out = *this;
				if (!valid)
					return out;
				const double dLat = nm * DEGREES_PER_NM;
				const double dLon = dLat / std::max(0.01, std::cos(CenterLatitude() * std::numbers::pi / 180.0));
				out.south -= dLat;
				out.north += dLat;
				out.west -= dLon;
				out.east += dLon;
				return out;
			}
		};

		bool IsControlPoint(const AptNode &node)
		{
			return node.hasControl && (std::abs(node.controlLatitude - node.latitude) > CONTROL_EPSILON || std::abs(node.controlLongitude - node.longitude) > CONTROL_EPSILON);
		}

		/* @brief A polygon placement's box (longitude / latitude planes only). */
		Box PolygonBox(const DsfPolygonPlacement &polygon)
		{
			Box box;
			for (const std::vector<DsfPoint> &winding : polygon.windings)
			{
				for (const DsfPoint &point : winding)
					box.Add(point.Latitude(), point.Longitude());
			}
			return box;
		}

		const std::string &DefinitionName(const std::vector<std::string> &definitions, const uint32_t index)
		{
			static const std::string unknown = "(unknown definition)";
			return index < definitions.size() ? definitions[index] : unknown;
		}

		// -------------------------------------------------------

		void ValidateName(const AptAirport &airport, GatewayValidation &out)
		{
			const std::string &name = airport.name;
			const std::string &id = airport.id;
			if (name.empty())
				out.Add(GatewaySeverity::Error, "name-empty", "The airport has no name");
			else
			{
				if (CountUtf8(name) > 30)
					out.Add(GatewaySeverity::Error, "name-too-long", "The airport name '" + name + "' is longer than 30 characters");
				if (IsAsciiSpace(name.front()) || IsAsciiSpace(name.back()))
					out.Add(GatewaySeverity::Error, "name-spaces", "The airport name starts or ends with a space");

				const auto lower = std::ranges::count_if(name, [](const char c) { return c >= 'a' && c <= 'z'; });
				const auto upper = std::ranges::count_if(name, [](const char c) { return c >= 'A' && c <= 'Z'; });
				if (upper > 2 && lower == 0)
					out.Add(GatewaySeverity::Error, "name-upper-case", "The airport name is all upper case");

				const std::string nameLower = ToLowerAscii(name);
				const std::string idLower = ToLowerAscii(id);
				if (ContainsWord(nameLower, "airport"))
					out.Add(GatewaySeverity::Warning, "name-style", "The airport name shouldn't include the word 'Airport'");
				if (ContainsWord(nameLower, "international") || ContainsWord(nameLower, "int") || ContainsWord(nameLower, "regional") || ContainsWord(nameLower, "municipal"))
					out.Add(GatewaySeverity::Warning, "name-style", "The airport name should use the abbreviations 'Intl', 'Rgnl' and 'Muni' instead of the full words");
				if (!idLower.empty() && idLower != "niue" && ContainsWord(nameLower, idLower))
					out.Add(GatewaySeverity::Warning, "name-style", "The airport name shouldn't include the airport ID: use the common name only");

				// "(X) Closed Field" style: closed airports are named "[X] ..."
				if (const size_t open = name.find_first_of("({["); open != std::string::npos)
				{
					const size_t close = name.find_first_of(")}", open);
					if (close != std::string::npos && close - open == 2 && (name[open + 1] == 'x' || name[open + 1] == 'X'))
						out.Add(GatewaySeverity::Error, "name-closed", "A closed airport's name must start with '[X]'");
				}
			}

			if (id.empty())
				out.Add(GatewaySeverity::Error, "id-empty", "The airport '" + name + "' has no airport ID");
			else if (!IsAllAlnum(id))
				out.Add(GatewaySeverity::Error, "id-characters", "The airport ID '" + id + "' must use the ASCII letters and digits only");
		}

		void ValidateMetadata(const AptAirport &airport, const Box &airportBox, GatewayValidation &out)
		{
			const auto invalid = [&out](const std::string_view key, const std::string &reason)
			{
				out.Add(GatewaySeverity::Error, "metadata", "Metadata '" + std::string(key) + "' is invalid: " + reason);
			};
			const auto value = [&airport](const std::string_view key) { return std::string(airport.GetMetadata(key)); };

			if (airport.HasMetadata("altimeter_setting"))
			{
				const std::string setting = value("altimeter_setting");
				if (!setting.empty() && setting != "QNH" && setting != "QFE")
					invalid("altimeter_setting", "it must be QNH or QFE (leave it empty when unsure)");
			}

			if (airport.HasMetadata("city"))
			{
				// VTCN is in Nan, Thailand: strtod reads "Nan" as a number
				const std::string city = value("city");
				if (!city.empty() && IsNumber(city) && !(city == "Nan" && value("country").ends_with("Thailand")))
					invalid("city", "a city can't be a number");
			}

			if (airport.HasMetadata("country"))
			{
				std::string country = value("country");
				if (!country.empty())
				{
					bool prefixed = country.size() >= 3 && IsAsciiAlpha(country[0]) && IsAsciiAlpha(country[1]) && IsAsciiAlpha(country[2]) && (country.size() == 3 || country[3] == ' ');
					if (prefixed)
					{
						const std::string code = country.substr(0, 3);
						country.erase(0, 3);
						while (!country.empty() && country.front() == ' ')
							country.erase(0, 1);
						if (!IsIso3166Alpha3(code))
							invalid("country", "the first 3 letters '" + code + "' aren't an upper case ISO-3166 country code");
						else if (country.size() >= 3 && IsIso3166Alpha3(country.substr(0, 3)) && (country.size() == 3 || country[3] == ' '))
							invalid("country", "it has two country codes, '" + code + "' and '" + country.substr(0, 3) + "': keep one");
					}
					else
						invalid("country", "it must start with a 3-letter ISO-3166 country code, then an optional name ('USA United States')");

					if (IsNumber(country))
						invalid("country", "the country name can't be a number");
					else if (!country.empty() && IsAsciiDigit(country.front()))
						invalid("country", "the country name can't start with a digit");
				}
			}

			// datum_lat / datum_lon: both numbers, within 1 nm of the airport
			if (airport.HasMetadata("datum_lat") || airport.HasMetadata("datum_lon"))
			{
				const std::string datumLat = value("datum_lat");
				const std::string datumLon = value("datum_lon");
				if (!datumLat.empty() || !datumLon.empty())
				{
					bool problem = true;
					if (!IsNumber(datumLat))
						invalid("datum_lat", "not a number");
					if (!IsNumber(datumLon))
						invalid("datum_lon", "not a number");
					if (IsNumber(datumLat) && IsNumber(datumLon))
					{
						const double lat = std::strtod(datumLat.c_str(), nullptr);
						const double lon = std::strtod(datumLon.c_str(), nullptr);
						const Box nearby = airportBox.Expanded(1.0);
						if (nearby.Contains(lat, lon))
							problem = false;
						else
						{
							if (lon < nearby.west || lon > nearby.east)
								invalid("datum_lon", "the coordinate isn't within 1 nm of the airport");
							if (lat < nearby.south || lat > nearby.north)
								invalid("datum_lat", "the coordinate isn't within 1 nm of the airport");
						}
					}
					if (problem)
						out.Add(GatewaySeverity::Error, "metadata", "Metadata datum_lat and datum_lon must both be valid and come as a pair");
				}
			}

			std::string reason;
			const std::string faa = value("faa_code");
			if (airport.HasMetadata("faa_code") && !faa.empty() && !CheckCode(faa, 3, 5, true, reason))
				invalid("faa_code", reason);
			const std::string iata = value("iata_code");
			if (airport.HasMetadata("iata_code") && !iata.empty() && !CheckCode(iata, 3, 3, false, reason))
				invalid("iata_code", reason);
			const std::string icao = value("icao_code");
			if (airport.HasMetadata("icao_code") && !icao.empty())
			{
				if (!CheckCode(icao, 4, 4, false, reason))
					invalid("icao_code", reason);
				else if (icao.front() == 'x' || icao.front() == 'X')
					invalid("icao_code", "'" + icao + "' starts with X, which real ICAO codes don't (X ids are made up by Laminar)");
			}
			const std::string local = value("local_code");
			if (airport.HasMetadata("local_code") && !local.empty() && !CheckCode(local, 3, 7, true, reason))
				invalid("local_code", reason);
			const std::string authority = value("local_authority");
			if (airport.HasMetadata("local_authority") && !authority.empty() && !CheckCode(authority, 3, 16, false, reason))
				invalid("local_authority", "it should be an acronym: " + reason);
			if (!faa.empty() && !local.empty())
				out.Add(GatewaySeverity::Error, "metadata", "Metadata faa_code and local_code are both set: keep only one of them");

			if (airport.HasMetadata("region_code"))
			{
				const std::string region = ToUpperAscii(value("region_code"));
				if (!region.empty() && !std::ranges::binary_search(REGION_CODES, std::string_view(region)))
					invalid("region_code", "'" + region + "' isn't an ICAO region code");
			}

			if (airport.HasMetadata("state"))
			{
				const std::string state = value("state");
				if (IsNumber(state))
					invalid("state", "a state can't be a number");
				else if (!state.empty() && IsAsciiDigit(state.front()))
					invalid("state", "a state can't start with a digit");
			}

			// Transition altitude / level: feet, or a flight level when under 1000
			int transitionAltitude = -1;
			int good = 0;
			const auto readTransition = [&](const std::string_view key, int &feet)
			{
				if (!airport.HasMetadata(key))
					return;
				const std::string text = value(key);
				if (text.empty())
					return;
				if (!IsNumber(text))
				{
					invalid(key, "it isn't a whole number");
					return;
				}
				feet = std::atoi(text.c_str());
				if (feet < 1000)
				{
					if (feet < 100 || feet > 300)
						invalid(key, "it isn't a flight level between 100 and 300");
					else
						++good;
					feet *= 100;
				}
				else if (feet > 30000)
					invalid(key, "it isn't an altitude below 30000 ft");
				else
					++good;
			};
			int transitionLevel = -1;
			readTransition("transition_alt", transitionAltitude);
			readTransition("transition_level", transitionLevel);
			if (good == 2 && (transitionLevel - transitionAltitude < 0 || transitionLevel - transitionAltitude > 3000))
				invalid("transition_level", "the transition altitude must be 0 to 3000 ft below the transition level");

			if (airport.HasMetadata("allows_circuits"))
			{
				const std::string circuits = value("allows_circuits");
				if (circuits != "0" && circuits != "1")
					invalid("allows_circuits", "it must be 0 or 1");
			}
			if (airport.HasMetadata("tower_service_type"))
			{
				const std::string tower = value("tower_service_type");
				if (tower != "atc" && tower != "fiso")
					invalid("tower_service_type", "it must be 'atc' (full ATC service) or 'fiso' (flight information service)");
			}
			if (airport.HasMetadata("gui_label"))
			{
				const std::string label = value("gui_label");
				if (label != "2D" && label != "3D")
					invalid("gui_label", "it must be '2D' or '3D'");
			}

			for (const AptMetadata &row : airport.metadata)
			{
				if (ToLowerAscii(row.value).find("http") != std::string::npos)
					out.Add(GatewaySeverity::Error, "metadata-url", "Metadata '" + row.key + "' contains 'http' and is likely a URL: '" + row.value + "'");
			}
		}

		/* @brief The four corners of a land runway (its ends, not the blast pads). */
		std::array<std::pair<double, double>, 4> RunwayCorners(const AptRunway &runway)
		{
			const AptRunwayEnd &a = runway.ends[0];
			const AptRunwayEnd &b = runway.ends[1];
			const double heading = InitialHeading(a.latitude, a.longitude, b.latitude, b.longitude);
			const double half = 0.5 * runway.width;
			std::array<std::pair<double, double>, 4> corners;
			MovePoint(a.latitude, a.longitude, heading - 90.0, half, corners[0].first, corners[0].second);
			MovePoint(a.latitude, a.longitude, heading + 90.0, half, corners[1].first, corners[1].second);
			MovePoint(b.latitude, b.longitude, heading + 90.0, half, corners[2].first, corners[2].second);
			MovePoint(b.latitude, b.longitude, heading - 90.0, half, corners[3].first, corners[3].second);
			return corners;
		}

		void ValidateBoundary(const AptAirport &airport, GatewayValidation &out)
		{
			if (!airport.runways.empty() && airport.boundaries.empty())
				out.Add(GatewaySeverity::Error, "boundary-missing", "The airport has runways but no airport boundary");

			for (size_t b = 0; b < airport.boundaries.size(); ++b)
			{
				const AptBoundary &boundary = airport.boundaries[b];
				const std::string name = "Airport boundary" + (boundary.description.empty() ? std::string() : " '" + boundary.description + "'");
				bool curved = false;
				size_t shortSides = 0;
				for (const AptNodeChain &ring : boundary.rings)
				{
					for (size_t i = 0; i < ring.nodes.size(); ++i)
					{
						curved |= IsControlPoint(ring.nodes[i]);
						if (i + 1 == ring.nodes.size() && !ring.closed)
							break;
						const AptNode &from = ring.nodes[i];
						const AptNode &to = ring.nodes[(i + 1) % ring.nodes.size()];
						if (ring.nodes.size() > 1 && GreatCircleDistance(from.latitude, from.longitude, to.latitude, to.longitude) < GATEWAY_MIN_BOUNDARY_SIDE_M)
							++shortSides;
					}
				}
				if (curved)
					out.Add(GatewaySeverity::Error, "boundary-bezier", name + " uses bezier curves: airport boundaries must be straight sides");
				if (shortSides > 0)
					out.Add(GatewaySeverity::Error, "boundary-short-side", name + " has " + Plural(shortSides, "side", "sides") + " shorter than 30 m: delete vertices that are too close");
			}

			// Each runway, taxiway and ramp start inside one boundary (its outline; curves ignored, like WorldEditor)
			std::vector<const AptRunway *> runwaysOutside;
			for (const AptRunway &runway : airport.runways)
				runwaysOutside.push_back(&runway);
			std::vector<size_t> taxiwaysOutside;
			for (size_t i = 0; i < airport.pavements.size(); ++i)
				taxiwaysOutside.push_back(i);
			std::vector<const AptStartup *> rampsOutside;
			for (const AptStartup &startup : airport.startups)
				rampsOutside.push_back(&startup);
			std::vector<std::string> partlyInside;

			for (const AptBoundary &boundary : airport.boundaries)
			{
				if (boundary.rings.empty())
					continue;
				const AptNodeChain &outline = boundary.rings[0];
				std::erase_if(runwaysOutside, [&](const AptRunway *runway)
				{
					return std::ranges::all_of(RunwayCorners(*runway), [&](const auto &corner) { return Inside(outline, corner.first, corner.second); });
				});
				std::erase_if(taxiwaysOutside, [&](const size_t index)
				{
					const AptPavement &pavement = airport.pavements[index];
					if (pavement.rings.empty() || pavement.rings[0].nodes.empty())
						return true;
					const std::vector<AptNode> &nodes = pavement.rings[0].nodes;
					const auto inside = std::ranges::count_if(nodes, [&](const AptNode &node) { return Inside(outline, node.latitude, node.longitude); });
					if (inside == static_cast<std::ptrdiff_t>(nodes.size()))
						return true;
					if (inside == 0)
						return false;
					partlyInside.push_back(pavement.description.empty() ? "pavement " + std::to_string(index + 1) : "'" + pavement.description + "'");
					return true;
				});
				std::erase_if(rampsOutside, [&](const AptStartup *startup) { return Inside(outline, startup->latitude, startup->longitude); });
			}
			if (!airport.boundaries.empty())
			{
				std::vector<std::string> names;
				for (const AptRunway *runway : runwaysOutside)
					names.push_back(runway->GetName());
				if (!names.empty())
					out.Add(GatewaySeverity::Error, "runway-outside-boundary", "Runway " + Examples(names) + " isn't fully inside the airport boundary");
				if (!partlyInside.empty())
					out.Add(GatewaySeverity::Error, "taxiway-outside-boundary", "Taxiway " + Examples(partlyInside) + " isn't fully inside the airport boundary");
				names.clear();
				for (const size_t index : taxiwaysOutside)
					names.push_back(airport.pavements[index].description.empty() ? "pavement " + std::to_string(index + 1) : "'" + airport.pavements[index].description + "'");
				if (!names.empty())
					out.Add(GatewaySeverity::Error, "taxiway-outside-boundary", "Taxiway " + Examples(names) + " isn't inside the airport boundary");
				names.clear();
				for (const AptStartup *startup : rampsOutside)
					names.push_back("'" + startup->name + "'");
				if (!names.empty())
					out.Add(GatewaySeverity::Error, "ramp-outside-boundary", "Ramp start " + Examples(names) + " isn't inside the airport boundary");
			}
		}

		void ValidateExtent(const AptAirport &airport, const DsfFile &overlay, const Box &bounds, GatewayValidation &out)
		{
			// At most 7 nm across (KEDW's lakebed runways and KSEA's Space Needle get three times that)
			if (bounds.valid)
			{
				const double factor = (airport.id == "KEDW" || airport.id == "KSEA") ? 3.0 : 1.0;
				const double maxLat = factor * GATEWAY_MAX_SPAN_NM * DEGREES_PER_NM;
				const double maxLon = maxLat / std::max(0.01, std::cos(bounds.CenterLatitude() * std::numbers::pi / 180.0));
				if (bounds.east - bounds.west > maxLon || bounds.north - bounds.south > maxLat)
					out.Add(GatewaySeverity::Error, "airport-too-large", "The airport is impossibly large (over 7 nm across): is part of it placed far away by accident?");
			}

			if (airport.boundaries.empty())
				return;
			Box boundaryBox;
			for (const AptBoundary &boundary : airport.boundaries)
			{
				for (const AptNodeChain &ring : boundary.rings)
				{
					for (const AptNode &node : ring.nodes)
						boundaryBox.Add(node.latitude, node.longitude);
				}
			}
			const Box allowed = boundaryBox.Expanded(GATEWAY_BOUNDARY_MARGIN_NM);

			// What WorldEditor tests: polygons (pavement, draped / facade / forest shapes) and point features. Runways,
			// painted lines, .lin / .str chains and roads (checked on their own) aren't.
			std::vector<std::string> distant;
			const auto point = [&](const std::string &what, const double latitude, const double longitude)
			{
				if (!allowed.Contains(latitude, longitude))
					distant.push_back(what);
			};
			for (size_t i = 0; i < airport.pavements.size(); ++i)
			{
				Box box;
				for (const AptNodeChain &ring : airport.pavements[i].rings)
				{
					for (const AptNode &node : ring.nodes)
						box.Add(node.latitude, node.longitude);
				}
				if (!allowed.Contains(box))
					distant.push_back("pavement " + std::to_string(i + 1));
			}
			for (const AptHelipad &helipad : airport.helipads)
				point("helipad " + helipad.designator, helipad.latitude, helipad.longitude);
			for (const AptStartup &startup : airport.startups)
				point("ramp start '" + startup.name + "'", startup.latitude, startup.longitude);
			for (const AptLegacyStartup &startup : airport.legacyStartups)
				point("start '" + startup.name + "'", startup.latitude, startup.longitude);
			for (const AptWindsock &windsock : airport.windsocks)
				point("windsock", windsock.latitude, windsock.longitude);
			for (const AptSign &sign : airport.signs)
				point("sign '" + sign.text + "'", sign.latitude, sign.longitude);
			for (const AptLightFixture &light : airport.lightFixtures)
				point("light fixture", light.latitude, light.longitude);
			if (airport.beacon)
				point("beacon", airport.beacon->latitude, airport.beacon->longitude);
			if (airport.viewpoint)
				point("tower viewpoint", airport.viewpoint->latitude, airport.viewpoint->longitude);
			for (const AptTruckParking &truck : airport.truckParkings)
				point("truck parking '" + truck.name + "'", truck.latitude, truck.longitude);
			for (const AptTruckDestination &truck : airport.truckDestinations)
				point("truck destination '" + truck.name + "'", truck.latitude, truck.longitude);
			for (const AptJetway &jetway : airport.jetways)
				point("jetway", jetway.latitude, jetway.longitude);
			if (airport.taxiNetwork)
			{
				for (const AptTaxiNode &node : airport.taxiNetwork->nodes)
					point("taxi route node " + std::to_string(node.id), node.latitude, node.longitude);
			}
			for (const DsfObjectPlacement &object : overlay.objects)
				point(DefinitionName(overlay.objectDefinitions, object.definition), object.latitude, object.longitude);
			for (const DsfPolygonPlacement &polygon : overlay.polygons)
			{
				const std::string &name = DefinitionName(overlay.polygonDefinitions, polygon.definition);
				if (EndsWithNoCase(name, ".lin") || EndsWithNoCase(name, ".str"))
					continue;
				if (!allowed.Contains(PolygonBox(polygon)))
					distant.push_back(name);
			}
			if (!distant.empty())
				out.Add(GatewaySeverity::Error, "far-outside-boundary", "The airport has scenery more than 0.6 nm outside its boundary: " + Examples(distant));
		}

		void ValidateRoads(const AptAirport &airport, const DsfFile &overlay, GatewayValidation &out)
		{
			if (overlay.roads.empty())
				return;
			Box allowed;
			for (const AptBoundary &boundary : airport.boundaries)
			{
				for (const AptNodeChain &ring : boundary.rings)
				{
					for (const AptNode &node : ring.nodes)
						allowed.Add(node.latitude, node.longitude);
				}
			}
			allowed = allowed.Expanded(GATEWAY_BOUNDARY_MARGIN_NM);

			// A road node a few meters off a tile edge, but not on it, gets a sliver of road in the next tile
			constexpr double METER_DEGREES = 1.0 / 111120.0;
			const auto nearTileEdge = [](const double latitude, const double longitude)
			{
				const double dLon = std::abs(std::round(longitude) - longitude);
				const double dLat = std::abs(std::round(latitude) - latitude);
				if (dLon == 0.0 || dLat == 0.0)
					return false;
				return dLon < 3.0 * METER_DEGREES || dLat < 2.0 * METER_DEGREES;
			};

			std::set<std::string> badNetworks;
			size_t badLayers = 0, outside = 0, edges = 0, loops = 0;
			for (const DsfRoadSegment &road : overlay.roads)
			{
				const std::string &network = DefinitionName(overlay.networkDefinitions, road.definition);
				if (!IsGatewayRoadNetwork(network))
					badNetworks.insert(network);
				if (road.startLevel < 0 || road.startLevel > 5 || road.endLevel < 0 || road.endLevel > 5)
					++badLayers;
				if (road.startJunction == road.endJunction)
					++loops;
				bool isOutside = false, isOnEdge = false;
				for (const DsfRoadPoint &point : road.points)
				{
					isOutside |= !allowed.Contains(point.latitude, point.longitude);
					if (!point.control)
						isOnEdge |= nearTileEdge(point.latitude, point.longitude);
				}
				outside += isOutside ? 1 : 0;
				edges += isOnEdge ? 1 : 0;
			}
			if (!badNetworks.empty())
				out.Add(GatewaySeverity::Error, "road-network", "Only roads from lib/g10/roads.net or lib/g10/roads_EU.net are allowed on the Gateway, not " + Examples({ badNetworks.begin(), badNetworks.end() }));
			if (badLayers > 0)
				out.Add(GatewaySeverity::Error, "road-layer", Plural(badLayers, "road has", "roads have") + " a layer outside 0 to 5");
			if (loops > 0)
				out.Add(GatewaySeverity::Error, "road-loop", Plural(loops, "road starts", "roads start") + " and ends at the same junction");
			if (outside > 0)
				out.Add(GatewaySeverity::Error, "road-outside", "The road network stretches too far from the airport (" + Plural(outside, "road", "roads") + " more than 0.6 nm past the boundary)");
			if (edges > 0)
				out.Add(GatewaySeverity::Error, "road-tile-edge", Plural(edges, "road has nodes", "roads have nodes") + " a few meters from a DSF tile edge: put them exactly on the edge or further away");
		}

		void ValidateArt(const AptAirport &airport, const DsfFile &overlay, const GatewayValidationOptions &options, GatewayValidation &out)
		{
			// Every art path the overlay places, with its use count
			std::map<std::string, size_t> used;
			for (const DsfObjectPlacement &object : overlay.objects)
				++used[DefinitionName(overlay.objectDefinitions, object.definition)];
			for (const DsfPolygonPlacement &polygon : overlay.polygons)
				++used[DefinitionName(overlay.polygonDefinitions, polygon.definition)];
			for (const DsfRoadSegment &road : overlay.roads)
				++used[DefinitionName(overlay.networkDefinitions, road.definition)];

			if (!used.empty() && !options.describeResource)
				out.Add(GatewaySeverity::Error, "art-unchecked", "The X-Plane library hasn't been scanned, so the art can't be checked against X-Plane's default library");
			else if (options.describeResource)
			{
				std::vector<std::string> custom, hidden;
				for (const auto &[path, count] : used)
				{
					const GatewayResourceInfo info = options.describeResource(path);
					const std::string label = "'" + path + "'" + (count > 1 ? " (" + std::to_string(count) + "x)" : std::string());
					if (!info.known || !info.defaultLibrary)
						custom.push_back(label + (info.known ? std::string() : " (no library has it)"));
					else if (info.privateOrDeprecated)
						hidden.push_back(label);
				}
				if (!custom.empty())
					out.Add(GatewaySeverity::Error, "art-not-default", Plural(custom.size(), "art path isn't", "art paths aren't") + " part of X-Plane's default library, so they can't go to the global airport database: " + Examples(custom));
				if (!hidden.empty())
					out.Add(GatewaySeverity::Error, "art-private", Plural(hidden.size(), "art path is", "art paths are") + " private or deprecated in X-Plane's library: " + Examples(hidden));
			}

			// Orthophotos (UV mapped .pol) need automatic subtexture selection
			std::set<std::string> orthophotos;
			for (const DsfPolygonPlacement &polygon : overlay.polygons)
			{
				const std::string &name = DefinitionName(overlay.polygonDefinitions, polygon.definition);
				if (polygon.parameter == 65535 && EndsWithNoCase(name, ".pol"))
					orthophotos.insert(name);
			}
			std::vector<std::string> badOrthophotos;
			for (const std::string &name : orthophotos)
			{
				const int boxes = options.countSubtextures ? options.countSubtextures(name) : -1;
				if (boxes <= 0)
					badOrthophotos.push_back("'" + name + "'");
			}
			if (!badOrthophotos.empty())
				out.Add(GatewaySeverity::Error, "orthophoto", "Only orthophotos with automatic subtexture selection (#subtex) can go to the Gateway: hide or remove " + Examples(badOrthophotos));

			// Object elevations: set_MSL is refused, set_AGL discouraged
			size_t msl = 0, agl = 0, aglRange = 0, mslRange = 0;
			for (const DsfObjectPlacement &object : overlay.objects)
			{
				if (object.mode == DsfObjectPlacement::Mode::Msl)
				{
					++msl;
					if (object.elevation < -1000.0 || object.elevation > 10000.0)
						++mslRange;
				}
				else if (object.mode == DsfObjectPlacement::Mode::Agl)
				{
					++agl;
					if (object.elevation < -100.0 || object.elevation > 100.0)
						++aglRange;
				}
			}
			if (msl > 0)
				out.Add(GatewaySeverity::Error, "set-msl", Plural(msl, "object uses", "objects use") + " set_MSL, which the Scenery Gateway doesn't allow");
			if (mslRange > 0)
				out.Add(GatewaySeverity::Error, "msl-range", Plural(mslRange, "object is", "objects are") + " outside the -1000 to +10000 m the rendering engine handles");
			if (agl > 0)
				out.Add(GatewaySeverity::Warning, "set-agl", Plural(agl, "object uses", "objects use") + " set_AGL, which is discouraged on the Gateway: use it only in well justified cases");
			if (aglRange > 0)
				out.Add(GatewaySeverity::Error, "agl-range", Plural(aglRange, "object is", "objects are") + " more than 100 m above or below the ground (set_AGL)");

			size_t customTrucks = 0;
			for (const AptTruckParking &truck : airport.truckParkings)
				customTrucks += truck.customObject.empty() ? 0 : 1;
			if (customTrucks > 0)
				out.Add(GatewaySeverity::Error, "custom-truck", Plural(customTrucks, "truck parking uses", "truck parkings use") + " a custom vehicle object, which the Gateway doesn't allow");

			std::vector<std::string> transparent;
			for (const AptRunway &runway : airport.runways)
			{
				if (runway.surface == SurfaceTransparent)
					transparent.push_back(runway.GetName());
			}
			if (!transparent.empty())
				out.Add(GatewaySeverity::Error, "runway-transparent", "Transparent runways aren't allowed on the Scenery Gateway: " + Examples(transparent));
		}
	}

	// -------------------------------------------------------

	size_t GatewayValidation::CountErrors() const
	{
		return static_cast<size_t>(std::ranges::count_if(problems, [](const GatewayProblem &p) { return p.severity == GatewaySeverity::Error; }));
	}

	size_t GatewayValidation::CountWarnings() const
	{
		return static_cast<size_t>(std::ranges::count_if(problems, [](const GatewayProblem &p) { return p.severity == GatewaySeverity::Warning; }));
	}

	bool GatewayValidation::Has(const std::string_view code) const
	{
		return std::ranges::any_of(problems, [code](const GatewayProblem &p) { return p.code == code; });
	}

	void GatewayValidation::Add(const GatewaySeverity severity, std::string code, std::string message)
	{
		problems.push_back({ severity, std::move(code), std::move(message) });
	}

	GatewayValidation ValidateGatewayAirport(const AptAirport &airport, const DsfFile &overlay, const GatewayValidationOptions &options)
	{
		GatewayValidation out;

		// The airport's whole extent: its rows and everything it places (control points aside)
		Box bounds;
		double south, west, north, east;
		if (airport.GetBounds(south, west, north, east))
		{
			bounds.Add(south, west);
			bounds.Add(north, east);
		}
		for (const DsfObjectPlacement &object : overlay.objects)
			bounds.Add(object.latitude, object.longitude);
		for (const DsfPolygonPlacement &polygon : overlay.polygons)
			bounds.Add(PolygonBox(polygon));
		for (const DsfRoadSegment &road : overlay.roads)
		{
			for (const DsfRoadPoint &point : road.points)
			{
				if (!point.control)
					bounds.Add(point.latitude, point.longitude);
			}
		}

		ValidateName(airport, out);
		if (airport.type == AirportType::Land && airport.runways.empty() && !airport.helipads.empty())
			out.Add(GatewaySeverity::Error, "no-runways", "The airport has no runways: make it a heliport, or add a runway");
		for (std::string &problem : ValidateAptAirport(airport))
			out.Add(GatewaySeverity::Error, "apt", std::move(problem));
		ValidateBoundary(airport, out);
		ValidateExtent(airport, overlay, bounds, out);
		ValidateArt(airport, overlay, options, out);
		ValidateRoads(airport, overlay, out);
		ValidateMetadata(airport, bounds, out);
		return out;
	}

	GatewayResourceInfo DescribeGatewayResource(const LibraryIndex &index, const std::string_view path)
	{
		GatewayResourceInfo info;
		if (path == "::FLATTEN::.pol")
		{
			info.known = true;
			info.defaultLibrary = true;
			return info;
		}
		const LibraryAsset *asset = index.Find(path);
		if (!asset)
			return info;

		info.known = true;
		bool anyPublic = false, anyPublicDefault = false;
		const std::vector<LibraryPackage> &packages = index.GetPackages();
		for (const AssetProvider &provider : asset->providers)
		{
			const bool isDefault = provider.package < packages.size() && packages[provider.package].source == PackageSource::DefaultScenery;
			const bool isPublic = !provider.isPrivate && !provider.deprecated;		// SEMI_DEPRECATED still passes
			anyPublic |= isPublic;
			if (isDefault)
			{
				info.defaultLibrary = true;
				anyPublicDefault |= isPublic;
			}
		}
		info.privateOrDeprecated = info.defaultLibrary ? !anyPublicDefault : !anyPublic;
		return info;
	}

	bool IsIso3166Alpha3(const std::string_view code)
	{
		return code.size() == 3 && std::ranges::binary_search(ISO3166_CODES, code);
	}

	const std::vector<std::string_view> &GetGatewayMetadataKeys()
	{
		static const std::vector<std::string_view> keys = {
			"allows_circuits", "altimeter_setting", "city", "country", "datum_lat", "datum_lon", "faa_code", "gui_label", "iata_code",
			"icao_code", "is_oilrig", "local_authority", "local_code", "region_code", "state", "tower_service_type", "transition_alt",
			"transition_level" };
		return keys;
	}

	bool IsGatewayRoadNetwork(const std::string_view path)
	{
		return path == "lib/g10/roads.net" || path == "lib/g10/roads_EU.net";
	}
}

// -------------------------------------------------------

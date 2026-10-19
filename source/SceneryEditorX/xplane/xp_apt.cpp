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
 * xp_apt.cpp
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#include "xp_apt.h"
#include "xp_text.h"
#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <set>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	namespace
	{
		constexpr double kPi = 3.14159265358979323846;
		constexpr double kDegToRad = kPi / 180.0;
		constexpr double kRadToDeg = 180.0 / kPi;
		constexpr double kEarthRadius = 6371008.8;	// Mean radius, meters

		struct NamedCode
		{
			int code;
			const char *name;
		};

		constexpr NamedCode SURFACES[] =
		{
			{ 1, "Asphalt" }, { 2, "Concrete" }, { 3, "Grass" }, { 4, "Dirt" }, { 5, "Gravel" }, { 12, "Dry lakebed" },
			{ 13, "Water" }, { 14, "Snow / ice" }, { 15, "Transparent" },
			{ 20, "Light asphalt 1" }, { 21, "Light asphalt 2" }, { 22, "Light asphalt 3" }, { 23, "Light asphalt 4" },
			{ 24, "Asphalt 2" }, { 25, "Asphalt 3" }, { 26, "Asphalt 4" },
			{ 27, "Dark asphalt 1" }, { 28, "Dark asphalt 2" }, { 29, "Dark asphalt 3" }, { 30, "Dark asphalt 4" },
			{ 31, "Very dark asphalt 1" }, { 32, "Very dark asphalt 2" }, { 33, "Very dark asphalt 3" }, { 34, "Very dark asphalt 4" },
			{ 35, "New asphalt 1" }, { 36, "New asphalt 2" }, { 37, "New asphalt 3" }, { 38, "New asphalt 4" },
			{ 50, "Light concrete 1" }, { 51, "Light concrete 2" }, { 52, "Light concrete 3" },
			{ 53, "Concrete 2" }, { 54, "Concrete 3" },
			{ 55, "Dark concrete 1" }, { 56, "Dark concrete 2" }, { 57, "Dark concrete 3" }
		};

		constexpr NamedCode PAINTED_LINES[] =
		{
			{ 0, "None" }, { 1, "Solid yellow" }, { 2, "Broken yellow" }, { 3, "Double solid yellow" },
			{ 4, "Runway hold position" }, { 5, "Other hold position" }, { 6, "ILS hold position" },
			{ 7, "Taxiway centerline enhanced" }, { 8, "Aircraft queue lane" }, { 9, "Aircraft queue lane double" },
			{ 20, "Solid white" }, { 21, "Chequerboard white" }, { 22, "Broken white" },
			{ 51, "Solid yellow (black border)" }, { 52, "Broken yellow (black border)" }, { 53, "Double solid yellow (black border)" },
			{ 54, "Runway hold position (black border)" }, { 55, "Other hold position (black border)" }, { 56, "ILS hold position (black border)" },
			{ 57, "Taxiway centerline enhanced (black border)" }, { 58, "Aircraft queue lane (black border)" }, { 59, "Aircraft queue lane double (black border)" }
		};

		constexpr NamedCode LIGHT_LINES[] =
		{
			{ 101, "Taxiway centerline lights" }, { 102, "Taxiway edge lights" }, { 103, "Hold short lights" },
			{ 104, "Runway hold short lights (pulsating)" }, { 105, "Safety zone centerline lights" }, { 106, "Critical zone lights" },
			{ 107, "Lead-off lights" }, { 108, "Lead-off lights (safety zone)" }
		};

		constexpr const char *RUNWAY_MARKINGS[] = { "None", "Visual", "Non-precision", "Precision", "UK non-precision", "UK precision", "EASA non-precision", "EASA precision" };
		constexpr const char *APPROACH_LIGHTS[] = { "None", "ALSF-I", "ALSF-II", "Calvert", "Calvert Cat II/III", "SSALR", "SSALF", "SALS", "MALSR", "MALSF", "MALS", "ODALS", "RAIL" };
		constexpr const char *REILS[] = { "None", "Omnidirectional", "Unidirectional" };
		constexpr const char *BEACONS[] = { "None", "White-green (civil)", "White-yellow (seaplane)", "Green-yellow-white (heliport)", "White-white-green (military)" };
		constexpr const char *SIGN_SIZES[] = { "", "Small taxiway", "Medium taxiway", "Large taxiway", "Large distance remaining", "Small distance remaining" };
		constexpr const char *LIGHT_FIXTURES[] = { "", "VASI", "PAPI 4L", "PAPI 4R", "Space Shuttle PAPI", "Tri-color VASI", "Wig-wag", "APAPI L", "APAPI R" };
		constexpr const char *FREQUENCY_TYPES[] = { "Recorded (ATIS / AWOS)", "Unicom / CTAF", "Clearance", "Ground", "Tower", "Approach", "Departure" };
		constexpr const char *JETWAY_STYLES[] = { "Light, solid sidewalls", "Light, glass sidewalls", "Dark, solid sidewalls", "Dark, glass sidewalls" };
		constexpr const char *JETWAY_SIZES[] = { "11 - 23 m", "14 - 29 m", "17 - 38 m", "20 - 47 m" };

		const std::vector<const char *> START_TYPES = { "gate", "hangar", "misc", "tie_down" };
		const std::vector<const char *> OPERATION_TYPES = { "none", "general_aviation", "airline", "cargo", "military" };
		const std::vector<const char *> TRUCK_TYPES = { "baggage_loader", "baggage_train", "crew_car", "crew_ferrari", "crew_limo", "pushback", "fuel_liners", "fuel_jets", "fuel_props", "food", "gpu" };
		const std::vector<const char *> AIRCRAFT_TYPES = { "heavy", "jets", "turboprops", "props", "helos", "fighters" };
		const std::vector<const char *> NODE_USAGES = { "init", "end", "both", "junc", "dest" };

		const char *LookupName(const NamedCode *table, const size_t count, const int code, const char *fallback)
		{
			for (size_t i = 0; i < count; ++i)
			{
				if (table[i].code == code)
					return table[i].name;
			}
			return fallback;
		}

		template<size_t N>
		const char *LookupIndex(const char *const (&table)[N], const int index, const char *fallback = "Unknown")
		{
			return index >= 0 && static_cast<size_t>(index) < N ? table[index] : fallback;
		}

		// -------------------------------------------------------

		/* @brief printf into a string. */
		void Append(std::string &out, const char *format, ...)
		{
			char buffer[512];
			va_list args;
			va_start(args, format);
			const int written = vsnprintf(buffer, sizeof(buffer), format, args);
			va_end(args);
			if (written > 0)
				out.append(buffer, std::min(static_cast<size_t>(written), sizeof(buffer) - 1));
		}

		void AppendCoord(std::string &out, const double latitude, const double longitude)
		{
			Append(out, " %.8f %.8f", latitude, longitude);
		}

		/* @brief Appends " text" (or just " " when empty, so a key with an empty value stays a complete row like WED writes). */
		void AppendText(std::string &out, const std::string_view text)
		{
			out.push_back(' ');
			out.append(text);
		}

		bool ValidLatitude(const double value) { return std::isfinite(value) && value >= -90.0 && value <= 90.0; }
		bool ValidLongitude(const double value) { return std::isfinite(value) && value >= -180.0 && value <= 180.0; }
	}

	// -------------------------------------------------------

	const char *GetAirportTypeName(const AirportType type)
	{
		switch (type)
		{
			case AirportType::Land:		return "Airport";
			case AirportType::Seaplane:	return "Seaplane base";
			case AirportType::Heliport:	return "Heliport";
		}
		return "Airport";
	}

	const char *GetSurfaceName(const int surface)
	{
		return LookupName(SURFACES, std::size(SURFACES), surface, "Unknown surface");
	}

	SurfaceFamily GetSurfaceFamily(const int surface)
	{
		switch (surface)
		{
			case 1: case 20: case 21: case 22: case 23: case 24: case 25: case 26: case 27: case 28: case 29: case 30:
			case 31: case 32: case 33: case 34: case 35: case 36: case 37: case 38:
				return SurfaceFamily::Asphalt;
			case 2: case 50: case 51: case 52: case 53: case 54: case 55: case 56: case 57:
				return SurfaceFamily::Concrete;
			case 3:		return SurfaceFamily::Grass;
			case 4:		return SurfaceFamily::Dirt;
			case 5:		return SurfaceFamily::Gravel;
			case 12:	return SurfaceFamily::Lakebed;
			case 13:	return SurfaceFamily::Water;
			case 14:	return SurfaceFamily::Snow;
			case 15:	return SurfaceFamily::Transparent;
			default:	return SurfaceFamily::Unknown;
		}
	}

	const std::vector<int> &GetSurfaceCodes()
	{
		static const std::vector<int> codes = []
		{
			std::vector<int> result;
			for (const NamedCode &entry : SURFACES)
				result.push_back(entry.code);
			return result;
		}();
		return codes;
	}

	const char *GetRunwayMarkingName(const int marking) { return LookupIndex(RUNWAY_MARKINGS, marking); }
	const char *GetApproachLightsName(const int code) { return LookupIndex(APPROACH_LIGHTS, code); }
	const char *GetReilName(const int code) { return LookupIndex(REILS, code); }
	const char *GetEdgeLightsName(const int code) { return code == 0 ? "None" : (code == 2 ? "Medium intensity" : (code == 1 ? "Low intensity" : (code == 3 ? "High intensity" : "Unknown"))); }
	const char *GetBeaconTypeName(const int type) { return LookupIndex(BEACONS, type); }
	const char *GetSignSizeName(const int size) { return LookupIndex(SIGN_SIZES, size); }
	const char *GetLightFixtureName(const int type) { return LookupIndex(LIGHT_FIXTURES, type); }
	const char *GetHelipadEdgeLightsName(const int code) { return code == 0 ? "None" : (code == 1 ? "Yellow" : "Unknown"); }

	const char *GetLineTypeName(const int type)
	{
		if (type >= 100)
			return LookupName(LIGHT_LINES, std::size(LIGHT_LINES), type, "Unknown lights");
		return LookupName(PAINTED_LINES, std::size(PAINTED_LINES), type, "Unknown line");
	}

	bool IsLightLineType(const int type)
	{
		return type >= 100;
	}

	const std::vector<int> &GetPaintedLineCodes()
	{
		static const std::vector<int> codes = []
		{
			std::vector<int> result;
			for (const NamedCode &entry : PAINTED_LINES)
				result.push_back(entry.code);
			return result;
		}();
		return codes;
	}

	const std::vector<int> &GetLightLineCodes()
	{
		static const std::vector<int> codes = []
		{
			std::vector<int> result;
			for (const NamedCode &entry : LIGHT_LINES)
				result.push_back(entry.code);
			return result;
		}();
		return codes;
	}

	const char *GetFrequencyTypeName(const AptFrequency::Type type)
	{
		return LookupIndex(FREQUENCY_TYPES, static_cast<int>(type));
	}

	const char *GetJetwayStyleName(const int style) { return LookupIndex(JETWAY_STYLES, style); }
	const char *GetJetwaySizeName(const int size) { return LookupIndex(JETWAY_SIZES, size); }

	const char *GetStartupTypeName(const std::string_view type)
	{
		if (type == "gate") return "Gate";
		if (type == "hangar") return "Hangar";
		if (type == "misc") return "Miscellaneous";
		if (type == "tie_down" || type == "tie-down") return "Tie-down";
		return "Unknown";
	}

	const char *GetTruckTypeName(const std::string_view type)
	{
		if (type == "baggage_loader") return "Baggage loader";
		if (type == "baggage_train") return "Baggage train";
		if (type == "crew_car") return "Crew car";
		if (type == "crew_ferrari") return "Crew Ferrari";
		if (type == "crew_limo") return "Crew limo";
		if (type == "pushback") return "Pushback tug";
		if (type == "fuel_liners") return "Fuel truck (airliners)";
		if (type == "fuel_jets") return "Fuel truck (jets)";
		if (type == "fuel_props") return "Fuel truck (props)";
		if (type == "food") return "Catering";
		if (type == "gpu") return "Ground power unit";
		return "Unknown";
	}

	const std::vector<const char *> &GetStartupTypes() { return START_TYPES; }
	const std::vector<const char *> &GetOperationTypes() { return OPERATION_TYPES; }
	const std::vector<const char *> &GetTruckTypes() { return TRUCK_TYPES; }
	const std::vector<const char *> &GetAircraftTypes() { return AIRCRAFT_TYPES; }
	const std::vector<const char *> &GetTaxiNodeUsages() { return NODE_USAGES; }

	// -------------------------------------------------------

	double GreatCircleDistance(const double lat1, const double lon1, const double lat2, const double lon2)
	{
		const double phi1 = lat1 * kDegToRad;
		const double phi2 = lat2 * kDegToRad;
		const double dPhi = (lat2 - lat1) * kDegToRad;
		const double dLambda = (lon2 - lon1) * kDegToRad;
		const double a = std::sin(dPhi * 0.5) * std::sin(dPhi * 0.5) + std::cos(phi1) * std::cos(phi2) * std::sin(dLambda * 0.5) * std::sin(dLambda * 0.5);
		return 2.0 * kEarthRadius * std::atan2(std::sqrt(a), std::sqrt(std::max(0.0, 1.0 - a)));
	}

	double InitialHeading(const double lat1, const double lon1, const double lat2, const double lon2)
	{
		const double phi1 = lat1 * kDegToRad;
		const double phi2 = lat2 * kDegToRad;
		const double dLambda = (lon2 - lon1) * kDegToRad;
		const double y = std::sin(dLambda) * std::cos(phi2);
		const double x = std::cos(phi1) * std::sin(phi2) - std::sin(phi1) * std::cos(phi2) * std::cos(dLambda);
		double heading = std::atan2(y, x) * kRadToDeg;
		if (heading < 0.0)
			heading += 360.0;
		return heading;
	}

	void MovePoint(const double lat, const double lon, const double headingDegrees, const double meters, double &outLat, double &outLon)
	{
		const double delta = meters / kEarthRadius;
		const double theta = headingDegrees * kDegToRad;
		const double phi1 = lat * kDegToRad;
		const double lambda1 = lon * kDegToRad;
		const double phi2 = std::asin(std::sin(phi1) * std::cos(delta) + std::cos(phi1) * std::sin(delta) * std::cos(theta));
		const double lambda2 = lambda1 + std::atan2(std::sin(theta) * std::sin(delta) * std::cos(phi1), std::cos(delta) - std::sin(phi1) * std::sin(phi2));
		outLat = phi2 * kRadToDeg;
		outLon = lambda2 * kRadToDeg;
		while (outLon > 180.0) outLon -= 360.0;
		while (outLon < -180.0) outLon += 360.0;
	}

	double AptRunway::GetLengthMeters() const
	{
		return GreatCircleDistance(ends[0].latitude, ends[0].longitude, ends[1].latitude, ends[1].longitude);
	}

	double AptRunway::GetHeading() const
	{
		return InitialHeading(ends[0].latitude, ends[0].longitude, ends[1].latitude, ends[1].longitude);
	}

	// -------------------------------------------------------

	std::string_view AptAirport::GetMetadata(const std::string_view key) const
	{
		for (const AptMetadata &entry : metadata)
		{
			if (entry.key == key)
				return entry.value;
		}
		return {};
	}

	bool AptAirport::HasMetadata(const std::string_view key) const
	{
		return std::ranges::any_of(metadata, [key](const AptMetadata &entry) { return entry.key == key; });
	}

	void AptAirport::SetMetadata(const std::string_view key, const std::string_view value)
	{
		for (AptMetadata &entry : metadata)
		{
			if (entry.key == key)
			{
				entry.value = std::string(value);
				return;
			}
		}
		metadata.push_back({ std::string(key), std::string(value) });
	}

	void AptAirport::RemoveMetadata(const std::string_view key)
	{
		std::erase_if(metadata, [key](const AptMetadata &entry) { return entry.key == key; });
	}

	std::string AptAirport::GetIcao() const
	{
		const std::string_view icao = GetMetadata("icao_code");
		return icao.empty() ? id : std::string(icao);
	}

	bool AptAirport::Is3D() const
	{
		return GetMetadata("gui_label") == "3D";
	}

	namespace
	{
		/* @brief Calls visitor(lat, lon) for every coordinate of an airport (control points excluded). */
		template<typename Visitor>
		void VisitCoordinates(const AptAirport &airport, Visitor &&visitor)
		{
			for (const AptRunway &runway : airport.runways)
			{
				for (const AptRunwayEnd &end : runway.ends)
					visitor(end.latitude, end.longitude);
			}
			for (const AptWaterRunway &runway : airport.waterRunways)
			{
				for (const AptWaterRunway::End &end : runway.ends)
					visitor(end.latitude, end.longitude);
			}
			for (const AptHelipad &helipad : airport.helipads)
				visitor(helipad.latitude, helipad.longitude);
			const auto chains = [&visitor](const std::vector<AptNodeChain> &list)
			{
				for (const AptNodeChain &chain : list)
				{
					for (const AptNode &node : chain.nodes)
						visitor(node.latitude, node.longitude);
				}
			};
			for (const AptPavement &pavement : airport.pavements)
				chains(pavement.rings);
			for (const AptLinearFeature &feature : airport.linearFeatures)
				chains(feature.chains);
			for (const AptBoundary &boundary : airport.boundaries)
				chains(boundary.rings);
			if (airport.viewpoint)
				visitor(airport.viewpoint->latitude, airport.viewpoint->longitude);
			for (const AptLegacyStartup &startup : airport.legacyStartups)
				visitor(startup.latitude, startup.longitude);
			if (airport.beacon && airport.beacon->type != 0)
				visitor(airport.beacon->latitude, airport.beacon->longitude);
			for (const AptWindsock &windsock : airport.windsocks)
				visitor(windsock.latitude, windsock.longitude);
			for (const AptSign &sign : airport.signs)
				visitor(sign.latitude, sign.longitude);
			for (const AptLightFixture &fixture : airport.lightFixtures)
				visitor(fixture.latitude, fixture.longitude);
			if (airport.taxiNetwork)
			{
				for (const AptTaxiNode &node : airport.taxiNetwork->nodes)
					visitor(node.latitude, node.longitude);
			}
			for (const AptStartup &startup : airport.startups)
				visitor(startup.latitude, startup.longitude);
			for (const AptTruckParking &truck : airport.truckParkings)
				visitor(truck.latitude, truck.longitude);
			for (const AptTruckDestination &destination : airport.truckDestinations)
				visitor(destination.latitude, destination.longitude);
			for (const AptJetway &jetway : airport.jetways)
				visitor(jetway.latitude, jetway.longitude);
		}
	}

	bool AptAirport::GetBounds(double &south, double &west, double &north, double &east) const
	{
		bool any = false;
		south = 90.0;
		north = -90.0;
		west = 180.0;
		east = -180.0;
		VisitCoordinates(*this, [&](const double latitude, const double longitude)
		{
			if (!ValidLatitude(latitude) || !ValidLongitude(longitude))
				return;
			any = true;
			south = std::min(south, latitude);
			north = std::max(north, latitude);
			west = std::min(west, longitude);
			east = std::max(east, longitude);
		});
		return any;
	}

	bool AptAirport::GetDatum(double &latitude, double &longitude) const
	{
		const std::string_view datumLat = GetMetadata("datum_lat");
		const std::string_view datumLon = GetMetadata("datum_lon");
		if (!datumLat.empty() && !datumLon.empty())
		{
			float lat = 0.0f, lon = 0.0f;
			if (TryParseFloat(datumLat, lat) && TryParseFloat(datumLon, lon) && ValidLatitude(lat) && ValidLongitude(lon))
			{
				// Floats lose precision past ~1 m; re-parse as double.
				latitude = std::strtod(std::string(datumLat).c_str(), nullptr);
				longitude = std::strtod(std::string(datumLon).c_str(), nullptr);
				return true;
			}
		}

		// Prefer the runways / helipads (the flight-relevant center), then anything else.
		double sumLat = 0.0, sumLon = 0.0;
		size_t count = 0;
		for (const AptRunway &runway : runways)
		{
			for (const AptRunwayEnd &end : runway.ends)
			{
				sumLat += end.latitude;
				sumLon += end.longitude;
				++count;
			}
		}
		for (const AptWaterRunway &runway : waterRunways)
		{
			for (const AptWaterRunway::End &end : runway.ends)
			{
				sumLat += end.latitude;
				sumLon += end.longitude;
				++count;
			}
		}
		for (const AptHelipad &helipad : helipads)
		{
			sumLat += helipad.latitude;
			sumLon += helipad.longitude;
			++count;
		}
		if (count > 0)
		{
			latitude = sumLat / static_cast<double>(count);
			longitude = sumLon / static_cast<double>(count);
			return true;
		}

		double south, west, north, east;
		if (!GetBounds(south, west, north, east))
			return false;
		latitude = 0.5 * (south + north);
		longitude = 0.5 * (west + east);
		return true;
	}

	size_t AptAirport::GetElementCount() const
	{
		size_t count = runways.size() + waterRunways.size() + helipads.size() + pavements.size() + linearFeatures.size() + boundaries.size() +
					   (viewpoint ? 1 : 0) + legacyStartups.size() + (beacon ? 1 : 0) + windsocks.size() + signs.size() + lightFixtures.size() +
					   startups.size() + truckParkings.size() + truckDestinations.size() + jetways.size();
		if (taxiNetwork)
			count += taxiNetwork->nodes.size() + taxiNetwork->edges.size();
		return count;
	}

	// -------------------------------------------------------
	// Parser
	// -------------------------------------------------------

	namespace
	{
		struct Parser
		{
			AptFile &file;
			const AptParseOptions &options;
			std::vector<std::string_view> tokens;
			uint32_t lineNumber = 0;

			AptAirport *airport = nullptr;
			std::vector<AptNodeChain> *chains = nullptr;	// Rings / chains of the current 110 / 120 / 130
			AptNodeChain *chain = nullptr;					// The chain being filled, null between chains
			AptTaxiEdge *lastEdge = nullptr;
			AptStartup *lastStartup = nullptr;
			AptTruckParking *lastTruck = nullptr;
			AptJetway *lastJetway = nullptr;

			Parser(AptFile &outFile, const AptParseOptions &parseOptions) : file(outFile), options(parseOptions) {}

			void Warn(const std::string &message)
			{
				file.warnings.push_back("Line " + std::to_string(lineNumber) + ": " + message);
			}

			std::string_view Token(const size_t index) const
			{
				return index < tokens.size() ? tokens[index] : std::string_view();
			}

			double Double(const size_t index, const double fallback = 0.0) const
			{
				const std::string_view text = Token(index);
				if (text.empty())
					return fallback;
				char *end = nullptr;
				const std::string copy(text);
				const double value = std::strtod(copy.c_str(), &end);
				return (end && *end == '\0') ? value : fallback;
			}

			float Float(const size_t index, const float fallback = 0.0f) const { return ParseFloat(Token(index), fallback); }
			int Int(const size_t index, const int fallback = 0) const { return ParseInt(Token(index), fallback); }
			bool Bool(const size_t index) const { return Int(index) != 0; }
			std::string Text(const std::string_view line, const size_t firstToken) const { return std::string(RestOfLine(line, firstToken)); }

			/* @brief Leaves any pavement / line / flow / network context: a new row kind started. */
			void EndNodes()
			{
				chains = nullptr;
				chain = nullptr;
			}

			bool RequireAirport(const int code)
			{
				if (airport)
					return true;
				Warn("Row " + std::to_string(code) + " before any airport header, ignored");
				return false;
			}

			bool RequireTokens(const size_t count, const int code)
			{
				if (tokens.size() >= count)
					return true;
				Warn("Row " + std::to_string(code) + " has " + std::to_string(tokens.size() - 1) + " fields, " + std::to_string(count - 1) + " expected");
				return false;
			}

			void StartAirport(const AirportType type, const std::string_view line)
			{
				EndNodes();
				lastEdge = nullptr;
				lastStartup = nullptr;
				lastTruck = nullptr;
				lastJetway = nullptr;

				AptAirport created;
				created.type = type;
				created.elevationFt = Int(1);
				created.id = std::string(Token(4));
				created.name = Text(line, 5);
				created.sourceLine = lineNumber;
				if (created.id.empty())
					Warn("Airport header without an id");
				file.airports.push_back(std::move(created));
				airport = &file.airports.back();
			}

			void ParseRunwayEnd(AptRunwayEnd &end, const size_t first)
			{
				end.designator = std::string(Token(first));
				end.latitude = Double(first + 1);
				end.longitude = Double(first + 2);
				end.displacedThreshold = Float(first + 3);
				end.blastPad = Float(first + 4);
				end.marking = Int(first + 5);
				end.approachLights = Int(first + 6);
				end.touchdownZoneLights = Bool(first + 7);
				end.reil = Int(first + 8);
			}

			void ParseNode(const int code)
			{
				if (!chains)
				{
					Warn("Node row " + std::to_string(code) + " outside a pavement, line or boundary, ignored");
					return;
				}

				const bool bezier = code == 112 || code == 114 || code == 116;
				const bool closes = code == 113 || code == 114;
				const bool ends = code == 115 || code == 116;
				if (!RequireTokens(bezier ? 5 : 3, code))
					return;

				if (!chain)
				{
					chains->emplace_back();
					chain = &chains->back();
				}

				AptNode node;
				node.latitude = Double(1);
				node.longitude = Double(2);
				size_t next = 3;
				if (bezier)
				{
					node.hasControl = true;
					node.controlLatitude = Double(3);
					node.controlLongitude = Double(4);
					next = 5;
				}

				// Trailing style codes: painted lines are below 100, light strings 100 and up (either may stand alone).
				for (; next < tokens.size(); ++next)
				{
					const int style = Int(next, -1);
					if (style < 0)
						break;
					if (IsLightLineType(style))
						node.lightType = style;
					else
						node.lineType = style;
				}

				chain->nodes.push_back(node);
				if (closes || ends)
				{
					chain->closed = closes;
					chain = nullptr;
				}
			}

			void ParseFrequency(const int code, const std::string_view line)
			{
				AptFrequency frequency;
				frequency.legacy = code < 1000;
				frequency.type = static_cast<AptFrequency::Type>(frequency.legacy ? code - 50 : code - 1050);
				frequency.frequencyKhz = frequency.legacy ? Int(1) * 10 : Int(1);
				frequency.name = Text(line, 2);
				airport->frequencies.push_back(std::move(frequency));
			}

			void ParseHeadingRange(const std::string_view text, int &from, int &to)
			{
				// Six digits: two three-digit headings ("280099" = 280 to 099).
				if (text.size() == 6)
				{
					from = ParseInt(text.substr(0, 3), 0);
					to = ParseInt(text.substr(3, 3), 359);
				}
				else
				{
					from = 0;
					to = 359;
				}
			}

			AptFlow *CurrentFlow(const int code)
			{
				if (airport->flows.empty())
				{
					Warn("Row " + std::to_string(code) + " before any 1000 flow, ignored");
					return nullptr;
				}
				return &airport->flows.back();
			}

			void ParseRow(const int code, const std::string_view line)
			{
				switch (code)
				{
					case 1: StartAirport(AirportType::Land, line); return;
					case 16: StartAirport(AirportType::Seaplane, line); return;
					case 17: StartAirport(AirportType::Heliport, line); return;
					default: break;
				}

				if (!RequireAirport(code))
					return;

				// Node rows continue the current pavement / line / boundary; anything else ends it.
				if (code >= 111 && code <= 116)
				{
					ParseNode(code);
					return;
				}
				EndNodes();

				switch (code)
				{
					case 100:
					{
						if (!RequireTokens(26, code))
							return;
						AptRunway runway;
						runway.width = Float(1);
						runway.surface = Int(2);
						runway.shoulder = Int(3);
						runway.smoothness = Float(4);
						runway.centerlineLights = Bool(5);
						runway.edgeLights = Int(6);
						runway.distanceSigns = Bool(7);
						ParseRunwayEnd(runway.ends[0], 8);
						ParseRunwayEnd(runway.ends[1], 17);
						airport->runways.push_back(std::move(runway));
						return;
					}
					case 101:
					{
						if (!RequireTokens(9, code))
							return;
						AptWaterRunway runway;
						runway.width = Float(1);
						runway.buoys = Bool(2);
						for (int i = 0; i < 2; ++i)
						{
							runway.ends[i].designator = std::string(Token(3 + i * 3));
							runway.ends[i].latitude = Double(4 + i * 3);
							runway.ends[i].longitude = Double(5 + i * 3);
						}
						airport->waterRunways.push_back(std::move(runway));
						return;
					}
					case 102:
					{
						if (!RequireTokens(12, code))
							return;
						AptHelipad helipad;
						helipad.designator = std::string(Token(1));
						helipad.latitude = Double(2);
						helipad.longitude = Double(3);
						helipad.heading = Float(4);
						helipad.length = Float(5);
						helipad.width = Float(6);
						helipad.surface = Int(7);
						helipad.markings = Int(8);
						helipad.shoulder = Int(9);
						helipad.smoothness = Float(10);
						helipad.edgeLights = Int(11);
						airport->helipads.push_back(std::move(helipad));
						return;
					}
					case 110:
					{
						AptPavement pavement;
						pavement.surface = Int(1, SurfaceAsphalt);
						pavement.smoothness = Float(2, 0.25f);
						pavement.textureHeading = Float(3);
						pavement.description = Text(line, 4);
						airport->pavements.push_back(std::move(pavement));
						chains = &airport->pavements.back().rings;
						return;
					}
					case 120:
					{
						AptLinearFeature feature;
						feature.description = Text(line, 1);
						airport->linearFeatures.push_back(std::move(feature));
						chains = &airport->linearFeatures.back().chains;
						return;
					}
					case 130:
					{
						AptBoundary boundary;
						boundary.description = Text(line, 1);
						airport->boundaries.push_back(std::move(boundary));
						chains = &airport->boundaries.back().rings;
						return;
					}
					case 14:
					{
						if (!RequireTokens(4, code))
							return;
						AptViewpoint viewpoint;
						viewpoint.latitude = Double(1);
						viewpoint.longitude = Double(2);
						viewpoint.heightFt = Float(3);
						viewpoint.name = Text(line, 5);
						if (airport->viewpoint)
							Warn("Second viewpoint (14) replaces the first");
						airport->viewpoint = std::move(viewpoint);
						return;
					}
					case 15:
					{
						if (!RequireTokens(4, code))
							return;
						AptLegacyStartup startup;
						startup.latitude = Double(1);
						startup.longitude = Double(2);
						startup.heading = Float(3);
						startup.name = Text(line, 4);
						airport->legacyStartups.push_back(std::move(startup));
						return;
					}
					case 18:
					{
						if (!RequireTokens(4, code))
							return;
						AptBeacon beacon;
						beacon.latitude = Double(1);
						beacon.longitude = Double(2);
						beacon.type = Int(3);
						beacon.name = Text(line, 4);
						if (airport->beacon)
							Warn("Second beacon (18) replaces the first");
						airport->beacon = std::move(beacon);
						return;
					}
					case 19:
					{
						if (!RequireTokens(4, code))
							return;
						AptWindsock windsock;
						windsock.latitude = Double(1);
						windsock.longitude = Double(2);
						windsock.lit = Bool(3);
						windsock.name = Text(line, 4);
						airport->windsocks.push_back(std::move(windsock));
						return;
					}
					case 20:
					{
						if (!RequireTokens(7, code))
							return;
						AptSign sign;
						sign.latitude = Double(1);
						sign.longitude = Double(2);
						sign.heading = Float(3);
						sign.size = Int(5);
						sign.text = Text(line, 6);
						airport->signs.push_back(std::move(sign));
						return;
					}
					case 21:
					{
						if (!RequireTokens(6, code))
							return;
						AptLightFixture fixture;
						fixture.latitude = Double(1);
						fixture.longitude = Double(2);
						fixture.type = Int(3);
						fixture.heading = Float(4);
						fixture.glideslope = Float(5);
						fixture.name = Text(line, 6);
						airport->lightFixtures.push_back(std::move(fixture));
						return;
					}
					case 50: case 51: case 52: case 53: case 54: case 55: case 56:
					case 1050: case 1051: case 1052: case 1053: case 1054: case 1055: case 1056:
					{
						if (!RequireTokens(2, code))
							return;
						ParseFrequency(code, line);
						return;
					}
					case 1000:
					{
						AptFlow flow;
						flow.name = Text(line, 1);
						airport->flows.push_back(std::move(flow));
						return;
					}
					case 1001:
					{
						AptFlow *flow = CurrentFlow(code);
						if (!flow || !RequireTokens(5, code))
							return;
						AptWindRule rule;
						rule.station = std::string(Token(1));
						rule.minDirection = Int(2);
						rule.maxDirection = Int(3);
						rule.maxSpeedKnots = Int(4);
						flow->windRules.push_back(std::move(rule));
						return;
					}
					case 1002:
					{
						AptFlow *flow = CurrentFlow(code);
						if (!flow || !RequireTokens(3, code))
							return;
						flow->ceilingRules.push_back({ std::string(Token(1)), Int(2) });
						return;
					}
					case 1003:
					{
						AptFlow *flow = CurrentFlow(code);
						if (!flow || !RequireTokens(3, code))
							return;
						flow->visibilityRules.push_back({ std::string(Token(1)), Float(2) });
						return;
					}
					case 1004:
					{
						AptFlow *flow = CurrentFlow(code);
						if (!flow || !RequireTokens(3, code))
							return;
						flow->timeRules.push_back({ Int(1), Int(2) });
						return;
					}
					case 1100:
					case 1110:
					{
						AptFlow *flow = CurrentFlow(code);
						if (!flow || !RequireTokens(7, code))
							return;
						AptRunwayUse use;
						use.highResolution = code == 1110;
						use.runway = std::string(Token(1));
						use.frequencyKhz = use.highResolution ? Int(2) : Int(2) * 10;
						const std::string_view traffic = Token(3);
						use.arrivals = traffic.find("arrivals") != std::string_view::npos;
						use.departures = traffic.find("departures") != std::string_view::npos;
						use.aircraftTypes = std::string(Token(4));
						ParseHeadingRange(Token(5), use.onCourseFrom, use.onCourseTo);
						ParseHeadingRange(Token(6), use.departureFrom, use.departureTo);
						use.name = Text(line, 7);
						flow->runwayUses.push_back(std::move(use));
						return;
					}
					case 1101:
					{
						AptFlow *flow = CurrentFlow(code);
						if (!flow || !RequireTokens(3, code))
							return;
						flow->vfrPatterns.push_back({ std::string(Token(1)), Token(2) == "right" });
						return;
					}
					case 1200:
					{
						if (!airport->taxiNetwork)
							airport->taxiNetwork.emplace();
						lastEdge = nullptr;
						return;
					}
					case 1201:
					{
						if (!airport->taxiNetwork)
							airport->taxiNetwork.emplace();
						if (!RequireTokens(5, code))
							return;
						AptTaxiNode node;
						node.latitude = Double(1);
						node.longitude = Double(2);
						node.usage = std::string(Token(3));
						node.id = Int(4);
						node.name = Text(line, 5);
						airport->taxiNetwork->nodes.push_back(std::move(node));
						return;
					}
					case 1202:
					case 1206:
					{
						if (!airport->taxiNetwork)
							airport->taxiNetwork.emplace();
						if (!RequireTokens(4, code))
							return;
						AptTaxiEdge edge;
						edge.groundVehicles = code == 1206;
						edge.from = Int(1);
						edge.to = Int(2);
						edge.oneWay = Token(3) == "oneway";
						edge.restriction = std::string(Token(4));
						edge.name = Text(line, 5);
						airport->taxiNetwork->edges.push_back(std::move(edge));
						lastEdge = &airport->taxiNetwork->edges.back();
						return;
					}
					case 1204:
					{
						if (!lastEdge)
						{
							Warn("Active zone (1204) without a preceding edge, ignored");
							return;
						}
						if (!RequireTokens(3, code))
							return;
						lastEdge->activeZones.push_back({ std::string(Token(1)), std::string(Token(2)) });
						return;
					}
					case 1300:
					{
						if (!RequireTokens(6, code))
							return;
						AptStartup startup;
						startup.latitude = Double(1);
						startup.longitude = Double(2);
						startup.heading = Float(3);
						startup.type = std::string(Token(4));
						startup.aircraftTypes = std::string(Token(5));
						startup.name = Text(line, 6);
						airport->startups.push_back(std::move(startup));
						lastStartup = &airport->startups.back();
						return;
					}
					case 1301:
					{
						if (!lastStartup)
						{
							Warn("Startup metadata (1301) without a preceding 1300, ignored");
							return;
						}
						lastStartup->hasMetadata = true;
						lastStartup->widthCode = std::string(Token(1));
						lastStartup->operationType = std::string(Token(2));
						lastStartup->airlines = Text(line, 3);
						return;
					}
					case 1302:
					{
						if (!RequireTokens(2, code))
							return;
						airport->metadata.push_back({ std::string(Token(1)), Text(line, 2) });
						return;
					}
					case 1400:
					{
						if (!RequireTokens(6, code))
							return;
						AptTruckParking truck;
						truck.latitude = Double(1);
						truck.longitude = Double(2);
						truck.heading = Float(3);
						truck.type = std::string(Token(4));
						truck.count = Int(5);
						truck.name = Text(line, 6);
						airport->truckParkings.push_back(std::move(truck));
						lastTruck = &airport->truckParkings.back();
						return;
					}
					case 1401:
					{
						if (!RequireTokens(5, code))
							return;
						AptTruckDestination destination;
						destination.latitude = Double(1);
						destination.longitude = Double(2);
						destination.heading = Float(3);
						destination.truckTypes = std::string(Token(4));
						destination.name = Text(line, 5);
						airport->truckDestinations.push_back(std::move(destination));
						return;
					}
					case 1402:
					{
						if (!lastTruck)
						{
							Warn("Custom truck object (1402) without a preceding 1400, ignored");
							return;
						}
						lastTruck->customObject = Text(line, 1);
						return;
					}
					case 1500:
					{
						if (!RequireTokens(9, code))
							return;
						AptJetway jetway;
						jetway.latitude = Double(1);
						jetway.longitude = Double(2);
						jetway.heading = Float(3);
						jetway.style = Int(4);
						jetway.size = Int(5);
						jetway.reserved = Int(6);
						jetway.tunnelLength = Float(7);
						jetway.cabinHeading = Float(8);
						airport->jetways.push_back(std::move(jetway));
						lastJetway = &airport->jetways.back();
						return;
					}
					case 1501:
					case 1502:
					{
						if (!lastJetway)
						{
							Warn("Custom jetway object (" + std::to_string(code) + ") without a preceding 1500, ignored");
							return;
						}
						lastJetway->customObjects.push_back(Text(line, 1));
						return;
					}
					default:
					{
						if (options.keepUnknownRows)
							airport->unknownRows.emplace_back(Trim(line));
						else
							Warn("Unknown row code " + std::to_string(code) + ", skipped");
						return;
					}
				}
			}
		};
	}

	bool ParseAptDat(const std::string_view text, AptFile &out, std::string *error, const AptParseOptions &options)
	{
		const auto fail = [error](const std::string &message)
		{
			if (error)
				*error = message;
			return false;
		};

		LineReader reader(text);
		std::string_view line;

		// Header: a line with I or A (byte order marker of old files), then the version row.
		bool sawHeader = false;
		size_t rewindPosition = 0;
		{
			std::string_view first;
			bool any = false;
			while (reader.Next(first))
			{
				const std::string_view trimmed = Trim(first);
				if (trimmed.empty())
					continue;
				any = true;

				// Strip a UTF-8 BOM.
				std::string_view marker = trimmed;
				if (marker.size() >= 3 && static_cast<unsigned char>(marker[0]) == 0xEF && static_cast<unsigned char>(marker[1]) == 0xBB && static_cast<unsigned char>(marker[2]) == 0xBF)
					marker.remove_prefix(3);

				if (marker == "I" || marker == "A")
				{
					sawHeader = true;
					std::string_view versionLine;
					while (reader.Next(versionLine) && Trim(versionLine).empty()) {}
					std::vector<std::string_view> tokens;
					Tokenize(versionLine, tokens);
					if (tokens.empty())
						return fail("Missing version row after the header");
					out.version = ParseInt(tokens[0], 0);
					if (out.version < 700)
						return fail("Unsupported apt.dat version " + std::string(tokens[0]));
					out.headerComment = std::string(RestOfLine(versionLine, 1));
				}
				else if (!options.requireHeader)
				{
					// No header: the first row is an airport header; re-read from the start of this line.
					rewindPosition = static_cast<size_t>(first.data() - text.data());
				}
				else
				{
					return fail("Not an apt.dat file (expected an I or A header line)");
				}
				break;
			}
			if (!any)
				return fail("Empty file");
		}

		if (!sawHeader)
			reader = LineReader(text.substr(rewindPosition));

		Parser parser(out, options);
		while (reader.Next(line))
		{
			parser.lineNumber = reader.GetLineNumber();
			Tokenize(line, parser.tokens);
			if (parser.tokens.empty())
				continue;

			const std::string_view first = parser.tokens[0];
			if (first.size() >= 1 && first[0] == '#')
				continue;

			int code = 0;
			const auto [ptr, ec] = std::from_chars(first.data(), first.data() + first.size(), code);
			if (ec != std::errc() || ptr != first.data() + first.size())
			{
				parser.Warn("Row doesn't start with a row code: '" + std::string(first) + "'");
				continue;
			}

			if (code == 99)
				break;
			parser.ParseRow(code, line);
		}

		return true;
	}

	bool LoadAptDat(const std::filesystem::path &path, AptFile &out, std::string *error)
	{
		std::string text;
		if (!ReadTextFile(path, text))
		{
			if (error)
				*error = "Could not read " + path.string();
			return false;
		}
		return ParseAptDat(text, out, error);
	}

	// -------------------------------------------------------
	// Writer
	// -------------------------------------------------------

	namespace
	{
		void WriteNodes(std::string &out, const std::vector<AptNodeChain> &chains)
		{
			for (const AptNodeChain &chain : chains)
			{
				const size_t count = chain.nodes.size();
				for (size_t i = 0; i < count; ++i)
				{
					const AptNode &node = chain.nodes[i];
					const bool last = i + 1 == count;
					int code = 111;
					if (last)
						code = chain.closed ? 113 : 115;
					if (node.hasControl)
						code += 1;

					Append(out, "%d", code);
					AppendCoord(out, node.latitude, node.longitude);
					if (node.hasControl)
						AppendCoord(out, node.controlLatitude, node.controlLongitude);

					// Open-string terminators (115 / 116) carry no style.
					if (!(last && !chain.closed))
					{
						if (node.lineType != 0)
							Append(out, " %d", node.lineType);
						if (node.lightType != 0)
							Append(out, " %d", node.lightType);
					}
					out.push_back('\n');
				}
			}
		}

		void WriteRunwayEnd(std::string &out, const AptRunwayEnd &end)
		{
			AppendText(out, end.designator);
			AppendCoord(out, end.latitude, end.longitude);
			Append(out, " %.2f %.2f %d %d %d %d", end.displacedThreshold, end.blastPad, end.marking, end.approachLights, end.touchdownZoneLights ? 1 : 0, end.reil);
		}
	}

	void WriteAptAirport(std::string &out, const AptAirport &airport)
	{
		Append(out, "%d %6d 0 0 ", static_cast<int>(airport.type), airport.elevationFt);
		out.append(airport.id);
		AppendText(out, airport.name);
		out.push_back('\n');

		for (const AptMetadata &entry : airport.metadata)
		{
			out.append("1302 ");
			out.append(entry.key);
			AppendText(out, entry.value);
			out.push_back('\n');
		}

		for (const AptRunway &runway : airport.runways)
		{
			Append(out, "100 %.2f %d %d %.2f %d %d %d", runway.width, runway.surface, runway.shoulder, runway.smoothness,
				runway.centerlineLights ? 1 : 0, runway.edgeLights, runway.distanceSigns ? 1 : 0);
			WriteRunwayEnd(out, runway.ends[0]);
			WriteRunwayEnd(out, runway.ends[1]);
			out.push_back('\n');
		}

		for (const AptWaterRunway &runway : airport.waterRunways)
		{
			Append(out, "101 %.2f %d", runway.width, runway.buoys ? 1 : 0);
			for (const AptWaterRunway::End &end : runway.ends)
			{
				AppendText(out, end.designator);
				AppendCoord(out, end.latitude, end.longitude);
			}
			out.push_back('\n');
		}

		for (const AptHelipad &helipad : airport.helipads)
		{
			out.append("102 ");
			out.append(helipad.designator);
			AppendCoord(out, helipad.latitude, helipad.longitude);
			Append(out, " %.2f %.2f %.2f %d %d %d %.2f %d\n", helipad.heading, helipad.length, helipad.width, helipad.surface, helipad.markings,
				helipad.shoulder, helipad.smoothness, helipad.edgeLights);
		}

		for (const AptPavement &pavement : airport.pavements)
		{
			Append(out, "110 %d %.2f %.2f", pavement.surface, pavement.smoothness, pavement.textureHeading);
			AppendText(out, pavement.description);
			out.push_back('\n');
			WriteNodes(out, pavement.rings);
		}

		for (const AptLinearFeature &feature : airport.linearFeatures)
		{
			out.append("120");
			AppendText(out, feature.description);
			out.push_back('\n');
			WriteNodes(out, feature.chains);
		}

		for (const AptBoundary &boundary : airport.boundaries)
		{
			out.append("130");
			AppendText(out, boundary.description);
			out.push_back('\n');
			WriteNodes(out, boundary.rings);
		}

		if (airport.viewpoint)
		{
			out.append("14");
			AppendCoord(out, airport.viewpoint->latitude, airport.viewpoint->longitude);
			Append(out, " %.2f 0", airport.viewpoint->heightFt);
			AppendText(out, airport.viewpoint->name);
			out.push_back('\n');
		}

		for (const AptLegacyStartup &startup : airport.legacyStartups)
		{
			out.append("15");
			AppendCoord(out, startup.latitude, startup.longitude);
			Append(out, " %.2f", startup.heading);
			AppendText(out, startup.name);
			out.push_back('\n');
		}

		if (airport.beacon)
		{
			out.append("18");
			AppendCoord(out, airport.beacon->latitude, airport.beacon->longitude);
			Append(out, " %d", airport.beacon->type);
			AppendText(out, airport.beacon->name);
			out.push_back('\n');
		}

		for (const AptWindsock &windsock : airport.windsocks)
		{
			out.append("19");
			AppendCoord(out, windsock.latitude, windsock.longitude);
			Append(out, " %d", windsock.lit ? 1 : 0);
			AppendText(out, windsock.name);
			out.push_back('\n');
		}

		for (const AptSign &sign : airport.signs)
		{
			out.append("20");
			AppendCoord(out, sign.latitude, sign.longitude);
			Append(out, " %.2f 0 %d", sign.heading, sign.size);
			AppendText(out, sign.text);
			out.push_back('\n');
		}

		for (const AptLightFixture &fixture : airport.lightFixtures)
		{
			out.append("21");
			AppendCoord(out, fixture.latitude, fixture.longitude);
			Append(out, " %d %.2f %.2f", fixture.type, fixture.heading, fixture.glideslope);
			AppendText(out, fixture.name);
			out.push_back('\n');
		}

		for (const AptFrequency &frequency : airport.frequencies)
		{
			const int type = static_cast<int>(frequency.type);
			if (frequency.legacy)
				Append(out, "%d %d", 50 + type, frequency.frequencyKhz / 10);
			else
				Append(out, "%d %d", 1050 + type, frequency.frequencyKhz);
			AppendText(out, frequency.name);
			out.push_back('\n');
		}

		for (const AptFlow &flow : airport.flows)
		{
			out.append("1000");
			AppendText(out, flow.name);
			out.push_back('\n');
			for (const AptWindRule &rule : flow.windRules)
			{
				out.append("1001 ");
				out.append(rule.station);
				Append(out, " %03d %03d %d\n", rule.minDirection, rule.maxDirection, rule.maxSpeedKnots);
			}
			for (const AptCeilingRule &rule : flow.ceilingRules)
			{
				out.append("1002 ");
				out.append(rule.station);
				Append(out, " %d\n", rule.minCeilingFt);
			}
			for (const AptVisibilityRule &rule : flow.visibilityRules)
			{
				out.append("1003 ");
				out.append(rule.station);
				Append(out, " %.1f\n", rule.minVisibilitySm);
			}
			for (const AptTimeRule &rule : flow.timeRules)
				Append(out, "1004 %04d %04d\n", rule.startUtc, rule.endUtc);
			for (const AptRunwayUse &use : flow.runwayUses)
			{
				Append(out, "%d ", use.highResolution ? 1110 : 1100);
				out.append(use.runway);
				if (use.highResolution)
					Append(out, " %06d ", use.frequencyKhz);
				else
					Append(out, " %05d ", use.frequencyKhz / 10);
				if (use.arrivals && use.departures)
					out.append("arrivals|departures");
				else
					out.append(use.arrivals ? "arrivals" : "departures");
				AppendText(out, use.aircraftTypes);
				Append(out, " %03d%03d %03d%03d", use.onCourseFrom, use.onCourseTo, use.departureFrom, use.departureTo);
				AppendText(out, use.name);
				out.push_back('\n');
			}
			for (const AptVfrPattern &pattern : flow.vfrPatterns)
			{
				out.append("1101 ");
				out.append(pattern.runway);
				out.append(pattern.rightTraffic ? " right\n" : " left\n");
			}
		}

		if (airport.taxiNetwork)
		{
			out.append("1200\n");
			for (const AptTaxiNode &node : airport.taxiNetwork->nodes)
			{
				out.append("1201");
				AppendCoord(out, node.latitude, node.longitude);
				AppendText(out, node.usage);
				Append(out, " %d", node.id);
				AppendText(out, node.name);
				out.push_back('\n');
			}
			for (const AptTaxiEdge &edge : airport.taxiNetwork->edges)
			{
				Append(out, "%d %d %d %s", edge.groundVehicles ? 1206 : 1202, edge.from, edge.to, edge.oneWay ? "oneway" : "twoway");
				AppendText(out, edge.restriction);
				AppendText(out, edge.name);
				out.push_back('\n');
				for (const AptActiveZone &zone : edge.activeZones)
				{
					out.append("1204 ");
					out.append(zone.classification);
					AppendText(out, zone.runways);
					out.push_back('\n');
				}
			}
		}

		for (const AptStartup &startup : airport.startups)
		{
			out.append("1300");
			AppendCoord(out, startup.latitude, startup.longitude);
			Append(out, " %.2f", startup.heading);
			AppendText(out, startup.type);
			AppendText(out, startup.aircraftTypes);
			AppendText(out, startup.name);
			out.push_back('\n');
			if (startup.hasMetadata)
			{
				out.append("1301 ");
				out.append(startup.widthCode);
				AppendText(out, startup.operationType);
				AppendText(out, startup.airlines);
				out.push_back('\n');
			}
		}

		for (const AptTruckParking &truck : airport.truckParkings)
		{
			out.append("1400");
			AppendCoord(out, truck.latitude, truck.longitude);
			Append(out, " %.2f", truck.heading);
			AppendText(out, truck.type);
			Append(out, " %d", truck.count);
			AppendText(out, truck.name);
			out.push_back('\n');
			if (!truck.customObject.empty())
			{
				out.append("1402");
				AppendText(out, truck.customObject);
				out.push_back('\n');
			}
		}

		for (const AptTruckDestination &destination : airport.truckDestinations)
		{
			out.append("1401");
			AppendCoord(out, destination.latitude, destination.longitude);
			Append(out, " %.2f", destination.heading);
			AppendText(out, destination.truckTypes);
			AppendText(out, destination.name);
			out.push_back('\n');
		}

		for (const AptJetway &jetway : airport.jetways)
		{
			out.append("1500");
			AppendCoord(out, jetway.latitude, jetway.longitude);
			Append(out, " %.2f %d %d %d %.2f %.2f\n", jetway.heading, jetway.style, jetway.size, jetway.reserved, jetway.tunnelLength, jetway.cabinHeading);
			for (const std::string &object : jetway.customObjects)
			{
				out.append("1501");
				AppendText(out, object);
				out.push_back('\n');
			}
		}

		for (const std::string &row : airport.unknownRows)
		{
			out.append(row);
			out.push_back('\n');
		}
	}

	std::string WriteAptDat(const AptFile &file)
	{
		std::string out;
		out.reserve(4096 + file.airports.size() * 2048);
		Append(out, "I\n%d", file.version);
		if (!file.headerComment.empty())
			AppendText(out, file.headerComment);
		out.append("\n\n");

		for (const AptAirport &airport : file.airports)
		{
			WriteAptAirport(out, airport);
			out.push_back('\n');
		}

		out.append("99\n");
		return out;
	}

	bool SaveAptDat(const std::filesystem::path &path, const AptFile &file, std::string *error)
	{
		std::error_code ec;
		std::filesystem::create_directories(path.parent_path(), ec);

		// Write next to the target, then replace it, so a failure can't leave a half-written apt.dat behind.
		const std::filesystem::path temp = path.string() + ".tmp";
		{
			std::ofstream stream(temp, std::ios::binary | std::ios::trunc);
			if (!stream)
			{
				if (error)
					*error = "Could not write " + temp.string();
				return false;
			}
			const std::string text = WriteAptDat(file);
			stream.write(text.data(), static_cast<std::streamsize>(text.size()));
			if (!stream)
			{
				if (error)
					*error = "Write failed for " + temp.string();
				return false;
			}
		}

		std::filesystem::rename(temp, path, ec);
		if (ec)
		{
			// Some file systems refuse to rename over an existing file; fall back to copy + remove.
			std::filesystem::copy_file(temp, path, std::filesystem::copy_options::overwrite_existing, ec);
			std::error_code ignored;
			std::filesystem::remove(temp, ignored);
			if (ec)
			{
				if (error)
					*error = "Could not replace " + path.string() + ": " + ec.message();
				return false;
			}
		}
		return true;
	}

	// -------------------------------------------------------
	// Validation
	// -------------------------------------------------------

	std::vector<std::string> ValidateAptAirport(const AptAirport &airport)
	{
		std::vector<std::string> problems;
		const auto problem = [&problems](std::string text) { problems.push_back(std::move(text)); };
		const auto checkCoordinate = [&problem](const std::string &what, const double latitude, const double longitude)
		{
			if (!ValidLatitude(latitude) || !ValidLongitude(longitude))
				problem(what + ": coordinate out of range (" + std::to_string(latitude) + ", " + std::to_string(longitude) + ")");
		};

		if (airport.id.empty())
			problem("Airport id is empty");
		else if (airport.id.size() > 7)
			problem("Airport id '" + airport.id + "' is longer than 7 characters");
		if (airport.name.empty())
			problem("Airport name is empty");

		if (airport.type == AirportType::Land && airport.runways.empty() && airport.helipads.empty())
			problem("A land airport needs at least one runway or helipad");
		if (airport.type == AirportType::Seaplane && airport.waterRunways.empty())
			problem("A seaplane base needs at least one water runway");
		if (airport.type == AirportType::Heliport && airport.helipads.empty())
			problem("A heliport needs at least one helipad");

		std::set<std::string> runwayEnds;
		for (const AptRunway &runway : airport.runways)
		{
			const std::string name = "Runway " + runway.GetName();
			if (runway.width < 1.0f)
				problem(name + ": width must be at least 1 m");
			for (const AptRunwayEnd &end : runway.ends)
			{
				checkCoordinate(name, end.latitude, end.longitude);
				if (end.designator.empty())
					problem(name + ": an end has no designator");
				else if (!runwayEnds.insert(end.designator).second)
					problem(name + ": runway end '" + end.designator + "' is used twice");
				if (end.marking < 0 || end.marking > 7)
					problem(name + ": unknown marking code " + std::to_string(end.marking));
				if (end.approachLights < 0 || end.approachLights > 12)
					problem(name + ": unknown approach lighting code " + std::to_string(end.approachLights));
			}
			const double length = runway.GetLengthMeters();
			if (length < 1.0)
				problem(name + ": the two ends are at the same place");
			if (runway.ends[0].displacedThreshold + runway.ends[1].displacedThreshold > length)
				problem(name + ": displaced thresholds overlap");
			if (GetSurfaceFamily(runway.surface) == SurfaceFamily::Unknown)
				problem(name + ": unknown surface code " + std::to_string(runway.surface));
		}

		for (const AptWaterRunway &runway : airport.waterRunways)
		{
			const std::string name = "Water runway " + runway.GetName();
			if (runway.width < 1.0f)
				problem(name + ": width must be at least 1 m");
			for (const AptWaterRunway::End &end : runway.ends)
				checkCoordinate(name, end.latitude, end.longitude);
		}

		std::set<std::string> helipadNames;
		for (const AptHelipad &helipad : airport.helipads)
		{
			const std::string name = "Helipad " + helipad.designator;
			checkCoordinate(name, helipad.latitude, helipad.longitude);
			if (helipad.length < 1.0f || helipad.width < 1.0f)
				problem(name + ": length and width must be at least 1 m");
			if (!helipadNames.insert(helipad.designator).second)
				problem(name + ": designator used twice");
		}

		const auto checkRings = [&](const std::string &what, const std::vector<AptNodeChain> &rings, const bool mustClose)
		{
			if (rings.empty())
				problem(what + ": has no nodes");
			for (size_t i = 0; i < rings.size(); ++i)
			{
				const AptNodeChain &ring = rings[i];
				if (mustClose && !ring.closed)
					problem(what + ": ring " + std::to_string(i + 1) + " isn't closed");
				if (ring.nodes.size() < (mustClose ? 3u : 2u))
					problem(what + ": ring " + std::to_string(i + 1) + " has only " + std::to_string(ring.nodes.size()) + " nodes");
				for (const AptNode &node : ring.nodes)
					checkCoordinate(what, node.latitude, node.longitude);
			}
		};
		for (size_t i = 0; i < airport.pavements.size(); ++i)
			checkRings("Pavement " + std::to_string(i + 1) + (airport.pavements[i].description.empty() ? "" : " (" + airport.pavements[i].description + ")"), airport.pavements[i].rings, true);
		for (size_t i = 0; i < airport.linearFeatures.size(); ++i)
			checkRings("Line " + std::to_string(i + 1) + (airport.linearFeatures[i].description.empty() ? "" : " (" + airport.linearFeatures[i].description + ")"), airport.linearFeatures[i].chains, false);
		for (size_t i = 0; i < airport.boundaries.size(); ++i)
			checkRings("Boundary " + std::to_string(i + 1), airport.boundaries[i].rings, true);

		if (airport.viewpoint)
			checkCoordinate("Viewpoint", airport.viewpoint->latitude, airport.viewpoint->longitude);
		if (airport.beacon)
			checkCoordinate("Beacon", airport.beacon->latitude, airport.beacon->longitude);
		for (const AptWindsock &windsock : airport.windsocks)
			checkCoordinate("Windsock " + windsock.name, windsock.latitude, windsock.longitude);
		for (const AptSign &sign : airport.signs)
		{
			checkCoordinate("Sign " + sign.text, sign.latitude, sign.longitude);
			if (sign.text.empty())
				problem("Sign without text");
			if (sign.size < 1 || sign.size > 5)
				problem("Sign " + sign.text + ": unknown size code " + std::to_string(sign.size));
		}
		for (const AptLightFixture &fixture : airport.lightFixtures)
		{
			checkCoordinate("Light fixture " + fixture.name, fixture.latitude, fixture.longitude);
			if (fixture.type < 1 || fixture.type > 8)
				problem("Light fixture " + fixture.name + ": unknown type " + std::to_string(fixture.type));
		}

		for (const AptFrequency &frequency : airport.frequencies)
		{
			// COM band, plus the NAV band some airports broadcast ATIS on.
			if (frequency.frequencyKhz < 108000 || frequency.frequencyKhz > 136990)
				problem("Frequency " + frequency.name + ": " + std::to_string(frequency.frequencyKhz) + " kHz is outside the COM / NAV bands");
		}

		for (const AptStartup &startup : airport.startups)
		{
			// Names repeat in a lot of real data (X-Plane tolerates it), so that isn't flagged here.
			checkCoordinate("Startup " + startup.name, startup.latitude, startup.longitude);
			if (startup.name.empty())
				problem("Startup location without a name");
			if (std::ranges::find(GetStartupTypes(), startup.type, [](const char *text) { return std::string_view(text); }) == GetStartupTypes().end() && startup.type != "tie-down")
				problem("Startup " + startup.name + ": unknown type '" + startup.type + "'");
		}

		if (airport.taxiNetwork)
		{
			std::set<int> nodeIds;
			for (const AptTaxiNode &node : airport.taxiNetwork->nodes)
			{
				checkCoordinate("Taxi node " + std::to_string(node.id), node.latitude, node.longitude);
				if (!nodeIds.insert(node.id).second)
					problem("Taxi node id " + std::to_string(node.id) + " is used twice");
			}
			std::set<int> used;
			for (const AptTaxiEdge &edge : airport.taxiNetwork->edges)
			{
				if (!nodeIds.contains(edge.from))
					problem("Taxi edge " + edge.name + ": start node " + std::to_string(edge.from) + " doesn't exist");
				if (!nodeIds.contains(edge.to))
					problem("Taxi edge " + edge.name + ": end node " + std::to_string(edge.to) + " doesn't exist");
				used.insert(edge.from);
				used.insert(edge.to);
			}
			for (const int id : nodeIds)
			{
				if (!used.contains(id))
					problem("Taxi node " + std::to_string(id) + " isn't part of any edge");
			}
		}

		for (const AptTruckParking &truck : airport.truckParkings)
			checkCoordinate("Truck parking " + truck.name, truck.latitude, truck.longitude);
		for (const AptTruckDestination &destination : airport.truckDestinations)
			checkCoordinate("Truck destination " + destination.name, destination.latitude, destination.longitude);
		for (size_t i = 0; i < airport.jetways.size(); ++i)
		{
			const AptJetway &jetway = airport.jetways[i];
			const std::string name = "Jetway " + std::to_string(i + 1);
			// Style and size codes beyond the documented 0..3 occur in Global Airports, so only the position is checked.
			checkCoordinate(name, jetway.latitude, jetway.longitude);
		}

		return problems;
	}
}

// -------------------------------------------------------

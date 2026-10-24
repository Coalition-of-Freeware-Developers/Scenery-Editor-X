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
 * xp_gateway_package.cpp
 * -------------------------------------------------------
 * Created: 24/10/2026
 * -------------------------------------------------------
 */
#include "xp_gateway_package.h"
#include "xp_gateway_validation.h"
#include <SceneryEditorX/filesystem/vfs.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <map>
#include <numbers>
#include <optional>
#include <set>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	namespace
	{
#include "xp_gateway_license.inl"

		bool Fail(std::string *error, std::string message)
		{
			if (error)
				*error = std::move(message);
			return false;
		}

		bool EndsWithNoCase(const std::string_view text, const std::string_view suffix)
		{
			if (text.size() < suffix.size())
				return false;
			return std::equal(suffix.begin(), suffix.end(), text.end() - static_cast<std::ptrdiff_t>(suffix.size()), [](const char a, const char b)
			{
				const auto lower = [](const char c) { return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c; };
				return lower(a) == lower(b);
			});
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

		bool IsExclusion(const std::string &key)
		{
			return key.starts_with("sim/exclude_");
		}

		bool IsBounds(const std::string &key)
		{
			return key == "sim/west" || key == "sim/east" || key == "sim/north" || key == "sim/south";
		}

		const std::string &DefinitionName(const std::vector<std::string> &definitions, const uint32_t index)
		{
			static const std::string unknown;
			return index < definitions.size() ? definitions[index] : unknown;
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

			[[nodiscard]] bool Contains(const double latitude, const double longitude) const
			{
				return valid && latitude >= south && latitude <= north && longitude >= west && longitude <= east;
			}

			[[nodiscard]] Box Expanded(const double nm) const
			{
				Box out = *this;
				if (!valid)
					return out;
				const double dLat = nm / 60.0;
				const double dLon = dLat / std::max(0.01, std::cos(0.5 * (south + north) * std::numbers::pi / 180.0));
				out.south -= dLat;
				out.north += dLat;
				out.west -= dLon;
				out.east += dLon;
				return out;
			}
		};

		/* @brief The airport's rows and every placement (bezier control points aside). */
		Box ContentBox(const AptAirport &airport, const DsfFile &overlay)
		{
			Box box;
			double south, west, north, east;
			if (airport.GetBounds(south, west, north, east))
			{
				box.Add(south, west);
				box.Add(north, east);
			}
			for (const DsfObjectPlacement &object : overlay.objects)
				box.Add(object.latitude, object.longitude);
			for (const DsfPolygonPlacement &polygon : overlay.polygons)
			{
				for (const std::vector<DsfPoint> &winding : polygon.windings)
				{
					for (const DsfPoint &point : winding)
						box.Add(point.Latitude(), point.Longitude());
				}
			}
			for (const DsfRoadSegment &road : overlay.roads)
			{
				for (const DsfRoadPoint &point : road.points)
				{
					if (!point.control)
						box.Add(point.latitude, point.longitude);
				}
			}
			return box;
		}

		/* @brief Where an airport is: its boundary, else everything in its rows. */
		Box AirportExtent(const AptAirport &airport)
		{
			Box extent;
			for (const AptBoundary &boundary : airport.boundaries)
			{
				for (const AptNodeChain &ring : boundary.rings)
				{
					for (const AptNode &node : ring.nodes)
						extent.Add(node.latitude, node.longitude);
				}
			}
			double south, west, north, east;
			if (!extent.valid && airport.GetBounds(south, west, north, east))
			{
				extent.Add(south, west);
				extent.Add(north, east);
			}
			return extent;
		}

		/* @brief A placement's definition index in `into`, adding the name when it's new. */
		std::vector<uint32_t> MapDefinitions(std::vector<std::string> &into, const std::vector<std::string> &from)
		{
			std::vector<uint32_t> map(from.size());
			for (size_t i = 0; i < from.size(); ++i)
			{
				const auto found = std::ranges::find(into, from[i]);
				map[i] = static_cast<uint32_t>(found - into.begin());
				if (found == into.end())
					into.push_back(from[i]);
			}
			return map;
		}
	}

	// -------------------------------------------------------

	std::string GatewayContentSummary::GetFeatures() const
	{
		std::string out;
		const auto add = [&out](const bool present, const int id)
		{
			if (present)
				out += (out.empty() ? "" : ",") + std::to_string(id);
		};
		add(atcFlows, GatewayFeatureAtcFlow);
		add(taxiRoutes, GatewayFeatureTaxiRoutes);
		add(truckRoutes, GatewayFeatureTruckRoutes);
		add(roads, GatewayFeatureRoads);
		return out;
	}

	std::vector<std::string> GatewayContentSummary::GetFeatureNames() const
	{
		std::vector<std::string> out;
		if (atcFlows)
			out.emplace_back("ATC flows");
		if (taxiRoutes)
			out.emplace_back("Taxi routes");
		if (truckRoutes)
			out.emplace_back("Truck routes");
		if (roads)
			out.emplace_back("Roads");
		return out;
	}

	GatewayContentSummary SummarizeGatewayContent(const AptAirport &airport, const DsfFile &overlay)
	{
		GatewayContentSummary out;
		out.runways = airport.runways.size();
		out.waterRunways = airport.waterRunways.size();
		out.helipads = airport.helipads.size();
		out.pavements = airport.pavements.size();
		out.lines = airport.linearFeatures.size();
		out.startups = airport.startups.size();
		out.jetways = airport.jetways.size();
		out.atcFlows = !airport.flows.empty();
		if (airport.taxiNetwork)
		{
			for (const AptTaxiEdge &edge : airport.taxiNetwork->edges)
			{
				out.taxiRoutes |= !edge.groundVehicles;
				out.truckRoutes |= edge.groundVehicles;
			}
		}

		for (const DsfObjectPlacement &object : overlay.objects)
		{
			if (EndsWithNoCase(DefinitionName(overlay.objectDefinitions, object.definition), ".agp"))
				++out.autogenPoints;
			else
				++out.objects;
		}
		for (const DsfPolygonPlacement &polygon : overlay.polygons)
		{
			const std::string &name = DefinitionName(overlay.polygonDefinitions, polygon.definition);
			if (EndsWithNoCase(name, ".fac"))
				++out.facades;
			else if (EndsWithNoCase(name, ".for"))
				++out.forests;
			else if (EndsWithNoCase(name, ".str"))
				++out.strings;
			else if (EndsWithNoCase(name, ".lin"))
				++out.linePlacements;
			else if (EndsWithNoCase(name, ".ags") || EndsWithNoCase(name, ".agb"))
				++out.autogen;
			else if (polygon.parameter == 65535)
				++out.orthophotos;
			else
				++out.polygons;
		}
		out.roadSegments = overlay.roads.size();
		out.roads = out.roadSegments > 0;
		out.exclusions = static_cast<size_t>(std::ranges::count_if(overlay.properties, [](const auto &property) { return IsExclusion(property.first); }));

		// WorldEditor's "3D": things that stick up (objects incl. .agp, facades, forests, object strings); jetways too
		out.is3D = out.objects + out.autogenPoints + out.facades + out.forests + out.strings + out.jetways > 0;
		out.hasDsf = !overlay.objects.empty() || !overlay.polygons.empty() || !overlay.roads.empty() || out.exclusions > 0;
		return out;
	}

	void ApplyGatewayGuiLabel(AptAirport &airport, const bool is3D)
	{
		airport.SetMetadata("gui_label", is3D ? "3D" : "2D");
	}

	bool BuildGatewayPackage(const GatewayPackageInput &input, GatewayPackage &out, std::string *error)
	{
		out = {};
		const std::string &icao = input.airport.id;
		if (icao.empty() || !std::ranges::all_of(icao, [](const char c) { return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); }))
			return Fail(error, "The airport ID '" + icao + "' can't name the Gateway files: use ASCII letters and digits");

		out.summary = SummarizeGatewayContent(input.airport, input.overlay);
		out.airport = input.airport;
		ApplyGatewayGuiLabel(out.airport, out.summary.is3D);

		AptFile apt;
		apt.headerComment = "Generated by " + input.creationAgent;
		apt.airports.push_back(out.airport);
		out.aptDat = WriteAptDat(apt);

		std::vector<ZipFile> pack;
		pack.push_back({ "Earth nav data/apt.dat", out.aptDat });

		if (out.summary.hasDsf)
		{
			// Every placement belongs to this airport: filter 0, the one sim/filter/aptid; its exclusions follow it
			const DsfFile &overlay = input.overlay;
			DsfFile dsf;
			dsf.objectDefinitions = overlay.objectDefinitions;
			dsf.polygonDefinitions = overlay.polygonDefinitions;
			dsf.networkDefinitions = overlay.networkDefinitions;
			dsf.objects = overlay.objects;
			dsf.polygons = overlay.polygons;
			dsf.roads = overlay.roads;
			for (DsfObjectPlacement &object : dsf.objects)
				object.filter = 0;
			for (DsfPolygonPlacement &polygon : dsf.polygons)
				polygon.filter = 0;
			for (DsfRoadSegment &road : dsf.roads)
				road.filter = 0;
			std::vector<std::pair<std::string, std::string>> exclusions;
			for (const auto &property : overlay.properties)
			{
				if (IsExclusion(property.first))
					exclusions.push_back(property);
			}

			// <ICAO>.txt: WorldEditor's airport overlay header covers the whole world
			dsf.properties = { { "sim/west", "-180" }, { "sim/east", "180" }, { "sim/north", "90" }, { "sim/south", "-90" }, { "sim/planet", "earth" },
				{ "sim/creation_agent", input.creationAgent }, { "sim/overlay", "1" }, { "sim/filter/aptid", ToUpperAscii(icao) } };
			dsf.properties.insert(dsf.properties.end(), exclusions.begin(), exclusions.end());
			dsf.west = -180.0;
			dsf.south = -90.0;
			dsf.east = 180.0;
			dsf.north = 90.0;
			out.dsfText = WriteDsfText(dsf, input.creationAgent);

			// The pack: one binary tile per 1x1 degree, bounds first like WorldEditor's tiles
			dsf.properties.erase(dsf.properties.begin(), dsf.properties.begin() + 4);
			for (DsfFile &tile : SplitDsfByTile(dsf, &out.warnings))
			{
				std::stable_partition(tile.properties.begin(), tile.properties.end(), [](const auto &property) { return IsBounds(property.first); });
				std::vector<uint8_t> bytes;
				std::string reason;
				const std::string name = GetDsfTileName(tile.south + 0.5, tile.west + 0.5);
				if (!WriteDsf(tile, bytes, &reason))
					return Fail(error, "DSF tile " + name + ": " + reason);
				pack.push_back({ "Earth nav data/" + GetDsfTileFolderName(tile.south + 0.5, tile.west + 0.5) + "/" + name + ".dsf", std::string(bytes.begin(), bytes.end()) });
			}
		}

		pack.push_back({ "README.txt", MakeGatewayReadme(out.airport.name, icao, input.uploader, input.comments) });
		pack.push_back({ "COPYING", GetGatewayLicenseText() });
		out.packFiles = std::move(pack);

		GatewaySubmission &submission = out.submission;
		submission.icao = icao;
		submission.airportName = out.airport.name;
		submission.artistComments = input.comments;
		submission.parentId = input.parentId;
		submission.is3D = out.summary.is3D;
		submission.features = out.summary.GetFeatures();
		submission.validatedAgainst = input.validatedAgainst.empty() ? std::string("Unknown") : input.validatedAgainst;
		submission.clientVersion = input.clientVersion;
		const Box box = ContentBox(out.airport, input.overlay);
		if (box.valid)
		{
			submission.latitude = 0.5 * (box.south + box.north);
			submission.longitude = 0.5 * (box.west + box.east);
		}
		for (const AptMetadata &row : out.airport.metadata)
		{
			if (std::ranges::binary_search(GetGatewayMetadataKeys(), std::string_view(row.key)))
				submission.metadata[row.key] = row.value;
		}
		submission.masterZip = MakeGatewayMasterZip(icao, out.aptDat, out.dsfText, out.packFiles, ZipMethod::Deflated);
		return true;
	}

	std::string MakeGatewayReadme(const std::string_view airportName, const std::string_view icao, const std::string_view uploader, const std::string_view comments)
	{
		const std::string title = std::string(airportName) + " (" + std::string(icao) + ")";
		const std::string rule(79, '-');
		std::string out;
		out += rule + "\n" + title + "\n" + rule + "\n\n";
		out += "This scenery pack was downloaded from the X-Plane Scenery Gateway:\n\n";
		out += "    https://gateway.x-plane.com/\n\n";
		out += "Airport: " + title + "\n\nUploaded by: " + std::string(uploader) + ".\n\n";
		out += "Author's Comments:\n\n" + std::string(comments) + "\n\n";
		out += "Installation Instructions:\n\n";
		out += "To install this scenery, drag this entire folder into X-Plane's Custom Scenery\n";
		out += "folder and re-start X-Plane.\n\n";
		out += "The scenery packs shared via the X-Plane Scenery Gateway are free software; you\n";
		out += "can redistribute it and/or modify it under the terms of the GNU General Public\n";
		out += "License as published by the Free Software Foundation; either version 2 of the\n";
		out += "License, or (at your option) any later version.  See the included COPYING file\n";
		out += "for complete terms.\n";
		return out;
	}

	const std::string &GetGatewayLicenseText()
	{
		static const std::string text = []
		{
			std::string joined;
			for (const char *part : GPL_V2_TEXT_PARTS)
				joined += part;
			return joined;
		}();
		return text;
	}

	std::string ReadXPlaneBuild(const std::string_view logText)
	{
		// WorldEditor: the version is the word after the first "X-Plane", when that's within the first 100 characters
		const size_t at = logText.find("X-Plane");
		if (at == std::string_view::npos || at >= 100 || at + 8 > logText.size())
			return {};
		std::string_view rest = logText.substr(at + 8);
		const size_t start = rest.find_first_not_of(" \t");
		if (start == std::string_view::npos)
			return {};
		rest.remove_prefix(start);
		const size_t end = rest.find_first_of(" \t\r\n");
		return std::string(rest.substr(0, std::min<size_t>(end == std::string_view::npos ? rest.size() : end, 63)));
	}

	std::string LoadXPlaneBuild(const std::filesystem::path &xplaneRoot)
	{
		const std::optional<std::filesystem::path> log = VFS::FindCaseInsensitive(xplaneRoot / "Log.txt");
		if (!log)
			return {};
		std::ifstream file(*log, std::ios::binary);
		char head[256] = {};
		file.read(head, sizeof(head));
		return ReadXPlaneBuild(std::string_view(head, static_cast<size_t>(file.gcount())));
	}

	void AppendDsfPlacements(DsfFile &into, const DsfFile &from)
	{
		const std::vector<uint32_t> objects = MapDefinitions(into.objectDefinitions, from.objectDefinitions);
		const std::vector<uint32_t> polygons = MapDefinitions(into.polygonDefinitions, from.polygonDefinitions);
		const std::vector<uint32_t> networks = MapDefinitions(into.networkDefinitions, from.networkDefinitions);
		for (DsfObjectPlacement object : from.objects)
		{
			if (object.definition >= objects.size())
				continue;
			object.definition = objects[object.definition];
			object.filter = -1;
			into.objects.push_back(object);
		}
		for (DsfPolygonPlacement polygon : from.polygons)
		{
			if (polygon.definition >= polygons.size())
				continue;
			polygon.definition = polygons[polygon.definition];
			polygon.filter = -1;
			into.polygons.push_back(std::move(polygon));
		}
		uint32_t junctionOffset = 0;
		for (const DsfRoadSegment &road : into.roads)
			junctionOffset = std::max({ junctionOffset, road.startJunction, road.endJunction });
		for (DsfRoadSegment road : from.roads)
		{
			if (road.definition >= networks.size())
				continue;
			road.definition = networks[road.definition];
			road.startJunction += junctionOffset;
			road.endJunction += junctionOffset;
			road.filter = -1;
			into.roads.push_back(std::move(road));
		}
	}

	void AppendDsf(DsfFile &into, const DsfFile &from)
	{
		AppendDsfPlacements(into, from);
		for (const auto &property : from.properties)
		{
			if (IsExclusion(property.first) && std::ranges::find(into.properties, property) == into.properties.end())
				into.properties.push_back(property);
		}
	}

	DsfFile SelectAirportPlacements(const DsfFile &tile, const AptAirport &airport, size_t *leftOut)
	{
		DsfFile out;
		out.objectDefinitions = tile.objectDefinitions;
		out.polygonDefinitions = tile.polygonDefinitions;
		out.networkDefinitions = tile.networkDefinitions;
		const Box nearby = AirportExtent(airport).Expanded(GATEWAY_BOUNDARY_MARGIN_NM);
		const std::string id = ToUpperAscii(airport.id);

		// Filters: index of this airport's sim/filter/aptid; exclusions after it (up to the next one) are its own, those
		// before the first one belong to no airport (all of them do in a tile without filters)
		int32_t filter = -1, filters = 0, section = -1;
		std::vector<std::pair<std::string, std::string>> unfiltered;
		for (const auto &[key, value] : tile.properties)
		{
			if (key == "sim/filter/aptid")
			{
				section = filters++;
				if (ToUpperAscii(value) == id)
					filter = section;
			}
			else if (IsExclusion(key) && filters == 0)
				unfiltered.emplace_back(key, value);
			else if (IsExclusion(key) && filter >= 0 && section == filter)
				out.properties.emplace_back(key, value);
		}
		if (filters == 0)
			out.properties = std::move(unfiltered);

		size_t skipped = 0;
		const auto keep = [&](const int32_t placementFilter, const double latitude, const double longitude)
		{
			const bool mine = filters > 0 ? placementFilter >= 0 && placementFilter == filter : nearby.Contains(latitude, longitude);
			skipped += mine ? 0 : 1;
			return mine;
		};
		for (const DsfObjectPlacement &object : tile.objects)
		{
			if (keep(object.filter, object.latitude, object.longitude))
				out.objects.push_back(object);
		}
		for (const DsfPolygonPlacement &polygon : tile.polygons)
		{
			if (!polygon.windings.empty() && !polygon.windings[0].empty() && keep(polygon.filter, polygon.windings[0][0].Latitude(), polygon.windings[0][0].Longitude()))
				out.polygons.push_back(polygon);
		}
		for (const DsfRoadSegment &road : tile.roads)
		{
			if (!road.points.empty() && keep(road.filter, road.points.front().latitude, road.points.front().longitude))
				out.roads.push_back(road);
		}
		if (leftOut)
			*leftOut += skipped;
		return out;
	}

	DsfFile LoadPackOverlay(const std::filesystem::path &packFolder, const AptAirport &airport, std::vector<std::string> *warnings)
	{
		DsfFile out;
		const Box nearby = AirportExtent(airport).Expanded(GATEWAY_BOUNDARY_MARGIN_NM);
		if (!nearby.valid)
			return out;

		size_t leftOut = 0;
		for (int lat = static_cast<int>(std::floor(nearby.south)); lat <= static_cast<int>(std::floor(nearby.north)); ++lat)
		{
			for (int lon = static_cast<int>(std::floor(nearby.west)); lon <= static_cast<int>(std::floor(nearby.east)); ++lon)
			{
				const std::optional<std::filesystem::path> path = VFS::FindCaseInsensitive(GetDsfTilePath(packFolder, lat + 0.5, lon + 0.5));
				if (!path)
					continue;
				DsfFile tile;
				std::string error;
				if (!LoadDsf(*path, tile, &error))
				{
					if (warnings)
						warnings->push_back("DSF " + path->filename().string() + ": " + error);
					continue;
				}
				AppendDsf(out, SelectAirportPlacements(tile, airport, &leftOut));
			}
		}
		if (leftOut > 0 && warnings)
			warnings->push_back(std::to_string(leftOut) + " placements in the pack's DSF tiles belong to no airport (or another one) and were left out");
		return out;
	}
}

// -------------------------------------------------------

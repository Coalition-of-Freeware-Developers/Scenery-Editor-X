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
 * project_gateway.cpp
 * -------------------------------------------------------
 * Created: 24/10/2026
 * -------------------------------------------------------
 */
#include "project_gateway.h"
#include "project_dsf.h"
#include <SceneryEditorX/xplane/xp_gateway_package.h>
#include <SceneryEditorX/xplane/xp_zip.h>
#include <algorithm>

// -------------------------------------------------------

namespace SceneryEditorX::ProjectFile
{
	namespace
	{
		bool Fail(std::string *error, std::string message)
		{
			if (error)
				*error = std::move(message);
			return false;
		}

		std::string ToLowerAscii(std::string text)
		{
			for (char &c : text)
			{
				if (c >= 'A' && c <= 'Z')
					c = static_cast<char>(c - 'A' + 'a');
			}
			return text;
		}

		/* @brief The Airport item placing an airport, and whether it (or a container above it) is hidden. */
		template<typename Items>
		auto FindAirportItem(Items &items, const std::string_view airportId, const bool parentHidden, bool &hidden) -> decltype(items.data())
		{
			for (auto &item : items)
			{
				const bool itemHidden = parentHidden || item.hidden;
				if (item.kind == ItemKind::Airport && item.airportId == airportId)
				{
					hidden = itemHidden;
					return &item;
				}
				if (item.IsContainer())
				{
					if (auto *found = FindAirportItem(item.children, airportId, itemHidden, hidden))
						return found;
				}
			}
			return nullptr;
		}

		/* @brief Visible items that no airport's hierarchy holds (WorldEditor uploads only the airport's own). */
		void CollectOutside(const std::vector<SceneryItem> &items, const bool parentHidden, std::vector<std::string> &out)
		{
			for (const SceneryItem &item : items)
			{
				if (parentHidden || item.hidden || item.kind == ItemKind::Airport || item.kind == ItemKind::Raw)
					continue;
				if (item.kind == ItemKind::Group)
				{
					CollectOutside(item.children, false, out);
					continue;
				}
				out.push_back(!item.name.empty() ? item.name : std::string(GetItemKindName(item.kind)) + (item.resource.empty() ? "" : " " + item.resource));
			}
		}
	}

	// -------------------------------------------------------

	bool CollectGatewayContent(const SceneryProject &project, const std::string_view airportId, GatewayContent &out, std::string *error)
	{
		out = {};
		const ProjectAirport *airport = project.FindAirport(airportId);
		if (!airport)
			return Fail(error, "The project has no airport '" + std::string(airportId) + "'");
		out.airport = airport->airport;

		bool hidden = false;
		const SceneryItem *item = FindAirportItem(project.items, airportId, false, hidden);
		if (item && hidden)
			return Fail(error, "Airport " + std::string(airportId) + " is hidden in the project: show it to upload it");
		CollectOutside(project.items, false, out.outside);

		if (item)
		{
			SceneryProject subset;
			subset.libraries = project.libraries;
			subset.items = item->children;
			DsfConversionOptions options;
			options.includeObjects = true;
			DsfConversion conversion = BuildProjectDsf(subset, options);
			out.overlay = std::move(conversion.dsf);
			out.warnings = std::move(conversion.warnings);
			out.hidden = conversion.hidden;
		}
		return true;
	}

	int GetGatewaySceneryId(const ProjectAirport &airport, const bool officialServer, const std::string_view serverKey)
	{
		if (officialServer)
			return std::max(0, airport.sceneryId);
		const auto found = std::ranges::find(airport.testSceneryIds, serverKey, &GatewayTestSceneryId::server);
		return found != airport.testSceneryIds.end() ? std::max(0, found->sceneryId) : 0;
	}

	void SetGatewaySceneryId(ProjectAirport &airport, const bool officialServer, const std::string_view serverKey, const int sceneryId)
	{
		if (officialServer)
		{
			airport.sceneryId = sceneryId > 0 ? sceneryId : -1;
			return;
		}
		std::vector<GatewayTestSceneryId> &ids = airport.testSceneryIds;
		const auto found = std::ranges::find(ids, serverKey, &GatewayTestSceneryId::server);
		if (sceneryId <= 0)
		{
			if (found != ids.end())
				ids.erase(found);
			return;
		}
		if (found != ids.end())
			found->sceneryId = sceneryId;
		else
		{
			ids.push_back({ std::string(serverKey), sceneryId });
			std::ranges::sort(ids, {}, &GatewayTestSceneryId::server);
		}
	}

	bool ImportGatewaySubmission(SceneryProject &project, const std::vector<uint8_t> &masterZip, const int sceneryId, const bool officialServer, const std::string_view serverKey,
		GatewayImportReport &report, std::string *error)
	{
		report = {};
		std::vector<XPlane::ZipEntry> master;
		std::string reason;
		if (!XPlane::ReadZipArchive(masterZip, master, &reason))
			return Fail(error, "The submission's archive can't be read: " + reason);

		// <ICAO>.dat and <ICAO>_Scenery_Pack.zip at the top of the archive
		const XPlane::ZipEntry *dat = nullptr;
		const XPlane::ZipEntry *packZip = nullptr;
		bool hasDsfText = false;
		for (const XPlane::ZipEntry &entry : master)
		{
			if (entry.name.find('/') != std::string::npos)
				continue;
			const std::string lower = ToLowerAscii(entry.name);
			if (!dat && lower.ends_with(".dat"))
				dat = &entry;
			else if (!packZip && lower.ends_with("_scenery_pack.zip"))
				packZip = &entry;
			else if (lower.ends_with(".txt"))
				hasDsfText = true;
		}
		if (!dat)
			return Fail(error, "The submission has no <ICAO>.dat");

		XPlane::AptFile apt;
		if (!XPlane::ParseAptDat(std::string_view(reinterpret_cast<const char *>(dat->data.data()), dat->data.size()), apt, &reason))
			return Fail(error, dat->name + ": " + reason);
		if (apt.airports.empty())
			return Fail(error, dat->name + " holds no airport");
		const std::string stem = dat->name.substr(0, dat->name.size() - 4);
		const auto named = std::ranges::find(apt.airports, stem, &XPlane::AptAirport::id);
		XPlane::AptAirport airport = named != apt.airports.end() ? *named : apt.airports.front();
		report.airportId = airport.id;
		report.warnings.insert(report.warnings.end(), apt.warnings.begin(), apt.warnings.end());

		// The pack's binary DSF tiles, filtered to this airport
		XPlane::DsfFile overlay;
		if (packZip)
		{
			std::vector<XPlane::ZipEntry> pack;
			if (!XPlane::ReadZipArchive(packZip->data, pack, &reason))
				report.warnings.push_back(packZip->name + " can't be read (" + reason + "): no DSF content was imported");
			size_t leftOut = 0;
			for (const XPlane::ZipEntry &entry : pack)
			{
				const std::string lower = ToLowerAscii(entry.name);
				if (lower.find("earth nav data/") == std::string::npos || !lower.ends_with(".dsf"))
					continue;
				XPlane::DsfFile tile;
				if (!XPlane::ReadDsf(entry.data.data(), entry.data.size(), tile, &reason))
				{
					report.warnings.push_back(entry.name + ": " + reason);
					continue;
				}
				XPlane::AppendDsf(overlay, XPlane::SelectAirportPlacements(tile, airport, &leftOut));
			}
			if (leftOut > 0)
				report.warnings.push_back(std::to_string(leftOut) + " DSF placements belong to another airport (or none) and weren't imported");
		}
		else if (hasDsfText)
			report.warnings.push_back("The submission has DSF text but no scenery pack: its DSF content wasn't imported");

		DsfImport imported = ImportDsfItems(overlay);
		report.items = imported.items.size();
		report.warnings.insert(report.warnings.end(), imported.warnings.begin(), imported.warnings.end());

		// Into the project: the airport (replacing one with the same ID) and its hierarchy
		ProjectAirport *target = project.FindAirport(airport.id);
		report.replaced = target != nullptr;
		if (!target)
		{
			project.airports.emplace_back();
			target = &project.airports.back();
		}
		target->airport = airport;
		SetGatewaySceneryId(*target, officialServer, serverKey, sceneryId);

		bool hidden = false;
		SceneryItem *item = FindAirportItem(project.items, airport.id, false, hidden);
		if (!item)
		{
			SceneryItem placed;
			placed.kind = ItemKind::Airport;
			placed.id = MakeItemId();
			placed.name = airport.name;
			placed.airportId = airport.id;
			project.items.push_back(std::move(placed));
			item = &project.items.back();
		}
		item->children = std::move(imported.items);

		if (!project.datum.valid)
		{
			double latitude = 0.0, longitude = 0.0;
			if (airport.GetDatum(latitude, longitude))
			{
				project.datum.latitude = latitude;
				project.datum.longitude = longitude;
				project.datum.elevation = airport.elevationFt * 0.3048;
				project.datum.valid = true;
			}
		}
		return true;
	}
}

// -------------------------------------------------------

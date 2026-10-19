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
 * scenery_pack.cpp
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#include "scenery_pack.h"
#include "project_dsf.h"
#include "project_io.h"
#include <SceneryEditorX/filesystem/vfs.h>
#include <SceneryEditorX/xplane/xp_airport_document.h>
#include <SceneryEditorX/xplane/xp_dsf.h>
#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <map>
#include <set>
#include <unordered_map>

// -------------------------------------------------------

namespace SceneryEditorX::ProjectFile
{
	namespace fs = std::filesystem;

	namespace
	{
		constexpr double FEET_TO_METERS = 0.3048;
		constexpr const char *SOURCE = "source";	// The project folder, read-only
		constexpr const char *PACK = "pack";		// The pack being written

		bool Fail(std::string *error, std::string message)
		{
			if (error)
				*error = std::move(message);
			return false;
		}

		std::string_view Trim(std::string_view text)
		{
			while (!text.empty() && (text.front() == ' ' || text.front() == '\t'))
				text.remove_prefix(1);
			while (!text.empty() && (text.back() == ' ' || text.back() == '\t' || text.back() == '\r' || text.back() == '\n'))
				text.remove_suffix(1);
			return text;
		}

		bool HasControlCharacter(const std::string_view text)
		{
			return std::ranges::any_of(text, [](const char c) { return static_cast<unsigned char>(c) < 32; });
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

		bool EndsWithNoCase(const std::string &text, const std::string_view suffix)
		{
			return text.size() >= suffix.size() && ToLowerAscii(text.substr(text.size() - suffix.size())) == suffix;
		}

		/* @brief The airport (its id) each item sits under in the hierarchy; items outside airports are left out. */
		void MapItemsToAirports(const std::vector<SceneryItem> &items, const std::string &airport, std::unordered_map<std::string, std::string> &out)
		{
			for (const SceneryItem &item : items)
			{
				const std::string &owner = item.kind == ItemKind::Airport ? item.airportId : airport;
				if (!owner.empty())
					out[item.id] = owner;
				if (!item.children.empty())
					MapItemsToAirports(item.children, owner, out);
			}
		}

		/* @brief Every item with an art resource, depth-first. */
		void CollectResourceItems(const std::vector<SceneryItem> &items, std::vector<const SceneryItem *> &out)
		{
			for (const SceneryItem &item : items)
			{
				if (HasResource(item.kind) && !item.resource.empty())
					out.push_back(&item);
				CollectResourceItems(item.children, out);
			}
		}

		/* @brief True for a "+40-090" style 10x10 degree folder or "+41-084.dsf" style tile name (without the extension). */
		bool IsTileName(const std::string_view name, const size_t latDigits, const size_t lonDigits)
		{
			if (name.size() != 2 + latDigits + lonDigits)
				return false;
			const auto digits = [](const std::string_view part) { return std::ranges::all_of(part, [](const char c) { return c >= '0' && c <= '9'; }); };
			return (name[0] == '+' || name[0] == '-') && digits(name.substr(1, latDigits)) &&
				   (name[1 + latDigits] == '+' || name[1 + latDigits] == '-') && digits(name.substr(2 + latDigits));
		}

		/**
		 * @brief Files a text art asset names beside itself: textures (TEXTURE..., with the .dds / .png twin X-Plane
		 * swaps in), and the objects an .agp / .str / .fac places. Paths are relative to the asset's folder.
		 */
		std::vector<std::string> FindAssetReferences(const std::string &text)
		{
			static constexpr std::string_view EXTENSIONS[] = { ".png", ".dds", ".obj", ".agp", ".fac", ".for", ".pol", ".lin", ".str" };
			std::vector<std::string> out;
			size_t start = 0;
			while (start < text.size())
			{
				size_t end = text.find('\n', start);
				if (end == std::string::npos)
					end = text.size();
				const std::string_view line = Trim(std::string_view(text).substr(start, end - start));
				start = end + 1;

				// Tokens: the keyword, then the first one naming a file
				std::vector<std::string_view> tokens;
				for (size_t i = 0; i < line.size();)
				{
					while (i < line.size() && (line[i] == ' ' || line[i] == '\t'))
						++i;
					const size_t from = i;
					while (i < line.size() && line[i] != ' ' && line[i] != '\t')
						++i;
					if (i > from)
						tokens.push_back(line.substr(from, i - from));
				}
				if (tokens.size() < 2)
					continue;
				const std::string keyword(tokens[0]);
				if (!keyword.starts_with("TEXTURE") && keyword != "OBJECT" && keyword != "OBJ" && keyword != "FACADE" && keyword != "DECAL" && keyword != "DECAL_LIB")
					continue;
				for (size_t i = 1; i < tokens.size(); ++i)
				{
					const std::string token(tokens[i]);
					if (std::ranges::any_of(EXTENSIONS, [&token](const std::string_view extension) { return EndsWithNoCase(token, extension); }))
					{
						out.push_back(token);
						break;
					}
				}
			}
			return out;
		}

		/* @brief Copies a pack-relative asset (and what it names) from the project folder into the pack, once each. */
		bool CopyAsset(const VFS::FileSystem &vfs, const std::string &relative, std::set<std::string> &copied, ExportReport &report, std::string *error, const int depth = 0)
		{
			const std::string normalized = VFS::NormalizeRelative(relative);
			if (normalized.empty() || !copied.insert(ToLowerAscii(normalized)).second || depth > 4)
				return true;
			const std::string from = VFS::MakeVirtualPath(SOURCE, normalized);
			if (!vfs.Exists(from))
				return true;	// Its twin texture or a library file: nothing to copy
			const std::string to = VFS::MakeVirtualPath(PACK, normalized);
			if (!vfs.Copy(from, to, true, error))
				return false;
			if (const std::optional<fs::path> written = vfs.Resolve(to))
				report.written.push_back(*written);
			++report.assets;

			// What the asset names, beside it
			const bool textual = !EndsWithNoCase(normalized, ".png") && !EndsWithNoCase(normalized, ".dds");
			std::string text;
			if (textual && vfs.Read(from, text))
			{
				const size_t slash = normalized.rfind('/');
				const std::string folder = slash == std::string::npos ? std::string() : normalized.substr(0, slash + 1);
				for (const std::string &reference : FindAssetReferences(text))
				{
					const std::string path = folder + reference;
					if (!CopyAsset(vfs, path, copied, report, error, depth + 1))
						return false;
					// X-Plane loads foo.dds in place of foo.png (and the other way round) when it's there
					const std::string base = path.substr(0, path.size() - 4);
					if (EndsWithNoCase(path, ".png") && !CopyAsset(vfs, base + ".dds", copied, report, error, depth + 1))
						return false;
					if (EndsWithNoCase(path, ".dds") && !CopyAsset(vfs, base + ".png", copied, report, error, depth + 1))
						return false;
				}
			}
			return true;
		}
	}

	const std::vector<std::string> &GetPackFolders()
	{
		static const std::vector<std::string> folders = { PACK_NAV_DATA, "objects", "textures" };
		return folders;
	}

	// -------------------------------------------------------
	// Coordinates

	bool ParseCoordinate(const std::string_view input, const bool latitude, double &value, std::string *error)
	{
		const char *what = latitude ? "latitude" : "longitude";
		std::string_view text = Trim(input);
		if (text.empty())
			return Fail(error, std::string("Enter the ") + what);

		// A hemisphere letter in place of the sign
		bool negative = false;
		bool hemisphere = false;
		if (const char last = static_cast<char>(text.back() & ~0x20); last == 'N' || last == 'S' || last == 'E' || last == 'W')
		{
			if (latitude != (last == 'N' || last == 'S'))
				return Fail(error, std::string("'") + text.back() + "' isn't a " + (latitude ? "latitude (N or S)" : "longitude (E or W)") + " hemisphere");
			negative = last == 'S' || last == 'W';
			hemisphere = true;
			text = Trim(text.substr(0, text.size() - 1));
		}

		if (!text.empty() && (text.front() == '+' || text.front() == '-'))
		{
			if (hemisphere)
				return Fail(error, "Use a sign or a hemisphere letter, not both");
			negative = text.front() == '-';
			text.remove_prefix(1);
		}

		const size_t dot = text.find('.');
		const std::string_view whole = text.substr(0, dot);
		const std::string_view decimals = dot == std::string_view::npos ? std::string_view() : text.substr(dot + 1);
		const auto digits = [](const std::string_view part) { return std::ranges::all_of(part, [](const char c) { return c >= '0' && c <= '9'; }); };
		if (whole.empty() || whole.size() > 3 || !digits(whole) || !digits(decimals) || (dot != std::string_view::npos && decimals.empty()))
			return Fail(error, std::string("Write the ") + what + " in decimal degrees, like " + (latitude ? "-50.50000000" : "122.30916667"));
		if (decimals.size() > COORDINATE_DECIMALS)
			return Fail(error, "Use at most " + std::to_string(COORDINATE_DECIMALS) + " decimal places");

		double magnitude = 0.0;
		const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), magnitude);
		if (ec != std::errc() || end != text.data() + text.size())
			return Fail(error, std::string("Write the ") + what + " in decimal degrees");
		const double limit = latitude ? 90.0 : 180.0;
		if (magnitude > limit)
			return Fail(error, std::string("A ") + what + " lies between -" + (latitude ? "90 and 90" : "180 and 180"));
		value = negative ? -magnitude : magnitude;
		return true;
	}

	std::string FormatCoordinate(const double value)
	{
		char text[32];
		std::snprintf(text, sizeof(text), "%.8f", value == 0.0 ? 0.0 : value);
		std::string out = text;
		if (out.starts_with("-") && std::ranges::all_of(out.substr(1), [](const char c) { return c == '0' || c == '.'; }))
			out.erase(0, 1);	// No "-0.00000000"
		return out;
	}

	// -------------------------------------------------------
	// New airport projects

	std::string MakeDefaultProjectName(const std::string_view icao, const std::string_view name)
	{
		const std::string id(Trim(icao));
		const std::string title(Trim(name));
		if (id.empty())
			return VFS::MakePortableName(title, "New Airport");
		return VFS::MakePortableName(title.empty() ? id : id + " - " + title, "New Airport");
	}

	fs::path GetNewProjectFolder(const NewAirportProject &spec)
	{
		const std::string name = Trim(spec.projectName).empty() ? MakeDefaultProjectName(spec.icao, spec.name) : std::string(Trim(spec.projectName));
		return spec.location / VFS::FromUtf8(name);
	}

	std::vector<FieldProblem> ValidateNewAirportProject(const NewAirportProject &spec)
	{
		std::vector<FieldProblem> problems;
		const auto problem = [&problems](const char *field, std::string message) { problems.push_back({ field, std::move(message) }); };

		const std::string_view icao = Trim(spec.icao);
		if (icao.empty())
			problem("icao", "Enter the airport's ICAO code");
		else if (icao.size() < 3 || icao.size() > 7)
			problem("icao", "An airport code has 3 to 7 letters and digits");
		else if (!std::ranges::all_of(icao, [](const char c) { return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'); }))
			problem("icao", "Use capital letters A-Z and digits only");

		if (Trim(spec.name).empty())
			problem("name", "Enter the airport's name");
		else if (HasControlCharacter(spec.name))
			problem("name", "The name can't hold tabs or line breaks");
		if (Trim(spec.country).empty())
			problem("country", "Enter the country");
		else if (HasControlCharacter(spec.country))
			problem("country", "The country can't hold tabs or line breaks");
		if (HasControlCharacter(spec.city))
			problem("city", "The city can't hold tabs or line breaks");
		if (HasControlCharacter(spec.state))
			problem("state", "The state, province or county can't hold tabs or line breaks");

		if (!std::isfinite(spec.latitude) || spec.latitude < -90.0 || spec.latitude > 90.0)
			problem("latitude", "A latitude lies between -90 and 90");
		if (!std::isfinite(spec.longitude) || spec.longitude < -180.0 || spec.longitude > 180.0)
			problem("longitude", "A longitude lies between -180 and 180");
		if (spec.latitude == 0.0 && spec.longitude == 0.0)
			problem("latitude", "0, 0 is in the open ocean: enter the airport's reference point");
		if (!std::isfinite(spec.elevationFeet) || spec.elevationFeet < MIN_ELEVATION_FEET || spec.elevationFeet > MAX_ELEVATION_FEET)
			problem("elevation", "Enter an elevation between " + std::to_string(static_cast<int>(MIN_ELEVATION_FEET)) + " and " + std::to_string(static_cast<int>(MAX_ELEVATION_FEET)) + " feet");

		const std::string name = Trim(spec.projectName).empty() ? MakeDefaultProjectName(spec.icao, spec.name) : std::string(Trim(spec.projectName));
		if (const std::string why = VFS::CheckPortableName(name); !why.empty())
			problem("projectName", why);

		std::error_code ec;
		if (spec.location.empty())
			problem("location", "Choose the folder the project goes in");
		else if (!fs::is_directory(spec.location, ec))
			problem("location", VFS::ToUtf8(spec.location) + " doesn't exist");
		else if (VFS::CheckPortableName(name).empty())
		{
			const fs::path folder = spec.location / VFS::FromUtf8(name);
			if (fs::exists(folder, ec) && (!fs::is_directory(folder, ec) || !fs::is_empty(folder, ec)))
				problem("projectName", VFS::ToUtf8(folder.filename()) + " already exists there: choose another name or location");
		}
		return problems;
	}

	SceneryProject MakeAirportProject(const NewAirportProject &spec)
	{
		const fs::path folder = GetNewProjectFolder(spec);
		SceneryProject project = MakeNewProject(VFS::ToUtf8(folder.filename()));
		project.info.packageName = project.info.name;
		project.info.author = spec.author;
		project.info.description = std::string(Trim(spec.name)) + " (" + std::string(Trim(spec.icao)) + ")";

		project.datum.latitude = spec.latitude;
		project.datum.longitude = spec.longitude;
		project.datum.elevation = spec.elevationFeet * FEET_TO_METERS;
		project.datum.valid = true;

		const std::string id(Trim(spec.icao));
		const std::string name(Trim(spec.name));
		const int elevationFt = static_cast<int>(std::lround(spec.elevationFeet));
		XPlane::AptAirport airport;
		if (spec.starterLayout)
			airport = XPlane::MakeNewAirport(id, name, spec.latitude, spec.longitude, spec.type, elevationFt);
		else
		{
			airport.type = spec.type;
			airport.id = id;
			airport.name = name;
			airport.elevationFt = elevationFt;
			airport.SetMetadata("gui_label", "2D");
			if (id.size() == 4 && std::ranges::all_of(id, [](const char c) { return c >= 'A' && c <= 'Z'; }))
				airport.SetMetadata("icao_code", id);
		}

		// The reference point at full precision, then the place names; written in key order like WorldEditor
		airport.SetMetadata("datum_lat", FormatCoordinate(spec.latitude));
		airport.SetMetadata("datum_lon", FormatCoordinate(spec.longitude));
		airport.SetMetadata("country", std::string(Trim(spec.country)));
		if (!Trim(spec.city).empty())
			airport.SetMetadata("city", std::string(Trim(spec.city)));
		if (!Trim(spec.state).empty())
			airport.SetMetadata("state", std::string(Trim(spec.state)));
		std::ranges::stable_sort(airport.metadata, [](const XPlane::AptMetadata &a, const XPlane::AptMetadata &b) { return a.key < b.key; });

		ProjectAirport entry;
		entry.airport = std::move(airport);
		project.airports.push_back(std::move(entry));
		NormalizeProject(project);		// Places the airport at the top of the hierarchy
		return project;
	}

	bool CreateAirportProject(const NewAirportProject &spec, fs::path &projectFile, std::string *error)
	{
		if (const std::vector<FieldProblem> problems = ValidateNewAirportProject(spec); !problems.empty())
			return Fail(error, problems.front().message);

		const fs::path folder = GetNewProjectFolder(spec);
		VFS::FileSystem vfs;
		if (!vfs.Mount(PACK, folder, false, error))
			return false;
		for (const std::string &name : GetPackFolders())
		{
			if (!vfs.CreateFolders(VFS::MakeVirtualPath(PACK, name), error))
				return false;
		}

		const SceneryProject project = MakeAirportProject(spec);
		projectFile = folder / VFS::FromUtf8(project.info.name + EDX_EXTENSION);
		std::vector<std::string> warnings;
		if (!SaveProjectFile(projectFile, project, ProjectFormat::Edx, &warnings, error))
			return false;
		WriteGitFiles(folder);
		return true;
	}

	// -------------------------------------------------------
	// Export

	bool ExportSceneryPack(const SceneryProject &project, const fs::path &projectFolder, const fs::path &packFolder, const ExportOptions &options,
		ExportReport &report, std::string *error)
	{
		report = ExportReport();
		if (packFolder.empty())
			return Fail(error, "Choose the scenery pack folder to export to");
		for (const fs::path &guarded : options.protectedFolders)
		{
			if (!guarded.empty() && VFS::IsInside(packFolder, guarded))
				return Fail(error, VFS::ToUtf8(packFolder) + " belongs to X-Plane itself: export into Custom Scenery or another folder");
		}

		VFS::FileSystem vfs;
		if (!vfs.Mount(PACK, packFolder, false, error))
			return false;
		const bool sameFolder = !projectFolder.empty() && VFS::IsInside(packFolder, projectFolder) && VFS::IsInside(projectFolder, packFolder);
		if (!projectFolder.empty())
			vfs.Mount(SOURCE, projectFolder, true);

		report.problems = ValidateProject(project);
		for (const std::string &name : GetPackFolders())
		{
			if (name == PACK_NAV_DATA && !vfs.CreateFolders(VFS::MakeVirtualPath(PACK, name), error))
				return false;
		}

		const auto write = [&](const std::string &virtualPath, const std::string_view bytes)
		{
			if (!vfs.Write(virtualPath, bytes, error))
				return false;
			if (const std::optional<fs::path> path = vfs.Resolve(virtualPath))
				report.written.push_back(*path);
			return true;
		};

		// Earth nav data/apt.dat
		if (options.aptDat)
		{
			const XPlane::AptFile apt = BuildAptFile(project);
			if (apt.airports.empty())
			{
				if (!project.airports.empty())
					report.warnings.push_back("Every airport is hidden: apt.dat wasn't written");
			}
			else
			{
				if (!write(VFS::MakeVirtualPath(PACK, std::string(PACK_NAV_DATA) + "/apt.dat"), XPlane::WriteAptDat(apt)))
					return false;
				report.airports = apt.airports.size();
			}
		}

		// Earth nav data/<10x10>/<1x1>.dsf: the overlay, airport content filtered by its airport
		std::set<std::string> tilesWritten;
		if (options.dsf)
		{
			DsfConversionOptions conversionOptions;
			conversionOptions.includeObjects = true;
			conversionOptions.includeHidden = options.includeHidden;
			DsfConversion conversion = BuildProjectDsf(project, conversionOptions);
			report.warnings.insert(report.warnings.end(), conversion.warnings.begin(), conversion.warnings.end());

			// sim/filter/aptid per airport that owns placements; each placement points at its airport's
			std::unordered_map<std::string, std::string> owners;
			MapItemsToAirports(project.items, std::string(), owners);
			std::map<std::string, int32_t> filters;
			std::vector<std::pair<std::string, std::string>> filterProperties;
			const auto filterOf = [&](const std::string &itemId) -> int32_t
			{
				const auto owner = owners.find(itemId);
				if (owner == owners.end())
					return -1;
				const auto [it, added] = filters.try_emplace(owner->second, static_cast<int32_t>(filters.size()));
				if (added)
					filterProperties.emplace_back("sim/filter/aptid", owner->second);
				return it->second;
			};
			for (size_t i = 0; i < conversion.dsf.objects.size() && i < conversion.objectItems.size(); ++i)
				conversion.dsf.objects[i].filter = filterOf(conversion.objectItems[i]);
			for (size_t i = 0; i < conversion.dsf.polygons.size() && i < conversion.polygonItems.size(); ++i)
				conversion.dsf.polygons[i].filter = filterOf(conversion.polygonItems[i]);
			for (size_t i = 0; i < conversion.dsf.roads.size() && i < conversion.roadItems.size(); ++i)
				conversion.dsf.roads[i].filter = filterOf(conversion.roadItems[i]);

			// WorldEditor's header: bounds (set per tile), planet, creation agent, overlay, airport filters, then exclusions
			std::vector<std::pair<std::string, std::string>> properties = {
				{ "sim/planet", "earth" }, { "sim/creation_agent", options.creationAgent }, { "sim/overlay", "1" } };
			properties.insert(properties.end(), filterProperties.begin(), filterProperties.end());
			for (const auto &property : conversion.dsf.properties)
			{
				if (property.first != "sim/overlay" && property.first != "sim/planet" && property.first != "sim/creation_agent" &&
					property.first != "sim/west" && property.first != "sim/east" && property.first != "sim/north" && property.first != "sim/south")
					properties.push_back(property);
			}
			conversion.dsf.properties = std::move(properties);

			for (XPlane::DsfFile &tile : XPlane::SplitDsfByTile(conversion.dsf, &report.warnings))
			{
				// SplitDsfByTile puts the bounds first (the overlay flag no longer leads): match WorldEditor's order
				std::stable_partition(tile.properties.begin(), tile.properties.end(), [](const auto &property)
				{
					return property.first == "sim/west" || property.first == "sim/east" || property.first == "sim/north" || property.first == "sim/south";
				});

				std::vector<uint8_t> bytes;
				std::string reason;
				const std::string name = XPlane::GetDsfTileName(tile.south + 0.5, tile.west + 0.5);
				if (!XPlane::WriteDsf(tile, bytes, &reason))
					return Fail(error, "DSF tile " + name + ": " + reason);
				const std::string path = VFS::MakeVirtualPath(PACK, std::string(PACK_NAV_DATA) + "/" + XPlane::GetDsfTileFolderName(tile.south + 0.5, tile.west + 0.5) + "/" + name + ".dsf");
				if (!write(path, std::string_view(reinterpret_cast<const char *>(bytes.data()), bytes.size())))
					return false;
				tilesWritten.insert(ToLowerAscii(path));
				++report.tiles;
				report.objects += tile.objects.size();
				report.polygons += tile.polygons.size();
			}
		}

		// Tiles an earlier export wrote that this one didn't
		for (const VFS::Entry &folder : vfs.List(VFS::MakeVirtualPath(PACK, PACK_NAV_DATA)))
		{
			if (!folder.directory || !IsTileName(folder.name, 2, 3))
				continue;
			for (const VFS::Entry &file : vfs.List(folder.path))
			{
				if (file.directory || !EndsWithNoCase(file.name, ".dsf") || !IsTileName(file.name.substr(0, file.name.size() - 4), 2, 3) || tilesWritten.contains(ToLowerAscii(file.path)))
					continue;
				if (options.removeStaleTiles && options.dsf)
				{
					const std::optional<fs::path> physical = vfs.Find(file.path);
					if (!vfs.Remove(file.path, error))
						return false;
					if (physical)
						report.removed.push_back(*physical);
				}
				else
					report.warnings.push_back(std::string(PACK_NAV_DATA) + "/" + folder.name + "/" + file.name + " is left from an earlier export: the project places nothing there now");
			}
		}

		// The pack's own art: copied from the project folder when the pack is somewhere else; the rest comes from libraries
		std::vector<const SceneryItem *> resourceItems;
		CollectResourceItems(project.items, resourceItems);
		std::set<std::string> resources, copied;
		for (const SceneryItem *item : resourceItems)
		{
			const std::string resource = VFS::NormalizeRelative(item->resource);
			if (!resources.insert(ToLowerAscii(resource)).second)
				continue;
			const bool local = !projectFolder.empty() && vfs.Exists(VFS::MakeVirtualPath(SOURCE, resource));
			if (!local)
			{
				++report.libraryResources;
				const bool fromLibrary = !item->library.empty() || resource.starts_with("lib/") || (options.isLibraryPath && options.isLibraryPath(item->resource));
				if (!fromLibrary && !projectFolder.empty())
					report.warnings.push_back(resource + " is neither in the project folder nor in a library X-Plane has");
				continue;
			}
			if (!sameFolder && options.copyAssets && !CopyAsset(vfs, resource, copied, report, error))
				return false;
		}

		// library.txt for library projects
		if (options.library && project.library)
		{
			if (!write(VFS::MakeVirtualPath(PACK, "library.txt"), WriteLibraryText(*project.library)))
				return false;
		}

		// earth.wed.xml so WorldEditor can open the pack
		if (options.wed)
		{
			const std::optional<fs::path> path = vfs.Resolve(VFS::MakeVirtualPath(PACK, WED_FILE_NAME));
			std::vector<std::string> warnings;
			if (!path || !SaveProjectFile(*path, project, ProjectFormat::Wed, &warnings, error))
				return false;
			report.written.push_back(*path);
			report.warnings.insert(report.warnings.end(), warnings.begin(), warnings.end());
		}
		return true;
	}
}

// -------------------------------------------------------

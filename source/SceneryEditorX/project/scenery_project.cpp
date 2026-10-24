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
 * scenery_project.cpp
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#include "scenery_project.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <ctime>
#include <map>
#include <random>
#include <set>
#include <unordered_set>

// -------------------------------------------------------

namespace SceneryEditorX::ProjectFile
{
	namespace
	{
		constexpr char HEX[] = "0123456789abcdef";

		std::mt19937_64 &GetRandom()
		{
			// Seeded once from the OS and the clock: ids made in the same process must never repeat.
			thread_local std::mt19937_64 random([]
			{
				std::random_device device;
				const uint64_t clock = static_cast<uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
				std::seed_seq seed{ device(), device(), device(), device(), static_cast<uint32_t>(clock), static_cast<uint32_t>(clock >> 32) };
				return std::mt19937_64(seed);
			}());
			return random;
		}

		void AppendHex(std::string &out, uint64_t value, const int digits)
		{
			for (int i = digits - 1; i >= 0; --i)
				out.push_back(HEX[(value >> (i * 4)) & 0xF]);
		}

		bool ValidLatitude(const double value) { return std::isfinite(value) && value >= -90.0 && value <= 90.0; }
		bool ValidLongitude(const double value) { return std::isfinite(value) && value >= -180.0 && value <= 180.0; }

		/* @brief Minimum nodes the outline of a kind needs (0 = no nodes). */
		size_t MinimumNodes(const ItemKind kind)
		{
			switch (kind)
			{
				case ItemKind::Polygon:
				case ItemKind::Orthophoto:
				case ItemKind::ExclusionPolygon:
				case ItemKind::Facade:
					return 3;
				case ItemKind::Line:
				case ItemKind::String:
				case ItemKind::Autogen:
				case ItemKind::ExclusionZone:
				case ItemKind::Road:
					return 2;
				case ItemKind::Forest:
					return 1;
				default:
					return 0;
			}
		}

		template<typename Item, typename Visitor>
		bool Walk(std::vector<Item> &items, const Visitor &visitor, const int depth)
		{
			for (Item &item : items)
			{
				if (!visitor(item, depth))
					return false;
				if (!Walk(item.children, visitor, depth + 1))
					return false;
			}
			return true;
		}

		template<typename Item>
		Item *Find(std::vector<Item> &items, const std::string_view id)
		{
			for (Item &item : items)
			{
				if (item.id == id)
					return &item;
				if (Item *found = Find(item.children, id))
					return found;
			}
			return nullptr;
		}

		std::vector<SceneryItem> *FindList(std::vector<SceneryItem> &items, const std::string_view id, size_t *index)
		{
			for (size_t i = 0; i < items.size(); ++i)
			{
				if (items[i].id == id)
				{
					if (index)
						*index = i;
					return &items;
				}
				if (std::vector<SceneryItem> *found = FindList(items[i].children, id, index))
					return found;
			}
			return nullptr;
		}

		void Extend(double &south, double &west, double &north, double &east, bool &any, const double latitude, const double longitude)
		{
			if (!ValidLatitude(latitude) || !ValidLongitude(longitude))
				return;
			if (!any)
			{
				south = north = latitude;
				west = east = longitude;
				any = true;
				return;
			}
			south = std::min(south, latitude);
			north = std::max(north, latitude);
			west = std::min(west, longitude);
			east = std::max(east, longitude);
		}
	}

	// -------------------------------------------------------

	const char *GetProjectKindName(const ProjectKind kind)
	{
		switch (kind)
		{
			case ProjectKind::Scenery:	return "scenery";
			case ProjectKind::Library:	return "library";
		}
		return "scenery";
	}

	const std::vector<std::pair<uint32_t, const char *>> &GetExclusionNames()
	{
		static const std::vector<std::pair<uint32_t, const char *>> names =
		{
			{ ExcludeObjects, "objects" }, { ExcludeFacades, "facades" }, { ExcludeForests, "forests" }, { ExcludeBeaches, "beaches" },
			{ ExcludeRoads, "roads" }, { ExcludeLines, "lines" }, { ExcludePolygons, "polygons" }, { ExcludeStrings, "strings" }
		};
		return names;
	}

	const char *GetItemKindName(const ItemKind kind)
	{
		switch (kind)
		{
			case ItemKind::Group:				return "group";
			case ItemKind::Airport:				return "airport";
			case ItemKind::Object:				return "object";
			case ItemKind::Polygon:				return "polygon";
			case ItemKind::Orthophoto:			return "orthophoto";
			case ItemKind::Line:				return "line";
			case ItemKind::String:				return "string";
			case ItemKind::Forest:				return "forest";
			case ItemKind::Facade:				return "facade";
			case ItemKind::Autogen:				return "autogen";
			case ItemKind::ExclusionZone:		return "exclusion-zone";
			case ItemKind::ExclusionPolygon:	return "exclusion-polygon";
			case ItemKind::Raw:					return "wed-raw";
			case ItemKind::RoadNode:			return "road-node";
			case ItemKind::Road:				return "road";
		}
		return "object";
	}

	bool HasResource(const ItemKind kind)
	{
		switch (kind)
		{
			case ItemKind::Object:
			case ItemKind::Polygon:
			case ItemKind::Orthophoto:
			case ItemKind::Line:
			case ItemKind::String:
			case ItemKind::Forest:
			case ItemKind::Facade:
			case ItemKind::Autogen:
			case ItemKind::Road:
				return true;
			default:
				return false;
		}
	}

	bool HasNodes(const ItemKind kind)
	{
		return MinimumNodes(kind) > 0;
	}

	// -------------------------------------------------------

	ProjectAirport *SceneryProject::FindAirport(const std::string_view id)
	{
		const auto it = std::ranges::find_if(airports, [id](const ProjectAirport &airport) { return airport.airport.id == id; });
		return it != airports.end() ? &*it : nullptr;
	}

	const ProjectAirport *SceneryProject::FindAirport(const std::string_view id) const
	{
		return const_cast<SceneryProject *>(this)->FindAirport(id);
	}

	SceneryItem *SceneryProject::FindItem(const std::string_view id)
	{
		return id.empty() ? nullptr : Find(items, id);
	}

	const SceneryItem *SceneryProject::FindItem(const std::string_view id) const
	{
		return const_cast<SceneryProject *>(this)->FindItem(id);
	}

	std::vector<SceneryItem> *SceneryProject::FindParentList(const std::string_view id, size_t *index)
	{
		return id.empty() ? nullptr : FindList(items, id, index);
	}

	void SceneryProject::ForEachItem(const std::function<bool(SceneryItem &item, int depth)> &visitor)
	{
		Walk(items, visitor, 0);
	}

	void SceneryProject::ForEachItem(const std::function<bool(const SceneryItem &item, int depth)> &visitor) const
	{
		Walk(const_cast<std::vector<SceneryItem> &>(items), [&visitor](SceneryItem &item, const int depth) { return visitor(item, depth); }, 0);
	}

	size_t SceneryProject::CountItems() const
	{
		size_t count = 0;
		ForEachItem([&count](const SceneryItem &, int) { ++count; return true; });
		return count;
	}

	size_t SceneryProject::CountItems(const ItemKind kind) const
	{
		size_t count = 0;
		ForEachItem([&count, kind](const SceneryItem &item, int) { count += item.kind == kind; return true; });
		return count;
	}

	const LibraryReference *SceneryProject::FindLibrary(const std::string_view shortId) const
	{
		const auto it = std::ranges::find_if(libraries, [shortId](const LibraryReference &library) { return library.shortId == shortId; });
		return it != libraries.end() ? &*it : nullptr;
	}

	// -------------------------------------------------------

	std::string MakeItemId()
	{
		std::string id;
		id.reserve(12);
		AppendHex(id, GetRandom()() & 0xFFFFFFFFFFFFull, 12);
		return id;
	}

	std::string MakeUuid()
	{
		uint64_t high = GetRandom()();
		uint64_t low = GetRandom()();
		high = (high & 0xFFFFFFFFFFFF0FFFull) | 0x0000000000004000ull;		// Version 4
		low = (low & 0x3FFFFFFFFFFFFFFFull) | 0x8000000000000000ull;		// Variant 10xx

		std::string uuid;
		uuid.reserve(36);
		AppendHex(uuid, high >> 32, 8);
		uuid.push_back('-');
		AppendHex(uuid, (high >> 16) & 0xFFFF, 4);
		uuid.push_back('-');
		AppendHex(uuid, high & 0xFFFF, 4);
		uuid.push_back('-');
		AppendHex(uuid, low >> 48, 4);
		uuid.push_back('-');
		AppendHex(uuid, low & 0xFFFFFFFFFFFFull, 12);
		return uuid;
	}

	std::string MakeLibraryShortId(const std::string_view libraryName)
	{
		uint32_t hash = 2166136261u;
		for (const char c : libraryName)
		{
			hash ^= static_cast<uint8_t>(c);
			hash *= 16777619u;
		}
		std::string id;
		AppendHex(id, hash, 8);
		return id;
	}

	std::string GetUtcTimestamp()
	{
		const std::time_t now = std::time(nullptr);
		std::tm utc{};
#if defined(_WIN32)
		gmtime_s(&utc, &now);
#else
		gmtime_r(&now, &utc);
#endif
		char text[32];
		std::strftime(text, sizeof(text), "%Y-%m-%dT%H:%M:%SZ", &utc);
		return text;
	}

	uint64_t HashFnv1a64(const std::string_view bytes, uint64_t seed)
	{
		for (const char c : bytes)
		{
			seed ^= static_cast<uint8_t>(c);
			seed *= 0x100000001b3ull;
		}
		return seed;
	}

	uint32_t Crc32(const void *data, const size_t size, uint32_t crc)
	{
		static const std::array<uint32_t, 256> table = []
		{
			std::array<uint32_t, 256> values{};
			for (uint32_t i = 0; i < 256; ++i)
			{
				uint32_t c = i;
				for (int k = 0; k < 8; ++k)
					c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
				values[i] = c;
			}
			return values;
		}();

		const auto *bytes = static_cast<const uint8_t *>(data);
		crc = ~crc;
		for (size_t i = 0; i < size; ++i)
			crc = table[(crc ^ bytes[i]) & 0xFF] ^ (crc >> 8);
		return ~crc;
	}

	// -------------------------------------------------------

	SceneryProject MakeNewProject(std::string name, const ProjectKind kind)
	{
		SceneryProject project;
		project.info.name = std::move(name);
		project.info.uuid = MakeUuid();
		project.info.kind = kind;
		project.info.created = GetUtcTimestamp();
		project.info.modified = project.info.created;
		project.info.packageName = project.info.name;
		if (kind == ProjectKind::Library)
		{
			project.library.emplace();
			project.library->version = 800;
		}
		return project;
	}

	std::vector<std::string> NormalizeProject(SceneryProject &project)
	{
		std::vector<std::string> changes;

		// Unique item ids (a merge can bring two items with the same id: the second one gets a new id)
		std::unordered_set<std::string> ids;
		project.ForEachItem([&](SceneryItem &item, int)
		{
			if (item.id.empty() || ids.contains(item.id))
			{
				const std::string old = item.id;
				do
					item.id = MakeItemId();
				while (ids.contains(item.id));
				changes.push_back(old.empty() ? "Item '" + item.name + "' had no id; it is now " + item.id
											  : "Item id " + old + " was used twice; the second '" + item.name + "' is now " + item.id);
			}
			ids.insert(item.id);
			return true;
		});

		// Airport items must name an airport of the project
		std::set<std::string> placed;
		const std::function<void(std::vector<SceneryItem> &)> prune = [&](std::vector<SceneryItem> &items)
		{
			for (size_t i = 0; i < items.size();)
			{
				SceneryItem &item = items[i];
				if (item.kind == ItemKind::Airport && (!project.FindAirport(item.airportId) || placed.contains(item.airportId)))
				{
					changes.push_back(project.FindAirport(item.airportId) ? "Airport " + item.airportId + " was placed twice; the second placement was removed"
																		  : "Airport item " + item.id + " names a missing airport '" + item.airportId + "'; its children moved up");
					std::vector<SceneryItem> children = std::move(item.children);
					items.erase(items.begin() + static_cast<std::ptrdiff_t>(i));
					items.insert(items.begin() + static_cast<std::ptrdiff_t>(i), std::make_move_iterator(children.begin()), std::make_move_iterator(children.end()));
					continue;
				}
				if (item.kind == ItemKind::Airport)
					placed.insert(item.airportId);
				if (!item.IsContainer() && !item.children.empty())
				{
					changes.push_back("Item " + item.id + " (" + GetItemKindName(item.kind) + ") can't hold children; they moved up");
					std::vector<SceneryItem> children = std::move(item.children);
					item.children.clear();
					// Inserting reallocates `items` (and `item`): the children are visited as siblings next.
					items.insert(items.begin() + static_cast<std::ptrdiff_t>(i) + 1, std::make_move_iterator(children.begin()), std::make_move_iterator(children.end()));
					++i;
					continue;
				}
				prune(item.children);
				++i;
			}
		};
		prune(project.items);

		// Airport ids must be unique too (the items reference them)
		std::set<std::string> airportIds;
		for (ProjectAirport &airport : project.airports)
		{
			const bool hadId = !airport.airport.id.empty();
			if (!hadId)
			{
				airport.airport.id = "XXXX";
				changes.push_back("An airport had no id; it is now XXXX");
			}
			if (airportIds.contains(airport.airport.id))
			{
				const std::string old = airport.airport.id;
				int suffix = 2;
				while (airportIds.contains(old + std::to_string(suffix)))
					++suffix;
				airport.airport.id = old + std::to_string(suffix);
				changes.push_back("Airport id " + old + " was used twice; the second is now " + airport.airport.id);
			}
			// An unnamed airport is named after its id when an .edX is read: name the one that just got its id the
			// same way, or the next load of this save would rename it (and a git-tracked .edX would change)
			if (!hadId && airport.airport.name.empty())
				airport.airport.name = airport.airport.id;
			airportIds.insert(airport.airport.id);
			if (!placed.contains(airport.airport.id))
			{
				SceneryItem item;
				item.kind = ItemKind::Airport;
				item.id = MakeItemId();
				while (ids.contains(item.id))
					item.id = MakeItemId();
				ids.insert(item.id);
				item.name = airport.airport.name;
				item.airportId = airport.airport.id;
				project.items.push_back(std::move(item));
				placed.insert(airport.airport.id);
				changes.push_back("Airport " + airport.airport.id + " was not in the hierarchy; it was added at the top level");
			}
		}

		for (LibraryReference &library : project.libraries)
		{
			if (library.shortId.empty())
			{
				library.shortId = MakeLibraryShortId(library.name.empty() ? library.localPath : library.name);
				changes.push_back("Library '" + library.name + "' got the short id " + library.shortId);
			}
		}

		if (project.info.uuid.empty())
		{
			project.info.uuid = MakeUuid();
			changes.push_back("The project had no uuid; it got " + project.info.uuid);
		}
		if (project.info.kind == ProjectKind::Library && !project.library)
		{
			project.library.emplace();
			project.library->version = 800;
		}
		return changes;
	}

	std::vector<std::string> ValidateProject(const SceneryProject &project)
	{
		std::vector<std::string> problems;
		std::unordered_set<std::string> ids;

		project.ForEachItem([&](const SceneryItem &item, int)
		{
			const std::string label = std::string(GetItemKindName(item.kind)) + " '" + (item.name.empty() ? item.id : item.name) + "'";
			if (item.id.empty())
				problems.push_back(label + " has no id");
			else if (!ids.insert(item.id).second)
				problems.push_back(label + " repeats the id " + item.id);

			if (HasResource(item.kind) && item.resource.empty())
				problems.push_back(label + " has no resource");
			if (!item.library.empty() && !project.FindLibrary(item.library))
				problems.push_back(label + " references the unknown library " + item.library);

			if (item.kind == ItemKind::Object)
			{
				if (!ValidLatitude(item.latitude) || !ValidLongitude(item.longitude))
					problems.push_back(label + " has an invalid position");
				if (item.showLevel < 1 || item.showLevel > 6)
					problems.push_back(label + " has show level " + std::to_string(item.showLevel) + " (1..6)");
			}

			if (const size_t minimum = MinimumNodes(item.kind); minimum > 0)
			{
				if (item.rings.empty() || item.rings.front().size() < minimum)
					problems.push_back(label + " needs at least " + std::to_string(minimum) + " nodes");
				if (item.kind == ItemKind::ExclusionZone && !item.rings.empty() && item.rings.front().size() != 2)
					problems.push_back(label + " needs exactly 2 corner nodes");
				for (const GeoRing &ring : item.rings)
				{
					for (const GeoNode &node : ring)
					{
						if (!ValidLatitude(node.latitude) || !ValidLongitude(node.longitude))
						{
							problems.push_back(label + " has a node outside -90..90 / -180..180");
							break;
						}
					}
				}
			}

			if ((item.kind == ItemKind::ExclusionZone || item.kind == ItemKind::ExclusionPolygon) && item.exclusions == 0)
				problems.push_back(label + " excludes nothing");
			if (item.kind == ItemKind::Airport && !project.FindAirport(item.airportId))
				problems.push_back(label + " names the missing airport '" + item.airportId + "'");
			if (item.kind == ItemKind::RoadNode && (!ValidLatitude(item.latitude) || !ValidLongitude(item.longitude)))
				problems.push_back(label + " has an invalid position");
			if (item.kind == ItemKind::Road && (item.roadSubtype < 0 || item.roadSubtype > 255))
				problems.push_back(label + " has road type " + std::to_string(item.roadSubtype) + " (DSF road types are 0..255)");
			return true;
		});

		// Roads run between road nodes, and their ends sit on them
		std::map<std::string, const SceneryItem *, std::less<>> roadNodes;
		project.ForEachItem([&roadNodes](const SceneryItem &item, int)
		{
			if (item.kind == ItemKind::RoadNode)
				roadNodes[item.id] = &item;
			return true;
		});
		project.ForEachItem([&](const SceneryItem &item, int)
		{
			if (item.kind != ItemKind::Road)
				return true;
			const std::string label = "road '" + (item.name.empty() ? item.id : item.name) + "'";
			for (const bool start : { true, false })
			{
				const std::string &id = start ? item.startNode : item.endNode;
				const auto it = roadNodes.find(id);
				if (it == roadNodes.end())
				{
					problems.push_back(label + (start ? " starts" : " ends") + " at the missing road node '" + id + "'");
					continue;
				}
				if (item.rings.empty() || item.rings[0].empty())
					continue;
				const GeoNode &end = start ? item.rings[0].front() : item.rings[0].back();
				if (std::abs(end.latitude - it->second->latitude) > 1e-7 || std::abs(end.longitude - it->second->longitude) > 1e-7)
					problems.push_back(label + (start ? "'s start" : "'s end") + " isn't at its road node");
			}
			if (!item.startNode.empty() && item.startNode == item.endNode && !item.rings.empty() && item.rings[0].size() < 3)
				problems.push_back(label + " starts and ends at one node with nothing between");
			return true;
		});

		std::set<std::string> airportIds;
		for (const ProjectAirport &airport : project.airports)
		{
			if (!airportIds.insert(airport.airport.id).second)
				problems.push_back("Airport id " + airport.airport.id + " is used twice");
			for (const std::string &problem : XPlane::ValidateAptAirport(airport.airport))
				problems.push_back(airport.airport.id + ": " + problem);
		}

		if (project.library)
		{
			std::set<std::string> regions;
			for (const XPlane::LibraryRegion &region : project.library->regions)
				regions.insert(region.name);
			for (const XPlane::LibraryExport &entry : project.library->exports)
			{
				if (entry.virtualPath.empty() || entry.realPath.empty())
					problems.push_back("Library export without a virtual or real path");
				else if (entry.virtualPath.find(' ') != std::string::npos)
					problems.push_back("Library virtual path '" + entry.virtualPath + "' contains a space");
				if (!entry.region.empty() && !regions.contains(entry.region))
					problems.push_back("Library export " + entry.virtualPath + " uses the undefined region " + entry.region);
			}
		}
		return problems;
	}

	bool GetProjectCenter(const SceneryProject &project, double &latitude, double &longitude)
	{
		if (project.datum.valid)
		{
			latitude = project.datum.latitude;
			longitude = project.datum.longitude;
			return true;
		}
		for (const ProjectAirport &airport : project.airports)
		{
			if (airport.airport.GetDatum(latitude, longitude))
				return true;
		}
		double south = 0.0, west = 0.0, north = 0.0, east = 0.0;
		if (!GetProjectBounds(project, south, west, north, east))
			return false;
		latitude = (south + north) * 0.5;
		longitude = (west + east) * 0.5;
		return true;
	}

	bool GetProjectBounds(const SceneryProject &project, double &south, double &west, double &north, double &east)
	{
		bool any = false;
		for (const ProjectAirport &airport : project.airports)
		{
			double s = 0.0, w = 0.0, n = 0.0, e = 0.0;
			if (airport.airport.GetBounds(s, w, n, e))
			{
				Extend(south, west, north, east, any, s, w);
				Extend(south, west, north, east, any, n, e);
			}
		}
		project.ForEachItem([&](const SceneryItem &item, int)
		{
			if (item.kind == ItemKind::Object)
				Extend(south, west, north, east, any, item.latitude, item.longitude);
			for (const GeoRing &ring : item.rings)
			{
				for (const GeoNode &node : ring)
					Extend(south, west, north, east, any, node.latitude, node.longitude);
			}
			return true;
		});
		return any;
	}

	std::vector<std::string> CollectResources(const SceneryProject &project)
	{
		std::set<std::string> resources;
		project.ForEachItem([&resources](const SceneryItem &item, int)
		{
			if (HasResource(item.kind) && !item.resource.empty())
				resources.insert(item.resource);
			return true;
		});
		return { resources.begin(), resources.end() };
	}

	// -------------------------------------------------------
	// Road networks

	std::vector<std::pair<SceneryItem *, bool>> FindRoadsAt(SceneryProject &project, const std::string_view nodeId)
	{
		std::vector<std::pair<SceneryItem *, bool>> roads;
		if (nodeId.empty())
			return roads;
		project.ForEachItem([&](SceneryItem &item, int)
		{
			if (item.kind != ItemKind::Road)
				return true;
			if (item.startNode == nodeId)
				roads.emplace_back(&item, true);
			if (item.endNode == nodeId)
				roads.emplace_back(&item, false);
			return true;
		});
		return roads;
	}

	std::vector<std::string> SnapRoadEnds(SceneryProject &project)
	{
		std::map<std::string, std::pair<double, double>, std::less<>> nodes;
		project.ForEachItem([&nodes](const SceneryItem &item, int)
		{
			if (item.kind == ItemKind::RoadNode)
				nodes[item.id] = { item.latitude, item.longitude };
			return true;
		});

		std::vector<std::string> moved;
		project.ForEachItem([&](SceneryItem &item, int)
		{
			if (item.kind != ItemKind::Road || item.rings.empty() || item.rings[0].size() < 2)
				return true;
			bool changed = false;
			for (const bool start : { true, false })
			{
				const auto it = nodes.find(start ? item.startNode : item.endNode);
				if (it == nodes.end())
					continue;
				GeoNode &end = start ? item.rings[0].front() : item.rings[0].back();
				if (end.latitude == it->second.first && end.longitude == it->second.second)
					continue;
				end.latitude = it->second.first;	// Handles are offsets: they travel with the node
				end.longitude = it->second.second;
				changed = true;
			}
			if (changed)
				moved.push_back(item.id);
			return true;
		});
		return moved;
	}
}

// -------------------------------------------------------

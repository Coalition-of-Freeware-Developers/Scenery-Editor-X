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
 * DsfImportTests.cpp
 * -------------------------------------------------------
 * Created: 24/10/2026
 * -------------------------------------------------------
 */
#include <catch2/catch_all.hpp>
#include <SceneryEditorX/project/project_dsf.h>
#include <SceneryEditorX/project/scenery_project.h>
#include <SceneryEditorX/xplane/xp_dsf.h>
#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <string_view>
#include <vector>

using namespace SceneryEditorX;
using namespace SceneryEditorX::ProjectFile;
using Catch::Matchers::ContainsSubstring;

// -------------------------------------------------------

namespace
{
	constexpr double LAT = 45.581;		// Everything sits in tile +45-123, well inside its edges
	constexpr double LON = -122.621;

	GeoNode Node(const double latitude, const double longitude)
	{
		GeoNode node;
		node.latitude = latitude;
		node.longitude = longitude;
		return node;
	}

	/* @brief A node with mirror-image handles (lo = -hi, written as 0.0 - x so a zero stays +0.0). */
	GeoNode Curved(const double latitude, const double longitude, const double dLat, const double dLon)
	{
		GeoNode node = Node(latitude, longitude);
		node.loLatitude = 0.0 - dLat;
		node.loLongitude = 0.0 - dLon;
		node.hiLatitude = dLat;
		node.hiLongitude = dLon;
		return node;
	}

	GeoNode WithUv(GeoNode node, const float s, const float t)
	{
		node.hasUv = true;
		node.s = s;
		node.t = t;
		return node;
	}

	/* @brief Counter-clockwise (east, then north): an outline the way BuildProjectDsf writes it. */
	GeoRing Square(const double latitude, const double longitude, const double size)
	{
		return { Node(latitude, longitude), Node(latitude, longitude + size), Node(latitude + size, longitude + size), Node(latitude + size, longitude) };
	}

	/* @brief Clockwise: a hole the way BuildProjectDsf writes it (as an outline, it gets turned around). */
	GeoRing ClockwiseSquare(const double latitude, const double longitude, const double size)
	{
		return { Node(latitude, longitude), Node(latitude + size, longitude), Node(latitude + size, longitude + size), Node(latitude, longitude + size) };
	}

	SceneryItem MakeItem(const ItemKind kind, const std::string &name, const std::string &resource = {})
	{
		SceneryItem item;
		item.kind = kind;
		item.id = MakeItemId();
		item.name = name;
		item.resource = resource;
		return item;
	}

	SceneryProject ProjectWith(std::vector<SceneryItem> items)
	{
		SceneryProject project = MakeNewProject("DSF import");
		project.items = std::move(items);
		return project;
	}

	DsfConversionOptions WithObjects()
	{
		DsfConversionOptions options;
		options.includeObjects = true;
		return options;
	}

	/**
	 * Every kind BuildProjectDsf writes, with every layout it has: objects (draped, MSL, AGL, autogen point), polygons
	 * (one turned around, one curved with split nodes), orthophotos (UV, UV + bezier), open curved and closed lines, open
	 * and closed curved strings, the three forest fills, facades with and without walls and curves, .ags with spawn
	 * switches and a hole, .agb, a road network and both exclusion forms.
	 */
	SceneryProject MakeEveryKindProject()
	{
		std::vector<SceneryItem> items;

		SceneryItem hangar = MakeItem(ItemKind::Object, "Hangar", "lib/airport/hangars/hangar.obj");
		hangar.latitude = LAT + 0.0101;
		hangar.longitude = LON + 0.0003;
		hangar.heading = 45.25f;
		SceneryItem tower = MakeItem(ItemKind::Object, "Tower", "objects/tower.obj");
		tower.latitude = LAT + 0.0102;
		tower.longitude = LON + 0.0004;
		tower.heading = 270.0f;
		tower.elevationMode = ElevationMode::Msl;
		tower.altitude = 12.5;
		SceneryItem mast = MakeItem(ItemKind::Object, "Mast", "objects/tower.obj");
		mast.latitude = LAT + 0.0103;
		mast.longitude = LON + 0.0005;
		mast.heading = 90.5f;
		mast.elevationMode = ElevationMode::Agl;
		mast.altitude = 3.0;
		SceneryItem farm = MakeItem(ItemKind::Object, "Farm", "autogen/farm.agp");
		farm.latitude = LAT + 0.0104;
		farm.longitude = LON + 0.0006;

		// Draped polygons: a clockwise outline with a hole, and a curved one (split corner, mirrored, plain, one handle)
		SceneryItem apron = MakeItem(ItemKind::Polygon, "Apron", "lib/airport/ground/asphalt.pol");
		apron.textureHeading = 45.5f;
		apron.rings = { ClockwiseSquare(LAT, LON, 0.002), ClockwiseSquare(LAT + 0.0005, LON + 0.0005, 0.0005) };
		SceneryItem pad = MakeItem(ItemKind::Polygon, "Pad", "polygons/concrete.pol");
		pad.textureHeading = 30.296875f;		// 30 + 38/128: what the parameter holds
		GeoRing padRing = Square(LAT + 0.003, LON, 0.002);
		padRing[0].loLatitude = 0.0003;
		padRing[0].hiLongitude = 0.0004;
		padRing[0].split = true;
		padRing[1] = Curved(padRing[1].latitude, padRing[1].longitude, 0.0002, 0.0001);
		padRing[3].hiLatitude = -0.0002;
		padRing[3].split = true;
		pad.rings = { padRing };

		SceneryItem ortho = MakeItem(ItemKind::Orthophoto, "Ortho", "orthos/tile.pol");
		GeoRing uv = Square(LAT + 0.006, LON, 0.002);
		uv = { WithUv(uv[0], 0.0f, 0.0f), WithUv(uv[1], 1.0f, 0.0f), WithUv(uv[2], 1.0f, 1.0f), WithUv(uv[3], 0.0f, 1.0f) };
		ortho.rings = { uv };
		SceneryItem curvedOrtho = MakeItem(ItemKind::Orthophoto, "Curved ortho", "orthos/tile.pol");
		GeoRing curvedUv = Square(LAT + 0.009, LON, 0.002);
		curvedUv = { WithUv(curvedUv[0], 0.0f, 0.0f), WithUv(curvedUv[1], 1.0f, 0.0f), WithUv(curvedUv[2], 1.0f, 1.0f), WithUv(curvedUv[3], 0.0f, 1.0f) };
		curvedUv[1].loLongitude = -0.0003;
		curvedUv[1].hiLatitude = 0.0002;
		curvedUv[1].sLo = -0.1f;
		curvedUv[1].sHi = 0.05f;
		curvedUv[1].tHi = 0.2f;
		curvedUv[1].split = true;
		curvedOrtho.rings = { curvedUv };

		// Lines: open with every kind of node (first and last count one handle), closed and straight
		SceneryItem taxiLine = MakeItem(ItemKind::Line, "Taxi line", "lines/taxi.lin");
		GeoNode split = Node(LAT + 0.013, LON + 0.002);
		split.loLatitude = -0.0003;
		split.hiLongitude = 0.0004;
		split.split = true;
		taxiLine.rings = { { Curved(LAT + 0.012, LON, 0.0001, 0.0), Curved(LAT + 0.012, LON + 0.001, 0.0, 0.0002), split, Curved(LAT + 0.014, LON + 0.002, 0.0, 0.0005) } };
		SceneryItem fence = MakeItem(ItemKind::Line, "Fence", "lines/fence.lin");
		fence.closed = true;
		fence.rings = { { Node(LAT + 0.012, LON + 0.004), Node(LAT + 0.012, LON + 0.005), Node(LAT + 0.013, LON + 0.005) } };

		// Strings: open and straight with whole meters, closed and curved (the closing node split) off the meter
		SceneryItem lights = MakeItem(ItemKind::String, "Edge lights", "strings/edge.str");
		lights.spacing = 30.0f;
		lights.rings = { { Node(LAT + 0.015, LON + 0.004), Node(LAT + 0.015, LON + 0.006) } };
		SceneryItem lamps = MakeItem(ItemKind::String, "Lamps", "strings/lamps.str");
		lamps.closed = true;
		lamps.spacing = 2.5f;
		GeoNode first = Node(LAT + 0.016, LON);
		first.loLatitude = 0.0001;
		first.hiLongitude = 0.0002;
		first.split = true;
		lamps.rings = { { first, Curved(LAT + 0.016, LON + 0.001, 0.0001, 0.0001), Node(LAT + 0.017, LON + 0.001) } };

		SceneryItem woods = MakeItem(ItemKind::Forest, "Woods", "forests/mixed.for");
		woods.density = 0.75f;
		woods.rings = { Square(LAT, LON + 0.004, 0.002), ClockwiseSquare(LAT + 0.0005, LON + 0.0045, 0.0005) };
		SceneryItem hedge = MakeItem(ItemKind::Forest, "Hedge", "forests/mixed.for");
		hedge.fill = ForestFill::Line;
		hedge.density = 0.5f;
		hedge.rings = { { Node(LAT + 0.0025, LON + 0.004), Node(LAT + 0.0025, LON + 0.005), Node(LAT + 0.0028, LON + 0.0055) },
			{ Node(LAT + 0.0029, LON + 0.004), Node(LAT + 0.0029, LON + 0.005) } };
		SceneryItem trees = MakeItem(ItemKind::Forest, "Trees", "forests/conifer.for");
		trees.fill = ForestFill::Points;
		trees.rings = { { Node(LAT + 0.0031, LON + 0.004), Node(LAT + 0.0032, LON + 0.0041), Node(LAT + 0.0033, LON + 0.0042) } };

		// Facades keep their node order (the clockwise one stays clockwise)
		SceneryItem office = MakeItem(ItemKind::Facade, "Office", "facades/office.fac");
		office.pickWalls = true;
		office.height = 14.0f;
		office.rings = { ClockwiseSquare(LAT + 0.004, LON + 0.004, 0.0005) };
		office.rings[0][1].wallType = 3;
		office.rings[0][2].wallType = 1;
		office.rings[0][3].wallType = 2;
		SceneryItem shed = MakeItem(ItemKind::Facade, "Shed", "facades/shed.fac");
		shed.height = 6.0f;
		shed.rings = { Square(LAT + 0.004, LON + 0.005, 0.0003) };
		SceneryItem terminal = MakeItem(ItemKind::Facade, "Terminal", "facades/terminal.fac");
		terminal.pickWalls = true;
		terminal.height = 9.0f;
		terminal.rings = { { Node(LAT + 0.005, LON + 0.004), Curved(LAT + 0.005, LON + 0.0045, 0.0001, 0.0), Node(LAT + 0.0055, LON + 0.0045), Node(LAT + 0.0055, LON + 0.004) } };
		terminal.rings[0][1].wallType = 2;
		terminal.rings[0][2].wallType = 1;
		SceneryItem hall = MakeItem(ItemKind::Facade, "Hall", "facades/hall.fac");
		hall.height = 12.0f;
		hall.rings = { Square(LAT + 0.005, LON + 0.005, 0.0004) };
		hall.rings[0][2].loLatitude = -0.0001;
		hall.rings[0][2].hiLongitude = -0.0002;
		hall.rings[0][2].split = true;

		// Autogen: switches after the first node (with a hole), at the first node, twice; blocks with and without one
		SceneryItem houses = MakeItem(ItemKind::Autogen, "Houses", "autogen/houses.ags");
		houses.height = 12.0f;
		houses.rings = { Square(LAT + 0.006, LON + 0.004, 0.002), ClockwiseSquare(LAT + 0.0065, LON + 0.0045, 0.0005) };
		houses.rings[0][1].spawnTiles = false;
		SceneryItem barns = MakeItem(ItemKind::Autogen, "Barns", "autogen/barns.ags");
		barns.height = 8.0f;
		barns.rings = { Square(LAT + 0.009, LON + 0.004, 0.001) };
		barns.rings[0][0].spawnTiles = false;
		SceneryItem mixed = MakeItem(ItemKind::Autogen, "Mixed", "autogen/houses.ags");
		mixed.height = 16.0f;
		mixed.rings = { Square(LAT + 0.009, LON + 0.006, 0.001) };
		mixed.rings[0][1].spawnTiles = false;
		mixed.rings[0][3].spawnTiles = false;
		SceneryItem block = MakeItem(ItemKind::Autogen, "Block", "autogen/block.agb");
		block.height = 20.0f;
		block.spelling = 4;
		block.rings = { Square(LAT + 0.011, LON + 0.004, 0.001) };
		SceneryItem quietBlock = MakeItem(ItemKind::Autogen, "Quiet block", "autogen/block.agb");
		quietBlock.height = 24.0f;
		quietBlock.spelling = 2;
		quietBlock.rings = { Square(LAT + 0.011, LON + 0.006, 0.001) };
		quietBlock.rings[0][2].spawnTiles = false;

		// Roads: two meeting at the middle node; the bridge curves through a split bend and has no handle into its end
		SceneryItem west = MakeItem(ItemKind::RoadNode, "West");
		west.latitude = LAT + 0.020;
		west.longitude = LON + 0.001;
		SceneryItem middle = MakeItem(ItemKind::RoadNode, "Middle");
		middle.latitude = LAT + 0.021;
		middle.longitude = LON + 0.003;
		SceneryItem east = MakeItem(ItemKind::RoadNode, "East");
		east.latitude = LAT + 0.0212;
		east.longitude = LON + 0.006;
		SceneryItem access = MakeItem(ItemKind::Road, "Access road", "lib/g10/roads.net");
		access.roadSubtype = 100;
		access.startNode = west.id;
		access.endNode = middle.id;
		access.rings = { { Node(west.latitude, west.longitude), Node(LAT + 0.0205, LON + 0.002), Node(middle.latitude, middle.longitude) } };
		SceneryItem bridge = MakeItem(ItemKind::Road, "Bridge", "lib/g10/roads.net");
		bridge.roadSubtype = 101;
		bridge.startNode = middle.id;
		bridge.endNode = east.id;
		bridge.startLayer = 1;
		bridge.endLayer = 1;
		GeoNode start = Node(middle.latitude, middle.longitude);
		start.hiLatitude = 0.0004;
		start.hiLongitude = 0.0003;
		GeoNode bend = Node(LAT + 0.0215, LON + 0.0045);
		bend.loLatitude = -0.0001;
		bend.loLongitude = -0.00015;
		bend.hiLatitude = 0.0002;
		bend.hiLongitude = 0.0001;
		bend.split = true;
		bridge.rings = { { start, bend, Node(east.latitude, east.longitude) } };

		SceneryItem zone = MakeItem(ItemKind::ExclusionZone, "Zone");
		zone.exclusions = ExcludeObjects | ExcludeForests | ExcludeStrings;
		zone.rings = { { Node(LAT + 0.019, LON - 0.019), Node(LAT + 0.021, LON - 0.017) } };
		SceneryItem noPolygons = MakeItem(ItemKind::ExclusionPolygon, "No polygons");
		noPolygons.exclusions = ExcludePolygons | ExcludeLines;
		noPolygons.rings = { { Node(45.603, -122.64), Node(45.603, -122.638), Node(45.605, -122.638), Node(45.605, -122.64) } };

		// Kinds mixed up: the DSF sorts them into objects, polygons and roads anyway
		items = { apron, hangar, zone, pad, tower, ortho, curvedOrtho, taxiLine, west, access, middle, fence, lights, lamps, mast,
			woods, hedge, trees, noPolygons, office, shed, terminal, hall, bridge, east, houses, barns, mixed, block, quietBlock, farm };
		return ProjectWith(std::move(items));
	}

	std::map<ItemKind, size_t> CountKinds(const std::vector<SceneryItem> &items)
	{
		std::map<ItemKind, size_t> counts;
		for (const SceneryItem &item : items)
			++counts[item.kind];
		return counts;
	}

	std::vector<const SceneryItem *> OfKind(const std::vector<SceneryItem> &items, const ItemKind kind)
	{
		std::vector<const SceneryItem *> found;
		for (const SceneryItem &item : items)
		{
			if (item.kind == kind)
				found.push_back(&item);
		}
		return found;
	}

	const SceneryItem &Original(const SceneryProject &project, const std::string_view name)
	{
		const auto it = std::ranges::find_if(project.items, [&name](const SceneryItem &item) { return item.name == name; });
		REQUIRE(it != project.items.end());
		return *it;
	}

	/* @brief Shoelace area in degrees: positive counter-clockwise. */
	double RingArea(const GeoRing &ring)
	{
		double sum = 0.0;
		for (size_t i = 0, j = ring.size() - 1; i < ring.size(); j = i++)
			sum += (ring[j].longitude - ring[0].longitude) * (ring[i].latitude - ring[0].latitude) - (ring[i].longitude - ring[0].longitude) * (ring[j].latitude - ring[0].latitude);
		return 0.5 * sum;
	}

	const SceneryItem *FindId(const std::vector<SceneryItem> &items, const std::string &id)
	{
		const auto it = std::ranges::find_if(items, [&id](const SceneryItem &item) { return item.id == id; });
		return it == items.end() ? nullptr : &*it;
	}

	/* @brief Imported nodes match the originals: positions, handles, split, walls, spawn flags and texture coordinates. */
	void CheckSameRing(const GeoRing &imported, const GeoRing &original)
	{
		REQUIRE(imported.size() == original.size());
		for (size_t i = 0; i < original.size(); ++i)
		{
			INFO("node " << i);
			const GeoNode &a = imported[i], &b = original[i];
			CHECK(a.latitude == Catch::Approx(b.latitude).margin(1e-12));
			CHECK(a.longitude == Catch::Approx(b.longitude).margin(1e-12));
			CHECK(a.loLatitude == Catch::Approx(b.loLatitude).margin(1e-12));
			CHECK(a.loLongitude == Catch::Approx(b.loLongitude).margin(1e-12));
			CHECK(a.hiLatitude == Catch::Approx(b.hiLatitude).margin(1e-12));
			CHECK(a.hiLongitude == Catch::Approx(b.hiLongitude).margin(1e-12));
			CHECK(a.split == b.split);
			CHECK(a.wallType == b.wallType);
			CHECK(a.spawnTiles == b.spawnTiles);
			CHECK(a.hasUv == b.hasUv);
			CHECK(a.s == Catch::Approx(b.s).margin(1e-6));
			CHECK(a.t == Catch::Approx(b.t).margin(1e-6));
			CHECK(a.sLo == Catch::Approx(b.sLo).margin(1e-6));
			CHECK(a.tLo == Catch::Approx(b.tLo).margin(1e-6));
			CHECK(a.sHi == Catch::Approx(b.sHi).margin(1e-6));
			CHECK(a.tHi == Catch::Approx(b.tHi).margin(1e-6));
		}
	}

	/**
	 * @brief Two DSFs place the same: definitions, parameters, planes, junctions and properties equal, coordinates within
	 * @p tolerance degrees, other values (texture coordinates, headings, elevations) within @p valueTolerance.
	 */
	void RequireSameDsf(const XPlane::DsfFile &a, const XPlane::DsfFile &b, const double tolerance, const double valueTolerance)
	{
		CHECK(b.objectDefinitions == a.objectDefinitions);
		CHECK(b.polygonDefinitions == a.polygonDefinitions);
		CHECK(b.networkDefinitions == a.networkDefinitions);
		CHECK(b.properties == a.properties);

		REQUIRE(b.objects.size() == a.objects.size());
		for (size_t i = 0; i < a.objects.size(); ++i)
		{
			INFO("object " << i);
			const XPlane::DsfObjectPlacement &x = a.objects[i], &y = b.objects[i];
			CHECK(y.definition == x.definition);
			CHECK(y.mode == x.mode);
			CHECK(std::abs(y.longitude - x.longitude) <= tolerance);
			CHECK(std::abs(y.latitude - x.latitude) <= tolerance);
			CHECK(std::abs(y.heading - x.heading) <= valueTolerance);
			CHECK(std::abs(y.elevation - x.elevation) <= valueTolerance);
		}

		REQUIRE(b.polygons.size() == a.polygons.size());
		for (size_t i = 0; i < a.polygons.size(); ++i)
		{
			const XPlane::DsfPolygonPlacement &x = a.polygons[i], &y = b.polygons[i];
			INFO("polygon " << i << " " << a.polygonDefinitions[x.definition]);
			CHECK(y.definition == x.definition);
			CHECK(y.parameter == x.parameter);
			REQUIRE(y.planes == x.planes);
			REQUIRE(y.windings.size() == x.windings.size());
			const uint8_t uvFrom = x.parameter == 65535 ? (x.planes == 8 ? 4 : 2) : 8;	// Texture coordinate planes
			for (size_t w = 0; w < x.windings.size(); ++w)
			{
				INFO("winding " << w);
				REQUIRE(y.windings[w].size() == x.windings[w].size());
				for (size_t k = 0; k < x.windings[w].size(); ++k)
				{
					INFO("point " << k);
					const XPlane::DsfPoint &p = x.windings[w][k], &q = y.windings[w][k];
					CHECK(q.planes == p.planes);
					for (uint8_t plane = 0; plane < p.planes; ++plane)
					{
						INFO("plane " << int(plane) << ": " << p.v[plane] << " / " << q.v[plane]);
						CHECK(std::abs(q.v[plane] - p.v[plane]) <= (plane >= uvFrom ? valueTolerance : tolerance));
					}
				}
			}
		}

		REQUIRE(b.roads.size() == a.roads.size());
		for (size_t i = 0; i < a.roads.size(); ++i)
		{
			INFO("road " << i);
			const XPlane::DsfRoadSegment &x = a.roads[i], &y = b.roads[i];
			CHECK(y.definition == x.definition);
			CHECK(y.subtype == x.subtype);
			CHECK(y.startJunction == x.startJunction);
			CHECK(y.endJunction == x.endJunction);
			CHECK(y.startLevel == x.startLevel);
			CHECK(y.endLevel == x.endLevel);
			REQUIRE(y.points.size() == x.points.size());
			for (size_t k = 0; k < x.points.size(); ++k)
			{
				INFO("point " << k);
				CHECK(y.points[k].control == x.points[k].control);
				CHECK(std::abs(y.points[k].longitude - x.points[k].longitude) <= tolerance);
				CHECK(std::abs(y.points[k].latitude - x.points[k].latitude) <= tolerance);
			}
		}
	}

	const std::map<ItemKind, size_t> EXPECTED_KINDS = {
		{ ItemKind::Object, 4 }, { ItemKind::Polygon, 2 }, { ItemKind::Orthophoto, 2 }, { ItemKind::Line, 2 }, { ItemKind::String, 2 },
		{ ItemKind::Forest, 4 }, { ItemKind::Facade, 4 }, { ItemKind::Autogen, 5 }, { ItemKind::RoadNode, 3 }, { ItemKind::Road, 2 },
		{ ItemKind::ExclusionZone, 1 }, { ItemKind::ExclusionPolygon, 1 } };		// The hedge's two rings are two placements
}

TEST_CASE("DSF import: every kind converts back to the same placements", "[project][dsf][import]")
{
	const SceneryProject project = MakeEveryKindProject();
	const DsfConversion first = BuildProjectDsf(project, WithObjects());
	REQUIRE(first.warnings.empty());
	CHECK(first.dsf.objects.size() == 4);
	CHECK(first.dsf.polygons.size() == 21);
	CHECK(first.dsf.roads.size() == 2);
	CHECK(first.dsf.GetExclusions().size() == 5);

	const DsfImport imported = ImportDsfItems(first.dsf);
	CHECK(imported.warnings.empty());
	CHECK(CountKinds(imported.items) == EXPECTED_KINDS);

	// Objects, polygons, road nodes, roads, exclusions
	std::vector<ItemKind> order;
	for (const SceneryItem &item : imported.items)
	{
		const ItemKind group = item.kind == ItemKind::Object || item.kind == ItemKind::RoadNode || item.kind == ItemKind::Road ||
			item.kind == ItemKind::ExclusionZone || item.kind == ItemKind::ExclusionPolygon ? item.kind : ItemKind::Polygon;
		if (order.empty() || order.back() != group)
			order.push_back(group);
	}
	CHECK(order == std::vector<ItemKind>{ ItemKind::Object, ItemKind::Polygon, ItemKind::RoadNode, ItemKind::Road, ItemKind::ExclusionZone, ItemKind::ExclusionPolygon });

	const DsfConversion second = BuildProjectDsf(ProjectWith(imported.items), WithObjects());
	CHECK(second.warnings.empty());
	RequireSameDsf(first.dsf, second.dsf, 1e-9, 1e-9);
	CHECK(GetDsfFingerprint(second.dsf) == GetDsfFingerprint(first.dsf));		// Bit for bit, in fact

	// And again: an imported project round trips onto itself
	const DsfImport again = ImportDsfItems(second.dsf);
	CHECK(GetDsfFingerprint(BuildProjectDsf(ProjectWith(again.items), WithObjects()).dsf) == GetDsfFingerprint(first.dsf));
}

TEST_CASE("DSF import: a written and read back tile converts back within the pool quantization", "[project][dsf][import]")
{
	const SceneryProject project = MakeEveryKindProject();
	const DsfConversion first = BuildProjectDsf(project, WithObjects());
	std::vector<uint8_t> bytes;
	std::string error;
	REQUIRE(XPlane::WriteDsf(first.dsf, bytes, &error));
	XPlane::DsfFile read;
	REQUIRE(XPlane::ReadDsf(bytes.data(), bytes.size(), read, &error));

	const DsfImport imported = ImportDsfItems(read);
	CHECK(imported.warnings.empty());
	CHECK(CountKinds(imported.items) == EXPECTED_KINDS);
	const DsfConversion second = BuildProjectDsf(ProjectWith(imported.items), WithObjects());
	CHECK(second.warnings.empty());
	// 16-bit pools put points within a few centimeters; texture coordinates within a step of 0..1
	RequireSameDsf(first.dsf, second.dsf, 1e-6, 1e-4);

	// Corners stay corners although their control points were quantized apart from them (the pad's third node: point 4)
	REQUIRE(read.polygons[1].windings[0].size() == 7);
	const XPlane::DsfPoint &corner = read.polygons[1].windings[0][4];
	CHECK((corner.v[2] != corner.v[0] || corner.v[3] != corner.v[1]));
	const std::vector<const SceneryItem *> polygons = OfKind(imported.items, ItemKind::Polygon);
	REQUIRE(polygons.size() == 2);
	const GeoRing &pad = polygons[1]->rings[0];
	REQUIRE(pad.size() == 4);
	CHECK_FALSE(pad[2].HasBezier());
	CHECK_FALSE(pad[3].HasLo());
	CHECK(pad[3].split);
}

TEST_CASE("DSF import: items come back as they were", "[project][dsf][import]")
{
	const SceneryProject project = MakeEveryKindProject();
	const DsfImport imported = ImportDsfItems(BuildProjectDsf(project, WithObjects()).dsf);
	REQUIRE(imported.warnings.empty());
	const std::vector<SceneryItem> &items = imported.items;
	for (const SceneryItem &item : items)
	{
		CHECK(item.id.size() == 12);
		CHECK(project.FindItem(item.id) == nullptr);		// New ids
	}

	SECTION("Objects: placement, elevation mode, name from the file")
	{
		const std::vector<const SceneryItem *> objects = OfKind(items, ItemKind::Object);
		REQUIRE(objects.size() == 4);
		const char *names[] = { "Hangar", "Tower", "Mast", "Farm" };
		const char *stems[] = { "hangar", "tower", "tower", "farm" };
		for (size_t i = 0; i < objects.size(); ++i)
		{
			const SceneryItem &original = Original(project, names[i]);
			CHECK(objects[i]->name == stems[i]);
			CHECK(objects[i]->resource == original.resource);
			CHECK(objects[i]->latitude == original.latitude);
			CHECK(objects[i]->longitude == original.longitude);
			CHECK(objects[i]->heading == original.heading);
			CHECK(objects[i]->elevationMode == original.elevationMode);
			CHECK(objects[i]->altitude == original.altitude);
		}
	}

	SECTION("Polygons: heading, split nodes and turned-around outlines")
	{
		const std::vector<const SceneryItem *> polygons = OfKind(items, ItemKind::Polygon);
		REQUIRE(polygons.size() == 2);
		CHECK(polygons[0]->textureHeading == 45.5f);
		REQUIRE(polygons[0]->rings.size() == 2);
		CHECK(RingArea(polygons[0]->rings[0]) > 0.0);		// Turned counter-clockwise by the export; the hole clockwise
		CHECK(RingArea(polygons[0]->rings[1]) < 0.0);
		CHECK(polygons[1]->textureHeading == 30.296875f);
		CheckSameRing(polygons[1]->rings[0], Original(project, "Pad").rings[0]);

		const std::vector<const SceneryItem *> orthos = OfKind(items, ItemKind::Orthophoto);
		REQUIRE(orthos.size() == 2);
		CheckSameRing(orthos[0]->rings[0], Original(project, "Ortho").rings[0]);
		CheckSameRing(orthos[1]->rings[0], Original(project, "Curved ortho").rings[0]);
	}

	SECTION("Lines and strings: split handles, open ends, closed strings")
	{
		const std::vector<const SceneryItem *> lines = OfKind(items, ItemKind::Line);
		REQUIRE(lines.size() == 2);
		CHECK_FALSE(lines[0]->closed);
		const GeoRing &chain = lines[0]->rings[0];
		CheckSameRing(chain, Original(project, "Taxi line").rings[0]);
		REQUIRE(chain.size() == 4);
		CHECK(chain[2].split);							// The split node's three points are one node again
		CHECK(chain[2].loLatitude == Catch::Approx(-0.0003).margin(1e-12));
		CHECK(chain[2].hiLongitude == Catch::Approx(0.0004).margin(1e-12));
		CHECK_FALSE(chain[0].split);					// Open ends: the handle that counts, mirrored
		CHECK(chain[0].loLatitude == Catch::Approx(-0.0001).margin(1e-12));
		CHECK(lines[1]->closed);
		CheckSameRing(lines[1]->rings[0], Original(project, "Fence").rings[0]);

		const std::vector<const SceneryItem *> strings = OfKind(items, ItemKind::String);
		REQUIRE(strings.size() == 2);
		CHECK_FALSE(strings[0]->closed);
		CHECK(strings[0]->spacing == 30.0f);
		CHECK(strings[1]->closed);						// Back on its first node: the repeat is dropped
		CHECK(strings[1]->spacing == 2.5f);
		CheckSameRing(strings[1]->rings[0], Original(project, "Lamps").rings[0]);
	}

	SECTION("Forests, facades and autogen")
	{
		const std::vector<const SceneryItem *> forests = OfKind(items, ItemKind::Forest);
		REQUIRE(forests.size() == 4);
		CHECK(forests[0]->fill == ForestFill::Area);
		CHECK(EncodeForestParameter(forests[0]->density, forests[0]->fill) == EncodeForestParameter(0.75f, ForestFill::Area));
		CHECK(forests[0]->rings.size() == 2);
		CHECK(forests[1]->fill == ForestFill::Line);
		CHECK(forests[2]->fill == ForestFill::Line);
		CHECK(forests[1]->rings[0].size() == 3);
		CHECK(forests[2]->rings[0].size() == 2);
		CHECK(forests[3]->fill == ForestFill::Points);
		CHECK(forests[3]->density == 1.0f);

		const std::vector<const SceneryItem *> facades = OfKind(items, ItemKind::Facade);
		REQUIRE(facades.size() == 4);
		const char *facadeNames[] = { "Office", "Shed", "Terminal", "Hall" };
		for (size_t i = 0; i < facades.size(); ++i)
		{
			INFO(facadeNames[i]);
			const SceneryItem &original = Original(project, facadeNames[i]);
			CHECK(facades[i]->pickWalls == original.pickWalls);
			CHECK(facades[i]->height == original.height);
			CheckSameRing(facades[i]->rings[0], original.rings[0]);
		}

		const std::vector<const SceneryItem *> autogen = OfKind(items, ItemKind::Autogen);
		REQUIRE(autogen.size() == 5);
		const char *autogenNames[] = { "Houses", "Barns", "Mixed", "Block", "Quiet block" };
		for (size_t i = 0; i < autogen.size(); ++i)
		{
			INFO(autogenNames[i]);
			const SceneryItem &original = Original(project, autogenNames[i]);
			CHECK(autogen[i]->resource == original.resource);
			CHECK(autogen[i]->height == original.height);
			CHECK(autogen[i]->spelling == original.spelling);
			REQUIRE(autogen[i]->rings.size() == original.rings.size());
			for (size_t r = 0; r < original.rings.size(); ++r)
				CheckSameRing(autogen[i]->rings[r], original.rings[r]);
		}
	}

	SECTION("Roads: one node per junction, shared by the roads meeting there")
	{
		const std::vector<const SceneryItem *> nodes = OfKind(items, ItemKind::RoadNode);
		const std::vector<const SceneryItem *> roads = OfKind(items, ItemKind::Road);
		REQUIRE(nodes.size() == 3);
		REQUIRE(roads.size() == 2);
		CHECK(roads[0]->endNode == roads[1]->startNode);
		CHECK(roads[0]->startNode != roads[1]->endNode);
		const char *names[] = { "West", "Middle", "East" };
		const std::string ends[] = { roads[0]->startNode, roads[0]->endNode, roads[1]->endNode };
		for (size_t i = 0; i < 3; ++i)
		{
			const SceneryItem *node = FindId(items, ends[i]);
			REQUIRE(node);
			CHECK(node->kind == ItemKind::RoadNode);
			CHECK(node->latitude == Original(project, names[i]).latitude);
			CHECK(node->longitude == Original(project, names[i]).longitude);
		}

		const SceneryItem &bridge = Original(project, "Bridge");
		CHECK(roads[1]->resource == bridge.resource);
		CHECK(roads[1]->roadSubtype == 101);
		CHECK(roads[1]->startLayer == 1);
		CHECK(roads[1]->endLayer == 1);
		CHECK(roads[0]->roadSubtype == 100);
		REQUIRE(roads[0]->rings[0].size() == 3);
		CHECK_FALSE(roads[0]->rings[0][1].HasBezier());

		const GeoRing &chain = roads[1]->rings[0];
		REQUIRE(chain.size() == 3);
		CHECK(chain[0].hiLatitude == Catch::Approx(0.0004).margin(1e-12));
		CHECK(chain[0].hiLongitude == Catch::Approx(0.0003).margin(1e-12));
		CHECK(chain[1].split);
		CHECK(chain[1].loLatitude == Catch::Approx(-0.0001).margin(1e-12));
		CHECK(chain[1].loLongitude == Catch::Approx(-0.00015).margin(1e-12));
		CHECK(chain[1].hiLatitude == Catch::Approx(0.0002).margin(1e-12));
		CHECK(chain[1].hiLongitude == Catch::Approx(0.0001).margin(1e-12));
		CHECK_FALSE(chain[2].HasLo());				// The control point the export made up for it isn't a handle
	}

	SECTION("Exclusions: one item per zone with all its types")
	{
		const std::vector<const SceneryItem *> zones = OfKind(items, ItemKind::ExclusionZone);
		REQUIRE(zones.size() == 1);
		CHECK(zones[0]->exclusions == (ExcludeObjects | ExcludeForests | ExcludeStrings));
		REQUIRE(zones[0]->rings.size() == 1);
		REQUIRE(zones[0]->rings[0].size() == 2);
		CHECK(zones[0]->rings[0][0].latitude == Catch::Approx(LAT + 0.019).margin(1e-6));
		CHECK(zones[0]->rings[0][1].longitude == Catch::Approx(LON - 0.017).margin(1e-6));

		const std::vector<const SceneryItem *> polygons = OfKind(items, ItemKind::ExclusionPolygon);
		REQUIRE(polygons.size() == 1);
		CHECK(polygons[0]->exclusions == (ExcludePolygons | ExcludeLines));
		CheckSameRing(polygons[0]->rings[0], Original(project, "No polygons").rings[0]);
	}
}

TEST_CASE("DSF import: exclusions merge only the way the export writes them", "[project][dsf][import]")
{
	XPlane::DsfFile dsf;
	const std::string a = "-122.640000/45.600000/-122.630000/45.610000";
	const std::string b = "-122.620000/45.600000/-122.610000/45.610000;-122.620000/45.600000,-122.610000/45.600000,-122.610000/45.610000";
	dsf.properties = { { "sim/overlay", "1" }, { "sim/exclude_obj", a }, { "sim/exclude_for", a }, { "sim/exclude_fac", a },
		{ "sim/exclude_lin", b }, { "sim/exclude_pol", b }, { "sim/exclude_net", a } };

	const DsfImport imported = ImportDsfItems(dsf);
	CHECK(imported.warnings.empty());
	REQUIRE(imported.items.size() == 4);
	CHECK(imported.items[0].kind == ItemKind::ExclusionZone);
	CHECK(imported.items[0].exclusions == (ExcludeObjects | ExcludeForests));
	CHECK(imported.items[1].exclusions == ExcludeFacades);		// Out of bit order: an item of its own
	CHECK(imported.items[2].kind == ItemKind::ExclusionPolygon);
	CHECK(imported.items[2].exclusions == (ExcludeLines | ExcludePolygons));
	CHECK(imported.items[2].rings[0].size() == 3);
	CHECK(imported.items[3].exclusions == ExcludeRoads);

	// Written back, the properties come out as they were
	const DsfConversion written = BuildProjectDsf(ProjectWith(imported.items));
	std::vector<std::pair<std::string, std::string>> exclusions;
	for (const auto &property : written.dsf.properties)
	{
		if (property.first.starts_with("sim/exclude_"))
			exclusions.push_back(property);
	}
	CHECK(exclusions == std::vector<std::pair<std::string, std::string>>(dsf.properties.begin() + 1, dsf.properties.end()));
}

TEST_CASE("DSF import: what can't be imported is a warning", "[project][dsf][import]")
{
	XPlane::DsfFile dsf;
	dsf.objectDefinitions = { "objects/shed.obj", "facades/wall.fac" };
	dsf.polygonDefinitions = { "beaches/sand.bch", "lines/taxi.lin" };
	dsf.networkDefinitions = { "roads/roads.txt" };
	dsf.terrainDefinitions = { "terrain/grass.ter" };
	dsf.terrainPatchCount = 12;

	XPlane::DsfObjectPlacement object;
	object.longitude = LON;
	object.latitude = LAT;
	object.filter = 0;							// Filters don't matter
	dsf.objects = { object, object, object };
	dsf.objects[1].definition = 1;
	dsf.objects[2].definition = 7;

	const auto point = [](const double latitude, const double longitude)
	{
		XPlane::DsfPoint p;
		p.planes = 2;
		p.v[0] = longitude;
		p.v[1] = latitude;
		return p;
	};
	XPlane::DsfPolygonPlacement beach;
	beach.windings = { { point(LAT, LON), point(LAT, LON + 0.001), point(LAT + 0.001, LON) } };
	XPlane::DsfPolygonPlacement dot;
	dot.definition = 1;
	dot.windings = { { point(LAT, LON) } };
	XPlane::DsfPolygonPlacement line = dot;
	line.windings[0].push_back(point(LAT, LON + 0.001));
	dsf.polygons = { beach, dot, line };

	XPlane::DsfRoadSegment road;
	road.startJunction = 1;
	road.endJunction = 2;
	road.points = { { LON, LAT, false }, { LON + 0.001, LAT, false } };
	dsf.roads = { road };
	dsf.properties = { { "sim/exclude_obj", "garbage" }, { "sim/exclude_xyz", "1/2/3/4" }, { "sim/exclude_fac", "-122.640000/45.600000/-122.630000/45.610000" } };

	const DsfImport imported = ImportDsfItems(dsf);
	const std::map<ItemKind, size_t> kinds = { { ItemKind::Object, 1 }, { ItemKind::Line, 1 }, { ItemKind::ExclusionZone, 1 } };
	CHECK(CountKinds(imported.items) == kinds);
	CHECK(imported.items[0].name == "shed");

	const std::vector<std::string> &warnings = imported.warnings;
	REQUIRE(warnings.size() == 8);
	CHECK_THAT(warnings[0], ContainsSubstring("object 1") && ContainsSubstring("wall.fac"));
	CHECK_THAT(warnings[1], ContainsSubstring("object 2") && ContainsSubstring("definition 7"));
	CHECK_THAT(warnings[2], ContainsSubstring("sand.bch"));
	CHECK_THAT(warnings[3], ContainsSubstring("polygon 1") && ContainsSubstring("at least 2"));
	CHECK_THAT(warnings[4], ContainsSubstring("roads.txt"));
	CHECK_THAT(warnings[5], ContainsSubstring("sim/exclude_obj") && ContainsSubstring("garbage"));
	CHECK_THAT(warnings[6], ContainsSubstring("sim/exclude_xyz"));
	CHECK_THAT(warnings[7], ContainsSubstring("terrain") && ContainsSubstring("12"));
}

TEST_CASE("DSF import: parameter decoders invert the encoders", "[project][dsf][import]")
{
	CHECK(DecodePolygonHeading(EncodePolygonHeading(90.5)) == 90.5);
	CHECK(DecodePolygonHeading(270) == 270.0);
	for (uint32_t parameter = 0; parameter < 360u * 128u; ++parameter)
	{
		if (EncodePolygonHeading(static_cast<float>(DecodePolygonHeading(static_cast<uint16_t>(parameter)))) != parameter)
			FAIL("heading parameter " << parameter);
	}

	CHECK(DecodeStringSpacing(30) == 30.0);
	CHECK(DecodeStringSpacing(10000 + 320) == 2.5);
	CHECK(EncodeStringSpacing(2.0401) == 10000 + 261);			// 261 / 128 = 2.039 is within 2 % of 2 m: decoded just above 2.04
	CHECK(EncodeStringSpacing(static_cast<float>(DecodeStringSpacing(10000 + 261))) == 10000 + 261);
	for (double spacing = 0.01; spacing < 700.0; spacing += 0.0137)
	{
		const uint16_t parameter = EncodeStringSpacing(spacing);
		if (EncodeStringSpacing(static_cast<float>(DecodeStringSpacing(parameter))) != parameter)
			FAIL("spacing " << spacing);
	}

	for (uint32_t parameter = 0; parameter < 3u * 256u; ++parameter)
	{
		const auto [density, fill] = DecodeForestParameter(static_cast<uint16_t>(parameter));
		if (EncodeForestParameter(density, fill) != parameter || density < 0.0f || density > 1.0f)
			FAIL("forest parameter " << parameter);
	}
	CHECK(DecodeForestParameter(255 + 512) == std::pair<float, ForestFill>{ 1.0f, ForestFill::Points });
	CHECK(DecodeForestParameter(9 * 256).second == ForestFill::Area);

	for (uint32_t parameter = 0; parameter <= 0xFFFFu; ++parameter)
	{
		const auto [height, low] = DecodeAutogenParameter(static_cast<uint16_t>(parameter));
		if (EncodeAutogenParameter(height, low) != parameter)
			FAIL("autogen parameter " << parameter);
	}
	CHECK(DecodeAutogenParameter(EncodeAutogenParameter(12.0f, 3)) == std::pair<float, int>{ 12.0f, 3 });
}

// -------------------------------------------------------

/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * GatewayValidationTests.cpp
 * -------------------------------------------------------
 * Scenery Gateway validation (xp_gateway_validation.*): WorldEditor's "Airport Scenery Gateway" export checks for
 * names and IDs, the boundary, the extent, art from the default library, roads and 1302 metadata.
 * -------------------------------------------------------
 */
#include <catch2/catch_all.hpp>
#include <algorithm>
#include <SceneryEditorX/xplane/xp_gateway_validation.h>
#include <SceneryEditorX/xplane/xp_library.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace SceneryEditorX::XPlane;

namespace
{
	constexpr double LAT = 44.4719;
	constexpr double LON = -73.1533;

	AptNode Node(const double latitude, const double longitude)
	{
		AptNode node;
		node.latitude = latitude;
		node.longitude = longitude;
		return node;
	}

	/* @brief A small airport that passes: one runway inside a boundary, a ramp start, Gateway-style metadata. */
	AptAirport MakeAirport()
	{
		AptAirport airport;
		airport.id = "KBTV";
		airport.name = "Burlington Intl";
		airport.elevationFt = 335;
		airport.SetMetadata("city", "Burlington");
		airport.SetMetadata("country", "USA United States");
		airport.SetMetadata("datum_lat", "44.47190000");
		airport.SetMetadata("datum_lon", "-73.15330000");
		airport.SetMetadata("gui_label", "2D");
		airport.SetMetadata("icao_code", "KBTV");
		airport.SetMetadata("region_code", "K6");
		airport.SetMetadata("state", "Vermont");

		AptRunway runway;
		runway.width = 45.0f;
		runway.ends[0].designator = "18";
		runway.ends[0].latitude = LAT;
		runway.ends[0].longitude = LON;
		runway.ends[1].designator = "36";
		runway.ends[1].latitude = LAT + 0.009;
		runway.ends[1].longitude = LON;
		airport.runways.push_back(runway);

		AptBoundary boundary;
		boundary.description = "Boundary";
		AptNodeChain ring;
		ring.nodes = { Node(LAT - 0.002, LON - 0.002), Node(LAT - 0.002, LON + 0.002), Node(LAT + 0.011, LON + 0.002), Node(LAT + 0.011, LON - 0.002) };
		boundary.rings.push_back(ring);
		airport.boundaries.push_back(boundary);

		AptStartup ramp;
		ramp.latitude = LAT + 0.004;
		ramp.longitude = LON + 0.001;
		ramp.name = "Ramp 1";
		airport.startups.push_back(ramp);
		return airport;
	}

	/* @brief Everything is X-Plane's own art. */
	GatewayValidationOptions DefaultLibrary()
	{
		GatewayValidationOptions options;
		options.describeResource = [](std::string_view) { return GatewayResourceInfo{ true, true, false }; };
		options.countSubtextures = [](std::string_view) { return 2; };
		return options;
	}

	GatewayValidation Validate(const AptAirport &airport, const DsfFile &overlay = {}, const GatewayValidationOptions &options = DefaultLibrary())
	{
		return ValidateGatewayAirport(airport, overlay, options);
	}

	/* @brief The message of the first problem with a code, "" when there's none. */
	std::string Message(const GatewayValidation &validation, const std::string_view code)
	{
		for (const GatewayProblem &problem : validation.problems)
		{
			if (problem.code == code)
				return problem.message;
		}
		return {};
	}

	std::string Describe(const GatewayValidation &validation)
	{
		std::string out;
		for (const GatewayProblem &problem : validation.problems)
			out += (problem.severity == GatewaySeverity::Error ? "E " : "W ") + problem.code + ": " + problem.message + "\n";
		return out;
	}

	DsfObjectPlacement Object(const uint32_t definition, const double latitude, const double longitude)
	{
		DsfObjectPlacement object;
		object.definition = definition;
		object.latitude = latitude;
		object.longitude = longitude;
		return object;
	}

	DsfRoadSegment Road(const double latitude, const double longitude, const double latitudeEnd, const double longitudeEnd)
	{
		DsfRoadSegment road;
		road.subtype = 1;
		road.startJunction = 1;
		road.endJunction = 2;
		road.points = { { longitude, latitude, false }, { longitudeEnd, latitudeEnd, false } };
		return road;
	}

	struct TempFolder
	{
		std::filesystem::path path;

		TempFolder()
		{
			path = std::filesystem::temp_directory_path() / ("sedx_gateway_validation_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
			std::filesystem::create_directories(path);
		}

		~TempFolder()
		{
			std::error_code ec;
			std::filesystem::remove_all(path, ec);
		}

		void Write(const std::string &relative, const std::string &text) const
		{
			const std::filesystem::path file = path / relative;
			std::filesystem::create_directories(file.parent_path());
			std::ofstream(file, std::ios::binary) << text;
		}
	};
}

TEST_CASE("Gateway validation: a well-formed airport passes", "[xplane][gateway][validation]")
{
	const GatewayValidation validation = Validate(MakeAirport());
	INFO(Describe(validation));
	CHECK(validation.problems.empty());
	CHECK_FALSE(validation.HasErrors());
	CHECK(validation.CountWarnings() == 0);
}

TEST_CASE("Gateway validation: airport name and ID", "[xplane][gateway][validation]")
{
	AptAirport airport = MakeAirport();

	airport.name.clear();
	CHECK(Validate(airport).Has("name-empty"));

	airport.name = "Burlington International Jetport Field";		// 38 characters
	GatewayValidation validation = Validate(airport);
	CHECK(validation.Has("name-too-long"));
	CHECK(validation.Has("name-style"));		// "International" is a warning of its own
	CHECK(validation.HasErrors());

	airport.name = " Burlington Intl";
	CHECK(Validate(airport).Has("name-spaces"));

	airport.name = "BURLINGTON INTL";
	CHECK(Validate(airport).Has("name-upper-case"));
	airport.name = "BTV";		// WorldEditor counts three capitals without a lower case letter as shouting
	CHECK(Validate(airport).Has("name-upper-case"));
	airport.name = "BT";
	CHECK_FALSE(Validate(airport).Has("name-upper-case"));

	// Closed airports are named "[X] ...": "(X)" and "{X}" are mistakes
	airport.name = "(X) Old Burlington";
	CHECK(Validate(airport).Has("name-closed"));
	airport.name = "{x} Old Burlington";
	CHECK(Validate(airport).Has("name-closed"));
	airport.name = "[X] Old Burlington";
	CHECK_FALSE(Validate(airport).Has("name-closed"));
	airport.name = "Burlington (Vermont)";
	CHECK_FALSE(Validate(airport).Has("name-closed"));

	// Style: warnings only
	for (const char *name : { "Burlington Airport", "Burlington Regional", "KBTV Burlington" })
	{
		airport.name = name;
		validation = Validate(airport);
		INFO(name << "\n" << Describe(validation));
		CHECK(validation.Has("name-style"));
		CHECK_FALSE(validation.HasErrors());
	}
	airport.name = "Burlington Airporter";		// Not the word "Airport"
	CHECK_FALSE(Validate(airport).Has("name-style"));

	// 30 characters of UTF-8 count as 30, not as their bytes
	airport.name = "\xC3\x9C" "berlingen Fl\xC3\xBCgplatz Bodensee";
	CHECK_FALSE(Validate(airport).Has("name-too-long"));

	airport.name = "Burlington Intl";
	airport.id = "KB-TV";
	CHECK(Validate(airport).Has("id-characters"));
	airport.id.clear();
	CHECK(Validate(airport).Has("id-empty"));
}

TEST_CASE("Gateway validation: the airport boundary", "[xplane][gateway][validation]")
{
	AptAirport airport = MakeAirport();

	SECTION("runways need a boundary")
	{
		airport.boundaries.clear();
		CHECK(Validate(airport).Has("boundary-missing"));
		airport.runways.clear();
		airport.type = AirportType::Heliport;
		AptHelipad helipad;
		helipad.latitude = LAT;
		helipad.longitude = LON;
		airport.helipads.push_back(helipad);
		CHECK_FALSE(Validate(airport).Has("boundary-missing"));
	}

	SECTION("no bezier curves")
	{
		AptNode &node = airport.boundaries[0].rings[0].nodes[1];
		node.hasControl = true;
		node.controlLatitude = node.latitude + 0.0005;
		node.controlLongitude = node.longitude;
		CHECK(Validate(airport).Has("boundary-bezier"));
		// A control point on its node is a corner
		node.controlLatitude = node.latitude;
		CHECK_FALSE(Validate(airport).Has("boundary-bezier"));
	}

	SECTION("sides at least 30 m long")
	{
		std::vector<AptNode> &nodes = airport.boundaries[0].rings[0].nodes;
		nodes.insert(nodes.begin() + 1, Node(LAT - 0.002, LON - 0.0018));		// ~16 m after the first node
		CHECK(Validate(airport).Has("boundary-short-side"));
	}

	SECTION("runways, taxiways and ramp starts inside it")
	{
		airport.runways[0].ends[1].latitude = LAT + 0.02;		// Past the north side
		CHECK(Validate(airport).Has("runway-outside-boundary"));

		airport = MakeAirport();
		airport.runways[0].width = 400.0f;						// The corners reach past the sides
		CHECK(Validate(airport).Has("runway-outside-boundary"));

		airport = MakeAirport();
		AptPavement pavement;
		AptNodeChain ring;
		ring.nodes = { Node(LAT, LON + 0.001), Node(LAT, LON + 0.003), Node(LAT + 0.001, LON + 0.003), Node(LAT + 0.001, LON + 0.001) };
		pavement.rings.push_back(ring);
		pavement.description = "Apron";
		airport.pavements.push_back(pavement);
		GatewayValidation validation = Validate(airport);
		CHECK(Message(validation, "taxiway-outside-boundary").find("'Apron' isn't fully inside") != std::string::npos);

		airport = MakeAirport();
		airport.startups[0].longitude = LON + 0.01;
		CHECK(Validate(airport).Has("ramp-outside-boundary"));
	}
}

TEST_CASE("Gateway validation: the airport's extent", "[xplane][gateway][validation]")
{
	AptAirport airport = MakeAirport();
	DsfFile overlay;
	overlay.objectDefinitions = { "lib/airport/vehicles/baggage_handling/tractor.obj" };

	// 0.5 nm past the boundary is fine, a whole nautical mile isn't
	overlay.objects = { Object(0, LAT + 0.011 + 0.5 / 60.0, LON) };
	CHECK_FALSE(Validate(airport, overlay).Has("far-outside-boundary"));
	overlay.objects = { Object(0, LAT + 0.011 + 1.0 / 60.0, LON) };
	GatewayValidation validation = Validate(airport, overlay);
	CHECK(Message(validation, "far-outside-boundary").find("tractor.obj") != std::string::npos);

	// Point features of the airport count too, runways and painted lines don't
	overlay.objects.clear();
	AptWindsock windsock;
	windsock.latitude = LAT - 0.03;
	windsock.longitude = LON;
	airport.windsocks.push_back(windsock);
	CHECK(Validate(airport, overlay).Has("far-outside-boundary"));

	// At most 7 nm across, whatever the boundary says (KSEA and KEDW get three times that)
	airport = MakeAirport();
	airport.runways[0].ends[1].latitude = LAT + 8.0 / 60.0;
	airport.boundaries[0].rings[0].nodes[2].latitude = LAT + 8.1 / 60.0;
	airport.boundaries[0].rings[0].nodes[3].latitude = LAT + 8.1 / 60.0;
	validation = Validate(airport);
	CHECK(validation.Has("airport-too-large"));
	airport.id = "KSEA";
	airport.SetMetadata("icao_code", "KSEA");
	CHECK_FALSE(Validate(airport).Has("airport-too-large"));
}

TEST_CASE("Gateway validation: art from the default library only", "[xplane][gateway][validation]")
{
	const AptAirport airport = MakeAirport();
	DsfFile overlay;
	overlay.objectDefinitions = { "lib/airport/Common_Elements/Hangars/Hangar_1.obj", "objects/my_tower.obj", "lib/old/thing.obj" };
	overlay.objects = { Object(0, LAT + 0.001, LON + 0.001), Object(1, LAT + 0.002, LON + 0.001), Object(1, LAT + 0.003, LON + 0.001), Object(2, LAT + 0.004, LON + 0.001) };

	GatewayValidationOptions options = DefaultLibrary();
	options.describeResource = [](const std::string_view path) -> GatewayResourceInfo
	{
		if (path.starts_with("objects/"))
			return { false, false, false };		// The pack's own file
		if (path.starts_with("lib/old/"))
			return { true, true, true };		// DEPRECATED in X-Plane's library
		return { true, true, false };
	};
	GatewayValidation validation = Validate(airport, overlay, options);
	INFO(Describe(validation));
	REQUIRE(validation.Has("art-not-default"));
	CHECK(validation.Has("art-private"));
	CHECK(Message(validation, "art-not-default").find("'objects/my_tower.obj' (2x) (no library has it)") != std::string::npos);
	CHECK(Message(validation, "art-not-default").find("Hangar_1") == std::string::npos);

	// Without a scanned library nothing can be vouched for
	options.describeResource = nullptr;
	CHECK(Validate(airport, overlay, options).Has("art-unchecked"));
	CHECK_FALSE(Validate(airport, {}, options).Has("art-unchecked"));		// No art, nothing to check

	// Orthophotos need automatic subtextures
	DsfFile orthophotos;
	orthophotos.polygonDefinitions = { "lib/airport/ground/signs/sign.pol" };
	DsfPolygonPlacement polygon;
	polygon.parameter = 65535;
	polygon.planes = 4;
	polygon.windings.push_back({});
	for (const auto &[dLat, dLon] : { std::pair{ 0.0, 0.0 }, { 0.0, 0.0005 }, { 0.0005, 0.0005 } })
	{
		DsfPoint point;
		point.planes = 4;
		point.v[0] = LON + dLon;
		point.v[1] = LAT + dLat;
		polygon.windings[0].push_back(point);
	}
	orthophotos.polygons.push_back(polygon);
	options = DefaultLibrary();
	CHECK_FALSE(Validate(airport, orthophotos, options).Has("orthophoto"));
	options.countSubtextures = [](std::string_view) { return 0; };
	CHECK(Validate(airport, orthophotos, options).Has("orthophoto"));
	options.countSubtextures = nullptr;
	CHECK(Validate(airport, orthophotos, options).Has("orthophoto"));
}

TEST_CASE("Gateway validation: object elevations, trucks, runway surfaces", "[xplane][gateway][validation]")
{
	AptAirport airport = MakeAirport();
	DsfFile overlay;
	overlay.objectDefinitions = { "lib/airport/Common_Elements/Hangars/Hangar_1.obj" };
	DsfObjectPlacement object = Object(0, LAT + 0.001, LON + 0.001);

	object.mode = DsfObjectPlacement::Mode::Msl;
	object.elevation = 120.0;
	overlay.objects = { object };
	GatewayValidation validation = Validate(airport, overlay);
	CHECK(validation.Has("set-msl"));
	CHECK(validation.HasErrors());

	object.mode = DsfObjectPlacement::Mode::Agl;
	object.elevation = 12.0;
	overlay.objects = { object };
	validation = Validate(airport, overlay);
	CHECK(validation.Has("set-agl"));
	CHECK_FALSE(validation.HasErrors());
	object.elevation = 150.0;
	overlay.objects = { object };
	CHECK(Validate(airport, overlay).Has("agl-range"));

	AptTruckParking truck;
	truck.latitude = LAT + 0.003;
	truck.longitude = LON + 0.001;
	truck.name = "GPU";
	airport.truckParkings.push_back(truck);
	CHECK_FALSE(Validate(airport).Has("custom-truck"));
	airport.truckParkings[0].customObject = "objects/my_gpu.obj";
	CHECK(Validate(airport).Has("custom-truck"));

	airport = MakeAirport();
	airport.runways[0].surface = SurfaceTransparent;
	CHECK(Validate(airport).Has("runway-transparent"));
}

TEST_CASE("Gateway validation: roads", "[xplane][gateway][validation]")
{
	const AptAirport airport = MakeAirport();
	DsfFile overlay;
	overlay.networkDefinitions = { "lib/g10/roads.net", "lib/g10/roads_EU.net", "lib/g8/roads.net" };
	DsfRoadSegment road = Road(LAT + 0.001, LON + 0.001, LAT + 0.002, LON + 0.0015);
	overlay.roads = { road };
	GatewayValidation validation = Validate(airport, overlay);
	INFO(Describe(validation));
	CHECK_FALSE(validation.HasErrors());

	road.definition = 1;
	overlay.roads = { road };
	CHECK_FALSE(Validate(airport, overlay).Has("road-network"));
	road.definition = 2;
	overlay.roads = { road };
	CHECK(Validate(airport, overlay).Has("road-network"));

	road.definition = 0;
	road.endLevel = 6;
	overlay.roads = { road };
	CHECK(Validate(airport, overlay).Has("road-layer"));

	road.endLevel = 0;
	road.points.back().latitude = LAT + 0.1;
	overlay.roads = { road };
	CHECK(Validate(airport, overlay).Has("road-outside"));

	// A node a meter off a tile edge, but not on it
	AptAirport edge = MakeAirport();
	DsfFile tile;
	tile.networkDefinitions = { "lib/g10/roads.net" };
	tile.roads = { Road(44.4719, -73.00001, 44.4725, -73.0005) };
	for (AptNode &node : edge.boundaries[0].rings[0].nodes)
		node.longitude += 0.15;
	edge.runways[0].ends[0].longitude += 0.15;
	edge.runways[0].ends[1].longitude += 0.15;
	edge.startups[0].longitude += 0.15;
	edge.SetMetadata("datum_lon", "-73.00330000");
	CHECK(Validate(edge, tile).Has("road-tile-edge"));
	CHECK(IsGatewayRoadNetwork("lib/g10/roads.net"));
	CHECK_FALSE(IsGatewayRoadNetwork("lib/g10/roads_eu.net"));
}

TEST_CASE("Gateway validation: 1302 metadata", "[xplane][gateway][validation]")
{
	AptAirport airport = MakeAirport();
	const auto has = [&](const char *key, const char *value, const char *code = "metadata")
	{
		AptAirport copy = airport;
		copy.SetMetadata(key, value);
		const GatewayValidation validation = Validate(copy);
		INFO(key << " = '" << value << "'\n" << Describe(validation));
		return validation.Has(code);
	};

	// Country: an upper case ISO-3166 alpha-3 code first, then an optional name
	CHECK_FALSE(has("country", "USA United States"));
	CHECK_FALSE(has("country", "USA"));
	CHECK_FALSE(has("country", "NZL New Zealand"));		// A three-letter first word isn't a second code
	CHECK(has("country", "Testland"));
	CHECK(has("country", "usa United States"));
	CHECK(has("country", "XYZ Nowhere"));
	CHECK(has("country", "USA USA United States"));
	CHECK(has("country", "USA 1st Republic"));
	CHECK(IsIso3166Alpha3("DEU"));
	CHECK_FALSE(IsIso3166Alpha3("deu"));
	CHECK_FALSE(IsIso3166Alpha3("XKX"));

	// Datum latitude / longitude: numbers, as a pair, within 1 nm of the airport
	CHECK(has("datum_lat", "north"));
	CHECK(has("datum_lon", "-73.30000000"));		// ~6.5 nm west
	{
		AptAirport copy = airport;
		copy.RemoveMetadata("datum_lon");
		CHECK(Validate(copy).Has("metadata"));
		copy.RemoveMetadata("datum_lat");
		CHECK_FALSE(Validate(copy).Has("metadata"));
	}

	// Codes
	CHECK(has("faa_code", "B"));
	CHECK_FALSE(has("faa_code", "BT1"));
	CHECK(has("iata_code", "BT1"));
	CHECK_FALSE(has("iata_code", "BTV"));
	CHECK(has("icao_code", "XBTV"));
	CHECK(has("icao_code", "KBT"));
	CHECK(has("local_code", "B"));
	CHECK(has("local_authority", "F4A"));
	CHECK_FALSE(has("local_authority", "FAA"));
	{
		AptAirport copy = airport;
		copy.SetMetadata("faa_code", "BTV");
		copy.SetMetadata("local_code", "BTV");
		CHECK(Validate(copy).Has("metadata"));
	}

	// Region codes (case doesn't matter)
	CHECK_FALSE(has("region_code", "k6"));
	CHECK_FALSE(has("region_code", "ED"));
	CHECK(has("region_code", "QQ"));

	// Words that mustn't be numbers
	CHECK(has("city", "12"));
	CHECK(has("state", "51"));
	CHECK(has("state", "1st State"));
	{
		AptAirport copy = airport;
		copy.SetMetadata("country", "THA Thailand");
		copy.SetMetadata("city", "Nan");		// strtod reads "Nan" as a number: VTCN is in Nan, Thailand
		CHECK_FALSE(Validate(copy).Has("metadata"));
	}

	// Transition altitude / level: feet, or flight levels under 1000
	CHECK_FALSE(has("transition_alt", "18000"));
	CHECK(has("transition_alt", "50"));
	CHECK(has("transition_alt", "40000"));
	CHECK(has("transition_alt", "high"));
	{
		AptAirport copy = airport;
		copy.SetMetadata("transition_alt", "5000");
		copy.SetMetadata("transition_level", "6000");		// 1000 ft above the altitude: fine
		CHECK_FALSE(Validate(copy).Has("metadata"));
		copy.SetMetadata("transition_level", "60");			// Under 1000: a flight level, and FL60 is too low for one
		CHECK(Validate(copy).Has("metadata"));
		copy.SetMetadata("transition_level", "100");		// FL100 = 10000 ft: 5000 ft above
		CHECK(Validate(copy).Has("metadata"));
		copy.SetMetadata("transition_alt", "180");			// FL180 above FL100
		CHECK(Validate(copy).Has("metadata"));
	}

	CHECK_FALSE(has("allows_circuits", "1"));
	CHECK(has("allows_circuits", "yes"));
	CHECK_FALSE(has("tower_service_type", "fiso"));
	CHECK(has("tower_service_type", "full"));
	CHECK(has("gui_label", "4D"));
	CHECK_FALSE(has("altimeter_setting", "QFE"));
	CHECK(has("altimeter_setting", "QNE"));

	// No URLs anywhere
	CHECK(has("city", "https://example.com", "metadata-url"));
	CHECK(has("state", "see HTTP site", "metadata-url"));

	const std::vector<std::string_view> &keys = GetGatewayMetadataKeys();
	CHECK(std::is_sorted(keys.begin(), keys.end()));
	CHECK(std::binary_search(keys.begin(), keys.end(), "datum_lat"));
	CHECK_FALSE(std::binary_search(keys.begin(), keys.end(), "flatten"));
}

TEST_CASE("Gateway validation: what X-Plane's library says about art", "[xplane][gateway][validation]")
{
	TempFolder folder;
	folder.Write("default/library.txt", "A\n800\nLIBRARY\n\nEXPORT lib/test/public.obj public.obj\nEXPORT lib/test/semi.obj semi.obj\n"
		"PRIVATE\nEXPORT lib/test/private.obj private.obj\nPUBLIC\nDEPRECATED\nEXPORT lib/test/old.obj old.obj\nPUBLIC\nSEMI_DEPRECATED\nEXPORT lib/test/semi2.obj semi.obj\n"
		// A status replaces the one before it, as in X-Plane's "1000 autogen" library (DEPRECATED, then SEMI_DEPRECATED)
		"DEPRECATED\nEXPORT lib/test/old2.obj old.obj\nSEMI_DEPRECATED\nEXPORT lib/test/semi3.obj semi.obj\nPRIVATE\nEXPORT lib/test/private2.obj private.obj\n");
	folder.Write("custom/library.txt", "A\n800\nLIBRARY\n\nEXPORT lib/test/public.obj mine.obj\nEXPORT ZDP_Library/tower.obj tower.obj\nEXPORT lib/test/private.obj mine.obj\n");

	std::vector<LibraryPackage> packages(2);
	packages[0].name = "Custom";
	packages[0].path = folder.path / "custom";
	packages[0].source = PackageSource::CustomScenery;
	packages[0].hasLibrary = true;
	packages[1].name = "Default";
	packages[1].path = folder.path / "default";
	packages[1].source = PackageSource::DefaultScenery;
	packages[1].hasLibrary = true;
	LibraryIndex index;
	index.Build(packages, { false });

	GatewayResourceInfo info = DescribeGatewayResource(index, "lib/test/public.obj");
	CHECK(info.known);
	CHECK(info.defaultLibrary);			// A custom library exporting it too doesn't matter
	CHECK_FALSE(info.privateOrDeprecated);

	info = DescribeGatewayResource(index, "lib/test/private.obj");
	CHECK(info.defaultLibrary);
	CHECK(info.privateOrDeprecated);	// The custom pack's public export doesn't make X-Plane's private one public

	CHECK(DescribeGatewayResource(index, "lib/test/old.obj").privateOrDeprecated);
	CHECK_FALSE(DescribeGatewayResource(index, "lib/test/semi2.obj").privateOrDeprecated);
	CHECK(DescribeGatewayResource(index, "lib/test/old2.obj").privateOrDeprecated);
	CHECK_FALSE(DescribeGatewayResource(index, "lib/test/semi3.obj").privateOrDeprecated);
	CHECK(DescribeGatewayResource(index, "lib/test/private2.obj").privateOrDeprecated);
	const LibraryAsset *semi = index.Find("lib/test/semi3.obj");
	REQUIRE(semi);
	CHECK(semi->providers[0].semiDeprecated);
	CHECK_FALSE(semi->providers[0].deprecated);

	info = DescribeGatewayResource(index, "ZDP_Library/tower.obj");
	CHECK(info.known);
	CHECK_FALSE(info.defaultLibrary);

	info = DescribeGatewayResource(index, "objects/nothing.obj");
	CHECK_FALSE(info.known);
	CHECK_FALSE(info.defaultLibrary);

	info = DescribeGatewayResource(index, "::FLATTEN::.pol");
	CHECK(info.defaultLibrary);
	CHECK_FALSE(info.privateOrDeprecated);
}

/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * GatewayPackageTests.cpp
 * -------------------------------------------------------
 * Packing an airport for the Scenery Gateway (xp_gateway_package.*): type and features from the content, the master
 * zip with <ICAO>.dat, the DSF2Text <ICAO>.txt and the pack (apt.dat, binary DSF tiles, README, COPYING), deflated
 * zips (xp_zip.*), the DSF text writer (xp_dsf.*), X-Plane's build from Log.txt and a pack's DSF content per airport.
 * -------------------------------------------------------
 */
#include <catch2/catch_all.hpp>
#include <SceneryEditorX/xplane/xp_dsf.h>
#include <SceneryEditorX/xplane/xp_gateway_package.h>
#include <SceneryEditorX/xplane/xp_zip.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <random>
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
		airport.SetMetadata("flatten", "1");			// apt.dat only: WorldEditor doesn't upload it as metadata
		airport.SetMetadata("gui_label", "3D");			// Wrong for a 2D airport: packaging sets it
		airport.SetMetadata("icao_code", "KBTV");

		AptRunway runway;
		runway.ends[0].designator = "18";
		runway.ends[0].latitude = LAT;
		runway.ends[0].longitude = LON;
		runway.ends[1].designator = "36";
		runway.ends[1].latitude = LAT + 0.009;
		runway.ends[1].longitude = LON;
		airport.runways.push_back(runway);

		AptBoundary boundary;
		AptNodeChain ring;
		ring.nodes = { Node(LAT - 0.002, LON - 0.002), Node(LAT - 0.002, LON + 0.002), Node(LAT + 0.011, LON + 0.002), Node(LAT + 0.011, LON - 0.002) };
		boundary.rings.push_back(ring);
		airport.boundaries.push_back(boundary);
		return airport;
	}

	DsfPoint Point(const double latitude, const double longitude, const uint8_t planes = 2)
	{
		DsfPoint point;
		point.planes = planes;
		point.v[0] = longitude;
		point.v[1] = latitude;
		return point;
	}

	/* @brief Objects, a facade, an exclusion and a road around the airport. */
	DsfFile MakeOverlay()
	{
		DsfFile dsf;
		dsf.properties = { { "sim/overlay", "1" }, { "sim/exclude_obj", "-73.155000/44.471000/-73.152000/44.473000" } };
		dsf.objectDefinitions = { "lib/airport/Common_Elements/Hangars/Hangar_1.obj" };
		dsf.polygonDefinitions = { "lib/buildings/facades/hangar.fac" };
		dsf.networkDefinitions = { "lib/g10/roads.net" };
		DsfObjectPlacement object;
		object.latitude = LAT + 0.004;
		object.longitude = LON + 0.001;
		object.heading = 90.0;
		dsf.objects.push_back(object);
		object.latitude = LAT + 0.005;
		object.mode = DsfObjectPlacement::Mode::Agl;
		object.elevation = 4.5;
		dsf.objects.push_back(object);

		DsfPolygonPlacement facade;
		facade.parameter = 12;
		facade.windings.push_back({ Point(LAT + 0.006, LON + 0.0010), Point(LAT + 0.006, LON + 0.0015), Point(LAT + 0.0065, LON + 0.0015), Point(LAT + 0.0065, LON + 0.0010) });
		dsf.polygons.push_back(facade);

		DsfRoadSegment road;
		road.subtype = 3;
		road.startJunction = 1;
		road.endJunction = 2;
		road.points = { { LON + 0.0010, LAT + 0.001, false }, { LON + 0.0012, LAT + 0.0015, true }, { LON + 0.0014, LAT + 0.0018, true }, { LON + 0.0016, LAT + 0.002, false } };
		dsf.roads.push_back(road);
		return dsf;
	}

	std::vector<ZipEntry> Unzip(const std::vector<uint8_t> &bytes)
	{
		std::vector<ZipEntry> entries;
		std::string error;
		INFO(error);
		REQUIRE(ReadZipArchive(bytes, entries, &error));
		return entries;
	}

	const ZipEntry *Find(const std::vector<ZipEntry> &entries, const std::string &name)
	{
		for (const ZipEntry &entry : entries)
		{
			if (entry.name == name)
				return &entry;
		}
		return nullptr;
	}

	std::string Text(const ZipEntry &entry)
	{
		return std::string(entry.data.begin(), entry.data.end());
	}

	/* @brief The compression method of each local header, in order. */
	std::vector<uint16_t> Methods(const std::vector<uint8_t> &zip)
	{
		std::vector<uint16_t> methods;
		size_t at = 0;
		const auto u16 = [&](const size_t offset) { return static_cast<uint32_t>(zip.at(offset) | (zip.at(offset + 1) << 8)); };
		const auto u32 = [&](const size_t offset) { return u16(offset) | (u16(offset + 2) << 16); };
		while (at + 30 <= zip.size() && u32(at) == 0x04034B50u)
		{
			methods.push_back(static_cast<uint16_t>(u16(at + 8)));
			at += 30 + u16(at + 26) + u16(at + 28) + u32(at + 18);
		}
		return methods;
	}
}

TEST_CASE("Gateway package: type and features from the content", "[xplane][gateway][package]")
{
	AptAirport airport = MakeAirport();
	GatewayContentSummary summary = SummarizeGatewayContent(airport, {});
	CHECK_FALSE(summary.is3D);
	CHECK_FALSE(summary.hasDsf);
	CHECK(summary.GetFeatures().empty());
	CHECK(summary.runways == 1);

	// ATC flows (1), aircraft taxi routes (2), truck routes (8), roads (86), in that order
	airport.flows.emplace_back();
	airport.taxiNetwork.emplace();
	AptTaxiEdge edge;
	airport.taxiNetwork->edges.push_back(edge);
	edge.groundVehicles = true;
	airport.taxiNetwork->edges.push_back(edge);
	const DsfFile overlay = MakeOverlay();
	summary = SummarizeGatewayContent(airport, overlay);
	CHECK(summary.GetFeatures() == "1,2,8,86");
	CHECK(summary.GetFeatureNames() == std::vector<std::string>{ "ATC flows", "Taxi routes", "Truck routes", "Roads" });
	CHECK(summary.is3D);
	CHECK(summary.hasDsf);
	CHECK(summary.objects == 2);
	CHECK(summary.facades == 1);
	CHECK(summary.roadSegments == 1);
	CHECK(summary.exclusions == 1);

	// Draped polygons and painted lines lie flat: 2D, but still DSF content
	DsfFile flat;
	flat.polygonDefinitions = { "lib/airport/lines/solid_yellow.lin", "lib/airport/ground/terrain/grass.pol" };
	DsfPolygonPlacement line;
	line.windings.push_back({ Point(LAT, LON), Point(LAT + 0.001, LON) });
	flat.polygons.push_back(line);
	line.definition = 1;
	line.windings[0].push_back(Point(LAT + 0.001, LON + 0.001));
	flat.polygons.push_back(line);
	summary = SummarizeGatewayContent(MakeAirport(), flat);
	CHECK_FALSE(summary.is3D);
	CHECK(summary.hasDsf);
	CHECK(summary.linePlacements == 1);
	CHECK(summary.polygons == 1);

	// Jetways stick up
	airport = MakeAirport();
	airport.jetways.emplace_back();
	CHECK(SummarizeGatewayContent(airport, {}).is3D);

	ApplyGatewayGuiLabel(airport, true);
	CHECK(airport.GetMetadata("gui_label") == "3D");
	ApplyGatewayGuiLabel(airport, false);
	CHECK(airport.GetMetadata("gui_label") == "2D");
}

TEST_CASE("Gateway package: a 2D airport", "[xplane][gateway][package]")
{
	GatewayPackageInput input;
	input.airport = MakeAirport();
	input.uploader = "Test Artist";
	input.comments = "Moved the boundary.";
	input.parentId = 900001;
	input.validatedAgainst = "12.4.4-b5-3a0f7e10";

	GatewayPackage package;
	std::string error;
	REQUIRE(BuildGatewayPackage(input, package, &error));
	const GatewaySubmission &submission = package.submission;
	CHECK(submission.icao == "KBTV");
	CHECK(submission.airportName == "Burlington Intl");
	CHECK(submission.artistComments == "Moved the boundary.");
	CHECK(submission.parentId == 900001);
	CHECK_FALSE(submission.is3D);
	CHECK(submission.features.empty());
	CHECK(submission.validatedAgainst == "12.4.4-b5-3a0f7e10");
	CHECK(submission.latitude == Catch::Approx(LAT + 0.0045));
	CHECK(submission.longitude == Catch::Approx(LON));
	CHECK(submission.metadata.at("gui_label") == "2D");				// Forced to the content
	CHECK(submission.metadata.at("country") == "USA United States");
	CHECK_FALSE(submission.metadata.contains("flatten"));			// Not a key WorldEditor uploads
	CHECK(package.airport.GetMetadata("gui_label") == "2D");
	CHECK(package.dsfText.empty());

	// The master zip: <ICAO>.dat and the pack, no DSF text without DSF content
	const std::vector<ZipEntry> master = Unzip(submission.masterZip);
	REQUIRE(master.size() == 2);
	const ZipEntry *dat = Find(master, "KBTV.dat");
	REQUIRE(dat);
	CHECK(Text(*dat) == package.aptDat);
	CHECK(package.aptDat.starts_with("I\n1200 Generated by Scenery Editor X\n"));
	CHECK(package.aptDat.find("\n1302 gui_label 2D\n") != std::string::npos);
	CHECK_FALSE(Find(master, "KBTV.txt"));
	const ZipEntry *packZip = Find(master, "KBTV_Scenery_Pack.zip");
	REQUIRE(packZip);
	CHECK(Methods(submission.masterZip).front() == 8);				// Deflated

	const std::vector<ZipEntry> pack = Unzip(packZip->data);
	REQUIRE(pack.size() == 3);
	CHECK(pack[0].name == "KBTV_Scenery_Pack/Earth nav data/apt.dat");
	CHECK(Text(pack[0]) == package.aptDat);
	const ZipEntry *readme = Find(pack, "KBTV_Scenery_Pack/README.txt");
	REQUIRE(readme);
	const std::string readmeText = Text(*readme);
	CHECK(readmeText.find("Burlington Intl (KBTV)") != std::string::npos);
	CHECK(readmeText.find("Uploaded by: Test Artist.") != std::string::npos);
	CHECK(readmeText.find("Moved the boundary.") != std::string::npos);
	CHECK(readmeText.find("GNU General Public\nLicense as published by the Free Software Foundation; either version 2") != std::string::npos);
	const ZipEntry *copying = Find(pack, "KBTV_Scenery_Pack/COPYING");
	REQUIRE(copying);
	CHECK(Text(*copying) == GetGatewayLicenseText());
	CHECK(GetGatewayLicenseText().size() == 18013);
	CHECK(GetGatewayLicenseText().find("GNU GENERAL PUBLIC LICENSE\n\t\t       Version 2, June 1991") != std::string::npos);
	CHECK(GetGatewayLicenseText().find("END OF TERMS AND CONDITIONS") != std::string::npos);

	// The upload body carries it all
	GatewayServer server;
	REQUIRE(ParseGatewayServer("http://127.0.0.1:8780", server));
	const GatewayHttpRequest request = MakeGatewayUploadRequest(server, submission, "Test Artist", "pw");
	CHECK(request.body.find("\"parentId\":900001") != std::string::npos);
	CHECK(request.body.find("\"type\":\"2D\"") != std::string::npos);

	// A first upload, and IDs that can't name files
	input.parentId = 0;
	REQUIRE(BuildGatewayPackage(input, package));
	CHECK(package.submission.parentId == 0);
	input.airport.id = "KB/TV";
	CHECK_FALSE(BuildGatewayPackage(input, package, &error));
	CHECK(error.find("KB/TV") != std::string::npos);
}

TEST_CASE("Gateway package: a 3D airport with DSF content", "[xplane][gateway][package]")
{
	GatewayPackageInput input;
	input.airport = MakeAirport();
	input.overlay = MakeOverlay();
	input.overlay.objects[0].filter = 4;		// Whatever filters it had: the package's are its own
	input.uploader = "Test Artist";

	GatewayPackage package;
	REQUIRE(BuildGatewayPackage(input, package));
	CHECK(package.submission.is3D);
	CHECK(package.submission.features == "86");
	CHECK(package.submission.metadata.at("gui_label") == "3D");
	CHECK(package.warnings.empty());

	// <ICAO>.txt: DSF2Text with WorldEditor's whole-world header, everything in this airport's filter
	const std::string &text = package.dsfText;
	REQUIRE_FALSE(text.empty());
	CHECK(text.starts_with("I\n800 written by Scenery Editor X\nDSF2TEXT\n\n"));
	CHECK(text.find("PROPERTY sim/west -180\nPROPERTY sim/east 180\nPROPERTY sim/north 90\nPROPERTY sim/south -90\nPROPERTY sim/planet earth\n") != std::string::npos);
	CHECK(text.find("PROPERTY sim/overlay 1\nPROPERTY sim/filter/aptid KBTV\nPROPERTY sim/exclude_obj -73.155000/44.471000/-73.152000/44.473000\n") != std::string::npos);
	CHECK(text.find("OBJECT_DEF lib/airport/Common_Elements/Hangars/Hangar_1.obj\n") != std::string::npos);
	CHECK(text.find("POLYGON_DEF lib/buildings/facades/hangar.fac\n") != std::string::npos);
	CHECK(text.find("NETWORK_DEF lib/g10/roads.net\n") != std::string::npos);
	CHECK(text.find("FILTER 0\nOBJECT 0 -73.152300000 44.475900000 90.000\n") != std::string::npos);
	CHECK(text.find("OBJECT_AGL 0 -73.152300000 44.476900000 4.50000 90.000\n") != std::string::npos);
	CHECK(text.find("BEGIN_POLYGON 0 12 2\nBEGIN_WINDING\nPOLYGON_POINT -73.152300000 44.477900000\n") != std::string::npos);
	CHECK(text.find("BEGIN_SEGMENT 0 3 1 -73.152300000 44.472900000 0.000000000\nSHAPE_POINT -73.152100000 44.473400000 1.000000000\n") != std::string::npos);
	CHECK(text.find("FILTER -1") == std::string::npos);

	// The pack: apt.dat, one binary tile, README, COPYING
	const std::vector<ZipEntry> master = Unzip(package.submission.masterZip);
	REQUIRE(master.size() == 3);
	CHECK(master[1].name == "KBTV.txt");
	CHECK(Text(master[1]) == text);
	const std::vector<ZipEntry> pack = Unzip(Find(master, "KBTV_Scenery_Pack.zip")->data);
	REQUIRE(pack.size() == 4);
	const ZipEntry *tile = Find(pack, "KBTV_Scenery_Pack/Earth nav data/+40-080/+44-074.dsf");
	REQUIRE(tile);
	DsfFile dsf;
	std::string error;
	REQUIRE(ReadDsf(tile->data.data(), tile->data.size(), dsf, &error));
	REQUIRE(dsf.properties.size() >= 9);
	CHECK(dsf.properties[0].first == "sim/west");
	CHECK(dsf.properties[3].first == "sim/south");
	CHECK(dsf.GetProperty("sim/west") == "-74");
	CHECK(dsf.GetProperty("sim/north") == "45");
	CHECK(dsf.GetProperty("sim/filter/aptid") == "KBTV");
	CHECK(dsf.GetProperty("sim/creation_agent") == "Scenery Editor X");
	CHECK(dsf.GetProperty("sim/exclude_obj") == "-73.155000/44.471000/-73.152000/44.473000");
	CHECK(dsf.properties.back().first == "sim/exclude_obj");		// After the aptid: the airport's own exclusion
	REQUIRE(dsf.objects.size() == 2);
	CHECK(dsf.objects[0].filter == 0);
	CHECK(dsf.objects[1].mode == DsfObjectPlacement::Mode::Agl);
	CHECK(dsf.objects[1].elevation == Catch::Approx(4.5).margin(0.01));
	REQUIRE(dsf.polygons.size() == 1);
	CHECK(dsf.polygons[0].parameter == 12);
	REQUIRE(dsf.roads.size() == 1);
	CHECK(dsf.roads[0].points.size() == 4);
	CHECK(dsf.roads[0].points[1].control);
}

TEST_CASE("Gateway package: an airport across a tile edge", "[xplane][gateway][package]")
{
	GatewayPackageInput input;
	input.airport = MakeAirport();
	input.overlay.objectDefinitions = { "lib/airport/Common_Elements/Hangars/Hangar_1.obj" };
	input.overlay.polygonDefinitions = { "lib/airport/ground/terrain/grass.pol" };
	DsfObjectPlacement object;
	object.latitude = LAT;
	object.longitude = -72.9995;		// The next tile east
	input.overlay.objects.push_back(object);
	DsfPolygonPlacement polygon;
	polygon.windings.push_back({ Point(LAT, -73.0005), Point(LAT, -72.9995), Point(LAT + 0.0005, -72.9995) });
	input.overlay.polygons.push_back(polygon);

	GatewayPackage package;
	REQUIRE(BuildGatewayPackage(input, package));
	size_t tiles = 0;
	for (const ZipFile &file : package.packFiles)
		tiles += file.name.ends_with(".dsf") ? 1 : 0;
	CHECK(tiles == 2);
	CHECK_FALSE(package.warnings.empty());		// The polygon reaches into the other tile
}

TEST_CASE("Gateway package: deflated zip archives", "[xplane][gateway][package]")
{
	std::string repetitive;
	for (int i = 0; i < 2000; ++i)
		repetitive += "1302 city Burlington\n";
	std::string noise(4096, '\0');
	std::mt19937 random(7);
	for (char &c : noise)
		c = static_cast<char>(random() & 0xFF);

	const std::vector<ZipFile> files = { { "text.txt", repetitive }, { "empty", "" }, { "dir/noise.bin", noise } };
	const std::vector<uint8_t> zip = WriteZipArchive(files, ZipMethod::Deflated);
	CHECK(zip.size() < repetitive.size() / 4);
	CHECK(Methods(zip) == std::vector<uint16_t>{ 8, 0, 0 });		// Noise and empty files stay stored
	const std::vector<ZipEntry> entries = Unzip(zip);
	REQUIRE(entries.size() == 3);
	CHECK(Text(entries[0]) == repetitive);
	CHECK(entries[1].data.empty());
	CHECK(Text(entries[2]) == noise);

	// The same files give the same bytes; stored archives stay readable by the old reader
	CHECK(WriteZipArchive(files, ZipMethod::Deflated) == zip);
	CHECK(Methods(WriteZipArchive(files, ZipMethod::Stored)) == std::vector<uint16_t>{ 0, 0, 0 });
	CHECK(WriteStoredZip(files) == WriteZipArchive(files, ZipMethod::Stored));

	const std::vector<ZipEntry> master = Unzip(MakeGatewayMasterZip("KBTV", repetitive, "", { { "Earth nav data/apt.dat", repetitive } }, ZipMethod::Deflated));
	REQUIRE(master.size() == 2);
	const std::vector<ZipEntry> pack = Unzip(master[1].data);
	CHECK(pack[0].name == "KBTV_Scenery_Pack/Earth nav data/apt.dat");
	CHECK(Text(pack[0]) == repetitive);
}

TEST_CASE("Gateway package: DSF2Text", "[xplane][gateway][package][dsf]")
{
	DsfFile dsf;
	dsf.properties = { { "sim/west", "-74" }, { "sim/east", "-73" }, { "sim/north", "45" }, { "sim/south", "44" }, { "sim/filter/aptid", "KBTV" } };
	dsf.objectDefinitions = { "lib/a.obj", "lib/b.agp" };
	dsf.polygonDefinitions = { "lib/c.fac" };
	DsfObjectPlacement object;
	object.definition = 1;
	object.longitude = -73.5;
	object.latitude = 44.5;
	object.heading = -90.0;			// Normalized like the binary writer
	object.filter = 0;
	dsf.objects.push_back(object);
	object.definition = 0;
	object.mode = DsfObjectPlacement::Mode::Msl;
	object.elevation = 123.25;
	object.heading = 45.0;
	object.filter = -1;
	dsf.objects.push_back(object);
	DsfPolygonPlacement facade;
	facade.parameter = 10;
	facade.planes = 3;
	facade.filter = -1;
	facade.windings.push_back({ Point(44.5, -73.5, 3), Point(44.5, -73.499, 3) });
	facade.windings[0][1].v[2] = 2.0;
	facade.windings.push_back({ Point(44.6, -73.5, 3) });
	dsf.polygons.push_back(facade);

	const std::string text = WriteDsfText(dsf, "Test 1.0");
	CHECK(text == "I\n800 written by Test 1.0\nDSF2TEXT\n\n"
		"PROPERTY sim/west -74\nPROPERTY sim/east -73\nPROPERTY sim/north 45\nPROPERTY sim/south 44\nPROPERTY sim/filter/aptid KBTV\n"
		"OBJECT_DEF lib/a.obj\nOBJECT_DEF lib/b.agp\nPOLYGON_DEF lib/c.fac\n"
		"FILTER 0\nOBJECT 1 -73.500000000 44.500000000 270.000\n"
		"FILTER -1\nOBJECT_MSL 0 -73.500000000 44.500000000 123.25000 45.000\n"
		"BEGIN_POLYGON 0 10 3\nBEGIN_WINDING\nPOLYGON_POINT -73.500000000 44.500000000 0.000000000\nPOLYGON_POINT -73.499000000 44.500000000 2.000000000\nEND_WINDING\n"
		"BEGIN_WINDING\nPOLYGON_POINT -73.500000000 44.600000000 0.000000000\nEND_WINDING\nEND_POLYGON\n");
}

TEST_CASE("Gateway package: X-Plane's build from Log.txt", "[xplane][gateway][package]")
{
	CHECK(ReadXPlaneBuild("Log.txt for X-Plane 12.4.4-b5-3a0f7e10 (build 124417 Intel 64-bit, Vulkan 8993c0)\nCompiled on Oct 20 2026\n") == "12.4.4-b5-3a0f7e10");
	CHECK(ReadXPlaneBuild("Log.txt for X-Plane 11.55r2 (build 115520 64-bit, Vulkan)\r\n") == "11.55r2");
	CHECK(ReadXPlaneBuild("") == "");
	CHECK(ReadXPlaneBuild("Log.txt for X-Plane") == "");
	CHECK(ReadXPlaneBuild(std::string(120, ' ') + "X-Plane 12.1.0") == "");		// WorldEditor only looks near the start

	const std::filesystem::path folder = std::filesystem::temp_directory_path() / ("sedx_xplane_log_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
	std::filesystem::create_directories(folder);
	CHECK(LoadXPlaneBuild(folder) == "");
	std::ofstream(folder / "Log.txt", std::ios::binary) << "Log.txt for X-Plane 12.1.3-r2-2f236 (build 121302)\n";
	CHECK(LoadXPlaneBuild(folder) == "12.1.3-r2-2f236");
	std::error_code ec;
	std::filesystem::remove_all(folder, ec);
}

TEST_CASE("Gateway package: a scenery pack's DSF content for one airport", "[xplane][gateway][package]")
{
	const AptAirport airport = MakeAirport();

	// Two airports in one tile: placements and exclusions go by their filter
	DsfFile tile;
	tile.properties = { { "sim/west", "-74" }, { "sim/east", "-73" }, { "sim/north", "45" }, { "sim/south", "44" }, { "sim/exclude_fac", "global" },
		{ "sim/filter/aptid", "KPBG" }, { "sim/exclude_obj", "theirs" }, { "sim/filter/aptid", "KBTV" }, { "sim/exclude_obj", "mine" } };
	tile.objectDefinitions = { "lib/a.obj", "lib/b.obj" };
	DsfObjectPlacement object;
	object.latitude = LAT;
	object.longitude = LON;
	object.filter = 1;
	tile.objects.push_back(object);
	object.definition = 1;
	object.filter = 0;
	tile.objects.push_back(object);
	object.filter = -1;
	tile.objects.push_back(object);
	size_t leftOut = 0;
	DsfFile mine = SelectAirportPlacements(tile, airport, &leftOut);
	REQUIRE(mine.objects.size() == 1);
	CHECK(mine.objects[0].definition == 0);
	CHECK(leftOut == 2);
	REQUIRE(mine.properties.size() == 1);
	CHECK(mine.properties[0] == std::pair<std::string, std::string>{ "sim/exclude_obj", "mine" });

	// A tile without filters: what's near the airport
	tile.properties = { { "sim/exclude_obj", "all" } };
	tile.objects[0].filter = -1;
	tile.objects[1].filter = -1;
	tile.objects[1].latitude = LAT + 0.5;
	tile.objects.pop_back();
	leftOut = 0;
	mine = SelectAirportPlacements(tile, airport, &leftOut);
	CHECK(mine.objects.size() == 1);
	CHECK(leftOut == 1);
	CHECK(mine.properties.size() == 1);

	// Merging renumbers definitions and keeps road networks apart
	DsfFile merged;
	merged.objectDefinitions = { "lib/b.obj" };
	DsfRoadSegment road;
	road.startJunction = 1;
	road.endJunction = 2;
	road.points = { { LON, LAT, false }, { LON + 0.001, LAT, false } };
	merged.networkDefinitions = { "lib/g10/roads.net" };
	merged.roads.push_back(road);
	DsfFile other;
	other.objectDefinitions = { "lib/a.obj", "lib/b.obj" };
	other.networkDefinitions = { "lib/g10/roads.net" };
	other.objects = tile.objects;
	other.objects[0].definition = 1;
	other.roads.push_back(road);
	other.properties = { { "sim/exclude_obj", "x" } };
	AppendDsf(merged, other);
	CHECK(merged.objectDefinitions == std::vector<std::string>{ "lib/b.obj", "lib/a.obj" });
	CHECK(merged.objects[0].definition == 0);
	REQUIRE(merged.roads.size() == 2);
	CHECK(merged.roads[1].startJunction == 3);
	CHECK(merged.roads[1].endJunction == 4);
	CHECK(merged.networkDefinitions.size() == 1);
	CHECK(merged.properties.size() == 1);
	AppendDsf(merged, other);
	CHECK(merged.properties.size() == 1);		// The same exclusion once
}

TEST_CASE("Gateway package: a pack folder's tiles", "[xplane][gateway][package]")
{
	const std::filesystem::path folder = std::filesystem::temp_directory_path() / ("sedx_gateway_pack_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
	std::filesystem::create_directories(folder / "Earth nav data" / "+40-080");

	GatewayPackageInput input;
	input.airport = MakeAirport();
	input.overlay = MakeOverlay();
	GatewayPackage package;
	REQUIRE(BuildGatewayPackage(input, package));
	for (const ZipFile &file : package.packFiles)
	{
		if (file.name.ends_with(".dsf"))
			std::ofstream(folder / file.name, std::ios::binary) << file.data;
	}

	std::vector<std::string> warnings;
	const DsfFile overlay = LoadPackOverlay(folder, input.airport, &warnings);
	CHECK(warnings.empty());
	CHECK(overlay.objects.size() == 2);
	CHECK(overlay.polygons.size() == 1);
	CHECK(overlay.roads.size() == 1);
	CHECK(overlay.GetProperty("sim/exclude_obj") == "-73.155000/44.471000/-73.152000/44.473000");

	// Packing what came back gives the same summary
	const GatewayContentSummary before = SummarizeGatewayContent(input.airport, input.overlay);
	const GatewayContentSummary after = SummarizeGatewayContent(input.airport, overlay);
	CHECK(after.GetFeatures() == before.GetFeatures());
	CHECK(after.objects == before.objects);
	CHECK(after.exclusions == before.exclusions);

	std::error_code ec;
	std::filesystem::remove_all(folder, ec);
}

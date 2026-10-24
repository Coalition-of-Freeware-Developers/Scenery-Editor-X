/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * GatewayProjectTests.cpp
 * -------------------------------------------------------
 * Projects and the Scenery Gateway (project_gateway.*): the scenery id each airport descends from on each server
 * (kept in .edX / .sex, test servers apart from the public Gateway), what an airport of a project uploads, and
 * bringing a Gateway submission (as xp_gateway_package packs it) into a project.
 * -------------------------------------------------------
 */
#include <catch2/catch_all.hpp>
#include <SceneryEditorX/project/project_dsf.h>
#include <SceneryEditorX/project/project_edx.h>
#include <SceneryEditorX/project/project_gateway.h>
#include <SceneryEditorX/project/project_sex.h>
#include <SceneryEditorX/xplane/xp_gateway_package.h>
#include <string>
#include <vector>

using namespace SceneryEditorX;
using namespace SceneryEditorX::ProjectFile;

namespace
{
	constexpr double LAT = 44.4719;
	constexpr double LON = -73.1533;
	constexpr const char *MOCK = "127.0.0.1:8780";

	GeoNode Geo(const double latitude, const double longitude)
	{
		GeoNode node;
		node.latitude = latitude;
		node.longitude = longitude;
		return node;
	}

	XPlane::AptAirport MakeAirport(const std::string &id = "KBTV")
	{
		XPlane::AptAirport airport;
		airport.id = id;
		airport.name = "Burlington Intl";
		airport.elevationFt = 335;
		airport.SetMetadata("country", "USA United States");
		airport.SetMetadata("datum_lat", "44.47190000");
		airport.SetMetadata("datum_lon", "-73.15330000");
		airport.SetMetadata("icao_code", id);
		XPlane::AptRunway runway;
		runway.ends[0].designator = "18";
		runway.ends[0].latitude = LAT;
		runway.ends[0].longitude = LON;
		runway.ends[1].designator = "36";
		runway.ends[1].latitude = LAT + 0.009;
		runway.ends[1].longitude = LON;
		airport.runways.push_back(runway);
		return airport;
	}

	SceneryItem Item(const ItemKind kind, const std::string &name, const std::string &resource)
	{
		SceneryItem item;
		item.kind = kind;
		item.id = MakeItemId();
		item.name = name;
		item.resource = resource;
		return item;
	}

	/* @brief KBTV with a hangar, a facade and a hidden polygon in its hierarchy, and a stray object outside it. */
	SceneryProject MakeProject()
	{
		SceneryProject project = MakeNewProject("KBTV - Burlington Intl");
		ProjectAirport airport;
		airport.airport = MakeAirport();
		project.airports.push_back(airport);

		SceneryItem placed;
		placed.kind = ItemKind::Airport;
		placed.id = MakeItemId();
		placed.name = "Burlington Intl";
		placed.airportId = "KBTV";

		SceneryItem hangar = Item(ItemKind::Object, "Hangar", "lib/airport/Common_Elements/Hangars/Hangar_1.obj");
		hangar.latitude = LAT + 0.004;
		hangar.longitude = LON + 0.001;
		hangar.heading = 90.0f;
		placed.children.push_back(hangar);

		SceneryItem facade = Item(ItemKind::Facade, "Terminal", "lib/airport/Common_Elements/Terminal_Buildings/terminal.fac");
		facade.height = 12.0f;
		facade.rings.push_back({ Geo(LAT + 0.006, LON + 0.001), Geo(LAT + 0.006, LON + 0.0015), Geo(LAT + 0.0065, LON + 0.0015), Geo(LAT + 0.0065, LON + 0.001) });
		SceneryItem group;
		group.kind = ItemKind::Group;
		group.id = MakeItemId();
		group.name = "Buildings";
		group.children.push_back(facade);
		placed.children.push_back(group);

		SceneryItem hidden = Item(ItemKind::Polygon, "Old apron", "lib/airport/ground/terrain/grass.pol");
		hidden.hidden = true;
		hidden.rings.push_back({ Geo(LAT, LON), Geo(LAT, LON + 0.001), Geo(LAT + 0.001, LON + 0.001) });
		placed.children.push_back(hidden);
		project.items.push_back(placed);

		SceneryItem stray = Item(ItemKind::Object, "Stray tower", "lib/airport/Common_Elements/Towers/tower.obj");
		stray.latitude = LAT + 0.2;
		stray.longitude = LON;
		project.items.push_back(stray);
		return project;
	}
}

TEST_CASE("Gateway project: scenery ids per server", "[project][gateway]")
{
	ProjectAirport airport;
	CHECK(GetGatewaySceneryId(airport, true, {}) == 0);
	CHECK(GetGatewaySceneryId(airport, false, MOCK) == 0);

	// The public Gateway's id is WorldEditor's scenery_id; test servers keep theirs apart
	SetGatewaySceneryId(airport, true, {}, 97429);
	SetGatewaySceneryId(airport, false, MOCK, 900002);
	SetGatewaySceneryId(airport, false, "localhost:9000", 900010);
	CHECK(airport.sceneryId == 97429);
	CHECK(GetGatewaySceneryId(airport, true, MOCK) == 97429);
	CHECK(GetGatewaySceneryId(airport, false, MOCK) == 900002);
	CHECK(GetGatewaySceneryId(airport, false, "localhost:9000") == 900010);
	CHECK(GetGatewaySceneryId(airport, false, "localhost:9001") == 0);
	REQUIRE(airport.testSceneryIds.size() == 2);
	CHECK(airport.testSceneryIds[0].server == MOCK);		// Sorted by server
	CHECK(airport.testSceneryIds[1].server == "localhost:9000");

	SetGatewaySceneryId(airport, false, MOCK, 900003);
	CHECK(GetGatewaySceneryId(airport, false, MOCK) == 900003);
	CHECK(airport.testSceneryIds.size() == 2);
	SetGatewaySceneryId(airport, false, MOCK, 0);
	CHECK(airport.testSceneryIds.size() == 1);
	SetGatewaySceneryId(airport, true, {}, 0);
	CHECK(airport.sceneryId == -1);
}

TEST_CASE("Gateway project: scenery ids in .edX and .sex", "[project][gateway]")
{
	SceneryProject project = MakeProject();
	const std::string untouched = WriteEdx(project);
	CHECK(untouched.find("test-scenery-ids") == std::string::npos);		// Projects without test uploads write the same bytes as before
	CHECK(untouched.find("scenery-id") == std::string::npos);

	SetGatewaySceneryId(project.airports[0], true, {}, 97429);
	SetGatewaySceneryId(project.airports[0], false, MOCK, 900002);
	const std::string text = WriteEdx(project);
	CHECK(text.find("\"scenery-id\": 97429") != std::string::npos);
	CHECK(text.find("\"test-scenery-ids\"") != std::string::npos);
	CHECK(text.find("\"server\": \"127.0.0.1:8780\"") != std::string::npos);

	SceneryProject edx;
	std::string error;
	REQUIRE(ParseEdx(text, edx, nullptr, &error));
	REQUIRE(edx.airports.size() == 1);
	CHECK(edx.airports[0].sceneryId == 97429);
	CHECK(edx.airports[0].testSceneryIds == project.airports[0].testSceneryIds);
	CHECK(WriteEdx(edx) == text);

	const std::string bytes = WriteSex(project);
	SceneryProject sex;
	REQUIRE(ParseSex(bytes.data(), bytes.size(), sex, nullptr, &error));
	CHECK(sex.airports[0].sceneryId == 97429);
	CHECK(sex.airports[0].testSceneryIds == project.airports[0].testSceneryIds);
}

TEST_CASE("Gateway project: what an airport uploads", "[project][gateway]")
{
	const SceneryProject project = MakeProject();
	GatewayContent content;
	std::string error;
	REQUIRE(CollectGatewayContent(project, "KBTV", content, &error));
	CHECK(content.airport.id == "KBTV");
	CHECK(content.overlay.objects.size() == 1);			// The hangar; the stray tower is outside the hierarchy
	CHECK(content.overlay.polygons.size() == 1);		// The facade (in a group); the hidden polygon stays home
	CHECK(content.hidden == 1);
	REQUIRE(content.outside.size() == 1);
	CHECK(content.outside[0] == "Stray tower");
	CHECK(content.overlay.objectDefinitions == std::vector<std::string>{ "lib/airport/Common_Elements/Hangars/Hangar_1.obj" });

	CHECK_FALSE(CollectGatewayContent(project, "KPBG", content, &error));
	CHECK(error.find("KPBG") != std::string::npos);

	SceneryProject hidden = project;
	hidden.items[0].hidden = true;
	CHECK_FALSE(CollectGatewayContent(hidden, "KBTV", content, &error));
	CHECK(error.find("hidden") != std::string::npos);

	// An airport no item places uploads its rows alone
	SceneryProject bare = project;
	bare.items.clear();
	REQUIRE(CollectGatewayContent(bare, "KBTV", content));
	CHECK(content.overlay.objects.empty());
	CHECK(content.outside.empty());
}

TEST_CASE("Gateway project: importing a submission", "[project][gateway]")
{
	// What an upload of the project's airport carries, as the Gateway would hand it back
	const SceneryProject source = MakeProject();
	GatewayContent content;
	REQUIRE(CollectGatewayContent(source, "KBTV", content));
	XPlane::GatewayPackageInput input;
	input.airport = content.airport;
	input.overlay = content.overlay;
	input.uploader = "Test Artist";
	XPlane::GatewayPackage package;
	REQUIRE(XPlane::BuildGatewayPackage(input, package));

	SECTION("into an empty project")
	{
		SceneryProject project = MakeNewProject("Imported");
		GatewayImportReport report;
		std::string error;
		REQUIRE(ImportGatewaySubmission(project, package.submission.masterZip, 900002, false, MOCK, report, &error));
		CHECK(report.airportId == "KBTV");
		CHECK_FALSE(report.replaced);
		CHECK(report.items == 2);
		INFO(report.warnings.size());
		REQUIRE(project.airports.size() == 1);
		CHECK(project.airports[0].airport.name == "Burlington Intl");
		CHECK(project.airports[0].airport.GetMetadata("gui_label") == "3D");
		CHECK(project.airports[0].sceneryId == -1);			// A test server's id never becomes the public parent
		CHECK(GetGatewaySceneryId(project.airports[0], false, MOCK) == 900002);
		CHECK(project.datum.valid);
		CHECK(project.datum.latitude == Catch::Approx(LAT));
		REQUIRE(project.items.size() == 1);
		CHECK(project.items[0].kind == ItemKind::Airport);
		CHECK(project.items[0].airportId == "KBTV");
		CHECK(project.items[0].children.size() == 2);

		// Uploading it again packs the same content
		GatewayContent again;
		REQUIRE(CollectGatewayContent(project, "KBTV", again));
		CHECK(again.overlay.objects.size() == content.overlay.objects.size());
		CHECK(again.overlay.polygons.size() == content.overlay.polygons.size());
		CHECK(again.overlay.objectDefinitions == content.overlay.objectDefinitions);
		CHECK(again.overlay.objects[0].latitude == Catch::Approx(content.overlay.objects[0].latitude).margin(1e-6));
		CHECK(again.overlay.polygons[0].parameter == content.overlay.polygons[0].parameter);
		CHECK(NormalizeProject(project).empty());
		CHECK(ValidateProject(project).empty());
	}

	SECTION("over the same airport")
	{
		SceneryProject project = MakeProject();
		const size_t before = project.CountItems();
		GatewayImportReport report;
		REQUIRE(ImportGatewaySubmission(project, package.submission.masterZip, 97429, true, {}, report));
		CHECK(report.replaced);
		CHECK(project.airports.size() == 1);
		CHECK(project.airports[0].sceneryId == 97429);
		CHECK(project.airports[0].testSceneryIds.empty());
		CHECK(project.items.size() == 2);					// The airport's item (children replaced) and the stray tower
		CHECK(project.items[0].children.size() == 2);
		CHECK(project.CountItems() == before - 2);			// The group and the hidden polygon weren't in the upload
	}

	SECTION("broken archives")
	{
		SceneryProject project = MakeNewProject("Imported");
		GatewayImportReport report;
		std::string error;
		CHECK_FALSE(ImportGatewaySubmission(project, { 'P', 'K', 3, 4 }, 1, false, MOCK, report, &error));
		CHECK_FALSE(error.empty());
		const std::vector<uint8_t> noDat = XPlane::WriteZipArchive({ { "README.txt", "nothing" } });
		CHECK_FALSE(ImportGatewaySubmission(project, noDat, 1, false, MOCK, report, &error));
		CHECK(error.find(".dat") != std::string::npos);
		CHECK(project.airports.empty());
		CHECK(project.items.empty());
	}
}

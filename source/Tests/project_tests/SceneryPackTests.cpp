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
 * SceneryPackTests.cpp
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#include <catch2/catch_all.hpp>
#include <SceneryEditorX/filesystem/vfs.h>
#include <SceneryEditorX/project/project_io.h>
#include <SceneryEditorX/project/scenery_pack.h>
#include <SceneryEditorX/xplane/xp_dsf.h>
#include <fstream>

using namespace SceneryEditorX;
using namespace SceneryEditorX::ProjectFile;
using Catch::Approx;
namespace fs = std::filesystem;

// -------------------------------------------------------

namespace
{
	struct Scratch
	{
		fs::path path;

		Scratch()
		{
			path = fs::temp_directory_path() / ("sedx_pack_tests_" + MakeItemId());
			fs::create_directories(path);
		}

		~Scratch()
		{
			std::error_code ec;
			fs::remove_all(path, ec);
		}
	};

	void WriteText(const fs::path &path, const std::string &text)
	{
		fs::create_directories(path.parent_path());
		std::ofstream(path, std::ios::binary) << text;
	}

	/* @brief KTST, a made-up field near Seattle, as the form would fill it in. */
	NewAirportProject TestSpec(const fs::path &location)
	{
		NewAirportProject spec;
		spec.icao = "KTST";
		spec.name = "Test Field";
		spec.country = "USA United States";
		spec.city = "Tacoma";
		spec.state = "Pierce County";
		spec.latitude = 47.44898333;
		spec.longitude = -122.30916667;
		spec.elevationFeet = 433.0;
		spec.location = location;
		spec.author = "Unit Tests";
		return spec;
	}

	bool HasProblem(const std::vector<FieldProblem> &problems, const std::string &field)
	{
		return std::ranges::any_of(problems, [&field](const FieldProblem &problem) { return problem.field == field; });
	}

	GeoNode Node(const double latitude, const double longitude)
	{
		GeoNode node;
		node.latitude = latitude;
		node.longitude = longitude;
		return node;
	}

	SceneryItem MakeItem(const ItemKind kind, const std::string &name, const std::string &resource)
	{
		SceneryItem item;
		item.kind = kind;
		item.id = MakeItemId();
		item.name = name;
		item.resource = resource;
		return item;
	}

	/* @brief The airport's hierarchy entry in a project MakeAirportProject made. */
	SceneryItem &AirportItem(SceneryProject &project)
	{
		for (SceneryItem &item : project.items)
		{
			if (item.kind == ItemKind::Airport)
				return item;
		}
		FAIL("The project has no airport item");
		return project.items.front();
	}
}

// -------------------------------------------------------

TEST_CASE("Coordinates are read as decimal degrees with up to 8 decimals", "[pack][coordinates]")
{
	double value = 0.0;
	std::string error;
	REQUIRE(ParseCoordinate("-50.50000000", true, value, &error));
	CHECK(value == -50.5);
	REQUIRE(ParseCoordinate("20.12345678", true, value, &error));
	CHECK(value == Approx(20.12345678).epsilon(1e-15));
	REQUIRE(ParseCoordinate("  47.44898333 ", true, value, &error));
	CHECK(value == Approx(47.44898333));
	REQUIRE(ParseCoordinate("+12", true, value, &error));
	CHECK(value == 12.0);
	REQUIRE(ParseCoordinate("50.5 S", true, value, &error));
	CHECK(value == -50.5);
	REQUIRE(ParseCoordinate("122.30916667W", false, value, &error));
	CHECK(value == Approx(-122.30916667));
	REQUIRE(ParseCoordinate("8.5 e", false, value, &error));
	CHECK(value == 8.5);
	REQUIRE(ParseCoordinate("-180", false, value, &error));
	CHECK(value == -180.0);
	REQUIRE(ParseCoordinate("90.00000000", true, value, &error));

	CHECK_FALSE(ParseCoordinate("20.123456789", true, value, &error));
	CHECK(error.find("8 decimal") != std::string::npos);
	CHECK_FALSE(ParseCoordinate("90.00000001", true, value, &error));
	CHECK_FALSE(ParseCoordinate("180.5", false, value, &error));
	CHECK_FALSE(ParseCoordinate("", true, value, &error));
	CHECK_FALSE(ParseCoordinate("abc", true, value, &error));
	CHECK_FALSE(ParseCoordinate("50.", true, value, &error));
	CHECK_FALSE(ParseCoordinate(".5", true, value, &error));
	CHECK_FALSE(ParseCoordinate("50 30", true, value, &error));		// Degrees minutes: not decimal
	CHECK_FALSE(ParseCoordinate("-50.5 S", true, value, &error));		// Sign and hemisphere
	CHECK_FALSE(ParseCoordinate("45.5 E", true, value, &error));		// A longitude hemisphere on a latitude
	CHECK_FALSE(ParseCoordinate("1e2", true, value, &error));

	CHECK(FormatCoordinate(-50.5) == "-50.50000000");
	CHECK(FormatCoordinate(20.12345678) == "20.12345678");
	CHECK(FormatCoordinate(-0.0) == "0.00000000");
	CHECK(FormatCoordinate(-0.000000001) == "0.00000000");
	REQUIRE(ParseCoordinate(FormatCoordinate(-122.30916667), false, value, &error));
	CHECK(value == Approx(-122.30916667).margin(1e-9));
}

TEST_CASE("The new project form is checked field by field", "[pack][create]")
{
	Scratch scratch;
	CHECK(ValidateNewAirportProject(TestSpec(scratch.path)).empty());

	const std::vector<FieldProblem> empty = ValidateNewAirportProject(NewAirportProject{});
	CHECK(HasProblem(empty, "icao"));
	CHECK(HasProblem(empty, "name"));
	CHECK(HasProblem(empty, "country"));
	CHECK(HasProblem(empty, "latitude"));		// 0, 0
	CHECK(HasProblem(empty, "location"));
	CHECK_FALSE(HasProblem(empty, "city"));		// Optional
	CHECK_FALSE(HasProblem(empty, "state"));

	NewAirportProject spec = TestSpec(scratch.path);
	spec.icao = "ktst";
	CHECK(HasProblem(ValidateNewAirportProject(spec), "icao"));
	spec.icao = "AB";
	CHECK(HasProblem(ValidateNewAirportProject(spec), "icao"));
	spec.icao = "XKTST12";
	CHECK(ValidateNewAirportProject(spec).empty());

	spec = TestSpec(scratch.path);
	spec.latitude = 91.0;
	spec.longitude = -181.0;
	spec.elevationFeet = 40000.0;
	const std::vector<FieldProblem> range = ValidateNewAirportProject(spec);
	CHECK(HasProblem(range, "latitude"));
	CHECK(HasProblem(range, "longitude"));
	CHECK(HasProblem(range, "elevation"));

	spec = TestSpec(scratch.path);
	spec.name = "Bad\nName";
	spec.city = "Tab\tCity";
	const std::vector<FieldProblem> text = ValidateNewAirportProject(spec);
	CHECK(HasProblem(text, "name"));
	CHECK(HasProblem(text, "city"));

	spec = TestSpec(scratch.path);
	spec.projectName = "CON";
	CHECK(HasProblem(ValidateNewAirportProject(spec), "projectName"));
	spec.location = scratch.path / "does not exist";
	CHECK(HasProblem(ValidateNewAirportProject(spec), "location"));

	// A folder already holding files is never taken over
	spec = TestSpec(scratch.path);
	WriteText(GetNewProjectFolder(spec) / "readme.txt", "someone's pack");
	CHECK(HasProblem(ValidateNewAirportProject(spec), "projectName"));

	CHECK(MakeDefaultProjectName("KTST", "Test Field") == "KTST - Test Field");
	CHECK(MakeDefaultProjectName("KSEA", "Seattle/Tacoma Intl") == "KSEA - Seattle_Tacoma Intl");
	CHECK(GetNewProjectFolder(TestSpec(scratch.path)) == scratch.path / "KTST - Test Field");
}

TEST_CASE("A new airport project carries the form's data", "[pack][create]")
{
	Scratch scratch;
	SceneryProject project = MakeAirportProject(TestSpec(scratch.path));
	CHECK(project.info.name == "KTST - Test Field");
	CHECK(project.info.packageName == "KTST - Test Field");
	CHECK(project.info.author == "Unit Tests");
	CHECK(project.datum.valid);
	CHECK(project.datum.latitude == 47.44898333);
	CHECK(project.datum.longitude == -122.30916667);
	CHECK(project.datum.elevation == Approx(433.0 * 0.3048));

	REQUIRE(project.airports.size() == 1);
	const XPlane::AptAirport &airport = project.airports[0].airport;
	CHECK(airport.id == "KTST");
	CHECK(airport.name == "Test Field");
	CHECK(airport.elevationFt == 433);
	CHECK(airport.type == XPlane::AirportType::Land);
	CHECK(airport.GetMetadata("icao_code") == "KTST");
	CHECK(airport.GetMetadata("country") == "USA United States");
	CHECK(airport.GetMetadata("city") == "Tacoma");
	CHECK(airport.GetMetadata("state") == "Pierce County");
	CHECK(airport.GetMetadata("datum_lat") == "47.44898333");
	CHECK(airport.GetMetadata("datum_lon") == "-122.30916667");
	CHECK(std::ranges::is_sorted(airport.metadata, {}, &XPlane::AptMetadata::key));
	CHECK(airport.runways.size() == 1);
	CHECK(XPlane::ValidateAptAirport(airport).empty());
	CHECK(AirportItem(project).airportId == "KTST");
	CHECK(ValidateProject(project).empty());

	// Optional fields stay out of the metadata when empty; heliports start with a helipad
	NewAirportProject spec = TestSpec(scratch.path);
	spec.city.clear();
	spec.state.clear();
	spec.type = XPlane::AirportType::Heliport;
	const SceneryProject heliport = MakeAirportProject(spec);
	CHECK(heliport.airports[0].airport.GetMetadata("city").empty());
	CHECK(std::ranges::none_of(heliport.airports[0].airport.metadata, [](const XPlane::AptMetadata &m) { return m.key == "state"; }));
	CHECK(heliport.airports[0].airport.helipads.size() == 1);
	CHECK(heliport.airports[0].airport.runways.empty());

	spec.starterLayout = false;
	CHECK(MakeAirportProject(spec).airports[0].airport.helipads.empty());
}

TEST_CASE("CreateAirportProject lays the pack out like Custom Scenery's", "[pack][create]")
{
	Scratch scratch;
	const NewAirportProject spec = TestSpec(scratch.path);
	fs::path projectFile;
	std::string error;
	REQUIRE(CreateAirportProject(spec, projectFile, &error));

	const fs::path pack = scratch.path / "KTST - Test Field";
	CHECK(projectFile == pack / "KTST - Test Field.edX");
	CHECK(fs::is_regular_file(projectFile));
	CHECK(fs::is_directory(pack / "Earth nav data"));
	CHECK(fs::is_directory(pack / "objects"));
	CHECK(fs::is_directory(pack / "textures"));
	CHECK(fs::exists(pack / ".gitattributes"));

	SceneryProject loaded;
	REQUIRE(LoadProjectFile(projectFile, loaded, nullptr, nullptr, &error));
	REQUIRE(loaded.airports.size() == 1);
	CHECK(loaded.airports[0].airport.GetMetadata("datum_lat") == "47.44898333");
	CHECK(loaded.datum.elevation == Approx(433.0 * 0.3048));

	// The second time the folder is taken
	fs::path again;
	CHECK_FALSE(CreateAirportProject(spec, again, &error));
	CHECK(error.find("already exists") != std::string::npos);
}

TEST_CASE("ExportSceneryPack writes apt.dat, DSF tiles and the pack's art", "[pack][export]")
{
	Scratch scratch;
	fs::path projectFile;
	std::string error;
	REQUIRE(CreateAirportProject(TestSpec(scratch.path), projectFile, &error));
	const fs::path projectFolder = projectFile.parent_path();

	SceneryProject project;
	REQUIRE(LoadProjectFile(projectFile, project, nullptr, nullptr, &error));

	// The pack's own hangar (with its texture and DDS twin), a library object, an apron, and a tree outside the airport
	WriteText(projectFolder / "objects" / "hangar.obj", "I\n800\nOBJ\n\nTEXTURE hangar.png\nTEXTURE_LIT hangar_LIT.png\nPOINT_COUNTS 0 0 0 0\n");
	WriteText(projectFolder / "objects" / "hangar.png", "png");
	WriteText(projectFolder / "objects" / "hangar.dds", "dds");
	WriteText(projectFolder / "objects" / "unused.obj", "I\n800\nOBJ\n");

	SceneryItem &airport = AirportItem(project);
	SceneryItem hangar = MakeItem(ItemKind::Object, "Hangar", "Objects/Hangar.obj");		// Case as a Windows author typed it
	hangar.latitude = 47.4495;
	hangar.longitude = -122.3085;
	hangar.heading = 90.0f;
	airport.children.push_back(hangar);
	SceneryItem tug = MakeItem(ItemKind::Object, "Tug", "lib/airport/Ramp_Equipment/Tug.obj");
	tug.latitude = 47.4492;
	tug.longitude = -122.3088;
	airport.children.push_back(tug);
	SceneryItem apron = MakeItem(ItemKind::Polygon, "Apron", "lib/airport/ground/pavement/asphalt_1.pol");
	apron.rings.push_back({ Node(47.4480, -122.3100), Node(47.4480, -122.3090), Node(47.4485, -122.3090), Node(47.4485, -122.3100) });
	airport.children.push_back(apron);
	SceneryItem tree = MakeItem(ItemKind::Object, "Tree", "lib/g10/trees/oak.obj");
	tree.latitude = 47.46;
	tree.longitude = -122.32;
	project.items.push_back(tree);		// Not under the airport: no filter
	SceneryItem hidden = MakeItem(ItemKind::Object, "Hidden", "lib/airport/Ramp_Equipment/Tug.obj");
	hidden.latitude = 47.4493;
	hidden.longitude = -122.3087;
	hidden.hidden = true;
	airport.children.push_back(hidden);

	ExportOptions options;
	ExportReport report;
	REQUIRE(ExportSceneryPack(project, projectFolder, projectFolder, options, report, &error));
	CHECK(report.airports == 1);
	CHECK(report.tiles == 1);
	CHECK(report.objects == 3);		// The hidden tug stays out
	CHECK(report.polygons == 1);
	CHECK(report.assets == 0);		// Exporting into the project's own folder copies nothing
	CHECK(report.libraryResources == 3);

	// apt.dat reads back
	XPlane::AptFile apt;
	REQUIRE(XPlane::LoadAptDat(projectFolder / "Earth nav data" / "apt.dat", apt, &error));
	REQUIRE(apt.airports.size() == 1);
	CHECK(apt.airports[0].id == "KTST");
	CHECK(apt.airports[0].GetMetadata("state") == "Pierce County");

	// The tile under the airport, with WorldEditor's header and the airport filter
	const fs::path tilePath = projectFolder / "Earth nav data" / "+40-130" / "+47-123.dsf";
	REQUIRE(fs::is_regular_file(tilePath));
	XPlane::DsfFile dsf;
	REQUIRE(XPlane::LoadDsf(tilePath, dsf, &error));
	CHECK(dsf.properties[0].first == "sim/west");
	CHECK(dsf.GetProperty("sim/west") == "-123");
	CHECK(dsf.GetProperty("sim/north") == "48");
	CHECK(dsf.GetProperty("sim/planet") == "earth");
	CHECK(dsf.GetProperty("sim/creation_agent") == "Scenery Editor X");
	CHECK(dsf.IsOverlay());
	CHECK(dsf.GetProperty("sim/filter/aptid") == "KTST");
	REQUIRE(dsf.objects.size() == 3);
	size_t filtered = 0;
	for (const XPlane::DsfObjectPlacement &object : dsf.objects)
	{
		const std::string &name = dsf.objectDefinitions[object.definition];
		CHECK(object.filter == (name == "lib/g10/trees/oak.obj" ? -1 : 0));
		filtered += object.filter == 0 ? 1 : 0;
		if (name == "Objects/Hangar.obj")
		{
			CHECK(object.latitude == Approx(47.4495).margin(1e-7));
			CHECK(object.heading == Approx(90.0).margin(0.01));
		}
	}
	CHECK(filtered == 2);
	REQUIRE(dsf.polygons.size() == 1);
	CHECK(dsf.polygons[0].filter == 0);

	// Exporting elsewhere copies the pack's art with what it names (and nothing else)
	const fs::path elsewhere = scratch.path / "Exported";
	REQUIRE(ExportSceneryPack(project, projectFolder, elsewhere, options, report, &error));
	CHECK(fs::is_regular_file(elsewhere / "Earth nav data" / "apt.dat"));
	CHECK(fs::is_regular_file(elsewhere / "Earth nav data" / "+40-130" / "+47-123.dsf"));
	CHECK(VFS::FindCaseInsensitive(elsewhere / "objects" / "hangar.obj").has_value());
	CHECK(VFS::FindCaseInsensitive(elsewhere / "objects" / "hangar.png").has_value());
	CHECK(VFS::FindCaseInsensitive(elsewhere / "objects" / "hangar.dds").has_value());
	CHECK_FALSE(fs::exists(elsewhere / "objects" / "unused.obj"));
	CHECK(report.assets == 3);

	// A tile left from an earlier export is reported, then removed on request
	WriteText(projectFolder / "Earth nav data" / "+40-130" / "+47-124.dsf", "old");
	REQUIRE(ExportSceneryPack(project, projectFolder, projectFolder, options, report, &error));
	CHECK(std::ranges::any_of(report.warnings, [](const std::string &warning) { return warning.find("+47-124.dsf") != std::string::npos; }));
	CHECK(fs::exists(projectFolder / "Earth nav data" / "+40-130" / "+47-124.dsf"));
	options.removeStaleTiles = true;
	REQUIRE(ExportSceneryPack(project, projectFolder, projectFolder, options, report, &error));
	CHECK(report.removed.size() == 1);
	CHECK_FALSE(fs::exists(projectFolder / "Earth nav data" / "+40-130" / "+47-124.dsf"));
	CHECK(fs::exists(tilePath));

	// WorldEditor's file on request
	options.wed = true;
	REQUIRE(ExportSceneryPack(project, projectFolder, projectFolder, options, report, &error));
	CHECK(fs::is_regular_file(projectFolder / "earth.wed.xml"));

	// X-Plane's own folders are refused
	options.protectedFolders = { scratch.path / "Global Scenery" };
	CHECK_FALSE(ExportSceneryPack(project, projectFolder, scratch.path / "Global Scenery" / "Global Airports", options, report, &error));
	CHECK(error.find("X-Plane") != std::string::npos);
}

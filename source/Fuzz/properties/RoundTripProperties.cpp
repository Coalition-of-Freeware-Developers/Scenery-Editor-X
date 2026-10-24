/**
 * -------------------------------------------------------
 * Scenery Editor X - Fuzzing
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * RoundTripProperties.cpp
 * -------------------------------------------------------
 * Round-trip properties (RapidCheck): random airports, projects, DSF tiles and paths go through a writer and the
 * matching reader. The fuzzers start from bytes and find crashes; these start from valid models and find what a
 * save -> load loses or changes, which is what git-tracked project files and exported scenery depend on. A failure
 * prints the smallest model RapidCheck could shrink it to.
 *
 * Run more cases with RC_PARAMS="max_success=1000"; replay a failure with the RC_PARAMS="reproduce=..." it prints.
 * -------------------------------------------------------
 */
#include <catch2/catch_test_macros.hpp>
#include <rapidcheck/catch.h>
#include <SceneryEditorX/filesystem/vfs.h>
#include <SceneryEditorX/project/project_edx.h>
#include <SceneryEditorX/project/project_sex.h>
#include <SceneryEditorX/project/scenery_pack.h>
#include <SceneryEditorX/xplane/xp_apt.h>
#include <SceneryEditorX/xplane/xp_dsf.h>
#include <cmath>
#include <string>
#include <vector>

using namespace SceneryEditorX;
using namespace SceneryEditorX::ProjectFile;
using namespace SceneryEditorX::XPlane;

namespace
{
	// -------------------------------------------------------
	// Generators (drawn inside a property with *gen)

	/* @brief Text from an alphabet, between min and max characters. */
	std::string Text(const std::string &alphabet, const int min, const int max)
	{
		const int length = *rc::gen::inRange(min, max + 1);
		std::string text;
		for (int i = 0; i < length; ++i)
			text += alphabet[static_cast<size_t>(*rc::gen::inRange<int>(0, static_cast<int>(alphabet.size())))];
		return text;
	}

	/* @brief A name: words of letters and digits separated by single spaces (what a scenery author types). */
	std::string Name()
	{
		const int words = *rc::gen::inRange(1, 4);
		std::string name;
		for (int i = 0; i < words; ++i)
			name += (i ? " " : "") + Text("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", 1, 8);
		return name;
	}

	/* @brief Degrees on the 1e-8 grid the formats write. */
	double Degrees(const double center, const double span)
	{
		const auto steps = static_cast<int64_t>(span * 1e8);
		return center + static_cast<double>(*rc::gen::inRange<int64_t>(-steps, steps + 1)) / 1e8;
	}

	AptNodeChain Chain(const double lat, const double lon, const bool closed)
	{
		AptNodeChain chain;
		chain.closed = closed;
		const int count = *rc::gen::inRange(3, 9);
		const std::vector<int> &lines = GetPaintedLineCodes();
		const std::vector<int> &lights = GetLightLineCodes();
		for (int i = 0; i < count; ++i)
		{
			AptNode node;
			node.latitude = Degrees(lat, 0.005);
			node.longitude = Degrees(lon, 0.005);
			if (*rc::gen::inRange(0, 3) == 0)
			{
				node.hasControl = true;
				node.controlLatitude = Degrees(node.latitude, 0.0005);
				node.controlLongitude = Degrees(node.longitude, 0.0005);
			}
			if (*rc::gen::inRange(0, 2) == 0)
				node.lineType = *rc::gen::elementOf(lines);
			if (*rc::gen::inRange(0, 4) == 0)
				node.lightType = *rc::gen::elementOf(lights);
			chain.nodes.push_back(node);
		}
		return chain;
	}

	AptAirport Airport()
	{
		AptAirport airport;
		airport.id = Text("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789", 3, 7);
		airport.name = Name();
		airport.elevationFt = *rc::gen::inRange(-1000, 15000);
		const double lat = Degrees(0.0, 80.0), lon = Degrees(0.0, 179.0);
		airport.SetMetadata("datum_lat", FormatCoordinate(lat));
		airport.SetMetadata("datum_lon", FormatCoordinate(lon));
		airport.SetMetadata("city", Name());

		const std::vector<int> &surfaces = GetSurfaceCodes();
		for (int i = *rc::gen::inRange(0, 3); i > 0; --i)
		{
			AptRunway runway;
			runway.width = static_cast<float>(*rc::gen::inRange(10, 80));
			runway.surface = *rc::gen::elementOf(surfaces);
			runway.ends[0].designator = std::to_string(*rc::gen::inRange(1, 19));
			runway.ends[1].designator = std::to_string(*rc::gen::inRange(19, 37));
			for (AptRunwayEnd &end : runway.ends)
			{
				end.latitude = Degrees(lat, 0.01);
				end.longitude = Degrees(lon, 0.01);
				end.marking = *rc::gen::inRange(0, 8);
				end.approachLights = *rc::gen::inRange(0, 13);
			}
			airport.runways.push_back(runway);
		}
		for (int i = *rc::gen::inRange(0, 4); i > 0; --i)
		{
			AptPavement pavement;
			pavement.surface = *rc::gen::elementOf(surfaces);
			pavement.description = Name();
			pavement.rings.push_back(Chain(lat, lon, true));
			if (*rc::gen::inRange(0, 3) == 0)
				pavement.rings.push_back(Chain(lat, lon, true));
			airport.pavements.push_back(pavement);
		}
		for (int i = *rc::gen::inRange(0, 3); i > 0; --i)
		{
			AptLinearFeature feature;
			feature.description = Name();
			feature.chains.push_back(Chain(lat, lon, *rc::gen::arbitrary<bool>()));
			airport.linearFeatures.push_back(feature);
		}
		for (int i = *rc::gen::inRange(0, 4); i > 0; --i)
		{
			AptStartup startup;
			startup.latitude = Degrees(lat, 0.01);
			startup.longitude = Degrees(lon, 0.01);
			startup.heading = static_cast<float>(*rc::gen::inRange(0, 3600)) / 10.0f;
			startup.name = Name();
			airport.startups.push_back(startup);
		}
		return airport;
	}

	GeoRing Ring(const double lat, const double lon, const int min)
	{
		GeoRing ring;
		for (int i = *rc::gen::inRange(min, min + 6); i > 0; --i)
		{
			GeoNode node;
			node.latitude = Degrees(lat, 0.01);
			node.longitude = Degrees(lon, 0.01);
			if (*rc::gen::inRange(0, 4) == 0)
			{
				node.hiLatitude = Degrees(node.latitude, 0.001);
				node.hiLongitude = Degrees(node.longitude, 0.001);
				node.loLatitude = Degrees(node.latitude, 0.001);
				node.loLongitude = Degrees(node.longitude, 0.001);
				node.split = *rc::gen::arbitrary<bool>();
			}
			node.wallType = *rc::gen::inRange(0, 4);
			ring.push_back(node);
		}
		return ring;
	}

	SceneryProject Project()
	{
		SceneryProject project;
		project.info.name = Name();
		project.info.author = Name();
		project.info.description = Name();
		project.info.packageName = Name();
		project.datum.latitude = Degrees(0.0, 80.0);
		project.datum.longitude = Degrees(0.0, 179.0);
		project.datum.elevation = *rc::gen::inRange(-400, 5000);
		project.datum.valid = true;
		const double lat = project.datum.latitude, lon = project.datum.longitude;

		if (*rc::gen::arbitrary<bool>())
		{
			ProjectAirport airport;
			airport.airport = Airport();
			project.airports.push_back(airport);
			SceneryItem item;
			item.kind = ItemKind::Airport;
			item.id = "airport-" + airport.airport.id;
			item.name = airport.airport.name;
			item.airportId = airport.airport.id;
			project.items.push_back(item);
		}

		const std::vector<std::pair<ItemKind, std::string>> kinds = {
			{ ItemKind::Polygon, "lib/airport/ground/pavement/asphalt/strips.pol" }, { ItemKind::Line, "lib/airport/lines/1_single_taxi.lin" },
			{ ItemKind::String, "lib/airport/lights/fast/101_centerline.str" }, { ItemKind::Forest, "lib/g10/forests/conifer.for" },
			{ ItemKind::Facade, "facades/terminal.fac" }, { ItemKind::Object, "objects/hangar.obj" }, { ItemKind::ExclusionZone, "" } };
		const int count = *rc::gen::inRange(0, 8);
		for (int i = 0; i < count; ++i)
		{
			const auto &[kind, resource] = *rc::gen::elementOf(kinds);
			SceneryItem item;
			item.kind = kind;
			item.id = "item-" + std::to_string(i);
			item.name = Name();
			item.resource = resource;
			item.hidden = *rc::gen::inRange(0, 5) == 0;
			item.locked = *rc::gen::inRange(0, 5) == 0;
			if (kind == ItemKind::Object)
			{
				item.latitude = Degrees(lat, 0.01);
				item.longitude = Degrees(lon, 0.01);
				item.heading = static_cast<float>(*rc::gen::inRange(0, 3600)) / 10.0f;
			}
			else if (kind == ItemKind::ExclusionZone)
			{
				GeoNode a, b;
				a.latitude = Degrees(lat, 0.01);
				a.longitude = Degrees(lon, 0.01);
				b.latitude = a.latitude + 0.01;
				b.longitude = a.longitude + 0.01;
				item.rings = { { a, b } };
				item.exclusions = static_cast<uint32_t>(*rc::gen::inRange(1, 256));
			}
			else
			{
				item.rings = { Ring(lat, lon, 3) };
				if (kind == ItemKind::Polygon && *rc::gen::arbitrary<bool>())
					item.rings.push_back(Ring(lat, lon, 3));
				item.closed = *rc::gen::arbitrary<bool>();
				item.height = static_cast<float>(*rc::gen::inRange(1, 400)) / 4.0f;
				item.density = static_cast<float>(*rc::gen::inRange(0, 101)) / 100.0f;
				item.spacing = static_cast<float>(*rc::gen::inRange(1, 200)) / 2.0f;
				item.textureHeading = static_cast<float>(*rc::gen::inRange(0, 3600)) / 10.0f;
				item.pickWalls = kind == ItemKind::Facade && *rc::gen::arbitrary<bool>();
			}
			project.items.push_back(item);
		}
		// What the editor holds: loading repairs a project (uuid, ids, airports missing from the hierarchy) and says so
		NormalizeProject(project);
		return project;
	}
}

// -------------------------------------------------------

TEST_CASE("Coordinates: the New Airport form's format and parser agree", "[properties][coordinate]")
{
	rc::prop("format -> parse gives the value back and formats the same", []
	{
		const bool latitude = *rc::gen::arbitrary<bool>();
		const double value = Degrees(0.0, latitude ? 90.0 : 180.0);
		const std::string text = FormatCoordinate(value);
		double parsed = 0.0;
		std::string error;
		RC_ASSERT(ParseCoordinate(text, latitude, parsed, &error));
		RC_ASSERT(std::abs(parsed - value) <= 5e-9);
		RC_ASSERT(FormatCoordinate(parsed) == text);
	});
}

TEST_CASE("apt.dat: what we write reads back the same", "[properties][apt]")
{
	rc::prop("write -> parse -> write gives the same text and the same airport", []
	{
		AptFile file;
		file.airports.push_back(Airport());
		const std::string written = WriteAptDat(file);

		AptFile reread;
		std::string error;
		RC_ASSERT(ParseAptDat(written, reread, &error));
		RC_ASSERT(reread.airports.size() == 1u);
		const AptAirport &a = file.airports[0], &b = reread.airports[0];
		RC_ASSERT(b.id == a.id);
		RC_ASSERT(b.name == a.name);
		RC_ASSERT(b.elevationFt == a.elevationFt);
		RC_ASSERT(b.runways.size() == a.runways.size());
		RC_ASSERT(b.pavements.size() == a.pavements.size());
		for (size_t i = 0; i < a.pavements.size(); ++i)
		{
			RC_ASSERT(b.pavements[i].rings.size() == a.pavements[i].rings.size());
			for (size_t r = 0; r < a.pavements[i].rings.size(); ++r)
				RC_ASSERT(b.pavements[i].rings[r].nodes.size() == a.pavements[i].rings[r].nodes.size());
		}
		RC_ASSERT(b.linearFeatures.size() == a.linearFeatures.size());
		RC_ASSERT(b.startups.size() == a.startups.size());
		RC_ASSERT(WriteAptDat(reread) == written);
	});
}

TEST_CASE("Projects: .edX and .sex keep everything a save writes", "[properties][project]")
{
	rc::prop(".edX: write -> parse -> write gives the same text", []
	{
		const SceneryProject project = Project();
		const std::string written = WriteEdx(project);
		SceneryProject reread;
		std::string error;
		RC_ASSERT(ParseEdx(written, reread, nullptr, &error));
		RC_ASSERT(reread.CountItems() == project.CountItems());
		RC_ASSERT(WriteEdx(reread) == written);
	});

	rc::prop(".sex: write -> parse -> write gives the same bytes", []
	{
		const SceneryProject project = Project();
		const std::string written = WriteSex(project);
		SceneryProject reread;
		std::string error;
		RC_ASSERT(ParseSex(written.data(), written.size(), reread, nullptr, &error));
		RC_ASSERT(reread.CountItems() == project.CountItems());
		RC_ASSERT(WriteSex(reread) == written);
	});

	rc::prop(".edX and .sex carry the same project", []
	{
		const SceneryProject project = Project();
		const std::string edx = WriteEdx(project);
		SceneryProject fromEdx;
		std::string error;
		RC_ASSERT(ParseEdx(edx, fromEdx, nullptr, &error));
		RC_ASSERT(WriteSex(fromEdx) == WriteSex(project));
	});
}

TEST_CASE("DSF: the writer's quantization stays within a few centimeters", "[properties][dsf]")
{
	rc::prop("write -> read keeps definitions, counts, parameters and positions", []
	{
		DsfFile dsf;
		dsf.properties = { { "sim/west", "-123" }, { "sim/south", "47" }, { "sim/east", "-122" }, { "sim/north", "48" }, { "sim/overlay", "1" } };
		dsf.objectDefinitions = { "objects/a.obj", "lib/airport/b.obj" };
		dsf.polygonDefinitions = { "lib/airport/ground/pavement/asphalt/strips.pol", "facades/x.fac" };
		const double lat = Degrees(47.5, 0.45), lon = Degrees(-122.5, 0.45);
		for (int i = *rc::gen::inRange(0, 12); i > 0; --i)
		{
			DsfObjectPlacement object;
			object.definition = static_cast<uint32_t>(*rc::gen::inRange(0, 2));
			object.latitude = Degrees(lat, 0.04);
			object.longitude = Degrees(lon, 0.04);
			object.heading = *rc::gen::inRange(0, 360);
			dsf.objects.push_back(object);
		}
		for (int i = *rc::gen::inRange(0, 5); i > 0; --i)
		{
			DsfPolygonPlacement polygon;
			polygon.definition = static_cast<uint32_t>(*rc::gen::inRange(0, 2));
			polygon.parameter = static_cast<uint16_t>(*rc::gen::inRange(0, 65536));
			std::vector<DsfPoint> winding;
			for (int p = *rc::gen::inRange(3, 9); p > 0; --p)
			{
				DsfPoint point;
				point.planes = 2;
				point.v[0] = Degrees(lon, 0.005);
				point.v[1] = Degrees(lat, 0.005);
				winding.push_back(point);
			}
			polygon.windings.push_back(winding);
			dsf.polygons.push_back(polygon);
		}

		std::vector<uint8_t> bytes;
		std::string error;
		RC_ASSERT(WriteDsf(dsf, bytes, &error));
		DsfFile back;
		RC_ASSERT(ReadDsf(bytes.data(), bytes.size(), back, &error));
		RC_ASSERT(back.objectDefinitions == dsf.objectDefinitions);
		RC_ASSERT(back.polygonDefinitions == dsf.polygonDefinitions);
		RC_ASSERT(back.objects.size() == dsf.objects.size());
		RC_ASSERT(back.polygons.size() == dsf.polygons.size());

		constexpr double DEGREES = 1e-6;	// ~10 cm
		for (size_t i = 0; i < dsf.objects.size(); ++i)
		{
			RC_ASSERT(back.objects[i].definition == dsf.objects[i].definition);
			RC_ASSERT(std::abs(back.objects[i].latitude - dsf.objects[i].latitude) < DEGREES);
			RC_ASSERT(std::abs(back.objects[i].longitude - dsf.objects[i].longitude) < DEGREES);
			RC_ASSERT(std::abs(back.objects[i].heading - dsf.objects[i].heading) < 0.01);
		}
		for (size_t i = 0; i < dsf.polygons.size(); ++i)
		{
			RC_ASSERT(back.polygons[i].parameter == dsf.polygons[i].parameter);
			RC_ASSERT(back.polygons[i].windings.size() == dsf.polygons[i].windings.size());
			for (size_t w = 0; w < dsf.polygons[i].windings.size(); ++w)
			{
				RC_ASSERT(back.polygons[i].windings[w].size() == dsf.polygons[i].windings[w].size());
				for (size_t p = 0; p < dsf.polygons[i].windings[w].size(); ++p)
				{
					RC_ASSERT(std::abs(back.polygons[i].windings[w][p].v[0] - dsf.polygons[i].windings[w][p].v[0]) < DEGREES);
					RC_ASSERT(std::abs(back.polygons[i].windings[w][p].v[1] - dsf.polygons[i].windings[w][p].v[1]) < DEGREES);
				}
			}
		}
	});
}

TEST_CASE("VFS: path rules hold for any spelling", "[properties][vfs]")
{
	rc::prop("normalizing settles in one pass and an accepted archive name stays in its folder", []
	{
		const std::vector<std::string> pieces = { "a", "B", ".", "..", "/", "\\", ":", "C:", "//", " ", "~", "e\xCC\x81" };
		std::string name;
		for (int i = *rc::gen::inRange(0, 12); i > 0; --i)
			name += *rc::gen::elementOf(pieces);

		bool escaped = false;
		const std::string normal = VFS::NormalizeRelative(name, &escaped);
		RC_ASSERT(VFS::NormalizeRelative(normal) == normal);

		const std::filesystem::path folder = std::filesystem::path("Custom Scenery") / "Pack";
		std::filesystem::path target;
		if (VFS::ResolveInside(folder, name, target))
			RC_ASSERT(VFS::IsInside(target, folder));
	});
}

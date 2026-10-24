/**
 * -------------------------------------------------------
 * Scenery Editor X - Fuzzing
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * fuzz_seed_writer.cpp
 * -------------------------------------------------------
 * Writes the seeds for the binary and project formats with our own writers, so the corpus always matches the current
 * format versions and no binary files are committed:
 *
 *   fuzz_seed_writer <out dir> <repository root>
 *
 * Projects: a new airport project (the New Airport Project form's starter layout), the same with one item of every
 * kind, and the repository's sample files (docs/MySceneryProject.edx, docs/XCoder/earth.wed.xml). Each is written as
 * .sex, .edX, earth.wed.xml, apt.dat and a DSF tile (plus a synthetic tile with every placement shape).
 * -------------------------------------------------------
 */
#include <SceneryEditorX/project/project_dsf.h>
#include <SceneryEditorX/project/project_edx.h>
#include <SceneryEditorX/project/project_sex.h>
#include <SceneryEditorX/project/project_wed.h>
#include <SceneryEditorX/project/scenery_pack.h>
#include <SceneryEditorX/xplane/xp_apt.h>
#include <SceneryEditorX/xplane/xp_dsf.h>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>

using namespace SceneryEditorX;
using namespace SceneryEditorX::ProjectFile;
namespace fs = std::filesystem;

namespace
{
	fs::path g_Out;

	void Write(const std::string &folder, const std::string &name, const std::string_view bytes)
	{
		fs::create_directories(g_Out / folder);
		std::ofstream file(g_Out / folder / name, std::ios::binary | std::ios::trunc);
		file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
		if (!file)
		{
			std::fprintf(stderr, "fuzz_seed_writer: couldn't write %s/%s\n", folder.c_str(), name.c_str());
			std::exit(1);
		}
	}

	bool ReadFile(const fs::path &path, std::string &out)
	{
		std::ifstream file(path, std::ios::binary);
		if (!file)
			return false;
		std::ostringstream text;
		text << file.rdbuf();
		out = text.str();
		return true;
	}

	GeoNode Node(const double latitude, const double longitude)
	{
		GeoNode node;
		node.latitude = latitude;
		node.longitude = longitude;
		return node;
	}

	GeoRing Square(const double latitude, const double longitude, const double half)
	{
		return { Node(latitude - half, longitude - half), Node(latitude - half, longitude + half), Node(latitude + half, longitude + half), Node(latitude + half, longitude - half) };
	}

	SceneryItem Item(const ItemKind kind, const std::string &id, const std::string &resource)
	{
		SceneryItem item;
		item.kind = kind;
		item.id = id;
		item.name = id;
		item.resource = resource;
		return item;
	}

	/* @brief The form's project with one item of every kind next to the airport. */
	SceneryProject MakeKitchenSink(SceneryProject project)
	{
		const double lat = 47.5, lon = -122.3;
		std::vector<SceneryItem> &items = project.items;

		SceneryItem polygon = Item(ItemKind::Polygon, "polygon-1", "lib/airport/ground/pavement/asphalt/strips.pol");
		polygon.rings = { Square(lat, lon + 0.002, 0.0004), Square(lat, lon + 0.002, 0.0001) };
		polygon.textureHeading = 37.5f;
		items.push_back(polygon);

		SceneryItem ortho = Item(ItemKind::Orthophoto, "ortho-1", "textures/ortho.pol");
		ortho.rings = { Square(lat + 0.002, lon, 0.0003) };
		for (GeoNode &node : ortho.rings[0])
		{
			node.hasUv = true;
			node.s = static_cast<float>(node.longitude > lon);
			node.t = static_cast<float>(node.latitude > lat + 0.002);
		}
		items.push_back(ortho);

		SceneryItem line = Item(ItemKind::Line, "line-1", "lib/airport/lines/1_single_taxi.lin");
		GeoNode curve = Node(lat + 0.0005, lon - 0.0005);
		curve.hiLatitude = lat + 0.0007;
		curve.hiLongitude = lon - 0.0002;
		curve.loLatitude = lat + 0.0003;
		curve.loLongitude = lon - 0.0008;
		line.rings = { { Node(lat, lon - 0.001), curve, Node(lat + 0.001, lon) } };
		items.push_back(line);

		SceneryItem string = Item(ItemKind::String, "string-1", "lib/airport/lights/fast/101_centerline.str");
		string.rings = line.rings;
		string.spacing = 15.0f;
		string.closed = true;
		items.push_back(string);

		SceneryItem forest = Item(ItemKind::Forest, "forest-1", "lib/g10/forests/conifer_cold.for");
		forest.rings = { Square(lat - 0.003, lon, 0.001) };
		forest.density = 0.7f;
		items.push_back(forest);

		SceneryItem facade = Item(ItemKind::Facade, "facade-1", "lib/airport/Ramp_Equipment/Fences/Fence_chain_4m.fac");
		facade.rings = { Square(lat - 0.001, lon - 0.002, 0.0002) };
		for (size_t i = 0; i < facade.rings[0].size(); ++i)
			facade.rings[0][i].wallType = static_cast<int>(i % 2);
		facade.height = 6.0f;
		facade.pickWalls = true;
		items.push_back(facade);

		SceneryItem object = Item(ItemKind::Object, "object-1", "lib/airport/Common_Elements/Hangars/Hangar_32m.obj");
		object.latitude = lat + 0.0015;
		object.longitude = lon + 0.0015;
		object.heading = 271.25f;
		object.elevationMode = ElevationMode::Ground;
		items.push_back(object);

		SceneryItem exclusion = Item(ItemKind::ExclusionZone, "exclusion-1", "");
		exclusion.rings = { { Node(lat - 0.01, lon - 0.01), Node(lat + 0.01, lon + 0.01) } };
		exclusion.exclusions = 0xFF;
		items.push_back(exclusion);

		SceneryItem start = Item(ItemKind::RoadNode, "node-1", "");
		start.latitude = lat - 0.002;
		start.longitude = lon - 0.002;
		SceneryItem end = Item(ItemKind::RoadNode, "node-2", "");
		end.latitude = lat - 0.002;
		end.longitude = lon + 0.002;
		SceneryItem road = Item(ItemKind::Road, "road-1", "lib/g10/roads.net");
		road.rings = { { Node(start.latitude, start.longitude), Node(end.latitude, end.longitude) } };
		road.startNode = start.id;
		road.endNode = end.id;
		road.roadSubtype = 42;
		road.endLayer = 1;

		SceneryItem group = Item(ItemKind::Group, "group-1", "");
		group.children = { start, end, road };
		items.push_back(group);
		return project;
	}

	DsfConversion ToDsf(const SceneryProject &project)
	{
		DsfConversionOptions options;
		options.includeObjects = true;
		DsfConversion conversion = BuildProjectDsf(project, options);
		XPlane::DsfFile &dsf = conversion.dsf;
		dsf.properties.insert(dsf.properties.begin(), { { "sim/west", "-123" }, { "sim/south", "47" }, { "sim/east", "-122" }, { "sim/north", "48" } });
		return conversion;
	}

	void WriteProject(const std::string &name, const SceneryProject &project)
	{
		Write("sex", name + ".sex", WriteSex(project));
		Write("edx", name + ".edX", WriteEdx(project));
		const std::string wed = WriteWed(project);
		Write("wed", name + ".earth.wed.xml", wed);
		Write("xml", name + ".earth.wed.xml", wed);

		XPlane::AptFile apt;
		for (const ProjectAirport &airport : project.airports)
			apt.airports.push_back(airport.airport);
		if (!apt.airports.empty())
			Write("apt_dat", name + ".dat", XPlane::WriteAptDat(apt));

		std::vector<uint8_t> bytes;
		std::string error;
		if (XPlane::WriteDsf(ToDsf(project).dsf, bytes, &error))
			Write("dsf", name + ".dsf", std::string_view(reinterpret_cast<const char *>(bytes.data()), bytes.size()));
		else
			std::fprintf(stderr, "fuzz_seed_writer: no DSF for %s: %s\n", name.c_str(), error.c_str());
	}
}

int main(const int argc, char **argv)
{
	if (argc < 3)
	{
		std::fprintf(stderr, "usage: fuzz_seed_writer <out dir> <repository root>\n");
		return 2;
	}
	g_Out = argv[1];
	const fs::path root = argv[2];

	NewAirportProject spec;
	spec.icao = "KFZZ";
	spec.name = "Fuzz Field";
	spec.country = "United States";
	spec.state = "Washington";
	spec.city = "Seattle";
	spec.latitude = 47.5;
	spec.longitude = -122.3;
	spec.elevationFeet = 433;
	spec.author = "Scenery Editor X fuzzing";
	const SceneryProject airport = MakeAirportProject(spec);
	WriteProject("new_airport", airport);
	WriteProject("every_item", MakeKitchenSink(airport));

	std::string text, error;
	if (ReadFile(root / "docs" / "MySceneryProject.edx", text))
	{
		Write("edx", "MySceneryProject.edx", text);
		SceneryProject sample;
		if (ParseEdx(text, sample, nullptr, &error))
			WriteProject("sample_edx", sample);
	}
	if (ReadFile(root / "docs" / "XCoder" / "earth.wed.xml", text))
	{
		Write("wed", "XCoder.earth.wed.xml", text);
		Write("xml", "XCoder.earth.wed.xml", text);
		SceneryProject xcoder;
		if (ParseWed(text, xcoder, nullptr, &error))
		{
			xcoder.info.name = "XCoder";
			WriteProject("xcoder_wed", xcoder);
		}
		else
			std::fprintf(stderr, "fuzz_seed_writer: docs/XCoder/earth.wed.xml didn't import: %s\n", error.c_str());
	}

	// One tile per 1x1 degree of the kitchen sink, the way the exporter splits them
	std::vector<std::string> warnings;
	int index = 0;
	for (const XPlane::DsfFile &tile : XPlane::SplitDsfByTile(ToDsf(MakeKitchenSink(airport)).dsf, &warnings))
	{
		std::vector<uint8_t> bytes;
		if (XPlane::WriteDsf(tile, bytes, &error))
			Write("dsf", "split_" + std::to_string(index++) + ".dsf", std::string_view(reinterpret_cast<const char *>(bytes.data()), bytes.size()));
	}

	// Regressions: facades the overlay builder once turned into millions of wall quads (more than 18 GB): the highest
	// DSF height on a 4 km footprint, and bezier control points at 0, 0 that make each side thousands of km long. In
	// fuzz_dsf, "a.fac" resolves to a type 1 facade and "ab.fac" to a type 2 one.
	{
		XPlane::DsfFile dsf;
		dsf.properties = { { "sim/west", "-123" }, { "sim/south", "47" }, { "sim/east", "-122" }, { "sim/north", "48" }, { "sim/overlay", "1" } };
		dsf.polygonDefinitions = { "facades/a.fac", "facades/ab.fac" };
		const auto point = [](const double lat, const double lon, const uint8_t planes)
		{
			XPlane::DsfPoint p;
			p.planes = planes;
			p.v[0] = lon;
			p.v[1] = lat;	// Planes 2 and 3 (the bezier control) stay at 0, 0
			return p;
		};
		for (uint32_t definition = 0; definition < 2; ++definition)
		{
			for (const uint8_t planes : { uint8_t(2), uint8_t(4) })
			{
				XPlane::DsfPolygonPlacement facade;
				facade.definition = definition;
				facade.parameter = planes == 2 ? 65535 : 6;
				facade.planes = planes;
				const double half = planes == 2 ? 0.018 : 0.0003;
				facade.windings = { { point(47.5 - half, -122.5 - half, planes), point(47.5 - half, -122.5 + half, planes),
									  point(47.5 + half, -122.5 + half, planes), point(47.5 + half, -122.5 - half, planes) } };
				dsf.polygons.push_back(facade);
			}
		}
		std::vector<uint8_t> bytes;
		if (XPlane::WriteDsf(dsf, bytes, &error))
			Write("dsf", "regression_huge_facades.dsf", std::string_view(reinterpret_cast<const char *>(bytes.data()), bytes.size()));
		else
			std::fprintf(stderr, "fuzz_seed_writer: no facade regression DSF: %s\n", error.c_str());
	}

	std::printf("fuzz_seed_writer: seeds written to %s\n", g_Out.string().c_str());
	return 0;
}

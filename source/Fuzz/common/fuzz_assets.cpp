/**
 * -------------------------------------------------------
 * Scenery Editor X - Fuzzing
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * fuzz_assets.cpp
 * -------------------------------------------------------
 * The asset pack the overlay harnesses draw with. The assets are the small ones the overlay unit tests use
 * (source/Tests/xplane_tests/OverlayTests.cpp), so they are known to parse.
 * -------------------------------------------------------
 */
#include "fuzz_assets.h"
#include "fuzz_common.h"
#include <chrono>
#include <fstream>
#include <random>
#include <string>
#include <system_error>

namespace fs = std::filesystem;

namespace SceneryEditorX::Fuzz
{
	namespace
	{
		const char *POL_TEXT =
			"A\n850\nDRAPED_POLYGON\n\n"
			"TEXTURE\t\tapron.png\n"
			"TEXTURE_NORMAL\t2\tapron_NML.png\n"
			"SCALE\t\t20 10\n"
			"LAYER_GROUP\ttaxiways +1\n"
			"SURFACE\t\tconcrete\n"
			"NO_BLEND 0.6\n";

		const char *LIN_TEXT =
			"A\n850\nLINE_PAINT\n\n"
			"TEXTURE apt_lines.png\n"
			"SCALE 12 3\n"
			"TEX_WIDTH 1024\n"
			"S_OFFSET 0 46 54 62\n"
			"S_OFFSET 1 70 80 90\n"
			"MIRROR\n"
			"LAYER_GROUP markings 2\n";

		const char *STR_TEXT =
			"A\n850\nOBJECT_STRING\n\n"
			"OBJECT -10 10 lamp.obj\n"
			"OBJECT 0 0 lamp.obj\n"
			"OFFSET 1.5\n";

		const char *FOR_TEXT =
			"A\n800\nFOREST\n\n"
			"TEXTURE trees.png\n"
			"SCALE_X 1024\nSCALE_Y 1024\n"
			"SPACING 30 30\nRANDOM 5 5\n"
			"TREE 0 0 256 512 128 60 12 24 2 0\n"
			"TREE 256 0 256 512 128 40 8 16 2 0\n";

		const char *FAC1_TEXT =
			"A\n800\nFACADE\n\n"
			"TEXTURE wall.png\n"
			"RING 1\n"
			"ROOF 0.5 0.5\nROOF 1 0.5\nROOF 1 1\nROOF 0.5 1\n"
			"WALL 0 100 0 0\n"
			"SCALE 20 10\n"
			"LEFT 0 0.25\n"
			"CENTER 0.25 0.5\n"
			"RIGHT 0.5 0.75\n"
			"BOTTOM 0 0.3\n"
			"MIDDLE 0.3 0.6\n"
			"TOP 0.6 1\n";

		const char *FAC2_TEXT =
			"A\n1000\nFACADE\n\n"
			"SHADER_WALL\nTEXTURE wall.png\n"
			"RING 1\n"
			"OBJ lamp.obj\n"
			"FLOOR f1\n"
			"ROOF_HEIGHT 3\n"
			"ROOF_HEIGHT 6\n"
			"SEGMENT 0 0\n"
			"MESH 0 100 1 4 6\n"
			"VERTEX 0 0 0 1 0 0 0 0\n"
			"VERTEX 0 0 -4 1 0 0 1 0\n"
			"VERTEX 0 3 -4 1 0 0 1 1\n"
			"VERTEX 0 3 0 1 0 0 0 1\n"
			"IDX 0 1 2\nIDX 0 2 3\n"
			"ATTACH_DRAPED 0 0.5 0 -2 0\n"
			"SEGMENT 1 0\n"
			"MESH 0 100 1 4 6\n"
			"VERTEX 0 0 0 1 0 0 0 0\n"
			"VERTEX 0 0 -2 1 0 0 0.5 0\n"
			"VERTEX 0 3 -2 1 0 0 0.5 1\n"
			"VERTEX 0 3 0 1 0 0 0 1\n"
			"IDX 0 1 2\nIDX 0 2 3\n"
			"WALL 0 999 0 360 wall\n"
			"SPELLING 0\n"
			"SPELLING 0 1\n"
			"SPELLING 0 0\n";

		const char *AGP_TEXT =
			"A\n1000\nAG_POINT\n\n"
			"TEXTURE ground.png\n"
			"TEXTURE_SCALE 1024 1024\n"
			"TEXTURE_WIDTH 64\n"
			"OBJECT lamp.obj\n"
			"TILE 0 0 1024 1024\n"
			"ANCHOR_PT 512 512\n"
			"OBJ_DRAPED 256 256 90 0\n";

		const char *NET_TEXT =
			"A\n800\nROADS\n\n"
			"TEXTURE 1 cores.png\n"
			"TEXTURE 0 plugs.png\n\n"
			"#VROAD local/city/dry (12.000000)\n"
			"ROAD_DRAPED 2 42 0.080 -1\n"
			"ROAD_DRAPE_CHOICE 0.0 0\n"
			"ROAD_DRAPE_CHOICE 8.0 5001\n\n"
			"ROAD_TYPE 5001 12.0000 64.0000 0 0.75 0.75 0.75\n"
			"ROAD_CENTER 6.000000\n"
			"SCALE 1024\n"
			"SEGMENT_DRAPED 0 0.000000 8000.000000 1.000000 1.000000 256.000000 11.000000 512.000000 asphalt\n"
			"SEGMENT_GRADED 1 0.000000 8000.000000 1.000000 0.000000 0.000000 0.000000 1.000000 0.000000 128.000000 concrete\n"
			"ROAD_TYPE 7 20.0000 30.0000 0 0.2 0.4 0.6\n";

		const char *OBJ_TEXT = "I\n800\nOBJ\n";

		struct AssetPack
		{
			fs::path folder;

			AssetPack()
			{
				// Unique per process: libFuzzer's -fork / -jobs runs several at once
				std::random_device random;
				const auto tick = static_cast<unsigned long long>(std::chrono::steady_clock::now().time_since_epoch().count());
				folder = fs::temp_directory_path() / ("sedx-fuzz-assets-" + std::to_string(random()) + "-" + std::to_string(tick % 1000000));
				fs::create_directories(folder);
				Write("apron.pol", POL_TEXT);
				Write("taxi.lin", LIN_TEXT);
				Write("lamps.str", STR_TEXT);
				Write("trees.for", FOR_TEXT);
				Write("shed.fac", FAC1_TEXT);
				Write("terminal.fac", FAC2_TEXT);
				Write("farm.agp", AGP_TEXT);
				Write("roads.net", NET_TEXT);
				Write("lamp.obj", OBJ_TEXT);
			}

			~AssetPack()
			{
				std::error_code ignored;
				fs::remove_all(folder, ignored);
			}

			void Write(const char *name, const char *text) const
			{
				std::ofstream file(folder / name, std::ios::binary);
				file << text;
			}
		};

		bool EndsWith(const std::string &text, const std::string_view suffix)
		{
			return text.size() >= suffix.size() && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
		}
	}

	const fs::path &GetAssetPack()
	{
		static const AssetPack pack;
		return pack.folder;
	}

	XPlane::AssetResolver MakeAssetResolver()
	{
		const fs::path &pack = GetAssetPack();
		return [pack](const std::string_view path) -> fs::path
		{
			std::string lower(path);
			for (char &c : lower)
				c = (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;

			if (EndsWith(lower, ".pol"))
				return pack / "apron.pol";
			if (EndsWith(lower, ".lin") || lower.find("lines/") != std::string::npos)
				return pack / "taxi.lin";
			if (EndsWith(lower, ".str") || lower.find("lights/") != std::string::npos)
				return pack / "lamps.str";
			if (EndsWith(lower, ".for"))
				return pack / "trees.for";
			if (EndsWith(lower, ".fac"))
				return pack / ((lower.size() % 2) ? "shed.fac" : "terminal.fac");	// Both facade types, chosen by the path
			if (EndsWith(lower, ".agp"))
				return pack / "farm.agp";
			if (EndsWith(lower, ".net"))
				return pack / "roads.net";
			if (EndsWith(lower, ".obj"))
				return pack / "lamp.obj";
			return {};
		};
	}

	void BuildPlacedAsset(const std::string_view extension, const uint8_t *data, const size_t size)
	{
		using namespace XPlane;

		const std::string name = "fuzzed" + std::string(extension);
		const fs::path file = GetAssetPack() / name;
		{
			std::ofstream out(file, std::ios::binary | std::ios::trunc);
			out.write(reinterpret_cast<const char *>(data), static_cast<std::streamsize>(size));
		}

		const AssetResolver pack = MakeAssetResolver();
		const AssetResolver resolver = [&](const std::string_view path) -> fs::path { return path == name ? file : pack(path); };
		const GeoProjection projection(45.6, -122.6);
		OverlayBuilder builder(resolver, projection);

		// planes 3: the wall type; planes 4: a bezier control point 5 m off the node
		const auto point = [&](const double east, const double north, const uint8_t planes = 2, const double wallType = 0.0) -> DsfPoint
		{
			DsfPoint p;
			p.planes = planes;
			projection.ToGeo(Point2{ east, -north }, p.v[1], p.v[0]);
			if (planes == 3)
				p.v[2] = wallType;
			else if (planes >= 4)
				projection.ToGeo(Point2{ east + 5.0, -north + 5.0 }, p.v[3], p.v[2]);
			return p;
		};
		const auto square = [&](const double half, const uint8_t planes = 2, const double wallType = 0.0) -> std::vector<DsfPoint>
		{
			return { point(-half, -half, planes, wallType), point(half, -half, planes, wallType), point(half, half, planes, wallType), point(-half, half, planes, wallType) };
		};

		DsfFile dsf;
		dsf.properties = { { "sim/west", "-123" }, { "sim/south", "45" }, { "sim/east", "-122" }, { "sim/north", "46" }, { "sim/overlay", "1" } };
		if (extension == ".agp")
		{
			dsf.objectDefinitions = { name };
			dsf.objects.push_back({ 0, -122.6, 45.6, 30.0, 0.0, DsfObjectPlacement::Mode::Draped });
			dsf.objects.push_back({ 0, -122.6005, 45.6005, 271.5, 3.0, DsfObjectPlacement::Mode::Agl });
		}
		else if (extension == ".net")
		{
			dsf.networkDefinitions = { name };
			for (const uint32_t subtype : { 0u, 1u, 42u, 5001u })
			{
				DsfRoadSegment road;
				road.subtype = subtype;
				road.startJunction = 1;
				road.endJunction = 2;
				road.endLevel = static_cast<int>(subtype % 3);
				DsfRoadPoint start, control, end;
				projection.ToGeo(Point2{ 0.0, 0.0 }, start.latitude, start.longitude);
				projection.ToGeo(Point2{ 40.0, -20.0 }, control.latitude, control.longitude);
				control.control = true;
				projection.ToGeo(Point2{ 80.0, 0.0 }, end.latitude, end.longitude);
				road.points = { start, control, end };
				dsf.roads.push_back(road);
			}
		}
		else
		{
			dsf.polygonDefinitions = { name };
			const auto add = [&](const uint16_t parameter, const uint8_t planes, std::vector<std::vector<DsfPoint>> windings)
			{
				DsfPolygonPlacement placement;
				placement.parameter = parameter;
				placement.planes = planes;
				placement.windings = std::move(windings);
				dsf.polygons.push_back(std::move(placement));
			};
			// Realistic placements, so the asset is what varies (fuzz_dsf throws the extreme placements at fixed assets)
			add(0, 2, { square(40.0) });
			add(90, 2, { square(60.0), square(20.0) });								// With a hole
			add(1, 2, { { point(-30.0, 0.0), point(30.0, 5.0), point(60.0, -40.0) } });	// Open (lines, strings)
			add(255, 2, { square(150.0) });											// Full forest density
			add(10, 3, { square(25.0, 3, 1.0) });									// Facade wall type 1
			add(6, 4, { square(30.0, 4) });										// Bezier control points
			add(800, 2, { square(15.0) });											// A skyscraper's height
			add(2, 2, { square(1000.0) });											// A 2 km fence / tree line
		}

		OverlayBuilder::Options options;
		options.area.Add(Point2{ -3000.0, -3000.0 });
		options.area.Add(Point2{ 3000.0, 3000.0 });
		OverlayBuild build;
		builder.Build(dsf, options, build);
		CheckOverlay(build);
	}

	void CheckOverlay(const XPlane::OverlayBuild &build)
	{
		for (const XPlane::OverlayMesh &mesh : build.meshes)
		{
			SEDX_FUZZ_CHECK(mesh.indices.size() % 3 == 0, std::string(XPlane::GetOverlayKindName(mesh.kind)) + " mesh with " + std::to_string(mesh.indices.size()) + " indices");
			for (const uint32_t index : mesh.indices)
				SEDX_FUZZ_CHECK(index < mesh.vertices.size(), std::string(XPlane::GetOverlayKindName(mesh.kind)) + " mesh index " + std::to_string(index) + " of " + std::to_string(mesh.vertices.size()) + " vertices");
		}
	}
}

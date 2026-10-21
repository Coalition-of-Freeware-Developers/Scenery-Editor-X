/**
 * -------------------------------------------------------
 * Scenery Editor X - Tests
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * OverlayTests.cpp
 * -------------------------------------------------------
 * DSF reader, the .pol / .lin / .str / .for / .fac / .agp asset parsers and the overlay geometry builder.
 * -------------------------------------------------------
 */
#include <catch2/catch_all.hpp>
#include <SceneryEditorX/xplane/xp_assets.h>
#include <SceneryEditorX/xplane/xp_dsf.h>
#include <SceneryEditorX/xplane/xp_overlay.h>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

using namespace SceneryEditorX::XPlane;
namespace fs = std::filesystem;

namespace
{
	// -------------------------------------------------------
	// A tiny DSF encoder: enough of the format to exercise the reader (atoms, 16-bit pools, commands)

	void PutU8(std::vector<uint8_t> &out, const uint8_t v) { out.push_back(v); }
	void PutU16(std::vector<uint8_t> &out, const uint16_t v) { out.push_back(static_cast<uint8_t>(v & 0xFF)); out.push_back(static_cast<uint8_t>(v >> 8)); }
	void PutU32(std::vector<uint8_t> &out, const uint32_t v) { for (int i = 0; i < 4; ++i) out.push_back(static_cast<uint8_t>((v >> (8 * i)) & 0xFF)); }
	void PutF32(std::vector<uint8_t> &out, const float v) { uint32_t bits; std::memcpy(&bits, &v, 4); PutU32(out, bits); }
	void PutString(std::vector<uint8_t> &out, const std::string &s) { out.insert(out.end(), s.begin(), s.end()); out.push_back(0); }

	/* @brief Wraps content in an atom: the id as big-endian characters stored little-endian ("DAEH" on disk for HEAD). */
	std::vector<uint8_t> Atom(const char *id, const std::vector<uint8_t> &content)
	{
		std::vector<uint8_t> out;
		const uint32_t code = (static_cast<uint32_t>(static_cast<uint8_t>(id[0])) << 24) | (static_cast<uint32_t>(static_cast<uint8_t>(id[1])) << 16) |
							  (static_cast<uint32_t>(static_cast<uint8_t>(id[2])) << 8) | static_cast<uint32_t>(static_cast<uint8_t>(id[3]));
		PutU32(out, code);
		PutU32(out, static_cast<uint32_t>(content.size() + 8));
		out.insert(out.end(), content.begin(), content.end());
		return out;
	}

	std::vector<uint8_t> Concat(const std::vector<std::vector<uint8_t>> &parts)
	{
		std::vector<uint8_t> out;
		for (const auto &part : parts)
			out.insert(out.end(), part.begin(), part.end());
		return out;
	}

	struct TestPool
	{
		std::vector<std::vector<uint16_t>> points;	// Raw per point (planes values)
		std::vector<std::pair<float, float>> scale;	// Per plane: scale, offset
	};

	std::vector<uint8_t> EncodePool(const TestPool &pool, const bool rle)
	{
		std::vector<uint8_t> out;
		PutU32(out, static_cast<uint32_t>(pool.points.size()));
		const uint8_t planes = static_cast<uint8_t>(pool.scale.size());
		PutU8(out, planes);
		for (uint8_t plane = 0; plane < planes; ++plane)
		{
			if (!rle)
			{
				PutU8(out, 0);	// raw
				for (const auto &point : pool.points)
					PutU16(out, point[plane]);
			}
			else
			{
				// RLE + differenced: one literal run holding the deltas
				PutU8(out, 3);
				PutU8(out, static_cast<uint8_t>(pool.points.size()));
				uint16_t last = 0;
				for (const auto &point : pool.points)
				{
					PutU16(out, static_cast<uint16_t>(point[plane] - last));
					last = point[plane];
				}
			}
		}
		return out;
	}

	std::vector<uint8_t> EncodeScale(const TestPool &pool)
	{
		std::vector<uint8_t> out;
		for (const auto &[scale, offset] : pool.scale)
		{
			PutF32(out, scale);
			PutF32(out, offset);
		}
		return out;
	}

	/* @brief Raw 16-bit value for a coordinate under a (scale, offset) plane: value = raw * scale / 65535 + offset. */
	uint16_t Raw(const double value, const std::pair<float, float> &plane)
	{
		return static_cast<uint16_t>(std::lround((value - plane.second) / plane.first * 65535.0));
	}

	std::vector<uint8_t> BuildTestDsf(const bool rle)
	{
		// Tile +45-123 (Portland): longitude plane spans [-123, -122], latitude [45, 46], heading [0, 360], elevation [0, 1000]
		const std::pair<float, float> lonPlane{ 1.0f, -123.0f }, latPlane{ 1.0f, 45.0f }, headingPlane{ 360.0f, 0.0f }, elevationPlane{ 1000.0f, 0.0f };

		// Pool 0: objects with heading and elevation (4 planes)
		TestPool objects;
		objects.scale = { lonPlane, latPlane, headingPlane, elevationPlane };
		objects.points.push_back({ Raw(-122.6, lonPlane), Raw(45.59, latPlane), Raw(90.0, headingPlane), Raw(12.0, elevationPlane) });
		objects.points.push_back({ Raw(-122.59, lonPlane), Raw(45.6, latPlane), Raw(180.0, headingPlane), Raw(0.0, elevationPlane) });
		objects.points.push_back({ Raw(-122.0, lonPlane), Raw(45.0, latPlane), Raw(0.0, headingPlane), Raw(0.0, elevationPlane) });

		// Pool 1: polygon points (2 planes): a 4-point square around (-122.6, 45.6)
		TestPool polygon;
		polygon.scale = { lonPlane, latPlane };
		polygon.points.push_back({ Raw(-122.601, lonPlane), Raw(45.599, latPlane) });
		polygon.points.push_back({ Raw(-122.599, lonPlane), Raw(45.599, latPlane) });
		polygon.points.push_back({ Raw(-122.599, lonPlane), Raw(45.601, latPlane) });
		polygon.points.push_back({ Raw(-122.601, lonPlane), Raw(45.601, latPlane) });

		// Pool 2: empty (networks are written as an empty pool in overlays)
		std::vector<uint8_t> emptyPool;
		PutU32(emptyPool, 0);
		PutU8(emptyPool, 0);

		std::vector<uint8_t> props;
		for (const char *kv : { "sim/west", "-123", "sim/east", "-122", "sim/south", "45", "sim/north", "46", "sim/overlay", "1", "sim/require_object", "1/0", "sim/exclude_obj", "-122.7/45.5/-122.5/45.7" })
			PutString(props, kv);

		std::vector<uint8_t> objt, poly, tert, netw;
		PutString(objt, "objects/hangar.obj");
		PutString(objt, "lib/airport/vehicles/truck.obj");
		PutString(poly, "polygons/apron.pol");
		PutString(poly, "lib/airport/lines/1_single_taxi.lin");

		std::vector<uint8_t> cmds;
		// Objects: definition 0 at point 0 (AGL mode through a comment), definition 1 at point 1
		PutU8(cmds, 34); PutU32(cmds, 6); PutU16(cmds, 2); PutU32(cmds, 1);	// COMMENT32: type 2 (AGL), value 1
		PutU8(cmds, 1); PutU16(cmds, 0);			// POOL_SELECT 0
		PutU8(cmds, 3); PutU8(cmds, 0);				// SET_DEFINITION8 0
		PutU8(cmds, 7); PutU16(cmds, 0);			// OBJECT point 0
		PutU8(cmds, 4); PutU16(cmds, 1);			// SET_DEFINITION16 1
		PutU8(cmds, 8); PutU16(cmds, 1); PutU16(cmds, 3);	// OBJECT_RANGE points 1..3
		// Polygons: definition 0 (pol, heading 45), definition 1 (line, closed)
		PutU8(cmds, 1); PutU16(cmds, 1);			// POOL_SELECT 1
		PutU8(cmds, 3); PutU8(cmds, 0);
		PutU8(cmds, 12); PutU16(cmds, 45); PutU8(cmds, 4); PutU16(cmds, 0); PutU16(cmds, 1); PutU16(cmds, 2); PutU16(cmds, 3);	// POLYGON
		PutU8(cmds, 3); PutU8(cmds, 1);
		PutU8(cmds, 13); PutU16(cmds, 1); PutU16(cmds, 0); PutU16(cmds, 4);	// POLYGON_RANGE
		// A nested polygon: outline plus a hole (the same ring twice is fine for the reader)
		PutU8(cmds, 3); PutU8(cmds, 0);
		PutU8(cmds, 14); PutU16(cmds, 90); PutU8(cmds, 2); PutU8(cmds, 4); PutU16(cmds, 0); PutU16(cmds, 1); PutU16(cmds, 2); PutU16(cmds, 3); PutU8(cmds, 3); PutU16(cmds, 0); PutU16(cmds, 1); PutU16(cmds, 2);

		std::vector<uint8_t> file;
		const std::string magic = "XPLNEDSF";
		file.insert(file.end(), magic.begin(), magic.end());
		PutU32(file, 1);
		const std::vector<uint8_t> body = Concat({
			Atom("HEAD", Atom("PROP", props)),
			Atom("DEFN", Concat({ Atom("TERT", tert), Atom("OBJT", objt), Atom("POLY", poly), Atom("NETW", netw) })),
			Atom("GEOD", Concat({ Atom("POOL", EncodePool(objects, rle)), Atom("SCAL", EncodeScale(objects)), Atom("POOL", EncodePool(polygon, rle)), Atom("SCAL", EncodeScale(polygon)), Atom("POOL", emptyPool), Atom("SCAL", {}) })),
			Atom("CMDS", cmds) });
		file.insert(file.end(), body.begin(), body.end());
		file.insert(file.end(), 16, 0);	// MD5 footer (not checked)
		return file;
	}

	// -------------------------------------------------------

	struct TempDir
	{
		fs::path path;
		TempDir()
		{
			path = fs::temp_directory_path() / ("sedx_overlay_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
			fs::create_directories(path);
		}
		~TempDir()
		{
			std::error_code ec;
			fs::remove_all(path, ec);
		}
		fs::path Write(const std::string &name, const std::string &text) const
		{
			const fs::path file = path / name;
			fs::create_directories(file.parent_path());
			std::ofstream(file, std::ios::binary) << text;
			return file;
		}
	};

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
		"OBJECT 0 0 lib/airport/lights/pole.obj\n"
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
		"OBJ objects/door.obj\n"
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
		"OBJECT objects/shed.obj\n"
		"TILE 0 0 1024 1024\n"
		"ANCHOR_PT 512 512\n"
		"OBJ_DRAPED 256 256 90 0\n";
}

// -------------------------------------------------------

TEST_CASE("DSF reader decodes a synthetic overlay tile", "[dsf]")
{
	for (const bool rle : { false, true })
	{
		const std::vector<uint8_t> bytes = BuildTestDsf(rle);
		DsfFile dsf;
		std::string error;
		REQUIRE(ReadDsf(bytes.data(), bytes.size(), dsf, &error));
		CHECK(error.empty());

		CHECK(dsf.IsOverlay());
		CHECK(dsf.west == Catch::Approx(-123.0));
		CHECK(dsf.north == Catch::Approx(46.0));
		CHECK(dsf.GetProperty("sim/require_object") == "1/0");
		REQUIRE(dsf.GetExclusions().size() == 1);
		CHECK(dsf.GetExclusions()[0].first == "sim/exclude_obj");

		REQUIRE(dsf.objectDefinitions.size() == 2);
		CHECK(dsf.objectDefinitions[1] == "lib/airport/vehicles/truck.obj");
		REQUIRE(dsf.polygonDefinitions.size() == 2);

		REQUIRE(dsf.objects.size() == 3);
		CHECK(dsf.objects[0].definition == 0);
		CHECK(dsf.objects[0].longitude == Catch::Approx(-122.6).epsilon(1e-4));
		CHECK(dsf.objects[0].latitude == Catch::Approx(45.59).epsilon(1e-4));
		CHECK(dsf.objects[0].heading == Catch::Approx(90.0).margin(0.01));
		CHECK(dsf.objects[0].elevation == Catch::Approx(12.0).margin(0.02));
		CHECK(dsf.objects[0].mode == DsfObjectPlacement::Mode::Agl);
		CHECK(dsf.objects[1].definition == 1);
		CHECK(dsf.objects[1].heading == Catch::Approx(180.0).margin(0.01));
		CHECK(dsf.objects[2].longitude == Catch::Approx(-122.0).epsilon(1e-4));

		REQUIRE(dsf.polygons.size() == 3);
		CHECK(dsf.polygons[0].definition == 0);
		CHECK(dsf.polygons[0].parameter == 45);
		CHECK(dsf.polygons[0].planes == 2);
		REQUIRE(dsf.polygons[0].windings.size() == 1);
		REQUIRE(dsf.polygons[0].windings[0].size() == 4);
		CHECK(dsf.polygons[0].windings[0][2].Longitude() == Catch::Approx(-122.599).epsilon(1e-5));
		CHECK(dsf.polygons[0].windings[0][2].Latitude() == Catch::Approx(45.601).epsilon(1e-5));
		CHECK(dsf.polygons[1].definition == 1);
		CHECK(dsf.polygons[1].parameter == 1);
		CHECK(dsf.polygons[1].windings[0].size() == 4);
		CHECK(dsf.polygons[2].windings.size() == 2);
		CHECK(dsf.polygons[2].windings[1].size() == 3);
		CHECK(dsf.roads.empty());
	}
}

TEST_CASE("DSF reader rejects what it can't read", "[dsf]")
{
	DsfFile dsf;
	std::string error;
	const std::string junk = "not a dsf at all, really not";
	CHECK_FALSE(ReadDsf(reinterpret_cast<const uint8_t *>(junk.data()), junk.size(), dsf, &error));
	CHECK_FALSE(error.empty());

	std::vector<uint8_t> sevenZip = { '7', 'z', 0xBC, 0xAF, 0x27, 0x1C };
	sevenZip.resize(64, 0);
	CHECK_FALSE(ReadDsf(sevenZip.data(), sevenZip.size(), dsf, &error));
	CHECK(error.find("7-zip") != std::string::npos);
}

TEST_CASE("DSF tile names and paths", "[dsf]")
{
	CHECK(GetDsfTileName(45.59, -122.6) == "+45-123");
	CHECK(GetDsfTileName(-33.9, 151.2) == "-34+151");
	CHECK(GetDsfTileFolderName(45.59, -122.6) == "+40-130");
	CHECK(GetDsfTileFolderName(-33.9, 151.2) == "-40+150");
	const fs::path tile = GetDsfTilePath("Pack", 45.59, -122.6);
	CHECK(tile.generic_string() == "Pack/Earth nav data/+40-130/+45-123.dsf");
}

// -------------------------------------------------------

TEST_CASE("Draped polygon asset", "[assets]")
{
	PolygonAsset pol;
	std::string error;
	REQUIRE(ParsePolygonAsset(POL_TEXT, pol, &error));
	CHECK(pol.texture == "apron.png");
	CHECK(pol.textureNormal == "apron_NML.png");
	CHECK(pol.scaleS == Catch::Approx(20.0f));
	CHECK(pol.scaleT == Catch::Approx(10.0f));
	CHECK(pol.layerGroup == "taxiways");
	CHECK(pol.layerOffset == 1);
	CHECK(pol.surface == "concrete");
	CHECK(pol.noBlendCutoff == Catch::Approx(0.6f));
	CHECK(pol.wrap);

	AssetHeader header;
	CHECK(ParseAssetHeader(POL_TEXT, header) != std::string::npos);
	CHECK(header.version == 850);
	CHECK(header.type == "DRAPED_POLYGON");
	CHECK_FALSE(ParsePolygonAsset("A\n850\nOBJ\n", pol, &error));
}

TEST_CASE("Line asset", "[assets]")
{
	LineAsset lin;
	REQUIRE(ParseLineAsset(LIN_TEXT, lin));
	CHECK(lin.texture == "apt_lines.png");
	CHECK(lin.scaleS == Catch::Approx(12.0f));
	CHECK(lin.scaleT == Catch::Approx(3.0f));
	CHECK(lin.mirror);
	CHECK(lin.layerGroup == "markings");
	CHECK(lin.layerOffset == 2);
	REQUIRE(lin.segments.size() == 2);
	CHECK(lin.segments[0].s1 == Catch::Approx(46.0f / 1024.0f));
	CHECK(lin.segments[0].sm == Catch::Approx(54.0f / 1024.0f));
	CHECK(lin.segments[0].s2 == Catch::Approx(62.0f / 1024.0f));
	CHECK(lin.segments[1].layer == 1);
	// 16 pixels of 1024 over 12 meters, less a pixel margin each side
	CHECK(lin.GetWidthMeters(0) == Catch::Approx(14.0f / 1024.0f * 12.0f).margin(0.02f));
}

TEST_CASE("Object string asset", "[assets]")
{
	ObjectStringAsset str;
	REQUIRE(ParseObjectStringAsset(STR_TEXT, str));
	REQUIRE(str.objects.size() == 2);
	CHECK(str.objects[0].rotationMin == Catch::Approx(-10.0f));
	CHECK(str.objects[0].rotationMax == Catch::Approx(10.0f));
	CHECK(str.objects[0].object == "lamp.obj");
	CHECK(str.objects[1].object == "lib/airport/lights/pole.obj");
	CHECK(str.offset == Catch::Approx(1.5f));
}

TEST_CASE("Forest asset", "[assets]")
{
	ForestAsset forest;
	REQUIRE(ParseForestAsset(FOR_TEXT, forest));
	CHECK(forest.texture == "trees.png");
	CHECK(forest.scaleX == Catch::Approx(1024.0f));
	CHECK(forest.spacingX == Catch::Approx(30.0f));
	CHECK(forest.randomY == Catch::Approx(5.0f));
	REQUIRE(forest.trees.size() == 2);
	CHECK(forest.trees[0].width == Catch::Approx(256.0f));
	CHECK(forest.trees[0].height == Catch::Approx(512.0f));
	CHECK(forest.trees[0].percent == Catch::Approx(60.0f));
	CHECK(forest.trees[0].minHeight == Catch::Approx(12.0f));
	CHECK(forest.trees[0].maxHeight == Catch::Approx(24.0f));
	CHECK(forest.trees[1].s == Catch::Approx(256.0f));
	CHECK(forest.maxHeight == Catch::Approx(24.0f));
}

TEST_CASE("Type 1 facade asset", "[assets]")
{
	FacadeAsset fac;
	std::string error;
	REQUIRE(ParseFacadeAsset(FAC1_TEXT, fac, &error));
	CHECK_FALSE(fac.IsType2());
	CHECK(fac.ring);
	CHECK(fac.wallTexture == "wall.png");
	CHECK(fac.hasRoof);
	REQUIRE(fac.roofUv.size() == 4);
	CHECK(fac.roofUv[2].first == Catch::Approx(1.0f));
	REQUIRE(fac.walls.size() == 1);
	const FacadeAsset::Wall &wall = fac.walls[0];
	CHECK(wall.scaleS == Catch::Approx(20.0f));
	CHECK(wall.scaleT == Catch::Approx(10.0f));
	REQUIRE(wall.leftPanels.size() == 1);
	CHECK(wall.leftPanels[0].second == Catch::Approx(0.25f));
	REQUIRE(wall.centerPanels.size() == 1);
	REQUIRE(wall.rightPanels.size() == 1);
	REQUIRE(wall.bottomFloors.size() == 1);
	REQUIRE(wall.middleFloors.size() == 1);
	REQUIRE(wall.topFloors.size() == 1);
	CHECK(wall.topFloors[0].first == Catch::Approx(0.6f));
}

TEST_CASE("Type 2 facade asset: segments run along -z and are right-adjusted", "[assets]")
{
	FacadeAsset fac;
	std::string error;
	REQUIRE(ParseFacadeAsset(FAC2_TEXT, fac, &error));
	CHECK(fac.IsType2());
	REQUIRE(fac.objects.size() == 1);
	CHECK(fac.objects[0] == "objects/door.obj");
	REQUIRE(fac.floors.size() == 1);
	const FacadeAsset::Floor &floor = fac.floors[0];
	CHECK(floor.name == "f1");
	REQUIRE(floor.roofHeights.size() == 2);
	CHECK(floor.roofHeights[1] == Catch::Approx(6.0f));
	REQUIRE(floor.segments.size() == 2);
	CHECK(floor.segments[0].width == Catch::Approx(4.0f));
	CHECK(floor.segments[1].width == Catch::Approx(2.0f));
	REQUIRE(floor.segments[0].meshes.size() == 1);
	const FacadeAsset::Mesh &mesh = floor.segments[0].meshes[0];
	CHECK(mesh.lodFar == Catch::Approx(100.0f));
	REQUIRE(mesh.positions.size() == 12);
	REQUIRE(mesh.indices.size() == 6);
	// Every z lies in [-width, 0]
	for (size_t i = 2; i < mesh.positions.size(); i += 3)
	{
		CHECK(mesh.positions[i] <= 0.0f + 1e-5f);
		CHECK(mesh.positions[i] >= -4.0f - 1e-5f);
	}
	REQUIRE(floor.segments[0].attachments.size() == 1);
	CHECK(floor.segments[0].attachments[0].object == 0);
	CHECK(floor.segments[0].attachments[0].z == Catch::Approx(-2.0f));
	REQUIRE(floor.walls.size() == 1);
	CHECK(floor.walls[0].spellings.size() == 3);
	CHECK(floor.walls[0].spellings[1] == std::vector<int>{ 0, 1 });
}

TEST_CASE("Autogen point asset", "[assets]")
{
	AutogenPointAsset agp;
	std::string error;
	REQUIRE(ParseAutogenPointAsset(AGP_TEXT, agp, &error));
	CHECK(agp.texture == "ground.png");
	REQUIRE(agp.objects.size() == 1);
	CHECK(agp.HasTile());
	REQUIRE(agp.tile.size() == 16);
	// TILE corners are pixels scaled by TEXTURE_SCALE to UVs and by TEXTURE_WIDTH (meters per texture width) to meters
	CHECK(agp.tile[8] == Catch::Approx(64.0f));	// x of the far corner, meters
	CHECK(agp.tile[10] == Catch::Approx(1.0f));	// s of the far corner
	CHECK(agp.anchorX == Catch::Approx(32.0f));
	REQUIRE(agp.placedObjects.size() == 1);
	CHECK(agp.placedObjects[0].x == Catch::Approx(16.0f));
	CHECK(agp.placedObjects[0].heading == Catch::Approx(90.0f));
}

TEST_CASE("Layer group order", "[assets]")
{
	CHECK(GetLayerGroupOrder("terrain") < GetLayerGroupOrder("taxiways"));
	CHECK(GetLayerGroupOrder("taxiways") < GetLayerGroupOrder("runways"));
	CHECK(GetLayerGroupOrder("runways") < GetLayerGroupOrder("markings"));
	CHECK(GetLayerGroupOrder("markings") < GetLayerGroupOrder("objects"));
	CHECK(GetLayerGroupOrder("objects") < GetLayerGroupOrder("cars"));
	CHECK(GetLayerGroupOrder("whatever") == GetLayerGroupOrder("objects"));
}

// -------------------------------------------------------

TEST_CASE("Overlay geometry helpers", "[overlay]")
{
	SECTION("Textured polygon UVs follow the scale and heading")
	{
		OverlayMesh mesh;
		const std::vector<std::vector<Point2>> rings = { { { 0.0, 0.0 }, { 20.0, 0.0 }, { 20.0, -10.0 }, { 0.0, -10.0 } } };
		AppendTexturedPolygon(mesh, rings, 0.5f, 20.0f, 10.0f, 0.0);
		REQUIRE(mesh.vertices.size() == 4);
		REQUIRE(mesh.indices.size() == 6);
		for (const OverlayVertex &vertex : mesh.vertices)
			CHECK(vertex.y == Catch::Approx(0.5f));
		// 20 m east is one repeat in s, 10 m north (-z) one repeat in t
		const auto find = [&mesh](const double x, const double z) -> const OverlayVertex &
		{
			for (const OverlayVertex &vertex : mesh.vertices)
				if (std::abs(vertex.x - x) < 1e-4 && std::abs(vertex.z - z) < 1e-4)
					return vertex;
			static OverlayVertex none;
			return none;
		};
		CHECK(find(20.0, 0.0).u == Catch::Approx(1.0f));
		CHECK(find(20.0, 0.0).v == Catch::Approx(0.0f).margin(1e-5));
		CHECK(find(0.0, -10.0).v == Catch::Approx(1.0f));
		CHECK(mesh.bounds.valid);
		CHECK(mesh.bounds.GetCenter().x == Catch::Approx(10.0));

		// Counter-clockwise from above: the signed area of each triangle in (x, -z) is positive
		for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
		{
			const OverlayVertex &a = mesh.vertices[mesh.indices[i]], &b = mesh.vertices[mesh.indices[i + 1]], &c = mesh.vertices[mesh.indices[i + 2]];
			const double area = (b.x - a.x) * (-(c.z - a.z)) - (-(b.z - a.z)) * (c.x - a.x);
			CHECK(area > 0.0);
		}
	}

	SECTION("Ribbon spans the line's texture slice and runs along the path")
	{
		OverlayMesh mesh;
		const std::vector<Point2> path = { { 0.0, 0.0 }, { 0.0, -30.0 } };
		AppendTexturedRibbon(mesh, path, false, 0.1f, 1.0f, 0.25f, 0.75f, 3.0f, false);
		REQUIRE(mesh.vertices.size() == 4);
		REQUIRE(mesh.indices.size() == 6);
		CHECK(mesh.vertices[0].u == Catch::Approx(0.25f));
		CHECK(mesh.vertices[1].u == Catch::Approx(0.75f));
		CHECK(mesh.vertices[0].v == Catch::Approx(0.0f));
		CHECK(mesh.vertices[2].v == Catch::Approx(10.0f));	// 30 m / 3 m per repeat
		CHECK(std::abs(mesh.vertices[0].x - mesh.vertices[1].x) == Catch::Approx(1.0f));

		OverlayMesh mirrored;
		AppendTexturedRibbon(mirrored, path, false, 0.1f, 1.0f, 0.25f, 0.75f, 20.0f, true);
		CHECK(mirrored.vertices[2].v == Catch::Approx(0.5f));	// 1.5 repeats, mirrored back to 0.5
	}

	SECTION("Line paint draws every S_OFFSET row with the path at its center pixel")
	{
		// Row 1 (layer 1) is a narrow yellow line over row 0 (layer 0), a black border; row 2 sits right of the path.
		// Listed out of layer order: the layers decide what paints over what.
		LineAsset lin;
		REQUIRE(ParseLineAsset("A\n850\nLINE_PAINT\n\nTEXTURE lines.png\nSCALE 10 5\nTEX_WIDTH 100\n"
							   "S_OFFSET 1 10 15 20\nS_OFFSET 0 0 15 30\nS_OFFSET 2 40 40 50\n", lin));
		REQUIRE(lin.segments.size() == 3);

		OverlayMesh mesh;
		const std::vector<Point2> path = { { 0.0, 0.0 }, { 0.0, -20.0 } };	// 20 m north
		AppendLinePaint(mesh, lin, path, false, 0.2f);
		REQUIRE(mesh.vertices.size() == 12);	// Three strips, two stations each
		REQUIRE(mesh.indices.size() == 18);

		// Layer order: border (u 0..0.3), then the yellow line (0.1..0.2), then the offset row (0.4..0.5)
		CHECK(mesh.vertices[0].u == Catch::Approx(0.0f));
		CHECK(mesh.vertices[1].u == Catch::Approx(0.3f));
		CHECK(mesh.vertices[4].u == Catch::Approx(0.1f));
		CHECK(mesh.vertices[5].u == Catch::Approx(0.2f));
		CHECK(mesh.vertices[8].u == Catch::Approx(0.4f));

		// Heading north, right of travel is east (+x). The border spans 1.5 m either side; the offset row starts on
		// the path (sm = s1) and reaches 1 m to the west (the s2 edge is left of travel).
		CHECK(mesh.vertices[0].x == Catch::Approx(1.5f));
		CHECK(mesh.vertices[1].x == Catch::Approx(-1.5f));
		CHECK(mesh.vertices[4].x == Catch::Approx(0.5f));
		CHECK(mesh.vertices[5].x == Catch::Approx(-0.5f));
		CHECK(mesh.vertices[8].x == Catch::Approx(0.0f).margin(1e-6));
		CHECK(mesh.vertices[9].x == Catch::Approx(-1.0f));
		CHECK(mesh.vertices[2].v == Catch::Approx(4.0f));	// 20 m at 5 m per repeat
		for (const OverlayVertex &vertex : mesh.vertices)
			CHECK(vertex.y == Catch::Approx(0.2f));

		// Counter-clockwise from above, whichever side of the path a strip lies
		for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
		{
			const OverlayVertex &a = mesh.vertices[mesh.indices[i]], &b = mesh.vertices[mesh.indices[i + 1]], &c = mesh.vertices[mesh.indices[i + 2]];
			CHECK((b.x - a.x) * (-(c.z - a.z)) - (-(b.z - a.z)) * (c.x - a.x) > 0.0);
		}
	}

	SECTION("Line paint caps, mirror and align")
	{
		LineAsset lin;
		REQUIRE(ParseLineAsset("A\n850\nLINE_PAINT\n\nTEXTURE lines.png\nSCALE 10 10\nTEX_WIDTH 100\nTEX_HEIGHT 100\n"
							   "S_OFFSET 0 0 5 10\nSTART_CAP 0 20 25 30 0 20\nEND_CAP 0 40 45 50 50 60\n", lin));
		const std::vector<Point2> path = { { 0.0, 0.0 }, { 0.0, -20.0 } };

		// Start cap 2 m (t 0..0.2), end cap 1 m (t 0.5..0.6), the body between
		OverlayMesh mesh;
		AppendLinePaint(mesh, lin, path, false, 0.0f);
		REQUIRE(mesh.vertices.size() == 12);
		CHECK(mesh.vertices[0].u == Catch::Approx(0.2f));		// Start cap's s range
		CHECK(mesh.vertices[0].v == Catch::Approx(0.0f));
		CHECK(mesh.vertices[2].z == Catch::Approx(-2.0f));
		CHECK(mesh.vertices[2].v == Catch::Approx(0.2f));
		CHECK(mesh.vertices[4].u == Catch::Approx(0.0f));		// Body
		CHECK(mesh.vertices[4].v == Catch::Approx(0.0f));
		CHECK(mesh.vertices[6].z == Catch::Approx(-19.0f));
		CHECK(mesh.vertices[6].v == Catch::Approx(1.7f));		// 17 m at 10 m per repeat
		CHECK(mesh.vertices[8].u == Catch::Approx(0.4f));		// End cap
		CHECK(mesh.vertices[8].v == Catch::Approx(0.5f));
		CHECK(mesh.vertices[10].v == Catch::Approx(0.6f));

		// A closed ring has no caps
		OverlayMesh ring;
		AppendLinePaint(ring, lin, { { 0.0, 0.0 }, { 10.0, 0.0 }, { 10.0, -10.0 }, { 0.0, -10.0 } }, true, 0.0f);
		REQUIRE(ring.vertices.size() == 10);	// Back to the first point
		CHECK(ring.vertices[8].v == Catch::Approx(4.0f));
		CHECK(ring.vertices[8].x == Catch::Approx(ring.vertices[0].x));

		// MIRROR: one strip per repeat, running back on odd repeats
		LineAsset mirrored;
		REQUIRE(ParseLineAsset("A\n850\nLINE_PAINT\n\nTEXTURE lines.png\nSCALE 10 8\nTEX_WIDTH 100\nS_OFFSET 0 0 5 10\nMIRROR\n", mirrored));
		OverlayMesh back;
		AppendLinePaint(back, mirrored, path, false, 0.0f);
		REQUIRE(back.vertices.size() == 12);	// 20 m: 8 + 8 + 4
		CHECK(back.vertices[2].v == Catch::Approx(1.0f));
		CHECK(back.vertices[4].v == Catch::Approx(1.0f));
		CHECK(back.vertices[6].v == Catch::Approx(0.0f).margin(1e-6));
		CHECK(back.vertices[10].v == Catch::Approx(0.5f));

		// ALIGN 1: 20 m at 9 m per repeat ends on 2 whole repeats, not 2.22
		LineAsset aligned = mirrored;
		aligned.mirror = false;
		aligned.align = 1;
		aligned.scaleT = 9.0f;
		OverlayMesh even;
		AppendLinePaint(even, aligned, path, false, 0.0f);
		REQUIRE(even.vertices.size() == 4);
		CHECK(even.vertices[2].v == Catch::Approx(2.0f));
	}

	SECTION("Resampling walks the path at the spacing")
	{
		std::vector<Point2> points;
		std::vector<double> headings;
		ResamplePath({ { 0.0, 0.0 }, { 0.0, -100.0 }, { 50.0, -100.0 } }, false, 25.0, 0.0, points, headings);
		REQUIRE(points.size() == 7);
		CHECK(points[1].z == Catch::Approx(-25.0));
		CHECK(headings[0] == Catch::Approx(0.0).margin(1e-6));		// North
		CHECK(headings[5] == Catch::Approx(90.0).margin(1e-6));		// East
		CHECK(points.back().x == Catch::Approx(50.0));

		ResamplePath({ { 0.0, 0.0 }, { 10.0, 0.0 }, { 10.0, 10.0 }, { 0.0, 10.0 } }, true, 10.0, 0.0, points, headings);
		CHECK(points.size() == 4);
	}
}

TEST_CASE("Overlay builder turns DSF placements into meshes and object placements", "[overlay]")
{
	TempDir temp;
	const fs::path pol = temp.Write("polygons/apron.pol", POL_TEXT);
	temp.Write("polygons/apron.png", "png");
	const fs::path lin = temp.Write("lines/taxi.lin", LIN_TEXT);
	const fs::path str = temp.Write("strings/lamps.str", STR_TEXT);
	temp.Write("strings/lamp.obj", "I\n800\nOBJ\n");
	const fs::path forest = temp.Write("forests/trees.for", FOR_TEXT);
	const fs::path fac1 = temp.Write("facades/shed.fac", FAC1_TEXT);
	const fs::path fac2 = temp.Write("facades/terminal.fac", FAC2_TEXT);
	temp.Write("facades/objects/door.obj", "I\n800\nOBJ\n");
	const fs::path hangar = temp.Write("objects/hangar.obj", "I\n800\nOBJ\n");
	const fs::path agp = temp.Write("autogen/farm.agp", AGP_TEXT);
	temp.Write("autogen/objects/shed.obj", "I\n800\nOBJ\n");

	std::map<std::string, fs::path> library = { { "lib/airport/lights/pole.obj", temp.Write("lib/pole.obj", "I\n800\nOBJ\n") } };
	const fs::path package = temp.path;
	int resolverCalls = 0;
	const AssetResolver resolver = [&](const std::string_view path) -> fs::path
	{
		++resolverCalls;
		const std::string key(path);
		if (const auto it = library.find(key); it != library.end())
			return it->second;
		const fs::path direct(key);
		if (direct.is_absolute())
			return fs::exists(direct) ? direct : fs::path();
		const fs::path local = package / direct;
		return fs::exists(local) ? local : fs::path();
	};

	const GeoProjection projection(45.6, -122.6);
	OverlayBuilder builder(resolver, projection);

	// A tile with one of everything around the datum
	DsfFile dsf;
	dsf.objectDefinitions = { "objects/hangar.obj", "autogen/farm.agp", "objects/missing.obj" };
	dsf.polygonDefinitions = { "polygons\\apron.pol", "lines/taxi.lin", "strings/lamps.str", "forests/trees.for", "facades/shed.fac", "facades/terminal.fac", "roads/road.net" };

	const auto point = [&](const double east, const double north, const uint8_t planes = 2) -> DsfPoint
	{
		DsfPoint p;
		p.planes = planes;
		projection.ToGeo(Point2{ east, -north }, p.v[1], p.v[0]);
		return p;
	};
	const auto square = [&](const double cx, const double cz, const double half) -> std::vector<DsfPoint>
	{
		return { point(cx - half, cz - half), point(cx + half, cz - half), point(cx + half, cz + half), point(cx - half, cz + half) };
	};

	dsf.objects.push_back({ 0, -122.6, 45.6, 45.0, 0.0, DsfObjectPlacement::Mode::Draped });
	dsf.objects.push_back({ 1, -122.6, 45.6, 0.0, 0.0, DsfObjectPlacement::Mode::Draped });	// .agp
	dsf.objects.push_back({ 2, -122.6, 45.6, 0.0, 0.0, DsfObjectPlacement::Mode::Draped });	// missing
	dsf.objects.push_back({ 0, -122.0, 45.0, 0.0, 0.0, DsfObjectPlacement::Mode::Draped });	// far away: culled

	DsfPolygonPlacement apron;
	apron.definition = 0;
	apron.parameter = 90;	// Texture heading
	apron.windings = { square(0.0, 0.0, 50.0) };
	dsf.polygons.push_back(apron);

	DsfPolygonPlacement line;
	line.definition = 1;
	line.parameter = 0;
	line.windings = { { point(-40.0, 0.0), point(40.0, 0.0) } };
	dsf.polygons.push_back(line);

	DsfPolygonPlacement lamps;
	lamps.definition = 2;
	lamps.parameter = 10;	// Spacing
	lamps.windings = { { point(0.0, -60.0), point(0.0, 60.0) } };
	dsf.polygons.push_back(lamps);

	DsfPolygonPlacement trees;
	trees.definition = 3;
	trees.parameter = 255;	// Full density, area fill
	trees.windings = { square(200.0, 200.0, 100.0) };
	dsf.polygons.push_back(trees);

	DsfPolygonPlacement shed;
	shed.definition = 4;
	shed.parameter = 6;		// Height
	shed.windings = { square(-200.0, 0.0, 10.0) };
	dsf.polygons.push_back(shed);

	DsfPolygonPlacement terminal;
	terminal.definition = 5;
	terminal.parameter = 3;
	terminal.windings = { square(300.0, -300.0, 8.0) };
	dsf.polygons.push_back(terminal);

	DsfPolygonPlacement road;
	road.definition = 6;
	road.windings = { { point(0.0, 0.0), point(10.0, 0.0) } };
	dsf.polygons.push_back(road);

	OverlayBuilder::Options options;
	options.area.Add(Point2{ -500.0, -500.0 });
	options.area.Add(Point2{ 500.0, 500.0 });
	options.areaMargin = 100.0;
	options.layerHeight = [](const std::string_view group, const int offset) { return group == "taxiways" ? 1.0f + 0.1f * offset : 0.25f; };

	OverlayBuild out;
	builder.Build(dsf, options, out);

	// Objects: the hangar, the .agp's shed, the lamps along the string, the facade door; the far one culled, the missing one warned
	CHECK(out.culled >= 1);
	CHECK(out.unsupported == 1);	// The road
	REQUIRE_FALSE(out.warnings.empty());
	bool missingWarned = false;
	for (const std::string &warning : out.warnings)
		missingWarned |= warning.find("missing.obj") != std::string::npos;
	CHECK(missingWarned);

	size_t hangars = 0, lampPosts = 0, sheds = 0, doors = 0;
	for (const OverlayObject &object : out.objects)
	{
		if (object.file == hangar)
		{
			++hangars;
			CHECK(object.heading == Catch::Approx(45.0f));
			CHECK(std::abs(object.x) < 0.5);
			CHECK(std::abs(object.z) < 0.5);
			CHECK(object.source == OverlayObject::Source::Dsf);
		}
		else if (object.source == OverlayObject::Source::String)
		{
			++lampPosts;
			CHECK(std::abs(object.x - 1.5) < 0.01);	// OFFSET to the right of a northbound path
			CHECK((object.name == "lamp.obj" || object.name == "pole.obj"));
		}
		else if (object.source == OverlayObject::Source::Autogen)
		{
			++sheds;
			CHECK(object.name == "shed.obj");
		}
		else if (object.source == OverlayObject::Source::Facade)
		{
			++doors;
			// The terminal is a 16 m square around (300, 300) local, drawn counter-clockwise from its south side. Its door
			// is attached 0.5 m outside each wall with heading 0: the OBJ's -Z runs along the wall, as X-Plane places it.
			const double south = std::abs(object.z - 308.5), east = std::abs(object.x - 308.5);
			const double north = std::abs(object.z - 291.5), west = std::abs(object.x - 291.5);
			const double heading = std::fmod(object.heading + 360.0, 360.0);
			if (south < 0.01)
				CHECK(heading == Catch::Approx(90.0).margin(0.01));		// The south wall runs east
			else if (east < 0.01)
				CHECK(heading == Catch::Approx(0.0).margin(0.01));		// The east wall runs north
			else if (north < 0.01)
				CHECK(heading == Catch::Approx(270.0).margin(0.01));
			else if (west < 0.01)
				CHECK(heading == Catch::Approx(180.0).margin(0.01));
			else
				FAIL("A door off its wall at " << object.x << ", " << object.z);
		}
	}
	CHECK(hangars == 1);
	CHECK(lampPosts == 13);	// 120 m at 10 m spacing, both ends
	CHECK(sheds == 1);
	CHECK(doors >= 1);

	// Meshes: the apron (textured, taxiways +1 -> 1.1 m), the line, the forest, two facades (walls + roofs), the agp tile
	std::map<OverlayKind, std::vector<const OverlayMesh *>> byKind;
	for (const OverlayMesh &mesh : out.meshes)
		byKind[mesh.kind].push_back(&mesh);

	REQUIRE(byKind[OverlayKind::DrapedPolygon].size() == 1);
	const OverlayMesh &apronMesh = *byKind[OverlayKind::DrapedPolygon][0];
	CHECK(apronMesh.texture == pol.parent_path() / "apron.png");
	CHECK(apronMesh.cutout);
	CHECK(apronMesh.cutoff == Catch::Approx(0.6f));
	CHECK_FALSE(apronMesh.blended);
	CHECK(apronMesh.placement == 0);
	REQUIRE(apronMesh.vertices.size() >= 4);
	for (const OverlayVertex &vertex : apronMesh.vertices)
		CHECK(vertex.y == Catch::Approx(1.1f));
	CHECK(apronMesh.bounds.GetRadius() == Catch::Approx(50.0 * std::sqrt(2.0)).epsilon(0.01));

	REQUIRE(byKind[OverlayKind::Line].size() == 1);
	const OverlayMesh &lineMesh = *byKind[OverlayKind::Line][0];
	CHECK(lineMesh.texture.filename() == "apt_lines.png");
	CHECK(lineMesh.cutout);
	// Both S_OFFSET rows, mirrored every 3 m along the 80 m line: 27 strips of two stations each per row
	CHECK(lineMesh.vertices.size() == 2 * 27 * 4);
	bool rowZero = false, rowOne = false;
	for (const OverlayVertex &vertex : lineMesh.vertices)
	{
		rowZero |= std::abs(vertex.u - 46.0f / 1024.0f) < 1e-5f;
		rowOne |= std::abs(vertex.u - 70.0f / 1024.0f) < 1e-5f;
	}
	CHECK(rowZero);
	CHECK(rowOne);

	REQUIRE(byKind[OverlayKind::Forest].size() == 1);
	const OverlayMesh &forestMesh = *byKind[OverlayKind::Forest][0];
	CHECK(forestMesh.twoSided);
	CHECK(forestMesh.vertices.size() % 4 == 0);
	CHECK(forestMesh.vertices.size() >= 8 * 20);	// 200 x 200 m at 30 m spacing: ~49 cells, 2 quads each
	float tallest = 0.0f;
	for (const OverlayVertex &vertex : forestMesh.vertices)
		tallest = std::max(tallest, vertex.y);
	CHECK(tallest >= 8.0f);
	CHECK(tallest <= 24.01f);

	REQUIRE(byKind[OverlayKind::Facade].size() >= 3);	// shed walls + roof, terminal walls (+ roof)
	bool shedWalls = false, terminalWalls = false;
	for (const OverlayMesh *mesh : byKind[OverlayKind::Facade])
	{
		float top = 0.0f;
		for (const OverlayVertex &vertex : mesh->vertices)
			top = std::max(top, vertex.y);
		if (mesh->resource == "facades/shed.fac" && mesh->name == "shed.fac")
		{
			shedWalls = true;
			CHECK(top == Catch::Approx(6.0f).margin(0.01f));
			CHECK(mesh->vertices.size() >= 4 * 4 * 3);	// 4 sides, 3 rows, >= 1 column each
		}
		if (mesh->resource == "facades/terminal.fac" && mesh->name == "terminal.fac")
		{
			terminalWalls = true;
			CHECK(top == Catch::Approx(3.0f).margin(0.01f));
			// Each 16 m side is spelled from 4 m / 2 m segments and stretched to fit: the mesh ends meet the corners
			CHECK(mesh->vertices.size() >= 4 * 4);
		}
	}
	CHECK(shedWalls);
	CHECK(terminalWalls);

	REQUIRE(byKind[OverlayKind::Autogen].size() == 1);
	CHECK(byKind[OverlayKind::Autogen][0]->texture.filename() == "ground.png");

	// Assets are parsed once: building again hits the caches, not the resolver
	const int callsBefore = resolverCalls;
	OverlayBuild again;
	builder.Build(dsf, options, again);
	CHECK(again.meshes.size() == out.meshes.size());
	CHECK(resolverCalls == callsBefore);	// Object lookups are remembered too
}

TEST_CASE("UV mapped draped polygons keep each point's UV whatever the triangulation does", "[overlay]")
{
	TempDir temp;
	temp.Write("ortho/tile.pol", "A\n850\nDRAPED_POLYGON\n\nTEXTURE_NOWRAP tile.dds\nSCALE 25 25\nLAYER_GROUP beaches -1\n");
	const fs::path package = temp.path;
	const AssetResolver resolver = [&](const std::string_view path) -> fs::path
	{
		const fs::path local = package / fs::path(std::string(path));
		return fs::exists(local) ? local : fs::path();
	};
	const GeoProjection projection(45.6, -122.6);
	OverlayBuilder builder(resolver, projection);

	// A clockwise (seen from above) 5-point outline with s = east / 100 + 0.5, t = north / 100 + 0.5
	const std::vector<Point2> corners = { { -50.0, 50.0 }, { 50.0, 50.0 }, { 50.0, -50.0 }, { 0.0, -20.0 }, { -50.0, -50.0 } };
	DsfFile dsf;
	dsf.polygonDefinitions = { "ortho/tile.pol" };
	DsfPolygonPlacement ortho;
	ortho.definition = 0;
	ortho.parameter = 65535;
	ortho.planes = 4;
	ortho.windings.emplace_back();
	for (const Point2 &corner : corners)
	{
		DsfPoint p;
		p.planes = 4;
		projection.ToGeo(corner, p.v[1], p.v[0]);
		p.v[2] = corner.x / 100.0 + 0.5;
		p.v[3] = -corner.z / 100.0 + 0.5;
		ortho.windings.back().push_back(p);
	}
	dsf.polygons.push_back(ortho);

	OverlayBuild out;
	builder.Build(dsf, OverlayBuilder::Options{}, out);
	REQUIRE(out.meshes.size() == 1);
	const OverlayMesh &mesh = out.meshes[0];
	REQUIRE(mesh.indices.size() == 9);
	for (const OverlayVertex &vertex : mesh.vertices)
	{
		CHECK(vertex.u == Catch::Approx(vertex.x / 100.0 + 0.5).margin(1e-4));
		CHECK(vertex.v == Catch::Approx(-vertex.z / 100.0 + 0.5).margin(1e-4));
	}
}

TEST_CASE("Forest fill modes: line forests are open chains, point forests single trees", "[overlay]")
{
	TempDir temp;
	temp.Write("forests/trees.for", FOR_TEXT);
	const fs::path package = temp.path;
	const AssetResolver resolver = [&](const std::string_view path) -> fs::path
	{
		const fs::path local = package / fs::path(std::string(path));
		return fs::exists(local) ? local : fs::path();
	};
	const GeoProjection projection(45.6, -122.6);
	OverlayBuilder builder(resolver, projection);

	// An L (x east, z south): two 300 m legs. Closing it, or filling the triangle it spans, would plant trees across
	// the middle, as on the runways of an airport ringed by tree lines and single trees.
	const std::vector<Point2> chain = { { 0.0, 0.0 }, { 300.0, 0.0 }, { 300.0, 300.0 } };
	std::vector<DsfPoint> winding;
	for (const Point2 &corner : chain)
	{
		DsfPoint p;
		p.planes = 2;
		projection.ToGeo(corner, p.v[1], p.v[0]);
		winding.push_back(p);
	}
	const auto distanceToChain = [&chain](const double x, const double z)
	{
		double nearest = 1e30;
		for (size_t i = 0; i + 1 < chain.size(); ++i)
		{
			const Point2 &a = chain[i], &b = chain[i + 1];
			const double dx = b.x - a.x, dz = b.z - a.z;
			const double t = std::clamp(((x - a.x) * dx + (z - a.z) * dz) / (dx * dx + dz * dz), 0.0, 1.0);
			nearest = std::min(nearest, std::hypot(x - (a.x + dx * t), z - (a.z + dz * t)));
		}
		return nearest;
	};

	DsfFile dsf;
	dsf.polygonDefinitions = { "forests/trees.for" };
	DsfPolygonPlacement line;
	line.definition = 0;
	line.parameter = 256 + 255;	// Line, full density
	line.windings = { winding };
	dsf.polygons.push_back(line);
	DsfPolygonPlacement points;
	points.definition = 0;
	points.parameter = 512 + 255;	// Points
	points.windings = { winding };
	dsf.polygons.push_back(points);

	OverlayBuild out;
	builder.Build(dsf, OverlayBuilder::Options{}, out);
	REQUIRE(out.meshes.size() == 2);

	// Trees stand within a crown (at most 1.5 widths of a 24 m tall, 12 m wide tree) of their trunk
	constexpr double crown = 20.0;

	// The line: rows of trees along both legs, none near the diagonal (its midpoint is 106 m from the legs)
	const OverlayMesh &lineMesh = out.meshes[0];
	CHECK(lineMesh.kind == OverlayKind::Forest);
	CHECK(lineMesh.vertices.size() >= 4 * 2 * 20);
	for (const OverlayVertex &vertex : lineMesh.vertices)
		CHECK(distanceToChain(vertex.x, vertex.z) < crown);

	// The points: one tree (two quads) on each, nothing in between
	const OverlayMesh &pointMesh = out.meshes[1];
	CHECK(pointMesh.vertices.size() == 3 * 2 * 4);
	for (const OverlayVertex &vertex : pointMesh.vertices)
	{
		double nearest = 1e30;
		for (const Point2 &corner : chain)
			nearest = std::min(nearest, std::hypot(vertex.x - corner.x, vertex.z - corner.z));
		CHECK(nearest < crown);
	}
}

TEST_CASE("Tiled pavement atlases: RUNWAY_TILE spans SCALE, TEXTURE_TILE repeats it per tile", "[overlay]")
{
	PolygonAsset pol;
	REQUIRE(ParsePolygonAsset("A\n850\nDRAPED_POLYGON\n\nTEXTURE a.png\nRUNWAY_TILE 4 8 256 512 ../maps/noise.png\nSCALE 22 44\n", pol));
	CHECK(pol.tiled);
	CHECK(pol.runwayTile);
	CHECK(pol.tilesX == 4);
	CHECK(pol.tilesY == 8);
	CHECK(pol.tileMapWidth == 256);
	CHECK(pol.tileMapHeight == 512);
	CHECK(pol.tileMap == "../maps/noise.png");

	// X-Plane 12's runway atlases: 4 x 8 square tiles under SCALE 22 44, so one tile (a slab) is 5.5 m
	float scaleS = 0.0f, scaleT = 0.0f;
	OverlayBuilder::GetPolygonUvScale(pol, scaleS, scaleT);
	CHECK(scaleS == Catch::Approx(22.0f));
	CHECK(scaleT == Catch::Approx(44.0f));

	PolygonAsset old;
	REQUIRE(ParsePolygonAsset("A\n850\nDRAPED_POLYGON\n\nTEXTURE a.png\nTEXTURE_TILE 4 2 256 256\nSCALE 10 20\n", old));
	CHECK(old.tiled);
	CHECK_FALSE(old.runwayTile);
	CHECK(old.tileMap.empty());
	OverlayBuilder::GetPolygonUvScale(old, scaleS, scaleT);
	CHECK(scaleS == Catch::Approx(40.0f));
	CHECK(scaleT == Catch::Approx(40.0f));
}

TEST_CASE("Runway polygon directives: markings atlas, noise and decals", "[assets]")
{
	// AEP's Runway_Light_Dirty.pol, abridged (Windows separators, tabs, a NO_BLEND without effect on runways)
	const char *text =
		"A\n850\nDRAPED_POLYGON\n\n#Materials\nTEXTURE Light_Dirty.png\nTEXTURE_NORMAL 1\tConcrete_NML.png\n\n"
		"RUNWAY_MARKINGS 1.0000 1.0000 1.0000 0.5000 ..\\Runway_Markings_Black.png\nNO_BLEND 0.00\n"
		"DECAL_PARAMS 4.0 0.0 0.0 0.0 0.0 0.0 0.0 1.0 0.0 0.0 0.0 0.0 0.0 0.0 Concrete_Detail_Grooved_Overlay.png\n"
		"DECAL_PARAMS 32.0 0.0 0.0 0.0 0.0 0.0 0.0 0.4 0.0 0.0 0.0 0.0 0.0 0.0 ..\\Concrete_Pebble_Detail.png\n"
		"NORMAL_DECAL_PARAMS 4.0 0.0 0.0 0.0 0.0 0.0 1.0 Concrete_Detail_Grooved_Overlay_NML.png 0.30234139\n"
		"RUNWAY_NOISE_PARAMS 0.1 0.37 0.7 0.7 0.25\n"
		"DECAL_LIB  lib/g10/decals/apt_concrete_1t.dcl\n"
		"LAYER_GROUP runways 0\nSCALE 22 44\nSURFACE concrete\n\nRUNWAY_TILE 4 8 256 256 Mapping.png\n";
	PolygonAsset pol;
	std::string error;
	REQUIRE(ParsePolygonAsset(text, pol, &error));
	CHECK(pol.runwayMarkings == "../Runway_Markings_Black.png");
	CHECK(pol.runwayMarkingsColor[0] == Catch::Approx(1.0f));
	CHECK(pol.runwayMarkingsColor[3] == Catch::Approx(0.5f));
	CHECK(pol.noiseParams[1] == Catch::Approx(0.37f));
	CHECK(pol.noiseParams[4] == Catch::Approx(0.25f));
	CHECK(pol.decal == "lib/g10/decals/apt_concrete_1t.dcl");
	CHECK(pol.tileMap == "Mapping.png");
	CHECK(pol.tileMapWidth == 256);

	REQUIRE(pol.decals.size() == 3);
	CHECK_FALSE(pol.decals[0].normal);
	CHECK(pol.decals[0].scaleS == Catch::Approx(4.0f));
	CHECK(pol.decals[0].scaleT == Catch::Approx(4.0f));
	CHECK(pol.decals[0].strength == Catch::Approx(1.0f));
	CHECK(pol.decals[0].texture == "Concrete_Detail_Grooved_Overlay.png");
	CHECK(pol.decals[1].scaleS == Catch::Approx(32.0f));
	CHECK(pol.decals[1].strength == Catch::Approx(0.4f));
	CHECK(pol.decals[1].texture == "../Concrete_Pebble_Detail.png");
	CHECK(pol.decals[2].normal);
	CHECK(pol.decals[2].strength == Catch::Approx(1.0f));
	CHECK(pol.decals[2].gain == Catch::Approx(0.30234139f));
	CHECK(pol.decals[2].texture == "Concrete_Detail_Grooved_Overlay_NML.png");

	// Rows too short to be decals are ignored
	PolygonAsset broken;
	REQUIRE(ParsePolygonAsset("A\n850\nDRAPED_POLYGON\n\nTEXTURE a.png\nDECAL_PARAMS 4.0 tex.png\nNORMAL_DECAL_PARAMS 6.0 0 0\n", broken));
	CHECK(broken.decals.empty());
	CHECK(broken.runwayMarkings.empty());
}

TEST_CASE("Decal library asset", "[assets]")
{
	// X-Plane 12's lib/g10/decals/apt_concrete_1t.dcl
	const char *text =
		"A\n1000\nDECAL\n\n# HF detail\n"
		"DECAL_PARAMS_PROJ\t3.0\t3.0\t0.0\t\t0.00\t0.00\t0.00\t0\t0\t0.0\t\t0.00\t0.00\t0.00\t0\t0\t0.75\t \ttextures/RGBA_asphalt_conc_1.png\n"
		"# LF modulator\n"
		"DECAL_PARAMS\t0.03125\t\t0.0\t\t0\t0\t0\t0\t0.0\t0.1\t\t0\t0\t0\t0\t0.0\t0.3\t \ttextures/RGBA_LF_concrete1.png\n"
		"NORMAL_DECAL_PARAMS_PROJ 2.0 4.0 0.0 0 0 0 0 0.5 textures/n.png 2.0\n";
	DecalLibraryAsset dcl;
	REQUIRE(ParseDecalLibraryAsset(text, dcl));
	REQUIRE(dcl.decals.size() == 3);
	CHECK(dcl.decals[0].scaleS == Catch::Approx(3.0f));
	CHECK(dcl.decals[0].scaleT == Catch::Approx(3.0f));
	CHECK(dcl.decals[0].strength == Catch::Approx(0.75f));
	CHECK(dcl.decals[0].texture == "textures/RGBA_asphalt_conc_1.png");
	CHECK(dcl.decals[1].scaleS == Catch::Approx(0.03125f));
	CHECK(dcl.decals[1].strength == Catch::Approx(0.3f));
	CHECK(dcl.decals[1].texture == "textures/RGBA_LF_concrete1.png");
	CHECK(dcl.decals[2].normal);
	CHECK(dcl.decals[2].scaleS == Catch::Approx(2.0f));
	CHECK(dcl.decals[2].scaleT == Catch::Approx(4.0f));
	CHECK(dcl.decals[2].strength == Catch::Approx(0.5f));
	CHECK(dcl.decals[2].gain == Catch::Approx(2.0f));

	CHECK_FALSE(ParseDecalLibraryAsset("A\n850\nDRAPED_POLYGON\n", dcl));
}

TEST_CASE("Runway marking atlas sprites", "[overlay]")
{
	// Glyphs 1..9, 0, L, R, C every 288 px of a 4096 px atlas from x = 296; quads 200 px wide, 520 tall, v up
	const RunwayMarkingSprite one = GetRunwayMarkingSprite(RunwayMarking::Glyph, '1');
	CHECK(0.5f * (one.u0 + one.u1) == Catch::Approx(296.0f / 4096.0f));
	CHECK(one.u1 - one.u0 == Catch::Approx(200.0f / 4096.0f));
	CHECK(one.v1 == Catch::Approx(1.0f - 30.0f / 2048.0f));
	CHECK(one.v0 == Catch::Approx(1.0f - 550.0f / 2048.0f));
	CHECK(one.paintWidth == Catch::Approx(152.0f / 200.0f));
	CHECK(one.paintLength == Catch::Approx(468.0f / 520.0f));
	const RunwayMarkingSprite zero = GetRunwayMarkingSprite(RunwayMarking::Glyph, '0');
	CHECK(0.5f * (zero.u0 + zero.u1) == Catch::Approx((296.0f + 9.0f * 288.0f) / 4096.0f));
	const RunwayMarkingSprite c = GetRunwayMarkingSprite(RunwayMarking::Glyph, 'C');
	CHECK(0.5f * (c.u0 + c.u1) == Catch::Approx((296.0f + 12.0f * 288.0f) / 4096.0f));

	// Bars sit in the lower part of the atlas, with an 8 px border around the paint
	const RunwayMarkingSprite bar = GetRunwayMarkingSprite(RunwayMarking::ThresholdBar);
	CHECK(bar.u0 == Catch::Approx(2042.0f / 4096.0f));
	CHECK(bar.u1 == Catch::Approx(2118.0f / 4096.0f));
	CHECK(bar.v0 == Catch::Approx(1.0f - 2036.0f / 2048.0f));
	CHECK(bar.v1 == Catch::Approx(1.0f - 588.0f / 2048.0f));
	CHECK(bar.paintWidth == Catch::Approx(60.0f / 76.0f));

	// The edge stripe spans the whole atlas height, so it can repeat along v
	const RunwayMarkingSprite edge = GetRunwayMarkingSprite(RunwayMarking::EdgeStripe);
	CHECK(edge.v0 == Catch::Approx(0.0f));
	CHECK(edge.v1 == Catch::Approx(1.0f));
	CHECK(edge.paintLength == Catch::Approx(1.0f));
}

namespace
{
	/* @brief A 3000 x 45 m runway running north (end 0 south of the origin), its ends "16L" style. */
	AptRunway MakeMarkedRunway(const GeoProjection &projection, const int marking0, const int marking1)
	{
		AptRunway runway;
		runway.width = 45.0f;
		runway.surface = 2;
		projection.ToGeo(Point2{ 0.0, 1500.0 }, runway.ends[0].latitude, runway.ends[0].longitude);
		projection.ToGeo(Point2{ 0.0, -1500.0 }, runway.ends[1].latitude, runway.ends[1].longitude);
		runway.ends[0].designator = "36L";
		runway.ends[1].designator = "18R";
		runway.ends[0].marking = marking0;
		runway.ends[1].marking = marking1;
		return runway;
	}

	/* @brief Center of a quad (4 vertices from index first) in local meters. */
	Point2 QuadCenter(const OverlayMesh &mesh, const size_t first)
	{
		Point2 center;
		for (size_t i = first; i < first + 4; ++i)
		{
			center.x += 0.25 * mesh.vertices[i].x;
			center.z += 0.25 * mesh.vertices[i].z;
		}
		return center;
	}
}

TEST_CASE("Runway markings from the apt.dat marking codes", "[overlay]")
{
	const GeoProjection projection(47.45, -122.3);

	SECTION("Precision runway: bars, designator, touchdown zone, aiming points, centerline, edge stripes")
	{
		const AptRunway runway = MakeMarkedRunway(projection, 3, 3);
		OverlayMesh mesh;
		REQUIRE(AppendRunwayMarkings(mesh, runway, projection, 0.5f));

		// Per end: 12 threshold bars (45 m wide), letter + 2 digits, 2 aiming points, 2 x (3 + 2 + 2 + 1 + 1) touchdown bars;
		// 45 centerline dashes between the designators; 2 edge stripes
		const size_t perEnd = 12 + 3 + 2 + 18;
		const size_t quads = 2 * perEnd + 45 + 2;
		CHECK(mesh.vertices.size() == 4 * quads);
		CHECK(mesh.indices.size() == 6 * quads);
		for (const OverlayVertex &vertex : mesh.vertices)
			CHECK(vertex.y == Catch::Approx(0.5f));

		// Every triangle counter-clockwise from above (the overlay convention)
		for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
		{
			const OverlayVertex &a = mesh.vertices[mesh.indices[i]], &b = mesh.vertices[mesh.indices[i + 1]], &c = mesh.vertices[mesh.indices[i + 2]];
			CHECK((b.z - a.z) * (c.x - a.x) - (b.x - a.x) * (c.z - a.z) >= 0.0f);
		}

		// The first bar of end 0: left of the centerline (heading north: west) by 1.5 bar widths, starting 6.1 m in
		const Point2 firstBar = QuadCenter(mesh, 0);
		CHECK(firstBar.x == Catch::Approx(-2.625).margin(1e-3));
		CHECK(firstBar.z == Catch::Approx(1500.0 - (6.1 + 0.5 * 45.72)).margin(1e-3));
		const Point2 secondBar = QuadCenter(mesh, 4);
		CHECK(secondBar.x == Catch::Approx(2.625).margin(1e-3));

		// The letter after the bars (nearer the threshold than the number), on the centerline, upright from the approach:
		// its texture top (v1) is the end further along the runway
		const size_t letter = 4 * 12;
		const Point2 letterCenter = QuadCenter(mesh, letter);
		CHECK(letterCenter.x == Catch::Approx(0.0).margin(1e-3));
		CHECK(letterCenter.z == Catch::Approx(1500.0 - (6.1 + 45.72 + 12.2 + 0.5 * 18.29)).margin(1e-3));
		const RunwayMarkingSprite l = GetRunwayMarkingSprite(RunwayMarking::Glyph, 'L');
		CHECK(mesh.vertices[letter].u == Catch::Approx(l.u0));
		CHECK(mesh.vertices[letter].v == Catch::Approx(l.v0));
		CHECK(mesh.vertices[letter + 2].v == Catch::Approx(l.v1));
		CHECK(mesh.vertices[letter + 2].z < mesh.vertices[letter].z);		// Further north = further along from end 0
		const Point2 three = QuadCenter(mesh, letter + 4);
		const Point2 six = QuadCenter(mesh, letter + 8);
		CHECK(three.x == Catch::Approx(-4.5).margin(1e-3));
		CHECK(six.x == Catch::Approx(4.5).margin(1e-3));
		CHECK(mesh.vertices[letter + 4].u == Catch::Approx(GetRunwayMarkingSprite(RunwayMarking::Glyph, '3').u0));

		// End 1 reads the other way: its bars start 6.1 m from the north end
		const Point2 otherBar = QuadCenter(mesh, 4 * perEnd);
		CHECK(otherBar.z == Catch::Approx(-1500.0 + (6.1 + 0.5 * 45.72)).margin(1e-3));
		CHECK(otherBar.x == Catch::Approx(2.625).margin(1e-3));		// Heading south, left is east
	}

	SECTION("Visual runway: designators and centerline only")
	{
		const AptRunway runway = MakeMarkedRunway(projection, 1, 1);
		OverlayMesh mesh;
		REQUIRE(AppendRunwayMarkings(mesh, runway, projection, 0.0f));
		// Designators 6.1 m from the threshold, centerline after them: 3 glyphs per end, then dashes
		const Point2 letter = QuadCenter(mesh, 0);
		CHECK(letter.z == Catch::Approx(1500.0 - (6.1 + 0.5 * 18.29)).margin(1e-3));
		const size_t dashes = mesh.vertices.size() / 4 - 6;
		CHECK(dashes > 40);
		CHECK(dashes < 50);
	}

	SECTION("Displaced threshold moves an end's markings in")
	{
		AptRunway runway = MakeMarkedRunway(projection, 2, 0);
		runway.ends[0].displacedThreshold = 300.0f;
		OverlayMesh mesh;
		REQUIRE(AppendRunwayMarkings(mesh, runway, projection, 0.0f));
		CHECK(QuadCenter(mesh, 0).z == Catch::Approx(1500.0 - (300.0 + 6.1 + 0.5 * 45.72)).margin(1e-3));
	}

	SECTION("No markings")
	{
		const AptRunway runway = MakeMarkedRunway(projection, 0, 0);
		OverlayMesh mesh;
		CHECK_FALSE(AppendRunwayMarkings(mesh, runway, projection, 0.0f));
		CHECK(mesh.vertices.empty());
	}
}

TEST_CASE("Overlay builder draws X-Plane 12 runway art: tile atlas, decals and markings", "[overlay]")
{
	TempDir temp;
	const fs::path runwayPol = temp.Write("lib/rwy/runway.pol",
		"A\n850\nDRAPED_POLYGON\n\nTEXTURE runway.png\nSCALE 22 44\nLAYER_GROUP runways 0\n"
		"RUNWAY_TILE 4 8 256 512 ../maps/noise.png\nRUNWAY_MARKINGS 1 0.9 0.8 0.5 RWY_markings.png\n"
		"DECAL_PARAMS 4.0 0.0 0.0 0.0 0.0 0.0 0.0 1.0 0.0 0.0 0.0 0.0 0.0 0.0 detail.png\n"
		"NORMAL_DECAL_PARAMS 6.0 0 0 0 0 0 1 ../textures/NDHF.png 1.0\n"
		"DECAL_LIB lib/decals/test.dcl\n");
	const fs::path dcl = temp.Write("decals/test.dcl", "A\n1000\nDECAL\n\nDECAL_PARAMS 0.5 0 0 0 0 0 0.0 0.3 0 0 0 0 0 0 textures/lf.png\n");
	const std::map<std::string, fs::path> library =
	{
		{ "lib/airport/default_runways/concrete/runway.pol", runwayPol },
		{ "lib/decals/test.dcl", dcl }
	};
	const AssetResolver resolver = [&](const std::string_view path) -> fs::path
	{
		const auto it = library.find(std::string(path));
		return it != library.end() ? it->second : fs::path();
	};

	const GeoProjection projection(47.45, -122.3);
	AptAirport airport;
	airport.id = "TEST";
	airport.runways.push_back(MakeMarkedRunway(projection, 3, 2));
	airport.runways.back().surface = 2;

	OverlayBuilder builder(resolver, projection);
	OverlayBuild out;
	builder.BuildAirportSurfaces(airport, {}, 0.2f, 0.3f, OverlayBuilder::Options{}, out);

	const OverlayMesh *surface = nullptr;
	const OverlayMesh *markings = nullptr;
	for (const OverlayMesh &mesh : out.meshes)
	{
		if (mesh.kind != OverlayKind::Runway)
			continue;
		(mesh.shading == OverlayShading::Markings ? markings : surface) = &mesh;
	}
	REQUIRE(surface);
	REQUIRE(markings);

	// The surface: a tile atlas whose UVs cover 22 x 44 m from end 0's center (u across, v along the runway)
	CHECK(surface->shading == OverlayShading::Tiled);
	CHECK(surface->tilesX == 4);
	CHECK(surface->tilesY == 8);
	CHECK(surface->tileMap.filename() == "noise.png");
	CHECK(surface->tileMapWidth == 256);
	CHECK(surface->tileMapHeight == 512);
	CHECK(surface->metersPerU == Catch::Approx(22.0f));
	CHECK(surface->metersPerV == Catch::Approx(44.0f));
	CHECK(surface->tangentX == Catch::Approx(1.0f).margin(1e-4));	// Heading north: u runs east
	for (const OverlayVertex &vertex : surface->vertices)
	{
		CHECK(vertex.u == Catch::Approx(vertex.x / 22.0f).margin(1e-3));
		CHECK(vertex.v == Catch::Approx((1500.0f - vertex.z) / 44.0f).margin(1e-3));
	}

	// Decals: the .pol's own, then its DECAL_LIB library's (paths relative to the file naming them)
	REQUIRE(surface->decals.size() == 3);
	CHECK(surface->decals[0].texture.filename() == "detail.png");
	CHECK_FALSE(surface->decals[0].normal);
	CHECK(surface->decals[1].normal);
	CHECK(surface->decals[1].scaleS == Catch::Approx(6.0f));
	CHECK(surface->decals[1].texture.parent_path().filename() == "textures");
	CHECK(surface->decals[2].texture.filename() == "lf.png");
	CHECK(surface->decals[2].strength == Catch::Approx(0.3f));
	CHECK(surface->decals[2].texture.parent_path().parent_path().filename() == "decals");

	// The markings: their own blended mesh, just above the runway, painted in the polygon's color
	CHECK(markings->placement == surface->placement);
	CHECK(markings->blended);
	CHECK(markings->texture.filename() == "RWY_markings.png");
	CHECK(markings->color[1] == Catch::Approx(0.9f));
	CHECK(markings->color[3] == Catch::Approx(0.5f));
	REQUIRE_FALSE(markings->vertices.empty());
	CHECK(markings->vertices.front().y > 0.2f);
	CHECK(markings->vertices.front().y < 0.3f);
}

TEST_CASE("Overlay builder library mappings", "[overlay]")
{
	CHECK(OverlayBuilder::GetSurfaceLibraryPolygon(1) == "lib/airport/ground/pavement/asphalt/strips.pol");
	CHECK(OverlayBuilder::GetSurfaceLibraryPolygon(2) == "lib/airport/ground/pavement/concrete/new.pol");
	CHECK(OverlayBuilder::GetSurfaceLibraryPolygon(29) == "lib/airport/ground/pavement/asphalt_D/plain.pol");
	CHECK(OverlayBuilder::GetSurfaceLibraryPolygon(57) == "lib/airport/ground/pavement/concrete_D/worn.pol");
	CHECK(OverlayBuilder::GetSurfaceLibraryPolygon(3).empty());		// Grass
	CHECK(OverlayBuilder::GetSurfaceLibraryPolygon(13).empty());	// Water
	CHECK(OverlayBuilder::GetRunwayLibraryPolygon(1, OverlayKind::Runway) == "lib/airport/default_runways/asphalt/runway.pol");
	CHECK(OverlayBuilder::GetRunwayLibraryPolygon(25, OverlayKind::Pavement) == "lib/airport/default_runways/asphalt/taxiway_plain.pol");
	CHECK(OverlayBuilder::GetRunwayLibraryPolygon(36, OverlayKind::Runway) == "lib/airport/default_runways/asphalt_D3/runway_patch.pol");
	CHECK(OverlayBuilder::GetRunwayLibraryPolygon(23, OverlayKind::Helipad) == "lib/airport/default_runways/asphalt_L/helipad_worn.pol");
	CHECK(OverlayBuilder::GetRunwayLibraryPolygon(2, OverlayKind::Runway) == "lib/airport/default_runways/concrete/runway.pol");
	CHECK(OverlayBuilder::GetRunwayLibraryPolygon(56, OverlayKind::Pavement) == "lib/airport/default_runways/concrete_D/taxiway_dirty.pol");
	CHECK(OverlayBuilder::GetRunwayLibraryPolygon(52, OverlayKind::Runway) == "lib/airport/default_runways/concrete_L/runway_worn.pol");
	CHECK(OverlayBuilder::GetRunwayLibraryPolygon(3, OverlayKind::Runway).empty());		// Grass
	CHECK(OverlayBuilder::GetRunwayLibraryPolygon(1, OverlayKind::DrapedPolygon).empty());
	CHECK(OverlayBuilder::GetLineLibraryPath(1) == "lib/airport/lines/1_");
	CHECK(OverlayBuilder::GetLineLibraryPath(51) == "lib/airport/lines/51_");
	CHECK(OverlayBuilder::GetLineLibraryPath(0).empty());
	CHECK(OverlayBuilder::GetLightStringLibraryPath(102) == "lib/airport/lights/fast/102_");
	CHECK(OverlayBuilder::GetLightStringLibraryPath(100).empty());
	CHECK(OverlayBuilder::GetLightStringSpacing(101) == Catch::Approx(15.0));
	CHECK(OverlayBuilder::GetLightStringSpacing(103) < OverlayBuilder::GetLightStringSpacing(102));
}

TEST_CASE("Overlay builder textures apt.dat surfaces through the library", "[overlay]")
{
	TempDir temp;
	const fs::path pol = temp.Write("lib/asphalt_strips.pol", "A\n850\nDRAPED_POLYGON\n\nTEXTURE asphalt.png\nSCALE 10 10\nLAYER_GROUP taxiways 0\n");
	const fs::path lin = temp.Write("lib/1_single_taxi.lin", LIN_TEXT);
	const fs::path lights = temp.Write("lib/101_centerline.str", "A\n850\nOBJECT_STRING\n\nOBJECT 0 0 light.obj\n");
	temp.Write("lib/light.obj", "I\n800\nOBJ\n");

	std::map<std::string, fs::path> library =
	{
		{ "lib/airport/ground/pavement/asphalt/strips.pol", pol },
		{ "lib/airport/lines/1_", lin },
		{ "lib/airport/lights/fast/101_", lights }
	};
	const AssetResolver resolver = [&](const std::string_view path) -> fs::path
	{
		const std::string key(path);
		if (const auto it = library.find(key); it != library.end())
			return it->second;
		const fs::path direct(key);
		return direct.is_absolute() && fs::exists(direct) ? direct : fs::path();
	};

	AptAirport airport;
	airport.id = "TEST";
	AptPavement pavement;
	pavement.surface = 1;
	pavement.textureHeading = 30.0f;
	AptNodeChain ring;
	ring.closed = true;
	const GeoProjection projection(45.6, -122.6);
	for (const Point2 &corner : { Point2{ -50.0, -50.0 }, Point2{ 50.0, -50.0 }, Point2{ 50.0, 50.0 }, Point2{ -50.0, 50.0 } })
	{
		AptNode node;
		projection.ToGeo(corner, node.latitude, node.longitude);
		node.lineType = 1;
		node.lightType = 101;
		ring.nodes.push_back(node);
	}
	pavement.rings.push_back(ring);
	airport.pavements.push_back(pavement);

	AptRunway runway;
	runway.width = 45.0f;
	runway.surface = 2;		// Concrete: not in the library map above -> no mesh
	projection.ToGeo(Point2{ 0.0, 500.0 }, runway.ends[0].latitude, runway.ends[0].longitude);
	projection.ToGeo(Point2{ 0.0, -500.0 }, runway.ends[1].latitude, runway.ends[1].longitude);
	airport.runways.push_back(runway);

	AptLinearFeature feature;
	AptNodeChain chain;
	for (const Point2 &at : { Point2{ 0.0, 0.0 }, Point2{ 100.0, 0.0 }, Point2{ 200.0, 0.0 } })
	{
		AptNode node;
		projection.ToGeo(at, node.latitude, node.longitude);
		node.lineType = at.x < 150.0 ? 1 : 99;	// The last segment has an unknown code
		chain.nodes.push_back(node);
	}
	feature.chains.push_back(chain);
	airport.linearFeatures.push_back(feature);

	OverlayBuilder builder(resolver, projection);
	OverlayBuilder::Options options;
	OverlayBuild out;
	builder.BuildAirportSurfaces(airport, { 0.1f }, 0.2f, 0.3f, options, out);

	size_t pavements = 0, runways = 0, pavementLines = 0, paintedLines = 0;
	for (const OverlayMesh &mesh : out.meshes)
	{
		switch (mesh.kind)
		{
			case OverlayKind::Pavement:
				++pavements;
				CHECK(mesh.texture.filename() == "asphalt.png");
				CHECK(mesh.placement == 0);
				for (const OverlayVertex &vertex : mesh.vertices)
					CHECK(vertex.y == Catch::Approx(0.1f));
				break;
			case OverlayKind::Runway:		++runways; break;
			case OverlayKind::PavementLine:	++pavementLines; CHECK(mesh.placement == 0); break;
			case OverlayKind::PaintedLine:
				++paintedLines;
				CHECK(mesh.placement == 0);
				for (const OverlayVertex &vertex : mesh.vertices)
					CHECK(vertex.y == Catch::Approx(0.3f));
				break;
			default: break;
		}
	}
	CHECK(pavements == 1);
	CHECK(runways == 0);
	CHECK(pavementLines == 1);
	CHECK(paintedLines == 1);

	// Light string along the pavement ring: 400 m at 15 m
	size_t lights101 = 0;
	for (const OverlayObject &object : out.objects)
	{
		if (object.source == OverlayObject::Source::AirportLights)
		{
			++lights101;
			CHECK(object.name == "light.obj");
		}
	}
	CHECK(lights101 >= 26);
	CHECK(lights101 <= 28);

	// Unknown line code 99 is left out, with one warning naming it
	size_t unknownWarnings = 0;
	for (const std::string &warning : out.warnings)
		unknownWarnings += warning.find("lines/99_") != std::string::npos;
	CHECK(unknownWarnings == 1);
}

TEST_CASE("Overlay builder prefers X-Plane 12's runway and taxiway art", "[overlay]")
{
	TempDir temp;
	const fs::path ground = temp.Write("lib/ground_concrete.pol", "A\n850\nDRAPED_POLYGON\n\nTEXTURE ground.png\nSCALE 10 10\n");
	const fs::path runwayPol = temp.Write("lib/rwy_concrete.pol", "A\n850\nDRAPED_POLYGON\n\nTEXTURE runway.png\nSCALE 22 44\nLAYER_GROUP runways 0\n");

	// The runway's X-Plane 12 polygon is exported, the taxiway's isn't (an X-Plane 11 style library): that one falls back
	const std::map<std::string, fs::path> library =
	{
		{ "lib/airport/ground/pavement/concrete/new.pol", ground },
		{ "lib/airport/default_runways/concrete/runway.pol", runwayPol }
	};
	const AssetResolver resolver = [&](const std::string_view path) -> fs::path
	{
		const auto it = library.find(std::string(path));
		return it != library.end() ? it->second : fs::path();
	};

	const GeoProjection projection(47.45, -122.3);
	AptAirport airport;
	airport.id = "TEST";
	AptPavement pavement;
	pavement.surface = 2;
	AptNodeChain ring;
	ring.closed = true;
	for (const Point2 &corner : { Point2{ -50.0, -50.0 }, Point2{ 50.0, -50.0 }, Point2{ 50.0, 50.0 }, Point2{ -50.0, 50.0 } })
	{
		AptNode node;
		projection.ToGeo(corner, node.latitude, node.longitude);
		ring.nodes.push_back(node);
	}
	pavement.rings.push_back(ring);
	airport.pavements.push_back(pavement);

	AptRunway runway;
	runway.width = 45.0f;
	runway.surface = 2;
	projection.ToGeo(Point2{ 100.0, 500.0 }, runway.ends[0].latitude, runway.ends[0].longitude);
	projection.ToGeo(Point2{ 100.0, -500.0 }, runway.ends[1].latitude, runway.ends[1].longitude);
	airport.runways.push_back(runway);

	OverlayBuilder builder(resolver, projection);
	OverlayBuild out;
	builder.BuildAirportSurfaces(airport, { 0.1f }, 0.2f, 0.3f, OverlayBuilder::Options{}, out);

	const OverlayMesh *runwayMesh = nullptr;
	const OverlayMesh *pavementMesh = nullptr;
	for (const OverlayMesh &mesh : out.meshes)
	{
		if (mesh.kind == OverlayKind::Runway)
			runwayMesh = &mesh;
		else if (mesh.kind == OverlayKind::Pavement)
			pavementMesh = &mesh;
	}
	REQUIRE(runwayMesh);
	REQUIRE(pavementMesh);
	CHECK(runwayMesh->resource == "lib/airport/default_runways/concrete/runway.pol");
	CHECK(runwayMesh->texture.filename() == "runway.png");
	CHECK(pavementMesh->resource == "lib/airport/ground/pavement/concrete/new.pol");
	CHECK(pavementMesh->texture.filename() == "ground.png");

	// Looking for the missing X-Plane 12 taxiway polygon isn't worth a warning
	for (const std::string &warning : out.warnings)
		CHECK(warning.find("default_runways") == std::string::npos);
}

// -------------------------------------------------------
// Road networks

namespace
{
	/* A small X-Plane 10+ network: two textures, a road type with a draped and a flat graded strip (plus a raised curb and
	   a far LOD that aren't drawn), a virtual type choosing it, and an old style type without strips. */
	const char *ROADS_NET = R"(A
800
ROADS

TEXTURE 1 textures/cores.png
TEXTURE 0 textures/plugs.png

#VROAD local/city/dry (12.000000)
ROAD_DRAPED 2 42 0.080 -1
ROAD_DRAPE_CHOICE 0.0 0
ROAD_DRAPE_CHOICE 8.0 5001

ROAD_TYPE 5001 12.0000 64.0000 0 0.75 0.75 0.75
ROAD_CENTER 6.000000
SCALE 1024
SEGMENT_DRAPED 0 0.000000 8000.000000 1.000000 1.000000 256.000000 11.000000 512.000000 asphalt
SEGMENT_DRAPED 0 8000.000000 20000.000000 1.000000 0.000000 0.000000 12.000000 1024.000000 asphalt
SEGMENT_GRADED 1 0.000000 8000.000000 1.000000 0.000000 0.000000 0.000000 1.000000 0.000000 128.000000 concrete
SEGMENT_GRADED 1 0.000000 8000.000000 1.000000 11.000000 0.000000 0.000000 12.000000 0.200000 64.000000
ROAD_TYPE 7 20.0000 30.0000 0 0.2 0.4 0.6
)";
}

TEST_CASE("Road network asset", "[assets][roads]")
{
	RoadNetworkAsset net;
	std::string error;
	REQUIRE(ParseRoadNetworkAsset(ROADS_NET, net, &error));
	REQUIRE(net.textures == std::vector<std::string>{ "textures/cores.png", "textures/plugs.png" });
	REQUIRE(net.virtualTypes.size() == 1);
	CHECK(net.virtualTypes[0].id == 42);
	CHECK(net.virtualTypes[0].description == "local/city/dry");
	CHECK(net.virtualTypes[0].roadType == 5001);

	// Subtype 42 draws as road type 5001: its near draped strip and its flat graded strip (lateral meters from the
	// center, s in texture widths); the far LOD and the raised curb are left out
	const RoadNetworkAsset::RoadType *type = net.FindRoadType(42);
	REQUIRE(type);
	CHECK(type->id == 5001);
	CHECK(type->width == Catch::Approx(12.0f));
	CHECK(type->length == Catch::Approx(64.0f));
	REQUIRE(type->strips.size() == 2);
	CHECK(type->strips[0].texture == 0);
	CHECK(type->strips[0].left == Catch::Approx(-5.0f));
	CHECK(type->strips[0].right == Catch::Approx(5.0f));
	CHECK(type->strips[0].sLeft == Catch::Approx(0.25f));
	CHECK(type->strips[0].sRight == Catch::Approx(0.5f));
	CHECK(type->strips[1].texture == 1);
	CHECK(type->strips[1].left == Catch::Approx(-6.0f));
	CHECK(type->strips[1].right == Catch::Approx(-5.0f));
	CHECK(net.GetSubtypeName(42) == "local/city/dry");

	// Old style: the subtype is the road type number
	const RoadNetworkAsset::RoadType *old = net.FindRoadType(7);
	REQUIRE(old);
	CHECK(old->strips.empty());
	CHECK(old->color[2] == Catch::Approx(0.6f));
	CHECK(net.GetSubtypeName(7) == "Road type 7");
	CHECK(net.FindRoadType(99) == nullptr);

	CHECK_FALSE(ParseRoadNetworkAsset("A\n800\nROADS\n", net, &error));
	CHECK_FALSE(ParseRoadNetworkAsset("A\n800\nLINE_PAINT\n", net, &error));
}

TEST_CASE("Overlay builder draws road segments as their road type's strips", "[overlay][roads]")
{
	TempDir temp;
	temp.Write("roads/roads.net", ROADS_NET);
	const fs::path package = temp.path;
	const AssetResolver resolver = [&](const std::string_view path) -> fs::path
	{
		const fs::path local = package / fs::path(std::string(path));
		return fs::exists(local) ? local : fs::path();
	};
	const GeoProjection projection(45.6, -122.6);
	OverlayBuilder builder(resolver, projection);

	// A 100 m road east, then a curve north-east (cubic: two control points), and an old style road
	const auto point = [&](const double x, const double z, const bool control = false)
	{
		DsfRoadPoint p;
		projection.ToGeo({ x, z }, p.latitude, p.longitude);
		p.control = control;
		return p;
	};
	DsfFile dsf;
	dsf.networkDefinitions = { "roads/roads.net" };
	DsfRoadSegment straight;
	straight.subtype = 42;
	straight.startJunction = 1;
	straight.endJunction = 2;
	straight.points = { point(0.0, 0.0), point(100.0, 0.0) };
	DsfRoadSegment curve = straight;
	curve.startJunction = 2;
	curve.endJunction = 3;
	curve.points = { point(100.0, 0.0), point(140.0, 0.0, true), point(160.0, -20.0, true), point(160.0, -60.0) };
	DsfRoadSegment old = straight;
	old.subtype = 7;
	old.points = { point(0.0, 50.0), point(50.0, 50.0) };
	dsf.roads = { straight, curve, old };

	OverlayBuild out;
	builder.Build(dsf, OverlayBuilder::Options{}, out);
	for (const std::string &warning : out.warnings)
		UNSCOPED_INFO(warning);
	CHECK(out.unsupported == 0);

	// Two meshes per strip road (one per texture), one plain band for the old style type
	std::vector<const OverlayMesh *> roads[3];
	for (const OverlayMesh &mesh : out.meshes)
	{
		REQUIRE(mesh.kind == OverlayKind::Road);
		REQUIRE(mesh.placement < 3);
		roads[mesh.placement].push_back(&mesh);
	}
	REQUIRE(roads[0].size() == 2);
	REQUIRE(roads[1].size() == 2);
	REQUIRE(roads[2].size() == 1);
	CHECK(roads[0][0]->name == "local/city/dry");
	CHECK(roads[0][0]->texture.filename() == "cores.png");
	CHECK(roads[0][0]->texture.parent_path().filename() == "textures");
	CHECK(roads[2][0]->texture.empty());
	CHECK(roads[2][0]->color[2] == Catch::Approx(0.6f));

	// The straight road's draped strip: 10 m wide around the path, right of travel (south, +Z) at s 0.5, along at 1 / 64 per meter
	const OverlayMesh &core = *roads[0][0];
	REQUIRE(core.vertices.size() == 4);
	CHECK(core.indices.size() == 6);
	CHECK(core.vertices[0].z == Catch::Approx(5.0f));
	CHECK(core.vertices[0].u == Catch::Approx(0.5f));
	CHECK(core.vertices[1].z == Catch::Approx(-5.0f));
	CHECK(core.vertices[1].u == Catch::Approx(0.25f));
	CHECK(core.vertices[2].x == Catch::Approx(100.0f).margin(1e-3));
	CHECK(core.vertices[2].v == Catch::Approx(100.0f / 64.0f).margin(1e-3));

	// The curve is flattened: more than its two vertices, ending heading north at (160, -60)
	std::vector<Point2> path;
	builder.FlattenRoad(curve, path, OverlayBuilder::Options{});
	REQUIRE(path.size() > 4);
	CHECK(path.front().x == Catch::Approx(100.0).margin(1e-6));
	CHECK(path.back().x == Catch::Approx(160.0).margin(1e-6));
	CHECK(path.back().z == Catch::Approx(-60.0).margin(1e-6));
	const Point2 &beforeEnd = path[path.size() - 2];
	CHECK(std::abs(beforeEnd.x - 160.0) < 0.05 * std::abs(beforeEnd.z + 60.0));	// Arriving heading north (second order off it)

	// Unknown road types and missing networks are reported, not fatal
	dsf.roads[0].subtype = 99;
	dsf.networkDefinitions = { "roads/missing.net" };
	OverlayBuild missing;
	builder.Build(dsf, OverlayBuilder::Options{}, missing);
	CHECK_FALSE(missing.warnings.empty());
}

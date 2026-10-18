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
		CHECK(dsf.networks.empty());
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
			++doors;
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
	CHECK(lineMesh.vertices.size() == 4);

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

TEST_CASE("Tiled pavement atlases cover SCALE meters per tile", "[overlay]")
{
	PolygonAsset pol;
	REQUIRE(ParsePolygonAsset("A\n850\nDRAPED_POLYGON\n\nTEXTURE a.png\nRUNWAY_TILE 4 8 256 512 ../maps/noise.png\nSCALE 22 44\n", pol));
	CHECK(pol.tiled);
	CHECK(pol.tilesX == 4);
	CHECK(pol.tilesY == 8);
}

TEST_CASE("Overlay builder library mappings", "[overlay]")
{
	CHECK(OverlayBuilder::GetSurfaceLibraryPolygon(1) == "lib/airport/ground/pavement/asphalt/strips.pol");
	CHECK(OverlayBuilder::GetSurfaceLibraryPolygon(2) == "lib/airport/ground/pavement/concrete/new.pol");
	CHECK(OverlayBuilder::GetSurfaceLibraryPolygon(29) == "lib/airport/ground/pavement/asphalt_D/plain.pol");
	CHECK(OverlayBuilder::GetSurfaceLibraryPolygon(57) == "lib/airport/ground/pavement/concrete_D/worn.pol");
	CHECK(OverlayBuilder::GetSurfaceLibraryPolygon(3).empty());		// Grass
	CHECK(OverlayBuilder::GetSurfaceLibraryPolygon(13).empty());	// Water
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

/**
 * -------------------------------------------------------
 * Scenery Editor X - Tests
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * TerrainTests.cpp
 * -------------------------------------------------------
 * The base terrain (plan Phase 10): 7-zip / LZMA unpacking of Global Scenery tiles, DSF terrain patches and rasters,
 * .ter terrain types, region-aware library resolution, the terrain meshes, and the surface overlays are draped on.
 * -------------------------------------------------------
 */
#include <catch2/catch_all.hpp>
#include <SceneryEditorX/environment/season_model.h>
#include <SceneryEditorX/filesystem/vfs.h>
#include <SceneryEditorX/xplane/xp_7z.h>
#include <SceneryEditorX/xplane/xp_assets.h>
#include <SceneryEditorX/xplane/xp_dsf.h>
#include <SceneryEditorX/xplane/xp_library.h>
#include <SceneryEditorX/xplane/xp_terrain.h>
#include <SceneryEditorX/xplane/xp_terrain_mesh.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

using namespace SceneryEditorX::XPlane;
namespace fs = std::filesystem;
using Catch::Matchers::WithinAbs;

namespace
{
#include "LzmaFixtures.inc"

	/* @brief The text the LZMA fixtures hold. */
	std::string LzmaSampleText()
	{
		std::string text;
		char line[128];
		for (int i = 0; i < 300; ++i)
		{
			std::snprintf(line, sizeof(line), "TERRAIN %d lib/g10/terrain10/urban_tmp_wet%d.ter %s\n", i, i % 13, std::string(static_cast<size_t>(i % 7), 'x').c_str());
			text += line;
		}
		return text;
	}

	void PutU8(std::vector<uint8_t> &out, const uint8_t v) { out.push_back(v); }
	void PutU16(std::vector<uint8_t> &out, const uint16_t v) { out.push_back(static_cast<uint8_t>(v & 0xFF)); out.push_back(static_cast<uint8_t>(v >> 8)); }
	void PutU32(std::vector<uint8_t> &out, const uint32_t v) { for (int i = 0; i < 4; ++i) out.push_back(static_cast<uint8_t>((v >> (8 * i)) & 0xFF)); }
	void PutU64(std::vector<uint8_t> &out, const uint64_t v) { for (int i = 0; i < 8; ++i) out.push_back(static_cast<uint8_t>((v >> (8 * i)) & 0xFF)); }
	void PutF32(std::vector<uint8_t> &out, const float v) { uint32_t bits; std::memcpy(&bits, &v, 4); PutU32(out, bits); }
	void PutString(std::vector<uint8_t> &out, const std::string &s) { out.insert(out.end(), s.begin(), s.end()); out.push_back(0); }

	/* @brief 7-zip's variable-length number: leading one bits of the first byte count the extra bytes. */
	void Put7zNumber(std::vector<uint8_t> &out, const uint64_t value)
	{
		for (int extra = 0; extra < 8; ++extra)
		{
			if (value < (1ull << (7 * (extra + 1))))
			{
				const uint8_t mask = static_cast<uint8_t>(0xFF00u >> extra);
				out.push_back(static_cast<uint8_t>(mask | (value >> (8 * extra))));
				for (int i = 0; i < extra; ++i)
					out.push_back(static_cast<uint8_t>(value >> (8 * i)));
				return;
			}
		}
		out.push_back(0xFF);
		PutU64(out, value);
	}

	/* @brief A one-file 7-zip archive around an LZMA stream (as X-Plane's Global Scenery tiles are). */
	std::vector<uint8_t> Build7z(const uint8_t *packed, const size_t packedSize, const uint64_t unpackSize, const uint32_t crc)
	{
		std::vector<uint8_t> header;
		PutU8(header, 0x01);					// Header
		PutU8(header, 0x04);					// MainStreamsInfo
		PutU8(header, 0x06);					// PackInfo: position 0, one stream, its size
		Put7zNumber(header, 0);
		Put7zNumber(header, 1);
		PutU8(header, 0x09);
		Put7zNumber(header, packedSize);
		PutU8(header, 0x00);
		PutU8(header, 0x07);					// UnpackInfo: one folder of one LZMA coder (lc 3, lp 0, pb 2, 64 KB)
		PutU8(header, 0x0B);
		Put7zNumber(header, 1);
		PutU8(header, 0x00);
		Put7zNumber(header, 1);
		PutU8(header, 0x23);
		header.insert(header.end(), { 0x03, 0x01, 0x01 });
		Put7zNumber(header, 5);
		header.insert(header.end(), { 0x5D, 0x00, 0x00, 0x01, 0x00 });
		PutU8(header, 0x0C);
		Put7zNumber(header, unpackSize);
		PutU8(header, 0x0A);					// The folder's CRC
		PutU8(header, 0x01);
		PutU32(header, crc);
		PutU8(header, 0x00);
		PutU8(header, 0x00);					// End of the streams info
		PutU8(header, 0x00);					// End of the header

		std::vector<uint8_t> start;
		PutU64(start, packedSize);
		PutU64(start, header.size());
		PutU32(start, Crc32(header.data(), header.size()));

		std::vector<uint8_t> archive(std::begin(SEVEN_ZIP_SIGNATURE), std::end(SEVEN_ZIP_SIGNATURE));
		archive.insert(archive.end(), { 0x00, 0x04 });
		PutU32(archive, Crc32(start.data(), start.size()));
		archive.insert(archive.end(), start.begin(), start.end());
		archive.insert(archive.end(), packed, packed + packedSize);
		archive.insert(archive.end(), header.begin(), header.end());
		return archive;
	}

	// -------------------------------------------------------
	// A tiny DSF with terrain: atoms, raw 16-bit pools, patches, a 2 x 2 elevation raster

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
		std::vector<std::pair<float, float>> scale;		// Per plane: scale, offset
		std::vector<std::vector<double>> points;		// Values per point
	};

	std::vector<uint8_t> EncodePool(const TestPool &pool)
	{
		std::vector<uint8_t> out;
		PutU32(out, static_cast<uint32_t>(pool.points.size()));
		PutU8(out, static_cast<uint8_t>(pool.scale.size()));
		for (size_t plane = 0; plane < pool.scale.size(); ++plane)
		{
			PutU8(out, 0);	// Raw
			for (const auto &point : pool.points)
				PutU16(out, static_cast<uint16_t>(std::lround((point[plane] - pool.scale[plane].second) / pool.scale[plane].first * 65535.0)));
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

	/**
	 * Tile +47-123. Pool 0 (lon, lat, elevation, normal east / north): a 0.01 degree square of four points at 100 m, one
	 * far point. Pool 1 adds border s / t. The elevation raster rises from 100 m (west edge) to 200 m (east edge).
	 */
	std::vector<uint8_t> BuildTerrainDsf()
	{
		const std::pair<float, float> lon{ 1.0f, -123.0f }, lat{ 1.0f, 47.0f }, elevation{ 65535.0f, -32768.0f }, normal{ 2.0f, -1.0f }, st{ 1.0f, 0.0f };
		TestPool base;
		base.scale = { lon, lat, elevation, normal, normal };
		base.points = {
			{ -122.50, 47.50, 100.0, 0.0, 0.0 },
			{ -122.49, 47.50, 100.0, 0.0, 0.0 },
			{ -122.49, 47.51, DSF_ELEVATION_FROM_RASTER, 0.0, 0.0 },
			{ -122.50, 47.51, 100.0, 0.0, 0.0 },
			{ -122.10, 47.90, 300.0, 0.0, 0.0 },		// Far from the area
		};
		TestPool border;
		border.scale = { lon, lat, elevation, normal, normal, st, st };
		border.points = {
			{ -122.50, 47.50, 100.0, 0.0, 0.0, 0.0, 0.0 },
			{ -122.49, 47.50, 100.0, 0.0, 0.0, 1.0, 0.0 },
			{ -122.49, 47.51, 100.0, 0.0, 0.0, 1.0, 1.0 },
			{ -122.50, 47.51, 100.0, 0.0, 0.0, 0.0, 1.0 },
		};

		std::vector<uint8_t> props;
		for (const char *kv : { "sim/west", "-123", "sim/east", "-122", "sim/south", "47", "sim/north", "48" })
			PutString(props, kv);
		std::vector<uint8_t> tert, demn;
		PutString(tert, "terrain_Water");
		PutString(tert, "lib/g10/terrain10/apt_tmp_wet.ter");
		PutString(demn, "elevation");
		PutString(demn, "spr1");

		std::vector<uint8_t> cmds;
		PutU8(cmds, 3); PutU8(cmds, 1);						// SET_DEFINITION8 1 (the .ter)
		PutU8(cmds, 18); PutU8(cmds, 1); PutF32(cmds, 0.0f); PutF32(cmds, -1.0f);	// TERRAIN_PATCH_FLAGS_LOD physical, 0 .. no limit
		PutU8(cmds, 1); PutU16(cmds, 0);					// POOL_SELECT 0
		PutU8(cmds, 23); PutU8(cmds, 3); PutU16(cmds, 0); PutU16(cmds, 1); PutU16(cmds, 2);	// TRIANGLE
		PutU8(cmds, 26); PutU8(cmds, 3); PutU16(cmds, 0); PutU16(cmds, 2); PutU16(cmds, 3);	// TRIANGLE_STRIP (one triangle)
		PutU8(cmds, 23); PutU8(cmds, 3); PutU16(cmds, 4); PutU16(cmds, 1); PutU16(cmds, 2);	// A triangle reaching the far point
		PutU8(cmds, 17); PutU8(cmds, 2);					// TERRAIN_PATCH_FLAGS overlay
		PutU8(cmds, 1); PutU16(cmds, 1);
		PutU8(cmds, 29); PutU8(cmds, 4); PutU16(cmds, 0); PutU16(cmds, 1); PutU16(cmds, 2); PutU16(cmds, 3);	// TRIANGLE_FAN (two)
		PutU8(cmds, 3); PutU8(cmds, 0);						// Water
		PutU8(cmds, 17); PutU8(cmds, 1);
		PutU8(cmds, 24); PutU8(cmds, 3); PutU16(cmds, 0); PutU16(cmds, 0); PutU16(cmds, 1); PutU16(cmds, 1); PutU16(cmds, 1); PutU16(cmds, 3);	// TRIANGLE_CROSS_POOL
		PutU8(cmds, 16);									// TERRAIN_PATCH: same flags
		PutU8(cmds, 1); PutU16(cmds, 0);
		PutU8(cmds, 25); PutU16(cmds, 0); PutU16(cmds, 3);	// TRIANGLE_RANGE 0..2

		// Rasters: elevation 2 x 2 post-centric signed 16-bit; spr1 1 x 1 unsigned 8-bit scaled
		std::vector<uint8_t> demi, demd, demi2, demd2;
		PutU8(demi, 1); PutU8(demi, 2); PutU16(demi, 5); PutU32(demi, 2); PutU32(demi, 2); PutF32(demi, 1.0f); PutF32(demi, 0.0f);
		for (const int16_t value : { int16_t(100), int16_t(200), int16_t(100), int16_t(200) })
			PutU16(demd, static_cast<uint16_t>(value));
		PutU8(demi2, 1); PutU8(demi2, 1); PutU16(demi2, 6); PutU32(demi2, 1); PutU32(demi2, 1); PutF32(demi2, 2.0f); PutF32(demi2, 0.0f);
		PutU8(demd2, 40);

		std::vector<uint8_t> file;
		const std::string magic = "XPLNEDSF";
		file.insert(file.end(), magic.begin(), magic.end());
		PutU32(file, 1);
		const std::vector<uint8_t> body = Concat({
			Atom("HEAD", Atom("PROP", props)),
			Atom("DEFN", Concat({ Atom("TERT", tert), Atom("OBJT", {}), Atom("POLY", {}), Atom("NETW", {}), Atom("DEMN", demn) })),
			Atom("GEOD", Concat({ Atom("POOL", EncodePool(base)), Atom("SCAL", EncodeScale(base)), Atom("POOL", EncodePool(border)), Atom("SCAL", EncodeScale(border)) })),
			Atom("CMDS", cmds),
			Atom("DEMS", Concat({ Atom("DEMI", demi), Atom("DEMD", demd), Atom("DEMI", demi2), Atom("DEMD", demd2) })) });
		file.insert(file.end(), body.begin(), body.end());
		file.insert(file.end(), 16, 0);
		return file;
	}

	// -------------------------------------------------------

	/* @brief Two triangles over [0, 100] x [0, 100] creased along x = z: y = 0.1 z where x > z, y = 0.1 x where z > x. */
	TerrainMeshPart CreasedSquare()
	{
		TerrainMeshPart part;
		part.definition = "test.ter";
		const auto vertex = [](const float x, const float y, const float z)
		{
			TerrainVertex v;
			v.x = x;
			v.y = y;
			v.z = z;
			return v;
		};
		part.vertices = { vertex(0, 0, 0), vertex(100, 0, 0), vertex(100, 10, 100), vertex(0, 0, 100) };
		part.indices = { 0, 2, 1, 0, 3, 2 };	// Counter-clockwise from above (y up, z south)
		return part;
	}

	float CreaseHeight(const double x, const double z)
	{
		return static_cast<float>(x > z ? 0.1 * z : 0.1 * x);
	}

	fs::path FindXPlane()
	{
		for (const fs::path &root : SceneryEditorX::VFS::FindXPlaneInstalls(12))
		{
			if (fs::exists(root / "Resources" / "default scenery"))
				return root;
		}
		return {};
	}

	/* @brief A texture as X-Plane finds it: the path as written (any case), else the same name as .dds / .png. */
	bool TextureExists(const fs::path &file)
	{
		if (SceneryEditorX::VFS::FindCaseInsensitive(file))
			return true;
		fs::path other = file;
		const std::string extension = file.extension().string();
		if (extension == ".png" || extension == ".PNG")
			other.replace_extension(".dds");
		else if (extension == ".dds" || extension == ".DDS")
			other.replace_extension(".png");
		else
			return false;
		return SceneryEditorX::VFS::FindCaseInsensitive(other).has_value();
	}

	struct TempDir
	{
		fs::path path;
		TempDir()
		{
			path = fs::temp_directory_path() / ("sedx_terrain_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
			fs::create_directories(path);
		}
		~TempDir()
		{
			std::error_code ec;
			fs::remove_all(path, ec);
		}
		void Write(const std::string &name, const std::string &text) const
		{
			const fs::path file = path / name;
			fs::create_directories(file.parent_path());
			std::ofstream(file, std::ios::binary) << text;
		}
	};
}

// -------------------------------------------------------

TEST_CASE("LZMA and LZMA2 streams decode to what liblzma packed", "[7z]")
{
	const std::string expected = LzmaSampleText();
	std::vector<uint8_t> out(expected.size());
	std::string error;

	SECTION("LZMA")
	{
		const uint8_t properties[5] = { 0x5D, 0x00, 0x00, 0x01, 0x00 };
		REQUIRE(DecodeLzma(properties, sizeof(properties), LZMA1_SAMPLE, sizeof(LZMA1_SAMPLE), out.data(), out.size(), &error));
		CHECK(std::string(out.begin(), out.end()) == expected);
	}

	SECTION("LZMA2")
	{
		REQUIRE(DecodeLzma2(LZMA2_SAMPLE, sizeof(LZMA2_SAMPLE), out.data(), out.size(), &error));
		CHECK(std::string(out.begin(), out.end()) == expected);
	}

	SECTION("LZMA2 stored chunks")
	{
		const std::vector<uint8_t> stored = { 0x01, 0x00, 0x02, 'a', 'b', 'c', 0x02, 0x00, 0x01, 'd', 'e', 0x00 };
		std::vector<uint8_t> plain(5);
		REQUIRE(DecodeLzma2(stored.data(), stored.size(), plain.data(), plain.size(), &error));
		CHECK(std::string(plain.begin(), plain.end()) == "abcde");
	}

	SECTION("Damage is reported, not decoded past")
	{
		std::vector<uint8_t> damaged(LZMA1_SAMPLE, LZMA1_SAMPLE + sizeof(LZMA1_SAMPLE));
		damaged.resize(damaged.size() / 2);
		const uint8_t properties[5] = { 0x5D, 0x00, 0x00, 0x01, 0x00 };
		CHECK_FALSE(DecodeLzma(properties, sizeof(properties), damaged.data(), damaged.size(), out.data(), out.size(), &error));
		CHECK_FALSE(error.empty());
	}
}

TEST_CASE("A one-file 7-zip archive unpacks and checks its CRC", "[7z]")
{
	const std::string expected = LzmaSampleText();
	const uint32_t crc = Crc32(reinterpret_cast<const uint8_t *>(expected.data()), expected.size());
	CHECK(Crc32(reinterpret_cast<const uint8_t *>("123456789"), 9) == 0xCBF43926u);	// The standard check value

	const std::vector<uint8_t> archive = Build7z(LZMA1_SAMPLE, sizeof(LZMA1_SAMPLE), expected.size(), crc);
	CHECK(Is7zArchive(archive.data(), archive.size()));
	std::vector<uint8_t> out;
	std::string error;
	REQUIRE(Extract7zFirstFile(archive.data(), archive.size(), out, &error));
	CHECK(std::string(out.begin(), out.end()) == expected);

	const std::vector<uint8_t> wrongCrc = Build7z(LZMA1_SAMPLE, sizeof(LZMA1_SAMPLE), expected.size(), crc ^ 1u);
	CHECK_FALSE(Extract7zFirstFile(wrongCrc.data(), wrongCrc.size(), out, &error));
	CHECK(error.find("CRC") != std::string::npos);

	std::vector<uint8_t> truncated(archive.begin(), archive.begin() + 100);
	CHECK_FALSE(Extract7zFirstFile(truncated.data(), truncated.size(), out, &error));
}

TEST_CASE("DSF terrain patches and rasters", "[dsf][terrain]")
{
	const std::vector<uint8_t> bytes = BuildTerrainDsf();
	std::string error;

	SECTION("Overlay reads skip them")
	{
		DsfFile dsf;
		REQUIRE(ReadDsf(bytes.data(), bytes.size(), dsf, &error));
		CHECK(dsf.terrainPatches.empty());
		CHECK(dsf.rasters.empty());
		CHECK(dsf.terrainPatchCount == 4);
		CHECK(dsf.terrainTriangleCount == 7);
		REQUIRE(dsf.terrainDefinitions.size() == 2);
		CHECK(dsf.rasterDefinitions == std::vector<std::string>{ "elevation", "spr1" });
	}

	SECTION("Terrain reads keep patches, points and rasters")
	{
		DsfReadOptions options;
		options.terrain = true;
		options.rasters = true;
		DsfFile dsf;
		REQUIRE(ReadDsf(bytes.data(), bytes.size(), dsf, options, &error));
		REQUIRE(dsf.terrainPatches.size() == 4);

		const DsfTerrainPatch &base = dsf.terrainPatches[0];
		CHECK(base.definition == 1);
		CHECK(base.flags == DsfTerrainPatch::Physical);
		CHECK(base.lodStart == 0.0f);
		CHECK(base.lodEnd == -1.0f);
		CHECK(base.planes == 5);
		CHECK(base.indices.size() == 9);
		CHECK(base.GetPointCount() == 5);					// Shared points stored once
		CHECK_THAT(base.GetPoint(0)[0], WithinAbs(-122.50, 2e-5));
		CHECK_THAT(base.GetPoint(0)[2], WithinAbs(100.0, 0.01));
		CHECK_THAT(base.GetPoint(2)[2], WithinAbs(DSF_ELEVATION_FROM_RASTER, 0.01));
		CHECK_THAT(base.GetPoint(0)[3], WithinAbs(0.0, 1e-4));

		const DsfTerrainPatch &overlay = dsf.terrainPatches[1];
		CHECK(overlay.IsOverlay());
		CHECK(overlay.planes == 7);
		CHECK(overlay.indices.size() == 6);
		CHECK_THAT(overlay.GetPoint(overlay.indices[2])[5], WithinAbs(1.0, 1e-4));

		const DsfTerrainPatch &crossPool = dsf.terrainPatches[2];
		CHECK(crossPool.definition == 0);
		CHECK(crossPool.indices.size() == 3);
		const DsfTerrainPatch &range = dsf.terrainPatches[3];
		CHECK(range.definition == 0);
		CHECK(range.flags == DsfTerrainPatch::Physical);	// Carried over from the patch before
		CHECK(range.indices.size() == 3);

		REQUIRE(dsf.rasters.size() == 2);
		const DsfRaster *elevation = dsf.FindRaster("elevation");
		REQUIRE(elevation);
		CHECK(elevation->IsPostCentric());
		CHECK(elevation->width == 2);
		CHECK_THAT(elevation->Sample(-123.0, 47.0, dsf.west, dsf.south, dsf.east, dsf.north), WithinAbs(100.0, 1e-3));
		CHECK_THAT(elevation->Sample(-122.5, 47.3, dsf.west, dsf.south, dsf.east, dsf.north), WithinAbs(150.0, 1e-3));
		CHECK_THAT(elevation->Sample(-121.0, 49.0, dsf.west, dsf.south, dsf.east, dsf.north), WithinAbs(200.0, 1e-3));	// Clamped
		const DsfRaster *spring = dsf.FindRaster("spr1");
		REQUIRE(spring);
		CHECK_THAT(spring->At(0, 0), WithinAbs(80.0, 1e-4));		// 40 x scale 2
	}

	SECTION("An area keeps the triangles that reach into it")
	{
		DsfReadOptions options;
		options.terrain = true;
		options.overlays = false;
		options.areaWest = -122.505;
		options.areaEast = -122.485;
		options.areaSouth = 47.495;
		options.areaNorth = 47.515;
		DsfFile dsf;
		REQUIRE(ReadDsf(bytes.data(), bytes.size(), dsf, options, &error));
		REQUIRE(dsf.terrainPatches.size() == 4);
		CHECK(dsf.terrainPatches[0].indices.size() == 9);	// The one reaching the far point still overlaps the area

		options.areaWest = -122.2;
		options.areaEast = -122.0;
		options.areaSouth = 47.8;
		options.areaNorth = 48.0;
		REQUIRE(ReadDsf(bytes.data(), bytes.size(), dsf, options, &error));
		REQUIRE(dsf.terrainPatches.size() == 1);			// Only that triangle
		CHECK(dsf.terrainPatches[0].indices.size() == 3);
	}

	SECTION("Meshes in the local frame")
	{
		DsfReadOptions options;
		options.terrain = true;
		options.rasters = true;
		DsfFile dsf;
		REQUIRE(ReadDsf(bytes.data(), bytes.size(), dsf, options, &error));

		const GeoProjection projection(47.50, -122.50);
		TerrainBuildOptions build;
		build.datumElevation = 100.0;
		build.earthCurvature = false;
		// Around the square's south-west corner: the triangle with the far point keeps 750 m away
		build.area.rings = { { Point2{ -10.0, -10.0 }, Point2{ 10.0, -10.0 }, Point2{ 10.0, 10.0 }, Point2{ -10.0, 10.0 } } };
		build.area.margin = 100.0;
		std::vector<TerrainMeshPart> parts;
		CHECK(BuildTerrainMeshes(dsf, projection, build, parts) == 6);
		REQUIRE(parts.size() == 3);

		const TerrainMeshPart &base = parts[0];
		CHECK(base.definition == "lib/g10/terrain10/apt_tmp_wet.ter");
		CHECK_FALSE(base.overlay);
		CHECK_FALSE(base.water);
		CHECK(base.indices.size() == 6);
		const TerrainMeshPart &overlay = parts[1];
		CHECK(overlay.overlay);
		CHECK(overlay.order == 1);
		const TerrainMeshPart &water = parts[2];
		CHECK(water.water);
		CHECK(water.indices.size() == 6);
		for (const TerrainVertex &vertex : water.vertices)
			CHECK(vertex.ny == 1.0f);

		// The raster-height corner: 0.49 degrees west of the east edge -> 100 + 100 x 0.51 = 151 m
		bool sawRasterPoint = false;
		for (const TerrainVertex &vertex : base.vertices)
		{
			if (vertex.y > 1.0f)
			{
				sawRasterPoint = true;
				CHECK_THAT(vertex.y, WithinAbs(51.0, 0.05));
				CHECK(vertex.x > 700.0f);		// East
				CHECK(vertex.z < -1000.0f);		// North (z is south)
			}
			else
				CHECK_THAT(vertex.y, WithinAbs(0.0, 0.01));
			// Raster slope: rising east by 100 m a degree of longitude, so the normal leans west
			CHECK(vertex.nx < 0.0f);
			CHECK(vertex.ny > 0.99f);
		}
		CHECK(sawRasterPoint);

		// Every triangle faces up
		for (const TerrainMeshPart &part : parts)
		{
			for (size_t i = 0; i + 2 < part.indices.size(); i += 3)
			{
				const TerrainVertex &a = part.vertices[part.indices[i]], &b = part.vertices[part.indices[i + 1]], &c = part.vertices[part.indices[i + 2]];
				CHECK((b.z - a.z) * (c.x - a.x) - (b.x - a.x) * (c.z - a.z) > 0.0f);
			}
		}

		CHECK_THAT(GetTerrainCurvatureDrop(1000.0, 0.0), WithinAbs(0.0785, 1e-3));
	}
}

TEST_CASE("Terrain types (.ter)", "[assets][terrain]")
{
	const char *text =
		"A\n800\nTERRAIN\n\n"
		"BASE_TEX\t../textures12/urban/grass_park_02.png\n"
		"COMPOSITE_TEX ../textures12/urban/soil_sdry_03.png\n"
		"NORMAL_TEX 20,000000 ..\\textures10\\shared\\clumpy_turf4_NML.png\n"
		"BORDER_TEX ../textures12/border/soft.png\n"
		"COMPOSITE_NOISE ../textures12/shared/mix_outlay_2.png\n"
		"TEXTURE_LIT ../textures12/urban_FAR/uw12_FAR_LIT.dds\n"
		"LIT_FADE_PARAMS 4096 7000 15000\n"
		"LUMINANCE 600\n"
		"FAR \t5000 \t20000 0.0625 ../textures12/urban_FAR/uw12_FAR.dds\n"
		"NORMAL_METALNESS\n"
		"SUPER_ROUGHNESS 0.5\n"
		"BUMP_LEVEL\t1.0\n"
		"PROJECTED 256 256\n"
		"COMPOSITE_PROJECTED 280 280\n"
		"PROJECT_ANGLE 0 1 0 0\n"
		"COMPOSITE_PARAMS\t0.25\t-1.00\t0.45\t0.96\t0.10\t1.40\t0.7\n"
		"NO_ALPHA\n"
		"COMPOSITE_BORDERS\n"
		"AUTO_SLOPE_HILL 2328 1164 35 40 ../textures10/loveg/hill.dds\n"
		"AUTO_SLOPE_CLIFF 2832 708 57 62 ../textures10/soil/cliff.dds\n"
		"AUTO_SLOPE_HEADING\n"
		"TEXTURE_TILE 4 4 64 64 ../textures10/shared/tiles_8x4.png\n"
		"TEXTURE_TERRAIN 160.0 160.0 ../textures/terrain_TRN.png\n"
		"SNOW_ALBEDO_LUMA 1.2 1.5 0.0 0.0\n"
		"#SURFACE grass\n"
		"DECAL_LIB lib/g10/decals/ter_wet_loveg_2.dcl\n"
		"NORMAL_DECAL_PARAMS 5 0 1 0 0 0 0 detail_NML.png 2\n"
		"EXCEPTION: NULL\n";
	TerrainAsset ter;
	std::string error;
	REQUIRE(ParseTerrainAsset(text, ter, &error));
	CHECK(ter.texture == "../textures12/urban/grass_park_02.png");
	CHECK(ter.wrap);
	CHECK(ter.projected);
	CHECK(ter.projectedS == 256.0f);
	CHECK(ter.textureNormal == "../textures10/shared/clumpy_turf4_NML.png");
	CHECK(ter.normalRatio == 20.0f);						// Decimal comma
	CHECK(ter.normalMetalness);
	CHECK(ter.superRoughness == 0.5f);
	CHECK(ter.compositeTexture == "../textures12/urban/soil_sdry_03.png");
	CHECK(ter.compositeProjectedT == 280.0f);
	REQUIRE(ter.compositeParams.size() == 7);
	CHECK(ter.compositeParams[1] == -1.0f);
	CHECK(ter.compositeBorders);
	CHECK(ter.borderTexture == "../textures12/border/soft.png");
	CHECK(ter.luminance == 600.0f);
	CHECK(ter.hasLitFade);
	CHECK(ter.litFade[2] == 15000.0f);
	CHECK(ter.hasFar);
	CHECK(ter.farRatio == 0.0625f);
	CHECK(ter.farTexture == "../textures12/urban_FAR/uw12_FAR.dds");
	CHECK(ter.hill.IsUsed());
	CHECK(ter.hill.angleStart == 35.0f);
	CHECK(ter.cliff.projectedT == 708.0f);
	CHECK(ter.autoSlopeHeading);
	CHECK(ter.tiled);
	CHECK(ter.tilesX == 4);
	CHECK(ter.tilePagesX == 64);
	CHECK(ter.tileMap == "../textures10/shared/tiles_8x4.png");
	CHECK(ter.textureTerrain == "../textures/terrain_TRN.png");
	CHECK(ter.terrainScaleS == 160.0f);
	CHECK(ter.hasSnowAlbedoLuma);
	CHECK(ter.snowAlbedoLuma[1] == 1.5f);
	CHECK(ter.decalLibrary == "lib/g10/decals/ter_wet_loveg_2.dcl");
	REQUIRE(ter.decals.size() == 1);
	CHECK(ter.decals[0].normal);
	CHECK(ter.unknown == std::vector<std::string>{ "EXCEPTION:" });
	CHECK(ter.GetTextures().size() == 12);

	TerrainAsset minimal;
	CHECK_FALSE(ParseTerrainAsset("A\n800\nTERRAIN\nPROJECTED 10 10\n", minimal, &error));
	CHECK_FALSE(ParseTerrainAsset("A\n850\nDRAPED_POLYGON\nTEXTURE x.png\n", minimal, &error));
	CHECK(IsDsfWaterTerrain("terrain_Water"));
	CHECK_FALSE(IsDsfWaterTerrain("lib/g10/terrain10/apt_tmp_wet.ter"));
}

TEST_CASE("Library exports resolve by region and season", "[library][terrain]")
{
	TempDir temp;
	temp.Write("Pack/library.txt",
		"A\n800\nLIBRARY\n\n"
		"REGION_DEFINE west\n"
		"REGION_BITMAP maps/west.png\n"
		"REGION west\n"
		"EXPORT lib/test/thing.ter west/thing.ter\n"
		"REGION_DEFINE all\n"
		"REGION_RECT -180 -90 179 89\n"
		"REGION all\n"
		"EXPORT_SEASON spr,sum lib/test/thing.ter base/thing.ter\n"
		"EXPORT_SEASON fal,win lib/test/thing.ter base/fal/thing.ter\n"
		"EXPORT_RATIO 1 lib/test/ratio.obj a.obj\n"
		"EXPORT_RATIO 3 lib/test/ratio.obj b.obj\n");
	LibraryPackage package;
	package.name = "Pack";
	package.path = temp.path / "Pack";
	package.hasLibrary = true;
	LibraryIndex index;
	index.Build({ package }, LibraryIndex::BuildOptions{ false });

	const auto resolve = [&index](const std::string &path, const bool located, const double lat, const double lon, const uint8_t season, const uint64_t hash = 0)
	{
		LibraryIndex::ResolveContext context;
		context.hasLocation = located;
		context.latitude = lat;
		context.longitude = lon;
		context.season = season;
		context.placementHash = hash;
		const AssetProvider *provider = index.ResolveProvider(path, context);
		return provider ? provider->realPath.filename().string() + "@" + provider->realPath.parent_path().filename().string() : std::string();
	};

	// Without the bitmap the region covers the world, as before
	CHECK(resolve("lib/test/thing.ter", true, 10.0, 10.0, SeasonSummer) == "thing.ter@west");

	const LibraryRegion *west = index.FindRegion("west");
	REQUIRE(west);
	CHECK(west->bitmapFile.filename() == "west.png");
	index.LoadRegionBitmaps([](const fs::path &, uint32_t &width, uint32_t &height, std::vector<uint8_t> &mask)
	{
		width = 360;
		height = 180;
		mask.assign(360 * 180, 0);
		mask[static_cast<size_t>(89 - 47) * 360 + (180 - 123)] = 255;	// The +47-123 tile
		return true;
	});

	CHECK(resolve("lib/test/thing.ter", true, 47.45, -122.3, SeasonSummer) == "thing.ter@west");
	CHECK(resolve("lib/test/thing.ter", true, 10.0, 10.0, SeasonSummer) == "thing.ter@base");
	CHECK(resolve("lib/test/thing.ter", true, 10.0, 10.0, SeasonWinter) == "thing.ter@fal");
	CHECK(resolve("lib/test/thing.ter", true, 10.0, 10.0, 0) == "thing.ter@base");
	CHECK(resolve("lib/test/thing.ter", false, 0.0, 0.0, 0) == "thing.ter@west");
	CHECK(resolve("lib/test/missing.ter", true, 10.0, 10.0, 0).empty());

	// Ratios: about one in four picks the first
	int first = 0;
	for (uint64_t hash = 1; hash <= 4000; ++hash)
		first += resolve("lib/test/ratio.obj", true, 10.0, 10.0, 0, hash).starts_with("a.obj") ? 1 : 0;
	CHECK(first > 850);
	CHECK(first < 1150);
	CHECK(resolve("lib/test/ratio.obj", true, 10.0, 10.0, 0).starts_with("a.obj"));
}

TEST_CASE("Terrain area around an airport", "[terrain]")
{
	TerrainArea area;
	area.rings = { { Point2{ 0.0, 0.0 }, Point2{ 100.0, 0.0 }, Point2{ 100.0, 100.0 }, Point2{ 0.0, 100.0 } } };
	area.margin = 50.0;
	CHECK(area.Overlaps({ 10, 10 }, { 20, 10 }, { 10, 20 }));			// Inside
	CHECK(area.Overlaps({ 120, 10 }, { 130, 10 }, { 120, 20 }));		// Within the margin
	CHECK_FALSE(area.Overlaps({ 200, 10 }, { 210, 10 }, { 200, 20 }));	// Beyond it
	CHECK(area.Overlaps({ -1000, -1000 }, { 1000, -1000 }, { 0, 1000 }));	// Around the whole airport
	CHECK(area.Overlaps({ 140, -500 }, { 145, 500 }, { 500, 0 }));		// An edge passing within the margin

	TerrainArea everywhere;
	CHECK(everywhere.Overlaps({ 1e6, 1e6 }, { 1e6 + 1, 1e6 }, { 1e6, 1e6 + 1 }));

	const GeoProjection projection(47.45, -122.3);
	double west, south, east, north;
	area.GetGeoBox(projection, west, south, east, north);
	CHECK(west < -122.3);
	CHECK(east > -122.3);
	CHECK(north > 47.45);
	CHECK(south < 47.45);
	CHECK_THAT((east - west) * projection.GetMetersPerDegreeLongitude(), WithinAbs(200.0, 0.5));
}

TEST_CASE("The terrain surface: heights, rays and draping", "[terrain]")
{
	TerrainSurface surface;
	surface.Build({ CreasedSquare() });
	REQUIRE(surface.GetTriangleCount() == 2);

	SECTION("Heights")
	{
		float y = 0.0f, normal[3];
		REQUIRE(surface.HeightAt(75.0, 25.0, y, normal));
		CHECK_THAT(y, WithinAbs(2.5, 1e-4));
		REQUIRE(surface.HeightAt(25.0, 75.0, y));
		CHECK_THAT(y, WithinAbs(2.5, 1e-4));
		REQUIRE(surface.HeightAt(50.0, 50.0, y));
		CHECK_THAT(y, WithinAbs(5.0, 1e-4));
		CHECK_FALSE(surface.HeightAt(150.0, 50.0, y));
		CHECK(surface.HeightOr(150.0, 50.0, -1.0f) == -1.0f);
	}

	SECTION("Rays")
	{
		const double origin[3] = { 75.0, 100.0, 25.0 }, down[3] = { 0.0, -1.0, 0.0 };
		double distance = 0.0;
		REQUIRE(surface.Raycast(origin, down, 1000.0, distance));
		CHECK_THAT(distance, WithinAbs(97.5, 1e-3));
		const double away[3] = { 0.0, 1.0, 0.0 };
		CHECK_FALSE(surface.Raycast(origin, away, 1000.0, distance));
		const double slanted[3] = { 0.0, -1.0, 0.2 };	// 100 - t = 0.1 (25 + 0.2 t) on the face x > z
		REQUIRE(surface.Raycast(origin, slanted, 1000.0, distance));
		CHECK_THAT(distance, WithinAbs(97.5 / 1.02, 1e-3));
		const double offEdge[3] = { 0.0, -1.0, 1.0 };	// Leaves the terrain (z > 100) before coming down to it
		CHECK_FALSE(surface.Raycast(origin, offEdge, 1000.0, distance));
	}

	SECTION("A quad across the crease is cut onto both faces")
	{
		// [10, 90] x [40, 60], 5 cm above the ground, u / v from the position
		std::vector<OverlayVertex> vertices(4);
		const double corners[4][2] = { { 10, 40 }, { 90, 40 }, { 90, 60 }, { 10, 60 } };
		for (int i = 0; i < 4; ++i)
		{
			vertices[i].x = static_cast<float>(corners[i][0]);
			vertices[i].z = static_cast<float>(corners[i][1]);
			vertices[i].y = 0.05f;
			vertices[i].u = vertices[i].x / 100.0f;
			vertices[i].v = vertices[i].z / 100.0f;
		}
		std::vector<uint32_t> indices = { 0, 2, 1, 0, 3, 2 };
		const uint32_t triangles = surface.Drape(vertices, indices);
		CHECK(triangles > 2);

		double area = 0.0;
		for (size_t i = 0; i + 2 < indices.size(); i += 3)
		{
			const OverlayVertex &a = vertices[indices[i]], &b = vertices[indices[i + 1]], &c = vertices[indices[i + 2]];
			area += std::abs((b.x - a.x) * (c.z - a.z) - (b.z - a.z) * (c.x - a.x)) * 0.5;
			// Flat on the terrain: the piece's center is on it too (each piece lies on one terrain face)
			const double cx = (a.x + b.x + c.x) / 3.0, cz = (a.z + b.z + c.z) / 3.0, cy = (a.y + b.y + c.y) / 3.0;
			CHECK_THAT(cy, WithinAbs(CreaseHeight(cx, cz) + 0.05, 1e-3));
			// Still facing up, same winding
			CHECK((b.z - a.z) * (c.x - a.x) - (b.x - a.x) * (c.z - a.z) > 0.0f);
		}
		CHECK_THAT(area, WithinAbs(80.0 * 20.0, 0.01));
		for (const OverlayVertex &vertex : vertices)
		{
			CHECK_THAT(vertex.y, WithinAbs(CreaseHeight(vertex.x, vertex.z) + 0.05, 1e-3));
			CHECK_THAT(vertex.u, WithinAbs(vertex.x / 100.0, 1e-4));
			CHECK_THAT(vertex.v, WithinAbs(vertex.z / 100.0, 1e-4));
			CHECK(vertex.ny > 0.9f);
		}
	}

	SECTION("Small drapes inside one face are lifted, outside ones kept")
	{
		std::vector<OverlayVertex> vertices(6);
		const float points[6][2] = { { 70, 10 }, { 80, 20 }, { 80, 10 }, { 200, 200 }, { 210, 210 }, { 210, 200 } };
		for (int i = 0; i < 6; ++i)
		{
			vertices[i].x = points[i][0];
			vertices[i].z = points[i][1];
			vertices[i].y = 0.1f;
		}
		std::vector<uint32_t> indices = { 0, 1, 2, 3, 4, 5 };
		CHECK(surface.Drape(vertices, indices) == 2);
		CHECK_THAT(vertices[indices[0]].y, WithinAbs(CreaseHeight(70, 10) + 0.1, 1e-4));
		CHECK_THAT(vertices[indices[3]].y, WithinAbs(0.1, 1e-6));
	}

	SECTION("Lifting 3D geometry")
	{
		std::vector<OverlayVertex> wall(2);
		wall[0].x = 75.0f;
		wall[0].z = 25.0f;
		wall[1] = wall[0];
		wall[1].y = 6.0f;
		surface.Lift(wall);
		CHECK_THAT(wall[0].y, WithinAbs(2.5, 1e-4));
		CHECK_THAT(wall[1].y, WithinAbs(8.5, 1e-4));
	}

	SECTION("Clipped drapes end where the terrain does")
	{
		// [50, 150] x [20, 40]: half of it over the terrain (x <= 100); and a triangle wholly off it
		std::vector<OverlayVertex> vertices(7);
		const float points[7][2] = { { 50, 20 }, { 150, 20 }, { 150, 40 }, { 50, 40 }, { 200, 200 }, { 210, 210 }, { 210, 200 } };
		for (int i = 0; i < 7; ++i)
		{
			vertices[i].x = points[i][0];
			vertices[i].z = points[i][1];
			vertices[i].y = 0.01f;
		}
		std::vector<uint32_t> kept = { 0, 2, 1, 0, 3, 2, 4, 5, 6 };
		std::vector<OverlayVertex> keptVertices = vertices;
		const uint32_t unclipped = surface.Drape(keptVertices, kept);

		std::vector<uint32_t> indices = { 0, 2, 1, 0, 3, 2, 4, 5, 6 };
		const uint32_t clipped = surface.Drape(vertices, indices, true);
		CHECK(clipped > 0);
		CHECK(clipped < unclipped);
		double area = 0.0;
		for (size_t i = 0; i + 2 < indices.size(); i += 3)
		{
			const OverlayVertex &a = vertices[indices[i]], &b = vertices[indices[i + 1]], &c = vertices[indices[i + 2]];
			area += std::abs((b.x - a.x) * (c.z - a.z) - (b.z - a.z) * (c.x - a.x)) * 0.5;
			for (const OverlayVertex *vertex : { &a, &b, &c })
			{
				CHECK(vertex->x <= 100.0f + 1e-3f);
				CHECK_THAT(vertex->y, WithinAbs(CreaseHeight(vertex->x, vertex->z) + 0.01, 1e-3));
			}
		}
		CHECK_THAT(area, WithinAbs(50.0 * 20.0, 0.01));
	}
}

TEST_CASE("Every X-Plane 12 default .ter parses with its textures", "[assets][terrain][xplane-install][library-sweep]")
{
	const fs::path xplane = FindXPlane();
	if (xplane.empty())
	{
		WARN("No X-Plane 12 install found (x-plane_install_12.txt): skipped");
		return;
	}
	// The .ter files the default libraries export (X-Plane 9's terrain/ folder ships files nothing exports, some naming
	// textures that aren't installed)
	const fs::path defaultScenery = xplane / "Resources" / "default scenery";
	std::set<std::string> exported;
	for (const auto &package : fs::directory_iterator(defaultScenery))
	{
		LibraryFile library;
		if (!package.is_directory() || !LoadLibraryFile(package.path() / "library.txt", library))
			continue;
		for (const LibraryExport &entry : library.exports)
		{
			std::string real = entry.realPath;
			std::ranges::replace(real, '\\', '/');
			exported.insert(SceneryEditorX::VFS::ToUtf8((package.path() / fs::path(real)).lexically_normal()));
		}
	}

	uint32_t files = 0, failures = 0, missing = 0, deadMissing = 0;
	std::string failed, unresolved;
	std::set<std::string> unknown;
	for (const auto &entry : fs::recursive_directory_iterator(defaultScenery))
	{
		if (!entry.is_regular_file() || (entry.path().extension() != ".ter" && entry.path().extension() != ".TER"))
			continue;
		++files;
		TerrainAsset ter;
		std::string error;
		if (!LoadTerrainAsset(entry.path(), ter, &error))
		{
			if (failures++ < 5)
				failed += error + "\n";
			continue;
		}
		unknown.insert(ter.unknown.begin(), ter.unknown.end());
		const bool isExported = exported.contains(SceneryEditorX::VFS::ToUtf8(entry.path().lexically_normal()));
		for (const std::string &texture : ter.GetTextures())
		{
			if (TextureExists(entry.path().parent_path() / texture))
				continue;
			if (!isExported)
				++deadMissing;
			else if (missing++ < 8)
				unresolved += SceneryEditorX::VFS::ToUtf8(entry.path()) + ": " + texture + "\n";
		}
	}
	CHECK(files > 9000);
	CHECK(exported.size() > 9000);
	INFO(failed);
	CHECK(failures == 0);
	INFO(unresolved);
	CHECK(missing == 0);
	if (deadMissing > 0)
		WARN(std::to_string(deadMissing) + " textures of .ter files no library exports aren't installed");
	// A stray "EXCEPTION: NULL" line ships in two files; nothing else is unknown
	for (const std::string &key : unknown)
		CHECK(key == "EXCEPTION:");
}

TEST_CASE("DSF rasters sampled at the nearest cell", "[dsf][terrain]")
{
	// Season days 360 | 10 side by side: blending them gives a day that is in neither season
	DsfRaster raster;
	raster.flags = 2 | 4;	// Unsigned, post-centric
	raster.width = 2;
	raster.height = 2;
	raster.values = { 360.0f, 10.0f, 350.0f, 20.0f };	// Row 0 is the south
	CHECK_THAT(raster.Sample(0.4, 0.0, 0.0, 0.0, 1.0, 1.0), WithinAbs(220.0, 1e-3));
	CHECK(raster.SampleNearest(0.4, 0.0, 0.0, 0.0, 1.0, 1.0) == 360.0f);
	CHECK(raster.SampleNearest(0.6, 0.0, 0.0, 0.0, 1.0, 1.0) == 10.0f);
	CHECK(raster.SampleNearest(0.6, 0.9, 0.0, 0.0, 1.0, 1.0) == 20.0f);
	CHECK(raster.SampleNearest(-5.0, 5.0, 0.0, 0.0, 1.0, 1.0) == 350.0f);	// Clamped to the edges

	// Pixel-centric: four cells, each the nearest over its quarter
	raster.flags = 2;
	CHECK(raster.SampleNearest(0.2, 0.2, 0.0, 0.0, 1.0, 1.0) == 360.0f);
	CHECK(raster.SampleNearest(0.7, 0.2, 0.0, 0.0, 1.0, 1.0) == 10.0f);
	CHECK(raster.SampleNearest(0.2, 0.7, 0.0, 0.0, 1.0, 1.0) == 350.0f);

	DsfRaster empty;
	CHECK(empty.SampleNearest(0.5, 0.5, 0.0, 0.0, 1.0, 1.0) == 0.0f);
}

TEST_CASE("The test airports' seasons from X-Plane 12's Global Scenery rasters", "[dsf][terrain][xplane-install]")
{
	using namespace SceneryEditorX::Environment;
	const fs::path xplane = FindXPlane();
	const fs::path tiles = xplane / "Global Scenery" / "X-Plane 12 Global Scenery" / "Earth nav data";
	if (xplane.empty() || !fs::exists(tiles / "+50-010" / "+53-007.dsf"))
	{
		WARN("No X-Plane 12 Global Scenery tile +53-007 found: skipped");
		return;
	}

	// The datum cell's ranges, as SceneTerrain::ReadSeasonRasters samples them
	const auto ranges = [&tiles](const char *folder, const char *file, const double latitude, const double longitude) -> std::optional<DsfSeasonRanges>
	{
		DsfReadOptions options;
		options.overlays = false;
		options.rasters = true;
		options.rasterNames.assign(DSF_SEASON_RASTERS.begin(), DSF_SEASON_RASTERS.end());
		DsfFile dsf;
		std::string error;
		REQUIRE(LoadDsf(tiles / folder / file, dsf, options, &error));
		REQUIRE(dsf.rasters.size() == 8);
		std::array<float, 8> days{};
		for (size_t i = 0; i < days.size(); ++i)
		{
			const DsfRaster *raster = dsf.FindRaster(DSF_SEASON_RASTERS[i]);
			REQUIRE(raster);
			CHECK(raster->width == 128);
			days[i] = raster->SampleNearest(longitude, latitude, dsf.west, dsf.south, dsf.east, dsf.north);
		}
		return MakeDsfSeasonRanges(days);
	};

	// EIDW: spring until July 9, summer from July 10 to October 1
	const std::optional<DsfSeasonRanges> eidw = ranges("+50-010", "+53-007.dsf", 53.4213, -6.2701);
	REQUIRE(eidw);
	CHECK(ComputeSeason(166, 53.4213, &*eidw).season == Season::Spring);		// 2026-06-15
	CHECK(ComputeSeason(258, 53.4213, &*eidw).season == Season::Summer);		// 2026-09-15
	CHECK(ComputeSeason(15, 53.4213, &*eidw).season == Season::Winter);
	CHECK(ComputeSeason(196, 53.4213, &*eidw).season == Season::Summer);

	// OMDB: no seasons (summer in January too); RCTP: summer but for January 4-23
	if (fs::exists(tiles / "+20+050" / "+25+055.dsf"))
	{
		const std::optional<DsfSeasonRanges> omdb = ranges("+20+050", "+25+055.dsf", 25.2528, 55.3644);
		REQUIRE(omdb);
		CHECK(omdb->valid == std::array<bool, 4>{ false, true, false, false });
		CHECK(ComputeSeason(15, 25.2528, &*omdb).season == Season::Summer);
	}
	if (fs::exists(tiles / "+20+120" / "+25+121.dsf"))
	{
		const std::optional<DsfSeasonRanges> rctp = ranges("+20+120", "+25+121.dsf", 25.0777, 121.2328);
		REQUIRE(rctp);
		CHECK(ComputeSeason(15, 25.0777, &*rctp).season == Season::Winter);
		CHECK(ComputeSeason(30, 25.0777, &*rctp).season == Season::Summer);
	}
}

TEST_CASE("The terrain around KSEA from X-Plane 12's Global Scenery", "[dsf][terrain][xplane-install]")
{
	const fs::path xplane = FindXPlane();
	const fs::path tile = xplane / "Global Scenery" / "X-Plane 12 Global Scenery" / "Earth nav data" / "+40-130" / "+47-123.dsf";
	if (xplane.empty() || !fs::exists(tile))
	{
		WARN("No X-Plane 12 Global Scenery tile +47-123 found: skipped");
		return;
	}
	CHECK(IsDsfCompressed(tile));

	// KSEA's runways plus a mile
	const GeoProjection projection(47.449, -122.309);
	TerrainBuildOptions build;
	build.area.rings = { { projection.ToLocal(47.4638, -122.3180), projection.ToLocal(47.4638, -122.3020), projection.ToLocal(47.4300, -122.3020), projection.ToLocal(47.4300, -122.3180) } };
	DsfReadOptions options;
	options.overlays = false;
	options.terrain = true;
	options.rasters = true;
	options.rasterNames = { "elevation", "sea_level", "spr1", "spr2", "sum1", "sum2", "fal1", "fal2", "win1", "win2" };
	build.area.GetGeoBox(projection, options.areaWest, options.areaSouth, options.areaEast, options.areaNorth);

	DsfFile dsf;
	std::string error;
	REQUIRE(LoadDsf(tile, dsf, options, &error));
	CHECK(dsf.terrainPatchCount > 10000);
	CHECK(dsf.terrainPatches.size() > 50);
	CHECK(dsf.terrainPatches.size() < 1000);
	CHECK(dsf.rasters.size() == 10);
	const DsfRaster *elevation = dsf.FindRaster("elevation");
	REQUIRE(elevation);
	CHECK(elevation->width == 1201);
	CHECK_THAT(elevation->Sample(-122.309, 47.449, dsf.west, dsf.south, dsf.east, dsf.north), WithinAbs(120.0, 15.0));

	build.datumElevation = elevation->Sample(-122.309, 47.449, dsf.west, dsf.south, dsf.east, dsf.north);
	build.waterCellSize = 64.0;
	std::vector<TerrainMeshPart> parts;
	CHECK(BuildTerrainMeshes(dsf, projection, build, parts) > 1000);
	std::set<std::string> definitions;
	for (const TerrainMeshPart &part : parts)
		definitions.insert(part.definition);
	CHECK(definitions.contains("lib/g10/terrain10/apt_tmp_wet.ter"));

	// The lakes around the field: cut into 64 m cells, each vertex's t the floor's depth in meters from the bathymetry
	// (sea_level raster) and the DSF's depth ratio, s its fetch ratio
	const auto water = std::ranges::find_if(parts, [](const TerrainMeshPart &part) { return part.water; });
	REQUIRE(water != parts.end());
	float deepest = 0.0f;
	for (size_t t = 0; t + 2 < water->indices.size(); t += 3)
	{
		const TerrainVertex &a = water->vertices[water->indices[t]], &b = water->vertices[water->indices[t + 1]], &c = water->vertices[water->indices[t + 2]];
		CHECK(std::max({ a.x, b.x, c.x }) - std::min({ a.x, b.x, c.x }) <= 64.0f + 1e-3f);
		CHECK(std::max({ a.z, b.z, c.z }) - std::min({ a.z, b.z, c.z }) <= 64.0f + 1e-3f);
	}
	for (const TerrainVertex &vertex : water->vertices)
	{
		CHECK(vertex.t >= 0.0f);
		CHECK(vertex.t < 200.0f);
		CHECK(vertex.s >= 0.0f);
		CHECK(vertex.s <= 1.0f);
		deepest = std::max(deepest, vertex.t);
	}
	CHECK(deepest > 0.5f);

	TerrainSurface surface;
	surface.Build(parts);
	float y = 1e6f;
	REQUIRE(surface.HeightAt(0.0, 0.0, y));
	CHECK_THAT(y, WithinAbs(0.0, 3.0));
	// The runway falls to the south: 16L's threshold stands above 34R's
	const Point2 north16 = projection.ToLocal(47.4638, -122.3079), south34 = projection.ToLocal(47.4314, -122.3080);
	CHECK(surface.HeightOr(north16.x, north16.z) > surface.HeightOr(south34.x, south34.z) + 5.0f);
}

TEST_CASE("KSEA's tile scenery within a mile, and X-Plane 12's beaches", "[dsf][terrain][xplane-install]")
{
	const fs::path xplane = FindXPlane();
	const fs::path tile = xplane / "Global Scenery" / "X-Plane 12 Global Scenery" / "Earth nav data" / "+40-130" / "+47-123.dsf";
	if (xplane.empty() || !fs::exists(tile))
	{
		WARN("No X-Plane 12 Global Scenery tile +47-123 found: skipped");
		return;
	}

	// Both beach files parse, with pieces for the types the tile uses
	for (const char *name : { "1200 beaches/coastlines.bch", "900 beaches/beach.bch" })
	{
		BeachAsset beach;
		std::string error;
		const bool loaded = LoadBeachAsset(xplane / "Resources" / "default scenery" / name, beach, &error);
		INFO(name << ": " << error);
		REQUIRE(loaded);
		CHECK(beach.GetTiles(5).size() >= 3);
		CHECK_FALSE(beach.transitions.empty());
	}

	const GeoProjection projection(47.449, -122.309);
	TerrainArea area;
	area.rings = { { projection.ToLocal(47.4638, -122.3180), projection.ToLocal(47.4638, -122.3020), projection.ToLocal(47.4300, -122.3020), projection.ToLocal(47.4300, -122.3180) } };
	DsfReadOptions options;
	options.overlaysOnlyInBaseMesh = true;
	area.GetGeoBox(projection, options.areaWest, options.areaSouth, options.areaEast, options.areaNorth);
	DsfFile dsf;
	std::string error;
	REQUIRE(LoadDsf(tile, dsf, options, &error));
	CHECK(dsf.roads.size() > 1000);
	CHECK(dsf.roads.size() < 20000);
	std::map<std::string, size_t> kinds;
	for (const DsfPolygonPlacement &polygon : dsf.polygons)
	{
		const std::string &definition = dsf.polygonDefinitions[polygon.definition];
		++kinds[definition.substr(definition.find_last_of('.') + 1)];
	}
	CHECK(kinds["for"] > 50);
	CHECK(kinds["bch"] > 0);
	CHECK(dsf.polygons.size() < 5000);	// Of 78,000 in the tile

	// The Global Airports overlay over it: passed over, its exclusion zones read
	const fs::path airports = xplane / "Global Scenery" / "Global Airports" / "Earth nav data" / "+40-130" / "+47-123.dsf";
	if (fs::exists(airports))
	{
		DsfFile overlay;
		REQUIRE(LoadDsf(airports, overlay, options, &error));
		CHECK(overlay.IsOverlay());
		CHECK(overlay.objects.empty());
		CHECK(overlay.polygons.empty());
		CHECK(overlay.GetExclusionZones().size() > 10);
	}
}

TEST_CASE("DSF exclusion zones", "[dsf][terrain]")
{
	DsfExclusion zone;
	SECTION("Boxes")
	{
		REQUIRE(ParseDsfExclusion("sim/exclude_for", "-122.320000/47.430000/-122.300000/47.460000", zone));
		CHECK(zone.kind == "for");
		CHECK(zone.west == -122.32);
		CHECK(zone.north == 47.46);
		CHECK(zone.polygon.empty());
		CHECK(zone.Contains(-122.31, 47.45));
		CHECK_FALSE(zone.Contains(-122.33, 47.45));
		CHECK_FALSE(zone.Contains(-122.31, 47.47));
		// Corners in any order
		REQUIRE(ParseDsfExclusion("sim/exclude_net", "-122.30/47.46/-122.32/47.43", zone));
		CHECK(zone.west == -122.32);
		CHECK(zone.south == 47.43);
	}
	SECTION("X-Plane 12 polygons, either separator")
	{
		// A triangle: west, south-east, north-east corners of its box
		for (const char *value : { "0/0/2/2;0/1,2/0,2/2", "0/0/2/2;0,1,2,0,2,2" })
		{
			REQUIRE(ParseDsfExclusion("sim/exclude_obj", value, zone));
			REQUIRE(zone.polygon.size() == 3);
			CHECK(zone.Contains(1.8, 1.0));
			CHECK_FALSE(zone.Contains(0.2, 0.2));	// In the box, not the triangle
			CHECK_FALSE(zone.Contains(3.0, 1.0));
		}
	}
	SECTION("Not zones")
	{
		CHECK_FALSE(ParseDsfExclusion("sim/west", "-123", zone));
		CHECK_FALSE(ParseDsfExclusion("sim/exclude_", "0/0/1/1", zone));
		CHECK_FALSE(ParseDsfExclusion("sim/exclude_for", "0/0/1", zone));
		CHECK_FALSE(ParseDsfExclusion("sim/exclude_for", "a/0/1/1", zone));
		CHECK_FALSE(ParseDsfExclusion("sim/exclude_for", "0/0/1/1;0/0,1", zone));
	}
	SECTION("From a tile's properties")
	{
		DsfFile dsf;
		dsf.properties = { { "sim/overlay", "1" }, { "sim/exclude_fac", "0/0/1/1" }, { "sim/exclude_for", "bad" }, { "sim/exclude_bch", "0/0/1/1;0/0,1/0,1/1" } };
		const std::vector<DsfExclusion> zones = dsf.GetExclusionZones();
		REQUIRE(zones.size() == 2);
		CHECK(zones[0].kind == "fac");
		CHECK(zones[1].kind == "bch");
		CHECK(zones[1].polygon.size() == 3);
	}
}

TEST_CASE("DSF overlay placements limited to an area", "[dsf][terrain]")
{
	// A base tile with scenery near its south-west corner, its north-east corner, and a forest around the south-west
	// area with no point in it
	DsfFile written;
	written.properties = { { "sim/west", "-123" }, { "sim/east", "-122" }, { "sim/north", "46" }, { "sim/south", "45" } };
	written.objectDefinitions = { "lib/a.obj" };
	written.polygonDefinitions = { "lib/trees.for", "lib/coast.bch" };
	written.networkDefinitions = { "lib/g10/roads.net" };
	const auto point = [](const double longitude, const double latitude, const double third = 0.0)
	{
		DsfPoint p;
		p.planes = 3;
		p.v[0] = longitude;
		p.v[1] = latitude;
		p.v[2] = third;
		return p;
	};
	DsfObjectPlacement near, far;
	near.longitude = -122.9;
	near.latitude = 45.1;
	far.longitude = -122.2;
	far.latitude = 45.8;
	written.objects = { near, far };
	DsfPolygonPlacement forestNear, forestFar, forestAround, beach;
	forestNear.definition = forestFar.definition = forestAround.definition = 0;
	forestNear.parameter = forestFar.parameter = forestAround.parameter = 255;
	forestNear.windings = { { point(-122.91, 45.09), point(-122.89, 45.09), point(-122.89, 45.11) } };
	forestFar.windings = { { point(-122.21, 45.79), point(-122.19, 45.79), point(-122.19, 45.81) } };
	forestAround.windings = { { point(-122.99, 45.01), point(-122.80, 45.01), point(-122.80, 45.19), point(-122.99, 45.19) } };
	beach.definition = 1;
	beach.parameter = 1;
	beach.planes = 3;
	beach.windings = { { point(-122.905, 45.095, 5), point(-122.895, 45.095, 5), point(-122.895, 45.105, 5) } };
	written.polygons = { forestNear, forestFar, forestAround, beach };
	for (auto &polygon : written.polygons)
		polygon.planes = polygon.definition == 1 ? 3 : 2;
	DsfRoadSegment roadNear, roadFar;
	roadNear.startJunction = 1;
	roadNear.endJunction = 2;
	roadNear.subtype = 50;
	roadNear.points = { { -122.91, 45.1 }, { -122.89, 45.1 } };
	roadFar = roadNear;
	roadFar.startJunction = 3;
	roadFar.endJunction = 4;
	roadFar.points = { { -122.21, 45.8 }, { -122.19, 45.8 } };
	written.roads = { roadNear, roadFar };

	std::vector<uint8_t> bytes;
	std::string error;
	REQUIRE(WriteDsf(written, bytes, &error));

	DsfReadOptions options;
	options.areaWest = -122.95;
	options.areaEast = -122.85;
	options.areaSouth = 45.05;
	options.areaNorth = 45.15;

	SECTION("Without an area, everything")
	{
		DsfFile all;
		REQUIRE(ReadDsf(bytes.data(), bytes.size(), all, &error));
		CHECK(all.objects.size() == 2);
		CHECK(all.polygons.size() == 4);
		CHECK(all.roads.size() == 2);
	}
	SECTION("With one, what reaches into it (polygons whole)")
	{
		DsfFile kept;
		REQUIRE(ReadDsf(bytes.data(), bytes.size(), kept, options, &error));
		REQUIRE(kept.objects.size() == 1);
		CHECK(kept.objects[0].longitude == Catch::Approx(-122.9).margin(1e-6));
		REQUIRE(kept.polygons.size() == 3);	// Near, around (in file order), the beach
		CHECK(kept.polygons[0].windings[0].size() == 3);
		CHECK(kept.polygons[1].windings[0].size() == 4);
		CHECK(kept.polygons[2].definition == 1);
		CHECK(kept.polygons[2].windings[0][0].v[2] == 5.0);	// The beach type plane
		REQUIRE(kept.roads.size() == 1);
		CHECK(kept.roads[0].startJunction == 1);
	}
	SECTION("Only a base mesh's overlays")
	{
		DsfReadOptions base = options;
		base.overlaysOnlyInBaseMesh = true;
		DsfFile kept;
		REQUIRE(ReadDsf(bytes.data(), bytes.size(), kept, base, &error));
		CHECK(kept.objects.size() == 1);

		DsfFile overlay = written;
		overlay.properties.emplace_back("sim/overlay", "1");
		overlay.properties.emplace_back("sim/exclude_for", "-122.95/45.05/-122.85/45.15");
		std::vector<uint8_t> overlayBytes;
		REQUIRE(WriteDsf(overlay, overlayBytes, &error));
		DsfFile skipped;
		REQUIRE(ReadDsf(overlayBytes.data(), overlayBytes.size(), skipped, base, &error));
		CHECK(skipped.IsOverlay());
		CHECK(skipped.objects.empty());
		CHECK(skipped.polygons.empty());
		CHECK(skipped.roads.empty());
		CHECK(skipped.GetExclusionZones().size() == 1);
	}
}

// -------------------------------------------------------

TEST_CASE("Water meshes cut into cells for tessellation, and the floor's depth", "[terrain][water]")
{
	// X-Plane 12's depth ratio: 0 the shore (the ground), 1 the bathymetry
	CHECK_THAT(WaterFloorDepth(10.0, -40.0, 0.5), WithinAbs(25.0, 1e-9));
	CHECK(WaterFloorDepth(10.0, -40.0, 0.0) == 0.0);
	CHECK(WaterFloorDepth(10.0, 12.0, 1.0) == 0.0);		// Bathymetry above the surface: no water under it
	CHECK_THAT(WaterFloorDepth(10.0, -40.0, 3.0), WithinAbs(50.0, 1e-9));

	// A 200 x 150 m rectangle in two triangles reaching into negative x, plus a triangle sharing an edge with it
	TerrainMeshPart part;
	part.water = true;
	const auto vertex = [](const float x, const float z)
	{
		TerrainVertex v;
		v.x = x;
		v.z = z;
		v.y = 0.01f * x;	// A plane: interpolation keeps it
		v.s = x / 200.0f;
		v.t = z / 150.0f;
		return v;
	};
	part.vertices = { vertex(-40.0f, 0.0f), vertex(160.0f, 0.0f), vertex(160.0f, 150.0f), vertex(-40.0f, 150.0f), vertex(260.0f, 70.0f) };
	part.indices = { 0, 2, 1, 0, 3, 2, 1, 2, 4 };	// Counter-clockwise from above (x east, z south)
	const auto area = [](const TerrainVertex &a, const TerrainVertex &b, const TerrainVertex &c)
	{
		return 0.5 * ((static_cast<double>(b.z) - a.z) * (static_cast<double>(c.x) - a.x) - (static_cast<double>(b.x) - a.x) * (static_cast<double>(c.z) - a.z));
	};
	double before = 0.0;
	for (size_t t = 0; t < part.indices.size(); t += 3)
		before += area(part.vertices[part.indices[t]], part.vertices[part.indices[t + 1]], part.vertices[part.indices[t + 2]]);
	REQUIRE(before > 0.0);

	constexpr double cell = 64.0;
	const uint32_t triangles = CutTerrainAlongGrid(part, cell);
	CHECK(triangles == part.indices.size() / 3);
	CHECK(triangles > 20u);

	double after = 0.0;
	std::map<std::pair<uint32_t, uint32_t>, int> edges;
	for (size_t t = 0; t < part.indices.size(); t += 3)
	{
		const TerrainVertex &a = part.vertices[part.indices[t]], &b = part.vertices[part.indices[t + 1]], &c = part.vertices[part.indices[t + 2]];
		const double piece = area(a, b, c);
		CHECK(piece > 0.0);		// Winding kept, no slivers
		after += piece;
		// Inside one cell
		const double minX = std::min({ a.x, b.x, c.x }), maxX = std::max({ a.x, b.x, c.x });
		const double minZ = std::min({ a.z, b.z, c.z }), maxZ = std::max({ a.z, b.z, c.z });
		CHECK(std::floor(minX / cell) == std::floor((maxX - 1e-3) / cell));
		CHECK(std::floor(minZ / cell) == std::floor((maxZ - 1e-3) / cell));
		for (int k = 0; k < 3; ++k)
		{
			const uint32_t i0 = part.indices[t + k], i1 = part.indices[t + (k + 1) % 3];
			++edges[{ std::min(i0, i1), std::max(i0, i1) }];
		}
	}
	CHECK_THAT(after, WithinAbs(before, 1e-2));

	// No T-junctions: an edge used once lies on the outline (the rectangle's edges, the third triangle's two outer edges)
	const auto onOutline = [](const TerrainVertex &a, const TerrainVertex &b)
	{
		const auto onLine = [](const TerrainVertex &p, const float x0, const float z0, const float x1, const float z1)
		{
			const double cross = (static_cast<double>(x1) - x0) * (static_cast<double>(p.z) - z0) - (static_cast<double>(z1) - z0) * (static_cast<double>(p.x) - x0);
			return std::abs(cross) / std::hypot(x1 - x0, z1 - z0) < 1e-3;
		};
		const float lines[5][4] = { { -40, 0, 160, 0 }, { -40, 150, 160, 150 }, { -40, 0, -40, 150 }, { 160, 0, 260, 70 }, { 160, 150, 260, 70 } };
		for (const auto &line : lines)
		{
			if (onLine(a, line[0], line[1], line[2], line[3]) && onLine(b, line[0], line[1], line[2], line[3]))
				return true;
		}
		return false;
	};
	for (const auto &[edge, uses] : edges)
	{
		INFO("edge " << edge.first << " - " << edge.second);
		CHECK(uses <= 2);
		if (uses == 1)
			CHECK(onOutline(part.vertices[edge.first], part.vertices[edge.second]));
	}

	// Attributes interpolate linearly
	for (const TerrainVertex &v : part.vertices)
	{
		if (v.x <= 160.0f)
		{
			CHECK_THAT(v.s, WithinAbs(v.x / 200.0f, 1e-4));
			CHECK_THAT(v.t, WithinAbs(v.z / 150.0f, 1e-4));
		}
		CHECK_THAT(v.y, WithinAbs(0.01f * v.x, 1e-3));
	}

	// No cut asked: as it was
	TerrainMeshPart whole;
	whole.vertices = { vertex(0.0f, 0.0f), vertex(500.0f, 0.0f), vertex(0.0f, 500.0f) };
	whole.indices = { 0, 2, 1 };
	CHECK(CutTerrainAlongGrid(whole, 0.0) == 1u);
	CHECK(whole.vertices.size() == 3);
}

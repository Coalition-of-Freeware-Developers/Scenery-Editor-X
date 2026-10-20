/**
 * -------------------------------------------------------
 * Scenery Editor X - Tests
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * FacadeWriterTests.cpp
 * -------------------------------------------------------
 * Writing facades: parse -> write -> parse gives the same asset for type 1 and type 2 facades (synthetic ones and the
 * X-Plane 12 library's own when an install is found), type 1 facades authored from a texture (panel edges, roofs) and
 * type 2 sidewalks / curbs generated from a cross-section, drawn by the overlay builder.
 * -------------------------------------------------------
 */
#include <catch2/catch_all.hpp>
#include <SceneryEditorX/filesystem/vfs.h>
#include <SceneryEditorX/xplane/xp_assets.h>
#include <SceneryEditorX/xplane/xp_dsf.h>
#include <SceneryEditorX/xplane/xp_facade.h>
#include <SceneryEditorX/xplane/xp_facade_authoring.h>
#include <SceneryEditorX/xplane/xp_facade_writer.h>
#include <SceneryEditorX/xplane/xp_overlay.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace SceneryEditorX::XPlane;
namespace fs = std::filesystem;

namespace
{
	// -------------------------------------------------------
	// Comparing assets field by field: the first difference, empty when the same

#define SEDX_SAME(a, b, what) do { if (!((a) == (b))) return std::string(what); } while (0)

	std::string Diff(const FacadeAsset::WallFilter &a, const FacadeAsset::WallFilter &b)
	{
		SEDX_SAME(a.minWidth, b.minWidth, "minWidth");
		SEDX_SAME(a.maxWidth, b.maxWidth, "maxWidth");
		SEDX_SAME(a.minHeading, b.minHeading, "minHeading");
		SEDX_SAME(a.maxHeading, b.maxHeading, "maxHeading");
		return {};
	}

	template<typename T, typename F>
	std::string DiffList(const std::vector<T> &a, const std::vector<T> &b, const std::string &what, F &&diff)
	{
		if (a.size() != b.size())
			return what + " count " + std::to_string(a.size()) + " vs " + std::to_string(b.size());
		for (size_t i = 0; i < a.size(); ++i)
		{
			const std::string d = diff(a[i], b[i]);
			if (!d.empty())
				return what + "[" + std::to_string(i) + "]." + d;
		}
		return {};
	}

	template<typename T>
	std::string DiffValues(const std::vector<T> &a, const std::vector<T> &b, const std::string &what)
	{
		if (a.size() != b.size())
			return what + " count " + std::to_string(a.size()) + " vs " + std::to_string(b.size());
		for (size_t i = 0; i < a.size(); ++i)
		{
			if (!(a[i] == b[i]))
				return what + "[" + std::to_string(i) + "]";
		}
		return {};
	}

#define SEDX_SAME_LIST(a, b, what, fn) do { const std::string d_ = DiffList(a, b, what, fn); if (!d_.empty()) return d_; } while (0)
#define SEDX_SAME_VALUES(a, b, what) do { const std::string d_ = DiffValues(a, b, what); if (!d_.empty()) return d_; } while (0)

	std::string Diff(const FacadeAsset::Wall &a, const FacadeAsset::Wall &b)
	{
		SEDX_SAME(a.name, b.name, "name");
		SEDX_SAME(a.minWidth, b.minWidth, "minWidth");
		SEDX_SAME(a.maxWidth, b.maxWidth, "maxWidth");
		SEDX_SAME_LIST(a.filters, b.filters, "filters", [](const auto &x, const auto &y) { return Diff(x, y); });
		SEDX_SAME(a.scaleS, b.scaleS, "scaleS");
		SEDX_SAME(a.scaleT, b.scaleT, "scaleT");
		SEDX_SAME(a.roofSlope, b.roofSlope, "roofSlope");
		SEDX_SAME(a.slant, b.slant, "slant");
		SEDX_SAME(a.basement, b.basement, "basement");
		SEDX_SAME(a.textureWidth, b.textureWidth, "textureWidth");
		SEDX_SAME(a.textureHeight, b.textureHeight, "textureHeight");
		SEDX_SAME_VALUES(a.leftPanels, b.leftPanels, "leftPanels");
		SEDX_SAME_VALUES(a.centerPanels, b.centerPanels, "centerPanels");
		SEDX_SAME_VALUES(a.rightPanels, b.rightPanels, "rightPanels");
		SEDX_SAME_VALUES(a.bottomFloors, b.bottomFloors, "bottomFloors");
		SEDX_SAME_VALUES(a.middleFloors, b.middleFloors, "middleFloors");
		SEDX_SAME_VALUES(a.topFloors, b.topFloors, "topFloors");
		return {};
	}

	std::string Diff(const FacadeAsset::Segment &a, const FacadeAsset::Segment &b)
	{
		SEDX_SAME(a.width, b.width, "width");
		SEDX_SAME_LIST(a.meshes, b.meshes, "meshes", [](const FacadeAsset::Mesh &x, const FacadeAsset::Mesh &y) -> std::string
		{
			SEDX_SAME(x.group, y.group, "group");
			SEDX_SAME(x.lodFar, y.lodFar, "lodFar");
			SEDX_SAME(x.cuts, y.cuts, "cuts");
			SEDX_SAME_VALUES(x.positions, y.positions, "positions");
			SEDX_SAME_VALUES(x.normals, y.normals, "normals");
			SEDX_SAME_VALUES(x.uvs, y.uvs, "uvs");
			SEDX_SAME_VALUES(x.indices, y.indices, "indices");
			return {};
		});
		SEDX_SAME_LIST(a.attachments, b.attachments, "attachments", [](const FacadeAsset::Attachment &x, const FacadeAsset::Attachment &y) -> std::string
		{
			SEDX_SAME(x.object, y.object, "object");
			SEDX_SAME(x.x, y.x, "x");
			SEDX_SAME(x.y, y.y, "y");
			SEDX_SAME(x.z, y.z, "z");
			SEDX_SAME(x.heading, y.heading, "heading");
			SEDX_SAME(x.draped, y.draped, "draped");
			SEDX_SAME(x.showLo, y.showLo, "showLo");
			SEDX_SAME(x.showHi, y.showHi, "showHi");
			return {};
		});
		return {};
	}

	std::string Diff(const FacadeAsset::Floor &a, const FacadeAsset::Floor &b)
	{
		SEDX_SAME(a.name, b.name, "name");
		SEDX_SAME_LIST(a.segments, b.segments, "segments", [](const auto &x, const auto &y) { return Diff(x, y); });
		SEDX_SAME_LIST(a.curvedSegments, b.curvedSegments, "curvedSegments", [](const auto &x, const auto &y) { return Diff(x, y); });
		SEDX_SAME_LIST(a.walls, b.walls, "walls", [](const FacadeAsset::WallRule &x, const FacadeAsset::WallRule &y) -> std::string
		{
			SEDX_SAME(x.name, y.name, "name");
			SEDX_SAME(x.minWidth, y.minWidth, "minWidth");
			SEDX_SAME(x.maxWidth, y.maxWidth, "maxWidth");
			SEDX_SAME(x.minHeading, y.minHeading, "minHeading");
			SEDX_SAME(x.maxHeading, y.maxHeading, "maxHeading");
			SEDX_SAME_LIST(x.filters, y.filters, "filters", [](const auto &p, const auto &q) { return Diff(p, q); });
			SEDX_SAME_VALUES(x.spellings, y.spellings, "spellings");
			return {};
		});
		SEDX_SAME_VALUES(a.roofHeights, b.roofHeights, "roofHeights");
		SEDX_SAME_LIST(a.roofs, b.roofs, "roofs", [](const FacadeAsset::Roof &x, const FacadeAsset::Roof &y) -> std::string
		{
			SEDX_SAME(x.height, y.height, "height");
			SEDX_SAME(x.twoSided, y.twoSided, "twoSided");
			SEDX_SAME_LIST(x.objects, y.objects, "objects", [](const FacadeAsset::RoofObject &p, const FacadeAsset::RoofObject &q) -> std::string
			{
				SEDX_SAME(p.object, q.object, "object");
				SEDX_SAME(p.s, q.s, "s");
				SEDX_SAME(p.t, q.t, "t");
				SEDX_SAME(p.heading, q.heading, "heading");
				SEDX_SAME(p.showLo, q.showLo, "showLo");
				SEDX_SAME(p.showHi, q.showHi, "showHi");
				return {};
			});
			return {};
		});
		SEDX_SAME(a.hardRoof, b.hardRoof, "hardRoof");
		SEDX_SAME(a.height, b.height, "height");
		return {};
	}

	std::string Diff(const FacadeAsset &a, const FacadeAsset &b)
	{
		SEDX_SAME(a.version, b.version, "version");
		SEDX_SAME(a.ring, b.ring, "ring");
		SEDX_SAME(a.twoSided, b.twoSided, "twoSided");
		SEDX_SAME(a.graded, b.graded, "graded");
		SEDX_SAME(a.wallTexture, b.wallTexture, "wallTexture");
		SEDX_SAME(a.roofTexture, b.roofTexture, "roofTexture");
		SEDX_SAME(a.wallTextureNormal, b.wallTextureNormal, "wallTextureNormal");
		SEDX_SAME(a.wallTextureNormalScale, b.wallTextureNormalScale, "wallTextureNormalScale");
		SEDX_SAME(a.roofTextureNormal, b.roofTextureNormal, "roofTextureNormal");
		SEDX_SAME(a.roofTextureNormalScale, b.roofTextureNormalScale, "roofTextureNormalScale");
		SEDX_SAME(a.noBlendCutoff, b.noBlendCutoff, "noBlendCutoff");
		SEDX_SAME(a.layerGroup, b.layerGroup, "layerGroup");
		SEDX_SAME(a.layerOffset, b.layerOffset, "layerOffset");
		SEDX_SAME(a.drapedLayerGroup, b.drapedLayerGroup, "drapedLayerGroup");
		SEDX_SAME(a.drapedLayerOffset, b.drapedLayerOffset, "drapedLayerOffset");
		SEDX_SAME_LIST(a.scrapers, b.scrapers, "scrapers", [](const FacadeAsset::Scraper &x, const FacadeAsset::Scraper &y) -> std::string
		{
			SEDX_SAME(x.minAgl, y.minAgl, "minAgl");
			SEDX_SAME(x.maxAgl, y.maxAgl, "maxAgl");
			SEDX_SAME(x.step, y.step, "step");
			SEDX_SAME(x.floors, y.floors, "floors");
			SEDX_SAME(x.models, y.models, "models");
			SEDX_SAME(x.pads, y.pads, "pads");
			SEDX_SAME_VALUES(x.rows, y.rows, "rows");
			return {};
		});
		SEDX_SAME_VALUES(a.wallShaderRows, b.wallShaderRows, "wallShaderRows");
		SEDX_SAME_VALUES(a.roofShaderRows, b.roofShaderRows, "roofShaderRows");
		SEDX_SAME_LIST(a.walls, b.walls, "walls", [](const auto &x, const auto &y) { return Diff(x, y); });
		SEDX_SAME(a.lodFar, b.lodFar, "lodFar");
		SEDX_SAME(a.hasRoof, b.hasRoof, "hasRoof");
		SEDX_SAME_VALUES(a.roofUv, b.roofUv, "roofUv");
		SEDX_SAME(a.roofScaled, b.roofScaled, "roofScaled");
		SEDX_SAME(a.roofCenter, b.roofCenter, "roofCenter");
		SEDX_SAME(a.roofTextureWidth, b.roofTextureWidth, "roofTextureWidth");
		SEDX_SAME(a.roofTextureHeight, b.roofTextureHeight, "roofTextureHeight");
		SEDX_SAME(a.roofScaleS, b.roofScaleS, "roofScaleS");
		SEDX_SAME(a.roofScaleT, b.roofScaleT, "roofScaleT");
		SEDX_SAME(a.floorsMin, b.floorsMin, "floorsMin");
		SEDX_SAME(a.floorsMax, b.floorsMax, "floorsMax");
		SEDX_SAME(a.doubled, b.doubled, "doubled");
		SEDX_SAME(a.hardWall, b.hardWall, "hardWall");
		SEDX_SAME(a.hardRoof, b.hardRoof, "hardRoof");
		SEDX_SAME_VALUES(a.objects, b.objects, "objects");
		SEDX_SAME_LIST(a.floors, b.floors, "floors", [](const auto &x, const auto &y) { return Diff(x, y); });
		SEDX_SAME(a.noRoofMesh, b.noRoofMesh, "noRoofMesh");
		SEDX_SAME(a.noWallMesh, b.noWallMesh, "noWallMesh");
		return {};
	}

#undef SEDX_SAME
#undef SEDX_SAME_LIST
#undef SEDX_SAME_VALUES

	FacadeAsset Parse(const std::string &text)
	{
		FacadeAsset asset;
		std::string error;
		INFO(error);
		REQUIRE(ParseFacadeAsset(text, asset, &error));
		return asset;
	}

	/* @brief parse -> write -> parse is the same asset, and writing that again gives the same text. Returns the text written. */
	std::string CheckRoundTrip(const FacadeAsset &asset)
	{
		const std::string written = WriteFacadeAsset(asset);
		const FacadeAsset again = Parse(written);
		const std::string diff = Diff(asset, again);
		INFO("first difference: " << diff);
		CHECK(diff.empty());
		CHECK(WriteFacadeAsset(again) == written);
		return written;
	}

	bool Contains(const std::string &text, const std::string &part)
	{
		return text.find(part) != std::string::npos;
	}

	struct TempDir
	{
		fs::path path;
		TempDir()
		{
			path = fs::temp_directory_path() / ("sedx_facade_writer_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
			fs::create_directories(path);
		}
		~TempDir()
		{
			std::error_code ec;
			fs::remove_all(path, ec);
		}
	};

	/* @brief The X-Plane 12 install X-Plane's install list names (empty when there is none). */
	fs::path FindXPlane()
	{
		for (const fs::path &root : SceneryEditorX::VFS::FindXPlaneInstalls(12))
		{
			if (fs::exists(root / "Resources" / "default scenery"))
				return root;
		}
		return {};
	}

	/* @brief A facade drawn on a footprint by the overlay builder (local meters east / north, counter-clockwise). */
	OverlayBuild BuildOverlay(const fs::path &package, const std::string &resource, const std::vector<std::pair<double, double>> &footprint, const uint16_t height)
	{
		const AssetResolver resolver = [&](const std::string_view path) -> fs::path
		{
			const fs::path local = package / fs::path(std::string(path));
			return fs::exists(local) ? local : fs::path();
		};
		const GeoProjection projection(45.6, -122.6);
		OverlayBuilder builder(resolver, projection);
		DsfFile dsf;
		dsf.polygonDefinitions = { resource };
		DsfPolygonPlacement placement;
		placement.definition = 0;
		placement.parameter = height;
		placement.planes = 2;
		std::vector<DsfPoint> winding;
		for (const auto &[east, north] : footprint)
		{
			DsfPoint p;
			p.planes = 2;
			projection.ToGeo(Point2{ east, -north }, p.v[1], p.v[0]);
			winding.push_back(p);
		}
		placement.windings = { winding };
		dsf.polygons.push_back(placement);
		OverlayBuild out;
		builder.Build(dsf, OverlayBuilder::Options(), out);
		return out;
	}

	// -------------------------------------------------------
	// Synthetic facades using every directive the writer knows

	const char *TYPE1_FACADE =
		"A\n800\nFACADE\n\n"
		"DOUBLED 1\n"
		"RING 1\n"
		"GRADED\n"
		"FLOORS_MIN 2\n"
		"FLOORS_MAX 8\n"
		"HARD_WALL concrete\n"
		"HARD_ROOF concrete\n"
		"LAYER_GROUP objects 1\n"
		"EXPORT lib/test/building.fac test\\building.fac\n"
		"SHADER_WALL\n"
		"TEXTURE ../textures\\wall_ALB.png\n"
		"TEXTURE_LIT ../textures\\wall_LIT.png\n"
		"TEXTURE_NORMAL 0.5 ../textures/wall_NML.png\n"
		"TWO_SIDED 0\n"
		"NO_BLEND 0.4\n"
		"SHADER_ROOF\n"
		"TEXTURE ../textures\\roof.png\n"
		"TEXTURE_NORMAL 1.0 ../textures\\roof_NML.png\n"
		"TWO_SIDED 0\n"
		"NO_BLEND 0.4\n"
		"LOD  0 15000\n"
		"FACADE_SCRAPER 0.000000 24.000000 3 0\n"
		"FACADE_SCRAPER_MODEL_OFFSET 0 0 0 lib/a.obj 1 1 0 0 0 - 1 1  -12 5 12 5 12 19 -12 19\n"
		"FACADE_SCRAPER_PAD 0 10 0 0 pad.obj 2 6\n"
		"TEX_SIZE 1024.000000 512.000000\n"
		"ROOF_SCALE 194.300000 3.800000 322.300000 9.700000 450.200000 239.500000 64.560387 57.643209\n"
		"TEX_SIZE 2048.000000 2048.000000\n"
		"WALL 30 120\n"
		"ROOF_SLOPE -2 SLANT\n"
		"SCALE 204.800035 204.799974\n"
		"BASEMENT_DEPTH 37.999902\n"
		"LEFT 1.000000 10.000000\n"
		"LEFT 10.000000 33.800000\n"
		"CENTER 63.000000 119.000000\n"
		"RIGHT 1900 2048\n"
		"BOTTOM 0 100\n"
		"MIDDLE 100 1800\n"
		"TOP 1800 2048\n"
		"TEX_SIZE 1000 1000\n"
		"WALL 2 500 0 0 fence gate\n"
		"WALL_RULE 0 10 90 270\n"
		"SCALE 25.6 25.6\n"
		"LEFT 593 631\n"
		"BOTTOM 640 703\n"
		"WALL 0 9999 0 0 fractions\n"
		"TEX_SIZE 1 1\n"
		"SCALE 10 10\n"
		"CENTER 0.1 0.9\n"
		"MIDDLE 0.2 0.8\n"
		"LOD 15000 30000\n"
		"WALL 0 9999\n"
		"SCALE 1 1\n"
		"CENTER 0 1\n"
		"MIDDLE 0 1\n";

	/* @brief A segment quad `width` meters along -z from `z0`, `height` up. */
	std::string Quad(const double z0, const double width, const double height, const int cuts = 1)
	{
		const auto f = [](const double v) { return std::to_string(v); };
		return "MESH 0 5000.000000 " + std::to_string(cuts) + " 4 6\n"
			"VERTEX 0 0 " + f(z0) + "\t0 0 1\t0.25 0\n"
			"VERTEX 0 0 " + f(z0 - width) + "\t0 0 1\t0.75 0\n"
			"VERTEX 0 " + f(height) + " " + f(z0 - width) + "\t0 0 1\t0.75 1\n"
			"VERTEX 0 " + f(height) + " " + f(z0) + "\t0 0 1\t0.25 1\n"
			"IDX 3 2 1 3 1 0\n";
	}

	std::string Type2Facade()
	{
		return std::string("A\n1000\nFACADE\n\n"
			"RING 0\n"
			"GRADED\n"
			"LAYER_GROUP objects 2\n"
			"LAYER_GROUP_DRAPED markings -1\n"
			"NO_ROOF_MESH \n"
			"FACADE_SCRAPER 100 300 4 2\n"
			"FACADE_SCRAPER_MODEL base.obj - 0 0\n"
			"SHADER_WALL\n"
			"TEXTURE ..\\..\\textures\\hangars_5_ALB.png\n"
			"TEXTURE_NORMAL 1.0 ..\\..\\textures\\hangars_5_NML.png\n"
			"TEXTURE_MODULATOR ..\\..\\textures\\hangars_5_MOD.png\n"
			"NORMAL_METALNESS \n"
			"NO_BLEND 0.5\n"
			"DECAL_LIB lib/g10/decals/bld_hangar_2.dcl\n"
			"SHADER_ROOF\n"
			"TEXTURE ..\\..\\textures\\concrete_1b_ALB.png\n"
			"DECAL_LIB lib/g10/decals/apt_sw_concrete_1.dcl\n"
			"SNOW_ALBEDO_LUMA 0.4 0.4 0.4 0.0\n"
			"ROOF_SCALE 8\n"
			"\n"
			"OBJ doorhandle_2.obj\n"
			"OBJ lights/pole.obj\n"
			"\n"
			"FLOOR h_0.15m\n"
			"ROOF_HEIGHT 0.150000\n"
			"TWO_SIDED_ROOF\n"
			"ROOF_OBJ_HEADING 1 10 5 90 2 6\n"
			"ROOF_OBJ 0 1.5 2.5\n"
			"HARD_ROOF concrete\n"
			"SEGMENT 0\n") + Quad(0.0, 1.0, 0.15) +
			"ATTACH_GRADED 0 0.049995 0.980000 -0.5 -90.000000 3 5\n"
			"ATTACH_DRAPED 1 0 0 -0.25 0\n"
			"SEGMENT 1\n" + Quad(1.0, 2.0, 0.15) +	// Starts at z 1: the parser shifts it to end at 0
			"SEGMENT_CURVED 0\n" + Quad(0.0, 1.0, 0.15, 2) +
			"SEGMENT_CURVED 1\n" + Quad(0.0, 2.0, 0.15, 3) +
			"WALL 0 10 270 90 front_short\n"
			"SPELLING 0\n"
			"SPELLING 0 0\n"
			"WALL 10 999 0 0 long\n"
			"WALL_RULE 0 999 90 180\n"
			"SPELLING 1\n"
			"SPELLING 1 0 1\n"
			"WALL 0 999 0 0\n"
			"SPELLING 0 1\n"
			"FLOOR high\n"
			"ROOF_HEIGHT 6 12\n"
			"SEGMENT 0\n" + Quad(0.0, 4.0, 12.0) +
			"WALL 0 999 0 0 tall\n"
			"SPELLING 0\n";
	}
}

// -------------------------------------------------------

TEST_CASE("Facade numbers are written short and read back exactly", "[facades][writer]")
{
	CHECK(FormatFacadeNumber(0.15f) == "0.15");
	CHECK(FormatFacadeNumber(1024.0f) == "1024");
	CHECK(FormatFacadeNumber(-0.06f) == "-0.06");
	CHECK(FormatFacadeNumber(0.0f) == "0");
	CHECK(FormatFacadeNumber(-0.0f) == "0");
	CHECK(FormatFacadeNumber(0.00001f) == "0.00001");	// Fixed notation, never "1e-05"
	for (const float value : { 0.1f, 1.0f / 3.0f, 204.800035f, 0.049938f, 1e-7f, 123456.79f })
		CHECK(std::stof(FormatFacadeNumber(value)) == value);
}

TEST_CASE("Type 1 facades round trip through the writer", "[facades][writer]")
{
	const FacadeAsset fac = Parse(TYPE1_FACADE);

	// What the parser keeps for the writer
	CHECK(fac.lodFar == Catch::Approx(15000.0f));
	REQUIRE(fac.walls.size() == 3);	// The far LOD's wall isn't kept
	CHECK(fac.walls[0].textureWidth == 2048.0f);
	CHECK(fac.walls[1].textureWidth == 1000.0f);
	CHECK(fac.walls[1].name == "fence gate");
	CHECK(fac.walls[1].filters.size() == 2);
	CHECK(fac.walls[2].textureWidth == 0.0f);	// "TEX_SIZE 1 1" right after its WALL: fractions
	CHECK(fac.walls[2].centerPanels[0].first == Catch::Approx(0.1f));
	CHECK(fac.roofScaled);
	CHECK(fac.roofTextureWidth == 1024.0f);
	CHECK(fac.roofTextureHeight == 512.0f);
	CHECK(fac.roofCenter.first == Catch::Approx(322.3f / 1024.0f));
	CHECK(fac.wallShaderRows == std::vector<std::string>{ "TEXTURE_LIT ../textures\\wall_LIT.png" });
	CHECK(fac.roofShaderRows == std::vector<std::string>{ "TWO_SIDED 0", "NO_BLEND 0.4" });
	CHECK(fac.noBlendCutoff == Catch::Approx(0.4f));
	CHECK(fac.wallTextureNormalScale == Catch::Approx(0.5f));
	CHECK(fac.roofTextureNormal == "../textures/roof_NML.png");
	REQUIRE(fac.scrapers.size() == 1);
	CHECK(fac.scrapers[0].rows.size() == 2);
	CHECK(fac.scrapers[0].models == 1);
	CHECK(fac.scrapers[0].pads == 1);

	const std::string written = CheckRoundTrip(fac);
	INFO(written);
	// Pixels come back as the file had them; unmodeled shader rows survive; EXPORT rows and far LODs don't
	CHECK(Contains(written, "\nTEX_SIZE 2048 2048\n"));
	CHECK(Contains(written, "\nLEFT 10 33.8\n"));
	CHECK(Contains(written, "\nRIGHT 1900 2048\n"));
	CHECK(Contains(written, "\nBASEMENT_DEPTH 37.9999\n"));	// The shortest number giving the same float
	CHECK(Contains(written, "\nROOF_SCALE 194.3 3.8 322.3 9.7 450.2 239.5 "));
	CHECK(Contains(written, "\nTEX_SIZE 1 1\n"));
	CHECK(Contains(written, "\nCENTER 0.1 0.9\n"));
	CHECK(Contains(written, "\nTEXTURE_LIT ../textures\\wall_LIT.png\n"));
	CHECK(Contains(written, "\nWALL 2 500 0 0 fence gate\nWALL_RULE 0 10 90 270\n"));
	CHECK(Contains(written, "\nROOF_SLOPE -2 SLANT\n"));
	CHECK(Contains(written, "\nLOD 0 15000\n"));
	CHECK_FALSE(Contains(written, "EXPORT"));
	CHECK_FALSE(Contains(written, "LOD 15000"));
}

TEST_CASE("Type 2 facades round trip through the writer", "[facades][writer]")
{
	const FacadeAsset fac = Parse(Type2Facade());
	CHECK(fac.roofScaleS == Catch::Approx(8.0f));	// ROOF_SCALE with one value: square
	CHECK(fac.roofScaleT == Catch::Approx(8.0f));
	CHECK(fac.wallShaderRows == std::vector<std::string>{ "TEXTURE_MODULATOR ..\\..\\textures\\hangars_5_MOD.png", "NORMAL_METALNESS", "DECAL_LIB lib/g10/decals/bld_hangar_2.dcl" });
	CHECK(fac.roofShaderRows == std::vector<std::string>{ "DECAL_LIB lib/g10/decals/apt_sw_concrete_1.dcl", "SNOW_ALBEDO_LUMA 0.4 0.4 0.4 0.0" });
	REQUIRE(fac.floors.size() == 2);
	CHECK(fac.floors[0].segments[1].width == Catch::Approx(2.0f));
	CHECK(fac.floors[1].roofs.size() == 2);

	const std::string written = CheckRoundTrip(fac);
	INFO(written);
	// Several heights on one ROOF_HEIGHT row are written one per row (the spec's form)
	CHECK(Contains(written, "\nROOF_HEIGHT 6\nROOF_HEIGHT 12\n"));
	CHECK(Contains(written, "\nROOF_HEIGHT 0.15\nTWO_SIDED_ROOF\nROOF_OBJ_HEADING 1 10 5 90 2 6\nROOF_OBJ 0 1.5 2.5\n"));
	CHECK(Contains(written, "\nATTACH_GRADED 0 0.049995 0.98 -0.5 -90 3 5\n"));
	CHECK(Contains(written, "\nATTACH_DRAPED 1 0 0 -0.25 0\n"));
	CHECK(Contains(written, "\nMESH 0 5000 2 4 6\n"));
	CHECK(Contains(written, "\nNORMAL_METALNESS\n"));
	CHECK(Contains(written, "\nROOF_SCALE 8 8\n"));
	CHECK(Contains(written, "\nTEXTURE ../../textures/hangars_5_ALB.png\n"));
	CHECK(Contains(written, "\nSEGMENT_CURVED 1\n"));
	CHECK(Contains(written, "\nFACADE_SCRAPER_MODEL base.obj - 0 0\n"));

	// Assets built in code (no shader rows, the defaults) write and read back too
	FacadeAsset plain = fac;
	plain.wallShaderRows.clear();
	plain.roofShaderRows.clear();
	plain.roofTexture.clear();
	plain.noBlendCutoff = -1.0f;
	plain.scrapers.clear();
	CheckRoundTrip(plain);
}

TEST_CASE("X-Plane 12 library facades round trip through the writer", "[facades][writer][xplane-install]")
{
	const fs::path xplane = FindXPlane();
	if (xplane.empty())
	{
		WARN("No X-Plane 12 install found (x-plane_install_12.txt): skipped");
		return;
	}
	const fs::path scenery = xplane / "Resources" / "default scenery";

	struct Sample
	{
		const char *path;
		std::vector<std::string> kept;	// Rows the rewritten file still has
	};
	const Sample samples[] =
	{
		// lib/airport/ground/sidewalk_15cm/sw_concrete.fac
		{ "airport scenery/ground/sidewalk/sidewalk_conc_02.fac", { "NORMAL_METALNESS", "DECAL_LIB lib/g10/decals/apt_sw_curb_1.dcl", "SNOW_ALBEDO_LUMA 0.4 0.2 0.0 0.0", "WALL 0 1000 0 0 natural_curb" } },
		// A hangar door (RING 0, attached door handles)
		{ "airport scenery/Common_Elements/Hangars/hangar_doorUni_01.fac", { "TEXTURE_MODULATOR ..\\..\\textures\\hangars_5_MOD.png", "NO_ROOF_MESH", "OBJ doorhandle_2.obj" } },
		// Type 1 buildings and fences
		{ "1000 autogen/US/urban_high/objects/33x35x30v0.fac", { "TEXTURE_LIT ../textures\\UrbanHighNE_LIT.png", "ROOF_SLOPE -2 SLANT", "FLOORS_MIN 6" } },
		{ "1000 autogen/global_objects/objects/fenceBricks01.fac", { "WALL 2 500 0 0 fence", "LEFT 0 47", "DOUBLED 1", "RING 0" } },
	};
	for (const Sample &sample : samples)
	{
		const fs::path file = scenery / sample.path;
		INFO(file.string());
		if (!fs::exists(file))
		{
			WARN("Not in this install: " << sample.path);
			continue;
		}
		FacadeAsset fac;
		std::string error;
		REQUIRE(LoadFacadeAsset(file, fac, &error));
		const std::string written = CheckRoundTrip(fac);
		for (const std::string &row : sample.kept)
		{
			INFO(row);
			CHECK(Contains(written, "\n" + row + "\n"));
		}
	}
}

TEST_CASE("Every X-Plane 12 default scenery facade round trips through the writer", "[facades][writer][xplane-install][library-sweep]")
{
	const fs::path xplane = FindXPlane();
	if (xplane.empty())
	{
		WARN("No X-Plane 12 install found (x-plane_install_12.txt): skipped");
		return;
	}
	const fs::path scenery = xplane / "Resources" / "default scenery";

	// Type 1 and 2: buildings, fences, sidewalks, hangars, jetways... (about 800 files, a few seconds)
	size_t count = 0;
	std::error_code ec;
	for (auto it = fs::recursive_directory_iterator(scenery, ec); it != fs::recursive_directory_iterator(); it.increment(ec))
	{
		if (ec)
			break;
		if (!it->is_regular_file() || it->path().extension() != ".fac")
			continue;
		FacadeAsset fac;
		if (!LoadFacadeAsset(it->path(), fac))
			continue;
		const std::string written = WriteFacadeAsset(fac);
		FacadeAsset again;
		REQUIRE(ParseFacadeAsset(written, again));
		const std::string diff = Diff(fac, again);
		INFO(it->path().string() << ": " << diff);
		CHECK(diff.empty());
		++count;
	}
	CHECK(count > 100);
}

TEST_CASE("Type 1 facades authored from a texture", "[facades][authoring]")
{
	FacadeAsset fac = MakeType1Facade("textures/brick.png", 1024, 512, 20.0f);
	CHECK_FALSE(fac.IsType2());
	CHECK(CheckFacadeAsset(fac).empty());
	REQUIRE(fac.walls.size() == 1);
	FacadeAsset::Wall &wall = fac.walls[0];
	CHECK(wall.scaleS == Catch::Approx(20.0f));
	CHECK(wall.scaleT == Catch::Approx(10.0f));	// Square pixels
	CHECK(wall.name == "Wall 1");

	std::string written = CheckRoundTrip(fac);
	CHECK(Contains(written, "\nTEX_SIZE 1024 512\n"));
	CHECK(Contains(written, "\nLEFT 0 256\nCENTER 256 768\nRIGHT 768 1024\n"));
	CHECK(Contains(written, "\nBOTTOM 0 128\nMIDDLE 128 384\nTOP 384 512\n"));
	CHECK(Contains(written, "\nROOF 0 0\nROOF 1024 0\nROOF 1024 512\nROOF 0 512\n"));
	CHECK(Contains(written, "\nTEXTURE textures/brick.png\n"));

	// Moving an edge drags both panels that share it, never past their other edges
	CHECK(GetFacadeTextureCuts(wall, false) == std::vector<float>{ 0.0f, 0.25f, 0.75f, 1.0f });
	CHECK(MoveFacadeTextureCut(wall, false, 0.25f, FacadeFromPixels(300.0f, 1024.0f)) == FacadeFromPixels(300.0f, 1024.0f));
	CHECK(wall.leftPanels[0].second == wall.centerPanels[0].first);
	const float clamped = MoveFacadeTextureCut(wall, false, wall.centerPanels[0].first, 0.95f, 0.01f);
	CHECK(clamped == Catch::Approx(0.74f));
	CHECK(MoveFacadeTextureCut(wall, false, 0.5f, 0.6f) == 0.5f);	// No edge there
	MoveFacadeTextureCut(wall, false, wall.centerPanels[0].first, FacadeFromPixels(300.0f, 1024.0f));

	// Split on a whole pixel; snapping keeps whole pixels whole
	REQUIRE(SplitFacadePanel(wall, FacadePanelRow::Center, 0));
	REQUIRE(wall.centerPanels.size() == 2);
	CHECK(wall.centerPanels[0].second == FacadeFromPixels(534.0f, 1024.0f));
	wall.middleFloors[0].second = 0.70001f;
	wall.topFloors[0].first = 0.70001f;
	SnapFacadeWallToPixels(wall);
	CHECK(wall.middleFloors[0].second == FacadeFromPixels(358.0f, 512.0f));
	written = CheckRoundTrip(fac);
	CHECK(Contains(written, "\nLEFT 0 300\nCENTER 300 534\nCENTER 534 768\n"));
	CHECK(Contains(written, "\nMIDDLE 128 358\nTOP 358 512\n"));

	// A second wall type for narrow walls, a repeated roof rectangle
	FacadeAsset::Wall narrow = MakeType1Wall(fac);
	narrow.filters[0].maxWidth = 6.0f;
	narrow.maxWidth = 6.0f;
	fac.walls.insert(fac.walls.begin(), narrow);
	SetFacadeRoofScaled(fac, 0.0f, 0.5f, 0.25f, 1.0f, 12.0f, 6.0f);
	CHECK(GetFacadeRoofMeters(fac).first == Catch::Approx(12.0f));
	CHECK(GetFacadeRoofMeters(fac).second == Catch::Approx(6.0f));
	written = CheckRoundTrip(fac);
	CHECK(Contains(written, "\nWALL 0 6 0 0 Wall 2\n"));
	CHECK(Contains(written, "\nROOF_SCALE 0 256 128 384 256 512 12 6\n"));

	// Problems the authoring panel lists
	FacadeAsset broken = fac;
	broken.wallTexture.clear();
	broken.walls[0].centerPanels[0] = { 0.5f, 0.4f };
	broken.walls[1].leftPanels.clear();
	broken.walls[1].centerPanels.clear();
	broken.walls[1].rightPanels.clear();
	const std::vector<std::string> problems = CheckFacadeAsset(broken);
	CHECK(problems.size() == 3);

	// The overlay builder draws it: walls on every side and a roof at the top floor
	TempDir temp;
	fs::create_directories(temp.path / "facades");
	REQUIRE(SaveFacadeAsset(temp.path / "facades" / "brick.fac", fac));
	const OverlayBuild out = BuildOverlay(temp.path, "facades/brick.fac", { { 0, 0 }, { 20, 0 }, { 20, 12 }, { 0, 12 } }, 12);
	size_t walls = 0, roofs = 0;
	float top = 0.0f;
	for (const OverlayMesh &mesh : out.meshes)
	{
		if (mesh.kind != OverlayKind::Facade)
			continue;
		(mesh.name.find("roof") != std::string::npos ? roofs : walls) += 1;
		for (const OverlayVertex &vertex : mesh.vertices)
			top = std::max(top, vertex.y);
	}
	CHECK(walls == 1);
	CHECK(roofs == 1);
	CHECK(top == Catch::Approx(12.0f).margin(0.01f));
}

TEST_CASE("Sidewalks and curbs generated from a cross-section", "[facades][authoring]")
{
	CurbFacadeSpec spec;
	spec.profile = GetCurbProfilePresets().front().points;
	spec.wallTexture = "textures/curb.png";
	spec.roofTexture = "textures/concrete.png";
	spec.textureS1 = 0.5f;
	spec.textureS2 = 1.0f;
	spec.textureT1 = 0.75f;
	spec.textureT2 = 1.0f;
	spec.textureLength = 2.0f;
	const FacadeAsset fac = BuildCurbFacade(spec);

	CHECK(fac.IsType2());
	CHECK(CheckFacadeAsset(fac).empty());
	CHECK(GetFacadeFamily(fac) == FacadeFamily::Ground);
	const FacadeHeights heights = GetFacadeHeights(fac);
	CHECK(heights.fixed);
	CHECK(heights.maximum == Catch::Approx(0.15f));
	REQUIRE(fac.floors.size() == 1);
	const FacadeAsset::Floor &floor = fac.floors[0];
	REQUIRE(floor.segments.size() == 5);
	REQUIRE(floor.curvedSegments.size() == 5);
	CHECK(floor.segments[4].width == Catch::Approx(4.0f));
	CHECK(floor.curvedSegments[4].meshes[0].cuts == 6);	// 4 m in 0.65 m pieces
	CHECK(floor.segments[4].meshes[0].cuts == 1);
	CHECK(floor.curvedSegments[4].meshes[0].positions.size() > floor.segments[4].meshes[0].positions.size());
	REQUIRE(floor.walls.size() == 1);
	CHECK(floor.walls[0].name == "curb");
	CHECK(floor.hardRoof == "concrete");

	// The texture strip restarts every 2 m and stays inside its rectangle; t runs from the top edge (t2) to the ground (t1)
	const FacadeAsset::Mesh &mesh = floor.segments[4].meshes[0];
	for (size_t v = 0; v < mesh.uvs.size() / 2; ++v)
	{
		CHECK(mesh.uvs[v * 2] >= 0.5f - 1e-5f);
		CHECK(mesh.uvs[v * 2] <= 1.0f + 1e-5f);
		CHECK(mesh.uvs[v * 2 + 1] >= 0.75f - 1e-5f);
		CHECK(mesh.uvs[v * 2 + 1] <= 1.0f + 1e-5f);
		if (mesh.positions[v * 3 + 1] == 0.15f && mesh.positions[v * 3] == 0.0f)
			CHECK(mesh.uvs[v * 2 + 1] == Catch::Approx(1.0f));
	}

	// Triangles wind like X-Plane's own facades: clockwise seen from the front (the face normal and the triangle's disagree)
	const auto windings = [](const FacadeAsset::Mesh &m, size_t &clockwise, size_t &total)
	{
		for (size_t i = 0; i + 2 < m.indices.size(); i += 3)
		{
			const auto p = [&](const uint32_t index, const int axis) { return static_cast<double>(m.positions[index * 3 + static_cast<size_t>(axis)]); };
			const uint32_t a = m.indices[i], b = m.indices[i + 1], c = m.indices[i + 2];
			const double ux = p(b, 0) - p(a, 0), uy = p(b, 1) - p(a, 1), uz = p(b, 2) - p(a, 2);
			const double vx = p(c, 0) - p(a, 0), vy = p(c, 1) - p(a, 1), vz = p(c, 2) - p(a, 2);
			const double cx = uy * vz - uz * vy, cy = uz * vx - ux * vz, cz = ux * vy - uy * vx;
			const double dot = cx * m.normals[a * 3] + cy * m.normals[a * 3 + 1] + cz * m.normals[a * 3 + 2];
			clockwise += dot < 0.0 ? 1 : 0;
			++total;
		}
	};
	size_t clockwise = 0, total = 0;
	for (const FacadeAsset::Segment &segment : floor.segments)
		windings(segment.meshes[0], clockwise, total);
	CHECK(total > 0);
	CHECK(clockwise == total);

	// The same holds for the library's sidewalk (what the convention was taken from)
	const fs::path xplane = FindXPlane();
	const fs::path sidewalk = xplane / "Resources/default scenery/airport scenery/ground/sidewalk/sidewalk_conc_02.fac";
	if (!xplane.empty() && fs::exists(sidewalk))
	{
		FacadeAsset library;
		REQUIRE(LoadFacadeAsset(sidewalk, library));
		size_t libraryClockwise = 0, libraryTotal = 0;
		for (const FacadeAsset::Segment &segment : library.floors[0].segments)
			for (const FacadeAsset::Mesh &m : segment.meshes)
				windings(m, libraryClockwise, libraryTotal);
		CHECK(libraryClockwise * 10 >= libraryTotal * 9);
	}

	// Lengths are spelled from the segments; long walls stretch little
	const std::vector<double> natural = GetFacadeNaturalLengths(fac, 0, 0, 20.0);
	REQUIRE(natural.size() >= 5);
	CHECK(natural[0] == Catch::Approx(0.25));
	double spelled = 0.0;
	PickFacadeSpelling(floor, floor.walls[0], 10.3, spelled);
	CHECK(std::abs(10.3 / spelled - 1.0) < 0.05);

	const std::string written = CheckRoundTrip(fac);
	CHECK(Contains(written, "\nFLOOR h_0.15m\nROOF_HEIGHT 0.15\nHARD_ROOF concrete\n"));
	CHECK(Contains(written, "\nWALL 0 9999 0 0 curb\nSPELLING 0\nSPELLING 1\n"));
	CHECK(Contains(written, "\nSHADER_ROOF\nTEXTURE textures/concrete.png\n"));

	// An open curb line drawn by the overlay builder: the curb along the line, no roof
	CurbFacadeSpec line = spec;
	line.ring = false;
	line.profile = GetCurbProfilePresets()[3].points;	// Planter wall, 45 cm
	const FacadeAsset planter = BuildCurbFacade(line);
	CHECK(GetFacadeFamily(planter) == FacadeFamily::Line);
	TempDir temp;
	REQUIRE(SaveFacadeAsset(temp.path / "curb.fac", fac));
	REQUIRE(SaveFacadeAsset(temp.path / "planter.fac", planter));
	const OverlayBuild ring = BuildOverlay(temp.path, "curb.fac", { { 0, 0 }, { 10, 0 }, { 10, 6 }, { 0, 6 } }, 0);
	bool roof = false;
	float curbTop = 0.0f, curbOut = 0.0f;
	for (const OverlayMesh &m : ring.meshes)
	{
		if (m.kind != OverlayKind::Facade)
			continue;
		if (m.name.find("roof") != std::string::npos)
		{
			roof = true;
			continue;
		}
		for (const OverlayVertex &vertex : m.vertices)
		{
			curbTop = std::max(curbTop, vertex.y);
			if (vertex.x > 1.0 && vertex.x < 9.0)
				curbOut = std::max(curbOut, vertex.z);	// South of the front wall (+z): outward
		}
	}
	CHECK(roof);
	CHECK(curbTop == Catch::Approx(0.15f).margin(1e-4f));
	CHECK(curbOut == Catch::Approx(0.15f).margin(1e-3f));
	const OverlayBuild open = BuildOverlay(temp.path, "planter.fac", { { 0, 0 }, { 10, 0 }, { 20, 5 } }, 0);
	CHECK(std::ranges::any_of(open.meshes, [](const OverlayMesh &m) { return m.kind == OverlayKind::Facade && !m.indices.empty(); }));
	CHECK(std::ranges::none_of(open.meshes, [](const OverlayMesh &m) { return m.name.find("roof") != std::string::npos; }));
}

TEST_CASE("Facade authoring file helpers", "[facades][authoring]")
{
	TempDir temp;
	// PNG: signature, IHDR length, "IHDR", width, height (big-endian)
	const unsigned char png[] = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A, 0, 0, 0, 13, 'I', 'H', 'D', 'R', 0, 0, 0x08, 0, 0, 0, 0x02, 0, 8, 6, 0, 0, 0, 0, 0, 0 };
	std::ofstream(temp.path / "a.png", std::ios::binary).write(reinterpret_cast<const char *>(png), sizeof(png));
	// DDS: "DDS ", header size 124, flags, height, width (little-endian)
	unsigned char dds[128] = { 'D', 'D', 'S', ' ', 124, 0, 0, 0, 7, 0x10, 0x08, 0, 0, 0x01, 0, 0, 0, 0x04, 0, 0 };
	std::ofstream(temp.path / "b.dds", std::ios::binary).write(reinterpret_cast<const char *>(dds), sizeof(dds));
	std::ofstream(temp.path / "c.txt") << "not an image, but long enough to read a header from";

	int width = 0, height = 0;
	CHECK(ReadTextureSize(temp.path / "a.png", width, height));
	CHECK(width == 2048);
	CHECK(height == 512);
	CHECK(ReadTextureSize(temp.path / "b.dds", width, height));
	CHECK(width == 1024);
	CHECK(height == 256);
	CHECK_FALSE(ReadTextureSize(temp.path / "c.txt", width, height));
	CHECK_FALSE(ReadTextureSize(temp.path / "missing.png", width, height));

	CHECK(MakeFacadeRelativePath("C:/pack/facades/a.fac", "C:/pack/textures/b.png") == "../textures/b.png");
	CHECK(MakeFacadeRelativePath("C:/pack/facades/a.fac", "C:/pack/facades/b.png") == "b.png");
	CHECK(MakeFacadeFileName("Hangar 2 (red)") == "Hangar_2__red_");
	CHECK(MakeFacadeFileName("") == "facade");
}

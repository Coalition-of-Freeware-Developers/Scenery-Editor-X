/**
 * -------------------------------------------------------
 * Scenery Editor X - Unit Tests
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * XPlaneFormatTests.cpp
 * -------------------------------------------------------
 * OBJ8 parsing and animation, library.txt / scenery_packs.ini parsing, the library index's priority rules and the
 * lights.txt catalog. Only the standard library is involved, so this suite also builds on the headless Linux CI image.
 * -------------------------------------------------------
 */
#include <catch2/catch_all.hpp>
#include <SceneryEditorX/xplane/xp_library.h>
#include <SceneryEditorX/xplane/xp_lights.h>
#include <SceneryEditorX/xplane/xp_obj8.h>
#include <SceneryEditorX/xplane/xp_text.h>
#include <cmath>
#include <filesystem>
#include <fstream>

using namespace SceneryEditorX::XPlane;
using Catch::Approx;

namespace
{
	constexpr const char *ANIMATED_OBJ = R"(I
800
OBJ

TEXTURE	textures/test_ALB.png
TEXTURE_NORMAL	textures/test_NML.png
NORMAL_METALNESS
TEXTURE_LIT	textures/test_LIT.png
GLOBAL_specular	0.8
POINT_COUNTS	6	0	0	6

VT	0 0 0	0 1 0	0 0
VT	1 0 0	0 1 0	1 0
VT	0 0 1	0 1 0	0 1
VT	0 1 0	0 1 0	0 0
VT	1 1 0	0 1 0	1 0
VT	0 1 1	0 1 0	0 1

IDX	0
IDX	1
IDX	2
IDX	3
IDX	4
IDX	5

ATTR_LOD	0	1000
TRIS	0 3
ANIM_begin
	ANIM_rotate_begin	0 1 0	sim/time/total_running_time_sec
	ANIM_rotate_key	0	0
	ANIM_rotate_key	4	360
	ANIM_keyframe_loop	4
	ANIM_rotate_end
	ANIM_hide	0 0.5	sim/time/total_running_time_sec
	ANIM_keyframe_loop	1
	TRIS	3 3
ANIM_end
ATTR_no_cull
TRIS	0 3
)";

	/* Writes a file, creating its folder. */
	void WriteFile(const std::filesystem::path &path, const std::string &text)
	{
		std::filesystem::create_directories(path.parent_path());
		std::ofstream file(path, std::ios::binary);
		file << text;
	}

	/* Rotates a point with a rigid transform. */
	std::array<float, 3> Apply(const RigidTransform &transform, const float x, const float y, const float z)
	{
		const float in[3] = { x, y, z };
		std::array<float, 3> out{};
		transform.TransformPoint(in, out.data());
		return out;
	}
}

// -------------------------------------------------------
// OBJ8
// -------------------------------------------------------

TEST_CASE("Dataref references parse array indices and empty placeholders", "[xplane][obj8]")
{
	const DatarefRef indexed = DatarefRef::Parse("sim/flightmodel2/gear/deploy_ratio[2]");
	CHECK(indexed.name == "sim/flightmodel2/gear/deploy_ratio");
	CHECK(indexed.index == 2);
	CHECK(indexed.ToString() == "sim/flightmodel2/gear/deploy_ratio[2]");

	CHECK_FALSE(DatarefRef::Parse("none").IsSet());
	CHECK_FALSE(DatarefRef::Parse("no_ref").IsSet());
	CHECK(DatarefRef::Parse("sim/time/zulu_time_sec").index == -1);
}

TEST_CASE("OBJ8 header is validated", "[xplane][obj8]")
{
	Obj8Data data;
	std::string error;
	CHECK_FALSE(ParseObj8("I\n700\nOBJ\n", data, &error));
	CHECK(error.find("700") != std::string::npos);
	CHECK_FALSE(ParseObj8("X\n800\nOBJ\n", data, &error));
	CHECK(ParseObj8("A\r800\rOBJ\r\rTEXTURE tex.png\r", data, &error)); // classic Mac line endings
	CHECK(data.texture == "tex.png");
}

TEST_CASE("OBJ8 parses textures, geometry, LODs, animation groups and batches", "[xplane][obj8]")
{
	Obj8Data data;
	std::string error;
	REQUIRE(ParseObj8(ANIMATED_OBJ, data, &error));

	CHECK(data.version == 800);
	CHECK(data.texture == "textures/test_ALB.png");
	CHECK(data.textureNormal == "textures/test_NML.png");
	CHECK(data.textureLit == "textures/test_LIT.png");
	CHECK(data.normalMetalness);
	CHECK(data.hasGlobalSpecular);
	CHECK(data.globalSpecular == Approx(0.8f));

	REQUIRE(data.vertices.size() == 6);
	REQUIRE(data.indices.size() == 6);
	CHECK(data.vertices[1].position[0] == Approx(1.0f));
	CHECK(data.vertices[2].uv[1] == Approx(1.0f));

	REQUIRE(data.lods.size() == 1);
	CHECK(data.lods[0].farDistance == Approx(1000.0f));
	CHECK(data.HasAdditiveLods());

	// One group with a looping keyframed rotation and a looping hide rule
	REQUIRE(data.nodes.size() == 1);
	REQUIRE(data.nodes[0].ops.size() == 1);
	const Obj8AnimOp &op = data.nodes[0].ops[0];
	CHECK(op.type == Obj8AnimOp::Type::Rotate);
	CHECK(op.rotKeys.size() == 2);
	CHECK(op.loop == Approx(4.0f));
	REQUIRE(data.visibility.size() == 1);
	CHECK(data.visibility[0].hide);
	CHECK(data.visibility[0].loop == Approx(1.0f));

	// Static, animated, then a no-cull batch (different state, so not merged)
	REQUIRE(data.batches.size() == 3);
	CHECK(data.batches[0].node == -1);
	CHECK(data.batches[1].node == 0);
	CHECK(data.batches[1].visibility.size() == 1);
	CHECK(data.batches[2].node == -1);
	CHECK_FALSE(data.batches[2].state.cull);
	CHECK(data.batches[2].visibility.empty()); // ANIM_end restored the parent's rules
	CHECK(data.GetTriangleCount() == 3);

	REQUIRE(data.datarefs.size() == 1);
	CHECK(data.datarefs[0].dataref.name == "sim/time/total_running_time_sec");
	CHECK(data.datarefs[0].useCount == 2);
	CHECK(data.warnings.empty());
}

TEST_CASE("OBJ8 parses X-Plane 12 material directives", "[xplane][obj8]")
{
	// Forms found in X-Plane 12's shipped art (tabs, misspellings, trailing values)
	constexpr const char *MATERIAL_OBJ = "I\n800\nOBJ\n"
		"TEXTURE\tbuildings_ALB.png\n"
		"TEXTURE_MAP normal buildings_NML.png\n"
		"TEXTURE_MAP material_gloss buildings_MG.png\n"
		"NORMAL_TRANSLUCENCY\n"
		"BLEND_HASH 0.25\n"
		"SUPER_ROUGHNESS 0.5\n"
		"WEATHER ../../textures/buildings_1_WM.png\n"
		"\tWEATHER_TRANSPARENT\t\n"
		"\tSNOW_ALBEDO_LUMA\t0.2 0.2 0.0 0.0\n"
		"DECAL_LIB\tlib/g10/decals/bld_hangar_1.dcl\n"
		"DECAL_PARAMS 50 0 0 0 0 0 0.5 0 0 0 0 0 0 0 DECALS/DECAL CARPET.png\n"
		"NORMAL_DECAL_PARAMS 50 0 0 0 0 1 0 DECAL_GRAIN_DENSE_normal.png 0.5\n"
		"TEXTURE_CONTROL ../textures12/apt/ter_sfx_conc_CTL.png\n"
		"TEXTURE_MODULATOR\t../../textures/hangars_5_MOD.png\n"
		"TEXTURE_TERRAIN 160.0 160.0 ../textures12/apt/apt_desert_01.png\n";

	Obj8Data data;
	REQUIRE(ParseObj8(MATERIAL_OBJ, data));
	CHECK(data.unknownCommandCount == 0);
	CHECK(data.warnings.empty());

	CHECK(data.textureNormal == "buildings_NML.png");
	CHECK(data.textureMaterialGloss == "buildings_MG.png");
	CHECK(data.normalTranslucency);
	CHECK_FALSE(data.normalMetalness);
	CHECK(data.blendHash);
	CHECK(data.blendHashRatio == Approx(0.25f));
	CHECK(data.superRoughness == Approx(0.5f));
	CHECK(data.weatherMask == "../../textures/buildings_1_WM.png");
	CHECK(data.weatherTransparent);
	CHECK(data.hasSnowAlbedoLuma);
	CHECK(data.snowAlbedoLuma[0] == Approx(0.2f));
	CHECK(data.snowAlbedoLuma[3] == Approx(0.0f));
	CHECK(data.decalLibrary == "lib/g10/decals/bld_hangar_1.dcl");
	CHECK(data.textureControl == "../textures12/apt/ter_sfx_conc_CTL.png");
	CHECK(data.textureModulator == "../../textures/hangars_5_MOD.png");
	CHECK(data.textureTerrain == "../textures12/apt/apt_desert_01.png");
	CHECK(data.textureTerrainScale[0] == Approx(160.0f));

	REQUIRE(data.decals.size() == 2);
	CHECK(data.decals[0].directive == "DECAL_PARAMS");
	CHECK(data.decals[0].values.size() == 14);
	CHECK(data.decals[0].values[6] == Approx(0.5f));
	CHECK(data.decals[0].texture == "DECALS/DECAL CARPET.png");
	CHECK(data.decals[0].trailing.empty());
	CHECK(data.decals[1].directive == "NORMAL_DECAL_PARAMS");
	CHECK(data.decals[1].values.size() == 7);
	CHECK(data.decals[1].texture == "DECAL_GRAIN_DENSE_normal.png");
	REQUIRE(data.decals[1].trailing.size() == 1);
	CHECK(data.decals[1].trailing[0] == Approx(0.5f));

	// NORMAL_METALNESS as shipped misspelled, after the draped normal map too
	for (const char *spelling : { "NORMAL_METALNESS", "NORMAL_metalness", "NORMAL METALNESS" })
	{
		Obj8Data metal;
		REQUIRE(ParseObj8(std::string("I\n800\nOBJ\nTEXTURE_NORMAL n.png\n") + spelling + "\nTEXTURE_DRAPED_NORMAL 1.0 d.png\n" + spelling + "\n", metal));
		INFO(spelling);
		CHECK(metal.normalMetalness);
		CHECK(metal.drapedNormalMetalness);
		CHECK(metal.unknownCommandCount == 0);
	}
}

TEST_CASE("Lenient numbers accept the decimal comma some shipped files use", "[xplane][text]")
{
	CHECK(ParseFloatLenient("20,000000") == Approx(20.0f));
	CHECK(ParseFloatLenient("-0,5") == Approx(-0.5f));
	CHECK(ParseFloatLenient("1.25") == Approx(1.25f));
	CHECK(ParseFloatLenient("1,2,3", 7.0f) == Approx(7.0f));		// Not a number
	CHECK(ParseFloatLenient("1.5,0", 7.0f) == Approx(7.0f));		// Both separators: not a number
	CHECK(ParseFloatLenient("abc", 7.0f) == Approx(7.0f));
	CHECK(ParseFloat("20,000000", 7.0f) == Approx(7.0f));			// The strict parser stays strict
	CHECK(ToNativeRelativePath(R"(..\textures\a.png)") == ToNativeRelativePath("../textures/a.png"));
}

TEST_CASE("OBJ8 merges consecutive TRIS with the same state", "[xplane][obj8]")
{
	Obj8Data data;
	REQUIRE(ParseObj8("I\n800\nOBJ\nTEXTURE t.png\nVT 0 0 0 0 1 0 0 0\nIDX10 0 0 0 0 0 0 0 0 0 0\nTRIS 0 3\nTRIS 3 3\nTRIS 9 0\n", data));
	REQUIRE(data.batches.size() == 1);
	REQUIRE(data.batches[0].ranges.size() == 1);
	CHECK(data.batches[0].ranges[0].count == 6);
}

TEST_CASE("OBJ8 accepts static ANIM_trans without calibration values", "[xplane][obj8]")
{
	// XPlane2Blender writes static offsets with six values only.
	Obj8Data data;
	REQUIRE(ParseObj8("I\n800\nOBJ\nTEXTURE t.png\nANIM_begin\nANIM_trans 0 -0.55 0 0 -0.55 0\nANIM_end\n", data));
	REQUIRE(data.nodes.size() == 1);
	REQUIRE(data.nodes[0].ops.size() == 1);
	const RigidTransform t = EvaluateAnimNode(data.nodes[0], {});
	CHECK(t.translation[1] == Approx(-0.55f));
	CHECK(data.warnings.size() == 0);
}

TEST_CASE("OBJ8 conditionals follow the emulated render settings", "[xplane][obj8]")
{
	Obj8Data data;
	REQUIRE(ParseObj8("I\n800\nOBJ\nIF GLOBAL_LIGHTING\nTEXTURE hdr.png\nELSE\nTEXTURE ldr.png\nENDIF\nIF NOT GLOBAL_SHADOWS\nTEXTURE_LIT nope.png\nENDIF\n", data));
	CHECK(data.texture == "hdr.png");
	CHECK(data.textureLit.empty());
}

TEST_CASE("Animation ops interpolate, extrapolate and loop", "[xplane][obj8][animation]")
{
	Obj8AnimOp op;
	op.type = Obj8AnimOp::Type::Translate;
	op.dataref = DatarefRef::Parse("test/value");
	op.transKeys = { Obj8TransKey{ 0.0f, { 0.0f, 0.0f, 0.0f } }, Obj8TransKey{ 1.0f, { 0.0f, 2.0f, 0.0f } } };

	CHECK(EvaluateAnimOp(op, 0.5f).translation[1] == Approx(1.0f));
	CHECK(EvaluateAnimOp(op, 2.0f).translation[1] == Approx(4.0f));	// linear extrapolation past the last key
	CHECK(EvaluateAnimOp(op, -1.0f).translation[1] == Approx(-2.0f));

	op.loop = 1.0f;
	CHECK(EvaluateAnimOp(op, 1.25f).translation[1] == Approx(0.5f));
	CHECK(ApplyKeyframeLoop(-0.25f, 1.0f) == Approx(0.75f));
}

TEST_CASE("Rotations follow the right-hand rule and ops compose like a matrix stack", "[xplane][obj8][animation]")
{
	// +90 degrees about +Y turns +X into -Z.
	Obj8AnimOp rotate;
	rotate.type = Obj8AnimOp::Type::Rotate;
	rotate.axis[0] = 0.0f;
	rotate.axis[1] = 1.0f;
	rotate.axis[2] = 0.0f;
	rotate.rotKeys = { Obj8RotKey{ 0.0f, 90.0f } };
	const std::array<float, 3> p = Apply(EvaluateAnimOp(rotate, 0.0f), 1.0f, 0.0f, 0.0f);
	CHECK(p[0] == Approx(0.0f).margin(1e-5));
	CHECK(p[2] == Approx(-1.0f));

	// Pivot pattern: translate to the pivot, rotate, translate back. The pivot itself stays put.
	Obj8AnimNode node;
	Obj8AnimOp to;
	to.type = Obj8AnimOp::Type::Translate;
	to.transKeys = { Obj8TransKey{ 0.0f, { 2.0f, 0.0f, 0.0f } } };
	Obj8AnimOp back = to;
	back.transKeys = { Obj8TransKey{ 0.0f, { -2.0f, 0.0f, 0.0f } } };
	node.ops = { to, rotate, back };

	const RigidTransform pivoted = EvaluateAnimNode(node, {});
	const std::array<float, 3> pivot = Apply(pivoted, 2.0f, 0.0f, 0.0f);
	CHECK(pivot[0] == Approx(2.0f));
	CHECK(pivot[2] == Approx(0.0f).margin(1e-5));
	const std::array<float, 3> arm = Apply(pivoted, 3.0f, 0.0f, 0.0f);
	CHECK(arm[0] == Approx(2.0f));
	CHECK(arm[2] == Approx(-1.0f));
}

TEST_CASE("Hide and show rules evaluate in order with loops", "[xplane][obj8][animation]")
{
	Obj8Data data;
	REQUIRE(ParseObj8(ANIMATED_OBJ, data));
	const std::vector<uint32_t> &rules = data.batches[1].visibility;

	const auto at = [&](const float seconds) { return EvaluateVisibility(data, rules, [seconds](const DatarefRef &) { return seconds; }); };
	CHECK_FALSE(at(0.25f));	// in [0, 0.5]
	CHECK(at(0.75f));
	CHECK_FALSE(at(3.25f));	// looped by 1 s
	CHECK(at(3.9f));
}

TEST_CASE("Dataref values read once evaluate like a reader", "[xplane][obj8][animation]")
{
	Obj8Data data;
	REQUIRE(ParseObj8(R"(I
800
OBJ
TEXTURE t.png
VT 0 0 0 0 1 0 0 0
IDX10 0 0 0 0 0 0 0 0 0 0
ANIM_begin
ANIM_trans 0 0 0 0 0 0
ANIM_trans_begin test/slide[2]
ANIM_trans_key 0 0 0 0
ANIM_trans_key 1 0 3 0
ANIM_trans_end
ANIM_rotate_begin 0 1 0 test/turn
ANIM_rotate_key 0 0
ANIM_rotate_key 10 90
ANIM_rotate_end
ANIM_hide 0.25 0.5 test/slide[2]
TRIS 0 3
ANIM_end
)", data));
	REQUIRE(data.nodes.size() == 1);
	REQUIRE(data.nodes[0].ops.size() == 3);
	REQUIRE(data.datarefs.size() == 2);

	// Every op and rule with a dataref points at its datarefs entry; the static op at none
	const Obj8AnimNode &node = data.nodes[0];
	CHECK(node.ops[0].datarefIndex == -1);
	REQUIRE(node.ops[1].datarefIndex >= 0);
	REQUIRE(node.ops[2].datarefIndex >= 0);
	CHECK(data.datarefs[static_cast<size_t>(node.ops[1].datarefIndex)].dataref == DatarefRef::Parse("test/slide[2]"));
	CHECK(data.datarefs[static_cast<size_t>(node.ops[2].datarefIndex)].dataref == DatarefRef::Parse("test/turn"));
	REQUIRE(data.visibility.size() == 1);
	CHECK(data.visibility[0].datarefIndex == node.ops[1].datarefIndex);

	const DatarefReader reader = [](const DatarefRef &ref) { return ref.name == "test/turn" ? 5.0f : (ref.index == 2 ? 0.3f : 9.0f); };
	std::vector<float> values;
	ReadDatarefs(data, reader, values);
	REQUIRE(values.size() == data.datarefs.size());
	CHECK(values[static_cast<size_t>(node.ops[1].datarefIndex)] == Approx(0.3f));

	const RigidTransform byReader = EvaluateAnimNode(node, reader);
	const RigidTransform byValues = EvaluateAnimNodeAt(node, values);
	for (int i = 0; i < 4; ++i)
		CHECK(byValues.rotation[i] == byReader.rotation[i]);
	for (int i = 0; i < 3; ++i)
		CHECK(byValues.translation[i] == byReader.translation[i]);
	CHECK(byValues.translation[1] == Approx(0.9f));

	REQUIRE(data.batches.size() == 1);
	CHECK_FALSE(EvaluateVisibilityAt(data, data.batches[0].visibility, values));
	CHECK(EvaluateVisibilityAt(data, data.batches[0].visibility, values) == EvaluateVisibility(data, data.batches[0].visibility, reader));
	values[static_cast<size_t>(data.visibility[0].datarefIndex)] = 0.75f;
	CHECK(EvaluateVisibilityAt(data, data.batches[0].visibility, values));

	// Missing values read 0, like an unset dataref
	CHECK(EvaluateAnimNodeAt(node, {}).translation[1] == Approx(0.0f));
}

// -------------------------------------------------------
// Library system
// -------------------------------------------------------

TEST_CASE("library.txt parses exports, regions, seasons and visibility", "[xplane][library]")
{
	const char *text = R"(A
1200
LIBRARY

EXPORT lib/test/object.obj		objects/my object.obj
EXPORT_RATIO 2 lib/test/object.obj	objects/variant.obj
EXPORT_BACKUP lib/test/backup.obj	objects/backup.obj
REGION_DEFINE test_region
REGION_RECT -10 40 5 50
REGION test_region
EXPORT_EXCLUDE lib/test/object.obj	objects/regional.obj
PRIVATE
EXPORT_SEASON win,spr lib/test/tree.for	forests/cold.for
DEPRECATED
EXPORT lib/test/old.fac	facades/old.fac
PUBLIC 20240101
EXPORT lib/test/new.pol	polygons/new.pol
)";

	LibraryFile file;
	std::string error;
	REQUIRE(ParseLibrary(text, file, &error));
	CHECK(file.version == 1200);
	REQUIRE(file.exports.size() == 7);

	CHECK(file.exports[0].realPath == "objects/my object.obj"); // rest of the line: paths keep their spaces
	CHECK(file.exports[1].ratio == Approx(2.0f));
	CHECK(file.exports[2].kind == ExportKind::Backup);
	CHECK(file.exports[3].kind == ExportKind::Exclude);
	CHECK(file.exports[3].region == "test_region");
	CHECK(file.exports[4].seasons == (SeasonWinter | SeasonSpring));
	CHECK(file.exports[4].isPrivate);
	CHECK(file.exports[5].deprecated);
	CHECK_FALSE(file.exports[6].isPrivate);
	CHECK_FALSE(file.exports[6].deprecated);
	CHECK(file.exports[6].publicDate == 20240101u);

	REQUIRE(file.regions.size() == 1);
	CHECK(file.regions[0].Contains(45.5, 0.2));
	CHECK_FALSE(file.regions[0].Contains(30.0, 0.2));

	CHECK(GetAssetType("lib/test/tree.for") == AssetType::Forest);
	CHECK(GetAssetType("LIB/TEST/OBJECT.OBJ") == AssetType::Object);
	CHECK(FormatSeasons(SeasonWinter | SeasonSpring) == "spr,win");
}

TEST_CASE("scenery_packs.ini keeps priority order and resolves paths", "[xplane][library]")
{
	const char *text = "I\n1000 Version\nSCENERY\n\nSCENERY_PACK Custom Scenery/Airport A/\nSCENERY_PACK_DISABLED Custom Scenery/Old Thing/\n"
					   "SCENERY_PACK *GLOBAL_AIRPORTS*\nSCENERY_PACK C:\\Libraries\\Shared Lib/\n";
	std::vector<SceneryPackEntry> entries;
	REQUIRE(ParseSceneryPacks(text, "X:/XP12", entries));
	REQUIRE(entries.size() == 4);
	CHECK(entries[0].path.filename() == "Airport A");
	CHECK(entries[0].enabled);
	CHECK_FALSE(entries[1].enabled);
	CHECK(entries[2].isGlobalAirports);
	CHECK(entries[2].path.filename() == "Global Airports");
	CHECK(entries[3].rawPath == "C:\\Libraries\\Shared Lib/");
}

TEST_CASE("Library index applies package priority and backup exports", "[xplane][library]")
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "sedx_xplane_library_test";
	std::filesystem::remove_all(root);

	WriteFile(root / "high" / "library.txt", "I\n800\nLIBRARY\nEXPORT lib/shared/a.obj objects/a.obj\nEXPORT_BACKUP lib/shared/b.obj objects/b.obj\n");
	WriteFile(root / "high" / "objects" / "a.obj", "I\n800\nOBJ\n");
	WriteFile(root / "low" / "library.txt", "I\n800\nLIBRARY\nEXPORT lib/shared/a.obj objects/a.obj\nEXPORT lib/shared/b.obj objects/missing.obj\n");
	WriteFile(root / "low" / "objects" / "a.obj", "I\n800\nOBJ\n");

	std::vector<LibraryPackage> packages(2);
	packages[0].name = "high";
	packages[0].path = root / "high";
	packages[0].hasLibrary = true;
	packages[0].priority = 0;
	packages[1].name = "low";
	packages[1].path = root / "low";
	packages[1].hasLibrary = true;
	packages[1].priority = 1;

	LibraryIndex index;
	index.Build(packages, LibraryIndex::BuildOptions{ true });

	REQUIRE(index.GetAssets().size() == 2);
	CHECK(index.GetExportCount() == 4);
	CHECK(index.GetMissingFileCount() == 2); // high's backup b.obj and low's missing.obj

	const LibraryAsset *a = index.Find("lib/shared/a.obj");
	REQUIRE(a != nullptr);
	REQUIRE(a->GetResolved() != nullptr);
	CHECK(a->GetResolved()->package == 0); // the higher-priority package wins

	// A real EXPORT in a lower package beats a higher package's EXPORT_BACKUP.
	const LibraryAsset *b = index.Find("lib/shared/b.obj");
	REQUIRE(b != nullptr);
	REQUIRE(b->GetResolved() != nullptr);
	CHECK(b->GetResolved()->package == 1);
	CHECK(b->providers.front().kind == ExportKind::Backup);

	CHECK(index.GetPackages()[0].assetCount == 2);
	CHECK(index.Resolve("lib/shared/a.obj").filename() == "a.obj");
	CHECK(index.Resolve("lib/unknown.obj").empty());

	std::filesystem::remove_all(root);
}

// -------------------------------------------------------
// lights.txt
// -------------------------------------------------------

namespace
{
	// Excerpts of X-Plane 12's lights.txt (tabs, trailing comments and "cd" intensities as in the real file).
	constexpr const char *LIGHTS_TXT = "A\n850\nLIGHT_SPECS\n\n"
		"# comment\n"
		"TEXTURE 1000_lights_close.dds\n"
		"X_DIVISIONS 16\n"
		"Y_DIVISIONS 8\n"
		"BILLBOARD_HW\ttaxi_b\t\t0.34\t0.34\t9.50\t1.0\t\t15cd\t\t1 3 5\t0.0\t\t0.0\t\t0.0\t\t1.0\t\t0\t\t0\t\t0\t\t1\n"
		"SPILL_HW_DIR\ttaxi_b\t\t0.34\t0.34\t9.50 \t0.4 \t15cd\t\t\t\t0.0\t\t0.0\t\t0.0\t\t1.0\t\t\t\t\t\t\t\t1\n"
		"SPILL_GND\t\ttaxi_b_halo\t\t1\t1\t3\t3\n"
		"BILLBOARD_HW\tapt_beacon_green_axial\t\t0.05\t1.0\t\t0.6\t\t1.0\t\t95000cd\t\t2 6 0 \t0\t\t-0.087\t-0.996\t0.975\t0\t\t0\t\t0\t\t0\t\t# 5 dn\n"
		"BILLBOARD_HW\tapt_beacon_white_glow\t\t1.0\t\t1.0\t\t1.0\t\t0.8\t\t2.5\t\t\t1 3\t7\t0\t\t0\t\t-31\t\t-30\t\t0\t\t0\t\t0\t\t0\n"
		"LIGHT_PARAM_DEF\tapron_light_billboard 5\tDX\tDY\tDZ\tWIDTH\tINTENSITY\t# <--- params order inside the obj\n"
		"BILLBOARD_HW\tapron_light_billboard\t1.0\t0.90\t0.8\t1\tINTENSITY \t4 2 0\t\tDX\tDY\tDZ\tWIDTH\t0\t\t0\t\t0\t\t1\n"
		"LIGHT_PARAM_DEF\tairplane_taxi_core\t\t5\tZERO\tZERO_\tNEG_ONE\tINDEX\tSIZE\n"
		"BILLBOARD_SW\tairplane_taxi_core\t\tZERO\tZERO_\tNEG_ONE\tINDEX\tSIZE\t1\t6\t5\t0\t0\t-0.8\t0.2\tsim/graphics/animation/lights/airplane_taxi_light_flash\n"
		"BILLBOARD_SW\tappch_rabbit_o\t\t\t0.99\t0.97\t1.33\t1\t\t25000cd\t\t1 2\t6\t0\t\t0\t\t0\t\t1\t\t\t\t\t\t\t\t\t\tsim/graphics/animation/lights/rabbit\n";
}

TEST_CASE("lights.txt parses records, texture grid and parameter definitions", "[xplane][lights]")
{
	LightCatalog catalog;
	std::string error;
	REQUIRE(catalog.Parse(LIGHTS_TXT, &error));
	CHECK(catalog.GetTexture() == "1000_lights_close.dds");
	CHECK(catalog.GetDivisionsX() == 16);
	CHECK(catalog.GetDivisionsY() == 8);

	const LightDefinition *taxi = catalog.Find("taxi_b");
	REQUIRE(taxi != nullptr);
	REQUIRE(taxi->records.size() == 2);
	CHECK(taxi->records[0].type == LightRecordType::BillboardHW);
	CHECK(taxi->records[0].args.size() == 16);
	CHECK(taxi->records[1].type == LightRecordType::SpillHWDir);

	const LightDefinition *apron = catalog.Find("apron_light_billboard");
	REQUIRE(apron != nullptr);
	CHECK(apron->params == std::vector<std::string>{ "DX", "DY", "DZ", "WIDTH", "INTENSITY" });

	CHECK_FALSE(catalog.Parse("I\n800\nOBJ\n"));
}

TEST_CASE("Named lights resolve to billboards and spill", "[xplane][lights]")
{
	LightCatalog catalog;
	REQUIRE(catalog.Parse(LIGHTS_TXT));

	Obj8Light light;
	light.kind = Obj8Light::Kind::Named;
	light.name = "taxi_b";

	ResolvedLight resolved;
	REQUIRE(catalog.Resolve(light, resolved));
	REQUIRE(resolved.billboards.size() == 1);
	REQUIRE(resolved.spills.size() == 1);

	const LightBillboardSpec &billboard = resolved.billboards[0];
	CHECK(billboard.IsPhotometric());
	CHECK(billboard.intensity == Approx(15.0f));
	CHECK(billboard.color[2] == Approx(9.5f));
	CHECK(billboard.day);
	// cell "1 3 5": column 3, row 5 of a 16 x 8 grid, lower-left origin
	CHECK(billboard.uv[0] == Approx(3.0f / 16.0f));
	CHECK(billboard.uv[1] == Approx(5.0f / 8.0f));
	CHECK(billboard.uv[2] == Approx(4.0f / 16.0f));
	CHECK(billboard.uv[3] == Approx(6.0f / 8.0f));

	const LightSpillSpec &spill = resolved.spills[0];
	CHECK(spill.intensity == Approx(15.0f));
	CHECK(spill.color[3] == Approx(0.4f));
	CHECK_FALSE(spill.IsSpot());
	CHECK(spill.GetRange() > 1.0f);

	// Double-size cells count in units of their size: "2 6 0" is the 128 px cell at x = 768.
	light.name = "apt_beacon_green_axial";
	REQUIRE(catalog.Resolve(light, resolved));
	REQUIRE(resolved.billboards.size() == 1);
	CHECK(resolved.billboards[0].uv[0] == Approx(12.0f / 16.0f));
	CHECK(resolved.billboards[0].uv[3] == Approx(2.0f / 8.0f));
	CHECK(resolved.billboards[0].width == Approx(0.975f));

	light.name = "taxi_b_halo"; // ground quads only: nothing to draw, but known
	CHECK(catalog.Resolve(light, resolved));
	CHECK(resolved.billboards.empty());

	light.name = "no_such_light";
	std::string warning;
	CHECK_FALSE(catalog.Resolve(light, resolved, &warning));
	CHECK_FALSE(warning.empty());
}

TEST_CASE("LIGHT_PARAM arguments replace parameter names", "[xplane][lights]")
{
	LightCatalog catalog;
	REQUIRE(catalog.Parse(LIGHTS_TXT));

	Obj8Light light;
	light.kind = Obj8Light::Kind::Param;
	light.name = "apron_light_billboard";
	light.params = { 0.0f, -0.5f, 0.866f, 0.7f, 2000.0f };

	ResolvedLight resolved;
	REQUIRE(catalog.Resolve(light, resolved));
	REQUIRE(resolved.billboards.size() == 1);
	const LightBillboardSpec &billboard = resolved.billboards[0];
	CHECK(billboard.direction[1] == Approx(-0.5f));
	CHECK(billboard.direction[2] == Approx(0.866f));
	CHECK(billboard.width == Approx(0.7f));
	CHECK(billboard.intensity == Approx(2000.0f)); // INTENSITY is photometric
	CHECK(billboard.uv[0] == Approx(0.5f));		   // "4 2 0": the 256 px cell at x = 512

	// Hint names take the OBJ's values; INDEX in the alpha slot is a dataref index, not a dimmer.
	light.name = "airplane_taxi_core";
	light.params = { 0.0f, 0.0f, -1.0f, 2.0f, 1.5f };
	REQUIRE(catalog.Resolve(light, resolved));
	REQUIRE(resolved.billboards.size() == 1);
	CHECK(resolved.billboards[0].size == Approx(1.5f));
	CHECK(resolved.billboards[0].color[2] == Approx(-1.0f));
	CHECK(resolved.billboards[0].color[3] == Approx(1.0f));
	CHECK_FALSE(resolved.billboards[0].IsPhotometric());

	light.name = "appch_rabbit_o";
	light.params.clear();
	REQUIRE(catalog.Resolve(light, resolved));
	REQUIRE(resolved.billboards.size() == 1);
	CHECK(resolved.billboards[0].processor == LightProcessor::Rabbit);

	// The OBJ side writes intensities with a unit too ("150cd"); they keep their parameter slot.
	Obj8Data data;
	REQUIRE(ParseObj8("I\n800\nOBJ\nPOINT_COUNTS 0 0 0 0\nLIGHT_PARAM apron_light_billboard 1 2 3  0 -0.5 0.866 0.7 150cd\n", data));
	REQUIRE(data.lights.size() == 1);
	CHECK(data.lights[0].params == std::vector<float>{ 0.0f, -0.5f, 0.866f, 0.7f, 150.0f });
	CHECK(data.lights[0].extraArgs.empty());
	REQUIRE(catalog.Resolve(data.lights[0], resolved));
	CHECK(resolved.billboards[0].intensity == Approx(150.0f));
}

TEST_CASE("Custom and legacy lights resolve from their own arguments", "[xplane][lights]")
{
	LightCatalog catalog;
	REQUIRE(catalog.Parse(LIGHTS_TXT));

	Obj8Light custom;
	custom.kind = Obj8Light::Kind::Custom;
	custom.params = { 1.0f, 0.5f, 0.25f, 1.0f, 3.0f, 0.0f, 0.5f, 0.25f, 1.0f };
	ResolvedLight resolved;
	REQUIRE(catalog.Resolve(custom, resolved));
	REQUIRE(resolved.billboards.size() == 1);
	CHECK(resolved.billboards[0].customTexture);
	CHECK(resolved.billboards[0].size == Approx(3.0f));
	CHECK(resolved.billboards[0].uv[1] == Approx(0.5f));
	CHECK(resolved.billboards[0].uv[2] == Approx(0.25f));

	Obj8Light spill;
	spill.kind = Obj8Light::Kind::SpillCustom;
	spill.params = { 1.0f, 1.0f, 1.0f, 1.0f, 12.0f, 0.0f, -1.0f, 0.0f, 0.5f };
	REQUIRE(catalog.Resolve(spill, resolved));
	REQUIRE(resolved.spills.size() == 1);
	CHECK(resolved.spills[0].IsSpot());
	CHECK(resolved.spills[0].GetRange() == Approx(12.0f));

	Obj8Light legacy;
	legacy.kind = Obj8Light::Kind::Legacy;
	legacy.params = { 1.0f, 0.0f, 0.0f };
	REQUIRE(catalog.Resolve(legacy, resolved));
	REQUIRE(resolved.billboards.size() == 1);
	CHECK(resolved.billboards[0].color[0] == Approx(1.0f));
	CHECK(resolved.billboards[0].color[1] == Approx(0.0f));
}

TEST_CASE("Light flashing, directionality and size follow lights.txt rules", "[xplane][lights]")
{
	// Steady, triangle, strobe (amplitude > 1), floor (amplitude < 1) and square wave (amplitude > 1000)
	CHECK(EvaluateFlash(0.0f, 0.0f, 0.0f, 12.3) == Approx(1.0f));
	CHECK(EvaluateFlash(1.0f, 0.0f, 1.0f, 0.5) == Approx(1.0f));
	CHECK(EvaluateFlash(1.0f, 0.0f, 1.0f, 0.0) == Approx(0.0f));
	CHECK(EvaluateFlash(1.0f, 0.0f, 10.0f, 0.25) == Approx(0.0f));
	CHECK(EvaluateFlash(1.0f, 0.0f, 10.0f, 0.5) == Approx(1.0f));
	CHECK(EvaluateFlash(1.0f, 0.0f, 0.5f, 0.0) == Approx(0.5f));
	CHECK(EvaluateFlash(1.0f, 0.0f, 1500.0f, 0.25) == Approx(1.0f));
	CHECK(EvaluateFlash(1.0f, 0.0f, 1500.0f, 0.75) == Approx(0.0f));
	CHECK(EvaluateFlash(1.0f, 0.25f, 1.0f, 0.25) == Approx(1.0f)); // phase in seconds

	// Photometric beam: width is the cosine of the half angle.
	LightBillboardSpec beam;
	beam.intensity = 1500.0f;
	beam.width = 0.99f;
	const float axis[3] = { 0.0f, 0.0f, 1.0f };
	const float onAxis[3] = { 0.0f, 0.0f, 1.0f };
	const float offAxis[3] = { 0.0f, std::sin(0.3f), std::cos(0.3f) }; // ~17 degrees
	CHECK(EvaluateBillboardDirectionality(beam, axis, onAxis) == Approx(1.0f));
	CHECK(EvaluateBillboardDirectionality(beam, axis, offAxis) == Approx(0.0f));

	// Legacy: pre-scaled direction plus a constant term
	LightBillboardSpec glow;
	glow.width = -30.0f;
	const float scaled[3] = { 0.0f, 0.0f, -31.0f };
	const float front[3] = { 0.0f, 0.0f, -1.0f };
	const float side[3] = { 1.0f, 0.0f, 0.0f };
	CHECK(EvaluateBillboardDirectionality(glow, scaled, front) == Approx(1.0f));
	CHECK(EvaluateBillboardDirectionality(glow, scaled, side) == Approx(0.0f));

	// Omnidirectional
	const LightBillboardSpec omni;
	const float zero[3] = { 0.0f, 0.0f, 0.0f };
	CHECK(EvaluateBillboardDirectionality(omni, zero, side) == Approx(1.0f));

	// Billboards grow slower than distance, and brighter lights are bigger.
	LightBillboardSpec dim, bright;
	dim.intensity = 15.0f;
	bright.intensity = 95000.0f;
	CHECK(GetBillboardRadius(dim, 1000.0f) / 1000.0f < GetBillboardRadius(dim, 10.0f) / 10.0f);
	CHECK(GetBillboardRadius(bright, 100.0f) > GetBillboardRadius(dim, 100.0f));

	CHECK(GetLightProcessor("sim/graphics/animation/lights/vasi_papi") == LightProcessor::Vasi);
	CHECK(GetLightProcessor("sim/graphics/animation/lights/strobe") == LightProcessor::Strobe);
	CHECK(GetLightProcessor("NULL") == LightProcessor::None);
}

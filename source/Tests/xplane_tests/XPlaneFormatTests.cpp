/**
 * -------------------------------------------------------
 * Scenery Editor X - Unit Tests
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * XPlaneFormatTests.cpp
 * -------------------------------------------------------
 * OBJ8 parsing and animation, library.txt / scenery_packs.ini parsing and the library index's priority rules.
 * Only the standard library is involved, so this suite also builds on the headless Linux CI image.
 * -------------------------------------------------------
 */
#include <catch2/catch_all.hpp>
#include <SceneryEditorX/xplane/xp_library.h>
#include <SceneryEditorX/xplane/xp_obj8.h>
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

/**
 * -------------------------------------------------------
 * Scenery Editor X - Fuzzing
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * fuzz_obj8.cpp
 * -------------------------------------------------------
 * OBJ8 models (xp_obj8): parsing, then what the object loader and the animation tick do with the result. The loader
 * indexes vertices with the parsed indices and batch ranges without checking them again, so the harness holds the
 * parser to that: every index, range, animation group, LOD and visibility rule must point inside the model.
 * -------------------------------------------------------
 */
#include "common/fuzz_common.h"
#include <SceneryEditorX/xplane/xp_obj8.h>
#include <cmath>
#include <limits>

using namespace SceneryEditorX::XPlane;

namespace
{
	void CheckReferences(const Obj8Data &obj, const int node, const int lod, const std::vector<uint32_t> &visibility)
	{
		SEDX_FUZZ_CHECK(node >= -1 && node < static_cast<int>(obj.nodes.size()), "animation group " + std::to_string(node));
		SEDX_FUZZ_CHECK(lod >= -1 && lod < static_cast<int>(obj.lods.size()), "LOD " + std::to_string(lod));
		for (const uint32_t rule : visibility)
			SEDX_FUZZ_CHECK(rule < obj.visibility.size(), "visibility rule " + std::to_string(rule));
	}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, const size_t size)
{
	Obj8Data obj;
	std::string error;
	if (!ParseObj8(SceneryEditorX::Fuzz::AsText(data, size), obj, &error))
		return 0;

	for (const uint32_t index : obj.indices)
		SEDX_FUZZ_CHECK(index < obj.vertices.size(), "index " + std::to_string(index) + " of " + std::to_string(obj.vertices.size()) + " vertices");

	for (const Obj8Batch &batch : obj.batches)
	{
		for (const Obj8Batch::Range &range : batch.ranges)
		{
			SEDX_FUZZ_CHECK(static_cast<uint64_t>(range.offset) + range.count <= obj.indices.size(),
				"TRIS range " + std::to_string(range.offset) + "+" + std::to_string(range.count) + " of " + std::to_string(obj.indices.size()) + " indices");
			SEDX_FUZZ_CHECK(range.count % 3 == 0, "TRIS range of " + std::to_string(range.count) + " indices");
		}
		CheckReferences(obj, batch.node, batch.lod, batch.visibility);
	}
	for (const Obj8Light &light : obj.lights)
		CheckReferences(obj, light.node, light.lod, light.visibility);
	for (size_t i = 0; i < obj.nodes.size(); ++i)
		SEDX_FUZZ_CHECK(obj.nodes[i].parent >= -1 && obj.nodes[i].parent < static_cast<int>(i), "parent of group " + std::to_string(i));

	// What SceneObjects::Tick evaluates every frame, at ordinary and extreme dataref values
	for (const float value : { 0.0f, 0.5f, 1.0f, -360.0f, 1.0e6f, -1.0e30f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN() })
	{
		const DatarefReader reader = [value](const DatarefRef &) { return value; };
		for (const Obj8AnimNode &node : obj.nodes)
			(void)EvaluateAnimNode(node, reader);
		for (const Obj8Batch &batch : obj.batches)
			(void)EvaluateVisibility(obj, batch.visibility, reader);
	}
	(void)obj.GetTriangleCount();
	(void)obj.HasAdditiveLods();
	return 0;
}

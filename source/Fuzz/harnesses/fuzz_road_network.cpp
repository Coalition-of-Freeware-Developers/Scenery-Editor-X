/**
 * -------------------------------------------------------
 * Scenery Editor X - Fuzzing
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * fuzz_road_network.cpp
 * -------------------------------------------------------
 * Road networks (.net) (xp_assets ParseRoadNetworkAsset, xp_overlay): parsing, then drawing the asset the way a DSF tile places it.
 * -------------------------------------------------------
 */
#include "common/fuzz_assets.h"
#include "common/fuzz_common.h"
#include <SceneryEditorX/xplane/xp_assets.h>

using namespace SceneryEditorX::XPlane;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, const size_t size)
{
	RoadNetworkAsset asset;
	std::string error;
	if (!ParseRoadNetworkAsset(SceneryEditorX::Fuzz::AsText(data, size), asset, &error))
		return 0;
	SceneryEditorX::Fuzz::BuildPlacedAsset(".net", data, size);
	return 0;
}

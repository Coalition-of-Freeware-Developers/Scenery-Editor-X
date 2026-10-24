/**
 * -------------------------------------------------------
 * Scenery Editor X - Fuzzing
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * fuzz_polygon.cpp
 * -------------------------------------------------------
 * Draped polygons (.pol) (xp_assets ParsePolygonAsset, xp_overlay): parsing, then drawing the asset the way a DSF tile places it.
 * -------------------------------------------------------
 */
#include "common/fuzz_assets.h"
#include "common/fuzz_common.h"
#include <SceneryEditorX/xplane/xp_assets.h>

using namespace SceneryEditorX::XPlane;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, const size_t size)
{
	PolygonAsset asset;
	std::string error;
	if (!ParsePolygonAsset(SceneryEditorX::Fuzz::AsText(data, size), asset, &error))
		return 0;
	SceneryEditorX::Fuzz::BuildPlacedAsset(".pol", data, size);
	return 0;
}

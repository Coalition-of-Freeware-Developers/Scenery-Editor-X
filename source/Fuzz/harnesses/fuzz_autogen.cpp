/**
 * -------------------------------------------------------
 * Scenery Editor X - Fuzzing
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * fuzz_autogen.cpp
 * -------------------------------------------------------
 * Autogen points (.agp) (xp_assets ParseAutogenPointAsset, xp_overlay): parsing, then drawing the asset the way a DSF tile places it.
 * -------------------------------------------------------
 */
#include "common/fuzz_assets.h"
#include "common/fuzz_common.h"
#include <SceneryEditorX/xplane/xp_assets.h>

using namespace SceneryEditorX::XPlane;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, const size_t size)
{
	AutogenPointAsset asset;
	std::string error;
	if (!ParseAutogenPointAsset(SceneryEditorX::Fuzz::AsText(data, size), asset, &error))
		return 0;
	SceneryEditorX::Fuzz::BuildPlacedAsset(".agp", data, size);
	return 0;
}

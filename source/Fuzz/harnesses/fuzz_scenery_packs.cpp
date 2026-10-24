/**
 * -------------------------------------------------------
 * Scenery Editor X - Fuzzing
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * fuzz_scenery_packs.cpp
 * -------------------------------------------------------
 * Custom Scenery/scenery_packs.ini (xp_library ParseSceneryPacks): X-Plane's scenery priority list, entries resolved
 * against an X-Plane folder (relative, absolute, Windows and POSIX spellings, *GLOBAL_AIRPORTS*).
 * -------------------------------------------------------
 */
#include "common/fuzz_assets.h"
#include "common/fuzz_common.h"
#include <SceneryEditorX/xplane/xp_library.h>

using namespace SceneryEditorX::XPlane;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, const size_t size)
{
	std::vector<SceneryPackEntry> entries;
	std::string error;
	if (!ParseSceneryPacks(SceneryEditorX::Fuzz::AsText(data, size), SceneryEditorX::Fuzz::GetAssetPack() / "X-Plane 12", entries, &error))
		return 0;
	for (const SceneryPackEntry &entry : entries)
	{
		(void)entry.path.filename();
		(void)entry.path.lexically_normal();
	}
	return 0;
}

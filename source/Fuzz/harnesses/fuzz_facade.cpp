/**
 * -------------------------------------------------------
 * Scenery Editor X - Fuzzing
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * fuzz_facade.cpp
 * -------------------------------------------------------
 * Facades (.fac: xp_assets ParseFacadeAsset, xp_facade, xp_facade_writer, xp_overlay): parsing, the writer's promise
 * (what it writes parses, and writing that again gives the same text), X-Plane's floor / wall / spelling choices for a
 * few footprints, and drawing the facade the way a DSF tile places it.
 * -------------------------------------------------------
 */
#include "common/fuzz_assets.h"
#include "common/fuzz_common.h"
#include <SceneryEditorX/xplane/xp_assets.h>
#include <SceneryEditorX/xplane/xp_facade.h>
#include <SceneryEditorX/xplane/xp_facade_writer.h>

using namespace SceneryEditorX::XPlane;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, const size_t size)
{
	FacadeAsset facade;
	std::string error;
	if (!ParseFacadeAsset(SceneryEditorX::Fuzz::AsText(data, size), facade, &error))
		return 0;

	const std::string written = WriteFacadeAsset(facade);
	FacadeAsset reread;
	SEDX_FUZZ_CHECK(ParseFacadeAsset(written, reread, &error), "written facade doesn't parse: " + error);
	SEDX_FUZZ_CHECK(WriteFacadeAsset(reread) == written, "facade text changes on a second round trip");

	// What the facade tool and the scene ask while laying out walls
	(void)GetFacadeFamily(facade);
	(void)GetFacadeWallNames(facade);
	const int walls = GetFacadeWallCount(facade);
	const FacadeHeights heights = GetFacadeHeights(facade);
	(void)DescribeFacadeHeights(heights);
	for (const double height : { 0.0, 3.0, 10.0, 75.0, -5.0 })
	{
		(void)SnapFacadeHeight(facade, height);
		(void)ChooseFacadeFloor(facade, height);
	}
	for (int wall = 0; wall < walls && wall < 4; ++wall)
	{
		(void)DescribeFacadeWall(facade, wall);
		const std::vector<double> natural = GetFacadeNaturalLengths(facade, 0, wall);
		(void)SnapFacadeLength(natural, 17.3);
	}

	const std::vector<FacadeSideInput> rectangle = { { 20.0, 0.0, 0 }, { 10.0, 90.0, 1 }, { 20.0, 180.0, 2 }, { 10.0, 270.0, 3 } };
	const std::vector<FacadeSideInput> odd = { { 0.0, 45.0, -1 }, { 1500.0, 361.0, 99 }, { 0.25, -720.0, 0 } };
	for (const bool pickWalls : { false, true })
	{
		(void)LayoutFacade(facade, rectangle, pickWalls, 9.0);
		(void)LayoutFacade(facade, odd, pickWalls, 120.0);
	}

	SceneryEditorX::Fuzz::BuildPlacedAsset(".fac", data, size);
	return 0;
}

/**
 * -------------------------------------------------------
 * Scenery Editor X - Fuzzing
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * fuzz_dsf.cpp
 * -------------------------------------------------------
 * DSF tiles (xp_dsf, xp_overlay): the binary reader (atoms, pools, the command stream), drawing every placement
 * through the overlay builder around the tile the way the airport scene does (an area of interest around the
 * datum), writing the tile back (what we write must read back with the same placements) and the per-tile split.
 * -------------------------------------------------------
 */
#include "common/fuzz_assets.h"
#include "common/fuzz_common.h"
#include <SceneryEditorX/xplane/xp_apt_geometry.h>
#include <SceneryEditorX/xplane/xp_dsf.h>
#include <SceneryEditorX/xplane/xp_overlay.h>
#include <algorithm>
#include <cmath>

using namespace SceneryEditorX::XPlane;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, const size_t size)
{
	DsfFile dsf;
	std::string error;
	if (!ReadDsf(data, size, dsf, &error))
		return 0;

	(void)dsf.IsOverlay();
	(void)dsf.GetExclusions();

	// Datum: the middle of the tile, else the first object (fuzzed bounds can be anything)
	double latitude = 0.5 * (dsf.south + dsf.north), longitude = 0.5 * (dsf.west + dsf.east);
	if (!std::isfinite(latitude) || !std::isfinite(longitude) || std::abs(latitude) > 90.0 || std::abs(longitude) > 180.0)
	{
		latitude = longitude = 0.0;
		if (!dsf.objects.empty() && std::isfinite(dsf.objects[0].latitude) && std::isfinite(dsf.objects[0].longitude))
		{
			latitude = std::clamp(dsf.objects[0].latitude, -90.0, 90.0);
			longitude = std::clamp(dsf.objects[0].longitude, -180.0, 180.0);
		}
	}

	OverlayBuilder builder(SceneryEditorX::Fuzz::MakeAssetResolver(), GeoProjection(latitude, longitude));
	OverlayBuilder::Options options;
	options.area.Add(Point2{ -3000.0, -3000.0 });
	options.area.Add(Point2{ 3000.0, 3000.0 });
	OverlayBuild build;
	builder.Build(dsf, options, build);
	SceneryEditorX::Fuzz::CheckOverlay(build);

	// Export: a tile we can write has to read back with the same content
	std::vector<uint8_t> bytes;
	if (WriteDsf(dsf, bytes, &error))
	{
		DsfFile reread;
		SEDX_FUZZ_CHECK(ReadDsf(bytes.data(), bytes.size(), reread, &error), "written DSF doesn't read back: " + error);
		SEDX_FUZZ_CHECK(reread.objects.size() == dsf.objects.size(), std::to_string(dsf.objects.size()) + " objects written, " + std::to_string(reread.objects.size()) + " read back");
		SEDX_FUZZ_CHECK(reread.polygons.size() == dsf.polygons.size(), std::to_string(dsf.polygons.size()) + " polygons written, " + std::to_string(reread.polygons.size()) + " read back");
		SEDX_FUZZ_CHECK(reread.roads.size() == dsf.roads.size(), std::to_string(dsf.roads.size()) + " road segments written, " + std::to_string(reread.roads.size()) + " read back");
	}

	std::vector<std::string> warnings;
	(void)SplitDsfByTile(dsf, &warnings);
	return 0;
}

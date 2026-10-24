/**
 * -------------------------------------------------------
 * Scenery Editor X - Fuzzing
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * fuzz_edx.cpp
 * -------------------------------------------------------
 * The JSON project format (.edX: project_edx) and what the editor does with a loaded project: saving it again (the
 * format is kept in git, so a save of what was loaded must load back and a second save must not change a byte), the
 * binary twin, and the project -> DSF conversion the viewport draws through the overlay builder and the exporter
 * writes as a DSF tile.
 * -------------------------------------------------------
 */
#include "common/fuzz_assets.h"
#include "common/fuzz_common.h"
#include <SceneryEditorX/project/project_dsf.h>
#include <SceneryEditorX/project/project_edx.h>
#include <SceneryEditorX/project/project_sex.h>
#include <SceneryEditorX/xplane/xp_dsf.h>
#include <SceneryEditorX/xplane/xp_overlay.h>
#include <cmath>

using namespace SceneryEditorX;
using namespace SceneryEditorX::ProjectFile;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, const size_t size)
{
	SceneryProject project;
	std::vector<std::string> warnings;
	std::string error;
	if (!ParseEdx(Fuzz::AsText(data, size), project, &warnings, &error))
		return 0;
	(void)project.CountItems();

	const std::string written = WriteEdx(project);
	SceneryProject reread;
	SEDX_FUZZ_CHECK(ParseEdx(written, reread, nullptr, &error), "written .edX doesn't load: " + error);
	SEDX_FUZZ_CHECK(WriteEdx(reread) == written, ".edX text changes on a second save");

	const std::string sex = WriteSex(project);
	SceneryProject fromSex;
	SEDX_FUZZ_CHECK(ParseSex(sex.data(), sex.size(), fromSex, nullptr, &error), "the .edX project doesn't save as loadable .sex: " + error);

	// What the viewport draws and the exporter writes
	DsfConversionOptions options;
	options.includeObjects = true;
	const DsfConversion conversion = BuildProjectDsf(project, options);
	SEDX_FUZZ_CHECK(conversion.objectItems.size() == conversion.dsf.objects.size(), "object items don't match the DSF objects");
	SEDX_FUZZ_CHECK(conversion.polygonItems.size() == conversion.dsf.polygons.size(), "polygon items don't match the DSF polygons");
	SEDX_FUZZ_CHECK(conversion.roadItems.size() == conversion.dsf.roads.size(), "road items don't match the DSF roads");

	double latitude = project.datum.latitude, longitude = project.datum.longitude;
	if (!std::isfinite(latitude) || !std::isfinite(longitude) || std::abs(latitude) > 90.0 || std::abs(longitude) > 180.0)
		latitude = longitude = 0.0;
	XPlane::OverlayBuilder builder(Fuzz::MakeAssetResolver(), XPlane::GeoProjection(latitude, longitude));
	XPlane::OverlayBuilder::Options overlay;
	overlay.area.Add(XPlane::Point2{ -3000.0, -3000.0 });
	overlay.area.Add(XPlane::Point2{ 3000.0, 3000.0 });
	XPlane::OverlayBuild build;
	builder.Build(conversion.dsf, overlay, build);
	Fuzz::CheckOverlay(build);

	XPlane::DsfFile tile = conversion.dsf;
	tile.properties.insert(tile.properties.begin(), { { "sim/west", "-180" }, { "sim/south", "-90" }, { "sim/east", "180" }, { "sim/north", "90" } });
	std::vector<uint8_t> bytes;
	if (XPlane::WriteDsf(tile, bytes, &error))
	{
		XPlane::DsfFile back;
		SEDX_FUZZ_CHECK(XPlane::ReadDsf(bytes.data(), bytes.size(), back, &error), "the project's DSF doesn't read back: " + error);
	}
	return 0;
}

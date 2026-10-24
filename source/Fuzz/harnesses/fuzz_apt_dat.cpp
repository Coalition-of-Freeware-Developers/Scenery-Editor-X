/**
 * -------------------------------------------------------
 * Scenery Editor X - Fuzzing
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * fuzz_apt_dat.cpp
 * -------------------------------------------------------
 * apt.dat (xp_apt, xp_apt_geometry, xp_overlay): parsing a whole file or an index excerpt, writing it back (what we
 * write must read back, and a second round trip must not change it), validation, and the geometry the airport scene
 * builds: flattened bezier chains, triangulated pavement, line ribbons, runway rectangles and the library-textured
 * surfaces.
 * -------------------------------------------------------
 */
#include "common/fuzz_assets.h"
#include "common/fuzz_common.h"
#include <SceneryEditorX/xplane/xp_apt.h>
#include <SceneryEditorX/xplane/xp_apt_geometry.h>
#include <SceneryEditorX/xplane/xp_overlay.h>

using namespace SceneryEditorX::XPlane;

namespace
{
	constexpr size_t MAX_AIRPORTS = 8;		// Geometry for the first few airports keeps each run fast

	void BuildGeometry(const AptAirport &airport)
	{
		double latitude = 0.0, longitude = 0.0;
		if (!airport.GetDatum(latitude, longitude))
			return;		// The scene draws nothing without a datum either
		const GeoProjection projection(latitude, longitude);

		std::vector<Point2> points;
		std::vector<uint32_t> indices;
		for (const AptPavement &pavement : airport.pavements)
		{
			std::vector<std::vector<Point2>> rings;
			for (const AptNodeChain &ring : pavement.rings)
				FlattenChain(ring, projection, rings.emplace_back());
			if (!rings.empty() && TriangulatePolygon(rings, points, indices))
			{
				SEDX_FUZZ_CHECK(indices.size() % 3 == 0, "pavement triangulation");
				for (const uint32_t index : indices)
					SEDX_FUZZ_CHECK(index < points.size(), "pavement index " + std::to_string(index) + " of " + std::to_string(points.size()));
			}
		}
		for (const AptLinearFeature &feature : airport.linearFeatures)
		{
			for (const AptNodeChain &chain : feature.chains)
			{
				std::vector<Point2> path;
				FlattenChain(chain, projection, path);
				BuildPolylineStrip(path, chain.closed, 0.3, points, indices);
				for (const uint32_t index : indices)
					SEDX_FUZZ_CHECK(index < points.size(), "line strip index " + std::to_string(index) + " of " + std::to_string(points.size()));
			}
		}
		for (const AptBoundary &boundary : airport.boundaries)
		{
			for (const AptNodeChain &ring : boundary.rings)
			{
				std::vector<Point2> path;
				FlattenChain(ring, projection, path);
				(void)SignedArea(path);
				(void)GetBounds(path);
			}
		}

		Point2 corners[4];
		for (const AptRunway &runway : airport.runways)
		{
			GetRunwayCorners(runway, projection, corners, false);
			GetRunwayCorners(runway, projection, corners, true);
		}
		for (const AptWaterRunway &runway : airport.waterRunways)
			GetWaterRunwayCorners(runway, projection, corners);
		for (const AptHelipad &helipad : airport.helipads)
			GetHelipadCorners(helipad, projection, corners);

		// The library-textured pass, the way BuildAirport stacks the pavement heights
		OverlayBuilder builder(SceneryEditorX::Fuzz::MakeAssetResolver(), projection);
		std::vector<float> heights(airport.pavements.size());
		for (size_t i = 0; i < heights.size(); ++i)
			heights[i] = 0.3f - 0.001f * static_cast<float>(i);
		OverlayBuild build;
		builder.BuildAirportSurfaces(airport, heights, 0.31f, 0.32f, OverlayBuilder::Options{}, build);
		SceneryEditorX::Fuzz::CheckOverlay(build);
	}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, const size_t size)
{
	const std::string_view text = SceneryEditorX::Fuzz::AsText(data, size);

	AptFile file;
	std::string error;
	if (!ParseAptDat(text, file, &error))
	{
		// The airport index hands the parser excerpts that start at an airport header
		AptParseOptions excerpt;
		excerpt.requireHeader = false;
		if (!ParseAptDat(text, file, &error, excerpt))
			return 0;
	}

	// What we write has to read back, and settle: writing what we read back gives the same text
	const std::string written = WriteAptDat(file);
	AptFile reread;
	SEDX_FUZZ_CHECK(ParseAptDat(written, reread, &error), "written apt.dat doesn't parse: " + error);
	SEDX_FUZZ_CHECK(reread.airports.size() == file.airports.size(),
		std::to_string(file.airports.size()) + " airports written, " + std::to_string(reread.airports.size()) + " read back");
	SEDX_FUZZ_CHECK(WriteAptDat(reread) == written, "apt.dat changes on a second round trip");

	for (size_t i = 0; i < file.airports.size() && i < MAX_AIRPORTS; ++i)
	{
		const AptAirport &airport = file.airports[i];
		(void)ValidateAptAirport(airport);
		(void)airport.GetElementCount();
		BuildGeometry(airport);
	}
	return 0;
}

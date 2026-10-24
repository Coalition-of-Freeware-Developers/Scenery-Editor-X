/**
 * -------------------------------------------------------
 * Scenery Editor X - Fuzzing
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * fuzz_coordinate.cpp
 * -------------------------------------------------------
 * What the New Airport Project form types into (scenery_pack): coordinates as decimal degrees with an optional sign
 * or hemisphere letter. A value the parser accepts must be in range, and its formatted text must parse back to text
 * that formats the same way.
 * -------------------------------------------------------
 */
#include "common/fuzz_common.h"
#include <SceneryEditorX/project/scenery_pack.h>
#include <cmath>

using namespace SceneryEditorX::ProjectFile;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, const size_t size)
{
	const std::string_view text = SceneryEditorX::Fuzz::AsText(data, size);
	for (const bool latitude : { true, false })
	{
		double value = 0.0;
		std::string error;
		if (!ParseCoordinate(text, latitude, value, &error))
			continue;

		const double limit = latitude ? 90.0 : 180.0;
		SEDX_FUZZ_CHECK(std::isfinite(value) && std::abs(value) <= limit, "accepted " + std::string(text) + " as " + std::to_string(value));

		const std::string formatted = FormatCoordinate(value);
		double reread = 0.0;
		SEDX_FUZZ_CHECK(ParseCoordinate(formatted, latitude, reread, &error), "formatted " + formatted + " doesn't parse: " + error);
		SEDX_FUZZ_CHECK(FormatCoordinate(reread) == formatted, formatted + " formats as " + FormatCoordinate(reread) + " after a round trip");
	}
	return 0;
}

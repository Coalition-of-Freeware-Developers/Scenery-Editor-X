/**
 * -------------------------------------------------------
 * Scenery Editor X - Fuzzing
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * fuzz_gateway_json.cpp
 * -------------------------------------------------------
 * Scenery Gateway API responses (xp_gateway_types): every record parser on the same text (the client can't know a
 * reply is well-formed), the base64 decoder, and the zip a scenery reply carries when zlib is available.
 * -------------------------------------------------------
 */
#include "common/fuzz_common.h"
#include <SceneryEditorX/xplane/xp_gateway_types.h>
#ifdef SEDX_FUZZ_HAS_ZLIB
	#include <SceneryEditorX/xplane/xp_zip.h>
#endif

using namespace SceneryEditorX::XPlane;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, const size_t size)
{
	const std::string_view json = SceneryEditorX::Fuzz::AsText(data, size);
	std::string error;

	std::vector<GatewayAirport> airports;
	(void)ParseGatewayAirports(json, airports, &error);

	GatewayAirportDetails details;
	if (ParseGatewayAirport(json, details, &error))
	{
		for (const GatewayScenery &scenery : details.scenery)
			(void)GatewayScenery::ShortDate(scenery.dateUploaded);
	}

	GatewaySceneryPack pack;
	if (ParseGatewayScenery(json, pack, &error))
	{
#ifdef SEDX_FUZZ_HAS_ZLIB
		std::vector<ZipEntry> entries;
		(void)ReadZipArchive(pack.masterZip, entries, &error);
#endif
	}

	GatewayStats stats;
	(void)ParseGatewayStats(json, stats, &error);
	std::vector<GatewayRelease> releases;
	(void)ParseGatewayReleases(json, releases, &error);

	std::vector<uint8_t> decoded;
	(void)DecodeBase64(json, decoded);
	return 0;
}

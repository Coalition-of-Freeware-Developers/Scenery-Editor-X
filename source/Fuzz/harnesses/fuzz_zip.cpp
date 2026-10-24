/**
 * -------------------------------------------------------
 * Scenery Editor X - Fuzzing
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * fuzz_zip.cpp
 * -------------------------------------------------------
 * Zip archives (xp_zip ReadZipArchive): the Scenery Gateway sends scenery as a zip holding another zip the artist
 * uploaded, so both the reader and the names inside are hostile input. Every entry name the install guard
 * (VFS::ResolveInside) accepts must land inside the pack folder; nested archives are read the way UnpackScenery does.
 * -------------------------------------------------------
 */
#include "common/fuzz_common.h"
#include <SceneryEditorX/filesystem/vfs.h>
#include <SceneryEditorX/xplane/xp_zip.h>

using namespace SceneryEditorX;
namespace fs = std::filesystem;

namespace
{
	void CheckNames(const std::vector<XPlane::ZipEntry> &entries)
	{
		const fs::path pack = fs::path("Custom Scenery") / "Fuzzed Pack";
		for (const XPlane::ZipEntry &entry : entries)
		{
			fs::path target;
			if (VFS::ResolveInside(pack, entry.name, target))
				SEDX_FUZZ_CHECK(VFS::IsInside(target, pack), "zip entry '" + entry.name + "' lands at " + VFS::ToUtf8(target));
		}
	}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, const size_t size)
{
	std::vector<XPlane::ZipEntry> entries;
	std::string error;
	if (!XPlane::ReadZipArchive(data, size, entries, &error))
		return 0;
	CheckNames(entries);

	// <ICAO>_Scenery_Pack.zip inside the Gateway's master zip
	for (const XPlane::ZipEntry &entry : entries)
	{
		std::vector<XPlane::ZipEntry> inner;
		if (entry.name.size() >= 4 && entry.name.compare(entry.name.size() - 4, 4, ".zip") == 0 && XPlane::ReadZipArchive(entry.data, inner, &error))
			CheckNames(inner);
	}
	return 0;
}

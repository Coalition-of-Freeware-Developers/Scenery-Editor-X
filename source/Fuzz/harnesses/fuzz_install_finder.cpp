/**
 * -------------------------------------------------------
 * Scenery Editor X - Fuzzing
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * fuzz_install_finder.cpp
 * -------------------------------------------------------
 * The files the X-Plane install finder reads at startup (xp_install_finder): x-plane_install_12.txt, Steam's
 * libraryfolders.vdf and appmanifest_*.acf, the first line of X-Plane's Log.txt and install folder names. Anyone's
 * machine can hold damaged copies of these, and startup must not fall over on them.
 * -------------------------------------------------------
 */
#include "common/fuzz_common.h"
#include <SceneryEditorX/xplane/xp_install_finder.h>

using namespace SceneryEditorX::XPlane;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, const size_t size)
{
	const std::string_view text = SceneryEditorX::Fuzz::AsText(data, size);

	for (const std::filesystem::path &folder : ParseInstallList(text))
		(void)folder.lexically_normal();
	for (const std::filesystem::path &folder : ParseSteamLibraryFolders(text))
		(void)folder.lexically_normal();
	(void)ParseSteamInstallDir(text);

	const std::string_view firstLine = text.substr(0, text.find('\n'));
	(void)ParseLogMajorVersion(firstLine);
	(void)MajorVersionFromName(firstLine);
	return 0;
}

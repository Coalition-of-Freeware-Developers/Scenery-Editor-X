/**
 * -------------------------------------------------------
 * Scenery Editor X - Fuzzing
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * fuzz_wed.cpp
 * -------------------------------------------------------
 * WorldEditor projects (earth.wed.xml: project_wed, xml_document): importing, exporting what was imported (it must
 * import again) and saving the import as .edX.
 * -------------------------------------------------------
 */
#include "common/fuzz_common.h"
#include <SceneryEditorX/project/project_edx.h>
#include <SceneryEditorX/project/project_wed.h>

using namespace SceneryEditorX::ProjectFile;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, const size_t size)
{
	SceneryProject project;
	std::vector<std::string> warnings;
	std::string error;
	if (!ParseWed(SceneryEditorX::Fuzz::AsText(data, size), project, &warnings, &error))
		return 0;

	const std::string written = WriteWed(project, &warnings);
	SceneryProject reread;
	SEDX_FUZZ_CHECK(ParseWed(written, reread, nullptr, &error), "exported earth.wed.xml doesn't import: " + error);

	const std::string edx = WriteEdx(project);
	SceneryProject fromEdx;
	SEDX_FUZZ_CHECK(ParseEdx(edx, fromEdx, nullptr, &error), "an imported WorldEditor project doesn't save as loadable .edX: " + error);
	return 0;
}

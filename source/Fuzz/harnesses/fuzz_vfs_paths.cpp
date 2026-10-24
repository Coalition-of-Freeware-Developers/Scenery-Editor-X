/**
 * -------------------------------------------------------
 * Scenery Editor X - Fuzzing
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * fuzz_vfs_paths.cpp
 * -------------------------------------------------------
 * The VFS's path and name rules (filesystem/vfs), which decide where every file the editor writes lands on Windows,
 * Linux and macOS: relative path normalization (settles after one pass, never climbs out), virtual "mount:/path"
 * splitting, the archive-name guard (an accepted name stays inside its folder) and portable names (a name made
 * portable passes the portability check). Each input line is one name.
 * -------------------------------------------------------
 */
#include "common/fuzz_common.h"
#include <SceneryEditorX/filesystem/vfs.h>
#include <algorithm>

using namespace SceneryEditorX;
namespace fs = std::filesystem;

namespace
{
	void CheckName(const std::string_view name)
	{
		bool escaped = false;
		const std::string normal = VFS::NormalizeRelative(name, &escaped);
		SEDX_FUZZ_CHECK(!escaped || normal.empty(), "escaped name normalizes to '" + normal + "'");
		bool again = false;
		SEDX_FUZZ_CHECK(VFS::NormalizeRelative(normal, &again) == normal && !again, "'" + normal + "' changes when normalized again");
		for (size_t start = 0; start <= normal.size();)
		{
			const size_t end = std::min(normal.find('/', start), normal.size());
			const std::string_view part = std::string_view(normal).substr(start, end - start);
			SEDX_FUZZ_CHECK(part != ".." && part != "." && (!part.empty() || normal.empty()), "'" + normal + "' keeps a '" + std::string(part) + "' component");
			start = end + 1;
		}

		std::string mount, relative;
		if (VFS::SplitVirtualPath(name, mount, relative))
		{
			SEDX_FUZZ_CHECK(!mount.empty(), "empty mount name in '" + std::string(name) + "'");
			const std::string rebuilt = VFS::MakeVirtualPath(mount, relative);
			std::string mount2, relative2;
			SEDX_FUZZ_CHECK(VFS::SplitVirtualPath(rebuilt, mount2, relative2) && mount2 == mount && relative2 == relative,
				"'" + rebuilt + "' doesn't split back into " + mount + " and '" + relative + "'");
			(void)VFS::Join(rebuilt, name);
		}

		const fs::path folder = fs::path("Custom Scenery") / "Pack";
		fs::path target;
		if (VFS::ResolveInside(folder, name, target))
			SEDX_FUZZ_CHECK(VFS::IsInside(target, folder), "'" + std::string(name) + "' resolves to " + VFS::ToUtf8(target));

		const std::string portable = VFS::MakePortableName(name);
		SEDX_FUZZ_CHECK(VFS::CheckPortableName(portable).empty(), "MakePortableName gave '" + portable + "': " + VFS::CheckPortableName(portable));
	}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, const size_t size)
{
	const std::string_view text = SceneryEditorX::Fuzz::AsText(data, size);
	size_t start = 0;
	while (start <= text.size())
	{
		size_t end = text.find('\n', start);
		if (end == std::string_view::npos)
			end = text.size();
		CheckName(text.substr(start, end - start));
		start = end + 1;
	}
	return 0;
}

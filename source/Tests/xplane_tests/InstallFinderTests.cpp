/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * InstallFinderTests.cpp
 * -------------------------------------------------------
 * Finding X-Plane 11 and 12 installs: X-Plane's install lists, Steam libraries
 * and manifests, uninstall entries and the usual folders, ranked best first.
 * -------------------------------------------------------
 */
#include <catch2/catch_all.hpp>
#include <SceneryEditorX/filesystem/vfs.h>
#include <SceneryEditorX/xplane/xp_install_finder.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace SceneryEditorX::XPlane;
namespace fs = std::filesystem;
namespace VFS = SceneryEditorX::VFS;

namespace
{
	struct TempDir
	{
		fs::path path;

		TempDir()
		{
			path = fs::temp_directory_path() / ("sedx_install_finder_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
			fs::create_directories(path);
		}

		~TempDir()
		{
			std::error_code ec;
			fs::remove_all(path, ec);
		}
	};

	void WriteText(const fs::path &file, const std::string &text)
	{
		fs::create_directories(file.parent_path());
		std::ofstream(file, std::ios::binary) << text;
	}

	/* @brief An X-Plane folder: complete (default scenery + Custom Scenery) or just Resources and the executable. */
	void MakeXPlane(const fs::path &root, const bool complete, const std::string &logFirstLine = {})
	{
		fs::create_directories(root / "Resources");
		WriteText(root / "X-Plane.exe", "");
		if (complete)
		{
			fs::create_directories(root / "Resources" / "default scenery");
			fs::create_directories(root / "Custom Scenery");
		}
		if (!logFirstLine.empty())
			WriteText(root / "Log.txt", logFirstLine + "\nmore log\n");
	}

	/* @brief A Valve KeyValues string with backslashes escaped, the way Steam writes paths. */
	std::string VdfString(const fs::path &path)
	{
		std::string escaped;
		for (const char c : VFS::ToUtf8(path))
		{
			if (c == '\\')
				escaped += '\\';
			escaped += c;
		}
		return "\"" + escaped + "\"";
	}

	const InstallCandidate *Find(const std::vector<InstallCandidate> &found, const fs::path &root)
	{
		for (const InstallCandidate &candidate : found)
		{
			if (VFS::PathKey(candidate.root) == VFS::PathKey(root))
				return &candidate;
		}
		return nullptr;
	}
}

TEST_CASE("X-Plane install lists: one folder per line, trailing separators and blank lines ignored", "[xplane][install]")
{
	const std::vector<fs::path> folders = ParseInstallList("K:/SteamLibrary/steamapps/common/X-Plane 12/\r\n\r\n  /home/pilot/X-Plane 11  \n");
	REQUIRE(folders.size() == 2);
	CHECK(folders[0].filename() == "X-Plane 12");
	CHECK(VFS::PathKey(folders[0]) == VFS::PathKey(fs::path("K:/SteamLibrary/steamapps/common/X-Plane 12")));
	CHECK(folders[1].filename() == "X-Plane 11");
	CHECK(ParseInstallList("").empty());
}

TEST_CASE("Steam libraryfolders.vdf: the current and the pre-2021 layouts", "[xplane][install]")
{
	const std::string current = R"("libraryfolders"
{
	"0"
	{
		"path"		"C:\\Program Files (x86)\\Steam"
		"label"		""
		"apps"
		{
			"228980"		"472565116"
		}
	}
	"1"
	{
		"path"		"K:\\SteamLibrary"
		"apps"
		{
			"2014780"		"80437282816"
		}
	}
})";
	const std::vector<fs::path> libraries = ParseSteamLibraryFolders(current);
	REQUIRE(libraries.size() == 2);
	CHECK(VFS::ToUtf8(libraries[0]) == "C:\\Program Files (x86)\\Steam");
	CHECK(VFS::ToUtf8(libraries[1]) == "K:\\SteamLibrary");

	const std::string old = R"("LibraryFolders"
{
	"TimeNextStatsReport"		"1234567890"
	"ContentStatsID"		"-4321"
	"1"		"D:\\Games\\Steam"
})";
	const std::vector<fs::path> oldLibraries = ParseSteamLibraryFolders(old);
	REQUIRE(oldLibraries.size() == 1);
	CHECK(VFS::ToUtf8(oldLibraries[0]) == "D:\\Games\\Steam");
}

TEST_CASE("Steam appmanifest: the install folder name", "[xplane][install]")
{
	CHECK(ParseSteamInstallDir("\"AppState\"\n{\n\t\"appid\"\t\t\"2014780\"\n\t\"name\"\t\t\"X-Plane 12\"\n\t\"installdir\"\t\t\"X-Plane 12\"\n}\n") == "X-Plane 12");
	CHECK(ParseSteamInstallDir("\"AppState\"\n{\n\t\"appid\"\t\t\"269950\"\n}\n").empty());
}

TEST_CASE("X-Plane major versions from Log.txt and from names", "[xplane][install]")
{
	CHECK(ParseLogMajorVersion("Log.txt for X-Plane 12.4.3-r2-15ff1e4d (build 124302 win64 Vulkan)") == 12);
	CHECK(ParseLogMajorVersion("Log.txt for X-Plane 11.55r2 (build 115502 64-bit, Vulkan)") == 11);
	CHECK(ParseLogMajorVersion("This is not a log") == 0);
	CHECK(MajorVersionFromName("X-Plane 11") == 11);
	CHECK(MajorVersionFromName("X-Plane 12 Demo") == 12);
	CHECK(MajorVersionFromName("XPlane12") == 12);
	CHECK(MajorVersionFromName("X-Plane") == 0);
}

TEST_CASE("Finding installs: every source, once each, complete and newest first", "[xplane][install]")
{
	TempDir temp;
	const fs::path root = temp.path;
	const auto now = fs::file_time_type::clock::now();

	// X-Plane 12 run an hour ago, listed by X-Plane itself (and again by an uninstall entry)
	const fs::path desktop12 = root / "Desktop" / "X-Plane 12";
	MakeXPlane(desktop12, true, "Log.txt for X-Plane 12.4.3-r2-15ff1e4d (build 124302)");
	fs::last_write_time(desktop12 / "Log.txt", now - std::chrono::hours(1));

	// A Steam X-Plane 12 run last week, in a second library that libraryfolders.vdf lists
	const fs::path library = root / "SteamLibrary";
	const fs::path steam12 = library / "steamapps" / "common" / "X-Plane 12";
	MakeXPlane(steam12, true, "Log.txt for X-Plane 12.1.4-r3 (build 121403)");
	fs::last_write_time(steam12 / "Log.txt", now - std::chrono::hours(24 * 7));
	WriteText(library / "steamapps" / "appmanifest_2014780.acf", "\"AppState\"\n{\n\t\"installdir\"\t\t\"X-Plane 12\"\n}\n");

	// A Steam X-Plane 11 in the Steam folder itself, never run (no Log.txt), in a folder named by its manifest
	const fs::path steam = root / "Steam";
	const fs::path steam11 = steam / "steamapps" / "common" / "XP11";
	MakeXPlane(steam11, true);
	WriteText(steam / "steamapps" / "appmanifest_269950.acf", "\"AppState\"\n{\n\t\"installdir\"\t\t\"XP11\"\n}\n");
	WriteText(steam / "steamapps" / "libraryfolders.vdf", "\"libraryfolders\"\n{\n\t\"0\"\n\t{\n\t\t\"path\"\t\t" + VdfString(steam) +
		"\n\t}\n\t\"1\"\n\t{\n\t\t\"path\"\t\t" + VdfString(library) + "\n\t}\n}\n");

	// A folder named "X-Plane 11" whose last run was X-Plane 12, and an incomplete X-Plane 12 (no Custom Scenery)
	const fs::path renamed = root / "Games" / "X-Plane 11";
	MakeXPlane(renamed, true, "Log.txt for X-Plane 12.0.9-r1 (build 120901)");
	fs::last_write_time(renamed / "Log.txt", now - std::chrono::hours(24 * 30));
	const fs::path incomplete = root / "Broken" / "X-Plane 12";
	MakeXPlane(incomplete, false);

	// Not X-Plane at all, and a folder that doesn't exist
	fs::create_directories(root / "Documents" / "X-Plane 12");

	WriteText(root / "x-plane_install_12.txt", VFS::ToUtf8(desktop12) + "/\n" + VFS::ToUtf8(root / "Missing" / "X-Plane 12") + "\n");

	InstallSearch search;
	search.installLists.push_back({ root / "x-plane_install_12.txt", 12 });
	search.installLists.push_back({ root / "x-plane_install_11.txt", 11 });		// Not there: skipped
	search.steamRoots.push_back(steam);
	search.registered.push_back({ desktop12, 12 });
	search.commonFolders.push_back({ renamed, 11 });
	search.commonFolders.push_back({ incomplete, 12 });
	search.commonFolders.push_back({ root / "Documents" / "X-Plane 12", 12 });

	const std::vector<InstallCandidate> found = FindInstallCandidates(search);
	REQUIRE(found.size() == 5);

	// Complete before incomplete, X-Plane 12 before 11, then the most recently run
	CHECK(VFS::PathKey(found[0].root) == VFS::PathKey(desktop12));
	CHECK(VFS::PathKey(found[1].root) == VFS::PathKey(steam12));
	CHECK(VFS::PathKey(found[2].root) == VFS::PathKey(renamed));
	CHECK(VFS::PathKey(found[3].root) == VFS::PathKey(steam11));
	CHECK(VFS::PathKey(found[4].root) == VFS::PathKey(incomplete));

	const InstallCandidate *first = Find(found, desktop12);
	REQUIRE(first);
	CHECK(first->source == InstallSource::InstallList);	// Found first through X-Plane's list, then deduplicated
	CHECK(first->majorVersion == 12);
	CHECK(first->complete);

	const InstallCandidate *xp11 = Find(found, steam11);
	REQUIRE(xp11);
	CHECK(xp11->source == InstallSource::Steam);
	CHECK(xp11->majorVersion == 11);					// From the Steam app: no log, and the folder name doesn't say
	CHECK(xp11->lastRun == fs::file_time_type{});

	const InstallCandidate *renamedFound = Find(found, renamed);
	REQUIRE(renamedFound);
	CHECK(renamedFound->majorVersion == 12);			// The log wins over the hint and the folder name

	const InstallCandidate *broken = Find(found, incomplete);
	REQUIRE(broken);
	CHECK_FALSE(broken->complete);
	CHECK(broken->source == InstallSource::CommonFolder);

	CHECK(Find(found, root / "Documents" / "X-Plane 12") == nullptr);
}

TEST_CASE("Finding installs with nothing to find", "[xplane][install]")
{
	InstallSearch search;
	search.installLists.push_back({ fs::path("/no/such/folder/x-plane_install_12.txt"), 12 });
	search.steamRoots.push_back(fs::path("/no/such/Steam"));
	search.commonFolders.push_back({ fs::path("/no/such/X-Plane 12"), 12 });
	CHECK(FindInstallCandidates(search).empty());
	CHECK(FindInstallCandidates(InstallSearch{}).empty());
}

TEST_CASE("This machine's search runs without throwing", "[xplane][install]")
{
	// Whatever is installed here: the point is that unreadable drives and folders are skipped, not thrown about
	std::vector<InstallCandidate> found;
	REQUIRE_NOTHROW(found = FindInstallCandidates());
	for (const InstallCandidate &candidate : found)
	{
		INFO(VFS::ToUtf8(candidate.root) << " X-Plane " << candidate.majorVersion << " via " << GetInstallSourceName(candidate.source));
		CHECK(VFS::IsXPlaneFolder(candidate.root));
	}
}

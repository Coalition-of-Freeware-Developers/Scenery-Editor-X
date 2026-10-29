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
 * VfsTests.cpp
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#include <catch2/catch_all.hpp>
#include <SceneryEditorX/filesystem/vfs.h>
#include <fstream>
#include <random>

using namespace SceneryEditorX;
namespace fs = std::filesystem;

// -------------------------------------------------------

namespace
{
	/* @brief A fresh folder under the temp folder, removed afterwards. */
	struct Scratch
	{
		fs::path path;

		Scratch()
		{
			std::random_device device;
			path = fs::temp_directory_path() / ("sedx_vfs_tests_" + std::to_string(device()) + "_" + std::to_string(device()));
			fs::create_directories(path);
		}

		~Scratch()
		{
			std::error_code ec;
			fs::remove_all(path, ec);
		}
	};

	void Touch(const fs::path &path, const std::string &text = "x")
	{
		fs::create_directories(path.parent_path());
		std::ofstream(path, std::ios::binary) << text;
	}

	bool IsCaseSensitive(const fs::path &folder)
	{
		Touch(folder / "case_probe");
		std::error_code ec;
		const bool sensitive = !fs::exists(folder / "CASE_PROBE", ec);
		fs::remove(folder / "case_probe", ec);
		return sensitive;
	}
}

// -------------------------------------------------------

TEST_CASE("Virtual paths normalize and never escape their mount", "[vfs][paths]")
{
	CHECK(VFS::NormalizeRelative("") == "");
	CHECK(VFS::NormalizeRelative("/") == "");
	CHECK(VFS::NormalizeRelative("a/b/c") == "a/b/c");
	CHECK(VFS::NormalizeRelative("/a//b/./c/") == "a/b/c");
	CHECK(VFS::NormalizeRelative("a\\b\\c") == "a/b/c");
	CHECK(VFS::NormalizeRelative("a/b/../c") == "a/c");
	CHECK(VFS::NormalizeRelative("Earth nav data/+40-090/../apt.dat") == "Earth nav data/apt.dat");

	bool escaped = false;
	CHECK(VFS::NormalizeRelative("a/../..", &escaped).empty());
	CHECK(escaped);
	CHECK(VFS::NormalizeRelative("../etc/passwd", &escaped).empty());
	CHECK(escaped);
	VFS::NormalizeRelative("a/..", &escaped);
	CHECK_FALSE(escaped);

	std::string mount, relative;
	REQUIRE(VFS::SplitVirtualPath("project:/Earth nav data/apt.dat", mount, relative));
	CHECK(mount == "project");
	CHECK(relative == "Earth nav data/apt.dat");
	REQUIRE(VFS::SplitVirtualPath("PROJECT:objects\\hangar.obj", mount, relative));
	CHECK(mount == "project");
	CHECK(relative == "objects/hangar.obj");
	REQUIRE(VFS::SplitVirtualPath("scenery:/", mount, relative));
	CHECK(relative.empty());

	CHECK_FALSE(VFS::SplitVirtualPath("C:/X-Plane 12/Custom Scenery", mount, relative));	// A drive, not a mount
	CHECK_FALSE(VFS::SplitVirtualPath("/home/user/file", mount, relative));
	CHECK_FALSE(VFS::SplitVirtualPath("project:/../outside", mount, relative));
	CHECK_FALSE(VFS::SplitVirtualPath("pro ject:/a", mount, relative));

	CHECK(VFS::MakeVirtualPath("Project", "/a//b/") == "project:/a/b");
	CHECK(VFS::Join("project:/Earth nav data", "+40-090/+41-084.dsf") == "project:/Earth nav data/+40-090/+41-084.dsf");
	CHECK(VFS::Join("project:/", "apt.dat") == "project:/apt.dat");
	CHECK(VFS::Join("project:/a", "../b") == "project:/b");
}

TEST_CASE("Names are checked and made portable for Windows, macOS and Linux", "[vfs][names]")
{
	CHECK(VFS::CheckPortableName("KSEA - Seattle Tacoma Intl").empty());
	CHECK(VFS::CheckPortableName("Earth nav data").empty());
	CHECK(VFS::CheckPortableName("Zürich Flughafen").empty());
	CHECK_FALSE(VFS::CheckPortableName("").empty());
	CHECK_FALSE(VFS::CheckPortableName("..").empty());
	CHECK_FALSE(VFS::CheckPortableName("a:b").empty());
	CHECK_FALSE(VFS::CheckPortableName("a/b").empty());
	CHECK_FALSE(VFS::CheckPortableName("what?").empty());
	CHECK_FALSE(VFS::CheckPortableName("name.").empty());
	CHECK_FALSE(VFS::CheckPortableName("name ").empty());
	CHECK_FALSE(VFS::CheckPortableName("CON").empty());
	CHECK_FALSE(VFS::CheckPortableName("con.txt").empty());
	CHECK_FALSE(VFS::CheckPortableName("LPT1").empty());
	CHECK(VFS::CheckPortableName("COM10").empty());
	CHECK(VFS::CheckPortableName("CONSOLE").empty());
	CHECK_FALSE(VFS::CheckPortableName(std::string(256, 'a')).empty());

	CHECK(VFS::MakePortableName("KSEA: Seattle/Tacoma?") == "KSEA_ Seattle_Tacoma_");
	CHECK(VFS::MakePortableName("  Field. . ") == "Field");
	CHECK(VFS::MakePortableName("CON") == "CON_");
	CHECK(VFS::MakePortableName("nul.txt") == "nul_.txt");
	CHECK(VFS::MakePortableName("...") == "Untitled");
	CHECK(VFS::MakePortableName("", "Scenery") == "Scenery");
	for (const std::string &name : std::vector<std::string>{ "a<b>c", "CON", "x|y*z", " lead", "trail.", std::string(300, 'z') })
		CHECK(VFS::CheckPortableName(VFS::MakePortableName(name)).empty());
}

TEST_CASE("Mounts resolve, list, read and write through virtual paths", "[vfs][mounts]")
{
	Scratch scratch;
	const fs::path pack = scratch.path / "KTST - Test Field";
	VFS::FileSystem vfs;

	std::string error;
	REQUIRE(vfs.Mount("project", pack, false, &error));
	CHECK(vfs.IsMounted("PROJECT"));
	CHECK_FALSE(vfs.Mount("x", pack, false, &error));		// One letter would read as a drive
	CHECK_FALSE(vfs.Mount("bad name", pack, false, &error));

	const std::optional<fs::path> resolved = vfs.Resolve("project:/Earth nav data/apt.dat");
	REQUIRE(resolved);
	CHECK(VFS::IsInside(*resolved, pack));
	CHECK(resolved->filename() == "apt.dat");
	CHECK_FALSE(vfs.Resolve("nothing:/a"));
	CHECK_FALSE(vfs.Resolve("project:/../escape"));

	// Writes create folders and are atomic (no temporary files left behind)
	REQUIRE(vfs.Write("project:/Earth nav data/apt.dat", "I\n1200 Generated by test\n", &error));
	REQUIRE(vfs.Write("project:/Earth nav data/apt.dat", "I\n1200 Replaced\n", &error));
	std::string bytes;
	REQUIRE(vfs.Read("project:/Earth nav data/apt.dat", bytes, &error));
	CHECK(bytes == "I\n1200 Replaced\n");
	REQUIRE(vfs.CreateFolders("project:/objects", &error));
	REQUIRE(vfs.Write("project:/objects/hangar.obj", "A\n800\nOBJ\n", &error));

	const std::vector<VFS::Entry> top = vfs.List("project:/");
	REQUIRE(top.size() == 2);
	CHECK(top[0].name == "Earth nav data");
	CHECK(top[0].directory);
	CHECK(top[1].name == "objects");
	const std::vector<VFS::Entry> all = vfs.List("project:/", true);
	REQUIRE(all.size() == 4);
	CHECK(all[1].path == "project:/Earth nav data/apt.dat");
	CHECK(all[1].size == 16);
	CHECK(all[3].path == "project:/objects/hangar.obj");
	for (const fs::directory_entry &entry : fs::recursive_directory_iterator(pack))
		CHECK(entry.path().filename().string().find(".tmp-") == std::string::npos);

	// Physical paths map back to virtual ones; the deepest mount wins
	REQUIRE(vfs.Mount("scenery", scratch.path));
	CHECK(vfs.ToVirtual(pack / "objects" / "hangar.obj") == "project:/objects/hangar.obj");
	CHECK(vfs.ToVirtual(scratch.path / "Other Pack" / "library.txt") == "scenery:/Other Pack/library.txt");
	CHECK_FALSE(vfs.ToVirtual(scratch.path.parent_path() / "elsewhere.txt"));

	// Copies (files and whole folders) and removals
	REQUIRE(vfs.Copy("project:/objects/hangar.obj", "project:/objects/hangar2.obj", false, &error));
	CHECK_FALSE(vfs.Copy("project:/objects/hangar.obj", "project:/objects/hangar2.obj", false, &error));
	REQUIRE(vfs.Copy("project:/objects", "scenery:/Export/objects", false, &error));
	CHECK(vfs.Exists("scenery:/Export/objects/hangar2.obj"));
	CHECK_FALSE(vfs.Remove("project:/objects", &error));		// Not empty
	REQUIRE(vfs.Remove("project:/objects/hangar2.obj", &error));
	CHECK_FALSE(vfs.Exists("project:/objects/hangar2.obj"));
	CHECK(vfs.Remove("project:/objects/never-there.obj", &error));

	// Read-only mounts refuse every change
	REQUIRE(vfs.Mount("xplane", scratch.path, true));
	CHECK(vfs.IsReadOnly("xplane"));
	CHECK_FALSE(vfs.Write("xplane:/Export/new.txt", "x", &error));
	CHECK(error.find("read-only") != std::string::npos);
	CHECK_FALSE(vfs.Remove("xplane:/Export/objects/hangar.obj", &error));
	CHECK_FALSE(vfs.Copy("project:/objects/hangar.obj", "xplane:/copy.obj", true, &error));
	CHECK(vfs.Exists("xplane:/Export/objects/hangar.obj"));

	CHECK(vfs.Unmount("project"));
	CHECK_FALSE(vfs.Unmount("project"));
	CHECK_FALSE(vfs.Exists("project:/Earth nav data/apt.dat"));
}

TEST_CASE("Lookups forgive the case Windows-made packs get wrong", "[vfs][case]")
{
	Scratch scratch;
	Touch(scratch.path / "Earth nav data" / "apt.dat");
	Touch(scratch.path / "objects" / "Hangar_Large.obj");

	const std::optional<fs::path> found = VFS::FindCaseInsensitive(scratch.path / "earth NAV data" / "APT.DAT");
	REQUIRE(found);
	CHECK(fs::is_regular_file(*found));
	CHECK_FALSE(VFS::FindCaseInsensitive(scratch.path / "earth nav data" / "missing.dat"));
	CHECK_FALSE(VFS::FindCaseInsensitive(scratch.path / "nowhere" / "apt.dat"));

	VFS::FileSystem vfs;
	REQUIRE(vfs.Mount("project", scratch.path));
	CHECK(vfs.Exists("project:/Objects/hangar_large.OBJ"));
	std::string bytes;
	CHECK(vfs.Read("project:/OBJECTS/HANGAR_LARGE.OBJ", bytes));

	// A write in other case lands in the folder that exists instead of a twin
	REQUIRE(vfs.Write("project:/EARTH NAV DATA/+40-090/+41-084.dsf", "dsf"));
	size_t folders = 0;
	for (const fs::directory_entry &entry : fs::directory_iterator(scratch.path))
		folders += entry.is_directory() ? 1 : 0;
	CHECK(folders == 2);
	CHECK(fs::exists(scratch.path / "Earth nav data" / "+40-090" / "+41-084.dsf"));

	if (IsCaseSensitive(scratch.path))
	{
		// Only a case-sensitive disk can hold both spellings: the lookup picks one deterministically
		Touch(scratch.path / "objects" / "hangar_large.obj", "lower");
		REQUIRE(vfs.Read("project:/objects/HANGAR_LARGE.obj", bytes));
		CHECK(bytes == "x");		// "Hangar_Large.obj" sorts before "hangar_large.obj"
	}
}

TEST_CASE("Path keys and case-forgiving checks", "[vfs][case]")
{
	CHECK(VFS::PathKey("a/b/../c/") == VFS::PathKey("a/c"));
	CHECK(VFS::PathKey("a/./b") == VFS::PathKey("a/b"));
#if defined(_WIN32)
	CHECK(VFS::PathKey("a\\b") == VFS::PathKey("a/b"));		// '\' only separates on Windows
#endif
#if defined(_WIN32) || defined(__APPLE__)
	CHECK(VFS::PathKey("C:/X-Plane 12/Custom Scenery") == VFS::PathKey("c:/x-plane 12/custom scenery"));
#else
	CHECK(VFS::PathKey("/X-Plane 12/Custom Scenery") != VFS::PathKey("/x-plane 12/custom scenery"));
#endif

	Scratch scratch;
	Touch(scratch.path / "Custom Scenery" / "Pack" / "Library.txt", "A\n800\nLIBRARY\n");
	CHECK(VFS::IsFile(scratch.path / "custom scenery" / "pack" / "library.txt"));
	CHECK(VFS::IsDirectory(scratch.path / "CUSTOM SCENERY" / "PACK"));
	CHECK_FALSE(VFS::IsFile(scratch.path / "custom scenery" / "pack"));
	CHECK_FALSE(VFS::IsDirectory(scratch.path / "custom scenery" / "pack" / "library.txt"));
	CHECK_FALSE(VFS::IsFile(scratch.path / "custom scenery" / "missing.txt"));

	std::string text;
	REQUIRE(VFS::ReadFile(scratch.path / "CUSTOM SCENERY" / "pack" / "LIBRARY.TXT", text));
	CHECK(text == "A\n800\nLIBRARY\n");
	CHECK_FALSE(VFS::ReadFile(scratch.path / "custom scenery" / "missing.txt", text));

	// ResolveCase gives the disk's spelling where it matters and leaves missing paths alone
	const fs::path resolved = VFS::ResolveCase(scratch.path / "custom scenery" / "pack");
	CHECK(fs::is_directory(resolved));
	CHECK(fs::equivalent(resolved, scratch.path / "Custom Scenery" / "Pack"));
	const fs::path missing = scratch.path / "custom scenery" / "nope";
	CHECK(VFS::ResolveCase(missing) == missing);
}

TEST_CASE("Platform folders and X-Plane installs are found the platform's way", "[vfs][platform]")
{
	const fs::path home = VFS::GetFolder(VFS::Folder::Home);
	const fs::path temp = VFS::GetFolder(VFS::Folder::Temp);
	CHECK(!temp.empty());
	CHECK(fs::is_directory(temp));
	if (!home.empty())
	{
		CHECK(home.is_absolute());
		CHECK_FALSE(VFS::GetFolder(VFS::Folder::Config).empty());
		CHECK_FALSE(VFS::GetFolder(VFS::Folder::Cache).empty());
		CHECK(VFS::GetAppFolder(VFS::Folder::Config).filename() == "SceneryEditorX");
		CHECK(VFS::GetXPlaneInstallList(12).filename() == "x-plane_install_12.txt");
	}

	Scratch scratch;
	const fs::path fake = scratch.path / "X-Plane 12";
	CHECK_FALSE(VFS::IsXPlaneFolder(fake));
	fs::create_directories(fake / "Resources");
	fs::create_directories(fake / "custom scenery");
	CHECK(VFS::IsXPlaneFolder(fake));
	CHECK(fs::equivalent(VFS::GetCustomSceneryFolder(fake), fake / "custom scenery"));	// The disk's spelling where case matters

	// Whatever this machine has installed must be real X-Plane folders
	for (const fs::path &install : VFS::FindXPlaneInstalls(12))
		CHECK(VFS::IsXPlaneFolder(install));
}

TEST_CASE("The app layout puts each kind of file where the platform expects it", "[vfs][layout]")
{
	const VFS::AppLayout layout = VFS::MakeAppLayout("SedxLayoutTest");
	for (const VFS::AppLocation location : { VFS::AppLocation::Settings, VFS::AppLocation::Data, VFS::AppLocation::Cache, VFS::AppLocation::Logs, VFS::AppLocation::Temp })
	{
		CHECK(layout.Get(location).is_absolute());
		CHECK_FALSE(layout.Get(location).empty());
	}
	CHECK(layout.settings.filename() == "SedxLayoutTest");
	CHECK(VFS::IsInside(layout.logs, layout.settings));
	CHECK(VFS::IsInside(layout.temp, VFS::GetFolder(VFS::Folder::Temp)));
	CHECK_FALSE(VFS::IsInside(layout.cache, layout.settings));		// Caches never sit in the (roaming) settings
#if defined(_WIN32)
	CHECK(layout.cache == layout.data / "cache");					// %LOCALAPPDATA%\<app>\cache
	CHECK_FALSE(VFS::IsInside(layout.data, layout.settings));		// Local, not Roaming
#elif defined(__APPLE__)
	CHECK(layout.cache.parent_path().filename() == "Caches");		// ~/Library/Caches/<app>
#endif

	// The process-wide layout can be replaced (tests, portable installs)
	const VFS::AppLayout original = VFS::GetAppLayout();
	VFS::AppLayout custom = layout;
	custom.cache = layout.temp / "custom-cache";
	VFS::SetAppLayout(custom);
	CHECK(VFS::GetAppLocation(VFS::AppLocation::Cache) == custom.cache);
	VFS::SetAppLayout(original);
	CHECK(VFS::GetAppLocation(VFS::AppLocation::Cache) == original.cache);
}

TEST_CASE("Legacy files move once, merging without overwriting anything", "[vfs][layout]")
{
	Scratch scratch;
	const fs::path roaming = scratch.path / "Roaming" / "SceneryEditorX";
	const fs::path local = scratch.path / "Local" / "SceneryEditorX";

	// What an older version left behind
	Touch(roaming / "airport_index.bin", "index");
	Touch(roaming / "gateway" / "airports.json", "airports");
	Touch(roaming / "gateway" / "scenery" / "100460.json", "5S4");
	Touch(roaming / "project-cache" / "abc" / "snapshot.sex", "old snapshot");
	Touch(roaming / "project-cache" / "abc" / "recovery.sex", "recovery");
	Touch(roaming / "user_preferences.cfg", "settings stay");

	// The new version already wrote part of the project cache
	Touch(local / "project-cache" / "abc" / "snapshot.sex", "new snapshot");

	const std::vector<std::string> report = VFS::MoveLegacyFiles({
		{ roaming / "airport_index.bin", local / "cache" / "airport_index.bin" },
		{ roaming / "gateway", local / "cache" / "gateway" },
		{ roaming / "project-cache", local / "project-cache" },
		{ roaming / "never-existed", local / "cache" / "never" },
		{ local / "cache", local / "cache" / "inside" } });

	// Moved whole
	CHECK(fs::exists(local / "cache" / "airport_index.bin"));
	CHECK_FALSE(fs::exists(roaming / "airport_index.bin"));
	CHECK(fs::exists(local / "cache" / "gateway" / "scenery" / "100460.json"));
	CHECK_FALSE(fs::exists(roaming / "gateway"));

	// Merged: the missing file moved, the one already there was kept (and the old copy stays put)
	std::string text;
	REQUIRE(VFS::ReadFile(local / "project-cache" / "abc" / "snapshot.sex", text));
	CHECK(text == "new snapshot");
	REQUIRE(VFS::ReadFile(local / "project-cache" / "abc" / "recovery.sex", text));
	CHECK(text == "recovery");
	REQUIRE(VFS::ReadFile(roaming / "project-cache" / "abc" / "snapshot.sex", text));
	CHECK(text == "old snapshot");
	CHECK_FALSE(fs::exists(roaming / "project-cache" / "abc" / "recovery.sex"));

	// Settings weren't touched; the report names what happened
	CHECK(fs::exists(roaming / "user_preferences.cfg"));
	REQUIRE(report.size() == 4);
	CHECK(report[0].starts_with("Moved "));
	CHECK(report[1].starts_with("Moved "));
	CHECK(report[2].find("1 item already there stayed in the old place") != std::string::npos);
	CHECK(report[3].starts_with("Not moving "));

	// A second run has nothing left to move except what's in the way
	const std::vector<std::string> again = VFS::MoveLegacyFiles({ { roaming / "project-cache", local / "project-cache" } });
	REQUIRE(again.size() == 1);
	CHECK(again[0].starts_with("Left "));

	// Mounting creates and maps every folder
	VFS::AppLayout layout;
	layout.settings = roaming;
	layout.data = local;
	layout.cache = local / "cache";
	layout.logs = roaming / "logs";
	layout.temp = scratch.path / "Temp" / "SceneryEditorX";
	VFS::FileSystem vfs;
	std::string error;
	REQUIRE(VFS::MountAppLayout(vfs, layout, &error));
	CHECK(fs::is_directory(layout.logs));
	CHECK(fs::is_directory(layout.temp));
	CHECK(vfs.Exists("cache:/airport_index.bin"));
	CHECK(vfs.Exists("data:/project-cache/abc/recovery.sex"));
	CHECK(vfs.Exists("user:/user_preferences.cfg"));
	REQUIRE(vfs.Write("logs:/SceneryEditorX.log", "line\n", &error));
	CHECK(fs::exists(layout.logs / "SceneryEditorX.log"));
}

TEST_CASE("UTF-8 survives the trip through paths", "[vfs][paths]")
{
	const std::string name = "LSZH - Z\xC3\xBCrich";
	CHECK(VFS::ToUtf8(VFS::FromUtf8(name)) == name);

	Scratch scratch;
	VFS::FileSystem vfs;
	REQUIRE(vfs.Mount("project", scratch.path));
	REQUIRE(vfs.Write("project:/" + name + "/Earth nav data/apt.dat", "I\n"));
	const std::vector<VFS::Entry> entries = vfs.List("project:/");
	REQUIRE(entries.size() == 1);
	CHECK(entries[0].name == name);
}

TEST_CASE("Folders outside the ANSI code page convert without the code page", "[vfs][paths]")
{
	// An install under a Japanese user name: path::string() / generic_string() throw for it on Windows (MSVC)
	const std::string folder = "C:/Users/\xE6\x97\xA5\xE6\x9C\xAC/AppData/Local/Programs/SceneryEditorX";
	const fs::path resources = VFS::FromUtf8(folder) / "resources" / "shaders";
	CHECK(VFS::ToGenericUtf8(resources) == folder + "/resources/shaders");
	CHECK(VFS::FromUtf8(VFS::ToUtf8(resources)) == resources);

	Scratch scratch;
	const fs::path file = scratch.path / VFS::FromUtf8("\xE6\x97\xA5\xE6\x9C\xAC") / "font.ttf";
	Touch(file, "bytes");
	std::string bytes;
	REQUIRE(VFS::ReadFile(VFS::FromUtf8(VFS::ToUtf8(file)), bytes));
	CHECK(bytes == "bytes");
}

/**
* -------------------------------------------------------
* Scenery Editor X - Unit Tests
* -------------------------------------------------------
* Copyright (c) 2026 Thomas Ray
* Copyright (c) 2026 Coalition of Freeware Developers
* -------------------------------------------------------
* SettingsTest.cpp
* -------------------------------------------------------
* Unit tests for ConfigFile: the libconfig settings files (settings.cfg, user_preferences.cfg, editor_settings.cfg)
* read and written through the VFS.
* -------------------------------------------------------
*/
#include <catch2/catch_all.hpp>
#include <SceneryEditorX/filesystem/vfs.h>
#include <SceneryEditorX/settings/config_file.h>
#include <fstream>
#include <random>
#include <sstream>

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
			path = fs::temp_directory_path() / ("sedx_settings_tests_" + std::to_string(device()) + "_" + std::to_string(device()));
			fs::create_directories(path);
		}

		~Scratch()
		{
			std::error_code ec;
			fs::remove_all(path, ec);
		}
	};

	void WriteText(const fs::path &path, const std::string &text)
	{
		fs::create_directories(path.parent_path());
		std::ofstream file(path, std::ios::binary);
		file << text;
	}

	std::string ReadText(const fs::path &path)
	{
		std::ifstream file(path, std::ios::binary);
		std::stringstream text;
		text << file.rdbuf();
		return text.str();
	}

	/* @brief A file system with "user" mounted on a scratch folder. */
	struct UserMount
	{
		Scratch scratch;
		VFS::FileSystem vfs;

		UserMount()
		{
			REQUIRE(vfs.Mount(VFS::MOUNT_USER, scratch.path / "settings"));
		}
	};
}

// -------------------------------------------------------

TEST_CASE("ConfigFile values by dotted path", "[settings][config]")
{
	ConfigFile file;

	SECTION("Every type round trips, with the groups on the way created")
	{
		CHECK(file.AddBoolOption("ui.dark", true));
		CHECK(file.AddIntOption("ui.font_size", 14));
		CHECK(file.AddInt64Option("user.opened", 1761312310LL));
		CHECK(file.AddFloatOption("editor.snapping.translation", 0.25));
		CHECK(file.AddStringOption("x_plane.path", "C:/X-Plane 12"));

		CHECK(file.GetBoolOption("ui.dark") == true);
		CHECK(file.GetIntOption("ui.font_size") == 14);
		CHECK(file.GetInt64Option("user.opened") == 1761312310LL);
		CHECK(file.GetFloatOption("editor.snapping.translation") == Catch::Approx(0.25));
		CHECK(file.GetStringOption("x_plane.path") == "C:/X-Plane 12");

		CHECK(file.HasOption("ui.font_size"));
		CHECK(file.HasGroup("editor.snapping"));
		CHECK_FALSE(file.HasOption("editor.snapping"));		// A group, not a value
		CHECK_FALSE(file.HasGroup("ui.font_size"));
	}

	SECTION("Defaults for what's missing")
	{
		CHECK(file.GetBoolOption("missing.flag", true) == true);
		CHECK(file.GetIntOption("missing.number", 7) == 7);
		CHECK(file.GetFloatOption("missing.ratio", 1.5) == Catch::Approx(1.5));
		CHECK(file.GetStringOption("missing.text", "fallback") == "fallback");
		std::string text = "unchanged";
		CHECK_FALSE(file.GetOption("missing.text", text));
		CHECK(text == "unchanged");
	}

	SECTION("Numbers convert, text reads as booleans and numbers")
	{
		file.AddIntOption("a.whole", 3);
		file.AddFloatOption("a.ratio", 2.75);
		file.SetOption("a.flag", "true");
		file.SetOption("a.count", "12");
		CHECK(file.GetFloatOption("a.whole") == Catch::Approx(3.0));
		CHECK(file.GetIntOption("a.ratio") == 2);
		CHECK(file.GetBoolOption("a.flag") == true);
		CHECK(file.GetIntOption("a.count") == 12);

		std::string text;
		CHECK(file.GetOption("a.whole", text));
		CHECK(text == "3");
		CHECK(file.GetOption("a.flag", text));
		CHECK(text == "true");
	}

	SECTION("A value of another type is replaced; groups are never replaced by values")
	{
		file.AddIntOption("ui.theme", 1);
		CHECK(file.AddStringOption("ui.theme", "dark"));
		CHECK(file.GetStringOption("ui.theme") == "dark");

		CHECK_FALSE(file.AddIntOption("ui", 5));				// ui is a group
		CHECK(file.HasGroup("ui"));
		CHECK_FALSE(file.AddIntOption("ui.theme.size", 5));	// ui.theme is a value
		CHECK(file.GetStringOption("ui.theme") == "dark");
		CHECK_FALSE(file.AddIntOption("ui.2d", 5));			// Not a name libconfig allows
		CHECK_FALSE(file.AddIntOption("", 5));
	}

	SECTION("Remove takes values and whole groups")
	{
		file.AddIntOption("user.recent_projects.project_0.index", 0);
		file.AddIntOption("user.recent_projects.count", 1);
		file.AddBoolOption("user.show_welcome_screen", true);
		CHECK(file.RemoveOption("user.recent_projects"));
		CHECK_FALSE(file.HasGroup("user.recent_projects"));
		CHECK(file.HasOption("user.show_welcome_screen"));
		CHECK_FALSE(file.RemoveOption("user.recent_projects"));
		CHECK_FALSE(file.RemoveOption("nothing.here"));
	}

	SECTION("Names and paths in file order")
	{
		file.AddIntOption("b.second", 2);
		file.AddIntOption("b.first", 1);
		file.AddIntOption("a.value", 3);
		CHECK(file.GetChildNames() == std::vector<std::string>{ "b", "a" });
		CHECK(file.GetChildNames("b") == std::vector<std::string>{ "second", "first" });
		CHECK(file.GetOptionPaths() == std::vector<std::string>{ "b.second", "b.first", "a.value" });
	}

	SECTION("Dirty only when something changes")
	{
		CHECK_FALSE(file.IsDirty());
		file.AddIntOption("ui.font_size", 12);
		CHECK(file.IsDirty());
		const std::string text = file.Serialize();

		ConfigFile copy;
		REQUIRE(copy.Parse(text));
		CHECK(copy.GetIntOption("ui.font_size") == 12);
	}

	SECTION("A float that comes out the same in the file is no change (0.1f against the 0.1 read back)")
	{
		Scratch scratch;
		WriteText(scratch.path / "editor_settings.cfg", "editor = { snapping = { scale = 0.1; translation = 1.0; }; };\n");
		ConfigFile loaded(VFS::ToUtf8(scratch.path / "editor_settings.cfg"));
		REQUIRE(loaded.Load() == ConfigFile::LoadResult::Loaded);
		CHECK(loaded.AddFloatOption("editor.snapping.scale", 0.1f));
		CHECK(loaded.AddFloatOption("editor.snapping.translation", 1.0f));
		CHECK_FALSE(loaded.IsDirty());
		CHECK(loaded.AddFloatOption("editor.snapping.scale", 0.25f));
		CHECK(loaded.IsDirty());
	}
}

TEST_CASE("ConfigFile files through the VFS", "[settings][config][vfs]")
{
	UserMount mount;
	const fs::path folder = mount.scratch.path / "settings";

	SECTION("A missing file, then one saved atomically (folders created, no temporary left behind)")
	{
		ConfigFile file("user:/settings.cfg", &mount.vfs);
		CHECK_FALSE(file.FileExists());
		CHECK(file.Load() == ConfigFile::LoadResult::Missing);

		file.AddStringOption("x_plane.path", "/home/me/X-Plane 12");
		file.AddIntOption("ui.font_size", 13);
		std::string error;
		REQUIRE(file.Save(&error));
		CHECK_FALSE(file.IsDirty());
		CHECK(file.FileExists());
		CHECK(fs::is_regular_file(folder / "settings.cfg"));
		CHECK(fs::equivalent(file.GetPhysicalPath(), folder / "settings.cfg"));

		size_t entries = 0;
		for (const auto &entry : fs::directory_iterator(folder))
		{
			(void)entry;
			++entries;
		}
		CHECK(entries == 1);

		ConfigFile again("user:/settings.cfg", &mount.vfs);
		REQUIRE(again.Load() == ConfigFile::LoadResult::Loaded);
		CHECK(again.GetStringOption("x_plane.path") == "/home/me/X-Plane 12");
		CHECK(again.GetIntOption("ui.font_size") == 13);
		CHECK_FALSE(again.IsDirty());
	}

	SECTION("Every instance has its own document (two files never share values)")
	{
		ConfigFile settings("user:/settings.cfg", &mount.vfs);
		ConfigFile preferences("user:/user_preferences.cfg", &mount.vfs);
		settings.AddStringOption("x_plane.path", "C:/X-Plane 12");
		preferences.AddBoolOption("user.show_welcome_screen", false);
		REQUIRE(settings.Save());
		REQUIRE(preferences.Save());

		ConfigFile reread("user:/user_preferences.cfg", &mount.vfs);
		REQUIRE(reread.Load() == ConfigFile::LoadResult::Loaded);
		CHECK_FALSE(reread.HasGroup("x_plane"));
		CHECK(reread.GetChildNames() == std::vector<std::string>{ "user" });
		CHECK_FALSE(settings.HasGroup("user"));
	}

	SECTION("A file spelled in another case is found (packs and settings copied from Windows)")
	{
		WriteText(folder / "Prefs" / "Editor_Settings.CFG", "editor = { snapping = { enabled = true; }; };\n");
		ConfigFile file("user:/prefs/editor_settings.cfg", &mount.vfs);
		REQUIRE(file.Load() == ConfigFile::LoadResult::Loaded);
		CHECK(file.GetBoolOption("editor.snapping.enabled") == true);
	}

	SECTION("A file that doesn't parse is reported and left alone; Backup keeps a copy")
	{
		const std::string broken = "ui = { font_size = 12; \n theme = ; };\n";
		WriteText(folder / "settings.cfg", broken);
		ConfigFile file("user:/settings.cfg", &mount.vfs);
		std::string error;
		CHECK(file.Load(&error) == ConfigFile::LoadResult::Invalid);
		CHECK(error.find("line 2") != std::string::npos);
		CHECK(file.GetChildNames().empty());
		CHECK(ReadText(folder / "settings.cfg") == broken);

		REQUIRE(file.Backup(".invalid", &error));
		CHECK(ReadText(folder / "settings.cfg.invalid") == broken);
	}

	SECTION("A UTF-8 byte order mark (Windows editors) is fine")
	{
		WriteText(folder / "settings.cfg", "\xEF\xBB\xBFui = { font_size = 15; };\n");
		ConfigFile file("user:/settings.cfg", &mount.vfs);
		REQUIRE(file.Load() == ConfigFile::LoadResult::Loaded);
		CHECK(file.GetIntOption("ui.font_size") == 15);
	}

	SECTION("A location whose mount isn't there fails, and so does a read-only mount's save")
	{
		ConfigFile unmounted("nowhere:/settings.cfg", &mount.vfs);
		std::string error;
		CHECK(unmounted.Load(&error) == ConfigFile::LoadResult::Failed);
		CHECK_FALSE(error.empty());
		CHECK_FALSE(unmounted.Save());

		REQUIRE(mount.vfs.Mount("xplane", mount.scratch.path / "xplane", true));
		ConfigFile readOnly("xplane:/settings.cfg", &mount.vfs);
		readOnly.AddIntOption("a.b", 1);
		CHECK_FALSE(readOnly.Save(&error));
		CHECK_FALSE(fs::exists(mount.scratch.path / "xplane" / "settings.cfg"));
	}

	SECTION("A physical location works without a mount (UTF-8 names included)")
	{
		const fs::path path = mount.scratch.path / VFS::FromUtf8("Pr\xC3\xA9" "f\xC3\xA9rences") / "user_preferences.cfg";
		ConfigFile file(VFS::ToUtf8(path));
		CHECK_FALSE(file.IsVirtual());
		file.AddStringOption("user.startup_project", "\xC3\x89t\xC3\xA9.edX");
		REQUIRE(file.Save());
		CHECK(fs::is_regular_file(path));

		ConfigFile again(VFS::ToUtf8(path));
		REQUIRE(again.Load() == ConfigFile::LoadResult::Loaded);
		CHECK(again.GetStringOption("user.startup_project") == "\xC3\x89t\xC3\xA9.edX");
	}

	SECTION("Saving over a file replaces it whole")
	{
		ConfigFile file("user:/settings.cfg", &mount.vfs);
		file.AddStringOption("ui.theme", "a much longer value than the next one");
		REQUIRE(file.Save());
		file.AddStringOption("ui.theme", "dark");
		REQUIRE(file.Save());

		ConfigFile again("user:/settings.cfg", &mount.vfs);
		REQUIRE(again.Load() == ConfigFile::LoadResult::Loaded);
		CHECK(again.GetStringOption("ui.theme") == "dark");
		CHECK(again.GetOptionPaths() == std::vector<std::string>{ "ui.theme" });
	}
}

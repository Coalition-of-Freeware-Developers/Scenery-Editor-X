# Scenery Editor X - Settings System Documentation

---

## Overview

The Settings System keeps the editor's configuration in a few small files in the user's settings folder. Every one of
them is read and written through the **virtual file system** (`filesystem/vfs.h`), so the same code works on Windows,
macOS and Linux:

- Files are addressed with virtual paths such as `user:/settings.cfg`. The `user:/` mount is the roaming settings
  folder each platform has, mounted at startup by `IO::FileSystem::InitAppLocations()`.
- Reads find a file whatever case the disk spells it in (settings copied from Windows to a case-sensitive Linux disk
  still load).
- Writes are atomic: the bytes go to a temporary file that is checked and then renamed over the target, so a crash,
  a full disk or a failed write never leaves a half-written settings file.
- Paths are UTF-8 everywhere (`VFS::ToUtf8` / `VFS::FromUtf8` at the `std::filesystem` boundary), so a user folder such
  as `C:\Users\Zoë` or `/home/日本` works.
- A file that doesn't parse is never silently overwritten: it is kept as `<name>.invalid` before the defaults replace
  it, and when even that copy can't be made the file is left alone and the defaults are used in memory only.

## Where the files are

| File | Virtual path | Contents | Class |
|---|---|---|---|
| `settings.cfg` | `user:/settings.cfg` | X-Plane install, application, UI and project defaults | `Settings` (`Application::GetSettings()`) |
| `user_preferences.cfg` | `user:/user_preferences.cfg` | Welcome screen, startup project, recent projects | `UserPreferences` |
| `editor_settings.cfg` | `user:/editor_settings.cfg` | Snapping steps, gizmo axes / pivot, viewport and display options | `EditorSettingsManager` / `EditorSettingsSerializer` |
| `editor_preferences.json` | `user:/editor_preferences.json` | Navigation keymap, orbit / zoom speeds, VIEWPORT 2 camera | `EditorPreferences` |
| `editor.ini` | `user:/editor.ini` | ImGui's dock layout and window placement | `UILayer` (ImGui's own file IO is off) |

`user:/` is:

| Platform | Folder |
|---|---|
| Windows | `%APPDATA%\SceneryEditorX` |
| macOS | `~/Library/Application Support/SceneryEditorX` |
| Linux | `$XDG_CONFIG_HOME/SceneryEditorX` (`~/.config/SceneryEditorX`) |

The other mounts the editor sets up are `data:/` (machine-local data), `cache:/` (rebuildable caches), `logs:/` and
`temp:/`; see `VFS::AppLocation` in `filesystem/vfs.h`.

## Architecture

### ConfigFile

**Location:** `source/SceneryEditorX/settings/config_file.h/.cpp`

One libconfig (`.cfg`) document and the file it lives in. Every instance has its own document and every method is
thread safe. It depends on the standard library, libconfig and `vfs.h` only, so it is unit tested on every platform
(`source/Tests/settings_tests/SettingsTest.cpp`).

```cpp
ConfigFile file("user:/my_module.cfg");      // or a physical path (UTF-8); an optional VFS::FileSystem * for tests

switch (file.Load(&error))
{
    case ConfigFile::LoadResult::Loaded:  break;           // read and parsed
    case ConfigFile::LoadResult::Missing: break;           // no file yet: empty document
    case ConfigFile::LoadResult::Invalid: file.Backup(".invalid"); break;  // doesn't parse: file left as it is
    case ConfigFile::LoadResult::Failed:  break;           // unreadable (or the mount isn't there): don't save over it
}

file.AddIntOption("module.size", 3);               // dotted paths; groups are created on the way
const int size = file.GetIntOption("module.size", 1);
if (file.IsDirty())
    file.Save(&error);                              // atomic, folders created as needed
```

Value rules:

- `Add*Option(path, value)` stores a value and returns `false` when the path can't hold one: a name libconfig doesn't
  allow (`2d`, `a b`), a value where a group should be, or a group where the value would go. A value of another type is
  replaced (an older version wrote it differently).
- `Get*Option(path, default)` returns the default when there's no value. Numbers convert between int, int64 and float;
  text that is a number or `true` / `false` reads as one (that is what `SetOption` stores).
- `HasOption` is true for values, `HasGroup` for groups; `RemoveOption` removes either.
- `GetChildNames(group)` and `GetOptionPaths()` list what's there in file order.
- `IsDirty()` is true only when something actually changed since the last `Load` / `Save`, so callers can skip writes.

### Settings

**Location:** `source/SceneryEditorX/settings/settings.h/.cpp`

The application's settings: a `ConfigFile` (so the whole option API above works on it) plus the X-Plane install and
application values. The editor keeps one for its lifetime, `Application::GetSettings()`, opened right after the app's
folders are set up (`Application::SetUpAppLocations()`), so `user:/` is mounted and older versions' files have moved.

```cpp
Settings &settings = Application::Get().GetSettings();
const std::string xplane = settings.GetXPlanePath();
const std::filesystem::path projects = settings.GetDefaultProjectFolder();   // "~" expanded, platform default

if (settings.SetXPlanePath(folder))      // validated (Resources/default scenery + Custom Scenery)
    settings.WriteSettings();
```

`Settings::Open(location)`:

1. Reads the file (`ReadSettings`). A file that doesn't parse is kept as `settings.cfg.invalid`.
2. Fills in every section and value the file lacks (`ApplyDefaults`): a new file, or one an older version wrote, gets
   the new settings without losing what's there.
3. Upgrades values older versions wrote that no longer work (the Windows-only `~\Documents\SceneryEditorX` project
   folder becomes the platform's Documents folder).
4. Searches for X-Plane when the saved folder isn't an X-Plane install (`DetectXPlanePath`, the X-Plane library's
   install finder: install lists, Steam, the registry, common folders).
5. Writes the file when anything changed. A file that exists but couldn't be read is never written over.

### UserPreferences

**Location:** `source/SceneryEditorX/settings/user_settings.h/.cpp`

Welcome screen, startup project and the recent projects list, in their own `ConfigFile`
(`UserPreferences::DEFAULT_LOCATION`, or a path passed to the constructor).

- Recent projects are matched by file, not by spelling: `C:\a\b.edX` and `c:/a/b.edX` are one entry on Windows and
  macOS (`VFS::PathKey`). The list keeps the 10 most recent; two projects opened in the same second keep their order.
- Times are stored as `DD/MM/YYYY HH:MM:SS` in local time, which every version reads. `StringToTime` parses them with
  fixed formats only (older versions tried the regional format first, which could read a day as a month), and it also
  reads ISO 8601 (`2026-10-24T13:45:10Z`, `TimeToIso8601`): the files can move to ISO once builds older than this one
  are no longer in use.
- `SavePreferences()` writes only when something changed; the destructor saves unsaved changes.
- Versions up to 0.2.30 shared one document between every settings file, so `user_preferences.cfg` got a copy of
  `settings.cfg`'s sections. They are removed the first time the file is loaded.

```
user =
{
  show_welcome_screen = true;
  startup_project = "";
  recent_projects =
  {
    project_0 =
    {
      name = "5S4 - Toledo State";
      file_path = "K:/X-Plane 12/Custom Scenery/5S4 - Toledo State/5S4.edX";
      last_opened = "24/10/2026 13:45:10";
    };
    count = 1;
  };
};
```

### EditorSettings

**Location:** `source/Editor/settings/editor_settings.h/.cpp`

`EditorSettings` holds the editor's tool options; `EditorSettingsManager` reads and writes one to its own `ConfigFile`
(`user:/editor_settings.cfg`); `EditorSettingsSerializer` is what the editor calls:

- `EditorSettingsSerializer::Init()` (in `EditorLayer::OnAttach`) loads the file into `EditorSettings::Get()` and applies
  the snapping and gizmo options to the transform tool (`TransformTool::GetSettings()`), so snapping on/off, the
  move / rotate / scale steps, local axes and the active-object pivot survive a restart.
- `EditorSettingsSerializer::Shutdown()` (in `EditorLayer::OnDetach`) reads the tool's current options back and saves
  them when they changed.
- Scripted self-tests (`--tools-selftest`, `--gizmo-selftest`, ...) start from the defaults and never write the file.

```
editor =
{
  snapping = { enabled = false; translation = 1.0; rotation = 15.0; scale = 0.1; };
  gizmo = { local_space = false; pivot_active = false; };
  viewport = { show_colliders = false; show_lights = true; show_grid = true; };
  display = { vsync = true; max_fps = 144; };
};
```

The `viewport` and `display` values are stored but not read by the viewport overlays or the frame limiter yet.

### EditorPreferences

**Location:** `source/Editor/settings/editor_preferences.h/.cpp`

The navigation keymap (Blender, 3ds Max, Maya), orbit inversion and speeds, smooth views and VIEWPORT 2's camera, kept
as JSON in `user:/editor_preferences.json` and written with `VFS::FileSystem::Write`. Edit them through
`EditorPreferences::Get()` and call `EditorPreferences::Save()` after a change (Edit > Preferences does).

### The ImGui layout (editor.ini)

`UILayer` turns ImGui's own file IO off (`io.IniFilename = nullptr`), loads `user:/editor.ini` with
`ImGui::LoadIniSettingsFromMemory` before the first frame, writes it through the VFS whenever ImGui sets
`io.WantSaveIniSettings` (a few seconds after a panel moved or docked) and once more when the UI shuts down.

## settings.cfg reference

```
application =
{
  version = "0.2.30.531";      # Written by the version that last saved the file
  no_titlebar = false;
};
x_plane =
{
  version = "X-Plane 12";
  path = "K:\\SteamLibrary\\steamapps\\common\\X-Plane 12";
  bin_path = "K:\\SteamLibrary\\steamapps\\common\\X-Plane 12\\bin";
  resources_path = "K:\\SteamLibrary\\steamapps\\common\\X-Plane 12\\Resources";
  is_steam = true;
};
ui =
{
  theme = "dark";
  font_size = 12;
  language = "english";
};
project =
{
  auto_save = true;
  auto_save_interval = 5;     # minutes
  backup_count = 3;
  default_project_dir = "C:\\Users\\me\\Documents\\SceneryEditorX";   # "~/..." is expanded to the home folder
};
```

Paths are stored as the operating system spells them (backslashes on Windows, escaped by libconfig).

## Adding a setting

1. Pick the file: application-wide behaviour goes in `settings.cfg` (`Settings`), tool options in
   `editor_settings.cfg` (`EditorSettings`), per-user history in `user_preferences.cfg` (`UserPreferences`).
2. Give it a default where the class fills in defaults (`Settings::ApplyDefaults`, the `EditorSettings` field
   initializer plus `EditorSettingsManager::ReadSettings` / `WriteSettings`). Files written by older versions then get
   it automatically.
3. Clamp numbers you read to the range the UI allows.
4. Never open the file with `std::ofstream`, `fopen` or libconfig's `readFile` / `writeFile`: go through `ConfigFile`
   (or `VFS::Get().Read` / `Write` for other formats).

A new kind of file:

```cpp
class MyModuleSettings
{
public:
    MyModuleSettings() : m_File("user:/my_module.cfg") { Load(); }

    void Load()
    {
        std::string error;
        if (m_File.Load(&error) == ConfigFile::LoadResult::Invalid)
            m_File.Backup(".invalid");
        m_Size = std::clamp(m_File.GetIntOption("my_module.size", 3), 1, 64);
    }

    bool Save()
    {
        m_File.AddIntOption("my_module.size", m_Size);
        return !m_File.IsDirty() || m_File.Save();
    }

private:
    ConfigFile m_File;
    int m_Size = 3;
};
```

## Testing

`SettingsTest` (`source/Tests/settings_tests/SettingsTest.cpp`) covers `ConfigFile`: every value type, conversions,
type replacement, groups never replaced by values, dirty tracking, saving through a VFS mount (atomic, no temporary
files left), two instances never sharing values, case-insensitive lookups, files that don't parse (reported with the
line, left alone, backed up), byte order marks, missing and read-only mounts, and physical UTF-8 paths. It needs only
the standard library and libconfig, so it builds with MSVC, GCC and Clang; see AGENTS.md for the command line.

## Platform notes

- **Windows:** `%APPDATA%` comes from the wide environment (`_wgetenv`), so profile folders with any characters work.
  The regional date format (`GetSystemDateTimeFormat`, display only) is read with `GetLocaleInfoW`.
- **macOS:** `~/Library/Application Support/SceneryEditorX`. The regional format comes from Core Foundation and its
  ICU pattern is converted to a `strftime` format.
- **Linux:** the XDG base directories are honoured (`$XDG_CONFIG_HOME`, and `XDG_DOCUMENTS_DIR` for the default
  project folder). The regional format is read with `newlocale` / `nl_langinfo_l`, without changing the process locale.

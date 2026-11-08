# Settings System Instructions for GitHub Copilot

## Overview

These instructions guide the implementation and use of the Settings System in Scenery Editor X. Settings are small
libconfig (`.cfg`) files in the user's settings folder, read and written through the virtual file system so they work
the same on Windows, macOS and Linux. `docs/settings-system-documentation.md` is the full reference.

| Class | File | Use it for |
|---|---|---|
| `ConfigFile` (`settings/config_file.h`) | any `.cfg` | One libconfig document + its file, through the VFS |
| `Settings` (`settings/settings.h`) | `user:/settings.cfg` | Application settings, the X-Plane install (`Application::Get().GetSettings()`) |
| `UserPreferences` (`settings/user_settings.h`) | `user:/user_preferences.cfg` | Welcome screen, startup project, recent projects |
| `EditorSettings*` (`Editor/settings/editor_settings.h`) | `user:/editor_settings.cfg` | Tool options (snapping, gizmo) |
| `EditorPreferences` (`Editor/settings/editor_preferences.h`) | `user:/editor_preferences.json` | Navigation keymap, viewport camera |

## Core Principles

### 1. All settings file IO goes through the VFS

```cpp
// ✅ CORRECT - a virtual path in the settings folder, read and written by ConfigFile
ConfigFile file("user:/my_module.cfg");
file.Load(&error);
file.AddIntOption("my_module.size", 3);
file.Save(&error);

// ❌ WRONG - working-folder paths (an install's is read-only), platform paths built by hand, direct file IO
// std::ofstream out("config/my_module.cfg");
// libconfig::Config cfg; cfg.writeFile((appdata / "x.cfg").string().c_str());   // ANSI path, not atomic
// std::getenv("APPDATA")                                                       // Windows only
```

- `user:/` (settings), `data:/`, `cache:/`, `logs:/` and `temp:/` are mounted at startup
  (`IO::FileSystem::InitAppLocations`). Don't open settings before `Application::SetUpAppLocations()` has run.
- Convert paths with `VFS::ToUtf8` / `VFS::FromUtf8`, never `path.string()` (it uses the ANSI code page on Windows).
- For formats other than libconfig (JSON, ImGui's ini) use `VFS::Get().Read(...)` / `VFS::Get().Write(...)`: atomic,
  folders created, case-insensitive lookups.

### 2. Every file has its own instance; nothing is static

`ConfigFile` and `Settings` keep their document per instance. Never add static document state: two files must never
share values (versions up to 0.2.30 did, and `user_preferences.cfg` ended up with a copy of `settings.cfg`).

### 3. Hierarchical, snake_case paths with defaults

```cpp
// ✅ CORRECT - dotted paths with module prefixes, a default on every read, numbers clamped to what the UI allows
const int size = std::clamp(file.GetIntOption("renderer.shadow_map_size", 2048), 256, 8192);
const bool enabled = file.GetBoolOption("editor.snapping.enabled", false);

// ❌ WRONG - flat names, no defaults
// file.GetIntOption("ShadowMapSize");
```

Names follow libconfig's rules: a letter first, then letters, digits, `_` or `-`. `Add*Option` returns `false` for a
name it can't store.

### 4. Never lose the user's file

```cpp
switch (file.Load(&error))
{
    case ConfigFile::LoadResult::Invalid:
        // Keep a copy before the defaults replace it; if that fails, don't write the file at all
        if (!file.Backup(".invalid", &backupError))
            m_Writable = false;
        break;
    case ConfigFile::LoadResult::Failed:
        m_Writable = false;     // It's there but unreadable: never write over it
        break;
    default:
        break;
}
```

### 5. Write only what changed

`ConfigFile::IsDirty()` is true only when a value actually changed. Store the values, then save when dirty (or when
there is no file yet):

```cpp
file.AddBoolOption("editor.gizmo.local_space", localSpace);
if (file.IsDirty() || !file.FileExists())
    file.Save(&error);
```

## Adding a setting

1. Choose the file (application behaviour: `Settings`; tool options: `EditorSettings`; user history: `UserPreferences`).
2. Give it a default where that class fills in defaults (`Settings::ApplyDefaults`, the `EditorSettings` field
   initializer and `EditorSettingsManager::ReadSettings` / `WriteSettings`), so files from older versions get it.
3. Validate and clamp what you read; log corrections with a tag.

## Logging

Use tagged logging: `SEDX_CORE_*_TAG("Settings", ...)` in AppCore, `EDITOR_*_TAG("EditorSettings", ...)` in the
editor. TRACE for reads and writes, INFO for one-time upgrades, WARN for files that didn't parse (with the line) and
values that were corrected, ERROR for files that couldn't be read or written. `ConfigFile` itself never logs: it returns
errors, the caller decides how loud they are.

## Testing

Put `ConfigFile`-level behaviour in `source/Tests/settings_tests/SettingsTest.cpp` (standard library + libconfig + VFS
only, so it builds on every platform). Test against a `VFS::FileSystem` of your own mounted on a scratch folder, never
the user's real `user:/`:

```cpp
VFS::FileSystem vfs;
vfs.Mount(VFS::MOUNT_USER, scratch / "settings");
ConfigFile file("user:/settings.cfg", &vfs);
```

## Platform Notes

| Platform | `user:/` |
|---|---|
| Windows | `%APPDATA%\SceneryEditorX` |
| macOS | `~/Library/Application Support/SceneryEditorX` |
| Linux | `$XDG_CONFIG_HOME/SceneryEditorX` (`~/.config/SceneryEditorX`) |

Paths stored in settings are UTF-8 as the operating system spells them. Paths a user might type with `~` (the default
project folder) are expanded with `VFS::GetFolder(VFS::Folder::Home)` when read (`Settings::GetDefaultProjectFolder`).

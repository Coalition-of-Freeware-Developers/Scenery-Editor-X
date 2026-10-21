# Where Scenery Editor X keeps files

Your projects and scenery packs live wherever you create them. Separately, the editor keeps its own settings, logs, recovery data and caches in your user profile, never in your projects or in X-Plane's folders.

## On Windows

| Folder | What's in it | Safe to delete? |
|---|---|---|
| `%APPDATA%\SceneryEditorX` | Settings and preferences | Resets the editor to its first-launch state |
| `%APPDATA%\SceneryEditorX\logs` | Log files and crash reports | Yes |
| `%LOCALAPPDATA%\SceneryEditorX\project-cache` | Recovery copies, backups and snapshots of your projects | Only when no project has unsaved work you want to recover |
| `%LOCALAPPDATA%\SceneryEditorX\cache` | Rebuildable caches | Always (they're rebuilt as needed) |
| `%TEMP%\SceneryEditorX` | Temporary files | Yes, while the editor isn't running |

Type a folder's name (for example `%LOCALAPPDATA%\SceneryEditorX`) into File Explorer's address bar to open it.

### Settings

| File | Holds |
|---|---|
| `editor_preferences.json` | Navigation keymap, invert and speed options, smooth views, VIEWPORT 2's camera |
| `EditorLayout.json` | Which panels are open and where they're docked |
| `xplane_library.json` | The X-Plane folder and the library folders you track |
| `xplane_airports.json` | Scenery packs the airport index follows |
| `settings.cfg` | General settings and recent projects |

### Logs

`logs\SceneryEditorX.log` is the editor's log: the same messages as the CONSOLE panel, kept after the editor closes. If the editor crashes, the reason and where it happened are added at the end, and a crash dump `SceneryEditorX_crash.dmp` is written beside it. Attach both when you [report a problem](troubleshooting.md#reporting-a-problem).

### The project cache
Each project file you open gets a folder in `project-cache`, named by a hash of the project's path. In it:

| File | What it is |
|---|---|
| `source.txt` | The path of the project file the folder belongs to |
| `snapshot.sex` | The project as last opened or saved; reopening an unchanged project reads it instead of parsing the file again |
| `recovery.sex` | Unsaved changes, written every minute while there are any and when the editor quits; removed when you save or discard them |
| `backups\` | The previous project file from before each save, named `<UTC time>-<number>-<file name>`; the newest five are kept |

To find a project's folder, open the `source.txt` files: each names its project.

### Caches

| Folder or file | What it is |
|---|---|
| `airport_index.bin` | The index of every airport in your install, so the AIRPORTS panel is ready at once |
| `gateway\` | Scenery Gateway downloads (the airport list is refreshed after a day) |
| `shaders\` | Compiled shaders, so startup is quick after the first run |
| `thumbnails\` | Library asset previews, redrawn when an asset's file changes |

## On macOS and Linux

The editor's file handling is cross-platform, ready for builds on other systems:

| | macOS | Linux |
|---|---|---|
| Settings and logs | `~/Library/Application Support/SceneryEditorX` | `$XDG_CONFIG_HOME/SceneryEditorX` (usually `~/.config`) |
| Project cache | `~/Library/Application Support/SceneryEditorX` | `$XDG_DATA_HOME/SceneryEditorX` (usually `~/.local/share`) |
| Caches | `~/Library/Caches/SceneryEditorX` | `$XDG_CACHE_HOME/SceneryEditorX` (usually `~/.cache`) |

## What the editor writes in X-Plane's folders

Only what you ask for:

- the scenery packs you create, save or export, wherever you choose (often in `Custom Scenery`);
- `Custom Scenery/scenery_packs.ini`, when you tick **List the pack in scenery_packs.ini** on export or **Add to the top** when installing from the Gateway.

It never writes into X-Plane's `Resources`, `Global Scenery` or default libraries. Every file it writes is written atomically: the new file replaces the old one only once it's complete, so a crash never leaves a half-written file behind.

## Moving from older test builds

Earlier test builds kept caches and recovery data in other places. On startup the editor moves anything it finds to the folders above, once, never overwriting a newer file, and logs each move under the *Files* tag in the console.

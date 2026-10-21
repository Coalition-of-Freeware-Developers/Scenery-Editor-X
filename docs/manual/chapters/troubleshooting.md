# Troubleshooting

Solutions to the problems testers run into most, and how to report one that isn't here. For any problem, the first place to look is the **CONSOLE** panel (or the log file, if the editor won't start): it usually says exactly what went wrong.

## Starting the editor

### A message says Vulkan is missing or too old

Scenery Editor X needs a graphics driver with Vulkan 1.3. Install the latest driver from your graphics card's maker ([NVIDIA](https://www.nvidia.com/drivers), [AMD](https://www.amd.com/support), [Intel](https://www.intel.com/content/www/us/en/support/detect.html)), then start the editor again. [Checking Vulkan](installing.md#checking-vulkan) explains each message; run `SceneryEditorX.exe --check-vulkan` to test without opening the editor.

On a laptop with two graphics chips, make sure the editor runs on the more capable one: **Windows Settings › System › Display › Graphics**, add `SceneryEditorX.exe` and choose **High performance**.

### The editor closes during startup without a message

Open `%APPDATA%\SceneryEditorX\logs\SceneryEditorX.log` and read the end. When the editor crashes it writes the reason and where it happened there, and a crash dump (`SceneryEditorX_crash.dmp`) beside it. [Report the problem](#reporting-a-problem) with both files.

If the log stops without a crash report, try starting the editor with `--verbose` for more detail, and include that log in your report.

### The first start is slow

The first time, the editor compiles its shaders and indexes your airports. Both are cached, so later starts are quicker. If startup is suddenly slow again, the cache may have been cleared (for example by the uninstaller's *Settings, logs and caches* option).

## X-Plane and libraries

### The X-PLANE LIBRARY is empty, or says the folder is wrong

- Click **Detect** in the install bar and choose your X-Plane 12 install from the list.
- Or click **Browse…** and choose the X-Plane folder itself, the one that holds `Custom Scenery`, `Resources` and the X-Plane application, not a folder inside it.
- Hover the status icon: it says what's wrong with the folder in use.

After changing the folder, wait for *Scanning libraries…* to finish.

### A library I installed doesn't show up

- Libraries installed while the editor is open need a **Rescan** (install bar, or **X-Plane › Rescan Libraries**).
- The pack must have a `library.txt` at its top level. Tick **All packages** in the **Libraries** tab to see packs without one.
- A library outside `Custom Scenery` must be tracked: **Track folder…**.

### Objects or facades are missing from the scene

Art the editor can't find or read isn't drawn, and is listed:

- PROPERTIES shows the art of the selected airport or project that couldn't be found;
- the CONSOLE names each missing file;
- the project's **VALIDATION** list names missing assets.

Usually a library the scenery depends on isn't installed. The asset's **Exported by** list in the X-PLANE LIBRARY shows which library should provide it; a red mark means that library lists the file but the file is missing.

## Airports

### An airport isn't in the Installed list

- Click **Rescan** in the AIRPORTS panel: a pack installed while the editor runs isn't indexed yet.
- Check the filters above the list: the scenery pack, the type and **3D only**.
- The list shows the first 1000 matches; type more of the name or code.

### I can't save an airport

X-Plane's Global Airports, and any airport file the editor can't write, are read-only. Use **Save As…** to write your edits into a scenery pack of your own.

### My exported airport doesn't show up in X-Plane
Check, in order:

1. **The pack is in `Custom Scenery`.** The Export dialog says *Outside Custom Scenery* when it isn't.
2. **The pack is listed and enabled in `scenery_packs.ini`.** Tick **List the pack in scenery_packs.ini** when exporting, or check the file: the pack's line should start with `SCENERY_PACK`, not `SCENERY_PACK_DISABLED`.
3. **Nothing with higher priority replaces it.** When two packs have an airport with the same ID, X-Plane uses the one in the pack higher in `scenery_packs.ini`.
4. **There are no validation problems.** X-Plane may ignore an airport it can't read; X-Plane's own `Log.txt` names the file and the problem.
5. **X-Plane was restarted.** X-Plane reads scenery packs when it starts.

## The viewport

### The viewport is empty or grey

The camera is probably far from the scene. Press <kbd>Home</kbd> (frame everything), or **Reset camera** at the right of the viewport toolbar. Some airports have an outlying element far from the field; select something near the field and press <kbd>F</kbd> (<kbd>Z</kbd> in 3ds Max) to frame it.

### The camera moves the wrong way

Orbiting follows the 3D-package convention: the scene turns with the mouse. To make the camera move with the mouse instead, tick **Invert orbit horizontally** and **Invert orbit vertically** in **Edit › Preferences…**. **Invert look up / down** does the same for right-button looking.

### Keys don't do anything

- Tool and camera keys only work with the mouse over the viewport.
- No shortcut works while you're typing in a field: click in the viewport first.
- While the right mouse button is held, <kbd>Q</kbd> <kbd>W</kbd> <kbd>E</kbd> fly the camera instead of picking tools.

### The editor is slow

- Check the frame time at the right of the status bar, and the RENDERER panel's **FRAME** and **GPU TIMINGS** to see where the time goes.
- Lower **Shadows** and **Ambient occlusion** in RENDERER › **QUALITY**, or turn off passes in **XP12 EMULATION**.
- Close VIEWPORT 2 when you don't need it: it renders the scene a second time.
- Very large airports (tens of thousands of objects) take a moment to load; objects keep appearing as they load.

## Projects

### The editor offered to recover unsaved work

The editor closed before you saved. **Restore** brings the changes back (nothing is written until you save), **Discard** throws them away, **Decide later** keeps them for next time. See [Recovering unsaved work](projects.md#recovery-and-backups).

### A project won't open

The CONSOLE says why:

- **A `.sex` file is damaged** (*checksum mismatch*, *truncated*, or a line-ending problem): it was probably copied as text. Get a good copy, or restore a backup from the project cache.
- **An `.edX` file has broken JSON** (often after a manual merge): the message names the line. Fix it in a text editor, or restore a backup.

Backups of the last five saves are in the project's cache folder: see [The project cache](file-locations.md#the-project-cache).

## Reporting a problem
Report problems on the project's [issue tracker](https://github.com/Coalition-of-Freeware-Developers/Scenery-Editor-X/issues). A good report has:

- **the version**, from **Help › About Scenery Editor X**;
- **what you did**, step by step, what you expected, and what happened instead;
- **the log**, `%APPDATA%\SceneryEditorX\logs\SceneryEditorX.log`, and after a crash, `SceneryEditorX_crash.dmp`;
- **a screenshot** if something looks wrong;
- **the project or airport**, if you can share it, or the airport ID and scenery pack;
- your **graphics card** and driver version.

For a performance problem, start the editor with `--perf-log` and reproduce it: the log then records frame timings every two seconds.

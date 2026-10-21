# Command-line options

`SceneryEditorX.exe` accepts options that open things at startup, set the environment, and help with testing and troubleshooting. Most people never need them; they're useful for shortcuts, scripts, and reporting a problem precisely.

Options start with two dashes. Those that take a value expect it as the next argument; quote values that contain spaces:

```bat
SceneryEditorX.exe --project "D:\Scenery\KSEA Seattle\KSEA Seattle.edX" --env-time 18:30
```

## Opening things

| Option | Effect |
|---|---|
| `--project <file>` | Opens a project (`.edX`, `.sex` or `earth.wed.xml`). This is what double-clicking a project file runs. A relative path is relative to the folder the editor was started from. |
| `--xp-airport <id>` | Opens an airport from the airport index by its ID, for example `--xp-airport KSEA`. |
| `--xp-airport-camera x,y,z,yaw,pitch` | After the airport (or project airport) opens, puts the camera here: meters from the airport's center (+X east, +Y up, +Z south), then heading and pitch in degrees. |
| `--xp-gateway <scenery id>` | Downloads and installs a Scenery Gateway submission by its scenery ID. |
| `--xp-package-root <folder>` | Where `--xp-gateway` installs the pack. |
| `--xp-object <file.obj>` | Places an X-Plane object file in the scene (repeat for several). |
| `--xp-asset <virtual path>` | Places a library asset by its virtual path, for example `--xp-asset lib/airport/Ramp_Equipment/Fuel_Truck.obj`, once the library scan finishes. |
| `--xp-position x,y,z` | Where `--xp-object` / `--xp-asset` place things, in meters (+X east, +Y up, +Z south). |
| `--viewport2` | Opens VIEWPORT 2 at startup. |
| `--focus-panel <id>` | Brings a panel's tab to the front: `Properties`, `Outliner`, `Project`, `Airports`, `XPlaneLibrary`, `FacadeInspector`, `FacadeEditor`, `Environment`, `Assets`, `Console`, `SceneSettings`, `CameraSettings`, `RendererStats`. |
| `--library-search <text>` | Starts the X-PLANE LIBRARY with this search. |
| `--library-grid` | Starts the X-PLANE LIBRARY in thumbnail grid view. |
| `--welcome` | Shows the first-launch keymap window even if you've already chosen. |

## Projects and export, without the dialogs

| Option | Effect |
|---|---|
| `--new-airport-project "ICAO;Name;Country;lat;lon;elevation ft[;folder]"` | Creates an airport project (in Custom Scenery unless a folder is given) and opens it. If it already exists, opens it. |
| `--new-scenery-project "Name;lat;lon[;folder]"` | Creates a scenery project. Leave the coordinates empty for a project without a reference point. |
| `--show-dialog new` / `export` | Opens the New Project or Export dialog at startup. |
| `--export-scenery [folder]` | Exports the opened project as a scenery pack (into its own folder, or the folder given) once it has loaded. |
| `--project-autosave <seconds>` | How often unsaved changes are written to the recovery file (60 by default). |

## Camera

| Option | Effect |
|---|---|
| `--camera-view <view>` | Starts in a view preset: `top`, `bottom`, `front`, `back`, `left` or `right`. |
| `--camera-ortho` | Starts in orthographic projection. |
| `--lock-camera` | Freezes the startup view: the mouse and keys don't move the camera. For captures and comparisons. |

## Renderer and environment
### Environment at startup

| Option | Value |
|---|---|
| `--env-date` | `YYYY-MM-DD` (local date) |
| `--env-time` | `HH:MM` local time (`--xp-time <hours>` takes decimal hours) |
| `--env-utc-offset` | Hours, −14 to 14 |
| `--env-latlon` | `lat,lon` in degrees (instead of the airport's location) |
| `--env-visibility` | Meters, 10 to 200000 |
| `--env-rain` | 0 to 1 |
| `--env-snow` | Snow cover, 0 to 1 |
| `--env-friction` | Runway state, 0 (dry) to 15 (heavily snowy and icy) |
| `--env-temperature` | °C, −80 to 60 |
| `--env-season` | `spr`, `sum`, `fal`, `win` or `auto` |
| `--xp-lights-on` | Lights and lit textures on in daylight |

### Comparison scenarios

| Option | Effect |
|---|---|
| `--env-scenario <file.json>` | Loads a parity scenario: a camera view, a time and the weather, in the format the X-Plane reference probe writes. |
| `--env-scenario-index <n or name>` | Which scenario in the file (the first by default). |
| `--reference-image <file.png>` | Loads a reference screenshot into the viewport's REFERENCE overlay. |
| `--reference-view <mode>` | `split`, `difference`, `flicker` or `off`. |
| `--capture-png <file.png>` | Saves the viewport's image to a PNG once the scene has settled, then quits. |
| `--capture-after-frames <n>` | How many frames to wait before capturing. |

### Renderer switches

| Option | Effect |
|---|---|
| `--no-<feature>` / `--with-<feature>` | Turns one X-Plane 12 emulation pass off or on: `atmosphere`, `aerial-perspective`, `env-probes`, `cloud-shadows`, `csm`, `ao`, `auto-exposure`, `exposure-fusion`, `bloom`, `tonemap`, `fxaa`, `sss`. |
| `--all-features-off` | Every switchable pass off. |
| `--ao-quality <0–4>` | The ambient occlusion quality level (0 off … 4 highest). |
| `--ao-gtao` | The older GTAO ambient occlusion instead of CACAO, for comparison. |
| `--debug-view <n or name>` | Starts with a RENDERER debug view (albedo, normals…). |
| `--no-gpu-scene` | Draws without the GPU-driven scene (one draw call per object), for comparison. |
| `--no-gpu-occlusion` | GPU-driven scene without occlusion culling. |
| `--full-shaders` | Compiles every shader at startup instead of on first use. |
| `--recompile-shaders` | Ignores the compiled-shader cache. |
| `--clear-cache` | Clears the editor's temporary cache at startup. |

## Diagnostics

| Option | Effect |
|---|---|
| `--check-vulkan` | Checks Vulkan the way the renderer will and exits without opening a window. Exit codes below. |
| `--perf-log` | Writes frame timings to the log every two seconds. |
| `--verbose` | More detail in the log. |
| `--renderdoc` | Loads RenderDoc's capture library at startup (developers; RenderDoc must be installed). |
| `--tools-selftest` | Runs the editor's automated viewport tool tests on the open airport or project and logs PASS / FAIL. For developers: it edits the scene (and undoes its edits). |

### Vulkan check exit codes

| Code | Meaning |
|---|---|
| `0` | Ready: a Vulkan 1.3 runtime and driver |
| `2` | No Vulkan runtime (`vulkan-1.dll`), or a damaged one |
| `3` | The Vulkan runtime is older than 1.3 |
| `4` | No graphics driver offers Vulkan |
| `5` | The driver offers Vulkan older than 1.3 |

```bat
SceneryEditorX.exe --check-vulkan
echo %ERRORLEVEL%
```

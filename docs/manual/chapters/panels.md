# Panel reference

Every panel, what's in it, and where it's explained in full. Show or hide a panel with its button on the panel strip (the right edge of the window) or from the **Window** menu; drag its tab to move it (see [Arranging panels](interface.md#arranging-panels)).

## VIEWPORT

The 3D view of the scene, with the viewport toolbar across its top: the editing tools and their options, orbit and fly navigation, the **View** menu, the **Reference image** menu and **Reset camera**. An overlay shows the navigation mode, the view and projection, and the render size.

See [Navigating the viewport](viewport-navigation.md) and [Selecting, transforming and undo](selecting-and-editing.md).

## VIEWPORT 2

A second view of the scene with its own camera, opened with **Window › New Viewport**. Each launch starts without it, and **Reset Layout** closes it. Its toolbar has **Top**, **Front** and **Right**, the orthographic toggle, **Frame the airport** and **Match the main viewport's camera**. See [A second viewport](viewport-navigation.md#a-second-viewport).

## PROPERTIES

The settings of the active selection, in sections:

| Section | Shown for |
|---|---|
| **AIRPORT** | An airport element: its own fields (runway ends, surfaces, signs, startup locations…) |
| **GENERAL PROPERTIES** | An entity: name and ID |
| **TRANSFORM** | An entity: position, rotation, scale |
| **X-PLANE OBJECT** | A placed object: file, library path, geometry, lights, visibility, animation, level of detail, whether it's in view; its animation datarefs |
| **COMPONENTS** | An entity's components |
| **FACADE** | A facade: facade, kind, height, pick walls, footprint, wall table, rebuilds |
| **NODES** | Nodes in the Nodes tool: position, handles, type, length, tightest bend, B-spline, closed; for roads, the road type and levels |

See [Selecting, transforming and undo](selecting-and-editing.md#the-properties-panel).

## OUTLINER

Every entity in the scene as a tree: the airport's elements, the project's scenery, placed objects and their parts. Click to select. The **+** button adds an **Empty Entity**.

## PROJECT

The open project:

| Section | Contents |
|---|---|
| Toolbar | New, Save, Save As…, Export, Close, Reveal |
| **PROJECT** | Kind, author, description, target X-Plane version, pack folder, datum and elevation, created and edited dates |
| **AIRPORTS** | The project's airports; **New airport**, **From apt.dat** |
| **HIERARCHY** | Every item the project places; right-click to hide, lock or remove |
| **LIBRARIES** | The X-Plane libraries the project uses; **Find Used Libraries** |
| **LIBRARY** | The project's own `library.txt` exports; **Add Files…**, **Add Export**, **Export library.txt…** |
| **VALIDATION** | Problems X-Plane may reject |

See [Projects](projects.md#the-project-panel).

## AIRPORTS

| Tab | Contents |
|---|---|
| **Installed** | Every airport in the install: search, scenery pack, type and 3D filters; Open, Gateway, Website |
| **Gateway** | The Scenery Gateway: the airport list, submissions, Install recommended / selected |
| **Airport** | The open airport: AIRPORT, METADATA, CONTENTS (with frequencies and flows), VALIDATION |

Toolbar: **New Airport…**, **Open apt.dat…**, **Rescan**. See [Airports](airports.md).

## X-PLANE LIBRARY

| Tab | Contents |
|---|---|
| **Assets** | Every library asset: search, filter drop-down with saved filters, list or thumbnail grid, grouped by library; the details pane with a preview, actions and *Exported by* |
| **Libraries** | The scanned libraries with their source, counts and issues; **Track folder…**, **All packages**, **Check files** |
| **Simulation** | The sim clock, **Force lights on**, and the datarefs that drive placed objects' animations |

Above the tabs, the install bar: the X-Plane folder, **Browse…**, **Detect**, **Rescan**. See [The X-Plane library](library.md).

## FACADES

The facade inspector: how the selected (or inspected) facade is built.

| Section | Contents |
|---|---|
| **FACADE** | The file and the objects it attaches |
| **FLOORS** | Each floor's height and roof |
| **WALLS** | Each wall type: name, rules, texture, panels or floors |
| **SPELLING PREVIEW** | How a wall's segments line up |
| **SEGMENTS** | The type 2 meshes |

Buttons: **Reload**, **Draw with it**, **Edit**. See [Drawing facades](facades.md#the-facades-inspector).

## FACADE EDITOR

Makes and edits facades: **New building**, **New sidewalk / curb**, **Edit selected**, **Open**; then name, Save, Undo, Redo, Revert, Place / Show preview, Live, Draw with it, Inspect; and the sections for the kind of facade (TEXTURES, WALL TYPES, ROOF, BUILDING; or CROSS-SECTION, TEXTURES, SEGMENTS). See [Making your own facades](facade-editor.md).

## ENVIRONMENT

| Section | Contents |
|---|---|
| **PRESETS** | Clear, Hazy, Foggy, Overcast, Rain, Snow; comparison scenarios |
| **DATE & TIME** | Date, local time, UTC offset, place (or the airport's) |
| **SUN & MOON** | Where they are, moon phase, UTC, daylight |
| **WEATHER** | Visibility, cloud layers, rain, temperature, wind, waves |
| **SURFACE** | Runway state, snow cover, wetting and drying over time |
| **SEASON** | The season, or automatic; progress toward the next |
| **SKY CONDITION** | What the weather does to the light |

See [Time, weather and the X-Plane 12 look](environment.md).

## ASSETS

A file shelf of the editor's resources folder with thumbnails: a folder tree (toggle it with the first button), the parent folder button and a breadcrumb, a search box, a filter (**All files**, **Textures**, **Shaders**, **X-Plane art assets**), the **Tile size**, and **Rescan the folder**.

## CONSOLE

The editor's log. **Clear** empties it; **Info**, **Warnings** and **Errors** choose what's listed; **Auto-scroll** follows new messages; the search box filters by message or source. Each line shows the time, the level, the part of the editor it came from, and the message.

The latest warning or error also appears in the status bar; clicking it opens the console. The same messages are written to the log file (see [Where Scenery Editor X keeps files](file-locations.md)).

## SCENE SETTINGS

| Section | Contents |
|---|---|
| **GENERAL PROPERTIES** | The scene's name, its file (or *Not saved*), how many entities it has |
| **ENVIRONMENT** | A quick time of day and wind; the ENVIRONMENT panel has the full model |

## CAMERA

| Section | Contents |
|---|---|
| **NAVIGATION** | Mode, projection, view, orbit center, distance, position, heading and pitch; Reset |
| **LENS** | Field of view |
| **EXPOSURE** | Aperture, shutter, ISO (for manual exposure) |

See [Navigating the viewport](viewport-navigation.md#the-camera-panel).

## RENDERER

| Section | Contents |
|---|---|
| **FRAME** | Frame time; main and render thread times |
| **OUTPUT** | Render and output resolution, swapchain, format |
| **GPU SCENE** | GPU-driven drawing, cull below, LOD scale, frustum and occlusion culling |
| **XP12 EMULATION** | A switch per X-Plane 12 pass; tone curve; All off / Defaults |
| **EXPOSURE** | Auto or manual, compensation, adaptation, bloom, exposure fusion |
| **ATMOSPHERE** | Haze layer, aerial perspective range, airglow, stars, reflection blur |
| **SHADOWS** | Soft edges and bias settings |
| **AMBIENT OCCLUSION** | Technique, follow camera height, radius, strength, power, horizon angle, sharpness |
| **QUALITY** | X-Plane 12's shadow, ambient occlusion, cloud, FSR and draw distance levels; FXAA; X-Plane defaults |
| **DEBUG VIEW** | One ingredient of the frame instead of the result |
| **GPU TIMINGS** | Time per pass |
| **GPU MEMORY** | Memory used, allocations, buffers, images |
| **USER INTERFACE** | Dear ImGui version, backends, floating panels |

See [Renderer quality and features](environment.md#renderer-quality-and-features).

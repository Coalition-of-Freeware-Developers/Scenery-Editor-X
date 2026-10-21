---
goal: Address the first round of tester feedback - viewport navigation (orthographic, top view, DCC keymaps, numpad views), a second pop-out viewport, projects without an airport, a cleaner library panel with saved filters, asset previews, large-polygon editing stutter, and the S_OFFSET and facade-object rendering bugs
version: 1.0
date_created: 2026-10-21
last_updated: 2026-10-21
owner: Scenery Editor X editor (Coalition of Freeware Developers)
status: 'In progress'
tags: [feature, bug, editor, viewport, camera, renderer, x-plane, project, ui]
---

# Introduction

![Status: In progress](https://img.shields.io/badge/status-In%20progress-yellow)

Testers sent a list of changes and bugs after using the alpha. This plan records what each item needs, what causes the
bugs (found by reading the code before changing it), the order the work lands in, and how each part is verified. The
work happens on branch `tester-feedback-round-1`, cut from `xp12-environment-rendering` (which contains all of `dev`
plus the unmerged XP12 renderer work the camera and overlay changes touch). Its pull request targets `dev`.

The tester list, as received:

1. Orthographic view doesn't work.
2. In orthographic view, users want a top-down view as well.
3. Open a secondary viewport and pop it out to a separate monitor.
4. Projects shouldn't have to be tied to an airport.
5. On first launch, let the user pick a default keymap: Blender (Alt or MMB orbits around the camera pivot, Shift+Alt /
   Shift+MMB pans, the wheel zooms), Autodesk 3ds Max or Autodesk Maya.
6. Numpad 5 toggles orthographic, numpad 7 is top-down, numpad period frames the selection. Rotation is inverted
   compared to what the editor does now.
7. Library panel: libraries collapse, and everything but what is searched or filtered is hidden.
8. A filter button that opens a drop-down for the search, to clean up the UI with a large expanded asset list.
9. Save the filters in the project.
10. Selecting and editing a large polygon freezes for short moments and the frame rate stutters.
11. The X-Plane `S_OFFSET` property doesn't appear to work properly.
12. Facade objects are rotated 90 degrees. (X-Plane effectively negates coordinates and rotations about the Y axis.)
13. Cached preview renders of library assets in the library and asset panels.

## 1. Requirements & Constraints

### Baseline facts (branch point `fa313ff7`)

- **BAS-001** (items 1, 2): `EditorCamera::SetProjection` (`source/Editor/core/editor_camera.cpp`) only sets
  `Camera::m_ProjectionType`. The renderer builds its camera from the primary camera entity's `CameraComponent` in
  `BuildSceneCameraView` (`source/SceneryEditorX/renderer/renderer.cpp:335`), which always builds a reverse-Z
  perspective projection; `Camera::ComputeMatrices` overwrites `m_ProjectionType` from the component every frame. The
  editor tools use `ViewportView::Build` (`source/Editor/core/viewport_view.cpp`) and `SceneViewport::PickGround`, both
  perspective-only. Orthographic therefore never reaches the screen.
- **BAS-002** (items 1, 2): Camera bases are `right = normalize(cross(forward, worldUp))` in the renderer, the tools and
  picking, which degenerates looking straight down; the orbit pitch is clamped to +-0.49 pi.
- **BAS-003** (items 5, 6): Camera input is read on the render thread by `Camera::ProcessInput`
  (`source/SceneryEditorX/scene/camera.cpp:160`, called from `Scene::Tick`) with fixed bindings: Alt+LMB orbit, MMB pan,
  RMB look, WASD/QE while RMB is held, wheel dolly. Dragging right orbits the camera to its right (the scene moves
  against the mouse) and RMB look turns left for a rightward mouse move and up for a downward one, the opposite of
  Blender, 3ds Max and Maya. `EditorSettings` (`source/Editor/settings/editor_settings.*`) is unused; the
  `Controls & Shortcuts` window (`source/Editor/core/windows/keybind_window.cpp`) has empty tabs. There is no first-run
  flow. Panel state is saved as JSON in `%APPDATA%/SceneryEditorX/EditorLayout.json` (`PanelManager::Serialize`).
- **BAS-004** (item 3): `Renderer` is a single-view static renderer: one render-target array (`s_RenderTargets`), one
  render / output resolution, one camera (`BuildSceneCameraView`), CPU draw lists culled with that camera, picking,
  exposure and frame capture tied to it. Dear ImGui multi-viewports are enabled (`ui_layer.cpp`), so panels can already
  float as OS windows on other monitors; a second camera view needs per-view render targets.
- **BAS-005** (item 4): File > New Airport Project (Ctrl+N) is the only guided way to start a project
  (`source/Editor/core/project_dialogs.cpp`). File > New Empty Project only asks for a file name: the project has no
  reference point, so `EditorProjects::LoadIntoScene` centers it at 0, 0 (`GeoProjection(0.0, 0.0)`) until something is
  placed. The model already supports a project datum (`SceneryProject::datum`, `GetProjectCenter`).
- **BAS-006** (items 7-9): The X-PLANE LIBRARY Assets tab (`source/Editor/panels/xplane_library_panel.cpp`) is one flat
  table of every exported asset with a search box, a type combo, a library combo and two checkboxes on one row. Filters
  are not saved anywhere.
- **BAS-007** (item 11): `ParseLineAsset` keeps every `S_OFFSET` row, but `OverlayBuilder::BuildLine` and the apt.dat
  painted lines (`source/SceneryEditorX/xplane/xp_overlay.cpp:1292` and `:2240`) draw only `segments[0]`, centered on the
  path. X-Plane and WorldEditor draw every row as its own strip, shifted so the path runs through the row's center pixel
  `sm` (`WED_PreviewLayer.cpp` `draw_line_preview`: half width `(s2 - s1) / 2 * scale_s`, offset
  `((s1 + s2) / 2 - sm) * scale_s`), and draw `START_CAP` / `END_CAP`. Multi-row lines (a yellow line over a black
  border) lose their border and off-center lines sit in the wrong place.
- **BAS-008** (item 12): Attached facade objects (`ATTACH_DRAPED` / `ATTACH_GRADED`) get
  `heading = DirectionToHeading(outward) + r` (`xp_overlay.cpp:1798`). The wall meshes map template +X to outward and
  -Z to along the wall; WorldEditor places attached OBJs in that same segment frame (`WED_FacadePreview.cpp`: the segment
  is rotated so -Z runs along the wall, then `draw_obj_at_xyz` turns the OBJ by `r` about -Y), so the OBJ's -Z must run
  along the wall: `heading = DirectionToHeading(along) + r`. The current heading is 90 degrees off. `ROOF_OBJ` /
  `ROOF_OBJ_HEADING` objects are parsed (`FacadeAsset::RoofObject`) but never placed.
- **BAS-009** (item 10): Dragging nodes edits the project item every frame; `EditorProjects::UpdateScenery` rebuilds
  the project's DSF and overlay whenever `GetDsfFingerprint` changes. To be measured before changing anything.
- **BAS-010** (item 13): No thumbnail or offscreen preview exists. The library panel parses the selected OBJ on the CPU
  for its statistics only; the ASSETS shelf draws a font glyph per file type.

### Requirements

- **REQ-001**: Orthographic projection works in the renderer, the editor tools (picking, gizmos, node and facade tools,
  drops) and the overlays. Switching projection keeps the framing at the orbit pivot (orthographic height
  `2 * distance * tan(fovY / 2)`), as Blender does. Distance-based LOD keeps using the orbit eye, so objects don't
  vanish in orthographic.
- **REQ-002**: View presets: top, bottom, front, back, left, right, available in perspective and orthographic. Numpad 7
  is top (Ctrl+7 bottom), 1 front (Ctrl+1 back), 3 right (Ctrl+3 left), 5 toggles orthographic, period frames the
  selection, 2 / 4 / 6 / 8 orbit in 15 degree steps, 9 looks from the opposite side, Home frames everything. Numpad keys
  work in every keymap.
- **REQ-003**: Navigation keymaps Blender, 3ds Max and Maya, chosen on first launch and changeable later
  (Edit > Preferences). Orbit follows the DCC convention: the scene turns with the mouse. RMB look turns toward the
  mouse. Invert options for horizontal and vertical orbit.
- **REQ-004**: A second viewport (Window > New Viewport) with its own camera, projection and view presets that can be
  docked or dragged out onto another monitor. The main viewport keeps its current performance when no second viewport
  is open.
- **REQ-005**: New Project offers Scenery (no airport), Airport and Library projects. A scenery project has a reference
  point that centers the scene; airports can be added to any project later.
- **REQ-006**: The library browser groups assets by library in collapsible sections, hides assets and libraries that
  don't match the search and filters, and moves the type / library / flag filters into a Filter drop-down.
- **REQ-007**: Named library filters are saved in the project (`.edX` and `.sex`), with undo, and loaded with it.
- **REQ-008**: Editing a large polygon (thousands of nodes) stays interactive: no frame over 50 ms while dragging nodes
  on the Debug build's reference machine, measured with `--perf-log`.
- **REQ-009**: Lines draw every `S_OFFSET` row in layer order with the path at each row's center pixel, plus start and
  end caps.
- **REQ-010**: Attached facade objects face the way X-Plane places them; roof objects are placed.
- **REQ-011**: Library assets and project art show a preview image rendered from the asset, cached on disk under
  `cache:/thumbnails` (rebuildable) and refreshed when the asset file changes.
- **CON-001**: Project format changes append fields at the end of a `Visit()` (binary chunks are size-prefixed) and keep
  `.edX` output deterministic (`docs` and `project_archive.h`).
- **CON-002**: Format and geometry code stays standard-library only and unit tested (AGENTS.md test suites).
- **CON-003**: Debug builds must keep exiting cleanly (no leaked GPU objects at shutdown).

## 2. Implementation Steps

### Phase 1 - X-Plane rendering fixes (items 11, 12)

- GOAL-001: Lines and facade objects draw the way X-Plane draws them.

| Task | Description | Completed | Date |
|------|-------------|-----------|------|
| TASK-001 | `BuildLine` and the apt.dat painted lines draw every `S_OFFSET` row (sorted by layer, stable) as an offset ribbon from `(sm - s1) * scaleS` right of travel to `(s2 - sm) * scaleS` left, with MIRROR and START_CAP / END_CAP like WorldEditor's `draw_line_preview`. | ✅ | 2026-10-21 |
| TASK-002 | Attached facade objects use the wall direction: `heading = DirectionToHeading(along) + r`. | ✅ | 2026-10-21 |
| TASK-003 | Roof objects (`ROOF_OBJ`, `ROOF_OBJ_HEADING`) are placed on the roof in the facade's front-wall frame. Deferred: the spec gives S / T as pixel positions while WorldEditor reads meters and tiles the object across the roof, and no tester reported them. | Deferred | |
| TASK-004 | `OverlayTests.cpp`: a two-row line, an off-center row, caps; an attached object's heading on walls facing each direction. | ✅ | 2026-10-21 |

### Phase 2 - Viewport camera and navigation (items 1, 2, 5, 6)

- GOAL-002: One editor-side camera controller drives every viewport, with orthographic, view presets and DCC keymaps.

| Task | Description | Completed | Date |
|------|-------------|-----------|------|
| TASK-005 | `CameraComponent` gains an orthographic height; `BuildSceneCameraView` builds a reverse-Z orthographic projection (near plane behind the eye so nothing in front of the orbit pivot is clipped) and takes the right vector from yaw. The shadow cascades, GPU cull and shaders are checked for perspective-only maths.| ✅ | 2026-10-21 |
| TASK-006 | `Editor/core/viewport_camera.*`: camera state (pivot, distance, yaw, pitch, projection, field of view), keymaps (Blender, 3ds Max, Maya), view presets with a short eased transition, numpad keys, frame selection / frame all. Writes the primary camera entity; the engine's `Camera::ProcessInput` stands down while the editor drives the camera. `--lock-camera` still freezes it.| ✅ | 2026-10-21 |
| TASK-007 | `ViewportView`, ImGuizmo (orthographic mode), `PickGround` and drops support orthographic.| ✅ | 2026-10-21 |
| TASK-008 | Editor preferences (`user:/editor_preferences.json`): keymap, invert options. First-launch welcome modal to pick the keymap; Edit > Preferences; the Controls & Shortcuts window lists the active bindings. Viewport toolbar: projection toggle and a view-preset menu.| ✅ | 2026-10-21 |

### Phase 3 - Library panel (items 7, 8, 9)

- GOAL-003: A compact, grouped library browser whose filters can be saved in the project.

| Task | Description | Completed | Date |
|------|-------------|-----------|------|
| TASK-009 | Assets grouped by library in collapsible sections (collapse / expand all); libraries and assets that don't match the search or filters are hidden; match counts per library. | | |
| TASK-010 | A Filter button (funnel) opening a drop-down: asset types (multi-select), libraries (multi-select with search), private / deprecated, missing files; a short summary of the active filters next to the search box; clear all. | | |
| TASK-011 | `SceneryProject::libraryFilters` (name, search, types, libraries by name, flags) in `.edX` / `.sex` (appended to `Visit`), edited through `ProjectDocument` (undo, dirty), ignored by WED import / export. The drop-down saves, applies, renames and deletes them. ProjectFormatTests round-trip them. | | |

### Phase 4 - Projects without an airport (item 4)

- GOAL-004: Any kind of scenery can be a project.

| Task | Description | Completed | Date |
|------|-------------|-----------|------|
| TASK-012 | `NewSceneryProject` / `ValidateNewSceneryProject` / `CreateSceneryProject` in `project/scenery_pack.*` (name, location, author, reference point and elevation as the datum, optional library), with SceneryPackTests. | | |
| TASK-013 | File > New Project (Ctrl+N): Scenery, Airport or Library, sharing the location / author fields; New Empty Project folds into it. | | |
| TASK-014 | PROJECT panel: add a new airport to an open project, remove an airport; a project without an airport opens centered on its datum. | | |

### Phase 5 - Large polygon editing (item 10)

- GOAL-005: Editing a polygon with thousands of nodes stays smooth.

| Task | Description | Completed | Date |
|------|-------------|-----------|------|
| TASK-015 | A reproducible case: a generated project with a ~5000 node draped polygon, a forest and a facade; `--tools-selftest` steps that select and drag its nodes; `--perf-log` scopes around the node tool, project sync and scenery rebuild. | | |
| TASK-016 | Fix what the measurement shows (expected: rebuild only the edited item's overlay meshes while dragging, defer full rebuilds to the end of the drag, cache triangulation and selection outlines). | | |

### Phase 6 - Asset previews (item 13)

- GOAL-006: Every library asset and project art file shows a preview image.

| Task | Description | Completed | Date |
|------|-------------|-----------|------|
| TASK-017 | `xplane/xp_thumbnail.*` (standard library): a small software rasterizer (depth buffer, perspective-correct texturing, alpha cut-out, sun + sky shading, 2x2 supersampling) that frames an asset: OBJ8 (LOD 0, at rest), facades (a default footprint through `OverlayBuilder`), polygons, lines, strings, forests, textures. CPU PNG / DDS decoding. Unit tests. | | |
| TASK-018 | Thumbnail cache: `cache:/thumbnails/<hash of path, size, time, version>.png`, background workers, GPU upload on the main thread under a time budget, LRU of GPU images. | | |
| TASK-019 | X-PLANE LIBRARY: preview column and a grid view, large preview in the details pane. ASSETS shelf: previews on X-Plane art and texture tiles. | | |

### Phase 7 - Second viewport (item 3)

- GOAL-007: A second camera view that can live on another monitor.

| Task | Description | Completed | Date |
|------|-------------|-----------|------|
| TASK-020 | Renderer views: per-view render targets, resolutions, camera, previous-frame matrices and CPU draw lists; the main view is view 0 and renders as before; extra views render after it in the same frame. Exposure comes from view 0; picking and captures stay on view 0 at first; selection outlines draw in every view. | | |
| TASK-021 | Window > New Viewport: a VIEWPORT 2 panel with its own camera controller, toolbar and view presets, dockable or floating on another monitor; its camera is remembered; closing it frees its targets. | | |

## 3. Alternatives

- **ALT-001**: Render the second viewport with a reduced unlit pass. Rejected: a parallel draw path for GPU-scene,
  CPU and drape draws to maintain, and it would not look like X-Plane 12.
- **ALT-002**: Render thumbnails on the GPU through the renderer. Deferred: it needs the multi-view work and an isolated
  scene; a CPU rasterizer runs on worker threads, is unit testable and caches the same way.
- **ALT-003**: Keep camera input in the engine's `Camera`. Rejected: the editor needs per-viewport cameras and keymaps,
  and the render-thread input path can't tell which viewport the mouse is over.

## 4. Dependencies

- **DEP-001**: Dear ImGui docking branch with multi-viewports (vendored, enabled).
- **DEP-002**: ImGuizmo (vendored) orthographic mode.
- **DEP-003**: X-Plane 12 install for the library sweeps and manual checks (`x-plane_install_12.txt`).

## 5. Files

- **FILE-001**: `source/SceneryEditorX/xplane/xp_overlay.*`, `xp_assets.*` (Phase 1).
- **FILE-002**: `source/SceneryEditorX/renderer/renderer*.cpp`, `scene/camera.*`, `scene/components/component_sets.h` (Phases 2, 7).
- **FILE-003**: `source/Editor/core/viewport_camera.*` (new), `editor_camera.*`, `viewport_view.*`, `editor_tools.*`, `panels/scene_viewport.*`, `core/windows/keybind_window.*`, `settings/editor_preferences.*` (new) (Phase 2).
- **FILE-004**: `source/Editor/panels/xplane_library_panel.*`, `source/SceneryEditorX/project/scenery_project.h`, `project_archive.h`, `project_document.*` (Phase 3).
- **FILE-005**: `source/SceneryEditorX/project/scenery_pack.*`, `source/Editor/core/project_dialogs.cpp`, `editor_project.cpp`, `panels/project_panel.cpp` (Phase 4).
- **FILE-006**: `source/Editor/core/node_tool.cpp`, `editor_project.cpp`, `source/SceneryEditorX/project/project_dsf.*` (Phase 5).
- **FILE-007**: `source/SceneryEditorX/xplane/xp_thumbnail.*` (new), `source/Editor/core/asset_thumbnails.*` (new), `panels/xplane_library_panel.cpp`, `panels/asset_shelf_panel.cpp` (Phase 6).

## 6. Testing

- **TEST-001**: XPlaneFormatTests / ProjectFormatTests (MSYS2 g++ and AGENTS.md recipes): new line, facade and project cases.
- **TEST-002**: `--tools-selftest` on the 5S4 project: orthographic picking and node edits, view presets, the library filter round trip, large-polygon drags with timings.
- **TEST-003**: RenderDoc captures of: orthographic top view of 5S4, a two-row painted line, a facade with attached objects, the second viewport.
- **TEST-004**: Clean exit (exit code 0, no leaked objects) with the second viewport open and with thumbnails cached.

## 7. Risks & Assumptions

- **RISK-001**: Per-view render targets double the GPU memory while a second viewport is open.
- **RISK-002**: Shaders may assume a perspective projection when they linearize depth; each is checked.
- **ASSUMPTION-001**: "Facade objects" means the OBJs a facade attaches to its walls and roofs (doors, poles, roof
  units); the wall meshes already line up with the footprint.
- **ASSUMPTION-002**: The `S_OFFSET` report is about `.lin` painted lines (the only X-Plane format with that row).

## 8. Related Specifications / Further Reading

- [X-Plane .lin / .fac file formats](https://developer.x-plane.com/docs/specs/)
- `plan/feature-xp12-environment-rendering-1.md` (renderer phases this branch builds on)

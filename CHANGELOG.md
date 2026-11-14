# Changelog

Notable changes to Scenery Editor X, newest first. It starts after 0.2.63.564; earlier changes are in the pull request
descriptions and the git history. Versions are `SEDX_VERSION_STRING` in `source/resource.h`, which goes up with every
change set (so running builds can be told apart), not only at a release: entries in one section can name different
versions.

## Unreleased

### Added

- **Interface languages** (0.2.90.591). The editor can show its menus, panels and messages in other languages:
  **Edit › Preferences › Interface** (or **Help › Language...**) switches at once, without a restart, and
  `ui.language = "system"` in `settings.cfg` (the new default) follows the operating system's languages; `--language
  <tag>` picks one for a run. German and Japanese ship as machine-drafted catalogs for native speakers to review;
  translations are made on Hosted Weblate (`docs/localization-translator-guide.md`). Chinese, Japanese and Korean use
  the system's regional font, merged into the UI fonts when chosen (the first one that has the language's characters,
  checked in its character map, 0.2.91.592). Translated widgets keep their English IDs, so the
  docking layout and window state stay as they are. `--language qps-ploc` (the pseudo-locale) shows which text isn't
  translatable yet, `--i18n-log-missing` lists the strings a language lacks, and a translator's copy in the settings
  folder's `locale` folder replaces the bundled one (**Help › Reload Translations**, which keeps the version in use when
  the file can't be read, 0.2.92.593). For developers: the `Tr`, `TrL`,
  `TrF`, `TrN`, `TrNF`, `Ctx` and `TrNoop` functions and the `Text` parameter type (`source/SceneryEditorX/i18n`,
  standard library only, `I18nTests`), `tools/i18n/sedx_i18n.py` (extract, update, check, lint), and the
  **Localization** workflow, which refreshes `resources/locale` on every pull request into `dev`
  (`docs/localization-system-documentation.md`). The `EditorUI` helpers translate their text, which covers most panel
  labels and tooltips; the remaining direct ImGui literals are listed in `tools/i18n/lint_baseline.txt` and converted
  area by area.

- Windows setup: **Settings › Apps › Installed apps › Scenery Editor X › Modify** (Control Panel's **Change**) opens
  setup's Update / Repair / Uninstall page, so the editor can be repaired without the setup file it was installed from.
  Setup keeps a copy of itself in the install folder for that (`setup\SceneryEditorX-Setup.exe`); Settings offered only
  Uninstall before. A copy whose `SceneryEditorX.exe` was deleted is offered Repair too, where setup took it for a
  first install. (Setup only: the editor itself is unchanged, so the version stays.)
- `--camera-fly <m/s>`: the camera flies 1 km out of its startup point and circles it at that speed, for measuring what
  camera movement costs. `--perf-log` also reports what the GPU deletion queue holds ("deletion queue entries / MB").
  (#69, 0.2.66.567; flying out instead of jumping, and counters kept as running totals, 0.2.67.568)

### Changed

- <kbd>F</kbd> (Frame Selection) frames whatever is selected, so the camera can find anything in a large scene: a model
  or X-Plane object by what it draws (it was framed as a 5 m point, so big buildings filled the screen), an apt.dat row
  with its lights, just the picked nodes when working on nodes (the Nodes tool, or Move / Rotate / Scale after picking
  them), a project item (or where it is when it draws nothing), a PROJECT folder or OUTLINER group with everything in it,
  and the airport itself. The pack's DSF art, locked items and groups frame too (only what the move gizmo could edit did,
  so read-only things and groups didn't frame at all). The camera keeps its direction and backs off until every corner of
  the selection is in view (`scene/view_framing.*`, `ViewFramingTests`): long, flat things such as runways frame tighter
  than their bounding sphere, and wide lenses no longer frame too close. <kbd>F</kbd> works in every keymap (3ds Max's
  front view stays on <kbd>Numpad 1</kbd>; <kbd>Z</kbd> still frames there), from any panel that has the keyboard (the
  OUTLINER, PROJECT, PROPERTIES; it needed the mouse over the viewport) and over VIEWPORT 2, which it frames in its own
  view. Clicking a folder or another item that draws nothing in the PROJECT hierarchy selects it (Ctrl toggles, Shift
  adds), a selected folder outlines its items in the viewport, and a double-click frames it as in the OUTLINER; Del never
  removes an airport selected there (its menu's Remove from project does). Frame All without an airport reads the meshes'
  real bounds (it read bounding boxes nothing kept up to date). The tools self-test frames each kind of target and presses
  <kbd>F</kbd> with the OUTLINER focused. (0.2.86.587; a Ctrl or Shift double-click in the OUTLINER or PROJECT hierarchy
  frames the item instead of selecting it twice, which toggled it back out; one framing measures at most 100,000
  entities and 200,000 instances however many targets are selected, breadth first so a cut-short walk still spans the
  selection, 0.2.87.588; F does nothing while a modal or a menu is open, and Frame All without an airport keeps to the
  same budget, 0.2.88.589)

### Removed

- `Ref(const std::shared_ptr<T>&)` in `utils/pointers.h`, removed in 0.2.52.553 (merged to `master` with PR #65). It shared
  the object with the `shared_ptr` without the `shared_ptr` knowing: unless the `shared_ptr` came from
  `Ref::ToSharedPtr()`, each side deleted the object on its own last release. Keep the object in a `Ref`
  (`CreateRef<T>(...)`) and call `ToSharedPtr()` where an API wants a `std::shared_ptr`. (#66, 0.2.52.553)

### Fixed

- The viewport camera (`MainCamera`) is no longer listed in the OUTLINER and can't be deleted: deleting it there, or
  with Del while it was selected, left a camera nothing could move until the editor restarted. `EditorScene::Destroy`
  refuses it and returns whether it destroyed anything. (#80, 0.2.84.585; Del with only the camera selected deletes
  nothing and keeps the selection, 0.2.85.586)
- Changing a dataref in a selected object's PROPERTIES (X-PLANE OBJECT › Animation) moves that object only. The table
  edited the control every object shares, so testing one jetway moved every jetway at the airport, and it did the same
  for any object with datarefs. Each placement now keeps its own controls for the datarefs changed there
  (`XPlaneObjectComponent::datarefOverrides`, laid over the shared values when its inputs are read), as X-Plane gives each
  instance of an object its own values; a button on the row hands it back to the shared value. The Simulation tab still
  sets the values every object shares. The tools self-test checks a placement and a twin of it. (#75, 0.2.83.584)
- Draped polygons with `TEXTURE_TILE` (the tiling shader) keep the size `SCALE` gives their texture: before, `SCALE` was
  taken as one tile, so a 4 x 4 atlas drew 4 times too large (and a 4 x 2 one under `SCALE 10 20` as a 40 m square).
  `TEXTURE_TILE` now works like `RUNWAY_TILE`: each tile-sized cell shows the tile its mapping texture picks, or a random
  one without a map. X-Plane 12's own asphalt taxiway art (`asphalt_*.pol`, `TEXTURE_TILE 4 4`, `SCALE 18 18`) uses it,
  so default airports' asphalt is affected too. (#82, 0.2.82.583)
- **Fly** navigation flies with <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> and <kbd>Q</kbd> / <kbd>E</kbd> without
  holding the right mouse button (while the mouse is over the viewport or it has the keyboard; not with Ctrl, Alt or Super
  held), and the keymap's orbit, pan and zoom drags no longer move the camera in it: the right mouse button turns it and
  the wheel moves it forward and back. In Fly navigation those keys pick no tools, set no node types and (Maya keymap)
  don't frame everything. The tools self-test drives it with scripted keys (`UI::SetScriptedKeys`). (#79, 0.2.74.575;
  those keys stay tool keys whenever the camera doesn't fly with them, such as with its input locked, 0.2.78.579; an orbit,
  pan or zoom drag held while switching to Fly ends, 0.2.80.581)
- The self-tests' scripted mouse also reports the main window as the one under the cursor: with the real cursor over
  another program, nothing was hovered and the gizmo checks of `--tools-selftest` / `--gizmo-selftest` failed.
  (0.2.74.575)
- Saving a WorldEditor project (`earth.wed.xml`) never writes a number with an exponent. A bezier handle moved in the
  editor sits a few micro-degrees off its node, and its offset came out as `3.702805202010495e-06`, which WorldEditor reads
  as 3.7 degrees (it stops at the `e`). Such values are now written in full (`0.000003702805202010495`), still exact.
  (#81, 0.2.76.577)
- A `WeakRef` to an object destroyed without its last `Ref` release (deleted by hand, or a stack or member object a `Ref`
  pointed to) now expires. Before, the object's control block stayed registered under its old address, and the next
  object there inherited it: that object's new WeakRefs were expired at once, or the old ones locked it. Both cases are
  misuse; `~RefCounted()` now cleans up after them. (#67, 0.2.64.565)
- A GPU buffer or image released after the device shut down (a static one destroyed with the program) is leaked with one
  warning instead of being freed through an allocator that is gone or a device that is: before, that dereferenced a
  null allocator. (#70, 0.2.66.567)

### Changed

- `MemoryAllocator::Init` takes `const Device&` instead of `const Ref<Device>&`. `Device`'s constructor called it with
  `this`, which made a temporary `Ref` that was only safe because the `QueueManager` already held one. (#68, 0.2.64.565)
- `Device`'s constructor holds a reference of its own while it runs, so a `Ref` made from `this` in there can't delete
  the device, whoever keeps theirs. (#68, 0.2.65.566)
- Documented on `Ref(T*)` and `WeakRef`: a temporary `Ref` made from `this` in a constructor or destructor deletes the
  object unless another `Ref` to it is kept, and an object whose reference count is 0 counts as destroyed, like with
  `std::weak_ptr`. (#68)

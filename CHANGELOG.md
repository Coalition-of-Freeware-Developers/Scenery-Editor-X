# Changelog

Notable changes to Scenery Editor X, newest first. It starts after 0.2.63.564; earlier changes are in the pull request
descriptions and the git history. Versions are `SEDX_VERSION_STRING` in `source/resource.h`, which goes up with every
change set (so running builds can be told apart), not only at a release: entries in one section can name different
versions.

## Unreleased

### Added

- `--camera-fly <m/s>`: the camera flies 1 km out of its startup point and circles it at that speed, for measuring what
  camera movement costs. `--perf-log` also reports what the GPU deletion queue holds ("deletion queue entries / MB").
  (#69, 0.2.66.567; flying out instead of jumping, and counters kept as running totals, 0.2.67.568)

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

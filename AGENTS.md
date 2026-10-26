# AGENTS.md

## Cursor Cloud specific instructions

> These notes target the **`dev`** branch (active development; `master`/`main` is older).
> Development currently happens on `dev` and feature branches off it.

### What this repo is (and what runs in the cloud VM)

Scenery Editor X (SEDX) is a **native C++20 / Vulkan desktop GUI application** for editing
X-Plane 12 scenery (see `README.md`). It is *not* a web/service app — there is no database,
backend, or container to start. "Running the product" means building with CMake + vcpkg and
launching the `SceneryEditorX` GUI executable, which requires the Vulkan SDK plus a GPU and
a display.

On `dev` the build system is genuinely **cross-platform** (Windows/macOS/Linux) via
`CMakePresets.json` + per-platform toolchains in `cmake/toolchains/`. However, the **full GUI
app still cannot be run on the headless cloud VM** (no GPU/display), and a full configure pulls
a heavy stack: the bundled `dependency/vcpkg` toolchain installs `curl`/`msdfgen`/`spdlog`/`stb`/
`portable-file-dialogs`; it also requires the Vulkan SDK (including the `slang` library via
`FIND_LIBRARY(... slang REQUIRED)`), `Freetype`, bundled `SDL`/`KTX`, and FetchContent clones of
`xMath`, `edX`, and `X-PlaneSceneryLibrary`. Don't expect a clean full build on this VM.

### What you CAN build and run on the headless Linux VM

The dependency-light, cross-platform Catch2 unit suites that only need `source/` headers:

- **`ConversionTests`** (`source/Tests/conversion_tests`) — unit-conversion utilities
  (`SceneryEditorX/utils/conversions/...`). Header-only, fully green.
- **`RefTests`** (`source/Tests/pointer_tests`) — the `Ref`/`WeakRef` smart-pointer system
  (`SceneryEditorX/utils/pointers.h`). Builds and runs.
- **`XPlaneFormatTests`** (`source/Tests/xplane_tests`) — OBJ8 parsing/animation evaluation, the
  `library.txt` / `scenery_packs.ini` parsers + library index, the `lights.txt` light catalog, and the
  apt.dat 12.00 parser/writer, airport geometry, airport index, Scenery Gateway JSON records, the DSF
  reader and writer (`DsfWriterTests.cpp`: 16-bit pool quantization, MD5 footer, airport filters, per-tile
  split, road networks as 32-bit pools of junction-to-junction segments), the .pol/.lin/.str/.for/.fac/.agp
  and .net (road types, virtual types, draped strips) asset parsers and the overlay geometry builder (roads
  drawn as their road type's strips) and facades (`FacadeTests.cpp`: `xp_facade.*` WALL_RULE / SEGMENT_CURVED /
  roof directives, X-Plane's floor and wall-rule choice, WorldEditor's spelling choice, natural wall lengths,
  curved walls bent along their bezier), the .fac writer and facade authoring (`FacadeWriterTests.cpp`:
  `xp_facade_writer.*` parse -> write -> parse gives the same asset for type 1 / type 2 facades, the install's
  default scenery facades included when an X-Plane 12 install is listed in x-plane_install_12.txt (tag
  `[library-sweep]`, a few seconds; exclude with `"~[library-sweep]"`); `xp_facade_authoring.*` type 1 facades from a
  texture (panel edges, roofs), type 2 sidewalks / curbs generated from a cross-section), and finding X-Plane 11 / 12
  installs (`InstallFinderTests.cpp`: `xp_install_finder.*` install lists, Steam libraryfolders.vdf / appmanifests,
  uninstall entries and usual folders, deduplicated and ranked complete / newest version / most recently run first),
  and the split-sum BRDF table (`BrdfLutTests.cpp`: `environment/brdf_lut.*`, GGX with height-correlated Smith; with an
  X-Plane 12 install listed it is compared against `Resources/brdf_lookup`, tag `[xplane-install]`, about a second), and
  asset thumbnails (`ThumbnailTests.cpp`: `xp_thumbnail.*` BC1-BC5 DDS decoding and mip choice, the software rasterizer's
  framing / depth / draped painter's order / cutout, WorldEditor-style previews per kind; with an X-Plane 12 install listed it
  draws default library assets, tag `[xplane-install]`, and `SEDX_THUMBNAIL_DUMP=<folder>` writes them as PNGs), and the
  base terrain (`TerrainTests.cpp`: `xp_7z.*` LZMA / LZMA2 against liblzma fixtures and one-file 7-zip archives, DSF terrain
  patches and rasters (`xp_dsf.*` DsfReadOptions, an area keeping only its triangles and the overlay placements reaching
  into it, overlays read only from base meshes, sim/exclude_* zones as boxes and X-Plane 12 polygons), `.ter` terrain types
  (`xp_terrain.*`; every default `.ter` parses with its textures, tag `[library-sweep]`), library resolution by REGION bitmap
  and season (`xp_library.*` ResolveProvider), the terrain meshes in the local frame and the surface drapes are cut onto
  (`xp_terrain_mesh.*`: heights, rays, exact draping across a crease, clipped at the terrain's edge; water meshes cut into
  64 m cells without T-junctions for tessellation, `CutTerrainAlongGrid`, and the floor's depth from X-Plane 12's depth
  ratio and the bathymetry, `WaterFloorDepth`, tag `[water]`); with an X-Plane 12
  install listed it reads KSEA's 7-zip Global Scenery tile and its scenery within a mile, tag `[xplane-install]`, about two
  seconds), and the terrain's own scenery in `OverlayTests.cpp` (`.bch` beaches parsed and laid along shores with the
  water on the left, forests planted only in the area and where the caller allows), and autogen (`AutogenTests.cpp`:
  `xp_autogen.*` .agp / .ags / .agb parsing, points facing their heading, strings lining their sides with corners,
  atomic spellings, stop points and holes, blocks fitting their grid with slop and GROUND_PT_FORMULA, object detail levels,
  the overlay builder's ground tiles / objects / facades / fill trees; every shipped autogen parses and lays out, tag
  `[library-sweep]`)
  (`SceneryEditorX/xplane/xp_obj8.*`, `xp_library.*`, `xp_lights.*`, `xp_apt.*`, `xp_apt_geometry.*`,
  `xp_airport_index.*`, `xp_gateway_types.*`, `xp_dsf.*`, `xp_7z.*`, `xp_assets.*`, `xp_overlay.*`, `xp_terrain.*`,
  `xp_terrain_mesh.*`, plus `filesystem/vfs.*`,
  which every reader goes through for case-insensitive lookups and atomic writes). Standard library
  plus the header-only `dependency/json` submodule, fully green.
- **`ProjectFormatTests`** (`source/Tests/project_tests`) — the project file formats: the `.edX` JSON
  reader/writer (determinism, git-friendly layout, the documented legacy layout), the `.sex` binary format
  (checksums, damage / truncation / CRLF detection, fuzzing, the encode cache), the XML DOM, WorldEditor
  `earth.wed.xml` import/export (`docs/XCoder/earth.wed.xml`), atomic writes, the project cache (snapshots,
  recovery, backups), `ProjectDocument` open/edit/save/close, the project -> DSF conversion the editor draws
  through the overlay builder, road networks (road node and road items through .edX / .sex / WED_RoadNode +
  WED_RoadEdge, junctions kept on their nodes, DSF segments with shared junction ids), and the viewport node
  editor's bezier core (`SplineEditTests.cpp`:
  `project/spline_edit.*` round trips apt.dat chains and project rings bit for bit, de Casteljau inserts, handles,
  deletes, smoothing, transforms, node types (corner / smooth / symmetric / free), bending a segment through a point,
  uniform cubic B-spline control polygons stored as bezier nodes and recovered from them, extending / closing /
  opening lines, lengths and tightest bend radius), facade footprints (`FacadeEditTests.cpp`: `project/facade_edit.*`
  rectangles / circles counter-clockwise from their front, curved walls cut into 0.5 - 0.8 m pieces (the 3D sidewalk
  recipe), squared corners, front wall, offsets, wall types moved by name), the cross-platform VFS (`VfsTests.cpp`: `filesystem/vfs.*` mounts, virtual
  paths, portable names, case-insensitive lookups, atomic writes, platform folders; the twin-spelling case only
  runs on a case-sensitive disk) and scenery packs (`SceneryPackTests.cpp`: `project/scenery_pack.*` new airport
  projects, coordinate parsing, export to apt.dat + DSF tiles + copied art) (`SceneryEditorX/project/*` plus
  `filesystem/vfs.*`, `xplane/xp_apt.*`, `xp_library.*`, `xp_apt_geometry.*`, `xp_airport_document.*`,
  `xp_airport_index.*`, `xp_dsf.*`, `xp_assets.*`, `xp_overlay.*`). Standard library plus `dependency/json`, fully green. Its
  `[performance]` case times a 22k-item project; exclude it with `"~[performance]"` for quick runs.

- **`EnvironmentTests`** (`source/Tests/environment_tests`) — the XP12 environment model behind the viewport's
  X-Plane 12 emulation (`plan/feature-xp12-environment-rendering-1.md`): parity scenario files
  (`ParityScenarioTests.cpp`: `environment/parity_scenario.*`, the format `tools/xp12_reference_probe` and
  `--env-scenario` share, `scenery_date` included (X-Plane keeps a tile's seasonal art from the date it loaded on;
  `World::SetSceneryDate`), plus the shipped `tools/xp12_reference_probe/scenarios/xp12_parity.json` and `xp12_airports.json`
  when run from the repo root) and the environment model (`EnvironmentTests.cpp`: NOAA sun against an independent almanac formula, Meeus moon
  examples 47.a / 48.a, the 16 runway-friction states and the wetting / drying integrator, seasons from date and latitude
  or DSF ranges, sky-condition weights, world-frame directions, the `World` object, the atmosphere's medium (X-Plane
  12.4.4's dumped values, visibility to the surface haze layer at 1% contrast, per-channel Mie, sky-condition multipliers,
  `environment/atmosphere_params.*`), the interim CPU sky light
  model (transmittance, sun / sky illuminance, energy bounds, `environment/sky_light.*`), plus the renderer's X-Plane
  quality levels (`renderer/renderer_quality.*`, from `settings.txt`), feature switches (`renderer/renderer_features.*`),
  exposure (`renderer/exposure.*`: EV100 camera mapping to middle grey, X-Plane 12.4.4's sun-elevation target and gains,
  histogram metering, adaptation) and when the environment
  probe is captured again (`renderer/env_probes.*`), and the sun-shadow cascades (`renderer/csm.*`: X-Plane 12.4.4's practical
  split scheme, slice bounding spheres, texel snapping that keeps edges still while the camera pans, caster culling), and
  the CPU half of the CACAO ambient occlusion (`renderer/cacao.*`, tag `[cacao]`: buffer sizes, the constants for SEDX's
  reverse-Z camera and their std140 layout, X-Plane 12.4.4's dynamic values by camera height, the dispatches per preset),
  and the CPU half of the screen-space reflections (`renderer/ssr.*`, tag `[ssr]`: X-Plane's gloss and `ssr/*` weights,
  the lobe's history mip, the min / max depth pyramid with odd sizes, and the hierarchical-Z march against an analytic
  ground-and-wall scene), and the CPU half of the water (`renderer/ocean.*`, tag `[ocean]`: the FFT ocean's cascades covering
  every wave number once, the sea state X-Plane's `water/spectrum/*` controls make of a wave amplitude, JONSWAP / TMA /
  dispersion, directional spreading that integrates to 1, cascades normalized to the requested significant height, the
  GPU's 256-point inverse FFT against a direct sum, the water's clarity, wave scale and tessellation factors). Standard
  library plus `dependency/json`, fully green.

- **`SettingsTest`** (`source/Tests/settings_tests`) — `ConfigFile` (`settings/config_file.*`), the libconfig
  settings files read and written through the VFS: value types and conversions, groups never replaced by values,
  dirty tracking (floats compared as written), atomic saves through a mount, separate instances never sharing values,
  case-insensitive lookups, unparsable files left alone and backed up, byte order marks, missing / read-only mounts,
  UTF-8 physical paths. Standard library plus libconfig compiled from `dependency/libconfig`.

The other suites are **not** Linux-buildable as written: `MemoryAllocatorTests` links `vulkan-1` and needs the Vulkan
SDK; `VulkanSmokeTests` (opt-in via `SEDX_ENABLE_VULKAN_SMOKE_TESTS`) needs the full engine.

### Key gotchas

- **Test airports.** Feature tests, performance runs and X-Plane 12 comparisons use the same fifteen airports: KGON
  (Groton-New London, coastal), KDEN (X-Coder's Denver custom scenery), KORD (O'Hare, very large default airport), KBOS
  (Zero Dollar Payware's Boston Logan) and KBTV (Burlington, Laminar Research's home field) in the United States, plus
  EIDW (Dublin), LEBL (Barcelona, coastal), EGLL (Heathrow, a freeware pack), LSZH (Zurich), OMDB (Dubai), RCTP
  (Taoyuan), RJTT (Haneda, coastal), YSSY (Sydney, southern hemisphere, coastal), SBGR (Guarulhos, southern
  hemisphere) and CYYZ (Toronto Pearson). Their views are
  `tools/xp12_reference_probe/scenarios/xp12_airports.json` (generated by `make_parity_scenarios.py`); on a GPU host
  `python -I tools/xp12_reference_probe/render_scenarios.py` renders them in SEDX and reports load times, frame timings
  and GPU passes per airport, and the reference probe's "test-airport" set renders them in X-Plane 12.
- **Where the editor keeps its own files** (`VFS::MakeAppLayout`, mounted as `user:/ data:/ cache:/ logs:/ temp:/`,
  set up by `IO::FileSystem::InitAppLocations()` at startup): settings in `%APPDATA%\SceneryEditorX` (logs in its
  `logs\`), recovery data in `%LOCALAPPDATA%\SceneryEditorX\project-cache`, rebuildable caches (airport index,
  Gateway downloads, compiled shaders) in `%LOCALAPPDATA%\SceneryEditorX\cache`. macOS uses `~/Library/Application
  Support` and `~/Library/Caches`, Linux the XDG folders. Startup moves files older builds kept elsewhere once
  (never overwriting) and logs each move under the `Files` tag. Deleting the cache folder is always safe.
- **Submodules use SSH URLs** (`git@github.com:` in `.gitmodules`) and ship uninitialized.
  Fetch over HTTPS. The startup update script initializes only `dependency/json` and
  `dependency/Catch2` (all the Linux test build needs). Heavier submodules (`vcpkg`, `SDL`,
  `entt`, `libconfig`, `msdf-atlas-gen`, `taskflow`, `IconFontCppHeaders`) are only needed for
  the full GUI build — fetch on demand with
  `git -c url."https://github.com/".insteadOf="git@github.com:" submodule update --init <path>`.
- **Submodule commits differ per branch.** After switching branches run
  `git -c url."https://github.com/".insteadOf="git@github.com:" submodule update --init --depth 1 dependency/json dependency/Catch2`
  so the checked-out submodule commit matches the branch.
- **Use `g++`/`gcc` explicitly, not the default `c++`.** On this image `/usr/bin/c++`
  resolves to `clang++`, which fails to link with `cannot find -lstdc++`. Pass
  `-DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++` to CMake, or invoke `g++` directly.
- **Everything is C++20** (`std::ranges`, `std::numbers`, `set::contains`). Compile with `-std=c++20`.
- **`RefTests` has a pre-existing crash on `dev`:** the "Ref interoperability with
  std::shared_ptr / Construction from std::shared_ptr" case (`source/Tests/pointer_tests/RefTest.cpp`)
  aborts with a double-free/`SIGABRT`. This is a code bug on this branch, not an environment
  issue. Its `[performance]`/`[stress]`/`[thread]`/`[race]` cases are very slow — exclude them
  with the tag filter below when you just want correctness results.

### Build & run the Linux-buildable tests (from repo root, after submodules are fetched)

```bash
C2=dependency/Catch2
# 1. Build Catch2 v3 once (cross-platform, ~10s)
cmake -S $C2 -B $C2/build-linux -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ \
      -DCMAKE_BUILD_TYPE=Release -DCATCH_INSTALL_DOCS=OFF -DBUILD_TESTING=OFF
cmake --build $C2/build-linux -j"$(nproc)"

mkdir -p build_linux_tests
INCS="-I$C2/src -I$C2/build-linux/generated-includes -Isource -Idependency/json/include"
LIBS="$C2/build-linux/src/libCatch2Main.a $C2/build-linux/src/libCatch2.a"

# 2. ConversionTests (fully green)
g++ -std=c++20 -O2 -pthread $INCS source/Tests/conversion_tests/*.cpp $LIBS \
    -o build_linux_tests/ConversionTests
./build_linux_tests/ConversionTests

# 3. RefTests (exclude slow perf/concurrency tags)
g++ -std=c++20 -O2 -pthread -DSEDX_NO_LOGGING -DZoneScoped= $INCS \
    source/Tests/pointer_tests/RefTest.cpp \
    source/Tests/pointer_tests/WeakRefTest.cpp \
    source/Tests/pointer_tests/RefPerformanceTest.cpp \
    source/Tests/pointer_tests/RefThreadSafetyTest.cpp $LIBS \
    -o build_linux_tests/RefTests
./build_linux_tests/RefTests "~[performance]~[stress]~[thread]~[race]~[safety]~[Threading]"

# 4. XPlaneFormatTests (fully green)
g++ -std=c++20 -O2 -pthread $INCS source/Tests/xplane_tests/XPlaneFormatTests.cpp \
    source/Tests/xplane_tests/AirportTests.cpp source/Tests/xplane_tests/OverlayTests.cpp \
    source/Tests/xplane_tests/DsfWriterTests.cpp source/Tests/xplane_tests/FacadeTests.cpp \
    source/Tests/xplane_tests/FacadeWriterTests.cpp source/Tests/xplane_tests/InstallFinderTests.cpp \
    source/Tests/xplane_tests/BrdfLutTests.cpp source/SceneryEditorX/environment/brdf_lut.cpp \
    source/Tests/xplane_tests/ThumbnailTests.cpp source/SceneryEditorX/xplane/xp_thumbnail.cpp \
    source/Tests/xplane_tests/TerrainTests.cpp source/Tests/xplane_tests/AutogenTests.cpp source/SceneryEditorX/xplane/xp_7z.cpp \
    source/SceneryEditorX/xplane/xp_terrain.cpp source/SceneryEditorX/xplane/xp_terrain_mesh.cpp \
    source/SceneryEditorX/filesystem/vfs.cpp \
    source/SceneryEditorX/xplane/xp_obj8.cpp source/SceneryEditorX/xplane/xp_library.cpp \
    source/SceneryEditorX/xplane/xp_lights.cpp source/SceneryEditorX/xplane/xp_apt.cpp \
    source/SceneryEditorX/xplane/xp_apt_geometry.cpp source/SceneryEditorX/xplane/xp_airport_index.cpp \
    source/SceneryEditorX/xplane/xp_gateway_types.cpp source/SceneryEditorX/xplane/xp_dsf.cpp \
    source/SceneryEditorX/xplane/xp_assets.cpp source/SceneryEditorX/xplane/xp_autogen.cpp source/SceneryEditorX/xplane/xp_facade.cpp \
    source/SceneryEditorX/xplane/xp_facade_writer.cpp source/SceneryEditorX/xplane/xp_facade_authoring.cpp \
    source/SceneryEditorX/xplane/xp_overlay.cpp source/SceneryEditorX/xplane/xp_install_finder.cpp $LIBS \
    -o build_linux_tests/XPlaneFormatTests
./build_linux_tests/XPlaneFormatTests

# 5. ProjectFormatTests (fully green)
g++ -std=c++20 -O2 -pthread $INCS source/Tests/project_tests/ProjectFormatTests.cpp \
    source/Tests/project_tests/SplineEditTests.cpp source/SceneryEditorX/project/spline_edit.cpp \
    source/Tests/project_tests/FacadeEditTests.cpp source/SceneryEditorX/project/facade_edit.cpp \
    source/Tests/project_tests/VfsTests.cpp source/Tests/project_tests/SceneryPackTests.cpp \
    source/SceneryEditorX/filesystem/vfs.cpp source/SceneryEditorX/project/scenery_pack.cpp \
    source/SceneryEditorX/xplane/xp_airport_document.cpp source/SceneryEditorX/xplane/xp_airport_index.cpp \
    source/SceneryEditorX/project/scenery_project.cpp source/SceneryEditorX/project/project_edx.cpp \
    source/SceneryEditorX/project/project_sex.cpp source/SceneryEditorX/project/project_wed.cpp \
    source/SceneryEditorX/project/xml_document.cpp source/SceneryEditorX/project/project_io.cpp \
    source/SceneryEditorX/project/project_cache.cpp source/SceneryEditorX/project/project_document.cpp \
    source/SceneryEditorX/project/project_dsf.cpp source/SceneryEditorX/xplane/xp_apt.cpp \
    source/SceneryEditorX/xplane/xp_library.cpp source/SceneryEditorX/xplane/xp_apt_geometry.cpp \
    source/SceneryEditorX/xplane/xp_dsf.cpp source/SceneryEditorX/xplane/xp_7z.cpp source/SceneryEditorX/xplane/xp_assets.cpp \
    source/SceneryEditorX/xplane/xp_autogen.cpp source/SceneryEditorX/xplane/xp_facade.cpp source/SceneryEditorX/xplane/xp_overlay.cpp $LIBS \
    -o build_linux_tests/ProjectFormatTests
./build_linux_tests/ProjectFormatTests "~[performance]"

# 6. EnvironmentTests (fully green; run from the repo root so the shipped scenario file is checked)
ENV=source/SceneryEditorX/environment
g++ -std=c++20 -O2 -pthread $INCS source/Tests/environment_tests/ParityScenarioTests.cpp \
    source/Tests/environment_tests/EnvironmentTests.cpp $ENV/parity_scenario.cpp $ENV/solar_position.cpp \
    $ENV/surface_weather.cpp $ENV/season_model.cpp $ENV/sky_conditions.cpp $ENV/environment_state.cpp \
    $ENV/environment.cpp $ENV/sky_light.cpp $ENV/atmosphere_params.cpp source/SceneryEditorX/renderer/renderer_quality.cpp \
    source/SceneryEditorX/renderer/renderer_features.cpp source/SceneryEditorX/renderer/exposure.cpp \
    source/SceneryEditorX/renderer/env_probes.cpp source/SceneryEditorX/renderer/csm.cpp source/SceneryEditorX/renderer/cacao.cpp \
    source/SceneryEditorX/renderer/ssr.cpp source/SceneryEditorX/renderer/ocean.cpp $LIBS -o build_linux_tests/EnvironmentTests
./build_linux_tests/EnvironmentTests

# 7. SettingsTest (green with MSYS2 g++ 15 on Windows; libconfig's C sources built with gcc)
LC=dependency/libconfig/lib
mkdir -p build_linux_tests/libconfig
for f in grammar libconfig scanctx scanner strbuf strvec util wincompat; do
  gcc -O2 -c -DLIBCONFIG_STATIC -DHAVE_USELOCALE -DHAVE_NEWLOCALE -DHAVE_FREELOCALE -I$LC $LC/$f.c \
      -o build_linux_tests/libconfig/$f.o
done
g++ -std=c++20 -O2 -pthread -DLIBCONFIG_STATIC -DLIBCONFIGXX_STATIC $INCS -I$LC \
    source/Tests/settings_tests/SettingsTest.cpp source/SceneryEditorX/settings/config_file.cpp \
    source/SceneryEditorX/filesystem/vfs.cpp $LC/libconfigcpp.cc build_linux_tests/libconfig/*.o $LIBS \
    -o build_linux_tests/SettingsTest
./build_linux_tests/SettingsTest
```

MinGW (MSYS2) has no `fsync`, which libconfig's `wincompat.h` maps its `posix_fsync` to there: on that toolchain drop the
`HAVE_*LOCALE` defines and add `-Wno-error=implicit-function-declaration` to the `gcc` line (wincompat.c then supplies it).

`build_linux_tests/` and `dependency/Catch2/build-linux/` are throwaway build output — do not commit them.

### Full cross-platform build (reference; needs a Vulkan-capable host, not this VM)

Use the presets in `CMakePresets.json`. On Linux with the Vulkan SDK installed and `VULKAN_SDK`
set: `cmake --preset linux-ninja-gcc` then `cmake --build --preset build-linux-gcc-debug`
(enable tests with `-DSEDX_ENABLE_TESTS=ON`). Requires `ninja`, the bundled `vcpkg` submodule
(plus `SDL`, `entt`, `libconfig`, etc.), the Vulkan SDK with `slang`, and `Freetype`. On Windows,
install CMake + Vulkan SDK, set `VULKAN_SDK`/`XPLANE_12_SDK`, and use the `windows-*` presets or
`SetupProject.bat`.

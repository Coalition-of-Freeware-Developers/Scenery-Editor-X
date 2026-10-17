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

The other suites are **not** Linux-buildable as written: `SettingsTest` links Windows libs
(`dxgi`, `d3d12`, `Shell32.lib`); `MemoryAllocatorTests` links `vulkan-1` and needs the Vulkan
SDK; `VulkanSmokeTests` (opt-in via `SEDX_ENABLE_VULKAN_SMOKE_TESTS`) needs the full engine.

### Key gotchas

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
INCS="-I$C2/src -I$C2/build-linux/generated-includes -Isource"
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
```

`build_linux_tests/` and `dependency/Catch2/build-linux/` are throwaway build output — do not commit them.

### Full cross-platform build (reference; needs a Vulkan-capable host, not this VM)

Use the presets in `CMakePresets.json`. On Linux with the Vulkan SDK installed and `VULKAN_SDK`
set: `cmake --preset linux-ninja-gcc` then `cmake --build --preset build-linux-gcc-debug`
(enable tests with `-DSEDX_ENABLE_TESTS=ON`). Requires `ninja`, the bundled `vcpkg` submodule
(plus `SDL`, `entt`, `libconfig`, etc.), the Vulkan SDK with `slang`, and `Freetype`. On Windows,
install CMake + Vulkan SDK, set `VULKAN_SDK`/`XPLANE_12_SDK`, and use the `windows-*` presets or
`SetupProject.bat`.

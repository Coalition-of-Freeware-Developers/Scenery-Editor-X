# Building from source

Scenery Editor X is open source under the MIT License. This appendix is a short orientation for anyone who wants to build it or contribute; the repository's own documents have the details and stay current with the code.

## What you need

| | Notes |
|---|---|
| **Git** | With access to GitHub. Submodules are listed with SSH URLs; see below for HTTPS. |
| **CMake** | A recent version; the build uses `CMakePresets.json`. |
| **A C++20 compiler** | On Windows, Visual Studio 2022 or 2026 with the *Desktop development with C++* workload (the configuration test builds use). Clang-cl and MinGW GCC presets exist too. |
| **The Vulkan SDK** | 1.4 or newer, with its `slang` shader compiler. Set the `VULKAN_SDK` environment variable to its folder. |
| **X-Plane 12** | To run and test the editor against real scenery. |

The C++ libraries come from the bundled `vcpkg` submodule; the first configure builds them, which takes a while.

## Getting the code

```bash
git clone --recurse-submodules https://github.com/Coalition-of-Freeware-Developers/Scenery-Editor-X.git
cd Scenery-Editor-X
git checkout dev
```

Development happens on the `dev` branch and on feature branches cut from it; `master` only moves for releases, and pull requests target `dev`. The submodules use SSH URLs; to fetch them over HTTPS instead:

```bash
git -c url."https://github.com/".insteadOf="git@github.com:" submodule update --init --recursive
```

After switching branches, update the submodules again: each branch pins its own submodule versions.

## Configuring and building

Pick a configure preset for your platform and compiler from `CMakePresets.json`, for example on Windows with Visual Studio 2026:

```bash
cmake --preset windows-msvc-vs2026
cmake --build build --config Debug --target SceneryEditorX
```

On Windows, `SetupProject.bat` does the configure step for you. The editor is built into `bin\` (`SceneryEditorX_debug.exe` for Debug builds). Run it with the repository root as the working folder, so it finds `resources\`.

| Topic | Where |
|---|---|
| Build environment on Windows (Ninja, MinGW, vcpkg triplets) | [BUILD_SETUP_WINDOWS.md](../../../BUILD_SETUP_WINDOWS.md) |
| Building and running the unit tests, including on Linux without a GPU | [AGENTS.md](../../../AGENTS.md) |
| Packaging an installer for testers | [installer/README.md](../../../installer/README.md) |
| Engine and editor systems (rendering, assets, serialization, settings…) | The [docs folder](../../) |
| Project file formats, in full | [Project File Formats](../../edx%20File%20Format%20Documentation.md) |
| Coding style | [docs/coding-style.md](../../coding-style.md) |

## Testing

The file formats, X-Plane parsers, geometry and environment model are standard C++ with no engine dependencies, and are covered by Catch2 test suites under `source/Tests`: `XPlaneFormatTests`, `ProjectFormatTests`, `EnvironmentTests`, `ConversionTests` and `RefTests`. They build and run on any platform, even without a GPU.

The viewport tools have an automated self-test inside the editor: start it with an airport or project and `--tools-selftest`, and it logs a PASS or FAIL per step, then undoes its edits.

## Contributing

- Report bugs and request features on the [issue tracker](https://github.com/Coalition-of-Freeware-Developers/Scenery-Editor-X/issues).
- Open pull requests against `dev`.
- Changes that affect what users see should update this manual too: see [Writing for this manual](writing-the-manual.md).

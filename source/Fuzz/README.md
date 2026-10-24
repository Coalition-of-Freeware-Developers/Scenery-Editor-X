# Scenery Editor X fuzzing

Fuzz tests for everything the editor reads from files it didn't write: X-Plane assets and scenery, Scenery Gateway
downloads, the user's projects and the files the install finder reads at startup. They look for crashes, memory
errors, undefined behaviour, hangs and runaway memory use, on Windows, Linux and macOS.

The engines, sanitizers and CI are existing open-source tools; the project code here is one small harness per format
(`harnesses/`), the seeds and dictionaries, and the CMake glue.

| Tool | Used for | Windows | Linux | macOS |
|---|---|---|---|---|
| [libFuzzer](https://llvm.org/docs/LibFuzzer.html) | The engine and the harness API (`LLVMFuzzerTestOneInput`) every other engine reuses | MSVC `/fsanitize=fuzzer`, LLVM clang++ | Clang | Homebrew LLVM (Apple's Clang has no libFuzzer) |
| AddressSanitizer, UndefinedBehaviorSanitizer | Memory errors and undefined behaviour become crashes the engine keeps | ASan (MSVC), ASan + UBSan (clang++) | both | both |
| [AFL++](https://github.com/AFLplusplus/AFLplusplus) | A second engine on the same harnesses; better at magic values | | `afl-clang-fast` | `afl-clang-fast` |
| [ClusterFuzzLite](https://google.github.io/clusterfuzzlite/) | Fuzzing in GitHub Actions: pull requests and nightly batches | | yes | |
| [RapidCheck](https://github.com/emil-e/rapidcheck) | Round-trip property tests (`properties/`): random models through a writer and its reader | yes | yes | yes |
| LLVM `StandaloneFuzzTargetMain.c` | Replays seed files through a harness on compilers without libFuzzer (GCC, Apple Clang) | | yes | yes |

## Quick start

The project in this folder configures on its own: it needs CMake 3.20+, a C++20 compiler and (optionally) zlib. It
uses `dependency/json` and `dependency/Catch2` when those submodules are checked out and downloads the same commits
otherwise. RapidCheck is downloaded at a pinned commit.

```bash
# Windows, MSVC (VS 2022 17.0+). zlib for fuzz_zip: the editor build's vcpkg_installed/x64-windows works
cmake -S source/Fuzz -B build/fuzz-msvc -A x64 -DCMAKE_PREFIX_PATH=<...>/vcpkg_installed/x64-windows
cmake --build build/fuzz-msvc --config RelWithDebInfo
ctest --test-dir build/fuzz-msvc -C RelWithDebInfo -j8

# Linux (or Windows from a VS developer prompt): LLVM clang++, ASan + UBSan
cmake -S source/Fuzz -B build/fuzz-clang -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build/fuzz-clang && ctest --test-dir build/fuzz-clang -j8
# (On Windows the build uses the release static C runtime whatever the build type: LLVM's libFuzzer needs it)

# macOS: brew install llvm ninja, then
cmake --preset mac-llvm -S source/Fuzz && cmake --build --preset mac-llvm -S source/Fuzz

# GCC (any OS): no fuzzing engine, but the corpora replay under ASan + UBSan
cmake -S source/Fuzz -B build/fuzz-gcc -G Ninja -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
```

`CMakePresets.json` here has the same set (`msvc`, `clang`, `mac-llvm`, `gcc`, `afl`) plus `*-smoke` test presets.

`ctest` runs `<harness>.corpus` (every seed through every harness, the regression check that must pass everywhere)
and the property tests. Configure with `-DSEDX_FUZZ_SMOKE_SECONDS=60` to add `<harness>.smoke` tests (label
`fuzz-smoke`) that fuzz each harness for that long.

## Fuzzing a harness

```bash
cd build/fuzz-msvc/RelWithDebInfo          # (build/fuzz-clang on Linux / macOS)
mkdir corpus-dsf
./fuzz_dsf corpus-dsf ../../../source/Fuzz/seeds/dsf ../generated-seeds/dsf -dict=../dictionaries/fuzz_dsf.dict \
    -timeout=10 -rss_limit_mb=2560 -malloc_limit_mb=1024 -max_total_time=600 -jobs=8
```

New inputs go to the first folder (`corpus-dsf`), crashes to `crash-<sha1>`, hangs to `timeout-<sha1>` and memory
blowups to `oom-<sha1>` / `slow-unit-<sha1>`. Then:

- **Reproduce:** `./fuzz_dsf crash-<sha1>` (runs that one input; a debugger works the same way).
- **Minimize:** `./fuzz_dsf -minimize_crash=1 -runs=10000 crash-<sha1>`.
- **Keep it:** once fixed, copy the input to `seeds/<format>/regression_<what>` so `<harness>.corpus` guards it on every
  platform from then on.
- **Shrink a corpus:** `./fuzz_dsf -merge=1 corpus-small corpus-dsf`.

On Windows, libFuzzer can't save the input when the harness overflows the stack (the report says
`AddressSanitizer: stack-overflow` and names no file); rerun with `-runs=` and `-print_final_stats=1` near the crash,
or reproduce on Linux, where it is saved.

### Performance problems

A hang or runaway allocation is a failure like a crash: `-timeout` (10 s here; libFuzzer's default is 20 minutes),
`-rss_limit_mb` (2560) and `-malloc_limit_mb` (1024, one allocation) stop the run and save the input. libFuzzer also
reports `-report_slow_units` inputs (10 s by default). The CMake cache variables `SEDX_FUZZ_TIMEOUT`,
`SEDX_FUZZ_RSS_LIMIT_MB` and `SEDX_FUZZ_MALLOC_LIMIT_MB` set the limits the tests use.

### AFL++ (Linux, macOS)

```bash
CC=afl-clang-fast CXX=afl-clang-fast++ cmake -S source/Fuzz -B build/fuzz-afl -G Ninja -DSEDX_FUZZ_ENGINE=afl -DSEDX_FUZZ_PROPERTIES=OFF
cmake --build build/fuzz-afl
echo core | sudo tee /proc/sys/kernel/core_pattern      # Linux only
source/Fuzz/scripts/run_afl.sh build/fuzz-afl 600 8     # 10 minutes per harness, 8 at a time
```

Findings land in `build/fuzz-afl/afl-work/<harness>/out/default/{crashes,hangs}`; they replay through the libFuzzer
build too.

## Harnesses

| Harness | Code under test | Besides "no crash, no UB, no hang" it checks |
|---|---|---|
| `fuzz_obj8` | `xp_obj8` | Every index, TRIS range, animation group, LOD and hide/show rule points inside the model (the object loader indexes with them unchecked); animation at extreme dataref values |
| `fuzz_apt_dat` | `xp_apt`, `xp_apt_geometry`, `xp_overlay` | Written apt.dat reads back, a second round trip changes nothing; pavement triangulation, line ribbons and library surfaces index inside their vertices |
| `fuzz_dsf` | `xp_dsf`, `xp_overlay` | Every placement drawn through the overlay builder; a tile we can write reads back with the same placements; per-tile split |
| `fuzz_library_txt`, `fuzz_scenery_packs` | `xp_library` | The library index's priority resolution stays inside its providers and packages |
| `fuzz_lights` | `xp_lights` | Named / parameterized OBJ lights resolve against the catalog |
| `fuzz_polygon`, `fuzz_line`, `fuzz_object_string`, `fuzz_forest`, `fuzz_autogen`, `fuzz_road_network` | `xp_assets`, `xp_overlay` | The asset drawn the way DSF tiles place it (squares, holes, lines, wall types, bezier curves, tall and long footprints) |
| `fuzz_facade` | `xp_assets`, `xp_facade`, `xp_facade_writer`, `xp_overlay` | The writer's promise (writes what parses; writing that again is the same text), floor / wall / spelling choices, drawing |
| `fuzz_gateway_json` | `xp_gateway_types` | Every Gateway record parser and base64 on the same reply, and the zip it carries |
| `fuzz_zip` | `xp_zip`, `VFS::ResolveInside` | Archive names the install guard accepts stay inside the pack (zip slip), nested archives |
| `fuzz_install_finder` | `xp_install_finder` | x-plane_install_12.txt, Steam .vdf / .acf, Log.txt lines (startup reads them) |
| `fuzz_vfs_paths` | `filesystem/vfs` | Normalizing settles in one pass and never climbs out; virtual paths split and rebuild; portable names pass the portability check |
| `fuzz_sex` | `project_sex` | Runs each input as is and with its CRCs fixed (so the decoders behind the checksums get fuzzed); a loaded project saves and loads back byte for byte, also as .edX |
| `fuzz_edx` | `project_edx`, `project_dsf`, `xp_overlay` | A save of what was loaded loads back and a second save changes no byte (git-tracked format); the DSF the viewport draws and the exporter writes |
| `fuzz_wed`, `fuzz_xml` | `project_wed`, `xml_document` | Exported earth.wed.xml imports again; written XML parses into a tree that writes the same text |
| `fuzz_coordinate` | `scenery_pack` | An accepted coordinate is in range; its formatted text parses back and formats the same |

`common/fuzz_assets.*` writes a tiny scenery pack (one asset of each kind) to the temp folder, so harnesses that fuzz
placements reach the real geometry builders instead of "asset not found".

### Adding a harness

1. `harnesses/fuzz_<format>.cpp` with `extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)` that
   calls the parser on the bytes and then whatever the editor does with the result. Use `SEDX_FUZZ_CHECK` for
   invariants (it aborts in every build type). Keep it standard-library only so it builds with every compiler.
2. Seeds in `seeds/<format>/` (small, our own files: never Laminar's or anyone else's scenery), or teach
   `tools/fuzz_seed_writer.cpp` to write them for binary formats.
3. Keywords in `dictionaries/<format>.dict` (libFuzzer / AFL++ format).
4. One `sedx_add_fuzzer(...)` line in `CMakeLists.txt`. If the code under test needs new sources, add them to
   `SEDX_FUZZ_CORE_SOURCES`.

## Seeds and dictionaries

- `seeds/<format>/`: committed text samples (most come from the unit tests' fixtures) and `regression_*` inputs that
  once crashed or hung. `.gitattributes` keeps them byte for byte (`-text`).
- `generated-seeds/` (build folder): `.sex`, `.edX`, earth.wed.xml, apt.dat, DSF tiles and zips written by our own
  writers at build time from a new airport project, a project with one item of every kind and the repository's
  `docs/` samples, so they always match the current format versions.
- `dictionaries/`: keywords taken from each parser's string literals plus the binary magics. Set
  `SEDX_FUZZ_AFL_DICTIONARIES` to AFL++'s `dictionaries/` folder to merge its `json.dict` / `xml.dict` in (they aren't
  copied here: AFL++ is AGPL licensed).

## Round-trip properties

`properties/RoundTripProperties.cpp` generates valid models (airports, projects, DSF tiles, coordinates, paths) and
checks that each writer and reader agree: apt.dat and .edX text and .sex bytes survive write -> read -> write
unchanged, .edX and .sex carry the same project, DSF quantization stays within 10 cm. A failure prints the smallest
model RapidCheck shrank it to. `RC_PARAMS="max_success=1000"` runs more cases; a failure prints the
`RC_PARAMS="reproduce=..."` that replays it.

## CI

- `.github/workflows/cflite_pr.yml`: ClusterFuzzLite on pull requests to `dev` / `master` that touch the parsers (ASan
  and UBSan, 10 minutes, code-change mode). `cflite_batch.yml`: every harness for an hour each night.
  `.clusterfuzzlite/` holds the build (OSS-Fuzz's Ubuntu 24.04 image, `SEDX_FUZZ_ENGINE=oss-fuzz`). Without a corpus
  storage repository each run starts from the seeds; create one and fill in the commented `storage-repo` lines to keep
  the corpus between runs.
- `.github/workflows/fuzz.yml`: Windows (MSVC), macOS (Homebrew LLVM) and Linux (GCC replay, AFL++) on the same
  pull requests: corpus replay, property tests and a minute of fuzzing per harness. Reproducers are uploaded as
  artifacts when a job fails.

## Engines (`SEDX_FUZZ_ENGINE`)

| Value | What links in | When |
|---|---|---|
| `auto` (default) | `oss-fuzz` when `LIB_FUZZING_ENGINE` is set, `libfuzzer` for MSVC / LLVM Clang, else `standalone` | |
| `libfuzzer` | `-fsanitize=fuzzer` (Clang) or `/fsanitize=fuzzer` (MSVC) | Local fuzzing |
| `afl` | AFL++'s driver, through `afl-clang-fast`'s `-fsanitize=fuzzer` | `scripts/run_afl.sh` |
| `standalone` | `third_party/llvm/StandaloneFuzzTargetMain.c` | GCC, Apple Clang: replay only |
| `oss-fuzz` | `$LIB_FUZZING_ENGINE`, flags from `$CFLAGS` / `$CXXFLAGS` | `.clusterfuzzlite/build.sh` |

`SEDX_FUZZ_SANITIZERS` (`auto`, `none` or a list such as `address;undefined`) picks the sanitizers. `clang-cl` isn't
supported (CMake links its targets with `link.exe`, which doesn't add the runtimes): use `clang++` or `cl.exe`.

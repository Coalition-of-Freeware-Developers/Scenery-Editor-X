#!/usr/bin/env bash
# Builds and runs Scenery Editor X's dependency-light Catch2 unit suites: the ones in AGENTS.md ("Build & run the
# Linux-buildable tests") that need only the standard library, dependency/json, dependency/Catch2 and
# dependency/libconfig. The unit-tests workflow (.github/workflows/unit-tests.yml) runs it on Linux (g++), macOS
# (Apple clang) and Windows (MSYS2 UCRT64 g++); it runs the same way on a developer machine.
#
#   bash scripts/ci/build_unit_tests.sh [--no-run] [Suite ...]
#
# Suites: ConversionTests RefTests XPlaneFormatTests ProjectFormatTests EnvironmentTests SettingsTest (default: all).
# Fetch the submodules first (they use SSH URLs):
#   git -c url."https://github.com/".insteadOf="git@github.com:" submodule update --init --depth 1 \
#       dependency/json dependency/Catch2 dependency/libconfig
#
# Environment:
#   CC, CXX           compilers (default gcc / g++; clang / clang++ on macOS)
#   SEDX_TEST_OUT     output folder (default build/unit-tests), without spaces: Catch2, objects, executables in bin/,
#                     JUnit reports in reports/<Suite>.xml. Objects are rebuilt on every run; Catch2 only when the
#                     compiler changes. Every other path is relative to the repository root, so its location doesn't
#                     matter; CC and CXX may hold a launcher too ("ccache g++").
#   SEDX_TEST_JOBS    parallel compiles (default: the number of CPUs)
#   SEDX_TEST_CXXFLAGS  extra flags for the suites' C++ compiles and links, not Catch2's: e.g. -fsanitize=address,
#                     or -DSEDX_FLOAT_CHARS_FALLBACK to run every suite on the float text fallback macOS builds use
#   MACOSX_DEPLOYMENT_TARGET   macOS only, default 13.0 like cmake/toolchains/sedx-mac.cmake, so a libc++ function
#                     only newer macOS versions ship fails here as it does in the editor build
#
# Cases tagged [xplane-install] or [library-sweep] read an X-Plane 12 install and are left out (CI machines have none),
# and so is ProjectFormatTests' [performance] timing case, as AGENTS.md runs it. Every suite runs even when another
# fails to build or fails its tests; the exit status is non-zero when any did.
#
# Keep the source lists below in step with the recipe in AGENTS.md. Test files are picked up from each suite's folder
# in source/Tests, so a new test file runs without changes here; a new engine source it needs goes in the list.

set -euo pipefail

SCRIPT="$(cd "$(dirname "$0")" && pwd)/$(basename "$0")"
cd "$(dirname "$SCRIPT")/../.."

case "$(uname -s)" in
	Darwin) PLATFORM=macos ;;
	*_NT*|MINGW*|MSYS*|CYGWIN*) PLATFORM=windows ;;		# MSYS2 reports MINGW64_NT-..., UCRT64_NT-..., CLANG64_NT-...
	*) PLATFORM=linux ;;
esac

if [ "$PLATFORM" = macos ]; then
	: "${CC:=clang}" "${CXX:=clang++}"
	: "${MACOSX_DEPLOYMENT_TARGET:=13.0}"
	export MACOSX_DEPLOYMENT_TARGET
else
	: "${CC:=gcc}" "${CXX:=g++}"
fi
export CC CXX

OUT=${SEDX_TEST_OUT:-build/unit-tests}
case "$OUT" in
	*" "*) echo "SEDX_TEST_OUT ($OUT) can't contain spaces: the compiler flags hold paths in it" >&2; exit 2 ;;
esac
OBJ=$OUT/obj
BIN=$OUT/bin
REPORTS=$OUT/reports
CATCH=dependency/Catch2
CATCH_BUILD=$OUT/catch2
LC=dependency/libconfig/lib
SE=source/SceneryEditorX
export SEDX_TEST_OUT=$OUT

PLATFORM_FLAGS=""
SYSTEM_LIBS=""
C_PLATFORM_FLAGS="-DHAVE_USELOCALE -DHAVE_NEWLOCALE -DHAVE_FREELOCALE"
case "$PLATFORM" in
	macos)
		PLATFORM_FLAGS="-mmacosx-version-min=$MACOSX_DEPLOYMENT_TARGET"
		C_PLATFORM_FLAGS="$C_PLATFORM_FLAGS $PLATFORM_FLAGS"
		;;
	windows)
		# vfs.cpp's known folders and install_finder's registry reads. MinGW has no fsync, which libconfig's
		# wincompat.h maps posix_fsync to, nor the POSIX locale calls: wincompat.c supplies the fallbacks.
		SYSTEM_LIBS="-ladvapi32 -lshell32 -lole32 -luuid"
		C_PLATFORM_FLAGS="-Wno-error=implicit-function-declaration"
		;;
esac

CXX_BASE="-std=c++20 -O2 -pthread $PLATFORM_FLAGS"
EXTRA_CXXFLAGS=${SEDX_TEST_CXXFLAGS:-}
export SEDX_TEST_CXXFLAGS="$EXTRA_CXXFLAGS"
INCS="-I$CATCH/src -I$CATCH_BUILD/generated-includes -Isource -Idependency/json/include"

# @brief The compiler flags of a flag set: std (most suites), ref (RefTests), settings (SettingsTest) or libconfig (C).
flags_for()
{
	case "$1" in
		std) echo "$CXX_BASE $EXTRA_CXXFLAGS $INCS" ;;
		ref) echo "$CXX_BASE $EXTRA_CXXFLAGS -DSEDX_NO_LOGGING -DZoneScoped= $INCS" ;;
		settings) echo "$CXX_BASE $EXTRA_CXXFLAGS -DLIBCONFIG_STATIC -DLIBCONFIGXX_STATIC $INCS -I$LC" ;;
		libconfig) echo "-O2 -DLIBCONFIG_STATIC $C_PLATFORM_FLAGS -I$LC" ;;
		*) echo "unknown flag set $1" >&2; return 1 ;;
	esac
}

# One compile, run by xargs in parallel: --compile-one <c|c++> <flag set> <source> <object>
if [ "${1:-}" = --compile-one ]; then
	kind=$2; flagset=$3; src=$4; obj=$5
	mkdir -p "$(dirname "$obj")"
	if [ "$kind" = c ]; then compiler=$CC; else compiler=$CXX; fi
	echo "  $kind $src"
	status=0
	# shellcheck disable=SC2046
	$compiler $(flags_for "$flagset") -c "$src" -o "$obj" > "$obj.log" 2>&1 || status=$?
	if [ -s "$obj.log" ]; then
		printf '%s\n' "--- $src ($kind, $flagset)"
		cat "$obj.log"
	fi
	if [ "$status" -ne 0 ]; then
		echo "FAILED to compile $src"
		exit 1
	fi
	exit 0
fi

ALL_SUITES="ConversionTests RefTests XPlaneFormatTests ProjectFormatTests EnvironmentTests SettingsTest"
RUN=1
SUITES=""
for arg in "$@"; do
	case "$arg" in
		--no-run) RUN=0 ;;
		-h|--help) awk 'NR > 1 && /^#/ { print; next } NR > 1 { exit }' "$SCRIPT"; exit 0 ;;	# The header comment
		-*) echo "unknown option $arg" >&2; exit 2 ;;
		*)
			case " $ALL_SUITES " in
				*" $arg "*) SUITES="$SUITES $arg" ;;
				*) echo "unknown suite $arg (one of: $ALL_SUITES)" >&2; exit 2 ;;
			esac
			;;
	esac
done
[ -n "$SUITES" ] || SUITES=$ALL_SUITES

# @brief "<flag set> <sources...>" of a suite: its test folder's .cpp files, then the engine sources it links.
suite_sources()
{
	case "$1" in
		ConversionTests)
			echo std source/Tests/conversion_tests/*.cpp ;;
		RefTests)
			echo ref source/Tests/pointer_tests/*.cpp ;;
		XPlaneFormatTests)
			echo std source/Tests/xplane_tests/*.cpp \
				$SE/environment/brdf_lut.cpp $SE/environment/season_model.cpp \
				$SE/filesystem/vfs.cpp \
				$SE/xplane/xp_thumbnail.cpp $SE/xplane/xp_block_compress.cpp $SE/xplane/xp_7z.cpp \
				$SE/xplane/xp_terrain.cpp $SE/xplane/xp_terrain_mesh.cpp \
				$SE/xplane/xp_obj8.cpp $SE/xplane/xp_library.cpp $SE/xplane/xp_lights.cpp \
				$SE/xplane/xp_apt.cpp $SE/xplane/xp_apt_geometry.cpp $SE/xplane/xp_airport_index.cpp \
				$SE/xplane/xp_gateway_types.cpp $SE/xplane/xp_dsf.cpp $SE/xplane/xp_assets.cpp \
				$SE/xplane/xp_autogen.cpp $SE/xplane/xp_facade.cpp $SE/xplane/xp_facade_writer.cpp \
				$SE/xplane/xp_facade_authoring.cpp $SE/xplane/xp_overlay.cpp $SE/xplane/xp_install_finder.cpp ;;
		ProjectFormatTests)
			echo std source/Tests/project_tests/*.cpp \
				$SE/project/spline_edit.cpp $SE/project/facade_edit.cpp $SE/project/scenery_pack.cpp \
				$SE/project/scenery_project.cpp $SE/project/project_edx.cpp $SE/project/project_sex.cpp \
				$SE/project/project_wed.cpp $SE/project/xml_document.cpp $SE/project/project_io.cpp \
				$SE/project/project_cache.cpp $SE/project/project_document.cpp $SE/project/project_dsf.cpp \
				$SE/filesystem/vfs.cpp \
				$SE/xplane/xp_airport_document.cpp $SE/xplane/xp_airport_index.cpp $SE/xplane/xp_apt.cpp \
				$SE/xplane/xp_library.cpp $SE/xplane/xp_apt_geometry.cpp $SE/xplane/xp_dsf.cpp $SE/xplane/xp_7z.cpp \
				$SE/xplane/xp_assets.cpp $SE/xplane/xp_autogen.cpp $SE/xplane/xp_facade.cpp $SE/xplane/xp_overlay.cpp ;;
		EnvironmentTests)
			echo std source/Tests/environment_tests/*.cpp \
				$SE/environment/parity_scenario.cpp $SE/environment/solar_position.cpp \
				$SE/environment/surface_weather.cpp $SE/environment/season_model.cpp \
				$SE/environment/sky_conditions.cpp $SE/environment/environment_state.cpp \
				$SE/environment/environment.cpp $SE/environment/sky_light.cpp $SE/environment/atmosphere_params.cpp \
				$SE/renderer/renderer_quality.cpp $SE/renderer/renderer_features.cpp $SE/renderer/exposure.cpp \
				$SE/renderer/env_probes.cpp $SE/renderer/csm.cpp $SE/renderer/cacao.cpp $SE/renderer/ssr.cpp \
				$SE/renderer/ocean.cpp ;;
		SettingsTest)
			echo settings source/Tests/settings_tests/*.cpp \
				$SE/settings/config_file.cpp $SE/filesystem/vfs.cpp $LC/libconfigcpp.cc ;;
	esac
}

# @brief The C sources a suite needs besides its C++ ones (libconfig for SettingsTest).
suite_c_sources()
{
	case "$1" in
		SettingsTest)
			for f in grammar libconfig scanctx scanner strbuf strvec util wincompat; do echo "$LC/$f.c"; done ;;
	esac
}

# @brief Catch2 test specs of a suite, one per line.
suite_specs()
{
	echo '~[xplane-install]'
	echo '~[library-sweep]'
	if [ "$1" = ProjectFormatTests ]; then echo '~[performance]'; fi
}

object_of() { local flat=${2//\//_}; echo "$OBJ/$1/$flat.o"; }

group() { if [ "${GITHUB_ACTIONS:-}" = true ]; then echo "::group::$*"; else echo "== $*"; fi; }
endgroup() { if [ "${GITHUB_ACTIONS:-}" = true ]; then echo "::endgroup::"; fi; }

cpu_count()
{
	nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2
}
JOBS=${SEDX_TEST_JOBS:-$(cpu_count)}

echo "Platform $PLATFORM ($(uname -s)), $JOBS jobs, output in $OUT"
echo "CC  = $CC: $($CC --version 2>&1 | head -n 1)"
echo "CXX = $CXX: $($CXX --version 2>&1 | head -n 1)"
[ -z "$EXTRA_CXXFLAGS" ] || echo "SEDX_TEST_CXXFLAGS = $EXTRA_CXXFLAGS"
[ "$PLATFORM" != macos ] || echo "MACOSX_DEPLOYMENT_TARGET = $MACOSX_DEPLOYMENT_TARGET"
# A file from each submodule: an uninitialized one is an empty folder
for file in dependency/json/include/nlohmann/json.hpp "$CATCH/CMakeLists.txt" "$LC/libconfig.c"; do
	if [ ! -f "$file" ]; then
		echo "$file is missing: fetch the submodules first (see the top of $SCRIPT)" >&2
		exit 2
	fi
done

# Catch2, built once per compiler with the same language standard as the tests.
catch_stamp="$CXX | $($CXX --version 2>&1 | head -n 1) | $CXX_BASE"
if [ ! -f "$CATCH_BUILD/src/libCatch2Main.a" ] || [ "$(cat "$CATCH_BUILD/.sedx-stamp" 2>/dev/null)" != "$catch_stamp" ]; then
	group "Catch2 ($(cmake --version | head -n 1))"
	rm -rf "$CATCH_BUILD"
	generator=""
	if command -v ninja > /dev/null 2>&1; then generator="-G Ninja"; fi
	osx=""
	if [ "$PLATFORM" = macos ]; then osx="-DCMAKE_OSX_DEPLOYMENT_TARGET=$MACOSX_DEPLOYMENT_TARGET"; fi
	# shellcheck disable=SC2086
	cmake -S "$CATCH" -B "$CATCH_BUILD" $generator $osx -DCMAKE_CXX_COMPILER="$CXX" -DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_CXX_STANDARD=20 -DCMAKE_CXX_STANDARD_REQUIRED=ON -DCATCH_INSTALL_DOCS=OFF -DBUILD_TESTING=OFF
	cmake --build "$CATCH_BUILD" --parallel "$JOBS"
	echo "$catch_stamp" > "$CATCH_BUILD/.sedx-stamp"
	endgroup
fi
# Release names: a Debug Catch2 build adds a d (libCatch2d.a), so change both if the build type above changes
CATCH_LIBS="$CATCH_BUILD/src/libCatch2Main.a $CATCH_BUILD/src/libCatch2.a"

rm -rf "$OBJ" "$REPORTS"
mkdir -p "$OBJ" "$BIN" "$REPORTS"

# @brief Compiles the sources a suite needs (those not built already in this run) and links it.
build_suite()
{
	local suite=$1 jobs=$OBJ/$1.jobs objects="" flagset src obj
	: > "$jobs"
	# shellcheck disable=SC2046
	set -- $(suite_sources "$suite")
	flagset=$1; shift
	for src in "$@"; do
		obj=$(object_of "$flagset" "$src")
		objects="$objects $obj"
		[ -f "$obj" ] || echo "c++ $flagset $src $obj" >> "$jobs"
	done
	for src in $(suite_c_sources "$suite"); do
		obj=$(object_of libconfig "$src")
		objects="$objects $obj"
		[ -f "$obj" ] || echo "c libconfig $src $obj" >> "$jobs"
	done

	if [ -s "$jobs" ] && ! xargs -n 4 -P "$JOBS" bash "$SCRIPT" --compile-one < "$jobs"; then
		return 1
	fi
	echo "  link $BIN/$suite"
	# shellcheck disable=SC2086
	$CXX $CXX_BASE $EXTRA_CXXFLAGS -o "$BIN/$suite" $objects $CATCH_LIBS $SYSTEM_LIBS
}

# @brief Runs a suite from the repository root with the console and JUnit reporters.
run_suite()
{
	local suite=$1 spec
	local -a specs
	specs=()
	while IFS= read -r spec; do specs+=("$spec"); done <<EOF
$(suite_specs "$suite")
EOF
	echo "  $BIN/$suite ${specs[*]}"
	# --name: the reports carry the suite's name on every platform (not SettingsTest.exe on Windows)
	"$BIN/$suite" "${specs[@]}" --name "$suite" --reporter console --reporter "junit::out=$REPORTS/$suite.xml"
}

# @brief An attribute of the <testsuite> element of a suite's JUnit report; nothing when the suite has no report.
# Catch2 v3's JUnit reporter counts assertions in tests= and failures= (not test cases), hence the summary's columns.
report_attribute()
{
	[ -f "$REPORTS/$1.xml" ] || return 0
	sed -n "s/.*<testsuite [^>]*$2=\"\\([^\"]*\\)\".*/\\1/p" "$REPORTS/$1.xml" | head -n 1 || true
}

failed=""
results=""
for suite in $SUITES; do
	echo
	echo "== $suite: build"
	start=$(date +%s)
	if ! build_suite "$suite"; then
		echo "== $suite: BUILD FAILED"
		failed="$failed $suite"
		results="$results$suite|build failed|$(( $(date +%s) - start ))\n"
		continue
	fi
	if [ "$RUN" -eq 0 ]; then
		results="$results$suite|built|$(( $(date +%s) - start ))\n"
		continue
	fi
	echo "== $suite: run"
	if run_suite "$suite"; then
		outcome=passed
	else
		outcome="FAILED (exit $?)"
		failed="$failed $suite"
	fi
	echo "== $suite: $outcome"
	results="$results$suite|$outcome|$(( $(date +%s) - start ))\n"
done

echo
echo "Summary ($PLATFORM, $($CXX --version 2>&1 | head -n 1)):"
summary="| Suite | Result | Assertions | Failed | Seconds |\n|---|---|---|---|---|\n"
while IFS='|' read -r suite outcome seconds; do
	[ -n "$suite" ] || continue
	assertions=$(report_attribute "$suite" tests)
	failures=$(report_attribute "$suite" failures)
	printf '  %-20s %-22s %8s assertions, %s failed, %ss\n' "$suite" "$outcome" "${assertions:--}" "${failures:--}" "$seconds"
	summary="$summary| $suite | $outcome | ${assertions:--} | ${failures:--} | $seconds |\n"
done <<EOF
$(printf '%b' "$results")
EOF
if [ -n "${GITHUB_STEP_SUMMARY:-}" ]; then
	{
		echo "### Unit tests: $PLATFORM ($($CXX --version 2>&1 | head -n 1))"
		echo
		printf '%b' "$summary"
	} >> "$GITHUB_STEP_SUMMARY"
fi

if [ -n "$failed" ]; then
	echo "Failed:$failed"
	exit 1
fi
if [ "$RUN" -eq 0 ]; then echo "All suites built."; else echo "All suites passed."; fi

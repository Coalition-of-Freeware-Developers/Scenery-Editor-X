#!/usr/bin/env bash
# -------------------------------------------------------
# Scenery Editor X - Fuzzing
# -------------------------------------------------------
# Copyright (c) 2026 Thomas Ray
# Copyright (c) 2026 Coalition of Freeware Developers
# -------------------------------------------------------
# run_afl.sh: runs AFL++ on every harness of an AFL++ build (Linux, macOS)
#
#   CC=afl-clang-fast CXX=afl-clang-fast++ cmake -S source/Fuzz -B build-afl -DSEDX_FUZZ_ENGINE=afl
#   cmake --build build-afl
#   source/Fuzz/scripts/run_afl.sh build-afl [seconds per harness=60] [parallel=CPU count] [harness ...]
#
# Each harness gets its committed and generated seeds and its dictionary; findings land in
# <build>/afl-work/<harness>/out/default/{crashes,hangs}. Exits non-zero when any harness crashed or hung.
# On Linux, AFL++ wants core dumps off the system crash handler first: echo core | sudo tee /proc/sys/kernel/core_pattern
# -------------------------------------------------------
set -eo pipefail		# (no -u: bash before 4.4 calls an empty array unbound)

BUILD="$(cd "$1" && pwd)"
SECONDS_EACH="${2:-60}"
PARALLEL="${3:-$(getconf _NPROCESSORS_ONLN)}"
shift $(( $# < 3 ? $# : 3 ))
FUZZ_ROOT="$(cd "$(dirname "$0")/.." && pwd)"

export AFL_SKIP_CPUFREQ=1 AFL_NO_UI=1 AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1 AFL_SKIP_CRASHES=1
export BUILD SECONDS_EACH FUZZ_ROOT

run_one() {
	local name="$1" work="$BUILD/afl-work/$1"
	rm -rf "$work"
	mkdir -p "$work/in"
	for dir in "$FUZZ_ROOT/seeds/${name#fuzz_}" "$BUILD/generated-seeds/${name#fuzz_}"; do
		if [ -d "$dir" ]; then
			find "$dir" -type f ! -name '.stamp' -exec cp {} "$work/in/" \;
		fi
	done
	local dict=()
	if [ -f "$BUILD/dictionaries/$name.dict" ]; then
		dict=(-x "$BUILD/dictionaries/$name.dict")
	fi
	afl-fuzz -V "$SECONDS_EACH" -t 10000 -m none -i "$work/in" -o "$work/out" "${dict[@]}" -- "$BUILD/$name" > "$work/afl.log" 2>&1 || true
	local crashes hangs
	crashes=$(find "$work/out" -path '*/crashes/id:*' 2>/dev/null | wc -l | tr -d ' ')
	hangs=$(find "$work/out" -path '*/hangs/id:*' 2>/dev/null | wc -l | tr -d ' ')
	echo "$name: $crashes crashes, $hangs hangs"
	[ "$crashes" -eq 0 ] && [ "$hangs" -eq 0 ]
}
export -f run_one

harnesses=("$@")
if [ ${#harnesses[@]} -eq 0 ]; then
	# (No mapfile: macOS ships bash 3.2)
	while IFS= read -r harness; do
		harnesses+=("$harness")
	done < <(find "$BUILD" -maxdepth 1 -type f -name 'fuzz_*' -perm -100 -exec basename {} \; | sort)
fi

status=0
printf '%s\n' "${harnesses[@]}" | xargs -P "$PARALLEL" -I{} bash -c 'run_one {}' || status=1
exit $status

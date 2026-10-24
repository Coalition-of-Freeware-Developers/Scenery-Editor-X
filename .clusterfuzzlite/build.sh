#!/bin/bash -eu
# ClusterFuzzLite / OSS-Fuzz build of source/Fuzz. The base image sets CC, CXX, CFLAGS, CXXFLAGS (sanitizer and
# coverage flags) and LIB_FUZZING_ENGINE; SEDX_FUZZ_ENGINE=oss-fuzz uses them as they are. The package target writes
# each fuzzer to $OUT with <fuzzer>_seed_corpus.zip, <fuzzer>.dict and <fuzzer>.options.
#
# dependency/json is an empty folder when the checkout skipped submodules; the fuzz project then downloads the same
# nlohmann/json commit.

AFL_DICTIONARIES=""
if [ -d "$SRC/aflplusplus/dictionaries" ]; then
	AFL_DICTIONARIES="$SRC/aflplusplus/dictionaries"
fi

cmake -S "$SRC/scenery-editor-x/source/Fuzz" -B "$WORK/sedx-fuzz" -G Ninja \
	-DSEDX_FUZZ_ENGINE=oss-fuzz \
	-DSEDX_FUZZ_PROPERTIES=OFF \
	-DSEDX_FUZZ_AFL_DICTIONARIES="$AFL_DICTIONARIES" \
	-DSEDX_FUZZ_OUT_DIR="$OUT"
cmake --build "$WORK/sedx-fuzz" --target sedx_fuzz_package -j"$(nproc)"

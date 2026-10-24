# LLVM StandaloneFuzzTargetMain.c

`StandaloneFuzzTargetMain.c` comes from the LLVM project, unchanged except for one line:

- Source: <https://github.com/llvm/llvm-project/blob/main/compiler-rt/lib/fuzzer/standalone/StandaloneFuzzTargetMain.c>
  (last upstream change 9b6135bf, 2019-02-26)
- License: Apache License v2.0 with LLVM Exceptions (SPDX `Apache-2.0 WITH LLVM-exception`), see
  <https://llvm.org/LICENSE.txt>
- Local change: inputs are opened with `"rb"` instead of `"r"`, so Windows (MinGW) builds read binary inputs byte for
  byte instead of translating line endings.

It is the `main()` of `SEDX_FUZZ_ENGINE=standalone` builds (GCC, Apple Clang): it runs every file named on the command
line through `LLVMFuzzerTestOneInput` once, without fuzzing, so the seed corpora and saved crash inputs replay on
compilers that have no libFuzzer.

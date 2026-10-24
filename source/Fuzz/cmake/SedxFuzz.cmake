# -------------------------------------------------------
# Scenery Editor X - Fuzzing
# -------------------------------------------------------
# Copyright (c) 2026 Thomas Ray
# Copyright (c) 2026 Coalition of Freeware Developers
# -------------------------------------------------------
# SedxFuzz.cmake
# -------------------------------------------------------
# Engine and sanitizer selection, sedx_add_fuzzer(), the corpus replay / smoke tests and the OSS-Fuzz package.
#
# Two interface targets carry the flags:
#   sedx_fuzz_instrumented  sanitizers + the engine's coverage instrumentation (the fuzzers and the code they call)
#   sedx_fuzz_sanitized     sanitizers only (programs that don't link an engine: the seed writer, property tests)
# -------------------------------------------------------

SET(SEDX_FUZZ_ENGINE "auto" CACHE STRING "Fuzzing engine: auto, libfuzzer, afl, standalone or oss-fuzz")
SET_PROPERTY(CACHE SEDX_FUZZ_ENGINE PROPERTY STRINGS auto libfuzzer afl standalone oss-fuzz)
SET(SEDX_FUZZ_SANITIZERS "auto" CACHE STRING "Sanitizers: auto, none, or a list such as address;undefined")
SET(SEDX_FUZZ_TIMEOUT "10" CACHE STRING "libFuzzer -timeout: seconds one input may take before it counts as a hang")
SET(SEDX_FUZZ_RSS_LIMIT_MB "2560" CACHE STRING "libFuzzer -rss_limit_mb: memory a run may use before it counts as a failure")
SET(SEDX_FUZZ_MALLOC_LIMIT_MB "1024" CACHE STRING "libFuzzer -malloc_limit_mb: largest single allocation allowed")

# -------------------------------------------------------
# Compiler family
# -------------------------------------------------------

SET(_sedx_cl FALSE)			# cl.exe
SET(_sedx_clang FALSE)		# LLVM clang / clang++ with the GNU command line (also afl-clang-fast)
SET(_sedx_appleclang FALSE)	# Apple's Clang: sanitizers, but no libFuzzer runtime
SET(_sedx_gcc FALSE)
IF(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	SET(_sedx_cl TRUE)
ELSEIF(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
	SET(_sedx_appleclang TRUE)
ELSEIF(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	IF(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
		MESSAGE(FATAL_ERROR "clang-cl isn't supported for fuzz builds: CMake links its targets with link.exe, which doesn't add "
			"the libFuzzer and sanitizer runtimes. Use clang++ (-DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++) or cl.exe.")
	ENDIF()
	SET(_sedx_clang TRUE)
ELSEIF(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	SET(_sedx_gcc TRUE)
ENDIF()

# -------------------------------------------------------
# Engine
# -------------------------------------------------------

SET(SEDX_FUZZ_ENGINE_RESOLVED "${SEDX_FUZZ_ENGINE}")
IF(SEDX_FUZZ_ENGINE STREQUAL "auto")
	IF(DEFINED ENV{LIB_FUZZING_ENGINE})
		SET(SEDX_FUZZ_ENGINE_RESOLVED oss-fuzz)
	ELSEIF(_sedx_cl OR _sedx_clang)
		SET(SEDX_FUZZ_ENGINE_RESOLVED libfuzzer)
	ELSE()
		SET(SEDX_FUZZ_ENGINE_RESOLVED standalone)
	ENDIF()
ENDIF()

IF(SEDX_FUZZ_ENGINE_RESOLVED STREQUAL "libfuzzer")
	IF(NOT (_sedx_cl OR _sedx_clang))
		MESSAGE(FATAL_ERROR "libFuzzer needs LLVM Clang or MSVC (${CMAKE_CXX_COMPILER_ID} found). On macOS use Homebrew's llvm: "
			"Apple's Clang ships no libFuzzer runtime. GCC builds can still replay inputs with -DSEDX_FUZZ_ENGINE=standalone.")
	ENDIF()
ELSEIF(SEDX_FUZZ_ENGINE_RESOLVED STREQUAL "afl")
	IF(NOT _sedx_clang)
		MESSAGE(FATAL_ERROR "AFL++ builds use its Clang wrappers: CC=afl-clang-fast CXX=afl-clang-fast++ (or afl-clang-lto).")
	ENDIF()
ELSEIF(SEDX_FUZZ_ENGINE_RESOLVED STREQUAL "standalone")
	IF(_sedx_cl)
		MESSAGE(FATAL_ERROR "MSVC has libFuzzer: use -DSEDX_FUZZ_ENGINE=libfuzzer (it replays files given on the command line too).")
	ENDIF()
ELSEIF(SEDX_FUZZ_ENGINE_RESOLVED STREQUAL "oss-fuzz")
	IF(NOT DEFINED ENV{LIB_FUZZING_ENGINE})
		MESSAGE(FATAL_ERROR "SEDX_FUZZ_ENGINE=oss-fuzz expects the OSS-Fuzz build environment (LIB_FUZZING_ENGINE, CFLAGS, CXXFLAGS).")
	ENDIF()
ELSE()
	MESSAGE(FATAL_ERROR "Unknown SEDX_FUZZ_ENGINE '${SEDX_FUZZ_ENGINE}': use auto, libfuzzer, afl, standalone or oss-fuzz.")
ENDIF()

# -------------------------------------------------------
# Sanitizers
# -------------------------------------------------------

SET(SEDX_FUZZ_SANITIZERS_RESOLVED "${SEDX_FUZZ_SANITIZERS}")
IF(SEDX_FUZZ_SANITIZERS STREQUAL "auto")
	IF(SEDX_FUZZ_ENGINE_RESOLVED STREQUAL "oss-fuzz")
		SET(SEDX_FUZZ_SANITIZERS_RESOLVED "")		# CFLAGS / CXXFLAGS already carry the run's sanitizer
	ELSEIF(_sedx_cl)
		SET(SEDX_FUZZ_SANITIZERS_RESOLVED "address")	# MSVC has no UndefinedBehaviorSanitizer
	ELSEIF(MINGW AND _sedx_gcc)
		SET(SEDX_FUZZ_SANITIZERS_RESOLVED "")		# MinGW GCC ships no sanitizer runtimes
	ELSEIF(_sedx_clang OR _sedx_appleclang OR _sedx_gcc)
		SET(SEDX_FUZZ_SANITIZERS_RESOLVED "address;undefined")
	ELSE()
		SET(SEDX_FUZZ_SANITIZERS_RESOLVED "")
	ENDIF()
ELSEIF(SEDX_FUZZ_SANITIZERS STREQUAL "none")
	SET(SEDX_FUZZ_SANITIZERS_RESOLVED "")
ENDIF()

IF(WIN32 AND _sedx_clang AND NOT MINGW)
	# LLVM's libFuzzer for Windows is built against the release static C runtime: /MT, iterator debugging off and no
	# STL sanitizer annotations. Everything linked with it has to match (the linker refuses mixes), Debug builds too.
	SET(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
	ADD_COMPILE_DEFINITIONS(_ITERATOR_DEBUG_LEVEL=0 _DISABLE_STRING_ANNOTATION _DISABLE_VECTOR_ANNOTATION)
ENDIF()

IF(_sedx_cl)
	LIST(REMOVE_ITEM SEDX_FUZZ_SANITIZERS_RESOLVED "undefined")
	# AddressSanitizer refuses the run-time checks Debug builds turn on
	FOREACH(lang C CXX)
		STRING(REPLACE "/RTC1" "" CMAKE_${lang}_FLAGS_DEBUG "${CMAKE_${lang}_FLAGS_DEBUG}")
	ENDFOREACH()
ENDIF()

ADD_LIBRARY(sedx_fuzz_sanitized INTERFACE)
ADD_LIBRARY(sedx_fuzz_instrumented INTERFACE)
TARGET_LINK_LIBRARIES(sedx_fuzz_instrumented INTERFACE sedx_fuzz_sanitized)
# The macro every fuzzing build defines (OSS-Fuzz sets it in CFLAGS); code may use it to relax checks fuzzers can't pass
TARGET_COMPILE_DEFINITIONS(sedx_fuzz_instrumented INTERFACE FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION)

SET(SEDX_FUZZ_SANITIZER_COMPILE_OPTIONS "")	# Also what third-party libraries linked into sanitized programs compile with
IF(SEDX_FUZZ_SANITIZERS_RESOLVED)
	IF(_sedx_cl)
		SET(SEDX_FUZZ_SANITIZER_COMPILE_OPTIONS /fsanitize=address /Zi)
		TARGET_COMPILE_OPTIONS(sedx_fuzz_sanitized INTERFACE ${SEDX_FUZZ_SANITIZER_COMPILE_OPTIONS})
		TARGET_LINK_OPTIONS(sedx_fuzz_sanitized INTERFACE /DEBUG /INCREMENTAL:NO)
	ELSE()
		LIST(JOIN SEDX_FUZZ_SANITIZERS_RESOLVED "," _sedx_list)
		SET(_sedx_flags -fsanitize=${_sedx_list} -fno-omit-frame-pointer -g)
		IF("undefined" IN_LIST SEDX_FUZZ_SANITIZERS_RESOLVED)
			# Undefined behaviour stops the run instead of printing and going on, so the fuzzer keeps the input
			LIST(APPEND _sedx_flags -fno-sanitize-recover=undefined)
			IF(WIN32)
				LIST(APPEND _sedx_flags -fno-sanitize=vptr)		# Not available on Windows targets
			ENDIF()
		ENDIF()
		SET(SEDX_FUZZ_SANITIZER_COMPILE_OPTIONS ${_sedx_flags})
		TARGET_COMPILE_OPTIONS(sedx_fuzz_sanitized INTERFACE ${_sedx_flags})
		TARGET_LINK_OPTIONS(sedx_fuzz_sanitized INTERFACE ${_sedx_flags})
	ENDIF()
ENDIF()

# Coverage instrumentation for the code under test, and what links the engine into a harness
ADD_LIBRARY(sedx_fuzz_engine INTERFACE)
IF(SEDX_FUZZ_ENGINE_RESOLVED STREQUAL "libfuzzer")
	IF(_sedx_cl)
		# /fsanitize=fuzzer adds the coverage instrumentation and names libFuzzer (with its main) as a default library
		TARGET_COMPILE_OPTIONS(sedx_fuzz_instrumented INTERFACE /fsanitize=fuzzer)
		TARGET_LINK_OPTIONS(sedx_fuzz_engine INTERFACE /DEBUG /INCREMENTAL:NO)
	ELSE()
		TARGET_COMPILE_OPTIONS(sedx_fuzz_instrumented INTERFACE -fsanitize=fuzzer-no-link)
		TARGET_LINK_OPTIONS(sedx_fuzz_engine INTERFACE -fsanitize=fuzzer)
	ENDIF()
ELSEIF(SEDX_FUZZ_ENGINE_RESOLVED STREQUAL "afl")
	# afl-clang-fast instruments every compile; -fsanitize=fuzzer at link swaps libFuzzer for AFL++'s driver
	TARGET_LINK_OPTIONS(sedx_fuzz_engine INTERFACE -fsanitize=fuzzer)
ELSEIF(SEDX_FUZZ_ENGINE_RESOLVED STREQUAL "oss-fuzz")
	SEPARATE_ARGUMENTS(_sedx_engine NATIVE_COMMAND "$ENV{LIB_FUZZING_ENGINE}")
	TARGET_LINK_OPTIONS(sedx_fuzz_engine INTERFACE ${_sedx_engine})
	IF(NOT "$ENV{LIB_FUZZING_ENGINE}" MATCHES "^-")
		# A library path (AFL++, honggfuzz builds): the driver is C++, so link it as a library
		TARGET_LINK_LIBRARIES(sedx_fuzz_engine INTERFACE ${_sedx_engine})
	ENDIF()
ELSE()
	# LLVM's StandaloneFuzzTargetMain.c: runs each file named on the command line through the harness, no fuzzing
	TARGET_SOURCES(sedx_fuzz_engine INTERFACE "${CMAKE_CURRENT_LIST_DIR}/../third_party/llvm/StandaloneFuzzTargetMain.c")
ENDIF()

# -------------------------------------------------------
# Sanitizer runtimes (and a DLL build of zlib) on Windows: copied next to the programs so CTest and the build can run them
# -------------------------------------------------------

SET(SEDX_FUZZ_RUNTIME_DLLS "")
IF(WIN32 AND SEDX_FUZZ_SANITIZERS_RESOLVED)
	GET_FILENAME_COMPONENT(_sedx_compiler_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
	SET(_sedx_dll_dirs "${_sedx_compiler_dir}")
	IF(_sedx_clang)
		EXECUTE_PROCESS(COMMAND "${CMAKE_CXX_COMPILER}" -print-resource-dir OUTPUT_VARIABLE _sedx_resource OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
		IF(_sedx_resource)
			FILE(TO_CMAKE_PATH "${_sedx_resource}" _sedx_resource)
			LIST(APPEND _sedx_dll_dirs "${_sedx_resource}/lib/windows")
		ENDIF()
	ENDIF()
	FOREACH(dir IN LISTS _sedx_dll_dirs)
		FILE(GLOB _sedx_found "${dir}/*clang_rt.asan_dynamic-*.dll" "${dir}/*clang_rt.asan_dbg_dynamic-*.dll" "${dir}/*clang_rt.ubsan_dynamic-*.dll")
		LIST(APPEND SEDX_FUZZ_RUNTIME_DLLS ${_sedx_found})
	ENDFOREACH()
	LIST(REMOVE_DUPLICATES SEDX_FUZZ_RUNTIME_DLLS)
ENDIF()

FUNCTION(sedx_fuzz_copy_runtime target)
	FOREACH(dll IN LISTS SEDX_FUZZ_RUNTIME_DLLS)
		ADD_CUSTOM_COMMAND(TARGET ${target} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${dll}" "$<TARGET_FILE_DIR:${target}>"
			VERBATIM)
	ENDFOREACH()
ENDFUNCTION()

# -------------------------------------------------------
# sedx_add_fuzzer(<name> <source> [DICT <file>] [SEEDS <folder>] [AFL_DICT <file>])
#   DICT      dictionaries/<file>, keywords of the format
#   SEEDS     seeds/<folder> (committed) and generated-seeds/<folder> (written at build time)
#   AFL_DICT  an AFL++ dictionary merged in when SEDX_FUZZ_AFL_DICTIONARIES points at AFL++'s dictionaries folder
# Registers <name>.corpus (replays every seed; must pass on every platform) and, when SEDX_FUZZ_SMOKE_SECONDS > 0 and
# the engine fuzzes, <name>.smoke (fuzzes for that long, label fuzz-smoke).
# -------------------------------------------------------

SET(SEDX_FUZZ_SCRIPTS "${CMAKE_CURRENT_LIST_DIR}")
SET_PROPERTY(GLOBAL PROPERTY SEDX_FUZZERS "")

FUNCTION(sedx_add_fuzzer name source)
	CMAKE_PARSE_ARGUMENTS(ARG "" "DICT;SEEDS;AFL_DICT" "" ${ARGN})

	ADD_EXECUTABLE(${name} ${source})
	TARGET_LINK_LIBRARIES(${name} PRIVATE sedx_fuzz_support sedx_fuzz_core sedx_fuzz_instrumented sedx_fuzz_engine)
	sedx_fuzz_copy_runtime(${name})
	ADD_DEPENDENCIES(${name} sedx_fuzz_seeds)

	# Dictionary: ours, plus the AFL++ one when available
	SET(dict "")
	IF(ARG_DICT)
		SET(dict "${CMAKE_BINARY_DIR}/dictionaries/${name}.dict")
		SET(source_dict "${CMAKE_CURRENT_SOURCE_DIR}/dictionaries/${ARG_DICT}")
		FILE(READ "${source_dict}" content)
		SET_PROPERTY(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${source_dict}")
		IF(ARG_AFL_DICT AND SEDX_FUZZ_AFL_DICTIONARIES AND EXISTS "${SEDX_FUZZ_AFL_DICTIONARIES}/${ARG_AFL_DICT}")
			FILE(READ "${SEDX_FUZZ_AFL_DICTIONARIES}/${ARG_AFL_DICT}" afl)
			STRING(APPEND content "\n# AFL++ ${ARG_AFL_DICT}\n${afl}")
		ENDIF()
		FILE(WRITE "${dict}" "${content}")
	ENDIF()

	SET(seed_dirs "")
	IF(ARG_SEEDS)
		IF(EXISTS "${SEDX_FUZZ_SEEDS}/${ARG_SEEDS}")
			LIST(APPEND seed_dirs "${SEDX_FUZZ_SEEDS}/${ARG_SEEDS}")
		ENDIF()
		LIST(APPEND seed_dirs "${SEDX_FUZZ_GENERATED_SEEDS}/${ARG_SEEDS}")
	ENDIF()
	LIST(JOIN seed_dirs "|" seed_arg)

	ADD_TEST(NAME ${name}.corpus
		COMMAND "${CMAKE_COMMAND}" -DFUZZER=$<TARGET_FILE:${name}> -DENGINE=${SEDX_FUZZ_ENGINE_RESOLVED} -DMODE=replay
			"-DSEEDS=${seed_arg}" -DTIMEOUT=${SEDX_FUZZ_TIMEOUT} -DRSS_LIMIT_MB=${SEDX_FUZZ_RSS_LIMIT_MB}
			-DMALLOC_LIMIT_MB=${SEDX_FUZZ_MALLOC_LIMIT_MB} -P "${SEDX_FUZZ_SCRIPTS}/run_fuzzer.cmake")
	SET_TESTS_PROPERTIES(${name}.corpus PROPERTIES LABELS "fuzz;fuzz-corpus")

	IF(SEDX_FUZZ_SMOKE_SECONDS GREATER 0 AND (SEDX_FUZZ_ENGINE_RESOLVED STREQUAL "libfuzzer" OR SEDX_FUZZ_ENGINE_RESOLVED STREQUAL "oss-fuzz"))
		MATH(EXPR test_timeout "${SEDX_FUZZ_SMOKE_SECONDS} + 120")
		ADD_TEST(NAME ${name}.smoke
			COMMAND "${CMAKE_COMMAND}" -DFUZZER=$<TARGET_FILE:${name}> -DENGINE=${SEDX_FUZZ_ENGINE_RESOLVED} -DMODE=fuzz
				"-DSEEDS=${seed_arg}" -DDICT=${dict} -DSECONDS=${SEDX_FUZZ_SMOKE_SECONDS} -DTIMEOUT=${SEDX_FUZZ_TIMEOUT}
				-DRSS_LIMIT_MB=${SEDX_FUZZ_RSS_LIMIT_MB} -DMALLOC_LIMIT_MB=${SEDX_FUZZ_MALLOC_LIMIT_MB}
				-DWORK=${CMAKE_BINARY_DIR}/fuzz-work/${name} -P "${SEDX_FUZZ_SCRIPTS}/run_fuzzer.cmake")
		SET_TESTS_PROPERTIES(${name}.smoke PROPERTIES LABELS "fuzz;fuzz-smoke" TIMEOUT ${test_timeout})
	ENDIF()

	SET_PROPERTY(GLOBAL APPEND PROPERTY SEDX_FUZZERS ${name})
	SET_PROPERTY(GLOBAL PROPERTY SEDX_FUZZER_${name}_DICT "${dict}")
	SET_PROPERTY(GLOBAL PROPERTY SEDX_FUZZER_${name}_SEEDS "${seed_arg}")
ENDFUNCTION()

# -------------------------------------------------------
# sedx_add_package_target(): sedx_fuzz_package lays the fuzzers out the way OSS-Fuzz and ClusterFuzzLite expect
# -------------------------------------------------------

FUNCTION(sedx_add_package_target)
	GET_PROPERTY(fuzzers GLOBAL PROPERTY SEDX_FUZZERS)
	SET(manifest "SET(FUZZERS \"${fuzzers}\")\n")
	FOREACH(name IN LISTS fuzzers)
		GET_PROPERTY(dict GLOBAL PROPERTY SEDX_FUZZER_${name}_DICT)
		GET_PROPERTY(seeds GLOBAL PROPERTY SEDX_FUZZER_${name}_SEEDS)
		STRING(APPEND manifest "SET(${name}_EXE \"$<TARGET_FILE:${name}>\")\n")
		STRING(APPEND manifest "SET(${name}_DICT \"${dict}\")\n")
		STRING(APPEND manifest "SET(${name}_SEEDS \"${seeds}\")\n")
	ENDFOREACH()
	STRING(APPEND manifest "SET(RSS_LIMIT_MB ${SEDX_FUZZ_RSS_LIMIT_MB})\nSET(MALLOC_LIMIT_MB ${SEDX_FUZZ_MALLOC_LIMIT_MB})\n")
	FILE(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/fuzzers-$<CONFIG>.cmake" CONTENT "${manifest}")

	SET(out "${SEDX_FUZZ_OUT_DIR}")
	IF(NOT out)
		SET(out "${CMAKE_BINARY_DIR}/out")
	ENDIF()
	ADD_CUSTOM_TARGET(sedx_fuzz_package
		COMMAND "${CMAKE_COMMAND}" -DMANIFEST=${CMAKE_BINARY_DIR}/fuzzers-$<CONFIG>.cmake -DOUT=${out}
			-DWORK=${CMAKE_BINARY_DIR}/package-work -P "${SEDX_FUZZ_SCRIPTS}/package_fuzzers.cmake"
		DEPENDS ${fuzzers} sedx_fuzz_seeds
		COMMENT "Packaging fuzzers into ${out}"
		VERBATIM)
ENDFUNCTION()

FUNCTION(sedx_fuzz_summary)
	GET_PROPERTY(fuzzers GLOBAL PROPERTY SEDX_FUZZERS)
	LIST(LENGTH fuzzers count)
	IF(SEDX_FUZZ_SANITIZERS_RESOLVED)
		SET(sanitizers "${SEDX_FUZZ_SANITIZERS_RESOLVED}")
	ELSEIF(SEDX_FUZZ_ENGINE_RESOLVED STREQUAL "oss-fuzz")
		SET(sanitizers "from CFLAGS / CXXFLAGS")
	ELSE()
		SET(sanitizers "none")
	ENDIF()
	MESSAGE(STATUS "=================================================")
	MESSAGE(STATUS "Scenery Editor X fuzzing: ${count} harnesses, engine ${SEDX_FUZZ_ENGINE_RESOLVED}, sanitizers ${sanitizers}")
	MESSAGE(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
	IF(SEDX_FUZZ_RUNTIME_DLLS)
		MESSAGE(STATUS "DLLs copied next to the programs: ${SEDX_FUZZ_RUNTIME_DLLS}")
	ENDIF()
	MESSAGE(STATUS "=================================================")
ENDFUNCTION()

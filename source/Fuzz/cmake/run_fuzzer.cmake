# -------------------------------------------------------
# Scenery Editor X - Fuzzing
# -------------------------------------------------------
# Copyright (c) 2026 Thomas Ray
# Copyright (c) 2026 Coalition of Freeware Developers
# -------------------------------------------------------
# run_fuzzer.cmake
# -------------------------------------------------------
# What the <fuzzer>.corpus and <fuzzer>.smoke tests run (cmake -P, so it behaves the same on every platform).
#
#   -DFUZZER=<exe> -DENGINE=<libfuzzer|afl|standalone|oss-fuzz> -DSEEDS=<dir|dir...>
#   -DMODE=replay   every seed file goes through the harness once (batches of files on the command line)
#   -DMODE=fuzz     libFuzzer fuzzes for -DSECONDS, writing new inputs to <WORK>/corpus and crashes to <WORK>/artifacts
#   -DTIMEOUT / -DRSS_LIMIT_MB / -DMALLOC_LIMIT_MB / -DDICT pass through to libFuzzer
# -------------------------------------------------------

IF(NOT FUZZER OR NOT EXISTS "${FUZZER}")
	MESSAGE(FATAL_ERROR "Fuzzer '${FUZZER}' not found (build it first)")
ENDIF()
GET_FILENAME_COMPONENT(name "${FUZZER}" NAME_WE)

STRING(REPLACE "|" ";" seed_dirs "${SEEDS}")
SET(existing_dirs "")
SET(files "")
FOREACH(dir IN LISTS seed_dirs)
	IF(IS_DIRECTORY "${dir}")
		LIST(APPEND existing_dirs "${dir}")
		FILE(GLOB_RECURSE found LIST_DIRECTORIES FALSE "${dir}/*")
		LIST(FILTER found EXCLUDE REGEX "/\\.stamp$")
		LIST(APPEND files ${found})
	ENDIF()
ENDFOREACH()
LIST(SORT files)

# libFuzzer-style limits: a slow or memory-hungry input fails the run like a crash does
SET(limits "")
IF(NOT ENGINE STREQUAL "standalone" AND NOT ENGINE STREQUAL "afl")
	IF(TIMEOUT)
		LIST(APPEND limits "-timeout=${TIMEOUT}")
	ENDIF()
	IF(RSS_LIMIT_MB)
		LIST(APPEND limits "-rss_limit_mb=${RSS_LIMIT_MB}")
	ENDIF()
	IF(MALLOC_LIMIT_MB)
		LIST(APPEND limits "-malloc_limit_mb=${MALLOC_LIMIT_MB}")
	ENDIF()
ENDIF()

IF(MODE STREQUAL "replay")
	LIST(LENGTH files count)
	IF(count EQUAL 0)
		MESSAGE(FATAL_ERROR "${name}: no seed files in ${SEEDS}")
	ENDIF()

	# Batches keep each command line well under Windows' 32k character limit
	SET(batch "")
	SET(batch_size 0)
	SET(done 0)
	FOREACH(file IN LISTS files ITEMS "")
		IF(NOT file STREQUAL "")
			LIST(APPEND batch "${file}")
			MATH(EXPR batch_size "${batch_size} + 1")
		ENDIF()
		IF(batch_size GREATER_EQUAL 48 OR (file STREQUAL "" AND batch_size GREATER 0))
			EXECUTE_PROCESS(COMMAND "${FUZZER}" ${limits} ${batch} RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE output)
			IF(NOT result EQUAL 0)
				MESSAGE("${output}")
				MESSAGE(FATAL_ERROR "${name}: an input failed (exit ${result}). The last 'Running:' line above names it.")
			ENDIF()
			MATH(EXPR done "${done} + ${batch_size}")
			SET(batch "")
			SET(batch_size 0)
		ENDIF()
	ENDFOREACH()
	MESSAGE(STATUS "${name}: ${done} inputs replayed without a failure")

ELSEIF(MODE STREQUAL "fuzz")
	IF(NOT SECONDS)
		SET(SECONDS 60)
	ENDIF()
	FILE(MAKE_DIRECTORY "${WORK}/corpus" "${WORK}/artifacts")
	SET(options "-max_total_time=${SECONDS}" "-print_final_stats=1" "-artifact_prefix=${WORK}/artifacts/")
	IF(DICT AND EXISTS "${DICT}")
		LIST(APPEND options "-dict=${DICT}")
	ENDIF()
	# New inputs go to the first folder, so the seed folders stay as they are
	EXECUTE_PROCESS(COMMAND "${FUZZER}" ${limits} ${options} "${WORK}/corpus" ${existing_dirs}
		RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE output)
	STRING(LENGTH "${output}" length)
	IF(length GREATER 6000)
		MATH(EXPR start "${length} - 6000")
		STRING(SUBSTRING "${output}" ${start} -1 output)
	ENDIF()
	MESSAGE("${output}")
	IF(NOT result EQUAL 0)
		FILE(GLOB artifacts "${WORK}/artifacts/*")
		MESSAGE(FATAL_ERROR "${name}: fuzzing failed (exit ${result}). Reproducer(s): ${artifacts}")
	ENDIF()
ELSE()
	MESSAGE(FATAL_ERROR "MODE must be replay or fuzz")
ENDIF()

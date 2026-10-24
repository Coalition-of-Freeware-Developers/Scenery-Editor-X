# -------------------------------------------------------
# Scenery Editor X - Fuzzing
# -------------------------------------------------------
# Copyright (c) 2026 Thomas Ray
# Copyright (c) 2026 Coalition of Freeware Developers
# -------------------------------------------------------
# package_fuzzers.cmake
# -------------------------------------------------------
# Lays out every fuzzer the way OSS-Fuzz / ClusterFuzzLite read $OUT:
#   <fuzzer>                    the program (plus any sanitizer runtime DLLs next to it on Windows)
#   <fuzzer>_seed_corpus.zip    every seed, named by its SHA-1 (duplicates collapse)
#   <fuzzer>.dict               the dictionary
#   <fuzzer>.options            libFuzzer limits
#
#   cmake -DMANIFEST=<fuzzers-<config>.cmake> -DOUT=<dir> -DWORK=<scratch dir> -P package_fuzzers.cmake
# -------------------------------------------------------

INCLUDE("${MANIFEST}")
FILE(MAKE_DIRECTORY "${OUT}")

FOREACH(name IN LISTS FUZZERS)
	SET(exe "${${name}_EXE}")
	FILE(COPY "${exe}" DESTINATION "${OUT}")
	IF(WIN32)
		GET_FILENAME_COMPONENT(exe_dir "${exe}" DIRECTORY)
		FILE(GLOB dlls "${exe_dir}/*.dll")
		IF(dlls)
			FILE(COPY ${dlls} DESTINATION "${OUT}")
		ENDIF()
	ENDIF()

	# Seeds, flattened and deduplicated by content
	SET(stage "${WORK}/${name}")
	FILE(REMOVE_RECURSE "${stage}")
	FILE(MAKE_DIRECTORY "${stage}")
	STRING(REPLACE "|" ";" seed_dirs "${${name}_SEEDS}")
	FOREACH(dir IN LISTS seed_dirs)
		IF(IS_DIRECTORY "${dir}")
			FILE(GLOB_RECURSE files LIST_DIRECTORIES FALSE "${dir}/*")
			FOREACH(file IN LISTS files)
				IF(NOT file MATCHES "/\\.stamp$")
					FILE(SHA1 "${file}" hash)
					FILE(COPY_FILE "${file}" "${stage}/${hash}")
				ENDIF()
			ENDFOREACH()
		ENDIF()
	ENDFOREACH()
	FILE(GLOB staged RELATIVE "${stage}" "${stage}/*")
	FILE(REMOVE "${OUT}/${name}_seed_corpus.zip")
	IF(staged)
		EXECUTE_PROCESS(COMMAND "${CMAKE_COMMAND}" -E tar cf "${OUT}/${name}_seed_corpus.zip" --format=zip ${staged}
			WORKING_DIRECTORY "${stage}" RESULT_VARIABLE result)
		IF(NOT result EQUAL 0)
			MESSAGE(FATAL_ERROR "Couldn't zip the seeds of ${name}")
		ENDIF()
	ENDIF()

	IF(${name}_DICT AND EXISTS "${${name}_DICT}")
		FILE(COPY_FILE "${${name}_DICT}" "${OUT}/${name}.dict")
	ENDIF()

	FILE(WRITE "${OUT}/${name}.options" "[libfuzzer]\nrss_limit_mb = ${RSS_LIMIT_MB}\nmalloc_limit_mb = ${MALLOC_LIMIT_MB}\n")
	IF(${name}_DICT AND EXISTS "${${name}_DICT}")
		FILE(APPEND "${OUT}/${name}.options" "dict = ${name}.dict\n")
	ENDIF()

	LIST(LENGTH staged seed_count)
	MESSAGE(STATUS "${name}: ${seed_count} seeds")
ENDFOREACH()

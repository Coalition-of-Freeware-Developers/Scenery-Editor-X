# -------------------------------------------------------
# Scenery Editor X - Fuzzing
# -------------------------------------------------------
# Copyright (c) 2026 Thomas Ray
# Copyright (c) 2026 Coalition of Freeware Developers
# -------------------------------------------------------
# CMakeLists.txt for the fuzz targets
# -------------------------------------------------------
# A project of its own: the parsers it fuzzes only need the standard library and nlohmann/json (zlib for the zip
# reader), so it configures without the Vulkan SDK, vcpkg or the editor's dependencies, on Windows, Linux and macOS.
#
#   cmake -S source/Fuzz -B build-fuzz [-DSEDX_FUZZ_ENGINE=...] [-DSEDX_FUZZ_SANITIZERS=...]
#   cmake --build build-fuzz
#   ctest --test-dir build-fuzz          (replays every seed corpus through every harness)
#
# Engines (SEDX_FUZZ_ENGINE): libfuzzer (Clang, MSVC), afl (AFL++ afl-clang-fast), standalone (any compiler: replays
# inputs, no fuzzing), oss-fuzz (ClusterFuzzLite / OSS-Fuzz: flags and engine come from CFLAGS / CXXFLAGS /
# LIB_FUZZING_ENGINE). See README.md.
# -------------------------------------------------------

CMAKE_MINIMUM_REQUIRED(VERSION 3.20)
PROJECT(SceneryEditorXFuzz VERSION 1.0.0 LANGUAGES C CXX)

SET(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)
SET(CMAKE_C_STANDARD 11)

GET_FILENAME_COMPONENT(SEDX_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
SET(SEDX_SOURCE "${SEDX_ROOT}/source")

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
INCLUDE(SedxFuzz)

# Optimized with debug info by default; OSS-Fuzz builds keep the optimization CFLAGS chose
IF(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES AND NOT SEDX_FUZZ_ENGINE_RESOLVED STREQUAL "oss-fuzz")
	SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
ENDIF()

OPTION(SEDX_FUZZ_PROPERTIES "Build the RapidCheck round-trip property tests" ON)
SET(SEDX_FUZZ_SMOKE_SECONDS "0" CACHE STRING "Seconds each harness fuzzes as a CTest (label fuzz-smoke); 0 registers none")
SET(SEDX_FUZZ_AFL_DICTIONARIES "" CACHE PATH "AFL++ dictionaries folder: json.dict / xml.dict are merged into the .edX, Gateway and XML dictionaries")
SET(SEDX_FUZZ_OUT_DIR "" CACHE PATH "Where the sedx_fuzz_package target lays out the fuzzers, seed corpora, dictionaries and options (OSS-Fuzz $OUT layout)")

# -------------------------------------------------------
# Dependencies: the repository's submodules when they're checked out, otherwise pinned downloads of the same projects
# -------------------------------------------------------

INCLUDE(FetchContent)

IF(EXISTS "${SEDX_ROOT}/dependency/json/include/nlohmann/json.hpp")
	ADD_LIBRARY(sedx_fuzz_json INTERFACE)
	TARGET_INCLUDE_DIRECTORIES(sedx_fuzz_json SYSTEM INTERFACE "${SEDX_ROOT}/dependency/json/include")
ELSE()
	MESSAGE(STATUS "dependency/json isn't checked out: downloading nlohmann/json (the submodule's commit)")
	FETCHCONTENT_DECLARE(nlohmann_json
		URL https://github.com/nlohmann/json/archive/3946872265598aed5a7aea68cad4d9d1f168bd4b.tar.gz
		DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
	FETCHCONTENT_MAKEAVAILABLE(nlohmann_json)
	ADD_LIBRARY(sedx_fuzz_json INTERFACE)
	TARGET_LINK_LIBRARIES(sedx_fuzz_json INTERFACE nlohmann_json::nlohmann_json)
ENDIF()

FIND_PACKAGE(ZLIB QUIET)
IF(NOT ZLIB_FOUND)
	MESSAGE(STATUS "zlib not found: fuzz_zip (the Scenery Gateway zip reader) is skipped. Point CMAKE_PREFIX_PATH at one "
		"(on Windows the editor build's vcpkg_installed/x64-windows works)")
ELSEIF(WIN32)
	# A DLL build of zlib (vcpkg's) has to sit next to the programs too
	FOREACH(include_dir IN LISTS ZLIB_INCLUDE_DIRS)
		FILE(GLOB _sedx_zlib_dlls "${include_dir}/../bin/zlib*.dll" "${include_dir}/../bin/libz.dll")
		LIST(APPEND SEDX_FUZZ_RUNTIME_DLLS ${_sedx_zlib_dlls})
	ENDFOREACH()
ENDIF()

# -------------------------------------------------------
# The code under test, compiled twice: once instrumented for the fuzzers, once with only the sanitizers for the seed
# writer and the property tests (which don't link a fuzzing engine)
# -------------------------------------------------------

SET(SEDX_FUZZ_CORE_SOURCES
	${SEDX_SOURCE}/SceneryEditorX/filesystem/vfs.cpp
	${SEDX_SOURCE}/SceneryEditorX/xplane/xp_obj8.cpp
	${SEDX_SOURCE}/SceneryEditorX/xplane/xp_library.cpp
	${SEDX_SOURCE}/SceneryEditorX/xplane/xp_lights.cpp
	${SEDX_SOURCE}/SceneryEditorX/xplane/xp_apt.cpp
	${SEDX_SOURCE}/SceneryEditorX/xplane/xp_apt_geometry.cpp
	${SEDX_SOURCE}/SceneryEditorX/xplane/xp_airport_document.cpp
	${SEDX_SOURCE}/SceneryEditorX/xplane/xp_airport_index.cpp
	${SEDX_SOURCE}/SceneryEditorX/xplane/xp_gateway_types.cpp
	${SEDX_SOURCE}/SceneryEditorX/xplane/xp_dsf.cpp
	${SEDX_SOURCE}/SceneryEditorX/xplane/xp_assets.cpp
	${SEDX_SOURCE}/SceneryEditorX/xplane/xp_facade.cpp
	${SEDX_SOURCE}/SceneryEditorX/xplane/xp_facade_writer.cpp
	${SEDX_SOURCE}/SceneryEditorX/xplane/xp_facade_authoring.cpp
	${SEDX_SOURCE}/SceneryEditorX/xplane/xp_overlay.cpp
	${SEDX_SOURCE}/SceneryEditorX/xplane/xp_install_finder.cpp
	${SEDX_SOURCE}/SceneryEditorX/project/spline_edit.cpp
	${SEDX_SOURCE}/SceneryEditorX/project/facade_edit.cpp
	${SEDX_SOURCE}/SceneryEditorX/project/scenery_project.cpp
	${SEDX_SOURCE}/SceneryEditorX/project/project_edx.cpp
	${SEDX_SOURCE}/SceneryEditorX/project/project_sex.cpp
	${SEDX_SOURCE}/SceneryEditorX/project/project_wed.cpp
	${SEDX_SOURCE}/SceneryEditorX/project/xml_document.cpp
	${SEDX_SOURCE}/SceneryEditorX/project/project_io.cpp
	${SEDX_SOURCE}/SceneryEditorX/project/project_cache.cpp
	${SEDX_SOURCE}/SceneryEditorX/project/project_document.cpp
	${SEDX_SOURCE}/SceneryEditorX/project/project_dsf.cpp
	${SEDX_SOURCE}/SceneryEditorX/project/scenery_pack.cpp
)

FUNCTION(sedx_fuzz_add_core name flags)
	ADD_LIBRARY(${name} STATIC ${SEDX_FUZZ_CORE_SOURCES})
	TARGET_INCLUDE_DIRECTORIES(${name} PUBLIC ${SEDX_SOURCE})
	TARGET_LINK_LIBRARIES(${name} PUBLIC sedx_fuzz_json PRIVATE ${flags})
	IF(WIN32)
		# xp_install_finder reads the registry and the desktop folder
		TARGET_LINK_LIBRARIES(${name} PUBLIC advapi32 shell32)
	ENDIF()
	IF(MSVC)
		TARGET_COMPILE_OPTIONS(${name} PRIVATE /bigobj)
	ENDIF()
	IF(ZLIB_FOUND)
		TARGET_SOURCES(${name} PRIVATE ${SEDX_SOURCE}/SceneryEditorX/xplane/xp_zip.cpp)
		TARGET_LINK_LIBRARIES(${name} PUBLIC ZLIB::ZLIB)
		TARGET_COMPILE_DEFINITIONS(${name} PUBLIC SEDX_FUZZ_HAS_ZLIB)
	ENDIF()
ENDFUNCTION()

sedx_fuzz_add_core(sedx_fuzz_core sedx_fuzz_instrumented)
sedx_fuzz_add_core(sedx_check_core sedx_fuzz_sanitized)

# Helpers every harness shares (the temporary asset pack the overlay builders draw with)
ADD_LIBRARY(sedx_fuzz_support STATIC common/fuzz_assets.cpp)
TARGET_INCLUDE_DIRECTORIES(sedx_fuzz_support PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
TARGET_LINK_LIBRARIES(sedx_fuzz_support PUBLIC sedx_fuzz_core PRIVATE sedx_fuzz_instrumented)

# -------------------------------------------------------
# Seeds: text files committed under seeds/, plus binary and project files our own writers produce at build time
# -------------------------------------------------------

ADD_EXECUTABLE(sedx_fuzz_seed_writer tools/fuzz_seed_writer.cpp)
TARGET_LINK_LIBRARIES(sedx_fuzz_seed_writer PRIVATE sedx_check_core sedx_fuzz_sanitized)
sedx_fuzz_copy_runtime(sedx_fuzz_seed_writer)		# It runs during the build

SET(SEDX_FUZZ_SEEDS "${CMAKE_CURRENT_SOURCE_DIR}/seeds")
SET(SEDX_FUZZ_GENERATED_SEEDS "${CMAKE_CURRENT_BINARY_DIR}/generated-seeds")
ADD_CUSTOM_COMMAND(
	OUTPUT "${SEDX_FUZZ_GENERATED_SEEDS}/.stamp"
	COMMAND "${CMAKE_COMMAND}" -E rm -rf "${SEDX_FUZZ_GENERATED_SEEDS}"
	COMMAND $<TARGET_FILE:sedx_fuzz_seed_writer> "${SEDX_FUZZ_GENERATED_SEEDS}" "${SEDX_ROOT}"
	COMMAND "${CMAKE_COMMAND}" -DSEEDS=${SEDX_FUZZ_SEEDS} -DOUT=${SEDX_FUZZ_GENERATED_SEEDS} -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/make_zip_seeds.cmake"
	COMMAND "${CMAKE_COMMAND}" -E touch "${SEDX_FUZZ_GENERATED_SEEDS}/.stamp"
	DEPENDS sedx_fuzz_seed_writer "${CMAKE_CURRENT_SOURCE_DIR}/cmake/make_zip_seeds.cmake"
	COMMENT "Writing generated fuzz seeds"
	VERBATIM)
ADD_CUSTOM_TARGET(sedx_fuzz_seeds ALL DEPENDS "${SEDX_FUZZ_GENERATED_SEEDS}/.stamp")

# -------------------------------------------------------
# Harnesses
# -------------------------------------------------------

ENABLE_TESTING()

# X-Plane formats
sedx_add_fuzzer(fuzz_obj8 harnesses/fuzz_obj8.cpp DICT obj8.dict SEEDS obj8)
sedx_add_fuzzer(fuzz_apt_dat harnesses/fuzz_apt_dat.cpp DICT apt_dat.dict SEEDS apt_dat)
sedx_add_fuzzer(fuzz_dsf harnesses/fuzz_dsf.cpp DICT dsf.dict SEEDS dsf)
sedx_add_fuzzer(fuzz_library_txt harnesses/fuzz_library_txt.cpp DICT library_txt.dict SEEDS library_txt)
sedx_add_fuzzer(fuzz_scenery_packs harnesses/fuzz_scenery_packs.cpp DICT scenery_packs.dict SEEDS scenery_packs)
sedx_add_fuzzer(fuzz_lights harnesses/fuzz_lights.cpp DICT lights.dict SEEDS lights)
sedx_add_fuzzer(fuzz_polygon harnesses/fuzz_polygon.cpp DICT assets.dict SEEDS polygon)
sedx_add_fuzzer(fuzz_line harnesses/fuzz_line.cpp DICT assets.dict SEEDS line)
sedx_add_fuzzer(fuzz_object_string harnesses/fuzz_object_string.cpp DICT assets.dict SEEDS object_string)
sedx_add_fuzzer(fuzz_forest harnesses/fuzz_forest.cpp DICT assets.dict SEEDS forest)
sedx_add_fuzzer(fuzz_facade harnesses/fuzz_facade.cpp DICT facade.dict SEEDS facade)
sedx_add_fuzzer(fuzz_autogen harnesses/fuzz_autogen.cpp DICT assets.dict SEEDS autogen)
sedx_add_fuzzer(fuzz_road_network harnesses/fuzz_road_network.cpp DICT road_network.dict SEEDS road_network)
sedx_add_fuzzer(fuzz_gateway_json harnesses/fuzz_gateway_json.cpp DICT gateway_json.dict SEEDS gateway_json AFL_DICT json.dict)
sedx_add_fuzzer(fuzz_install_finder harnesses/fuzz_install_finder.cpp DICT install_finder.dict SEEDS install_finder)
sedx_add_fuzzer(fuzz_vfs_paths harnesses/fuzz_vfs_paths.cpp DICT vfs_paths.dict SEEDS vfs_paths)
IF(ZLIB_FOUND)
	sedx_add_fuzzer(fuzz_zip harnesses/fuzz_zip.cpp DICT zip.dict SEEDS zip)
ENDIF()

# Project files
sedx_add_fuzzer(fuzz_sex harnesses/fuzz_sex.cpp DICT sex.dict SEEDS sex)
sedx_add_fuzzer(fuzz_edx harnesses/fuzz_edx.cpp DICT edx.dict SEEDS edx AFL_DICT json.dict)
sedx_add_fuzzer(fuzz_wed harnesses/fuzz_wed.cpp DICT wed.dict SEEDS wed AFL_DICT xml.dict)
sedx_add_fuzzer(fuzz_xml harnesses/fuzz_xml.cpp DICT wed.dict SEEDS xml AFL_DICT xml.dict)
sedx_add_fuzzer(fuzz_coordinate harnesses/fuzz_coordinate.cpp SEEDS coordinate)

# -------------------------------------------------------
# Round-trip property tests (RapidCheck inside Catch2)
# -------------------------------------------------------

IF(SEDX_FUZZ_PROPERTIES)
	ADD_SUBDIRECTORY(properties)
ENDIF()

# -------------------------------------------------------
# OSS-Fuzz / ClusterFuzzLite layout: <fuzzer>, <fuzzer>_seed_corpus.zip, <fuzzer>.dict, <fuzzer>.options
# -------------------------------------------------------

sedx_add_package_target()

sedx_fuzz_summary()

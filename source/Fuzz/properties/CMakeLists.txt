# -------------------------------------------------------
# Scenery Editor X - Fuzzing
# -------------------------------------------------------
# Copyright (c) 2026 Thomas Ray
# Copyright (c) 2026 Coalition of Freeware Developers
# -------------------------------------------------------
# CMakeLists.txt for the round-trip property tests
# -------------------------------------------------------
# RapidCheck generates random models (airports, projects, DSF tiles, paths), runs them through a writer and the
# matching reader, and shrinks any failure to the smallest model that still fails. Catch2 runs the properties, so
# they register with CTest like the unit tests. Works with every compiler (no fuzzing engine involved).
# -------------------------------------------------------

# Catch2: the submodule when it's checked out, else its commit from GitHub
IF(EXISTS "${SEDX_ROOT}/dependency/Catch2/CMakeLists.txt")
	ADD_SUBDIRECTORY("${SEDX_ROOT}/dependency/Catch2" "${CMAKE_CURRENT_BINARY_DIR}/Catch2" EXCLUDE_FROM_ALL)
ELSE()
	MESSAGE(STATUS "dependency/Catch2 isn't checked out: downloading Catch2 (the submodule's commit)")
	FETCHCONTENT_DECLARE(Catch2
		URL https://github.com/catchorg/Catch2/archive/be2dfb45cc7862ef08f46b967bcda72c9b7a9c59.tar.gz
		DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
	FETCHCONTENT_MAKEAVAILABLE(Catch2)
ENDIF()

# RapidCheck has no releases: pinned to a commit
FETCHCONTENT_DECLARE(rapidcheck
	URL https://github.com/emil-e/rapidcheck/archive/2c3c4365aca21ef4e612768fdc95b1ce8b39a651.tar.gz
	DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FETCHCONTENT_MAKEAVAILABLE(rapidcheck)

# MSVC's AddressSanitizer refuses to link sanitized and plain STL code together, so the libraries get the same flags
FOREACH(dependency rapidcheck Catch2 Catch2WithMain)
	IF(TARGET ${dependency})
		TARGET_COMPILE_OPTIONS(${dependency} PRIVATE ${SEDX_FUZZ_SANITIZER_COMPILE_OPTIONS})	# (Options, not a target: they export themselves)
	ENDIF()
ENDFOREACH()

ADD_EXECUTABLE(SedxRoundTripProperties
	RoundTripProperties.cpp
)
TARGET_INCLUDE_DIRECTORIES(SedxRoundTripProperties PRIVATE "${rapidcheck_SOURCE_DIR}/extras/catch/include")
TARGET_LINK_LIBRARIES(SedxRoundTripProperties PRIVATE sedx_check_core rapidcheck Catch2::Catch2WithMain sedx_fuzz_sanitized)
IF(MSVC)
	TARGET_COMPILE_OPTIONS(SedxRoundTripProperties PRIVATE /bigobj /W4)
ELSE()
	TARGET_COMPILE_OPTIONS(SedxRoundTripProperties PRIVATE -Wall -Wextra)
ENDIF()
sedx_fuzz_copy_runtime(SedxRoundTripProperties)

INCLUDE("${Catch2_SOURCE_DIR}/extras/Catch.cmake" OPTIONAL RESULT_VARIABLE _sedx_catch_module)
IF(NOT _sedx_catch_module)
	INCLUDE("${SEDX_ROOT}/dependency/Catch2/extras/Catch.cmake")
ENDIF()
# RC_PARAMS=max_success=<n> raises the number of random models per property (default 100)
CATCH_DISCOVER_TESTS(SedxRoundTripProperties TEST_PREFIX "properties::" PROPERTIES LABELS "properties" DISCOVERY_MODE PRE_TEST)

# -------------------------------------------------------
# Scenery Editor X - Fuzzing
# -------------------------------------------------------
# Copyright (c) 2026 Thomas Ray
# Copyright (c) 2026 Coalition of Freeware Developers
# -------------------------------------------------------
# make_zip_seeds.cmake
# -------------------------------------------------------
# Zip archives for fuzz_zip, packed the way a Scenery Gateway download is (an apt.dat and an earth.wed.xml), from the
# committed seeds and the generated ones. CMake's own archiver writes them, so no zip tool is needed.
#
#   cmake -DSEEDS=<source/Fuzz/seeds> -DOUT=<generated-seeds> -P make_zip_seeds.cmake
# -------------------------------------------------------

SET(stage "${OUT}/zip-stage")
FILE(REMOVE_RECURSE "${stage}")
FILE(MAKE_DIRECTORY "${stage}/one" "${stage}/pack/Earth nav data" "${OUT}/zip")

FILE(GLOB apt "${SEEDS}/apt_dat/*")
LIST(GET apt 0 first_apt)
FILE(COPY_FILE "${first_apt}" "${stage}/one/apt.dat")

FILE(COPY_FILE "${first_apt}" "${stage}/pack/Earth nav data/apt.dat")
FILE(GLOB wed "${OUT}/wed/*")
IF(wed)
	LIST(GET wed 0 first_wed)
	FILE(COPY_FILE "${first_wed}" "${stage}/pack/earth.wed.xml")
ENDIF()

EXECUTE_PROCESS(COMMAND "${CMAKE_COMMAND}" -E tar cf "${OUT}/zip/single_file.zip" --format=zip apt.dat
	WORKING_DIRECTORY "${stage}/one" RESULT_VARIABLE r1)
EXECUTE_PROCESS(COMMAND "${CMAKE_COMMAND}" -E tar cf "${OUT}/zip/gateway_pack.zip" --format=zip .
	WORKING_DIRECTORY "${stage}/pack" RESULT_VARIABLE r2)
FILE(REMOVE_RECURSE "${stage}")
IF(NOT r1 EQUAL 0 OR NOT r2 EQUAL 0)
	MESSAGE(FATAL_ERROR "Couldn't write the zip seeds")
ENDIF()

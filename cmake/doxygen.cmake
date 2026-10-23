## This file is included conditionally by the top-level CMakeLists when
## SEDX_ENABLE_DOXYGEN is ON. Keep a defensive check here as well in case
## it is included directly elsewhere.
IF(NOT SEDX_ENABLE_DOXYGEN)
    MESSAGE(STATUS "Doxygen disabled (SEDX_ENABLE_DOXYGEN=OFF)")
    RETURN()
ENDIF()

# Find Doxygen package
FIND_PACKAGE(Doxygen)

IF(NOT DOXYGEN_FOUND)
    MESSAGE(FATAL_ERROR "Doxygen not found. Please install Doxygen to generate documentation.")
ENDIF()

# Set the path to the Doxyfile
SET(DOXYFILE_PATH ${CMAKE_SOURCE_DIR}/scripts/Doxyfile)

# Check if the Doxyfile exists
IF(NOT EXISTS ${DOXYFILE_PATH})
    MESSAGE(FATAL_ERROR "Doxyfile not found at ${DOXYFILE_PATH}")
ENDIF()

# Set the output directory for the documentation
SET(DOXYGEN_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/docs/doxygen")

# Add a custom target to generate the Doxygen documentation
ADD_CUSTOM_TARGET(doxygen
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_PATH}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM
)

# Ensure the output directory exists
FILE(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR})

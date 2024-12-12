if(BUILD_DOXYGEN)
    # Find Doxygen package
    find_package(Doxygen)

    if(NOT DOXYGEN_FOUND)
        message(FATAL_ERROR "Doxygen not found. Please install Doxygen to generate documentation.")
    endif()

    # Set the path to the Doxyfile
    set(DOXYFILE_PATH "${CMAKE_SOURCE_DIR}/scripts/Doxyfile")

    # Check if the Doxyfile exists
    if(NOT EXISTS ${DOXYFILE_PATH})
        message(FATAL_ERROR "Doxyfile not found at ${DOXYFILE_PATH}")
    endif()

    # Set the output directory for the documentation
    set(DOXYGEN_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/docs/doxygen")

    # Add a custom target to generate the Doxygen documentation
    add_custom_target(doxygen
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_PATH}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )

    # Ensure the output directory exists
    file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR})
endif()

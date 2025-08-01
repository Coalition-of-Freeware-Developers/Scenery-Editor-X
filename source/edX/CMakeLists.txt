# --------------------------------
# EDX File Format Library
# --------------------------------
MESSAGE(STATUS "=================================================")
MESSAGE(STATUS "Generating edX Project Format Library")

SET (PROJECT_CONFIG_FILES
	${CMAKE_SOURCE_DIR}/.clang-format
	${CMAKE_SOURCE_DIR}/.editorconfig
	${CMAKE_SOURCE_DIR}/SceneryEditorX.licenseheader
)

SET(EDX_HEADER_FILES
    edXConfig.h
    edXProjectFile.h
    edXLibraryFile.h
    edXManager.h
    edXTimeUtils.h
    resource.h
)

SET(EDX_SOURCE_FILES
    edXProjectFile.cpp
    edXLibraryFile.cpp
    edXManager.cpp
    edXTimeUtils.cpp
    edXWriter.cpp
    edXReader.cpp
    edXLibraryWriter.cpp
    edXLibraryReader.cpp
)

SET(EDX_EXAMPLE_FILES
    edXExample.cpp
)

SET(EDX_DOCUMENTATION_FILES
    README.md
)

ADD_LIBRARY(edX
	SHARED
		${EDX_HEADER_FILES}
		${EDX_SOURCE_FILES}
		${PROJECT_CONFIG_FILES}
    	edX.rc
)

# Optional: Build example executable
OPTION(EDX_BUILD_EXAMPLES "Build edX format examples" OFF)
IF(EDX_BUILD_EXAMPLES)
    ADD_EXECUTABLE(edXExample ${EDX_EXAMPLE_FILES})
    TARGET_LINK_LIBRARIES(edXExample PRIVATE edX)
    TARGET_COMPILE_FEATURES(edXExample PRIVATE cxx_std_17)
ENDIF()

# Set C++17 standard
TARGET_COMPILE_FEATURES(edX PRIVATE cxx_std_17)

# Source groups for IDE organization
SOURCE_GROUP("API"
	FILES
	    edXManager.h
)

SOURCE_GROUP("Project Format"
	FILES
	    edXProjectFile.h
	    edXProjectFile.cpp
	    edXWriter.cpp
	    edXReader.cpp
)

SOURCE_GROUP("Library Format"
	FILES
	    edXLibraryFile.h
	    edXLibraryFile.cpp
	    edXLibraryWriter.cpp
	    edXLibraryReader.cpp
)

SOURCE_GROUP("Utilities"
	FILES
	    edXTimeUtils.h
	    edXTimeUtils.cpp
)

SOURCE_GROUP("Examples"
	FILES
	    edXExample.cpp
)

SOURCE_GROUP("Documentation"
	FILES
	    README.md
)

SOURCE_GROUP("Resource"
	FILES
	    resource.h
	    edX.rc
)

TARGET_COMPILE_DEFINITIONS(edX
    PRIVATE
        $<$<CONFIG:Debug>:_CONSOLE>
        $<$<CONFIG:Debug>:_DEBUG>
        EDX_EXPORTS  # Define this when building the DLL
)

# Link against system libraries if needed
IF(WIN32)
    TARGET_LINK_LIBRARIES(edX PRIVATE
		nlohmann_json::nlohmann_json
	)
ELSEIF(APPLE)
    TARGET_LINK_LIBRARIES(edX PRIVATE
		nlohmann_json::nlohmann_json
	)
ELSEIF(UNIX)
    TARGET_LINK_LIBRARIES(edX PRIVATE
		nlohmann_json::nlohmann_json
	)
ENDIF()

# Set output directory
SET_TARGET_PROPERTIES(edX PROPERTIES
    OUTPUT_NAME "edX"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

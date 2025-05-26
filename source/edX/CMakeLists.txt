# --------------------------------
# EDX
# --------------------------------
MESSAGE(STATUS "=================================================")
MESSAGE(STATUS "Generating edX Project Format")

SET (PROJECT_CONFIG_FILES
	${CMAKE_SOURCE_DIR}/.clang-format
	${CMAKE_SOURCE_DIR}/.editorconfig
	${CMAKE_SOURCE_DIR}/SceneryEditorX.licenseheader
)

SET(EDX_HEADER_FILES
    edXLibraryFile.h
    edXProjectFile.h
    resource.h
)

SET(EDX_SOURCE_FILES
    edXWriter.cpp
    edXReader.cpp
    edXLibraryWriter.cpp
    edXLibraryReader.cpp
)

ADD_LIBRARY(edX
	SHARED
		${EDX_HEADER_FILES}
		${EDX_SOURCE_FILES}
		${PROJECT_CONFIG_FILES}
    	edXWriter.cpp
    	edXReader.cpp
    	edXLibraryWriter.cpp
    	edXLibraryReader.cpp
    	edXconfig.h
    	edX.rc
)

SOURCE_GROUP("Project Format"
	FILES
	    edXProjectFile.h
	    edXWriter.cpp
	    edXReader.cpp
	    edXconfig.h
)

SOURCE_GROUP("Library Format"
	FILES
	    edXLibraryFile.h
	    edXLibraryWriter.cpp
	    edXLibraryReader.cpp
)

source_group("Resource"
	FILES
	    resource.h
	    edX.rc
)

TARGET_COMPILE_DEFINITIONS(edX
    PRIVATE
        $<$<CONFIG:Debug>:_CONSOLE>
        $<$<CONFIG:Debug>:_DEBUG>
        EDX_API
)

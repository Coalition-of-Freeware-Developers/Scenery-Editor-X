# --------------------------------
# EDX
# --------------------------------
message(STATUS "=================================================")
message(STATUS "Generating edX Project Format")

set(EDX_HEADER_FILES
    edXLibraryFile.h
    edXProjectFile.h
    resource.h
)

set(EDX_SOURCE_FILES
    edXWriter.cpp
    edXReader.cpp
    edXLibraryWriter.cpp
    edXLibraryReader.cpp
)

add_library(edX SHARED ${EDX_HEADER_FILES} ${EDX_SOURCE_FILES}
    edXWriter.cpp
    edXReader.cpp
    edXLibraryWriter.cpp
    edXLibraryReader.cpp
    edXconfig.h
    edX.rc
)

source_group("Project Format" FILES
    edXProjectFile.h
    edXWriter.cpp
    edXReader.cpp
    edXconfig.h
)

source_group("Library Format" FILES
    edXLibraryFile.h
    edXLibraryWriter.cpp
    edXLibraryReader.cpp
)

source_group("Resource" FILES
    resource.h
    edX.rc
)

TARGET_COMPILE_DEFINITIONS(edX
    PRIVATE
        $<$<CONFIG:Debug>:_CONSOLE>
        $<$<CONFIG:Debug>:_DEBUG>
        EDX_API
)

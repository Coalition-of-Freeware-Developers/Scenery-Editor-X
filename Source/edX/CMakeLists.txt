# --------------------------------
# EDX
# --------------------------------
message(STATUS "=================================================")
message(STATUS "Generating edX Project Format")


add_library(edX SHARED ${EDX_HEADER_FILES} ${EDX_SOURCE_FILES}
    edXWriter.cpp
    edXReader.cpp
    edXLibraryWriter.cpp
    edXLibraryReader.cpp
    edX.rc
)


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

source_group("Project Format" FILES
    edXProjectFile.h
    edXWriter.cpp
    edXReader.cpp
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

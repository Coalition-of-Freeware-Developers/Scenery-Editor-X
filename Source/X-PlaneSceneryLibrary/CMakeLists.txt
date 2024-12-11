# --------------------------------
# X-PlaneSceneryLibrary
# --------------------------------

message(STATUS "=================================================")
message(STATUS "Generating X-Plane Scenery Library Project")


add_library(X-PlaneSceneryLibrary
    ${XPLIB_HEADER_FILES}
    ${XPLIB_SOURCE_FILES}
    DebugMain.cpp
    TextUtils.cpp
    XPLayerGroups.cpp
    XPLibrarySystem.cpp
    XPObj.cpp
    X-PlaneSceneryLibrary.rc
)

set(XPLIB_HEADER_FILES
    XPObj.h
    XPLibrarySystem.h
    XPLibraryPath.h
    XPLayerGroups.h
    X-PlaneSceneryLibraryUtils.h
    X-PlaneSceneryLibraryParsers.h
    XPAsset.h
    TextUtils.h
    resource.h
)

set(XPLIB_SOURCE_FILES
    DebugMain.cpp
    TextUtils.cpp
    XPLayerGroups.cpp
    XPLibrarySystem.cpp
    XPObj.cpp
)

source_group("Header Files" FILES
    XPObj.h
    XPLibrarySystem.h
    XPLibraryPath.h
    XPLayerGroups.h
    X-PlaneSceneryLibraryUtils.h
    X-PlaneSceneryLibraryParsers.h
    XPAsset.h
    TextUtils.h
)

source_group("Source Files" FILES
    TextUtils.cpp
    XPLayerGroups.cpp
    XPLibrarySystem.cpp
    XPObj.cpp
)

source_group("Resource" FILES
    resource.h
    X-PlaneSceneryLibrary.rc
)

source_group("Tests" FILES
    DebugMain.cpp
)

# --------------------------------
# X-PlaneSceneryLibrary
# --------------------------------

MESSAGE(STATUS "=================================================")
MESSAGE(STATUS "Generating X-Plane Scenery Library Project")


ADD_LIBRARY(X-PlaneSceneryLibrary
    ${XPLIB_HEADER_FILES}
    ${XPLIB_SOURCE_FILES}
    DebugMain.cpp
    TextUtils.cpp
    XPLayerGroups.cpp
    XPLibrarySystem.cpp
    XPObj.cpp
    X-PlaneSceneryLibrary.rc
)

SET(XPLIB_HEADER_FILES
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

SET(XPLIB_SOURCE_FILES
    DebugMain.cpp
    TextUtils.cpp
    XPLayerGroups.cpp
    XPLibrarySystem.cpp
    XPObj.cpp
)

SOURCE_GROUP("Header Files" FILES
    XPObj.h
    XPLibrarySystem.h
    XPLibraryPath.h
    XPLayerGroups.h
    X-PlaneSceneryLibraryUtils.h
    X-PlaneSceneryLibraryParsers.h
    XPAsset.h
    TextUtils.h
)

SOURCE_GROUP("Source Files" FILES
    TextUtils.cpp
    XPLayerGroups.cpp
    XPLibrarySystem.cpp
    XPObj.cpp
)

SOURCE_GROUP("Resource" FILES
    resource.h
    X-PlaneSceneryLibrary.rc
)

SOURCE_GROUP("Tests" FILES
    DebugMain.cpp
)

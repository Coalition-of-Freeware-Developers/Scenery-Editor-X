# --------------------------------
# X-PlaneSceneryLibrary
# --------------------------------

MESSAGE(STATUS "=================================================")
MESSAGE(STATUS "Generating X-Plane Scenery Library Project")

FILE(GLOB XPLIB_HEADER_FILES *.h *.hpp)
FILE(GLOB XPLIB_SOURCE_FILES *.cpp)

ADD_LIBRARY(X-PlaneSceneryLibrary
    ${XPLIB_HEADER_FILES}
    ${XPLIB_SOURCE_FILES}
    X-PlaneSceneryLibrary.rc
)
SOURCE_GROUP("Header Files" FILES
	${XPLIB_HEADER_FILES}
)
SOURCE_GROUP("Source Files" FILES
	${XPLIB_SOURCE_FILES}
)
SOURCE_GROUP("Resource" FILES
    resource.h
    X-PlaneSceneryLibrary.rc
)
SOURCE_GROUP("Tests" FILES
    DebugMain.cpp
)

TARGET_COMPILE_DEFINITIONS(X-PlaneSceneryLibrary
    PUBLIC
        XPdata
        $<$<CONFIG:Debug>:SEDX_DEBUG>
        $<$<CONFIG:Release>:SEDX_RELEASE>
)

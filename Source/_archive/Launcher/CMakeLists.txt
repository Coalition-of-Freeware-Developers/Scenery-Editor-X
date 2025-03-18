# --------------------------------
# Scenery Editor X | Launcher
# --------------------------------

MESSAGE(STATUS "=================================================")
MESSAGE(STATUS "Generating Launcher")

ADD_EXECUTABLE(Launcher
    ${ICON_FILES}
    ${REGISTRY_KEYS}
    ${CMAKE_SOURCE_DIR}/assets/splash_screen.bmp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/core/version.h
    core/Main.cpp
    launcher-resource.h
    Launcher.rc
    core/Logging.cpp
    registry/RegInit.h
    registry/RegCheck.h
    core/ElevatePerms.h
    core/DirectoryManager.cpp
    core/SplashHandler.h
    core/Updater.h
    core/Updater.cpp
)

SOURCE_GROUP("Resource" FILES
    ${ICON_FILES}
    ${REGISTRY_KEYS}
    ${CMAKE_SOURCE_DIR}/assets/splash_screen.bmp
    launcher-resource.h
    Launcher.rc
)

SOURCE_GROUP("Core" FILES
    core/Main.cpp
    core/Logging.cpp
    core/DirectoryManager.cpp
    core/SplashHandler.h
    core/Updater.h
    core/Updater.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/core/version.h
)

SOURCE_GROUP("Registry" FILES
    registry/RegInit.h
    registry/RegCheck.h
    core/ElevatePerms.h
)

SOURCE_GROUP("Registry/Keys" FILES
    registry/edXLibReg.rgs
    registry/edXReg.rgs
    registry/ProgramReg.rgs
)

TARGET_LINK_LIBRARIES(Launcher PRIVATE
    spdlog::spdlog_header_only
    dxgi
    d3d12
    CURL::libcurl
    Shell32.lib
)

TARGET_INCLUDE_DIRECTORIES(Launcher PRIVATE
    ${CMAKE_SOURCE_DIR}/dependency/imgui
    ${CMAKE_SOURCE_DIR}/dependency/imgui/backends
    ${CMAKE_SOURCE_DIR}/dependency/imgui/misc
    ${CMAKE_SOURCE_DIR}/dependency/ImGuizmo
    ${CMAKE_SOURCE_DIR}/Source/SceneryEditorX
    ${CMAKE_SOURCE_DIR}/Source/SceneryEditorX/logging
    ${CURL_INCLUDE_DIRS}
)

TARGET_LINK_DIRECTORIES(Launcher PRIVATE ${CURL_LIBRARY_DIRS})

TARGET_LINK_OPTIONS(Launcher PRIVATE /SUBSYSTEM:WINDOWS)

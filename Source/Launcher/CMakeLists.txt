# --------------------------------
# Scenery Editor X | Launcher
# --------------------------------

message(STATUS "=================================================")
message(STATUS "Generating Launcher")


source_group("Resource" FILES
    ${ICON_FILES}
    ${REGISTRY_KEYS}
    ${CMAKE_SOURCE_DIR}/assets/splash_screen.bmp
    resource.h
    Launcher.rc
)

source_group("Core" FILES
    core/Main.cpp
    core/Logging.cpp
    core/DirectoryManager.cpp
    core/SplashHandler.h
    core/Updater.h
    core/Updater.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/core/version.h
)

source_group("Registry" FILES
    registry/RegInit.h
    registry/RegCheck.h
    core/ElevatePerms.h
)

add_executable(Launcher
    ${ICON_FILES}
    ${REGISTRY_KEYS}
    ${CMAKE_SOURCE_DIR}/assets/splash_screen.bmp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/core/version.h
    core/Main.cpp
    resource.h
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

target_link_libraries(Launcher PRIVATE
    spdlog::spdlog_header_only
    dxgi
    d3d12
    CURL::libcurl
    Shell32.lib
)

target_include_directories(Launcher PRIVATE
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/logging
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/
    ${CURL_INCLUDE_DIRS}
)

target_link_directories(Launcher PRIVATE ${CURL_LIBRARY_DIRS})

target_link_options(Launcher PRIVATE /SUBSYSTEM:WINDOWS)

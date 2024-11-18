cmake_minimum_required(VERSION 3.10)

# Build System
# --------------------------------

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/Dependency/vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "")

project(SceneryEditorX VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Packages
# --------------------------------

find_package(glm      CONFIG REQUIRED)
find_package(spdlog   CONFIG REQUIRED)
find_package(imgui    CONFIG REQUIRED)
find_package(imguizmo CONFIG REQUIRED)

find_path(PORTABLE_FILE_DIALOGS_INCLUDE_DIRS "portable-file-dialogs.h")

# EDX
# --------------------------------

add_library(edX
    Source/edX/edXWriter.cpp
    Source/edX/edXReader.cpp
    Source/edX/edXLibraryWriter.cpp
    Source/edX/edXLibraryReader.cpp
)

# X-PlaneSceneryLibrary
# --------------------------------

add_library(X-PlaneSceneryLibrary
    Source/X-PlaneSceneryLibrary/DebugMain.cpp
    Source/X-PlaneSceneryLibrary/TextUtils.cpp
    Source/X-PlaneSceneryLibrary/XPLayerGroups.cpp
    Source/X-PlaneSceneryLibrary/XPLibrarySystem.cpp
    Source/X-PlaneSceneryLibrary/XPObj.cpp
)

# SceneryEditorX
# --------------------------------

# Glob the optick sources and compile the executable with them directly.
file(GLOB OPTICK_SOURCES Dependency/optick/src/*.cpp)

add_executable(SceneryEditorX
    Source/SceneryEditorX/Main.cpp
    ${OPTICK_SOURCES}
)

# Include
target_include_directories(SceneryEditorX PRIVATE 
    Source/SceneryEditorX/Include
    Dependency/optick/src/
    ${PORTABLE_FILE_DIALOGS_INCLUDE_DIRS}
)

# Link
target_link_libraries(SceneryEditorX PRIVATE 
    glm::glm-header-only
    spdlog::spdlog_header_only
    imgui::imgui
    imguizmo::imguizmo
)

# Precompiled Header
target_precompile_headers(SceneryEditorX PRIVATE Source/SceneryEditorX/Include/Precompiled.h)
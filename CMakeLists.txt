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
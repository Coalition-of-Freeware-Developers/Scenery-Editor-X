cmake_minimum_required(VERSION 3.28)

# --------------------------------
# Global Setup
# --------------------------------
set(CMAKE_CONFIGURATION_TYPES Debug Release CACHE TYPE INTERNAL FORCE)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/dependency/vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT SceneryEditorX)

# Require builds to be outside of source tree.
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)

if(EXISTS "${LOC_PATH}")
    message(FATAL_ERROR "You cannot build in a source directory (or any directory with a CMakeLists.txt file). Please use a build directory instead.")
endif()

# Enable folders (for Visual Studio).
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
# Sets all CMake default generated project targets to their own filter.
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "_CMakePredefinedTargets")

# set default build types
if(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "" FORCE)
endif()

if(WIN32)
    set(CMAKE_SYSTEM_VERSION 10.0)
    if(CMAKE_GENERATOR MATCHES "Visual Studio")
        add_compile_options("/MP")
    endif()
endif()

# --------------------------------
# Scenery Editor X Project Setup
# --------------------------------

project(SceneryEditorX VERSION 1.0)

message(STATUS "=================================================")
message(STATUS "Beginning project generation for Scenery Editor X")
message(STATUS "=================================================")

set(FONT_FILES
    ${CMAKE_SOURCE_DIR}/assets/fonts/industry/Industry-Bold.otf
    ${CMAKE_SOURCE_DIR}/assets/fonts/industry/Industry-Book.otf
    ${CMAKE_SOURCE_DIR}/assets/fonts/industry/Industry-Medium.otf
    ${CMAKE_SOURCE_DIR}/assets/fonts/industry/Industry-Thin.otf
    ${CMAKE_SOURCE_DIR}/assets/fonts/opensans/OpenSans-Bold.ttf
    ${CMAKE_SOURCE_DIR}/assets/fonts/opensans/OpenSans-BoldItalic.ttf
    ${CMAKE_SOURCE_DIR}/assets/fonts/opensans/OpenSans-ExtraBold.ttf
    ${CMAKE_SOURCE_DIR}/assets/fonts/opensans/OpenSans-ExtraBoldItalic.ttf
    ${CMAKE_SOURCE_DIR}/assets/fonts/opensans/OpenSans-Italic.ttf
    ${CMAKE_SOURCE_DIR}/assets/fonts/opensans/OpenSans-Light.ttf
    ${CMAKE_SOURCE_DIR}/assets/fonts/opensans/OpenSans-LightItalic.ttf
    ${CMAKE_SOURCE_DIR}/assets/fonts/opensans/OpenSans-Regular.ttf
    ${CMAKE_SOURCE_DIR}/assets/fonts/opensans/OpenSans-SemiBold.ttf
    ${CMAKE_SOURCE_DIR}/assets/fonts/opensans/OpenSans-SemiBoldItalic.ttf
)

set(REGISTRY_KEYS
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/Registry/edXLibReg.rgs
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/Registry/edXReg.rgs
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/Registry/ProgramReg.rgs
)

set(ICON_FILES
    ${CMAKE_SOURCE_DIR}/assets/icon.ico
    ${CMAKE_SOURCE_DIR}/assets/icon_edX.ico
    ${CMAKE_SOURCE_DIR}/assets/icon_testing.ico
)

# Glob the optick sources and compile the executable with them directly.
file(GLOB OPTICK_SOURCES dependency/optick/src/*.cpp)
message(STATUS "Optick Graphics Debugger location: ${CMAKE_SOURCE_DIR}/dependency/optick/src/")

# --------------------------------
# Packages
# --------------------------------

find_package(glm      CONFIG REQUIRED)
find_package(glfw3    CONFIG REQUIRED)
find_package(spdlog   CONFIG REQUIRED)
find_package(imgui    CONFIG REQUIRED)
find_package(imguizmo CONFIG REQUIRED)
find_package(Stb             REQUIRED)
find_package(CURL     CONFIG REQUIRED)

find_path(PORTABLE_FILE_DIALOGS_INCLUDE_DIRS "portable-file-dialogs.h")

# --------------------------------
# EDX
# --------------------------------
add_subdirectory(source/edX)

# --------------------------------
# File Format Tester
# --------------------------------
add_subdirectory(source/edX/testing)

# --------------------------------
# X-PlaneSceneryLibrary
# --------------------------------
add_subdirectory(source/X-PlaneSceneryLibrary)

# --------------------------------
# SceneryEditorX | Launcher
# --------------------------------
add_subdirectory(source/Launcher)

# --------------------------------
# VulkanRender
# --------------------------------
add_subdirectory(source/VulkanRenderer)

# --------------------------------
# SceneryEditorX
# --------------------------------
add_subdirectory(source/SceneryEditorX)

# Define SEDX_DEBUG for Debug builds using generator expressions
target_compile_definitions(Launcher         PRIVATE $<$<CONFIG:Debug>:SEDX_DEBUG> $<$<CONFIG:Release>:SEDX_RELEASE>)
target_compile_definitions(SceneryEditorX   PRIVATE $<$<CONFIG:Debug>:SEDX_DEBUG> $<$<CONFIG:Release>:SEDX_RELEASE>)
target_compile_definitions(VulkanRenderer   PRIVATE $<$<CONFIG:Debug>:SEDX_DEBUG> $<$<CONFIG:Release>:SEDX_RELEASE>)

# --------------------------------
# Defines
# --------------------------------

target_compile_definitions(SceneryEditorX   PRIVATE WIN32_LEAN_AND_MEAN)
target_compile_definitions(Launcher         PRIVATE WIN32_LEAN_AND_MEAN)
target_compile_definitions(VulkanRenderer   PRIVATE WIN32_LEAN_AND_MEAN)

# --------------------------------
# Precompiled Header
# --------------------------------
message(STATUS "=================================================")
message(STATUS "Generating precompiled headers")
target_precompile_headers(SceneryEditorX PRIVATE source/SceneryEditorX/xpeditor_pch.h)



set_property(TARGET Launcher VulkanRenderer PROPERTY FOLDER "Core")
set_property(TARGET FormatTester PROPERTY FOLDER "Tools")
set_property(TARGET edX X-PlaneSceneryLibrary PROPERTY FOLDER "File Formats")

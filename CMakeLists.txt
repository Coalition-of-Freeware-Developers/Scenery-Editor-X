CMAKE_MINIMUM_REQUIRED(VERSION 3.8...4.0)

# Allow legacy FetchContent_Populate calls for compatibility with our embedding patch
IF(POLICY CMP0169)
    CMAKE_POLICY(SET CMP0169 OLD)
ENDIF()

# --------------------------------
# Global Setup
# --------------------------------
SET(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/dependency/vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "")
SET(CMAKE_CONFIGURATION_TYPES Debug Release CACHE STRING INTERNAL FORCE)
SET(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1 CACHE INTERNAL "No dev warnings")
SET(VCPKG_INSTALL_OPTIONS --no-print-usage)
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON) # Remove when min is 3.26, see CMP0143
SET_PROPERTY(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT SceneryEditorX)
SET_PROPERTY(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "_CMakePredefinedTargets")

# --------------------------------
# Scenery Editor X Project Setup
# --------------------------------
PROJECT(SceneryEditorX
    VERSION 1.0.0
    DESCRIPTION "A 3d scenery editor for X-Plane 12 airports."
    LANGUAGES CXX C
    HOMEPAGE_URL "https://github.com/Coalition-of-Freeware-Developers/Scenery-Editor-X"
)

# --------------------------------
# Platform configuration (split out per-OS)
# --------------------------------
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
IF(WIN32)
    INCLUDE("${CMAKE_SOURCE_DIR}/cmake/platform_win.cmake")
ELSEIF(APPLE)
    INCLUDE("${CMAKE_SOURCE_DIR}/cmake/platform_mac.cmake")
ELSEIF(UNIX)
    INCLUDE("${CMAKE_SOURCE_DIR}/cmake/platform_linux.cmake")
ENDIF()

# --------------------------------
# Output directories
# --------------------------------
SET(BIN_DIR ${CMAKE_SOURCE_DIR}/bin/${CMAKE_CFG_INTDIR})
SET(LIBS_DIR ${CMAKE_SOURCE_DIR}/lib/${CMAKE_CFG_INTDIR})
IF(TARGET ${PROJECT_NAME})
    SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/")
ENDIF()

# --------------------------------
# USE FINDVULKAN MODULE ADDED WITH CMAKE 3.7
# --------------------------------
IF(NOT CMAKE_VERSION VERSION_LESS 3.7.0)
    MESSAGE(STATUS "Using module to find Vulkan")
    FIND_PACKAGE(Vulkan)
    SET(VULKAN_VERS ${Vulkan_FOUND})
    MESSAGE(STATUS "Using Vulkan SDK version: ${VULKAN_VERS}")
ENDIF()

# --------------------------------
# Assets configuration
# --------------------------------
INCLUDE("${CMAKE_SOURCE_DIR}/cmake/assets.cmake")

# used for pulling external dependencies like xMath)
INCLUDE(FetchContent)

# --------------------------------
# Tracy Profiler Options
# --------------------------------
OPTION(TRACY_ENABLE "Enable Tracy profiler" ON)
OPTION(TRACY_ON_DEMAND "Enable Tracy on-demand profiling" OFF)
OPTION(TRACY_NO_EXIT "Enable Tracy profiler even without exit" OFF)
OPTION(TRACY_NO_BROADCAST "Disable Tracy broadcast" OFF)
OPTION(SEDX_TRACY_CALLSTACK "Size of the collected collected call stacks" 48)
OPTION(SEDX_ENABLE_TRACY_CPU_MEMORY "Enable CPU memory profiling with Tracy" OFF)
OPTION(SEDX_ENABLE_TRACY_GPU "Enable GPU profiling with Tracy" OFF)

# --------------------------------
# Tracy Profiler Setup
# --------------------------------
IF(TRACY_ENABLE)
    IF(TRACY_ENABLE_CPU_MEMORY AND USE_MIMALLOC)
        MESSAGE(FATAL_ERROR "Tracy cannot be used with mimalloc")
    ENDIF()
	# --------------------------------
    IF(SEDX_TRACY_CALLSTACK STREQUAL "OFF")
        SET(SEDX_TRACY_CALLSTACK 48)
    ENDIF()
	# --------------------------------
    ADD_COMPILE_DEFINITIONS(
		TRACY_ENABLE
		TRACY_CALLSTACK=${SEDX_TRACY_CALLSTACK}
        TRACY_DELAYED_INIT
		TRACY_MANUAL_LIFETIME
	)
	# --------------------------------
	IF(SEDX_ENABLE_TRACY_CPU_MEMORY)
    	ADD_COMPILE_DEFINITIONS(SEDX_TRACY_CPU_MEMORY)
    ENDIF()
	# --------------------------------
    IF(SEDX_ENABLE_TRACY_GPU)
        ADD_COMPILE_DEFINITIONS(
            SEDX_TRACY_GPU
            TRACY_VK_USE_SYMBOL_TABLE
        )
    ENDIF()
	# --------------------------------
    IF(TRACY_ON_DEMAND)
        ADD_COMPILE_DEFINITIONS(TRACY_ON_DEMAND)
    ENDIF()
	# --------------------------------
    IF(TRACY_NO_EXIT)
        ADD_COMPILE_DEFINITIONS(TRACY_NO_EXIT)
    ENDIF()
	# --------------------------------
    IF(TRACY_NO_BROADCAST)
        ADD_COMPILE_DEFINITIONS(TRACY_NO_BROADCAST)
    ENDIF()
	# --------------------------------
ENDIF()

# --------------------------------
# Doxygen Options
# --------------------------------
OPTION(BUILD_DOXYGEN "Build Doxygen documentation" OFF)
IF(BUILD_DOXYGEN)
    INCLUDE(cmake/doxygen.cmake)
ENDIF()

# --------------------------------
# Libconfig Options
# --------------------------------
# Override libconfig options before adding the subdirectory
OPTION(BUILD_EXAMPLES "Enable examples" OFF)
OPTION(BUILD_SHARED_LIBS "Enable shared library" OFF)
OPTION(BUILD_TESTS "Enable tests" OFF)
OPTION(BUILD_FUZZERS "Enable fuzzers" OFF)

# --------------------------------
# BUILD SYSTEM
# --------------------------------
SET(CMAKE_C_STANDARD 17)
SET(CMAKE_C_STANDARD_REQUIRED True)
SET(CMAKE_C_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_STANDARD_REQUIRED True)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)
ADD_COMPILE_DEFINITIONS(
    $<$<CONFIG:Debug>:SEDX_DEBUG>
    $<$<CONFIG:Release>:SEDX_RELEASE>
)
MESSAGE(VCPKG_ROOT = " ${CMAKE_TOOLCHAIN_FILE} ")

# --------------------------------
# REQUIRE BUILDS TO BE OUTSIDE OF SOURCE TREE.
# --------------------------------
FILE(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)

IF(EXISTS "${LOC_PATH}")
    MESSAGE(FATAL_ERROR "You cannot build in a source directory (or any directory with a CMakeLists.txt file). Please use a build directory instead.")
ENDIF()

# SET DEFAULT BUILD TYPES
IF(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    SET(CMAKE_BUILD_TYPE "Debug" CACHE STRING "" FORCE)
ENDIF()

MESSAGE(STATUS "=================================================")
MESSAGE(STATUS "Beginning project generation for Scenery Editor X")
MESSAGE(STATUS "=================================================")

# --------------------------------
# PACKAGES VCPKG
# --------------------------------
FIND_PACKAGE(Vulkan                        REQUIRED)
FIND_PACKAGE(msdfgen				CONFIG REQUIRED)
FIND_PACKAGE(spdlog                 CONFIG REQUIRED)
FIND_PACKAGE(tracy				    CONFIG REQUIRED)
FIND_PACKAGE(Stb                           REQUIRED)
FIND_PACKAGE(CURL                   CONFIG REQUIRED)
FIND_PACKAGE(Doxygen                          QUIET)
FIND_PACKAGE(VulkanMemoryAllocator  CONFIG REQUIRED)

FIND_PATH(PORTABLE_FILE_DIALOGS_INCLUDE_DIRS "portable-file-dialogs.h")

# --------------------------------
# GLFW
# --------------------------------
ADD_SUBDIRECTORY(dependency/glfw)

# --------------------------------
# libconfig
# --------------------------------
ADD_SUBDIRECTORY(dependency/libconfig)

# --------------------------------
# Animation Compression Library - ACL
# --------------------------------
#ADD_SUBDIRECTORY(dependency/acl)

# --------------------------------
# Catch2
# --------------------------------
ADD_SUBDIRECTORY(dependency/Catch2)

# --------------------------------
# EnTT
# --------------------------------
ADD_SUBDIRECTORY(dependency/entt)

# --------------------------------
# MSDF-Atlas-Generator
# --------------------------------
#ADD_SUBDIRECTORY(dependency/msdf-atlas-gen)

# --------------------------------
# json-cpp-gen
# --------------------------------
ADD_SUBDIRECTORY(dependency/json-cpp-gen)

# --------------------------------
# nlohmann/json
# --------------------------------
ADD_SUBDIRECTORY(dependency/json)

# --------------------------------
# Enable CTest/CTest integration at the top-level so tests added in subdirectories
# are discoverable from the build root via `ctest`.
# --------------------------------
INCLUDE(CTest)
IF(BUILD_TESTING)
    ENABLE_TESTING()
	ADD_SUBDIRECTORY(${CMAKE_SOURCE_DIR}/source/Tests)
ENDIF()

# --------------------------------
# Math Library
# --------------------------------
# If you want xMath tests in parent, pass -DXMATH_BUILD_TESTS=ON
# Optionally -DXMATH_VENDOR_DEPS=ON if Catch2 isn’t provided.
# --------------------------------
# Prefer embedding xMath via FetchContent to avoid relying on a possibly
# of stale/broken system installation/config package.
# --------------------------------
# Shallow clone, disabled submodules to avoid SSH-required deps: (GIT_SHALLOW TRUE, GIT_SUBMODULES "")
# Configure xMath build options before adding the subdirectory
# --------------------------------
SET(XMATH_BUILD_TESTS OFF CACHE BOOL "Build xMath tests" FORCE)
SET(XMATH_VENDOR_DEPS OFF CACHE BOOL "Vendor deps for xMath" FORCE)

FETCHCONTENT_DECLARE(
    xMath
    GIT_REPOSITORY https://github.com/The3dVehicleguy/xMath.git
    GIT_TAG main
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
    GIT_SUBMODULES ""
)

FETCHCONTENT_GETPROPERTIES(xMath)
IF(NOT xmath_POPULATED)
    FETCHCONTENT_POPULATE(xMath)
    IF(EXISTS "${xmath_SOURCE_DIR}/cmake/math.cmake")
        FILE(READ "${xmath_SOURCE_DIR}/cmake/math.cmake" XMATH_MATH_CMAKE)
        STRING(REPLACE "CMAKE_SOURCE_DIR" "CMAKE_CURRENT_SOURCE_DIR" XMATH_MATH_CMAKE "${XMATH_MATH_CMAKE}")
        FILE(WRITE "${xmath_SOURCE_DIR}/cmake/math.cmake" "${XMATH_MATH_CMAKE}")
    ENDIF()
    ADD_SUBDIRECTORY("${xmath_SOURCE_DIR}" "${xmath_BINARY_DIR}")
ENDIF()

# --------------------------------
# ImGui
# --------------------------------
ADD_LIBRARY(imgui
	${CMAKE_SOURCE_DIR}/dependency/imgui/imgui.cpp
	${CMAKE_SOURCE_DIR}/dependency/imgui/imgui_draw.cpp
	${CMAKE_SOURCE_DIR}/dependency/imgui/imgui_widgets.cpp
	${CMAKE_SOURCE_DIR}/dependency/imgui/imgui_tables.cpp
	${CMAKE_SOURCE_DIR}/dependency/imgui/imgui_demo.cpp
	${CMAKE_SOURCE_DIR}/dependency/imgui/imgui_widgets.cpp
	${CMAKE_SOURCE_DIR}/dependency/imgui/backends/imgui_impl_glfw.cpp
	${CMAKE_SOURCE_DIR}/dependency/imgui/backends/imgui_impl_vulkan.cpp
)
TARGET_INCLUDE_DIRECTORIES(imgui
    PRIVATE
        ${CMAKE_SOURCE_DIR}/dependency/imgui
        ${CMAKE_SOURCE_DIR}/dependency/imgui/backends
        ${CMAKE_SOURCE_DIR}/dependency/imgui/misc
)
TARGET_LINK_LIBRARIES(imgui
	PRIVATE
		glfw
		Vulkan::Vulkan
		GPUOpen::VulkanMemoryAllocator
)

# --------------------------------
# AppCore
# --------------------------------
ADD_SUBDIRECTORY(source/SceneryEditorX)

# --------------------------------
# Launcher
# --------------------------------
ADD_SUBDIRECTORY(source/Launcher)

# --------------------------------
# Scenery Editor X
# --------------------------------
ADD_SUBDIRECTORY(source/Editor)

# --------------------------------
# EDX
# --------------------------------
ADD_SUBDIRECTORY(source/edX)

# --------------------------------
# Crash Handler Service
# --------------------------------
ADD_SUBDIRECTORY(source/CrashHandler)

# --------------------------------
# X-PLANE SCENERY LIBRARY
# --------------------------------
ADD_SUBDIRECTORY(source/X-PlaneSceneryLibrary)

# --------------------------------
# PRECOMPILED HEADER
# --------------------------------
MESSAGE(STATUS "=================================================")
MESSAGE(STATUS "Generating precompiled headers")

TARGET_PRECOMPILE_HEADERS(SceneryEditorX PRIVATE ${CMAKE_SOURCE_DIR}/Source/Editor/xpeditor_pch.h)
TARGET_PRECOMPILE_HEADERS(AppCore PRIVATE ${CMAKE_SOURCE_DIR}/Source/SceneryEditorX/app_pch.h)
TARGET_PRECOMPILE_HEADERS(Launcher PRIVATE ${CMAKE_SOURCE_DIR}/Source/Launcher/startup_pch.h)

SET_PROPERTY(TARGET CrashHandler PROPERTY FOLDER "Tools")
SET_PROPERTY(TARGET MemoryAllocatorTests MathTests RefTests SettingsTest EdxTests EdxDemoGenerator ConversionTests PROPERTY FOLDER "Tests")
SET_PROPERTY(TARGET edX PROPERTY FOLDER "File Formats")
SET_PROPERTY(TARGET glfw uninstall update_mappings PROPERTY FOLDER "Dependency/GLFW3")
SET_PROPERTY(TARGET xMath imgui json-cpp-gen nlohmann_json PROPERTY FOLDER "Dependency")
SET_PROPERTY(TARGET libconfig libconfig++ PROPERTY FOLDER "Dependency/LibConfig")
SET_PROPERTY(TARGET Catch2 Catch2WithMain PROPERTY FOLDER "Dependency/Catch2")

FOREACH(TARGET IN ITEMS Launcher SceneryEditorX AppCore MathTests MemoryAllocatorTests ConversionTests RefTests SettingsTest EdxTests EdxDemoGenerator CrashHandler Catch2 Catch2WithMain nlohmann_json json-cpp-gen imgui xMath libconfig libconfig++ edX X-PlaneSceneryLibrary glfw)
    SET_TARGET_PROPERTIES(${TARGET} PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY ${LIBS_DIR}
        LIBRARY_OUTPUT_DIRECTORY ${LIBS_DIR}
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
    )
ENDFOREACH()

# Ensure debugger working directory is set for the startup target once it exists
IF(TARGET SceneryEditorX)
    SET_TARGET_PROPERTIES(SceneryEditorX PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/")
ENDIF()

# --------------------------------
FOREACH(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/lib/${OUTPUTCONFIG})
    SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/lib/${OUTPUTCONFIG})
    SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/bin/${OUTPUTCONFIG})
ENDFOREACH(OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES)
# --------------------------------

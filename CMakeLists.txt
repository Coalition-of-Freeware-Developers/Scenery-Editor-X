CMAKE_MINIMUM_REQUIRED(VERSION 3.8...4.0)

# Allow legacy FetchContent_Populate calls for compatibility with our embedding patch
IF(POLICY CMP0169)
    CMAKE_POLICY(SET CMP0169 OLD)
ENDIF()

############################################
# Global Setup
############################################

SET(TOOLCHAIN_DIRECTORY "${CMAKE_SOURCE_DIR}/cmake/toolchains")
IF(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    IF(WIN32 AND EXISTS "${TOOLCHAIN_DIRECTORY}/sedx-windows.cmake")
        SET(CMAKE_TOOLCHAIN_FILE "${TOOLCHAIN_DIRECTORY}/sedx-windows.cmake" CACHE STRING "" FORCE)
    ELSEIF(APPLE AND EXISTS "${TOOLCHAIN_DIRECTORY}/sedx-mac.cmake")
        SET(CMAKE_TOOLCHAIN_FILE "${TOOLCHAIN_DIRECTORY}/sedx-mac.cmake" CACHE STRING "" FORCE)
    ELSEIF(UNIX AND EXISTS "${TOOLCHAIN_DIRECTORY}/sedx-linux.cmake")
        SET(CMAKE_TOOLCHAIN_FILE "${TOOLCHAIN_DIRECTORY}/sedx-linux.cmake" CACHE STRING "" FORCE)
    ENDIF()
ENDIF()

SET(CMAKE_CONFIGURATION_TYPES Debug Release CACHE STRING INTERNAL FORCE)
SET(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1 CACHE INTERNAL "No dev warnings")
SET(VCPKG_INSTALL_OPTIONS --no-print-usage)
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON) # Remove when min is 3.26, see CMP0143
SET_PROPERTY(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT SceneryEditorX)
SET_PROPERTY(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "_CMakePredefinedTargets")

############################################
# Scenery Editor X - Project Setup
############################################
PROJECT(SceneryEditorX
    VERSION 1.0.0
    DESCRIPTION "A 3d scenery editor for X-Plane 12 airports."
    LANGUAGES CXX C
    HOMEPAGE_URL "https://github.com/Coalition-of-Freeware-Developers/Scenery-Editor-X"
)

############################################
# Platform configuration
# (Handled by SEDX toolchains; keep module path for local cmake helpers)
############################################
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

############################################
# Core dependency discovery helpers and options
############################################

# FetchContent availability (for optional vendoring)
IF(CMAKE_VERSION VERSION_GREATER_EQUAL 3.11)
    INCLUDE(FetchContent)
ELSE()
    MESSAGE(STATUS "CMake < 3.11: FetchContent is not available; using bundled subdirectories where applicable.")
ENDIF()

############################################
# Assets configuration
############################################
INCLUDE("${CMAKE_SOURCE_DIR}/cmake/assets.cmake")

############################################
# Build options (dependency toggles, features)
############################################

OPTION(SEDX_ENABLE_TESTS              	"Build test targets" OFF)
OPTION(SEDX_ENABLE_DOXYGEN            	"Build Doxygen documentation" OFF)
OPTION(SEDX_ENABLE_VERSION_INCREMENT 	"Enable automatic semantic versioning, increment on build" OFF)
#OPTION(SEDX_ENABLE_VMA                	"Enable Vulkan Memory Allocator (GPUOpen VMA)" ON)
OPTION(SEDX_BUILD_IMGUI               	"Build Dear ImGui (bundled sources)" ON)
OPTION(SEDX_WITH_IMGUI_DEMO           	"Compile ImGui demo window" ON)
OPTION(SEDX_USE_SYSTEM_SDL           	"Prefer system-provided sdl3 package" OFF)
OPTION(SEDX_USE_SYSTEM_JSON           	"Prefer system-provided nlohmann_json package" OFF)
OPTION(SEDX_USE_SYSTEM_ENTT           	"Prefer system-provided EnTT package" OFF)
OPTION(SEDX_VERBOSE_LOGGING			 	"Enable verbose logging of Scenery Editor X" ON)
#OPTION(SEDX_USE_SYSTEM_CATCH2         	"Prefer system-provided Catch2 package" OFF)
#OPTION(SEDX_USE_SYSTEM_SPDLOG         	"Prefer system-provided spdlog package" ON)
OPTION(SEDX_USE_FETCHCONTENT_XMATH    	"Fetch xMath via FetchContent" ON)
OPTION(SEDX_USE_FETCHCONTENT_EDX      	"Fetch edX via FetchContent" ON)
OPTION(SEDX_USE_FETCHCONTENT_XPLIB    	"Fetch X-PlaneSceneryLibrary via FetchContent" ON)
#OPTION(SEDX_PROFILING_ENABLED 			"Enable profiling features (Tracy, etc.)" ON)
OPTION(TRACY_ENABLE 					"Enable Tracy profiler" ON)
OPTION(SEDX_USE_FETCHCONTENT_TRACY    	"Fetch Tracy via FetchContent" ON)
#OPTION(SEDX_ENABLE_MSDFGEN            	"Enable msdfgen usage" ON)
#OPTION(SEDX_ENABLE_CURL               	"Enable CURL usage" ON)

SET(XMATH_BUILD_TESTS OFF CACHE BOOL "Build xMath tests" FORCE)
SET(XMATH_VENDOR_DEPS OFF CACHE BOOL "Vendor deps for xMath" FORCE)

############################################
# Doxygen (optional)
############################################
IF(SEDX_ENABLE_DOXYGEN)
    INCLUDE(cmake/doxygen.cmake)
ENDIF()

# ###########################################
# Tracy Profiler (optional)
# ###########################################
IF(TRACY_ENABLE)
	INCLUDE(cmake/tracyprofiling.cmake)

	IF(DEFINED tracy_SOURCE_DIR) # Add Tracy public include dir to all main targets if tracy is available
		MESSAGE(STATUS "Adding Tracy public include dir: ${tracy_SOURCE_DIR}/public")

		FOREACH(_tracy_target SceneryEditorX Launcher Editor CrashHandler)
			IF(TARGET ${_tracy_target})
				TARGET_INCLUDE_DIRECTORIES(${_tracy_target} PRIVATE ${tracy_SOURCE_DIR}/public)
			ENDIF()
		ENDFOREACH()
	ENDIF()
ENDIF()

############################################
# Global build system defaults
############################################
# Override libconfig/bundled defaults before adding subdirectories
OPTION(BUILD_EXAMPLES  		"Enable examples (3rd-party)" OFF)
OPTION(BUILD_SHARED_LIBS 	"Build shared libraries by default" OFF)
OPTION(BUILD_TESTS     		"Enable 3rd-party tests" OFF)
OPTION(BUILD_FUZZERS  		"Enable 3rd-party fuzzers" OFF)

############################################
# Build system defaults
############################################
SET(CMAKE_C_STANDARD 17)
SET(CMAKE_C_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)
ADD_COMPILE_DEFINITIONS(
    $<$<CONFIG:Debug>:SEDX_DEBUG>
    $<$<CONFIG:Release>:SEDX_RELEASE>
)
MESSAGE(STATUS "Toolchain file: ${CMAKE_TOOLCHAIN_FILE}")

# Require out-of-source builds
FILE(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)

IF(EXISTS "${LOC_PATH}")
    MESSAGE(FATAL_ERROR "You cannot build in a source directory (or any directory with a CMakeLists.txt file). Please use a build directory instead.")
ENDIF()

# Default build type
IF(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    SET(CMAKE_BUILD_TYPE "Debug" CACHE STRING "")
ENDIF()

MESSAGE(STATUS "=================================================")
MESSAGE(STATUS "Beginning project generation for Scenery Editor X")
MESSAGE(STATUS "=================================================")

############################################
# Packages and dependencies
############################################
FIND_PACKAGE(Vulkan REQUIRED)
FIND_PACKAGE(Stb REQUIRED)
FIND_PACKAGE(spdlog CONFIG REQUIRED)
FIND_PACKAGE(msdfgen CONFIG REQUIRED)
FIND_PACKAGE(Doxygen QUIET)
FIND_PACKAGE(VulkanMemoryAllocator CONFIG REQUIRED)
FIND_PACKAGE(CURL CONFIG REQUIRED)

FIND_PATH(PORTABLE_FILE_DIALOGS_INCLUDE_DIRS "portable-file-dialogs.h") # Portable File Dialogs header-only
FIND_LIBRARY(Slang_LIBRARY NAMES slang HINTS "$ENV{VULKAN_SDK}/lib" REQUIRED)

IF(WIN32)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR")
ELSEIF(LINUX)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WAYLAND_KHR")
ENDIF()

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNOMINMAX -D_USE_MATH_DEFINES")

IF(MSVC)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
ENDIF()

############################################
# Add KTX library (bundled sources)
############################################
SET(KTX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dependency/ktx)
SET(KTX_SOURCES
	${KTX_DIR}/lib/texture.c
	${KTX_DIR}/lib/hashlist.c
	${KTX_DIR}/lib/checkheader.c
	${KTX_DIR}/lib/swap.c
	${KTX_DIR}/lib/memstream.c
	${KTX_DIR}/lib/filestream.c
	${KTX_DIR}/lib/vkloader.c)

ADD_LIBRARY(ktx STATIC ${KTX_SOURCES})
TARGET_INCLUDE_DIRECTORIES(ktx PUBLIC
	${KTX_DIR}/include
	${KTX_DIR}/other_include
	${KTX_DIR}/lib
)

# ktx's vkloader.c includes <vulkan/vulkan.h> via ktxvulkan.h
IF(TARGET Vulkan::Headers)
	TARGET_LINK_LIBRARIES(ktx PRIVATE Vulkan::Headers)
ELSE()
	TARGET_INCLUDE_DIRECTORIES(ktx PRIVATE ${Vulkan_INCLUDE_DIRS})
ENDIF()

############################################
# SDL (system or bundled)
############################################
IF(SEDX_USE_SYSTEM_SDL)
    FIND_PACKAGE(SDL3 CONFIG REQUIRED)
ELSE()
    # Disable precompiled headers for SDL to avoid SDL_internal.h include issues
    SET(CMAKE_DISABLE_PRECOMPILE_HEADERS ON CACHE BOOL "" FORCE)
    ADD_SUBDIRECTORY(dependency/SDL)
    # Re-enable precompiled headers for other projects
    SET(CMAKE_DISABLE_PRECOMPILE_HEADERS OFF CACHE BOOL "" FORCE)
ENDIF()

############################################
# libconfig (bundled)
############################################
ADD_SUBDIRECTORY(dependency/libconfig)

############################################
# Animation Compression Library - ACL
############################################
#ADD_SUBDIRECTORY(dependency/acl)

############################################
# Catch2 (system or bundled)
############################################
IF(SEDX_USE_SYSTEM_CATCH2)
    FIND_PACKAGE(Catch2 CONFIG REQUIRED)
ELSE()
    ADD_SUBDIRECTORY(dependency/Catch2)
ENDIF()

############################################
# EnTT (system or bundled)
############################################
IF(SEDX_USE_SYSTEM_ENTT)
    FIND_PACKAGE(EnTT CONFIG REQUIRED)
ELSE()
    ADD_SUBDIRECTORY(dependency/entt)
ENDIF()

############################################
# nlohmann/json (system or bundled)
############################################
IF(SEDX_USE_SYSTEM_JSON)
    FIND_PACKAGE(nlohmann_json CONFIG REQUIRED)
    # Create compatibility alias to match in-tree target name when using system package
    IF(TARGET nlohmann_json::nlohmann_json AND NOT TARGET nlohmann_json)
        ADD_LIBRARY(nlohmann_json INTERFACE IMPORTED)
        TARGET_LINK_LIBRARIES(nlohmann_json INTERFACE nlohmann_json::nlohmann_json)
    ENDIF()
ELSE()
    ADD_SUBDIRECTORY(dependency/json)
ENDIF()

############################################
# Testing setup (CTest + project tests)
############################################
INCLUDE(CTest)
IF(SEDX_ENABLE_TESTS)
    SET(BUILD_TESTING ON CACHE BOOL "" FORCE)
    ENABLE_TESTING()
    ADD_SUBDIRECTORY(${CMAKE_SOURCE_DIR}/source/Tests)
ELSE()
    SET(BUILD_TESTING OFF CACHE BOOL "" FORCE)
ENDIF()

############################################
# Math Library (xMath via FetchContent)
############################################
IF(SEDX_USE_FETCHCONTENT_XMATH AND CMAKE_VERSION VERSION_GREATER_EQUAL 3.11)
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
ENDIF()

############################################
# edX File Format Library (edX via FetchContent)
############################################
IF(SEDX_USE_FETCHCONTENT_EDX AND CMAKE_VERSION VERSION_GREATER_EQUAL 3.11)
    FETCHCONTENT_DECLARE(
        edX
        GIT_REPOSITORY https://github.com/Coalition-of-Freeware-Developers/edX-Library.git
        GIT_TAG main
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
        GIT_SUBMODULES ""
    )
    FETCHCONTENT_GETPROPERTIES(edX)
    IF(NOT edx_POPULATED)
        FETCHCONTENT_POPULATE(edX)
        IF(EXISTS "${edx_SOURCE_DIR}/cmake/edx.cmake")
            FILE(READ "${edx_SOURCE_DIR}/cmake/edx.cmake" EDX_CMAKE)
            STRING(REPLACE "CMAKE_SOURCE_DIR" "CMAKE_CURRENT_SOURCE_DIR" EDX_CMAKE "${EDX_CMAKE}")
            FILE(WRITE "${edx_SOURCE_DIR}/cmake/edx.cmake" "${EDX_CMAKE}")
        ENDIF()
        # Make edX tolerant of pre-existing nlohmann_json target from our bundled submodule
        IF(EXISTS "${edx_SOURCE_DIR}/CMakeLists.txt")
            FILE(READ "${edx_SOURCE_DIR}/CMakeLists.txt" _EDX_TOP)
            # Replace uppercase FIND_PACKAGE call
            STRING(REPLACE "FIND_PACKAGE(nlohmann_json CONFIG REQUIRED)"
                           "IF(NOT TARGET nlohmann_json AND NOT TARGET nlohmann_json::nlohmann_json)\n    FIND_PACKAGE(nlohmann_json CONFIG REQUIRED)\nENDIF()"
                           _EDX_TOP "${_EDX_TOP}")
            # Replace lowercase find_package call if present
            STRING(REPLACE "find_package(nlohmann_json CONFIG REQUIRED)"
                           "if(NOT TARGET nlohmann_json AND NOT TARGET nlohmann_json::nlohmann_json)\n    find_package(nlohmann_json CONFIG REQUIRED)\nendif()"
                           _EDX_TOP "${_EDX_TOP}")
            FILE(WRITE "${edx_SOURCE_DIR}/CMakeLists.txt" "${_EDX_TOP}")
        ENDIF()
        ADD_SUBDIRECTORY("${edx_SOURCE_DIR}" "${edx_BINARY_DIR}")
    ENDIF()
ENDIF()

############################################
# X-Plane Scenery Library (XPLIB via FetchContent)
############################################
IF(SEDX_USE_FETCHCONTENT_XPLIB AND CMAKE_VERSION VERSION_GREATER_EQUAL 3.11)
    FETCHCONTENT_DECLARE(
        XPSceneryLib
        GIT_REPOSITORY https://github.com/Coalition-of-Freeware-Developers/X-PlaneSceneryLibrary.git
        GIT_TAG main
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
        GIT_SUBMODULES ""
    )
    FETCHCONTENT_GETPROPERTIES(XPSceneryLib)
    IF(NOT xpscenerylib_POPULATED)
        FETCHCONTENT_POPULATE(XPSceneryLib)
        IF(EXISTS "${xpscenerylib_SOURCE_DIR}/cmake/xplib.cmake")
            FILE(READ "${xpscenerylib_SOURCE_DIR}/cmake/xplib.cmake" XPSCENERYLIB_CMAKE)

            # Normalize variables to be relative to the xplib CMakeLists location
            STRING(REPLACE "CMAKE_SOURCE_DIR" "CMAKE_CURRENT_SOURCE_DIR" XPSCENERYLIB_CMAKE "${XPSCENERYLIB_CMAKE}")

            # Do not clear platform sources after appending the .rc (fix upstream reset bug)
            STRING(REPLACE "SET(XPLIB_PLATFORM_SOURCES)" "# (patched) Avoid clearing XPLIB_PLATFORM_SOURCES" XPSCENERYLIB_CMAKE "${XPSCENERYLIB_CMAKE}")

            # Ensure source files are actually discovered from xplib/src and headers from xplib/includes
            STRING(REPLACE "FILE(GLOB XPLIB_HEADER_FILES *.h *.hpp)"
                           "FILE(GLOB_RECURSE XPLIB_HEADER_FILES\n    \"${CMAKE_CURRENT_SOURCE_DIR}/xplib/includes/*.h\"\n    \"${CMAKE_CURRENT_SOURCE_DIR}/xplib/includes/*.hpp\"\n)"
                           XPSCENERYLIB_CMAKE "${XPSCENERYLIB_CMAKE}")

            STRING(REPLACE "FILE(GLOB XPLIB_SOURCE_FILES *.cpp)"
                           "FILE(GLOB_RECURSE XPLIB_SOURCE_FILES\n    \"${CMAKE_CURRENT_SOURCE_DIR}/xplib/src/*.cpp\"\n)"
                           XPSCENERYLIB_CMAKE "${XPSCENERYLIB_CMAKE}")

            # Remove stray RESOURCE_FILE reference if not defined
            STRING(REPLACE "${RESOURCE_FILE}" "" XPSCENERYLIB_CMAKE "${XPSCENERYLIB_CMAKE}")

            # Rename target to a single canonical name: XPSceneryLib
            STRING(REPLACE "ADD_LIBRARY(X-PlaneSceneryLibrary" "ADD_LIBRARY(XPSceneryLib" XPSCENERYLIB_CMAKE "${XPSCENERYLIB_CMAKE}")
            STRING(REPLACE "TARGET_COMPILE_DEFINITIONS(X-PlaneSceneryLibrary" "TARGET_COMPILE_DEFINITIONS(XPSceneryLib" XPSCENERYLIB_CMAKE "${XPSCENERYLIB_CMAKE}")
            STRING(REPLACE "TARGET_INCLUDE_DIRECTORIES(X-PlaneSceneryLibrary" "TARGET_INCLUDE_DIRECTORIES(XPSceneryLib" XPSCENERYLIB_CMAKE "${XPSCENERYLIB_CMAKE}")
            STRING(REPLACE "SET_TARGET_PROPERTIES(X-PlaneSceneryLibrary" "SET_TARGET_PROPERTIES(XPSceneryLib" XPSCENERYLIB_CMAKE "${XPSCENERYLIB_CMAKE}")
            STRING(REPLACE "TARGET X-PlaneSceneryLibrary" "TARGET XPSceneryLib" XPSCENERYLIB_CMAKE "${XPSCENERYLIB_CMAKE}")
            STRING(REPLACE "TARGET_COMPILE_OPTIONS(X-PlaneSceneryLibrary" "TARGET_COMPILE_OPTIONS(XPSceneryLib" XPSCENERYLIB_CMAKE "${XPSCENERYLIB_CMAKE}")

            # Fix wrong target name in UTF-8 compile option
            STRING(REPLACE "TARGET_COMPILE_OPTIONS(xMath PRIVATE /utf-8)" "TARGET_COMPILE_OPTIONS(XPSceneryLib PRIVATE /utf-8)" XPSCENERYLIB_CMAKE "${XPSCENERYLIB_CMAKE}")

            # Ensure CMake can determine the linker language and set output name consistently
            STRING(REPLACE "SET_TARGET_PROPERTIES(XPSceneryLib PROPERTIES"
                           "SET_TARGET_PROPERTIES(XPSceneryLib PROPERTIES\n    LINKER_LANGUAGE CXX\n    OUTPUT_NAME \"XPSceneryLib\""
                           XPSCENERYLIB_CMAKE "${XPSCENERYLIB_CMAKE}")

            FILE(WRITE "${xpscenerylib_SOURCE_DIR}/cmake/xplib.cmake" "${XPSCENERYLIB_CMAKE}")
        ENDIF()
        # Upstream may unconditionally add_subdirectory(tests). If it's absent, stub it.
        IF(NOT EXISTS "${xpscenerylib_SOURCE_DIR}/tests/CMakeLists.txt")
            FILE(MAKE_DIRECTORY "${xpscenerylib_SOURCE_DIR}/tests")
            FILE(WRITE "${xpscenerylib_SOURCE_DIR}/tests/CMakeLists.txt" "# Stubbed tests for XPSceneryLib\n")
        ENDIF()
        # Ensure top-level continues to reference XPSceneryLib (no hyphen) consistently
        IF(EXISTS "${xpscenerylib_SOURCE_DIR}/CMakeLists.txt")
            FILE(READ "${xpscenerylib_SOURCE_DIR}/CMakeLists.txt" _XPL_TOP)
            STRING(REPLACE "IF(TARGET X-PlaneSceneryLibrary)" "IF(TARGET XPSceneryLib)" _XPL_TOP "${_XPL_TOP}")
            STRING(REPLACE "TARGET_INCLUDE_DIRECTORIES(X-PlaneSceneryLibrary" "TARGET_INCLUDE_DIRECTORIES(XPSceneryLib" _XPL_TOP "${_XPL_TOP}")
            STRING(REPLACE "SET_PROPERTY(TARGET X-PlaneSceneryLibrary" "SET_PROPERTY(TARGET XPSceneryLib" _XPL_TOP "${_XPL_TOP}")
            STRING(REPLACE "FOREACH(TARGET IN ITEMS X-PlaneSceneryLibrary)" "FOREACH(TARGET IN ITEMS XPSceneryLib)" _XPL_TOP "${_XPL_TOP}")
            STRING(REPLACE "TARGET_COMPILE_OPTIONS(X-PlaneSceneryLibrary" "TARGET_COMPILE_OPTIONS(XPSceneryLib" _XPL_TOP "${_XPL_TOP}")
            STRING(REPLACE "TARGET_COMPILE_DEFINITIONS(X-PlaneSceneryLibrary" "TARGET_COMPILE_DEFINITIONS(XPSceneryLib" _XPL_TOP "${_XPL_TOP}")
            STRING(REPLACE "ALIAS X-PlaneSceneryLibrary" "ALIAS XPSceneryLib" _XPL_TOP "${_XPL_TOP}")
            STRING(REPLACE "ADD_LIBRARY(XPSceneryLib::XPSceneryLib ALIAS XPSceneryLib)" "ADD_LIBRARY(XPSceneryLib::XPSceneryLib ALIAS XPSceneryLib)" _XPL_TOP "${_XPL_TOP}")
            FILE(WRITE "${xpscenerylib_SOURCE_DIR}/CMakeLists.txt" "${_XPL_TOP}")
        ENDIF()
        ADD_SUBDIRECTORY("${xpscenerylib_SOURCE_DIR}" "${xpscenerylib_BINARY_DIR}")
    ENDIF()
ENDIF()

# Ensure edX links against XPSceneryLib when available; otherwise fallback
IF(TARGET edX AND TARGET XPSceneryLib)
    TARGET_LINK_LIBRARIES(edX PUBLIC XPSceneryLib)
ENDIF()

############################################
# ImGui (system or bundled sources)
############################################
IF(SEDX_BUILD_IMGUI)
    ADD_LIBRARY(imgui
        ${CMAKE_SOURCE_DIR}/dependency/imgui/imgui.cpp
        ${CMAKE_SOURCE_DIR}/dependency/imgui/imgui_draw.cpp
        ${CMAKE_SOURCE_DIR}/dependency/imgui/imgui_widgets.cpp
        ${CMAKE_SOURCE_DIR}/dependency/imgui/imgui_tables.cpp
        $<$<BOOL:${SEDX_WITH_IMGUI_DEMO}>:${CMAKE_SOURCE_DIR}/dependency/imgui/imgui_demo.cpp>
        ${CMAKE_SOURCE_DIR}/dependency/imgui/backends/imgui_impl_sdl3.cpp
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
            SDL3::SDL3
            Vulkan::Vulkan
            GPUOpen::VulkanMemoryAllocator
    )
ENDIF()

############################################
# AppCore
############################################
ADD_SUBDIRECTORY(source/SceneryEditorX)

############################################
# Launcher
############################################
ADD_SUBDIRECTORY(source/Launcher)

############################################
# Scenery Editor X
############################################
ADD_SUBDIRECTORY(source/Editor)

############################################
# Crash Handler Service
############################################
ADD_SUBDIRECTORY(source/CrashHandler)

############################################
# Precompiled Headers
############################################
MESSAGE(STATUS "=================================================")
MESSAGE(STATUS "Generating precompiled headers")

INCLUDE(${CMAKE_SOURCE_DIR}/cmake/pch.cmake)
INCLUDE(${CMAKE_SOURCE_DIR}/cmake/sedx_targets.cmake)

SEDX_ENABLE_PCH(SceneryEditorX ${CMAKE_SOURCE_DIR}/source/Editor/xpeditor_pch.h)
SEDX_ENABLE_PCH(AppCore        ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/app_pch.h)
SEDX_ENABLE_PCH(Launcher       ${CMAKE_SOURCE_DIR}/source/Launcher/startup_pch.h)

# Group and output dirs
SEDX_GROUP_TARGETS()
## Configure per-target output directories using ${CMAKE_CFG_INTDIR} once
## Note: Avoid mixing target properties with global per-config variables to prevent
## paths like lib/Debug/Debug from occurring (especially with multi-config generators).
SEDX_APPLY_OUTPUT_DIRS(${LIBS_DIR} ${BIN_DIR})

############################################
# Set Command Line Definitions
############################################
## Configure Visual Studio debugger to pass `--verbose` to our executables
# If the CMake option SEDX_VERBOSE_LOGGING is enabled and the build config is Debug,
# pass the `--verbose` CLI flag to the debugger so developers get verbose logging
# automatically when launching from the IDE.
FOREACH(_sedx_dbg_target SceneryEditorX Launcher CrashHandler)
    IF(TARGET ${_sedx_dbg_target})
        # Only add --verbose when building Debug and SEDX_VERBOSE_LOGGING is true
        SET_TARGET_PROPERTIES(${_sedx_dbg_target} PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "$<$<AND:$<CONFIG:Debug>,$<BOOL:${SEDX_VERBOSE_LOGGING}>>:--verbose>" )
    ENDIF()
ENDFOREACH()

# Ensure debugger working directory is set for startup target (after it exists)
IF(TARGET SceneryEditorX)
    SEDX_SET_DEBUGGER_WD(SceneryEditorX)
ENDIF()

############################################

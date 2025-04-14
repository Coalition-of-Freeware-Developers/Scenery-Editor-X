CMAKE_MINIMUM_REQUIRED(VERSION 3.8...4.0)

# --------------------------------
# Global Setup
# --------------------------------
SET(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/dependency/vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "")
SET(CMAKE_CONFIGURATION_TYPES Debug Release CACHE STRING INTERNAL FORCE)
SET(CMAKE_SYSTEM_NAME "Windows" CACHE STRING "")
SET(CMAKE_SYSTEM_PROCESSOR "x64" CACHE STRING "")
SET(CMAKE_SYSTEM_VERSION 10.0 CACHE STRING "")
SET(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION "10.0" CACHE STRING "")
SET(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1 CACHE INTERNAL "No dev warnings")
SET(VCPKG_INSTALL_OPTIONS --no-print-usage)
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)
SET_PROPERTY(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT SceneryEditorX)
SET_PROPERTY(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "_CMakePredefinedTargets")
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

SET(BIN_DIR ${CMAKE_SOURCE_DIR}/bin/${CMAKE_CFG_INTDIR})
SET(LIBS_DIR ${CMAKE_SOURCE_DIR}/lib/${CMAKE_CFG_INTDIR})
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/")

# USE FINDVULKAN MODULE ADDED WITH CMAKE 3.7
IF(NOT CMAKE_VERSION VERSION_LESS 3.7.0)
    MESSAGE(STATUS "Using module to find Vulkan")
    FIND_PACKAGE(Vulkan)
    SET(VULKAN_VERS ${Vulkan_FOUND})
    MESSAGE(STATUS "Using Vulkan SDK version: ${VULKAN_VERS}")
ENDIF()

IF(TARGET WIN32)
    FILE(VULKAN_VALIDATOR "C:/VulkanSDK/${VULKAN_VERS}/Config/vk_layer_settings.txt")
    SET(VK_USE_PLATFORM_WIN32_KHR)
ELSEIF(TARGET UNIX)
    FILE(VULKAN_VALIDATOR "$HOME/.local/share/vulkan/settings.d/vk_layer_settings.txt")
ENDIF()

# --------------------------------
# DIRECTORIES
# --------------------------------
SET(ASSET_DIR   "${CMAKE_SOURCE_DIR}/assets")
SET(FONT_DIR    "${CMAKE_SOURCE_DIR}/assets/fonts")
SET(MODEL_DIR   "${CMAKE_SOURCE_DIR}/assets/models")
SET(SHADER_DIR  "${CMAKE_SOURCE_DIR}/assets/shaders")
SET(TEXTURE_DIR "${CMAKE_SOURCE_DIR}/assets/textures")

# --------------------------------
# BUILD SYSTEM
# --------------------------------
SET(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_STANDARD_REQUIRED True)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)
SET(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS}
    /EHsc
	/Zc:preprocessor
    -DVK_USE_PLATFORM_WIN32_KHR
    -DNOMINMAX
    -D_USE_MATH_DEFINES "
)
ADD_DEFINITIONS(
    -D_CRT_SECURE_NO_WARNINGS
    -DCMAKE_GENERATOR_PLATFORM=x64
)
MESSAGE(VCPKG_ROOT = " ${CMAKE_TOOLCHAIN_FILE} ")

# --------------------------------
IF(CMAKE_GENERATOR MATCHES "Visual Studio")
    ADD_COMPILE_OPTIONS(${COMMON_COMPILE_OPTIONS})
    SET(COMPILE_FLAGS ${COMMON_COMPILE_FLAGS})
    SET(CMAKE_CXX_SCAN_FOR_MODULES OFF)
    SET(CMAKE_INCREMENTAL_LINKING ON)
    SET(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL")
    ADD_COMPILE_DEFINITIONS(${COMMON_COMPILE_DEF})
    MESSAGE("Added Compile Definitions: ${COMMON_COMPILE_DEF}")
ENDIF()

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

# --------------------------------
# Scenery Editor X Project Setup
# --------------------------------
PROJECT(SceneryEditorX
    VERSION 1.0.0
    DESCRIPTION "A 3d scenery editor for X-Plane 12 airports."
    LANGUAGES CXX C
    HOMEPAGE_URL "https://github.com/The3dVehicleguy/Scenery-Editor-X"
)

MESSAGE(STATUS "=================================================")
MESSAGE(STATUS "Beginning project generation for Scenery Editor X")
MESSAGE(STATUS "=================================================")

SET(FONT_FILES
    ${FONT_DIR}/industry/Industry-Bold.otf
    ${FONT_DIR}/industry/Industry-Book.otf
    ${FONT_DIR}/industry/Industry-Medium.otf
    ${FONT_DIR}/industry/Industry-Thin.otf
    ${FONT_DIR}/opensans/OpenSans-Bold.ttf
    ${FONT_DIR}/opensans/OpenSans-BoldItalic.ttf
    ${FONT_DIR}/opensans/OpenSans-ExtraBold.ttf
    ${FONT_DIR}/opensans/OpenSans-ExtraBoldItalic.ttf
    ${FONT_DIR}/opensans/OpenSans-Italic.ttf
    ${FONT_DIR}/opensans/OpenSans-Light.ttf
    ${FONT_DIR}/opensans/OpenSans-LightItalic.ttf
    ${FONT_DIR}/opensans/OpenSans-Regular.ttf
    ${FONT_DIR}/opensans/OpenSans-SemiBold.ttf
    ${FONT_DIR}/opensans/OpenSans-SemiBoldItalic.ttf
)

SET(REGISTRY_KEYS
    ${CMAKE_SOURCE_DIR}/source/Launcher/Registry/edXLibReg.rgs
    ${CMAKE_SOURCE_DIR}/source/Launcher/Registry/edXReg.rgs
    ${CMAKE_SOURCE_DIR}/source/Launcher/Registry/ProgramReg.rgs
)

SET(ICON_FILES
    ${ASSET_DIR}/icon.ico
    ${ASSET_DIR}/icon.png
    ${ASSET_DIR}/icon_edX.ico
    ${ASSET_DIR}/icon_testing.ico
    ${ASSET_DIR}/icon_testing.png
)

# --------------------------------
# PACKAGES
# --------------------------------
FIND_PACKAGE(Vulkan                        REQUIRED)
FIND_PACKAGE(glm                    CONFIG REQUIRED)
FIND_PACKAGE(spdlog                 CONFIG REQUIRED)
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
# EDX
# --------------------------------
ADD_SUBDIRECTORY(source/edX)

# --------------------------------
# FILE FORMAT TESTER
# --------------------------------
ADD_SUBDIRECTORY(source/edX/testing)

# --------------------------------
# X-PLANE SCENERY LIBRARY
# --------------------------------
ADD_SUBDIRECTORY(source/X-PlaneSceneryLibrary)

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
		glm::glm-header-only
		GPUOpen::VulkanMemoryAllocator
)

# --------------------------------
# SCENERY EDITOR X
# --------------------------------
ADD_SUBDIRECTORY(source/SceneryEditorX)

FOREACH(TARGET IN ITEMS SceneryEditorX
        PUBLIC
            $<$<CONFIG:Debug>:SEDX_DEBUG>
            $<$<CONFIG:Release>:SEDX_RELEASE>
            UNICODE
            _UNICODE
            _CRT_SECURE_NO_WARNINGS
            #WIN32_LEAN_AND_MEAN
    )
ENDFOREACH()

# --------------------------------
# PRECOMPILED HEADER
# --------------------------------
MESSAGE(STATUS "=================================================")
MESSAGE(STATUS "Generating precompiled headers")

TARGET_PRECOMPILE_HEADERS(SceneryEditorX PRIVATE ${CMAKE_SOURCE_DIR}/Source/SceneryEditorX/xpeditor_pch.h)

SET_PROPERTY(TARGET FormatTester PROPERTY FOLDER "Tools")
SET_PROPERTY(TARGET edX X-PlaneSceneryLibrary PROPERTY FOLDER "File Formats")
SET_PROPERTY(TARGET glfw docs uninstall update_mappings PROPERTY FOLDER "Dependency/GLFW3")
SET_PROPERTY(TARGET imgui PROPERTY FOLDER "Dependency")

FOREACH(TARGET IN ITEMS SceneryEditorX FormatTester imgui edX X-PlaneSceneryLibrary glfw)
    SET_TARGET_PROPERTIES(${TARGET} PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY ${LIBS_DIR}
        LIBRARY_OUTPUT_DIRECTORY ${LIBS_DIR}
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
    )
ENDFOREACH()

# --------------------------------
FOREACH(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/lib/${OUTPUTCONFIG})
    SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/lib/${OUTPUTCONFIG})
    SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/bin/${OUTPUTCONFIG})
ENDFOREACH(OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES)

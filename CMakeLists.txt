cmake_minimum_required(VERSION 3.22..)

# --------------------------------
# Build System
# --------------------------------
set(CMAKE_CONFIGURATION_TYPES Debug Release CACHE TYPE INTERNAL FORCE)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/dependency/vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "")

# --------------------------------
# Global Setup
# --------------------------------
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
    if(CMAKE_GENERATOR MATCHES "Visual Studio")
        add_compile_options("/MP")
    endif()
endif()

# --------------------------------
# Scenery Editor X Project Setup
# --------------------------------

project(SceneryEditorX VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(FONT_FILES
    assets/fonts/industry/Industry-Bold.otf
    assets/fonts/industry/Industry-Book.otf
    assets/fonts/industry/Industry-Medium.otf
    assets/fonts/industry/Industry-Thin.otf
    assets/fonts/opensans/OpenSans-Bold.ttf
    assets/fonts/opensans/OpenSans-BoldItalic.ttf
    assets/fonts/opensans/OpenSans-ExtraBold.ttf
    assets/fonts/opensans/OpenSans-ExtraBoldItalic.ttf
    assets/fonts/opensans/OpenSans-Italic.ttf
    assets/fonts/opensans/OpenSans-Light.ttf
    assets/fonts/opensans/OpenSans-LightItalic.ttf
    assets/fonts/opensans/OpenSans-Regular.ttf
    assets/fonts/opensans/OpenSans-SemiBold.ttf
    assets/fonts/opensans/OpenSans-SemiBoldItalic.ttf
)

set(REGISTRY_KEYS
    source/SceneryEditorX/Registry/edXLibReg.rgs
    source/SceneryEditorX/Registry/edXReg.rgs
    source/SceneryEditorX/Registry/ProgramReg.rgs
)

set(ICON_FILES
    assets/icon.ico
    assets/icon_edX.ico
    assets/icon_testing.ico
)

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

set(EDX_HEADER_FILES
    source/edX/edXLibraryFile.h
    source/edX/edXProjectFile.h
    source/edX/resource.h
)

set(EDX_SOURCE_FILES
    source/edX/edXWriter.cpp
    source/edX/edXReader.cpp
    source/edX/edXLibraryWriter.cpp
    source/edX/edXLibraryReader.cpp
)

source_group("Project Format" FILES
    source/edX/edXProjectFile.h
    source/edX/edXWriter.cpp
    source/edX/edXReader.cpp
)

source_group("Library Format" FILES
    source/edX/edXLibraryFile.h
    source/edX/edXLibraryWriter.cpp
    source/edX/edXLibraryReader.cpp
)

source_group("Resource" FILES
    source/edX/resource.h
    source/edX/edX.rc
)

add_library(edX SHARED ${EDX_HEADER_FILES} ${EDX_SOURCE_FILES}
    source/edX/edXWriter.cpp
    source/edX/edXReader.cpp
    source/edX/edXLibraryWriter.cpp
    source/edX/edXLibraryReader.cpp
    source/edX/edX.rc
)

# --------------------------------
# File Format Tester
# --------------------------------

add_executable(FormatTester
    source/edX/testing/main.cpp
    source/edX/testing/FileBrowser.cpp
    source/edX/testing/FileBrowser.hpp
    source/edX/testing/FormatTester.rc
    source/edX/testing/resource.h
    source/edX/edXProjectFile.h
)

target_link_libraries(FormatTester PRIVATE
    glm::glm-header-only
    spdlog::spdlog_header_only
    imgui::imgui
    glfw
    dxgi
    d3d12
)

target_include_directories(FormatTester PRIVATE
    source/SceneryEditorX/
    ${PORTABLE_FILE_DIALOGS_INCLUDE_DIRS}
    $ENV{VULKAN_SDK}/Include/
)

# --------------------------------
# X-PlaneSceneryLibrary
# --------------------------------

set(XPLIB_HEADER_FILES
    source/X-PlaneSceneryLibrary/XPObj.h
    source/X-PlaneSceneryLibrary/XPLibrarySystem.h
    source/X-PlaneSceneryLibrary/XPLibraryPath.h
    source/X-PlaneSceneryLibrary/XPLayerGroups.h
    source/X-PlaneSceneryLibrary/X-PlaneSceneryLibraryUtils.h
    source/X-PlaneSceneryLibrary/X-PlaneSceneryLibraryParsers.h
    source/X-PlaneSceneryLibrary/XPAsset.h
    source/X-PlaneSceneryLibrary/TextUtils.h
    source/X-PlaneSceneryLibrary/resource.h
)

set(XPLIB_SOURCE_FILES
    source/X-PlaneSceneryLibrary/DebugMain.cpp
    source/X-PlaneSceneryLibrary/TextUtils.cpp
    source/X-PlaneSceneryLibrary/XPLayerGroups.cpp
    source/X-PlaneSceneryLibrary/XPLibrarySystem.cpp
    source/X-PlaneSceneryLibrary/XPObj.cpp
)

source_group("Header Files" FILES
    source/X-PlaneSceneryLibrary/XPObj.h
    source/X-PlaneSceneryLibrary/XPLibrarySystem.h
    source/X-PlaneSceneryLibrary/XPLibraryPath.h
    source/X-PlaneSceneryLibrary/XPLayerGroups.h
    source/X-PlaneSceneryLibrary/X-PlaneSceneryLibraryUtils.h
    source/X-PlaneSceneryLibrary/X-PlaneSceneryLibraryParsers.h
    source/X-PlaneSceneryLibrary/XPAsset.h
    source/X-PlaneSceneryLibrary/TextUtils.h
)

source_group("Source Files" FILES
    source/X-PlaneSceneryLibrary/TextUtils.cpp
    source/X-PlaneSceneryLibrary/XPLayerGroups.cpp
    source/X-PlaneSceneryLibrary/XPLibrarySystem.cpp
    source/X-PlaneSceneryLibrary/XPObj.cpp
)

source_group("Resource" FILES
    source/X-PlaneSceneryLibrary/resource.h
    source/X-PlaneSceneryLibrary/X-PlaneSceneryLibrary.rc
)

source_group("Tests" FILES
    source/X-PlaneSceneryLibrary/DebugMain.cpp
)

add_library(X-PlaneSceneryLibrary
    ${XPLIB_HEADER_FILES}
    ${XPLIB_SOURCE_FILES}
    source/X-PlaneSceneryLibrary/DebugMain.cpp
    source/X-PlaneSceneryLibrary/TextUtils.cpp
    source/X-PlaneSceneryLibrary/XPLayerGroups.cpp
    source/X-PlaneSceneryLibrary/XPLibrarySystem.cpp
    source/X-PlaneSceneryLibrary/XPObj.cpp
    source/X-PlaneSceneryLibrary/X-PlaneSceneryLibrary.rc
)

# --------------------------------
# SceneryEditorX | Launcher
# --------------------------------

source_group("Resource" FILES
    ${ICON_FILES}
    ${REGISTRY_KEYS}
    assets/splash_screen.bmp
    source/Launcher/resource.h
    source/Launcher/Launcher.rc
)

source_group("Core" FILES
    source/Launcher/Main.cpp
    source/SceneryEditorX/version.h
    source/Launcher/Logging.cpp
    source/Launcher/DirectoryManager.cpp
    source/Launcher/SplashHandler.h
    source/Launcher/Updater.h
    source/Launcher/Updater.cpp
)

source_group("Registry" FILES
    source/Launcher/RegInit.h
    source/Launcher/RegCheck.h
    source/Launcher/ElevatePerms.h
)

add_executable(Launcher
    ${ICON_FILES}
    ${REGISTRY_KEYS}
    assets/splash_screen.bmp
    source/Launcher/Main.cpp
    source/Launcher/resource.h
    source/Launcher/Launcher.rc
    source/Launcher/Logging.cpp
    source/Launcher/RegInit.h
    source/Launcher/RegCheck.h
    source/Launcher/ElevatePerms.h
    source/Launcher/DirectoryManager.cpp
    source/Launcher/SplashHandler.h
    source/Launcher/Updater.h
    source/Launcher/Updater.cpp
    source/SceneryEditorX/version.h
)

target_link_libraries(Launcher PRIVATE
    spdlog::spdlog_header_only
    dxgi
    d3d12
    CURL::libcurl
    Shell32.lib
)

target_include_directories(Launcher PRIVATE
    source/SceneryEditorX/Logging
    source/SceneryEditorX/
    ${CURL_INCLUDE_DIRS}
)

target_link_directories(Launcher PRIVATE ${CURL_LIBRARY_DIRS})

target_link_options(Launcher PRIVATE /SUBSYSTEM:WINDOWS)

# --------------------------------
# VulkanRender
# --------------------------------

add_library(VulkanRenderer
    source/VulkanRenderer/benchmark.hpp
    source/VulkanRenderer/camera.hpp
    source/VulkanRenderer/CommandLineParser.hpp
    source/VulkanRenderer/Entrypoints.h
    source/VulkanRenderer/frustum.hpp
    source/VulkanRenderer/keycodes.hpp
    source/VulkanRenderer/main.cpp
    source/VulkanRenderer/threadpool.hpp
    source/VulkanRenderer/VulkanAndroid.cpp
    source/VulkanRenderer/VulkanAndroid.h
    source/VulkanRenderer/VulkanBuffer.cpp
    source/VulkanRenderer/VulkanBuffer.h
    source/VulkanRenderer/VulkanDebug.cpp
    source/VulkanRenderer/VulkanDebug.h
    source/VulkanRenderer/VulkanDevice.cpp
    source/VulkanRenderer/VulkanDevice.h
    source/VulkanRenderer/vulkanexamplebase.cpp
    source/VulkanRenderer/vulkanexamplebase.h
    source/VulkanRenderer/VulkanFrameBuffer.hpp
    source/VulkanRenderer/VulkanglTFModel.cpp
    source/VulkanRenderer/VulkanglTFModel.h
    source/VulkanRenderer/VulkanInitializers.hpp
    source/VulkanRenderer/VulkanRaytracingSample.cpp
    source/VulkanRenderer/VulkanRaytracingSample.h
    source/VulkanRenderer/VulkanSwapChain.cpp
    source/VulkanRenderer/VulkanSwapChain.h
    source/VulkanRenderer/VulkanTexture.cpp
    source/VulkanRenderer/VulkanTexture.h
    source/VulkanRenderer/VulkanTools.cpp
    source/VulkanRenderer/VulkanTools.h
    source/VulkanRenderer/VulkanUIOverlay.cpp
    source/VulkanRenderer/VulkanUIOverlay.h
)

target_link_libraries(VulkanRenderer PRIVATE
    glm::glm-header-only
    spdlog::spdlog_header_only
    imgui::imgui
    glfw
    dxgi
    d3d12
    Shell32.lib
)

target_include_directories(VulkanRenderer PRIVATE
    source/VulkanRenderer/
    dependency/optick/src/
    ${PORTABLE_FILE_DIALOGS_INCLUDE_DIRS}
    $ENV{VULKAN_SDK}/Include/
    ${Stb_INCLUDE_DIR}
)

target_link_options(VulkanRenderer PRIVATE /SUBSYSTEM:WINDOWS)

# --------------------------------
# SceneryEditorX
# --------------------------------

# Glob the optick sources and compile the executable with them directly.
file(GLOB OPTICK_SOURCES dependency/optick/src/*.cpp)

# file(GLOB IMGUI_INCLUDES Dependency/optick/src/*.cpp)
add_executable(SceneryEditorX
    ${OPTICK_SOURCES}
    ${ICON_FILES}
    ${REGISTRY_KEYS}
    ${FONT_FILES}

    source/SceneryEditorX/Main.cpp
    source/SceneryEditorX/Core/Application.cpp
    source/SceneryEditorX/Core/Profiler.h
    source/SceneryEditorX/Core/VK_Window.cpp
    source/SceneryEditorX/Core/Window.hpp
    source/SceneryEditorX/Platform/Windows/FileManager/FileDialog.cpp
    source/SceneryEditorX/Platform/Windows/FileManager/FileDialog.h
    source/SceneryEditorX/Platform/Windows/FileManager/LockFile.cpp
    source/SceneryEditorX/Platform/Windows/EditorConfig.hpp
    source/SceneryEditorX/Logging/Logging.cpp
    source/SceneryEditorX/Renderer/Camera.cpp
    source/SceneryEditorX/Renderer/VK_Pipeline.cpp
    source/SceneryEditorX/UI/About.cpp
    source/SceneryEditorX/UI/ContentBrowser.cpp
    source/SceneryEditorX/UI/Fonts.h
    source/SceneryEditorX/UI/LayerStack.cpp
    source/SceneryEditorX/UI/MenuBar.cpp
    source/SceneryEditorX/UI/Modals.cpp
    source/SceneryEditorX/UI/SettingsPanel.cpp
    source/SceneryEditorX/UI/Styles.cpp
    source/SceneryEditorX/UI/UI.h
    source/SceneryEditorX/Options/Settings.cpp
    source/SceneryEditorX/SceneryEditorX.rc
    source/SceneryEditorX/resource.h
    source/SceneryEditorX/version.h
)

source_group("External/Optick" FILES ${OPTICK_SOURCES})

source_group("Core" FILES
    source/SceneryEditorX/Main.cpp
    source/SceneryEditorX/Core/Application.cpp
    source/SceneryEditorX/Core/DirectoryManager.cpp
    source/SceneryEditorX/Core/Window.hpp
)

source_group("Core/Platform/Windows" FILES
    source/SceneryEditorX/Platform/Windows/FileManager/FileDialog.cpp
    source/SceneryEditorX/Platform/Windows/FileManager/FileDialog.h
    source/SceneryEditorX/Platform/Windows/FileManager/LockFile.cpp
    source/SceneryEditorX/Platform/Windows/EditorConfig.hpp
)

source_group("Core/Logger" FILES
    source/SceneryEditorX/Logging/Logging.cpp
)

source_group("Core/Renderer" FILES
    source/SceneryEditorX/Renderer/Camera.cpp
    source/SceneryEditorX/Renderer/VK_Pipeline.cpp
    source/SceneryEditorX/Core/VK_Window.cpp
)

source_group("Core/Profiler" FILES
    source/SceneryEditorX/Core/Profiler.h
)

source_group("Core/Settings" FILES
    source/SceneryEditorX/Options/Settings.cpp
)

source_group("Core/UI" FILES
    source/SceneryEditorX/UI/About.cpp
    source/SceneryEditorX/UI/ContentBrowser.cpp
    source/SceneryEditorX/UI/Fonts.h
    source/SceneryEditorX/UI/LayerStack.cpp
    source/SceneryEditorX/UI/MenuBar.cpp
    source/SceneryEditorX/UI/Modals.cpp
    source/SceneryEditorX/UI/SettingsPanel.cpp
    source/SceneryEditorX/UI/Styles.cpp
    source/SceneryEditorX/UI/UI.h
)

source_group("Resource" FILES
    source/SceneryEditorX/resource.h
    source/SceneryEditorX/version.h
    source/SceneryEditorX/SceneryEditorX.rc
)

source_group("Resource/Registry" FILES
    ${REGISTRY_KEYS}
)

source_group("Resource/Icons" FILES
    ${ICON_FILES}
)

source_group("Resource/Fonts" FILES
    ${FONT_FILES}
)

# Include
target_include_directories(SceneryEditorX PRIVATE
    source/SceneryEditorX/
    dependency/optick/src/
    ${REGISTRY_KEYS}
    ${PORTABLE_FILE_DIALOGS_INCLUDE_DIRS}
    $ENV{VULKAN_SDK}/Include/
    ${Stb_INCLUDE_DIR}
    ${CURL_INCLUDE_DIRS}
)

# Link
target_link_libraries(SceneryEditorX PRIVATE
    glm::glm-header-only
    spdlog::spdlog_header_only
    imgui::imgui
    imguizmo::imguizmo
    CURL::libcurl
    glfw
    dxgi
    d3d12
    Shell32.lib
)

target_link_directories(SceneryEditorX PRIVATE $ENV{VULKAN_SDK}/Lib/)
target_link_directories(SceneryEditorX PRIVATE ${CURL_INCLUDE_DIRS})

# Define SEDX_DEBUG for Debug builds using generator expressions
target_compile_definitions(Launcher         PRIVATE $<$<CONFIG:Debug>:SEDX_DEBUG> $<$<CONFIG:Release>:SEDX_RELEASE>)
target_compile_definitions(SceneryEditorX   PRIVATE $<$<CONFIG:Debug>:SEDX_DEBUG> $<$<CONFIG:Release>:SEDX_RELEASE>)
target_compile_definitions(VulkanRenderer   PRIVATE $<$<CONFIG:Debug>:SEDX_DEBUG> $<$<CONFIG:Release>:SEDX_RELEASE>)

# --------------------------------
# Defines
# --------------------------------

target_compile_definitions(SceneryEditorX   PRIVATE WIN32_LEAN_AND_MEAN)
target_compile_definitions(FormatTester     PRIVATE WIN32_LEAN_AND_MEAN)
target_compile_definitions(Launcher         PRIVATE WIN32_LEAN_AND_MEAN)
target_compile_definitions(VulkanRenderer   PRIVATE WIN32_LEAN_AND_MEAN)

# --------------------------------
# Precompiled Header
# --------------------------------

target_precompile_headers(SceneryEditorX PRIVATE source/SceneryEditorX/xpeditor_pch.h)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT SceneryEditorX)

set_property(TARGET Launcher PROPERTY FOLDER "Core")
set_property(TARGET VulkanRenderer PROPERTY FOLDER "Core")
set_property(TARGET FormatTester PROPERTY FOLDER "Tools")
set_property(TARGET edX PROPERTY FOLDER "File Formats")
set_property(TARGET X-PlaneSceneryLibrary PROPERTY FOLDER "File Formats")

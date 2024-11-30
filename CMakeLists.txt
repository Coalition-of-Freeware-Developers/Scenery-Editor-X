cmake_minimum_required(VERSION 3.22..)

# --------------------------------
# Build System
# --------------------------------
set(CMAKE_CONFIGURATION_TYPES Debug Release CACHE TYPE INTERNAL FORCE)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/Dependency/vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "")

# --------------------------------
# Boost Librarys
# --------------------------------

# set(Boost_USE_STATIC_LIBS ON)
# set(Boost_INCLUDE_DIR ${_VCPKG_INSTALLED_DIR}/x64-windows-static/include)

# find_package(boost  COMPONENTS array asio chrono filesystem geometry io iostream)
# define  set(Boost_INCLUDE_DIR "path")

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
    Assets/Fonts/industry/Industry-Bold.otf
    Assets/Fonts/industry/Industry-Book.otf
    Assets/Fonts/industry/Industry-Medium.otf
    Assets/Fonts/industry/Industry-Thin.otf
    Assets/Fonts/opensans/OpenSans-Bold.ttf
    Assets/Fonts/opensans/OpenSans-BoldItalic.ttf
    Assets/Fonts/opensans/OpenSans-ExtraBold.ttf
    Assets/Fonts/opensans/OpenSans-ExtraBoldItalic.ttf
    Assets/Fonts/opensans/OpenSans-Italic.ttf
    Assets/Fonts/opensans/OpenSans-Light.ttf
    Assets/Fonts/opensans/OpenSans-LightItalic.ttf
    Assets/Fonts/opensans/OpenSans-Regular.ttf
    Assets/Fonts/opensans/OpenSans-SemiBold.ttf
    Assets/Fonts/opensans/OpenSans-SemiBoldItalic.ttf
)

set(REGISTRY_KEYS
    Source/SceneryEditorX/Registry/edXLibReg.rgs
    Source/SceneryEditorX/Registry/edXReg.rgs
    Source/SceneryEditorX/Registry/ProgramReg.rgs
)

set(ICON_FILES
    Assets/icon.ico
    Assets/icon_edX.ico
    Assets/icon_testing.ico
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
    Source/edX/edXLibraryFile.h
    Source/edX/edXProjectFile.h
    Source/edX/resource.h
)

set(EDX_SOURCE_FILES
    Source/edX/edXWriter.cpp
    Source/edX/edXReader.cpp
    Source/edX/edXLibraryWriter.cpp
    Source/edX/edXLibraryReader.cpp
)

source_group("Project Format" FILES
    Source/edX/edXProjectFile.h
    Source/edX/edXWriter.cpp
    Source/edX/edXReader.cpp
)

source_group("Library Format" FILES
    Source/edX/edXLibraryFile.h
    Source/edX/edXLibraryWriter.cpp
    Source/edX/edXLibraryReader.cpp
)

source_group("Resource" FILES
    Source/edX/resource.h
    Source/edX/edX.rc
)

add_library(edX SHARED ${EDX_HEADER_FILES} ${EDX_SOURCE_FILES}
    Source/edX/edXWriter.cpp
    Source/edX/edXReader.cpp
    Source/edX/edXLibraryWriter.cpp
    Source/edX/edXLibraryReader.cpp
    Source/edX/edX.rc
)

# --------------------------------
# File Format Tester
# --------------------------------

add_executable(FormatTester
    Source/edX/testing/main.cpp
    Source/edX/testing/FileBrowser.cpp
    Source/edX/testing/FileBrowser.hpp
    Source/edX/testing/FormatTester.rc
    Source/edX/testing/resource.h
    Source/edX/edXProjectFile.h
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
    Source/SceneryEditorX/
    ${PORTABLE_FILE_DIALOGS_INCLUDE_DIRS}
    $ENV{VULKAN_SDK}/Include/
)

# --------------------------------
# X-PlaneSceneryLibrary
# --------------------------------

set(XPLIB_HEADER_FILES
    Source/X-PlaneSceneryLibrary/XPObj.h
    Source/X-PlaneSceneryLibrary/XPLibrarySystem.h
    Source/X-PlaneSceneryLibrary/XPLibraryPath.h
    Source/X-PlaneSceneryLibrary/XPLayerGroups.h
    Source/X-PlaneSceneryLibrary/X-PlaneSceneryLibraryUtils.h
    Source/X-PlaneSceneryLibrary/X-PlaneSceneryLibraryParsers.h
    Source/X-PlaneSceneryLibrary/XPAsset.h
    Source/X-PlaneSceneryLibrary/TextUtils.h
    Source/X-PlaneSceneryLibrary/resource.h
)

set(XPLIB_SOURCE_FILES
    Source/X-PlaneSceneryLibrary/DebugMain.cpp
    Source/X-PlaneSceneryLibrary/TextUtils.cpp
    Source/X-PlaneSceneryLibrary/XPLayerGroups.cpp
    Source/X-PlaneSceneryLibrary/XPLibrarySystem.cpp
    Source/X-PlaneSceneryLibrary/XPObj.cpp
)

source_group("Header Files" FILES
    Source/X-PlaneSceneryLibrary/XPObj.h
    Source/X-PlaneSceneryLibrary/XPLibrarySystem.h
    Source/X-PlaneSceneryLibrary/XPLibraryPath.h
    Source/X-PlaneSceneryLibrary/XPLayerGroups.h
    Source/X-PlaneSceneryLibrary/X-PlaneSceneryLibraryUtils.h
    Source/X-PlaneSceneryLibrary/X-PlaneSceneryLibraryParsers.h
    Source/X-PlaneSceneryLibrary/XPAsset.h
    Source/X-PlaneSceneryLibrary/TextUtils.h
)

source_group("Source Files" FILES
    Source/X-PlaneSceneryLibrary/TextUtils.cpp
    Source/X-PlaneSceneryLibrary/XPLayerGroups.cpp
    Source/X-PlaneSceneryLibrary/XPLibrarySystem.cpp
    Source/X-PlaneSceneryLibrary/XPObj.cpp
)

source_group("Resource" FILES
    Source/X-PlaneSceneryLibrary/resource.h
    Source/X-PlaneSceneryLibrary/X-PlaneSceneryLibrary.rc
)

source_group("Tests" FILES
    Source/X-PlaneSceneryLibrary/DebugMain.cpp
)

add_library(X-PlaneSceneryLibrary
    ${XPLIB_HEADER_FILES}
    ${XPLIB_SOURCE_FILES}
    Source/X-PlaneSceneryLibrary/DebugMain.cpp
    Source/X-PlaneSceneryLibrary/TextUtils.cpp
    Source/X-PlaneSceneryLibrary/XPLayerGroups.cpp
    Source/X-PlaneSceneryLibrary/XPLibrarySystem.cpp
    Source/X-PlaneSceneryLibrary/XPObj.cpp
    Source/X-PlaneSceneryLibrary/X-PlaneSceneryLibrary.rc
)

# --------------------------------
# SceneryEditorX | Launcher
# --------------------------------

source_group("Resource" FILES
    ${ICON_FILES}
    ${REGISTRY_KEYS}
    Assets/splash_screen.bmp
    Source/Launcher/resource.h
    Source/Launcher/Launcher.rc
)

source_group("Core" FILES
    Source/Launcher/Main.cpp
    Source/SceneryEditorX/version.h
    Source/Launcher/Logging.cpp
    Source/Launcher/DirectoryManager.cpp
    Source/Launcher/SplashHandler.h
    Source/Launcher/Updater.h
    Source/Launcher/Updater.cpp
)

source_group("Registry" FILES
    Source/Launcher/RegInit.h
    Source/Launcher/RegCheck.h
    Source/Launcher/ElevatePerms.h
)

add_executable(Launcher
    ${ICON_FILES}
    ${REGISTRY_KEYS}
    Assets/splash_screen.bmp
    Source/Launcher/Main.cpp
    Source/Launcher/resource.h
    Source/Launcher/Launcher.rc
    Source/Launcher/Logging.cpp
    Source/Launcher/RegInit.h
    Source/Launcher/RegCheck.h
    Source/Launcher/ElevatePerms.h
    Source/Launcher/DirectoryManager.cpp
    Source/Launcher/SplashHandler.h
    Source/Launcher/Updater.h
    Source/Launcher/Updater.cpp
    Source/SceneryEditorX/version.h
)
target_link_libraries(Launcher PRIVATE Shell32.lib)
target_link_libraries(Launcher PRIVATE
    spdlog::spdlog_header_only
    dxgi
    d3d12
    CURL::libcurl
)

target_include_directories(Launcher PRIVATE
    Source/SceneryEditorX/Logging
    Source/SceneryEditorX/
    ${CURL_INCLUDE_DIRS}
)

target_link_directories(Launcher PRIVATE ${CURL_LIBRARY_DIRS})

target_link_options(Launcher PRIVATE /SUBSYSTEM:WINDOWS)

# --------------------------------
# VulkanRender
# --------------------------------

add_library(VulkanRenderer
    Source/VulkanRenderer/benchmark.hpp
    Source/VulkanRenderer/camera.hpp
    Source/VulkanRenderer/CommandLineParser.hpp
    Source/VulkanRenderer/Entrypoints.h
    Source/VulkanRenderer/frustum.hpp
    Source/VulkanRenderer/keycodes.hpp
    Source/VulkanRenderer/main.cpp
    Source/VulkanRenderer/threadpool.hpp
    Source/VulkanRenderer/VulkanAndroid.cpp
    Source/VulkanRenderer/VulkanAndroid.h
    Source/VulkanRenderer/VulkanBuffer.cpp
    Source/VulkanRenderer/VulkanBuffer.h
    Source/VulkanRenderer/VulkanDebug.cpp
    Source/VulkanRenderer/VulkanDebug.h
    Source/VulkanRenderer/VulkanDevice.cpp
    Source/VulkanRenderer/VulkanDevice.h
    Source/VulkanRenderer/vulkanexamplebase.cpp
    Source/VulkanRenderer/vulkanexamplebase.h
    Source/VulkanRenderer/VulkanFrameBuffer.hpp
    Source/VulkanRenderer/VulkanglTFModel.cpp
    Source/VulkanRenderer/VulkanglTFModel.h
    Source/VulkanRenderer/VulkanInitializers.hpp
    Source/VulkanRenderer/VulkanRaytracingSample.cpp
    Source/VulkanRenderer/VulkanRaytracingSample.h
    Source/VulkanRenderer/VulkanSwapChain.cpp
    Source/VulkanRenderer/VulkanSwapChain.h
    Source/VulkanRenderer/VulkanTexture.cpp
    Source/VulkanRenderer/VulkanTexture.h
    Source/VulkanRenderer/VulkanTools.cpp
    Source/VulkanRenderer/VulkanTools.h
    Source/VulkanRenderer/VulkanUIOverlay.cpp
    Source/VulkanRenderer/VulkanUIOverlay.h
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
    Source/VulkanRenderer/
    Dependency/optick/src/
    ${PORTABLE_FILE_DIALOGS_INCLUDE_DIRS}
    $ENV{VULKAN_SDK}/Include/
    ${Stb_INCLUDE_DIR}
)

target_link_options(VulkanRenderer PRIVATE /SUBSYSTEM:WINDOWS)

# --------------------------------
# SceneryEditorX
# --------------------------------

# Glob the optick sources and compile the executable with them directly.
file(GLOB OPTICK_SOURCES Dependency/optick/src/*.cpp)

# file(GLOB IMGUI_INCLUDES Dependency/optick/src/*.cpp)
add_executable(SceneryEditorX
    ${OPTICK_SOURCES}
    ${ICON_FILES}
    ${REGISTRY_KEYS}
    ${FONT_FILES}

    Source/SceneryEditorX/Main.cpp
    Source/SceneryEditorX/Core/Application.cpp
    Source/SceneryEditorX/Core/Profiler.h
    Source/SceneryEditorX/Core/VK_Window.cpp
    Source/SceneryEditorX/Platform/Windows/FileManager/FileDialog.cpp
    Source/SceneryEditorX/Platform/Windows/FileManager/FileDialog.h
    Source/SceneryEditorX/Platform/Windows/FileManager/LockFile.cpp
    Source/SceneryEditorX/Logging/Logging.cpp
    Source/SceneryEditorX/Renderer/Camera.cpp
    Source/SceneryEditorX/Renderer/VK_Pipeline.cpp
    Source/SceneryEditorX/UI/About.cpp
    Source/SceneryEditorX/UI/ContentBrowser.cpp
    Source/SceneryEditorX/UI/Fonts.h
    Source/SceneryEditorX/UI/LayerStack.cpp
    Source/SceneryEditorX/UI/MenuBar.cpp
    Source/SceneryEditorX/UI/Modals.cpp
    Source/SceneryEditorX/UI/SettingsPanel.cpp
    Source/SceneryEditorX/UI/Styles.cpp
    Source/SceneryEditorX/UI/UI.h
    Source/SceneryEditorX/Options/Settings.cpp
    Source/SceneryEditorX/SceneryEditorX.rc
    Source/SceneryEditorX/resource.h
    Source/SceneryEditorX/version.h
)

source_group("External/Optick" FILES ${OPTICK_SOURCES})

source_group("Core" FILES
    Source/SceneryEditorX/Main.cpp
    Source/SceneryEditorX/Core/Application.cpp
    Source/SceneryEditorX/Core/DirectoryManager.cpp
)

source_group("Core/Platform/Windows" FILES
    Source/SceneryEditorX/Platform/Windows/FileManager/FileDialog.cpp
    Source/SceneryEditorX/Platform/Windows/FileManager/FileDialog.h
    Source/SceneryEditorX/Platform/Windows/FileManager/LockFile.cpp
)

source_group("Core/Logger" FILES
    Source/SceneryEditorX/Logging/Logging.cpp
)

source_group("Core/Renderer" FILES
    Source/SceneryEditorX/Renderer/Camera.cpp
    Source/SceneryEditorX/Renderer/VK_Pipeline.cpp
    Source/SceneryEditorX/Core/VK_Window.cpp
)

source_group("Core/Profiler" FILES
    Source/SceneryEditorX/Core/Profiler.h
)

source_group("Core/Settings" FILES
    Source/SceneryEditorX/Options/Settings.cpp
)

source_group("Core/UI" FILES
    Source/SceneryEditorX/UI/About.cpp
    Source/SceneryEditorX/UI/ContentBrowser.cpp
    Source/SceneryEditorX/UI/Fonts.h
    Source/SceneryEditorX/UI/LayerStack.cpp
    Source/SceneryEditorX/UI/MenuBar.cpp
    Source/SceneryEditorX/UI/Modals.cpp
    Source/SceneryEditorX/UI/SettingsPanel.cpp
    Source/SceneryEditorX/UI/Styles.cpp
    Source/SceneryEditorX/UI/UI.h
)

source_group("Resource" FILES
    Source/SceneryEditorX/resource.h
    Source/SceneryEditorX/version.h
    Source/SceneryEditorX/SceneryEditorX.rc
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
    Source/SceneryEditorX/
    Dependency/optick/src/
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

target_precompile_headers(SceneryEditorX PRIVATE Source/SceneryEditorX/xpeditor_pch.h)

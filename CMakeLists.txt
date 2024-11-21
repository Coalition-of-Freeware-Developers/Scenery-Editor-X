cmake_minimum_required(VERSION 3.22..)


# Build System
# --------------------------------

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/Dependency/vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "")

# Boost Librarys
# --------------------------------
#set(Boost_USE_STATIC_LIBS ON)

#set(Boost_INCLUDE_DIR ${_VCPKG_INSTALLED_DIR}/x64-windows-static/include)

#find_package(boost  COMPONENTS array asio chrono filesystem geometry io iostream)

#define  set(Boost_INCLUDE_DIR "path")

project(SceneryEditorX VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Packages
# --------------------------------

find_package(glm      CONFIG REQUIRED)
find_package(glfw3    CONFIG REQUIRED)
find_package(spdlog   CONFIG REQUIRED)
find_package(imgui    CONFIG REQUIRED)
find_package(imguizmo CONFIG REQUIRED)
find_package(Stb             REQUIRED)

find_path(PORTABLE_FILE_DIALOGS_INCLUDE_DIRS "portable-file-dialogs.h")

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

add_library(X-PlaneSceneryLibrary ${XPLIB_HEADER_FILES} ${XPLIB_SOURCE_FILES}
    Source/X-PlaneSceneryLibrary/DebugMain.cpp
    Source/X-PlaneSceneryLibrary/TextUtils.cpp
    Source/X-PlaneSceneryLibrary/XPLayerGroups.cpp
    Source/X-PlaneSceneryLibrary/XPLibrarySystem.cpp
    Source/X-PlaneSceneryLibrary/XPObj.cpp
    Source/X-PlaneSceneryLibrary/X-PlaneSceneryLibrary.rc
)

# SceneryEditorX
# --------------------------------

set(ICON_FILES
    Assets/icon.ico
    Assets/icon.png
    Assets/splash_screen.png
)

set(FONT_FILES
    Assets/Fonts/industry/Industry-Bold.ttf
    Assets/Fonts/industry/Industry-Book.ttf
    Assets/Fonts/industry/Industry-Medium.ttf
    Assets/Fonts/industry/Industry-Thin.ttf
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

# Glob the optick sources and compile the executable with them directly.
file(GLOB OPTICK_SOURCES Dependency/optick/src/*.cpp)
# file(GLOB IMGUI_INCLUDES Dependency/optick/src/*.cpp)

add_executable(SceneryEditorX
    ${OPTICK_SOURCES}
    ${ICON_FILES}
    # ${FONT_FILES}
    Source/SceneryEditorX/Main.cpp
    Source/SceneryEditorX/core/Application.cpp
    Source/SceneryEditorX/core/DirectoryManager.cpp
    Source/SceneryEditorX/core/VK_Window.cpp
    Source/SceneryEditorX/io/FileDialog.cpp
    Source/SceneryEditorX/io/FileDialog.h
    Source/SceneryEditorX/io/LockFile.cpp
    Source/SceneryEditorX/logging/Logging.cpp
    Source/SceneryEditorX/renderer/Camera.cpp
    Source/SceneryEditorX/renderer/VK_Pipeline.cpp
    Source/SceneryEditorX/ui/About.cpp
    Source/SceneryEditorX/ui/ContentBrowser.cpp
    Source/SceneryEditorX/ui/Fonts.h
    Source/SceneryEditorX/ui/LayerStack.cpp
    Source/SceneryEditorX/ui/MenuBar.cpp
    Source/SceneryEditorX/ui/Modals.cpp
    Source/SceneryEditorX/ui/SettingsPanel.cpp
    Source/SceneryEditorX/ui/Styles.cpp
    Source/SceneryEditorX/ui/UI.h
    Source/SceneryEditorX/options/Settings.cpp
    Source/SceneryEditorX/SceneryEditorX.rc
    Source/SceneryEditorX/resource.h
)

source_group("External/Optick" FILES ${OPTICK_SOURCES})
# source_group("External/Icons" FILES ${ICON_FILES})
# source_group("External/Fonts" FILES ${FONT_FILES})


source_group("Core" FILES
    Source/SceneryEditorX/Main.cpp
    Source/SceneryEditorX/core/Application.cpp
    Source/SceneryEditorX/core/DirectoryManager.cpp
    Source/SceneryEditorX/core/VK_Window.cpp
)

source_group("IO" FILES
    Source/SceneryEditorX/io/FileDialog.cpp
    Source/SceneryEditorX/io/FileDialog.h
    Source/SceneryEditorX/io/LockFile.cpp
)

source_group("Logger" FILES
    Source/SceneryEditorX/logging/Logging.cpp
)

source_group("Renderer" FILES
    Source/SceneryEditorX/renderer/Camera.cpp
    Source/SceneryEditorX/renderer/VK_Pipeline.cpp
)

source_group("Settings" FILES
    Source/SceneryEditorX/options/Settings.cpp
)

source_group("UI" FILES
    Source/SceneryEditorX/ui/About.cpp
    Source/SceneryEditorX/ui/ContentBrowser.cpp
    Source/SceneryEditorX/ui/Fonts.h
    Source/SceneryEditorX/ui/LayerStack.cpp
    Source/SceneryEditorX/ui/MenuBar.cpp
    Source/SceneryEditorX/ui/Modals.cpp
    Source/SceneryEditorX/ui/SettingsPanel.cpp
    Source/SceneryEditorX/ui/Styles.cpp
    Source/SceneryEditorX/ui/UI.h
)

source_group("Resource" FILES
    ${ICON_FILES}
    Source/SceneryEditorX/resource.h
    Source/SceneryEditorX/SceneryEditorX.rc
)

# Include
target_include_directories(SceneryEditorX PRIVATE
    Source/SceneryEditorX/
    Dependency/optick/src/
    ${PORTABLE_FILE_DIALOGS_INCLUDE_DIRS}
    $ENV{VULKAN_SDK}/Include/
    ${Stb_INCLUDE_DIR}
)

# Link
target_link_libraries(SceneryEditorX PRIVATE
    #boost::array
    #boost::asio
    #boost::chrono
    #boost::filesystem
    #boost::geometry
    #boost::io
    #boost::iostream
    glm::glm-header-only
    spdlog::spdlog_header_only
    imgui::imgui
    imguizmo::imguizmo
    glfw
    dxgi
    d3d12
)

target_link_directories(SceneryEditorX PRIVATE
    $ENV{VULKAN_SDK}/Lib/
)

# Defines
target_compile_definitions(SceneryEditorX PRIVATE WIN32_LEAN_AND_MEAN)

# Precompiled Header
target_precompile_headers(SceneryEditorX PRIVATE Source/SceneryEditorX/core/xpeditor_pch.h)

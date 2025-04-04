# --------------------------------
# Scenery Editor X
# --------------------------------

MESSAGE(STATUS "=================================================")
MESSAGE(STATUS "Generating Scenery Editor X Project")

# --------------------------------

FILE(GLOB MAC_PLATFORM platform/mac/*.cpp platform/mac/*.h platform/mac/*.hpp)
FILE(GLOB LINUX_PLATFORM platform/linux/*.cpp platform/linux/*.h platform/linux/*.hpp)
FILE(GLOB WIN_PLATFORM platform/windows/*.cpp platform/windows/*.h platform/windows/*.hpp)
FILE(GLOB RENDERER_FILES renderer/*.cpp renderer/*.h renderer/*.hpp)
FILE(GLOB UI_FILES ui/*.cpp ui/*.h ui/*.hpp)

SET(LAUNCHER_DIR ${CMAKE_SOURCE_DIR}/source/Launcher)
SET (PROJECT_CONFIG_FILES
	${CMAKE_SOURCE_DIR}/.clang-format
	${CMAKE_SOURCE_DIR}/.editorconfig
	${CMAKE_SOURCE_DIR}/SceneryEditorX.licenseheader
)

ADD_EXECUTABLE(glslang::validator IMPORTED)
FIND_PROGRAM(GLSLANG_VALIDATOR "glslangValidator" HINTS $ENV{VULKAN_SDK}/bin REQUIRED)
SET_PROPERTY(TARGET glslang::validator PROPERTY IMPORTED_LOCATION "${GLSLANG_VALIDATOR}")

# --------------------------------
ADD_EXECUTABLE(SceneryEditorX
    ${ICON_FILES}
    ${FONT_FILES}
    ${RENDERER_FILES}
    ${SCENE_FILES}
	#${UI_FILES}
    ${MAC_PLATFORM}
    ${LINUX_PLATFORM}
    ${WIN_PLATFORM}
    ${REGISTRY_KEYS}
	${PROJECT_CONFIG_FILES}
    ${CMAKE_SOURCE_DIR}/assets/splash_screen.bmp
    # --------------------------------
	core/application.cpp
    core/application.h
    core/base.hpp
	core/cmd_line.cpp
	core/cmd_line.h
	core/editor.h
	core/editor.cpp
    core/entry_point.h
	core/layer.cpp
	core/layer.h
    core/transforms.cpp
	core/version.h
	core/window.cpp
    core/window.h
    # --------------------------------
    scene/asset_manager.cpp
    scene/asset_manager.h
    # --------------------------------
    platform/system_detection.h
	platform/editor_config.hpp
	platform/windows/file_manager.cpp
	platform/windows/file_manager.hpp
	platform/windows/platform_utils.h
	platform/windows/FileManager/CfgLoader.h
	platform/windows/FileManager/CfgLoader.cpp
	platform/windows/FileManager/FileDialog.h
	platform/windows/FileManager/FileDialog.cpp
	platform/windows/FileManager/LockFile.cpp
    # --------------------------------
	logging/profiler.hpp
    logging/logging.cpp
    logging/logging.hpp
	# --------------------------------
	ui/colors.h
	ui/fonts.h
	ui/gui_context.cpp
	ui/gui_context.h
	ui/styles.cpp
	ui/ui.cpp
	ui/ui.h
	ui/ui_manager.h
	ui/panels/about.cpp
	ui/panels/content_browser.cpp
	ui/panels/layer_stack.cpp
	ui/panels/loading_radial.cpp
	ui/panels/menu_bar.cpp
	ui/panels/settings_panel.cpp
	ui/panels/viewport.cpp
	ui/actions/modals.cpp
	ui/actions/viewports.cpp
    # --------------------------------
    SceneryEditorX.rc
    resource.h

    # --------------------------------
    # LAUNCHER FILES
    # --------------------------------

    #${LAUNCHER_DIR}/core/updater.h
    #${LAUNCHER_DIR}/core/updater.cpp
    #${LAUNCHER_DIR}/core/launcher_main.cpp
    #${LAUNCHER_DIR}/core/launcher_main.h
    #${LAUNCHER_DIR}/core/elevate_perms.h
    #${LAUNCHER_DIR}/core/directory_manager.cpp
    #${LAUNCHER_DIR}/core/directory_manager.cpp
    #${LAUNCHER_DIR}/core/splash_handler.h
    #${LAUNCHER_DIR}/core/updater.h
    #${LAUNCHER_DIR}/core/updater.cpp

    # --------------------------------

    #${LAUNCHER_DIR}/registry/reg_init.h
    #${LAUNCHER_DIR}/registry/reg_check.h

    # --------------------------------
)

# --------------------------------
SOURCE_GROUP("Core" FILES
	core/application.h
	core/application.cpp
	core/cmd_line.cpp
	core/cmd_line.h
	core/base.hpp
	core/editor.h
	core/editor.cpp
	core/entry_point.h
	core/version.h
	xpeditor_pch.h
)
SOURCE_GROUP("Editor/Core" FILES
    core/window.cpp
    core/window.h
)
SOURCE_GROUP("Core/Platform" FILES
    platform/system_detection.h
)
SOURCE_GROUP("Core/Platform/Windows" FILES
    ${WIN_PLATFORM}
)
SOURCE_GROUP("Core/Platform/Linux" FILES
    ${LINUX_PLATFORM}
)
SOURCE_GROUP("Core/Platform/Mac" FILES
    ${MAC_PLATFORM}
)
SOURCE_GROUP("Core/Coordinates" FILES
	core/transforms.cpp
)
SOURCE_GROUP("Core/Logger" FILES
    logging/logging.cpp
    logging/logging.hpp
)
SOURCE_GROUP("Editor/Renderer" FILES
    ${RENDERER_FILES}
)
SOURCE_GROUP("Editor/Scene" FILES
    scene/asset_manager.cpp
    scene/asset_manager.h
)
SOURCE_GROUP("Editor/UI" FILES
	ui/colors.h
	ui/fonts.h
	ui/gui_context.cpp
	ui/gui_context.h
	ui/styles.cpp
	ui/ui.cpp
	ui/ui.h
	ui/ui_manager.h
)
SOURCE_GROUP("Editor/UI/Panels" FILES
	ui/panels/about.cpp
	ui/panels/content_browser.cpp
	ui/panels/layer_stack.cpp
	ui/panels/loading_radial.cpp
	ui/panels/menu_bar.cpp
	ui/panels/settings_panel.cpp
	ui/panels/viewport.cpp
)
SOURCE_GROUP("Editor/UI/Actions" FILES
	ui/actions/modals.cpp
	ui/actions/viewports.cpp
)
#SOURCE_GROUP("Editor/Core/Renderer/Scene" FILES
#    ${SCENE_FILES}
#)

#SOURCE_GROUP("Core/Settings" FILES
#    options/Settings.cpp
#)
SOURCE_GROUP("Resource" FILES
    resource.h
    SceneryEditorX.rc
)
SOURCE_GROUP("Resource/Icons" FILES
    ${ICON_FILES}
)
SOURCE_GROUP("Resource/Images" FILES
    ${CMAKE_SOURCE_DIR}/assets/splash_screen.bmp
)
SOURCE_GROUP("Resource/Fonts" FILES
    ${FONT_FILES}
)
SOURCE_GROUP("Resource/Registry" FILES
    ${REGISTRY_KEYS}
)
SOURCE_GROUP("Launcher/Core" FILES
    ${LAUNCHER_DIR}/core/launcher_main.cpp
    ${LAUNCHER_DIR}/core/launcher_main.h
    ${LAUNCHER_DIR}/core/directory_manager.cpp
    ${LAUNCHER_DIR}/core/directory_manager.hpp
    ${LAUNCHER_DIR}/core/splash_handler.h
    ${LAUNCHER_DIR}/core/updater.h
    ${LAUNCHER_DIR}/core/updater.cpp
)
SOURCE_GROUP("Launcher/Registry" FILES
    ${LAUNCHER_DIR}/registry/reg_init.h
    ${LAUNCHER_DIR}/registry/reg_check.h
    ${LAUNCHER_DIR}/core/elevate_perms.h
)
SOURCE_GROUP("Launcher/Registry/Keys" FILES
    ${LAUNCHER_DIR}/registry/edXLibReg.rgs
    ${LAUNCHER_DIR}/registry/edXReg.rgs
    ${LAUNCHER_DIR}/registry/ProgramReg.rgs
)

# --------------------------------
# SHADERS
# --------------------------------

if(WIN32)
    cmake_path(SET GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin/glslangValidator.exe")
else()
    cmake_path(SET GLSL_VALIDATOR "$ENV{VULKAN_SDK}/bin/glslangValidator")
endif()
message("GLSL VALIDATOR ${GLSL_VALIDATOR}")
add_compile_definitions(GLSL_VALIDATOR="${GLSL_VALIDATOR}")

# --------------------------------

TARGET_INCLUDE_DIRECTORIES(SceneryEditorX
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Source
		${CMAKE_SOURCE_DIR}/dependency
        ${CMAKE_SOURCE_DIR}/dependency/imgui
        ${CMAKE_SOURCE_DIR}/dependency/imgui/backends
        ${CMAKE_SOURCE_DIR}/dependency/imgui/misc
        ${CMAKE_SOURCE_DIR}/dependency/ImGuizmo
        ${REGISTRY_KEYS}
        ${PORTABLE_FILE_DIALOGS_INCLUDE_DIRS}
        $ENV{VULKAN_SDK}/Include/
        ${Stb_INCLUDE_DIR}
        ${CURL_INCLUDE_DIRS}
)

# --------------------------------

TARGET_LINK_LIBRARIES(SceneryEditorX
    PRIVATE
        glm::glm-header-only
        spdlog::spdlog_header_only
        GPUOpen::VulkanMemoryAllocator
        CURL::libcurl
        Vulkan::Vulkan
		imgui
		X-PlaneSceneryLibrary
        glfw
        dxgi
        d3d12
        Shell32.lib
)

#TARGET_LINK_OPTIONS(SceneryEditorX
#    PRIVATE
#        /SUBSYSTEM:WINDOWS
#)

# --------------------------------

TARGET_COMPILE_DEFINITIONS(SceneryEditorX
    PUBLIC
        EDITOR
        LAUNCHER
        $<$<CONFIG:Debug>:SEDX_DEBUG>
        $<$<CONFIG:Release>:SEDX_RELEASE>
        VK_USE_PLATFORM_WIN32_KHR
        GLFW_INCLUDE_VULKAN
        GLFW_EXPOSE_NATIVE_WIN32
        CURL_STATICLIB
)

# --------------------------------

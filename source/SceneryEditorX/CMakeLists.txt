# --------------------------------
# Scenery Editor X
# --------------------------------

MESSAGE(STATUS "=================================================")
MESSAGE(STATUS "Generating Scenery Editor X Project")

# --------------------------------

FILE(GLOB LOGGER logging/*.cpp logging//*.h logging//*.hpp)
FILE(GLOB PLATFORM platform/*.cpp platform/*.h platform/*.hpp)
FILE(GLOB RENDERER_FILES renderer/*.cpp renderer/*.h renderer/*.hpp)
FILE(GLOB SCENE_FILES scene/*.cpp scene/*.h scene/*.hpp)
FILE(GLOB VULKAN_FILES vulkan/*.cpp vulkan/*.h vulkan/*.hpp)
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
	${VULKAN_FILES}
    ${SCENE_FILES}
	${LOGGER}
    ${PLATFORM}
    ${REGISTRY_KEYS}
	${PROJECT_CONFIG_FILES}
    ${CMAKE_SOURCE_DIR}/assets/splash_screen.bmp
    # --------------------------------
	core/application_data.h
    core/base.hpp
	core/encoding.cpp
    core/encoding.hpp
	core/layer.cpp
	core/layer.h
    core/main.cpp
	core/memory.cpp
	core/memory.h
	core/serializer.hpp
    core/transforms.cpp
	core/version.h
    core/window.cpp
    core/window.h
	# --------------------------------
	core/editor/editor.cpp
	core/editor/editor.h
	core/editor/scenery_gateway.cpp
	core/editor/scenery_gateway.h
    # --------------------------------
	renderer/shaders/shader.cpp
	renderer/shaders/shader.h
	renderer/shaders/shader_compiler.cpp
	renderer/shaders/shader_compiler.h
	# --------------------------------
	ui/colors.h
	ui/fonts.h
	ui/ui_context.cpp
	ui/ui_context.h
	ui/styles.cpp
	ui/ui.cpp
	ui/ui.h
	ui/ui_manager.h
	ui/ui_globals.cpp
	ui/panels/content_browser.cpp
	ui/panels/layer_stack.cpp
	ui/panels/loading_radial.cpp
	ui/panels/menu_bar.cpp
	ui/panels/settings_panel.cpp
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
	core/application_data.h
	core/base.hpp
	core/encoding.cpp
	core/encoding.hpp
	core/layer.cpp
	core/layer.h
	core/main.cpp
	core/memory.cpp
	core/memory.h
	core/serializer.hpp
	core/version.h
    core/window.cpp
    core/window.h
)
SOURCE_GROUP("Editor/Core" FILES
	core/editor/editor.cpp
	core/editor/editor.h
	core/editor/scenery_gateway.cpp
	core/editor/scenery_gateway.h
)
SOURCE_GROUP("Core/Platform" FILES
	${PLATFORM}
)
SOURCE_GROUP("Core/Coordinates" FILES
	core/transforms.cpp
)
SOURCE_GROUP("Core/Logger" FILES
	${LOGGER}
)
SOURCE_GROUP("Core/Vulkan" FILES
	${VULKAN_FILES}
)
SOURCE_GROUP("Core/Vulkan/Globals" FILES
	vulkan/buffer_data.h
	vulkan/image_data.cpp
	vulkan/image_data.h
	vulkan/render_data.h
)
SOURCE_GROUP("Editor/Renderer" FILES
    ${RENDERER_FILES}
)
SOURCE_GROUP("Editor/Renderer/Shaders" FILES
	renderer/shaders/shader.cpp
	renderer/shaders/shader.h
	renderer/shaders/shader_compiler.cpp
	renderer/shaders/shader_compiler.h
)
SOURCE_GROUP("Editor/Scene" FILES
	${SCENE_FILES}
)
SOURCE_GROUP("Editor/UI" FILES
	ui/colors.h
	ui/fonts.h
	ui/ui_context.cpp
	ui/ui_context.h
	ui/styles.cpp
	ui/ui.cpp
	ui/ui.h
	ui/ui_manager.h
	ui/ui_globals.cpp
)
SOURCE_GROUP("Editor/UI/Panels" FILES
	ui/panels/about.cpp
	ui/panels/content_browser.cpp
	ui/panels/layer_stack.cpp
	ui/panels/loading_radial.cpp
	ui/panels/menu_bar.cpp
	ui/panels/settings_panel.cpp
)
SOURCE_GROUP("Editor/UI/Actions" FILES
	ui/actions/modals.cpp
	ui/actions/viewports.cpp
)
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
        ${CMAKE_SOURCE_DIR}/source
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
		libconfig
		libconfig++
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
		SPDLOG_USE_STD_FORMAT
        VK_USE_PLATFORM_WIN32_KHR
        GLFW_INCLUDE_VULKAN
        GLFW_EXPOSE_NATIVE_WIN32
        CURL_STATICLIB
)

# --------------------------------

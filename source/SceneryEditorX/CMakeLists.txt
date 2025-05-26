# --------------------------------
# Scenery Editor X | Application Core
# --------------------------------

MESSAGE(STATUS "=================================================")
MESSAGE(STATUS "Generating Application Core")

# --------------------------------

FILE(GLOB CORE core/*.cpp core/*.h core/*.hpp)
FILE(GLOB LOGGER logging/*.cpp logging//*.h logging//*.hpp)
FILE(GLOB PLATFORM platform/*.cpp platform/*.h platform/*.hpp)
FILE(GLOB RENDERER renderer/*.cpp renderer/*.h renderer/*.hpp)
FILE(GLOB UI_FILES ui/*.cpp ui/*.h ui/*.hpp)
FILE(GLOB UTILITIES_FILES utils/*.cpp utils/*.h utils/*.hpp)

SET (PROJECT_CONFIG_FILES
	${CMAKE_SOURCE_DIR}/.clang-format
	${CMAKE_SOURCE_DIR}/.editorconfig
	${CMAKE_SOURCE_DIR}/SceneryEditorX.licenseheader
)

ADD_EXECUTABLE(glslang::validator IMPORTED)
FIND_PROGRAM(GLSLANG_VALIDATOR "glslangValidator" HINTS $ENV{VULKAN_SDK}/bin REQUIRED)
SET_PROPERTY(TARGET glslang::validator PROPERTY IMPORTED_LOCATION "${GLSLANG_VALIDATOR}")

# --------------------------------

ADD_LIBRARY(AppCore
	${CORE}
	${LOGGER}
    ${PLATFORM}
	${RENDERER}
	${PROJECT_CONFIG_FILES}
    ${REGISTRY_KEYS}
	${UTILITIES_FILES}
    ${CMAKE_SOURCE_DIR}/assets/splash_screen.bmp
    # --------------------------------
	${CMAKE_SOURCE_DIR}/source/SceneryEditorX/EntryPoint.h
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
    resource.h
)

# --------------------------------

SOURCE_GROUP("Core" FILES
	${CORE}
	${CMAKE_SOURCE_DIR}/source/SceneryEditorX/EntryPoint.h
)
SOURCE_GROUP("Platform" FILES
	${PLATFORM}
)
SOURCE_GROUP("Logger" FILES
	${LOGGER}
)
SOURCE_GROUP("Renderer" FILES
	${RENDERER}
)
SOURCE_GROUP("Utilities" FILES
	${UTILITIES_FILES}
)
SOURCE_GROUP("UI" FILES
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
SOURCE_GROUP("UI/Panels" FILES
	ui/panels/about.cpp
	ui/panels/content_browser.cpp
	ui/panels/layer_stack.cpp
	ui/panels/loading_radial.cpp
	ui/panels/menu_bar.cpp
	ui/panels/settings_panel.cpp
)
SOURCE_GROUP("UI/Actions" FILES
	ui/actions/modals.cpp
	ui/actions/viewports.cpp
)
SOURCE_GROUP("Resource" FILES
    resource.h
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

IF(WIN32)
    cmake_path(SET GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin/glslangValidator.exe")
ELSE()
    cmake_path(SET GLSL_VALIDATOR "$ENV{VULKAN_SDK}/bin/glslangValidator")
ENDIF()
MESSAGE("GLSL VALIDATOR ${GLSL_VALIDATOR}")
ADD_COMPILE_DEFINITIONS(GLSL_VALIDATOR="${GLSL_VALIDATOR}")

# --------------------------------

TARGET_INCLUDE_DIRECTORIES(AppCore
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

TARGET_LINK_LIBRARIES(AppCore
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

# --------------------------------

TARGET_COMPILE_DEFINITIONS(AppCore
    PUBLIC
        APPCORE
        $<$<CONFIG:Debug>:SEDX_DEBUG>
        $<$<CONFIG:Release>:SEDX_RELEASE>
		SPDLOG_USE_STD_FORMAT
        VK_USE_PLATFORM_WIN32_KHR
        GLFW_INCLUDE_VULKAN
        GLFW_EXPOSE_NATIVE_WIN32
        CURL_STATICLIB
)

# --------------------------------

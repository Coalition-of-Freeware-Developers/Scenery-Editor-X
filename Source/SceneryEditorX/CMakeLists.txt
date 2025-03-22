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
    ${MAC_PLATFORM}
    ${LINUX_PLATFORM}
    ${WIN_PLATFORM}
    ${REGISTRY_KEYS}
	${PROJECT_CONFIG_FILES}
    ${CMAKE_SOURCE_DIR}/assets/splash_screen.bmp
    # --------------------------------
    core/base.hpp
    core/main.cpp
    core/version.h
    core/window.cpp
    core/window.h
    # --------------------------------
    scene/asset_manager.cpp
    scene/asset_manager.h
    # --------------------------------
    platform/system_detection.h
    platform/platform.h
    # --------------------------------
    logging/logging.cpp
    logging/logging.hpp
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

SOURCE_GROUP("Editor/Core" FILES
    core/base.hpp
    core/main.cpp
    core/version.h
    core/window.cpp
    core/window.h
)

SOURCE_GROUP("Core/Platform" FILES
    platform/system_detection.h
    platform/platform.h
    core/version.h
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
#SOURCE_GROUP("Editor/Core/Renderer/Scene" FILES
#    ${SCENE_FILES}
#)

#SOURCE_GROUP("Core/Settings" FILES
#    options/Settings.cpp
#)

#SOURCE_GROUP("Core/UI" FILES
#    UI/About.cpp
#    UI/ContentBrowser.cpp
#    UI/Fonts.h
#    UI/LayerStack.cpp
#    UI/MenuBar.cpp
#    UI/Modals.cpp
#    UI/SettingsPanel.cpp
#    UI/Styles.cpp
#    UI/UI.h
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
        volk
        Vulkan::Vulkan
        #vulkan-1.lib
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

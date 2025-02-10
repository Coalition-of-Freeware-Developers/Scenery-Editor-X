# --------------------------------
# Scenery Editor X
# --------------------------------

MESSAGE(STATUS "=================================================")
MESSAGE(STATUS "Generating Scenery Editor X Project")

FILE(GLOB RENDERER_FILES
    renderer/*.cpp
    renderer/*.h
    renderer/*.hpp
)
FILE(GLOB SCENE_FILES
    scene/*
)

ADD_EXECUTABLE(SceneryEditorX
    ${ICON_FILES}
    ${FONT_FILES}
    ${RENDERER_FILES}
    ${SCENE_FILES}

    core/Main.cpp
    core/Application.cpp
    core/Profiler.h
    core/VK_Window.cpp
    core/Window.hpp
    core/version.h
    platform/Windows/FileManager/FileDialog.cpp
    platform/Windows/FileManager/FileDialog.h
    platform/Windows/FileManager/LockFile.cpp
    platform/Windows/EditorConfig.hpp
    logging/Logging.cpp
    logging/Logging.hpp
    UI/About.cpp
    UI/ContentBrowser.cpp
    UI/Fonts.h
    UI/LayerStack.cpp
    UI/MenuBar.cpp
    UI/Modals.cpp
    UI/SettingsPanel.cpp
    UI/Styles.cpp
    UI/UI.h
    options/Settings.cpp

    SceneryEditorX.rc
    resource.h
)


SOURCE_GROUP("Core" FILES
    core/Main.cpp
    core/Application.cpp
    core/DirectoryManager.cpp
    core/Window.hpp
)

SOURCE_GROUP("Core/Platform/Windows/File Manager" FILES
    platform/windows/FileManager/FileDialog.cpp
    platform/windows/FileManager/FileDialog.h
    platform/windows/FileManager/LockFile.cpp
    platform/windows/EditorConfig.hpp
    platform/windows/FileManager.cpp
    platform/windows/FileManager.hpp
    platform/windows/PlatformUtils.h
    platform/windows/Window.cpp
    platform/windows/WindowProperties.cpp
)

SOURCE_GROUP("Core/Logger" FILES
    logging/Logging.cpp
    logging/Logging.hpp
)

SOURCE_GROUP("Core/Renderer" FILES
    ${RENDERER_FILES}
    core/VK_Window.cpp
)

SOURCE_GROUP("Core/Renderer/Scene" FILES
    ${SCENE_FILES}
)

SOURCE_GROUP("Core/Profiler" FILES
    core/Profiler.h
)

SOURCE_GROUP("Core/Settings" FILES
    options/Settings.cpp
)

SOURCE_GROUP("Core/UI" FILES
    UI/About.cpp
    UI/ContentBrowser.cpp
    UI/Fonts.h
    UI/LayerStack.cpp
    UI/MenuBar.cpp
    UI/Modals.cpp
    UI/SettingsPanel.cpp
    UI/Styles.cpp
    UI/UI.h
)

SOURCE_GROUP("Resource" FILES
    resource.h
    core/version.h
    SceneryEditorX.rc
)

SOURCE_GROUP("Resource/Icons" FILES
    ${ICON_FILES}
)

SOURCE_GROUP("Resource/Fonts" FILES
    ${FONT_FILES}
)

# Include
TARGET_INCLUDE_DIRECTORIES(SceneryEditorX
    PRIVATE
        ${CMAKE_SOURCE_DIR}/dependency/imgui
        ${CMAKE_SOURCE_DIR}/dependency/imgui/backends
        ${CMAKE_SOURCE_DIR}/dependency/imgui/misc
        ${CMAKE_SOURCE_DIR}/dependency/ImGuizmo
        ${CMAKE_SOURCE_DIR}/Source/SceneryEditorX/
        ${REGISTRY_KEYS}
        ${PORTABLE_FILE_DIALOGS_INCLUDE_DIRS}
        $ENV{VULKAN_SDK}/Include/
        ${Stb_INCLUDE_DIR}
        ${CURL_INCLUDE_DIRS}
)

# Link
TARGET_LINK_LIBRARIES(SceneryEditorX
    PRIVATE
        glm::glm-header-only
        spdlog::spdlog_header_only
        CURL::libcurl
        Vulkan
        glfw
        dxgi
        d3d12
        Shell32.lib
)

TARGET_LINK_DIRECTORIES(SceneryEditorX
    PRIVATE
        $ENV{VULKAN_SDK}/Lib/
        ${CURL_INCLUDE_DIRS}
)

target_compile_definitions(SceneryEditorX
    PRIVATE
        $<$<CONFIG:Debug>:SEDX_DEBUG>
        EDITOR
        VK_USE_PLATFORM_WIN32_KHR
        GLFW_INCLUDE_VULKAN
        GLFW_EXPOSE_NATIVE_WIN32
        CURL_STATICLIB
)


# --------------------------------
# Scenery Editor X
# --------------------------------

MESSAGE(STATUS "=================================================")
MESSAGE(STATUS "Generating Scenery Editor X Project")

ADD_EXECUTABLE(SceneryEditorX
    ${ICON_FILES}
    ${FONT_FILES}

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
    renderer/Camera.cpp
    renderer/VK_Pipeline.cpp
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

SOURCE_GROUP("Core/Platform/Windows" FILES
    platform/Windows/FileManager/FileDialog.cpp
    platform/Windows/FileManager/FileDialog.h
    platform/Windows/FileManager/LockFile.cpp
    platform/Windows/EditorConfig.hpp
)

SOURCE_GROUP("Core/Logger" FILES
    logging/Logging.cpp
)

SOURCE_GROUP("Core/Renderer" FILES
    renderer/Camera.cpp
    renderer/VK_Pipeline.cpp
    core/VK_Window.cpp
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
        ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/
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


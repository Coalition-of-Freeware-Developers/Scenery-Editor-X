# --------------------------------
# Scenery Editor X
# --------------------------------

message(STATUS "=================================================")
message(STATUS "Generating Scenery Editor X Project")

add_executable(SceneryEditorX
    ${OPTICK_SOURCES}
    ${ICON_FILES}
    ${REGISTRY_KEYS}
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

source_group("External/Optick" FILES ${OPTICK_SOURCES})

source_group("Core" FILES
    core/Main.cpp
    core/Application.cpp
    core/DirectoryManager.cpp
    core/Window.hpp
)

source_group("Core/Platform/Windows" FILES
    platform/Windows/FileManager/FileDialog.cpp
    platform/Windows/FileManager/FileDialog.h
    platform/Windows/FileManager/LockFile.cpp
    platform/Windows/EditorConfig.hpp
)

source_group("Core/Logger" FILES
    logging/Logging.cpp
)

source_group("Core/Renderer" FILES
    renderer/Camera.cpp
    renderer/VK_Pipeline.cpp
    core/VK_Window.cpp
)

source_group("Core/Profiler" FILES
    core/Profiler.h
)

source_group("Core/Settings" FILES
    options/Settings.cpp
)

source_group("Core/UI" FILES
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

source_group("Resource" FILES
    resource.h
    core/version.h
    SceneryEditorX.rc
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
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/
    ${CMAKE_SOURCE_DIR}/dependency/optick/src/
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

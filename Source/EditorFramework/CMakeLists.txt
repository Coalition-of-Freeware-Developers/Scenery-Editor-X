# --------------------------------
# Application Editor Framework
# --------------------------------

message(STATUS "=================================================")
message(STATUS "Generating Application Framework")


add_library(EditorFramework
    Main.cpp
    Logging.cpp

    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/UI/About.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/UI/ContentBrowser.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/UI/Fonts.h
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/UI/LayerStack.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/UI/MenuBar.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/UI/Modals.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/UI/SettingsPanel.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/UI/Styles.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/UI/UI.h

    ${CMAKE_SOURCE_DIR}/source/edX/edXProjectFile.h
)

source_group("Core" FILES
    main.cpp
    ${CMAKE_SOURCE_DIR}/source/edX/edXProjectFile.h
)

source_group("Core/Logger" FILES
    Logging.cpp
)

source_group("Core/Settings" FILES
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/Options/Settings.cpp
)

source_group("Core/UI" FILES
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/UI/About.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/UI/ContentBrowser.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/UI/Fonts.h
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/UI/LayerStack.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/UI/MenuBar.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/UI/Modals.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/UI/SettingsPanel.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/UI/Styles.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/UI/UI.h
)

target_link_libraries(EditorFramework PRIVATE
    glm::glm-header-only
    spdlog::spdlog_header_only
    CURL::libcurl
    glfw
    dxgi
    d3d12
    Shell32.lib
)

target_include_directories(EditorFramework PRIVATE
    ${CMAKE_SOURCE_DIR}/Source/SceneryEditorX/Logging
    ${CMAKE_SOURCE_DIR}/Source/SceneryEditorX
    ${CURL_INCLUDE_DIRS}
    ${PORTABLE_FILE_DIALOGS_INCLUDE_DIRS}
    $ENV{VULKAN_SDK}/Include/
)

target_link_directories(EditorFramework PRIVATE $ENV{VULKAN_SDK}/Lib/)
target_link_directories(EditorFramework PRIVATE ${CURL_INCLUDE_DIRS})

# --------------------------------
# Defines
# --------------------------------

target_compile_definitions(EditorFramework PRIVATE $<$<CONFIG:Debug>:SEDX_DEBUG> $<$<CONFIG:Release>:SEDX_RELEASE>)
target_compile_definitions(EditorFramework PRIVATE WIN32_LEAN_AND_MEAN)


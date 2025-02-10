# --------------------------------
# Application Editor Framework
# --------------------------------

MESSAGE(STATUS "=================================================")
MESSAGE(STATUS "Generating Application Framework")


ADD_LIBRARY(EditorFramework
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

SOURCE_GROUP("Core" FILES
    main.cpp
    ${CMAKE_SOURCE_DIR}/source/edX/edXProjectFile.h
)

SOURCE_GROUP("Core/Logger" FILES
    Logging.cpp
)

SOURCE_GROUP("Core/Settings" FILES
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/Options/Settings.cpp
)

SOURCE_GROUP("Core/UI" FILES
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

TARGET_LINK_LIBRARIES(EditorFramework PRIVATE
    glm::glm-header-only
    spdlog::spdlog_header_only
    CURL::libcurl
    glfw
    dxgi
    d3d12
    Shell32.lib
)

TARGET_INCLUDE_DIRECTORIES(EditorFramework PRIVATE
    ${CMAKE_SOURCE_DIR}/Source/SceneryEditorX/Logging
    ${CMAKE_SOURCE_DIR}/Source/SceneryEditorX
    ${CURL_INCLUDE_DIRS}
    ${PORTABLE_FILE_DIALOGS_INCLUDE_DIRS}
    $ENV{VULKAN_SDK}/Include/
)

TARGET_LINK_DIRECTORIES(EditorFramework PRIVATE $ENV{VULKAN_SDK}/Lib/)
TARGET_LINK_DIRECTORIES(EditorFramework PRIVATE ${CURL_INCLUDE_DIRS})

# --------------------------------
# Defines
# --------------------------------

TARGET_COMPILE_DEFINITIONS(EditorFramework PRIVATE $<$<CONFIG:Debug>:SEDX_DEBUG> $<$<CONFIG:Release>:SEDX_RELEASE>)
TARGET_COMPILE_DEFINITIONS(EditorFramework PRIVATE WIN32_LEAN_AND_MEAN)


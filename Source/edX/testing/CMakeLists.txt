# --------------------------------
# File Format Tester
# --------------------------------

MESSAGE(STATUS "=================================================")
MESSAGE(STATUS "Generating edX Format Tester")


ADD_EXECUTABLE(FormatTester
    main.cpp
    FileBrowser.cpp
    FileBrowser.hpp
    FormatTester.rc
    resource.h
    ${CMAKE_SOURCE_DIR}/source/edX/edXProjectFile.h
)

TARGET_LINK_LIBRARIES(FormatTester PRIVATE
    glm::glm-header-only
    spdlog::spdlog_header_only
    glfw
    dxgi
    d3d12
    Shell32.lib
)

TARGET_INCLUDE_DIRECTORIES(FormatTester PRIVATE
    ${CMAKE_SOURCE_DIR}/dependency/imgui
    ${CMAKE_SOURCE_DIR}/dependency/imgui/backends
    ${CMAKE_SOURCE_DIR}/dependency/imgui/misc
    ${CMAKE_SOURCE_DIR}/dependency/ImGuizmo
    ${CMAKE_SOURCE_DIR}/Source/SceneryEditorX/
    ${PORTABLE_FILE_DIALOGS_INCLUDE_DIRS}
    $ENV{VULKAN_SDK}/Include/
    ${Stb_INCLUDE_DIR}
)

# --------------------------------
# Defines
# --------------------------------

TARGET_COMPILE_DEFINITIONS(FormatTester PRIVATE WIN32_LEAN_AND_MEAN)

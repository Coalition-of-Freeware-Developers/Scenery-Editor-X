# --------------------------------
# File Format Tester
# --------------------------------

message(STATUS "Beginning project generation for Scenery Editor X | edX Format Tester")

add_executable(FormatTester
    main.cpp
    FileBrowser.cpp
    FileBrowser.hpp
    FormatTester.rc
    resource.h
    ${CMAKE_SOURCE_DIR}/Source/edX/edXProjectFile.h
)

target_link_libraries(FormatTester PRIVATE
    glm::glm-header-only
    spdlog::spdlog_header_only
    imgui::imgui
    glfw
    dxgi
    d3d12
)

target_include_directories(FormatTester PRIVATE
    ${CMAKE_SOURCE_DIR}/Source/SceneryEditorX/
    ${PORTABLE_FILE_DIALOGS_INCLUDE_DIRS}
    $ENV{VULKAN_SDK}/Include/
)

# --------------------------------
# Defines
# --------------------------------

target_compile_definitions(FormatTester PRIVATE WIN32_LEAN_AND_MEAN)

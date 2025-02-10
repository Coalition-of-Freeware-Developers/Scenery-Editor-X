# --------------------------------
# VulkanRender
# --------------------------------

MESSAGE(STATUS "=================================================")
MESSAGE(STATUS "Generating Vulkan Renderer")

FILE(GLOB VULKANRENDERER_SOURCES
    ${CMAKE_SOURCE_DIR}/Source/VulkanRenderer/Common/*.cpp
    ${CMAKE_SOURCE_DIR}/Source/VulkanRenderer/Common/*.h

    ${CMAKE_SOURCE_DIR}/Source/VulkanRenderer/Sources/*.cpp
    ${CMAKE_SOURCE_DIR}/Source/VulkanRenderer/Sources/*.h
)
FILE(GLOB VULKANSHADER_SOURCES
    ${CMAKE_SOURCE_DIR}/Source/VulkanRenderer/Shaders/*.vert
    ${CMAKE_SOURCE_DIR}/Source/VulkanRenderer/Shaders/*.frag
)

SOURCE_GROUP("Shaders" FILES ${VULKANSHADER_SOURCES})

ADD_LIBRARY(VulkanRenderer
    ${VULKANRENDERER_SOURCES}
    ${VULKANSHADER_SOURCES}
)

TARGET_LINK_LIBRARIES(VulkanRenderer PRIVATE
    glm::glm-header-only
    spdlog::spdlog_header_only
    glfw
    dxgi
    d3d12
    Shell32.lib
)

TARGET_INCLUDE_DIRECTORIES(VulkanRenderer PRIVATE
    ${CMAKE_SOURCE_DIR}/dependency/imgui
    ${CMAKE_SOURCE_DIR}/dependency/imgui/backends
    ${CMAKE_SOURCE_DIR}/dependency/imgui/misc
    ${CMAKE_SOURCE_DIR}/dependency/ImGuizmo
    ${CMAKE_SOURCE_DIR}/Source/VulkanRenderer
    ${PORTABLE_FILE_DIALOGS_INCLUDE_DIRS}
    $ENV{VULKAN_SDK}/Include/
    ${Stb_INCLUDE_DIR}
)

TARGET_LINK_OPTIONS(VulkanRenderer PRIVATE /SUBSYSTEM:WINDOWS)

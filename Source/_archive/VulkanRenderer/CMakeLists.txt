# --------------------------------
# VulkanRender
# --------------------------------

message(STATUS "=================================================")
message(STATUS "Generating Vulkan Renderer")


add_library(VulkanRenderer
    benchmark.hpp
    camera.hpp
    CommandLineParser.hpp
    Entrypoints.h
    frustum.hpp
    keycodes.hpp
    main.cpp
    threadpool.hpp
    VulkanAndroid.cpp
    VulkanAndroid.h
    VulkanBuffer.cpp
    VulkanBuffer.h
    VulkanDebug.cpp
    VulkanDebug.h
    VulkanDevice.cpp
    VulkanDevice.h
    vulkanexamplebase.cpp
    vulkanexamplebase.h
    VulkanFrameBuffer.hpp
    VulkanglTFModel.cpp
    VulkanglTFModel.h
    VulkanInitializers.hpp
    VulkanRaytracingSample.cpp
    VulkanRaytracingSample.h
    VulkanSwapChain.cpp
    VulkanSwapChain.h
    VulkanTexture.cpp
    VulkanTexture.h
    VulkanTools.cpp
    VulkanTools.h
    VulkanUIOverlay.cpp
    VulkanUIOverlay.h
)

target_link_libraries(VulkanRenderer PRIVATE
    glm::glm-header-only
    spdlog::spdlog_header_only
    glfw
    dxgi
    d3d12
    Shell32.lib
)

target_include_directories(VulkanRenderer PRIVATE
    ${CMAKE_SOURCE_DIR}/dependency/imgui
    ${CMAKE_SOURCE_DIR}/dependency/imgui/backends
    ${CMAKE_SOURCE_DIR}/dependency/imgui/misc
    ${CMAKE_SOURCE_DIR}/dependency/ImGuizmo
    ${CMAKE_SOURCE_DIR}/Source/VulkanRenderer
    ${PORTABLE_FILE_DIALOGS_INCLUDE_DIRS}
    $ENV{VULKAN_SDK}/Include/
    ${Stb_INCLUDE_DIR}
)

target_link_options(VulkanRenderer PRIVATE /SUBSYSTEM:WINDOWS)

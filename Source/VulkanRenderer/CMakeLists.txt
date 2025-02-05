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
    imgui::imgui
    glfw
    dxgi
    d3d12
    Shell32.lib
)

target_include_directories(VulkanRenderer PRIVATE
    source/VulkanRenderer/
    dependency/optick/src/
    ${PORTABLE_FILE_DIALOGS_INCLUDE_DIRS}
    $ENV{VULKAN_SDK}/Include/
    ${Stb_INCLUDE_DIR}
)

target_link_options(VulkanRenderer PRIVATE /SUBSYSTEM:WINDOWS)

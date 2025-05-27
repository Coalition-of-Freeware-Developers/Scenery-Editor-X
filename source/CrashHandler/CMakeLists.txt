# --------------------------------
# Crash Handler Service
# --------------------------------

MESSAGE(STATUS "=================================================")
MESSAGE(STATUS "Crash Handler Service Project")

# --------------------------------

SET(PROJECT_CONFIG_FILES
	${CMAKE_SOURCE_DIR}/.clang-format
	${CMAKE_SOURCE_DIR}/.editorconfig
	${CMAKE_SOURCE_DIR}/SceneryEditorX.licenseheader
)

SET(PROJECT_RESOURCE_FILES
	${CMAKE_SOURCE_DIR}/source/CrashHandler/CrashHandler.rc
	${CMAKE_SOURCE_DIR}/source/CrashHandler/resource.h
)
# --------------------------------

ADD_EXECUTABLE(CrashHandler
	${PROJECT_CONFIG_FILES}
	${PROJECT_RESOURCE_FILES}
	# --------------------------------
	breakpad.cpp
	crash_handler.cpp
	crash_handler.h
	dialog.cpp
	dialog.h
	service.cpp
	service_ipc.h
	# --------------------------------
)

# --------------------------------

TARGET_INCLUDE_DIRECTORIES(CrashHandler
    PRIVATE
        ${CMAKE_SOURCE_DIR}/source
		${CMAKE_SOURCE_DIR}/dependency
        ${CMAKE_SOURCE_DIR}/dependency/imgui
        ${CMAKE_SOURCE_DIR}/dependency/imgui/backends
        ${CMAKE_SOURCE_DIR}/dependency/imgui/misc
        ${CMAKE_SOURCE_DIR}/dependency/ImGuizmo
        ${REGISTRY_KEYS}
        ${PORTABLE_FILE_DIALOGS_INCLUDE_DIRS}
        $ENV{VULKAN_SDK}/Include/
        ${Stb_INCLUDE_DIR}
        ${CURL_INCLUDE_DIRS}
)

# --------------------------------

TARGET_LINK_LIBRARIES(CrashHandler
    PRIVATE
        glm::glm-header-only
        spdlog::spdlog_header_only
        GPUOpen::VulkanMemoryAllocator
        CURL::libcurl
        Vulkan::Vulkan
		imgui
		libconfig
		libconfig++
        glfw
        dxgi
        d3d12
        Shell32.lib
)

# --------------------------------

TARGET_COMPILE_DEFINITIONS(CrashHandler
    PUBLIC
        EDITOR
        LAUNCHER
        $<$<CONFIG:Debug>:SEDX_DEBUG>
        $<$<CONFIG:Release>:SEDX_RELEASE>
		SPDLOG_USE_STD_FORMAT
        VK_USE_PLATFORM_WIN32_KHR
        GLFW_INCLUDE_VULKAN
        GLFW_EXPOSE_NATIVE_WIN32
        CURL_STATICLIB
)

# --------------------------------

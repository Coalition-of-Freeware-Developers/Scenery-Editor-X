# --------------------------------
# Scenery Editor X | Launcher Application
# --------------------------------

MESSAGE(STATUS "=================================================")
MESSAGE(STATUS "Generating Scenery Editor X Launcher Project")

# --------------------------------
FILE(GLOB CORE core/*.cpp core/*.h)
FILE(GLOB REGISTRY_KEYS registry/*.rgs)
FILE(GLOB UI ui/*.h ui/*.cpp)
SET(LAUNCHER_DIR ${CMAKE_SOURCE_DIR}/source/Launcher)

SET (PROJECT_CONFIG_FILES
	${CMAKE_SOURCE_DIR}/.clang-format
	${CMAKE_SOURCE_DIR}/.editorconfig
	${CMAKE_SOURCE_DIR}/SceneryEditorX.licenseheader
)

# --------------------------------

ADD_EXECUTABLE(Launcher
	${CORE}
	${PROJECT_CONFIG_FILES}
	${REGISTRY_KEYS}
    ${CMAKE_SOURCE_DIR}/assets/splash_screen.bmp
	# --------------------------------
    Launcher.rc
    resource.h
)

# --------------------------------
SOURCE_GROUP("Resource" FILES
	resource.h
	Launcher.rc
)
SOURCE_GROUP("Resource/Images" FILES
    ${CMAKE_SOURCE_DIR}/assets/splash_screen.bmp
)
SOURCE_GROUP("Resource/Registry" FILES
    ${REGISTRY_KEYS}
)
SOURCE_GROUP("Core" FILES
	${CORE}
)
SOURCE_GROUP("Registry" FILES
	${LAUNCHER_DIR}/registry/reg_check.h
	${LAUNCHER_DIR}/registry/reg_init.h
)
SOURCE_GROUP("Registry/Keys" FILES
	${REGISTRY_KEYS}
)

# --------------------------------

TARGET_INCLUDE_DIRECTORIES(Launcher
    PRIVATE
        ${CMAKE_SOURCE_DIR}/source
		${CMAKE_SOURCE_DIR}/dependency
        ${CMAKE_SOURCE_DIR}/dependency/imgui
        ${CMAKE_SOURCE_DIR}/dependency/imgui/backends
        ${CMAKE_SOURCE_DIR}/dependency/imgui/misc
        ${REGISTRY_KEYS}
        ${PORTABLE_FILE_DIALOGS_INCLUDE_DIRS}
        $ENV{VULKAN_SDK}/Include/
        ${Stb_INCLUDE_DIR}
        ${CURL_INCLUDE_DIRS}
)

# --------------------------------

TARGET_LINK_LIBRARIES(Launcher
    PRIVATE
        glm::glm-header-only
        spdlog::spdlog_header_only
        GPUOpen::VulkanMemoryAllocator
        CURL::libcurl
        Vulkan::Vulkan
		imgui
		AppCore
		libconfig
		libconfig++
		X-PlaneSceneryLibrary
        glfw
        dxgi
        d3d12
        Shell32.lib
)

# --------------------------------

TARGET_COMPILE_DEFINITIONS(Launcher
    PUBLIC
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

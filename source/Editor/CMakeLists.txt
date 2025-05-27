# --------------------------------
# Scenery Editor X | Editor Application
# --------------------------------

MESSAGE(STATUS "=================================================")
MESSAGE(STATUS "Generating Scenery Editor X Project")

# --------------------------------

FILE(GLOB CORE core/*.cpp core/*.h core/*.hpp)
FILE(GLOB SETTINGS settings/*.cpp settings/*.h settings/*.hpp)
FILE(GLOB UI ui/*.cpp ui/*.h ui/*.hpp)
SET(LAUNCHER_DIR ${CMAKE_SOURCE_DIR}/source/Launcher)
SET (PROJECT_CONFIG_FILES
	${CMAKE_SOURCE_DIR}/.clang-format
	${CMAKE_SOURCE_DIR}/.editorconfig
	${CMAKE_SOURCE_DIR}/SceneryEditorX.licenseheader
)

# --------------------------------

ADD_EXECUTABLE(SceneryEditorX
	${CORE}
	${SETTINGS}
	${UI}
	${ICON_FILES}
    ${FONT_FILES}
    ${REGISTRY_KEYS}
	${PROJECT_CONFIG_FILES}
    ${CMAKE_SOURCE_DIR}/assets/splash_screen.bmp
    # --------------------------------
    SceneryEditorX.rc
    resource.h
)

# --------------------------------

SOURCE_GROUP("Core" FILES
	${CORE}
)
SOURCE_GROUP("UI" FILES
	${UI}
)
SOURCE_GROUP("UI/Panels" FILES
	${CMAKE_SOURCE_DIR}/source/ui/panels/content_browser.cpp
	${CMAKE_SOURCE_DIR}/source/ui/panels/layer_stack.cpp
	${CMAKE_SOURCE_DIR}/source/ui/panels/loading_radial.cpp
	${CMAKE_SOURCE_DIR}/source/ui/panels/menu_bar.cpp
	${CMAKE_SOURCE_DIR}/source/ui/panels/settings_panel.cpp
)
SOURCE_GROUP("UI/Actions" FILES
	${CMAKE_SOURCE_DIR}/source/ui/actions/modals.cpp
	${CMAKE_SOURCE_DIR}/source/ui/actions/viewports.cpp
)
SOURCE_GROUP("Settings" FILES
	${SETTINGS}
)
SOURCE_GROUP("Resource" FILES
    resource.h
    SceneryEditorX.rc
)
SOURCE_GROUP("Resource/Icons" FILES
    ${ICON_FILES}
)
SOURCE_GROUP("Resource/Images" FILES
    ${CMAKE_SOURCE_DIR}/assets/splash_screen.bmp
)
SOURCE_GROUP("Resource/Fonts" FILES
    ${FONT_FILES}
)
SOURCE_GROUP("Resource/Registry" FILES
    ${REGISTRY_KEYS}
)

# --------------------------------

TARGET_INCLUDE_DIRECTORIES(SceneryEditorX
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

TARGET_LINK_LIBRARIES(SceneryEditorX
    PRIVATE
		AppCore
        glm::glm-header-only
        spdlog::spdlog_header_only
        GPUOpen::VulkanMemoryAllocator
        CURL::libcurl
        Vulkan::Vulkan
		imgui
		libconfig
		libconfig++
		X-PlaneSceneryLibrary
		GraphicsEngine
        glfw
        dxgi
        d3d12
        Shell32.lib
)

# --------------------------------

TARGET_COMPILE_DEFINITIONS(SceneryEditorX
    PUBLIC
        EDITOR
        $<$<CONFIG:Debug>:SEDX_DEBUG>
        $<$<CONFIG:Release>:SEDX_RELEASE>
		SPDLOG_USE_STD_FORMAT
        VK_USE_PLATFORM_WIN32_KHR
        GLFW_INCLUDE_VULKAN
        GLFW_EXPOSE_NATIVE_WIN32
        CURL_STATICLIB
)

# --------------------------------

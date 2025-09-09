# --------------------------------
# Scenery Editor X | Editor Application
# --------------------------------

MESSAGE(STATUS "=================================================")
MESSAGE(STATUS "Generating Scenery Editor X Project")

# --------------------------------

FILE(GLOB CORE core/*.cpp core/*.h core/*.hpp)
FILE(GLOB MODULES modules/*.cpp modules/*.h modules/*.hpp)
FILE(GLOB SETTINGS settings/*.cpp settings/*.h settings/*.hpp)

FILE(GLOB UI_ACTIONS ui/actions/*.cpp ui/actions/*.h ui/actions/*.hpp)
FILE(GLOB UI_PANELS ui/panels/*.cpp ui/panels/*.h ui/panels/*.hpp)
FILE(GLOB UI_RESOURCES ui/resources/*.cpp ui/resources/*.h ui/resources/*.hpp)

SET(UI_FILES ${CMAKE_SOURCE_DIR}/source/Editor/ui)

SET (PROJECT_CONFIG_FILES
	${CMAKE_SOURCE_DIR}/.clang-format
	${CMAKE_SOURCE_DIR}/.editorconfig
	${CMAKE_SOURCE_DIR}/SceneryEditorX.licenseheader
)

# --------------------------------

ADD_EXECUTABLE(SceneryEditorX
	${CORE}
	${MODULES}
	${SETTINGS}
	${UI_ACTIONS}
	${UI_PANELS}
	${UI_RESOURCES}
	${ICON_FILES}
    ${FONT_FILES}
	${PROJECT_CONFIG_FILES}
    # --------------------------------
	${UI_FILES}/ui.cpp
	${UI_FILES}/ui.h
	${UI_FILES}/ui_context.cpp
	${UI_FILES}/ui_context.h
	${UI_FILES}/ui_globals.cpp
	${UI_FILES}/ui_manager.h
	# --------------------------------
    SceneryEditorX.rc
    resource.h
)

# --------------------------------

SOURCE_GROUP("Core" FILES
	${CORE}
)
SOURCE_GROUP("Modules" FILES
	${MODULES}
)
SOURCE_GROUP("UI" FILES
	${UI_FILES}/ui.cpp
	${UI_FILES}/ui.h
	${UI_FILES}/ui_context.cpp
	${UI_FILES}/ui_context.h
	${UI_FILES}/ui_globals.cpp
	${UI_FILES}/ui_manager.h
)
SOURCE_GROUP("UI/Actions" FILES
	${UI_ACTIONS}
)
SOURCE_GROUP("UI/Panels" FILES
	${UI_PANELS}
)
SOURCE_GROUP("UI/Resources" FILES
	${UI_RESOURCES}
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
SOURCE_GROUP("Resource/Fonts" FILES
    ${FONT_FILES}
)

# --------------------------------

TARGET_INCLUDE_DIRECTORIES(SceneryEditorX
    PRIVATE
        ${CMAKE_SOURCE_DIR}/source
		${CMAKE_SOURCE_DIR}/dependency
		${CMAKE_SOURCE_DIR}/dependency/xMath
		${CMAKE_SOURCE_DIR}/dependency/entt/src
        ${CMAKE_SOURCE_DIR}/dependency/imgui
        ${CMAKE_SOURCE_DIR}/dependency/imgui/backends
        ${CMAKE_SOURCE_DIR}/dependency/imgui/misc
        ${CMAKE_SOURCE_DIR}/dependency/ImGuizmo
        ${PORTABLE_FILE_DIALOGS_INCLUDE_DIRS}
        $ENV{VULKAN_SDK}/Include/
        ${Stb_INCLUDE_DIR}
        ${CURL_INCLUDE_DIRS}
)

# --------------------------------

TARGET_LINK_LIBRARIES(SceneryEditorX
    PRIVATE
		AppCore
		nlohmann_json::nlohmann_json
        spdlog::spdlog_header_only
        GPUOpen::VulkanMemoryAllocator
        CURL::libcurl
        Vulkan::Vulkan
		imgui
		libconfig
		libconfig++
		X-PlaneSceneryLibrary
        glfw
        dxgi
        d3d12
        Shell32.lib
		xMath::xMath
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

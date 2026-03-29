# --------------------------------
# Scenery Editor X | Editor Application
# --------------------------------

MESSAGE(STATUS "=================================================")
MESSAGE(STATUS "Generating Scenery Editor X Project")

# --------------------------------
FILE(GLOB CORE LIST_DIRECTORIES false CONFIGURE_DEPENDS core/*.cpp core/*.h core/*.hpp)
FILE(GLOB CORE_CHILD_WINDOWS core/windows/*.cpp core/windows/*.h core/windows/*.hpp)
FILE(GLOB MODULES modules/*.cpp modules/*.h modules/*.hpp)
FILE(GLOB PROJECT projects/*.cpp projects/*.h projects/*.hpp)
FILE(GLOB SETTINGS settings/*.cpp settings/*.h settings/*.hpp)

FILE(GLOB UI_ACTIONS ui/actions/*.cpp ui/actions/*.h ui/actions/*.hpp)
FILE(GLOB UI_PANELS ui/panels/*.cpp ui/panels/*.h ui/panels/*.hpp)
FILE(GLOB UI_RESOURCES ui/resources/*.cpp ui/resources/*.h ui/resources/*.hpp)
FILE(GLOB UI_SOURCE LIST_DIRECTORIES false CONFIGURE_DEPENDS ui/source/*.cpp ui/source/*.h ui/source/*.hpp)
FILE(GLOB UI_IMGUIZMO ui/source/imguizmo/*.cpp ui/source/imguizmo/*.h ui/source/imguizmo/*.hpp)
FILE(GLOB UI_IMGUI LIST_DIRECTORIES false CONFIGURE_DEPENDS ui/source/imgui/*.cpp ui/source/imgui/*.h ui/source/imgui/*.hpp)
FILE(GLOB UI_IMGUI_BACKENDS LIST_DIRECTORIES false CONFIGURE_DEPENDS ui/source/imgui/backends/*.cpp ui/source/imgui/backends/*.h ui/source/imgui/backends/*.hpp)
FILE(GLOB UI_IMGUI_BACKENDS_VULKAN ui/source/imgui/backends/vulkan/*.cpp ui/source/imgui/backends/vulkan/*.h ui/source/imgui/backends/vulkan/*.hpp)
FILE(GLOB UI_IMGUI_CPP ui/source/imgui/misc/cpp/*.cpp ui/source/imgui/misc/cpp/*.h ui/source/imgui/misc/cpp/*.hpp)
FILE(GLOB UI_IMGUI_DEBUGGERS ui/source/imgui/misc/debuggers/*.cpp ui/source/imgui/misc/debuggers/*.h ui/source/imgui/misc/debuggers/*.hpp)
FILE(GLOB UI_IMGUI_FONT ui/source/imgui/misc/font/*.cpp ui/source/imgui/misc/font/*.h ui/source/imgui/misc/font/*.hpp)
FILE(GLOB UI_IMGUI_FREETYPE ui/source/imgui/misc/freetype/*.cpp ui/source/imgui/misc/freetype/*.h ui/source/imgui/misc/freetype/*.hpp)
FILE(GLOB UI_IMGUI_SINGLE ui/source/imgui/misc/single_file/*.cpp ui/source/imgui/misc/single_file/*.h ui/source/imgui/misc/single_file/*.hpp)

FILE(GLOB UI_FILES LIST_DIRECTORIES false CONFIGURE_DEPENDS "ui/*.cpp" "ui/*.h" "ui/*.hpp")

SET (PROJECT_CONFIG_FILES
	${CMAKE_SOURCE_DIR}/.clang-format
	${CMAKE_SOURCE_DIR}/.editorconfig
	${CMAKE_SOURCE_DIR}/SceneryEditorX.licenseheader
)

# --------------------------------
ADD_EXECUTABLE(SceneryEditorX WIN32
	entrypoint.cpp
	${CORE}
	${CORE_CHILD_WINDOWS}
	${MODULES}
	${PROJECT}
	${SETTINGS}
	${UI_ACTIONS}
	${UI_PANELS}
	${UI_RESOURCES}
	${UI_SOURCE}
	${UI_IMGUIZMO}
	${UI_IMGUI}
	${UI_IMGUI_CPP}
	${UI_IMGUI_DEBUGGERS}
	${UI_IMGUI_FONT}
	${UI_IMGUI_FREETYPE}
	${UI_IMGUI_SINGLE}
	${UI_IMGUI_BACKENDS}
	${UI_IMGUI_BACKENDS_VULKAN}
	${ICON_FILES}
	${FONT_FILES}
	${PROJECT_CONFIG_FILES}

	# --------------------------------
	${UI_FILES}

	# --------------------------------
	${RESOURCE_FILE}
	SceneryEditorX.rc
)

# --------------------------------

SOURCE_GROUP("Core" FILES
	${CORE}
)
SOURCE_GROUP("Core/Windows" FILES
	${CORE_CHILD_WINDOWS}
)
SOURCE_GROUP("Modules" FILES
	${MODULES}
)
SOURCE_GROUP("Projects" FILES
	${PROJECT}
)
SOURCE_GROUP("UI" FILES
	${UI_FILES}
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
SOURCE_GROUP("UI/Source/Imguizmo" FILES
	${UI_IMGUIZMO}
)
SOURCE_GROUP("UI/Source/Imgui" FILES
	${UI_IMGUI}
)
SOURCE_GROUP("UI/Source/Imgui/Backends" FILES
	${UI_IMGUI_BACKENDS}
)
SOURCE_GROUP("UI/Source/Imgui/Backends/Vulkan" FILES
	${UI_IMGUI_BACKENDS_VULKAN}
)
SOURCE_GROUP("UI/Source/Imgui/Misc/Cpp" FILES
	${UI_IMGUI_CPP}
)
SOURCE_GROUP("UI/Source/Imgui/Misc/Debuggers" FILES
	${UI_IMGUI_DEBUGGERS}
)
SOURCE_GROUP("UI/Source/Imgui/Misc/Font" FILES
	${UI_IMGUI_FONT}
)
SOURCE_GROUP("UI/Source/Imgui/Misc/Freetype" FILES
	${UI_IMGUI_FREETYPE}
)
SOURCE_GROUP("UI/Source/Imgui/Misc/Single File" FILES
	${UI_IMGUI_SINGLE}
)
SOURCE_GROUP("Settings" FILES
	${SETTINGS}
)
SOURCE_GROUP("Resource" FILES
	${RESOURCE_FILE}
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
		${CMAKE_SOURCE_DIR}/dependency/taskflow
		${CMAKE_SOURCE_DIR}/dependency/ktx/include
		${CMAKE_SOURCE_DIR}/dependency/ktx/other_include
		${PORTABLE_FILE_DIALOGS_INCLUDE_DIRS}
		$ENV{VULKAN_SDK}/Include/
		$ENV{VULKAN_SDK}/Include/vma
		${Stb_INCLUDE_DIR}
		${CURL_INCLUDE_DIRS}
)

# --------------------------------

TARGET_LINK_LIBRARIES(SceneryEditorX
	PRIVATE
		AppCore
		nlohmann_json::nlohmann_json
		spdlog::spdlog_header_only
		CURL::libcurl
		Vulkan::Vulkan
		$<$<AND:$<CONFIG:Debug>,$<BOOL:${TRACY_ENABLE}>>:TracyClient>
		ktx
		libconfig
		libconfig++
		XPSceneryLib::XPSceneryLib
		SDL3::SDL3
		dxgi
		d3d12
		Shell32.lib
		xMath::xMath
		Freetype::Freetype
		edX
)

# --------------------------------

TARGET_COMPILE_DEFINITIONS(SceneryEditorX
	PUBLIC
		EDITOR
		$<$<CONFIG:Debug>:SEDX_DEBUG>
		$<$<CONFIG:Release>:SEDX_RELEASE>
		SPDLOG_USE_STD_FORMAT
		VK_USE_PLATFORM_WIN32_KHR
		VK_USE_64_BIT_NON_DISPATCHABLE_HANDLES
		CURL_STATICLIB
)

# ---------------------------------
IF(SEDX_ENABLE_VERSION_INCREMENT STREQUAL "ON")
	IF (WIN32)
		FIND_PACKAGE(Python3 COMPONENTS Interpreter REQUIRED)

		# Script path
		SET(SEDX_VERSION_SCRIPT ${CMAKE_SOURCE_DIR}/scripts/increment_sedx_build.py)

		IF (MSVC)
			# PRE_BUILD supported by Visual Studio
			ADD_CUSTOM_COMMAND(
				TARGET SceneryEditorX
				PRE_BUILD
				COMMAND ${Python3_EXECUTABLE} ${SEDX_VERSION_SCRIPT}
				WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
				COMMENT "Incrementing Scenery Editor X build number (resource.h)"
				VERBATIM
			)
		ELSE()
			# Fallback for other Windows generators
			ADD_CUSTOM_TARGET(IncrementSceneryEditorXVersion
				COMMAND ${Python3_EXECUTABLE} ${SEDX_VERSION_SCRIPT}
				WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
				COMMENT "Incrementing Scenery Editor X build number (resource.h)"
				VERBATIM
				BYPRODUCTS ${CMAKE_SOURCE_DIR}/source/resource.h
			)
			ADD_DEPENDENCIES(SceneryEditorX IncrementSceneryEditorXVersion)
		ENDIF()
	ELSE()
		IF (WIN32)
			MESSAGE(STATUS "Version increment disabled for QuickDebug configuration")
		ENDIF()
	ENDIF()
ENDIF()
# --------------------------------

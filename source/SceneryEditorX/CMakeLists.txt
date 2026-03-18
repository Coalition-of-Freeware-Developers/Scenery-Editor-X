# --------------------------------
# Scenery Editor X | Application Core
# --------------------------------

MESSAGE(STATUS "=================================================")
MESSAGE(STATUS "Generating Application Core")

# --------------------------------
FILE(GLOB ASSET_ANIMATION_FILES "asset/animation/*.h" "asset/animation/*.hpp" "asset/animation/*.cpp")
FILE(GLOB ASSET_IMPORTER_FILES "asset/importers/*.h" "asset/importers/*.hpp" "asset/importers/*.cpp")
FILE(GLOB ASSET_EXPORTER_FILES "asset/exporters/*.h" "asset/exporters/*.hpp" "asset/exporters/*.cpp")
FILE(GLOB ASSET_MANAGER_FILES "asset/managers/*.h" "asset/managers/*.hpp" "asset/managers/*.cpp")
FILE(GLOB ASSET_MATERIAL_FILES "asset/material/*.h" "asset/material/*.hpp" "asset/material/*.cpp")
FILE(GLOB ASSET_MESH_FILES "asset/mesh/*.h" "asset/mesh/*.hpp" "asset/mesh/*.cpp")
FILE(GLOB ASSET_REGISTRY_FILES "asset/registry/*.h" "asset/registry/*.hpp" "asset/registry/*.cpp")
FILE(GLOB ASSET_DIR LIST_DIRECTORIES false CONFIGURE_DEPENDS "asset/*.h" "asset/*.hpp" "asset/*.cpp")
FILE(GLOB ASSET_BITMAP_FILES "asset/bitmap/*.h" "asset/bitmap/*.hpp" "asset/bitmap/*.cpp")
FILE(GLOB ASSET_MODEL_FILES "asset/model/*.h" "asset/model/*.hpp" "asset/model/*.cpp")

FILE(GLOB CORE_BASE_ROOT LIST_DIRECTORIES false CONFIGURE_DEPENDS "core/*.h" "core/*.hpp" "core/*.cpp")
FILE(GLOB CORE_APP "core/application/*.h" "core/application/*.hpp" "core/application/*.cpp")
FILE(GLOB CORE_EVENTS "core/events/*.h" "core/events/*.hpp" "core/events/*.cpp")
FILE(GLOB CORE_IDENTIFIERS "core/identifiers/*.h" "core/identifiers/*.hpp" "core/identifiers/*.cpp")
FILE(GLOB CORE_INPUT "core/input/*.h" "core/input/*.hpp" "core/input/*.cpp")
FILE(GLOB CORE_MEMORY "core/memory/*.h" "core/memory/*.hpp" "core/memory/*.cpp")
FILE(GLOB CORE_LAYERS "core/layers/*.h" "core/layers/*.hpp" "core/layers/*.cpp")
FILE(GLOB CORE_THREADING "core/threading/*.h" "core/threading/*.hpp" "core/threading/*.cpp")
FILE(GLOB CORE_TIME "core/time/*.h" "core/time/*.hpp" "core/time/*.cpp")
FILE(GLOB CORE_RESOURCE "core/resource/*.cpp" "core/resource/*.h")
FILE(GLOB CORE_WINDOW "core/window/*.h" "core/window/*.hpp" "core/window/*.cpp")
FILE(GLOB CORE_PLATFORM_ROOT LIST_DIRECTORIES false CONFIGURE_DEPENDS "core/platform/*.h" "core/platform/*.hpp" "core/platform/*.cpp")
FILE(GLOB CORE_PLATFORM_CONFIG_FILES "core/platform/config/*.h" "core/platform/config/*.hpp" "core/platform/config/*.cpp")
FILE(GLOB CORE_PLATFORM_FILESYSTEM "core/platform/filesystem/*.h" "core/platform/filesystem/*.hpp" "core/platform/filesystem/*.cpp")
FILE(GLOB CORE_WINDOWS_PLATFORM "core/platform/windows/*.h" "core/platform/windows/*.hpp" "core/platform/windows/*.cpp")
FILE(GLOB CORE_MAC_PLATFORM "core/platform/mac/*.h" "core/platform/mac/*.hpp" "core/platform/mac/*.cpp")
FILE(GLOB CORE_LINUX_PLATFORM "core/platform/linux/*.h" "core/platform/linux/*.hpp" "core/platform/linux/*.cpp")

# --------------------------------

FILE(GLOB RENDERER_FILES LIST_DIRECTORIES false CONFIGURE_DEPENDS "renderer/*.h" "renderer/*.hpp" "renderer/*.cpp")

# --------------------------------

FILE(GLOB RENDERER_VULKAN LIST_DIRECTORIES false CONFIGURE_DEPENDS "renderer/vulkan/*.h" "renderer/vulkan/*.hpp" "renderer/vulkan/*.cpp")
FILE(GLOB RENDERER_DEBUG_FILES "renderer/vulkan/debug/*.h" "renderer/vulkan/debug/*.hpp" "renderer/vulkan/debug/*.cpp")
FILE(GLOB RENDERER_PIPELINE_FILES "renderer/vulkan/pipeline/*.h" "renderer/vulkan/pipeline/*.hpp" "renderer/vulkan/pipeline/*.cpp")
FILE(GLOB RENDERER_SHADER_FILES "renderer/vulkan/shader/*.h" "renderer/vulkan/shader/*.hpp" "renderer/vulkan/shader/*.cpp")
FILE(GLOB RENDERER_SYNC_FILES "renderer/vulkan/sync/*.h" "renderer/vulkan/sync/*.hpp" "renderer/vulkan/sync/*.cpp")

# --------------------------------

FILE(GLOB RENDERER_KOSMICKRISP LIST_DIRECTORIES false CONFIGURE_DEPENDS "renderer/kosmickrisp/*.h" "renderer/kosmickrisp/*.hpp" "renderer/kosmickrisp/*.cpp")

# TODO: Add file groups for the renderer files, such as render passes, shaders, etc.

# --------------------------------

FILE(GLOB LOGGER "logging/*.cpp" "logging/*.h" "logging/*.hpp")
FILE(GLOB PROJECT "project/*.cpp" "project/*.h" "project/*.hpp")
FILE(GLOB REFLECTION_FILES "utils/reflection/*.cpp" "utils/reflection/*.h" "utils/reflection/*.hpp")
FILE(GLOB SCENE LIST_DIRECTORIES false CONFIGURE_DEPENDS "scene/*.cpp" "scene/*.h" "scene/*.hpp")
FILE(GLOB SCENE_COMPONENTS "scene/components/*.cpp" "scene/components/*.h" "scene/components/*.hpp")
FILE(GLOB SLANG_SHADERS LIST_DIRECTORIES false CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/resources/shaders/*.slang")
FILE(GLOB SHADER_FILES "shaders/*.slang" "shaders/*.glsl" "shaders/*.frag" "shaders/*.vert" "shaders/*.comp" "shaders/*.geom" "shaders/*.tesc" "shaders/*.tese")
FILE(GLOB SERIALIZER_FILES "serialization/*.h" "serialization/*.cpp" "serialization/*.hpp")
FILE(GLOB SETTINGS_FILES "settings/*.h" "settings/*.hpp" "settings/*.cpp")
FILE(GLOB SCENE_FILES "scene/*.cpp" "scene/*.h" "scene/*.hpp")

FILE(GLOB UTILS_ROOT LIST_DIRECTORIES false CONFIGURE_DEPENDS "utils/*.cpp" "utils/*.h" "utils/*.hpp")
FILE(GLOB UTILS_REFLECTION "utils/reflection/*.cpp" "utils/reflection/*.h" "utils/reflection/*.hpp")
FILE(GLOB UTILS_CONVERSIONS "utils/conversions/*.cpp" "utils/conversions/*.h" "utils/conversions/*.hpp")
FILE(GLOB UTILS_FILESTREAM "utils/filestreaming/*.cpp" "utils/filestreaming/*.h" "utils/filestreaming/*.hpp")

# FILE(GLOB VULKAN_RENDER_PASS "renderer/render_pass/*.cpp" "renderer/render_pass/*.h" "renderer/render_pass/*.hpp")
# FILE(GLOB VULKAN_SHADER_CODE "renderer/shaders/*.cpp" "renderer/shaders/*.h" "renderer/shaders/*.hpp")
#FILE(GLOB UI_ROOT LIST_DIRECTORIES false CONFIGURE_DEPENDS "ui/*.cpp" "ui/*.h" "ui/*.hpp")
#FILE(GLOB UI_ACTIONS "ui/actions/*.cpp" "ui/actions/*.h" "ui/actions/*.hpp")
#FILE(GLOB UI_PANELS "ui/panels/*.cpp" "ui/panels/*.h" "ui/panels/*.hpp")
#FILE(GLOB FONT_CODE "renderer/fonts/*.h" "renderer/fonts/*.hpp" "renderer/fonts/*.cpp")
SET (PROJECT_CONFIG_FILES
	${CMAKE_SOURCE_DIR}/.clang-format
	${CMAKE_SOURCE_DIR}/.editorconfig
	${CMAKE_SOURCE_DIR}/SceneryEditorX.licenseheader
)

ADD_EXECUTABLE(glslang::validator IMPORTED)
FIND_PROGRAM(GLSLANG_VALIDATOR "glslangValidator" HINTS $ENV{VULKAN_SDK}/bin REQUIRED)
SET_PROPERTY(TARGET glslang::validator PROPERTY IMPORTED_LOCATION "${GLSLANG_VALIDATOR}")

# --------------------------------
ADD_LIBRARY(AppCore
	${ASSET_ANIMATION_FILES}
	${ASSET_IMPORTER_FILES}
	${ASSET_EXPORTER_FILES}
	${ASSET_MATERIAL_FILES}
	${ASSET_MANAGER_FILES}
	${ASSET_MESH_FILES}
	${ASSET_REGISTRY_FILES}
	${ASSET_DIR}
	${ASSET_BITMAP_FILES}
	${ASSET_MODEL_FILES}

	${CORE_APP}
	${CORE_EVENTS}
	${CORE_IDENTIFIERS}
	${CORE_INPUT}
	${CORE_MEMORY}
	${CORE_LAYERS}
	${CORE_RESOURCE}
	${CORE_THREADING}
	${CORE_TIME}
	${CORE_WINDOW}

	${LOGGER}
	${PROJECT}
	${SETTINGS_FILES}

	${CORE_BASE_ROOT}
	${CORE_PLATFORM_ROOT}
	${CORE_PLATFORM_CONFIG_FILES}
	${CORE_PLATFORM_FILESYSTEM}
	${CORE_WINDOWS_PLATFORM}
	${CORE_MAC_PLATFORM}
	${CORE_LINUX_PLATFORM}

	${RENDERER_FILES}
	${RENDERER_VULKAN}
	${RENDERER_DEBUG_FILES}
	${RENDERER_PIPELINE_FILES}
	${RENDERER_SHADER_FILES}
	${RENDERER_SYNC_FILES}
	${RENDERER_KOSMICKRISP}

	${SCENE}
	${SCENE_COMPONENTS}
	${PROJECT_CONFIG_FILES}
	${SERIALIZER_FILES}
	${SCENE_FILES}
	${REFLECTION_FILES}
	${SLANG_SHADERS}

	# ${DEBUG_RENDERER_FILES}

	# ${VULKAN_RENDER_PASS}
	# ${VULKAN_SHADER_CODE}
	# ${FONT_CODE}
	# ${UI_ROOT}
	# ${UI_PANELS}
	# ${UI_ACTIONS}
	${UTILS_ROOT}
	${UTILS_REFLECTION}
	${UTILS_CONVERSIONS}
	${UTILS_FILESTREAM}

	# --------------------------------
	${RESOURCE_FILE}
	cpp.hint
)

# --------------------------------
SOURCE_GROUP("Asset" FILES
	${ASSET_DIR}
)
SOURCE_GROUP("Asset/Animation" FILES
	${ASSET_ANIMATION_FILES}
)
SOURCE_GROUP("Asset/Bitmap" FILES
	${ASSET_BITMAP_FILES}
)
SOURCE_GROUP("Asset/Importers" FILES
	${ASSET_IMPORTER_FILES}
)
SOURCE_GROUP("Asset/Exporters" FILES
	${ASSET_EXPORTER_FILES}
)
SOURCE_GROUP("Asset/Material" FILES
	${ASSET_MATERIAL_FILES}
)
SOURCE_GROUP("Asset/Managers" FILES
	${ASSET_MANAGER_FILES}
)
SOURCE_GROUP("Asset/Model" FILES
	${ASSET_MODEL_FILES}
)
SOURCE_GROUP("Asset/Registry" FILES
	${ASSET_REGISTRY_FILES}
)
SOURCE_GROUP("Core" FILES
	${CORE_BASE_ROOT}
)
SOURCE_GROUP("Core/Application" FILES
	${CORE_APP}
)
SOURCE_GROUP("Core/Events" FILES
	${CORE_EVENTS}
)
SOURCE_GROUP("Core/Identifiers" FILES
	${CORE_IDENTIFIERS}
)
SOURCE_GROUP("Core/Input" FILES
	${CORE_INPUT}
)
SOURCE_GROUP("Core/Logging" FILES
	${LOGGER}
)
SOURCE_GROUP("Core/Memory" FILES
	${CORE_MEMORY}
)
SOURCE_GROUP("Core/Layers" FILES
	${CORE_LAYERS}
)
SOURCE_GROUP("Core/Threading" FILES
	${CORE_THREADING}
)
SOURCE_GROUP("Core/Time" FILES
	${CORE_TIME}
)
SOURCE_GROUP("Core/Window" FILES
	${CORE_WINDOW}
)
SOURCE_GROUP("Core/Resource" FILES
	${CORE_RESOURCE}
)
SOURCE_GROUP("Core/Platform" FILES
	${CORE_PLATFORM_ROOT}
)
SOURCE_GROUP("Core/Platform/Config" FILES
	${CORE_PLATFORM_CONFIG_FILES}
)
SOURCE_GROUP("Core/Platform/Files" FILES
	${CORE_PLATFORM_FILESYSTEM}
)
SOURCE_GROUP("Core/Platform/Windows" FILES
	${CORE_WINDOWS_PLATFORM}
)
SOURCE_GROUP("Core/Platform/Mac" FILES
	${CORE_MAC_PLATFORM}
)
SOURCE_GROUP("Core/Platform/Linux" FILES
	${CORE_LINUX_PLATFORM}
)
SOURCE_GROUP("Project" FILES
	${PROJECT}
)
SOURCE_GROUP("Renderer" FILES
	${RENDERER_FILES}
)
SOURCE_GROUP("Renderer/Fonts" FILES
	${FONT_CODE}
)
SOURCE_GROUP("Renderer/Kosmickrisp" FILES
	${RENDERER_KOSMICKRISP}
)
SOURCE_GROUP("Renderer/Shaders" FILES
	${VULKAN_SHADER_CODE}
)
SOURCE_GROUP("Renderer/Vulkan" FILES
	${RENDERER_VULKAN}
	${VULKAN_FILES}
)
SOURCE_GROUP("Renderer/Vulkan/Asset" FILES
	${RENDERER_ASSET_FILES}
)
SOURCE_GROUP("Renderer/Vulkan/Debug" FILES
	${RENDERER_DEBUG_FILES}
)
SOURCE_GROUP("Renderer/Vulkan/Pipeline" FILES
	${RENDERER_PIPELINE_FILES}
)
SOURCE_GROUP("Renderer/Vulkan/Shader" FILES
	${RENDERER_SHADER_FILES}
)
SOURCE_GROUP("Renderer/Vulkan/Synchronization" FILES
	${RENDERER_SYNC_FILES}
)
SOURCE_GROUP("Resources" FILES
	${RESOURCE_FILE}
	cpp.hint
)
SOURCE_GROUP("Resources/Fonts" FILES
	${FONT_FILES}
)
SOURCE_GROUP("Resources/Icons" FILES
	${ICON_FILES}
)
SOURCE_GROUP("Resources/Shaders" FILES ${SLANG_SHADERS})
SOURCE_GROUP("Scene" FILES
	${SCENE}
	${SCENE_FILES}
)
SOURCE_GROUP("Scene/Components" FILES
	${SCENE_COMPONENTS}
)
SOURCE_GROUP("Scene/Camera" FILES
	${SCENE_CAMERA}
)
SOURCE_GROUP("Settings" FILES
	${SETTINGS_FILES}
)
SOURCE_GROUP("Serialization" FILES
	${SERIALIZER_FILES}
)
#SOURCE_GROUP("UI" FILES
#	${UI_ROOT}
#)
#SOURCE_GROUP("UI/Actions" FILES ${UI_ACTIONS})
#SOURCE_GROUP("UI/Panels" FILES ${UI_PANELS})
SOURCE_GROUP("Utilities" FILES
	${UTILS_ROOT}
)
SOURCE_GROUP("Utilities/File Streaming" FILES
	${UTILS_FILESTREAM}
)
SOURCE_GROUP("Utilities/Reflection" FILES
	${UTILS_REFLECTION}
)
SOURCE_GROUP("Utilities/Conversions" FILES
	${UTILS_CONVERSIONS}
)
SOURCE_GROUP("Utilities/Reflection" FILES ${REFLECTION_FILES})

# --------------------------------
# SHADERS
# --------------------------------
# TODO: Needs to be updated to use the SLang compiler instead of glslang.

IF(WIN32)
    cmake_path(SET GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin/glslangValidator.exe")
ELSE()
    cmake_path(SET GLSL_VALIDATOR "$ENV{VULKAN_SDK}/bin/glslangValidator")
ENDIF()
MESSAGE("GLSL VALIDATOR ${GLSL_VALIDATOR}")
ADD_COMPILE_DEFINITIONS(GLSL_VALIDATOR="${GLSL_VALIDATOR}")
TARGET_COMPILE_FEATURES(AppCore PRIVATE cxx_std_20)

# --------------------------------
TARGET_INCLUDE_DIRECTORIES(AppCore
	PRIVATE
	${CMAKE_SOURCE_DIR}/source
	${CMAKE_SOURCE_DIR}/dependency
	${CMAKE_SOURCE_DIR}/dependency/xMath
	${CMAKE_SOURCE_DIR}/dependency/entt/src
	${CMAKE_SOURCE_DIR}/dependency/imgui
	${CMAKE_SOURCE_DIR}/dependency/tracy/public
	${CMAKE_SOURCE_DIR}/dependency/imgui/backends
	${CMAKE_SOURCE_DIR}/dependency/imgui/misc
	${CMAKE_SOURCE_DIR}/dependency/ImGuizmo
	${CMAKE_SOURCE_DIR}/dependency/taskflow
	${CMAKE_SOURCE_DIR}/dependency/tinyobj
	${CMAKE_SOURCE_DIR}/dependency/ktx/include
	${CMAKE_SOURCE_DIR}/dependency/ktx/other_include
	${REGISTRY_KEYS}
	${PORTABLE_FILE_DIALOGS_INCLUDE_DIRS}
	$ENV{VULKAN_SDK}/Include/
	${Stb_INCLUDE_DIR}
	${CURL_INCLUDE_DIRS}
)

# --------------------------------

TARGET_LINK_LIBRARIES(AppCore
	PRIVATE
		nlohmann_json::nlohmann_json
		spdlog::spdlog_header_only
		GPUOpen::VulkanMemoryAllocator
		CURL::libcurl
		Vulkan::Vulkan
		imgui
		libconfig
		libconfig++
		XPSceneryLib::XPSceneryLib
    	SDL3::SDL3
		dxgi
		d3d12
		ktx
		${Slang_LIBRARY}
		Shell32.lib
		xMath::xMath
		$<$<AND:$<CONFIG:Debug>,$<BOOL:${TRACY_ENABLE}>>:TracyClient>
		msdfgen::msdfgen
)

# --------------------------------

TARGET_COMPILE_DEFINITIONS(AppCore
	PUBLIC
		APPCORE
		$<$<CONFIG:Debug>:SEDX_DEBUG>
		$<$<CONFIG:Release>:SEDX_RELEASE>
		SPDLOG_USE_STD_FORMAT
		VK_NO_PROTOTYPES
		VK_USE_PLATFORM_WIN32_KHR
		CURL_STATICLIB
)

# --------------------------------

# --------------------------------
# Scenery Editor X | Application Core
# --------------------------------

MESSAGE(STATUS "=================================================")
MESSAGE(STATUS "Generating Application Core")

# --------------------------------
FILE(GLOB ASSET_ANIMATION_FILES asset/animation/*.h asset/animation/*.hpp asset/animation/*.cpp)
FILE(GLOB ASSET_IMPORTER_FILES asset/importers/*.h asset/importers/*.hpp asset/importers/*.cpp)
FILE(GLOB ASSET_EXPORTER_FILES asset/exporters/*.h asset/exporters/*.hpp asset/exporters/*.cpp)
FILE(GLOB ASSET_MANAGER_FILES asset/managers/*.h asset/managers/*.hpp asset/managers/*.cpp)
FILE(GLOB ASSET_MATERIAL_FILES asset/material/*.h asset/material/*.hpp asset/material/*.cpp)
FILE(GLOB ASSET_MESH_FILES asset/mesh/*.h asset/mesh/*.hpp asset/mesh/*.cpp)
FILE(GLOB ASSET_REGISTRY_FILES asset/registry/*.h asset/registry/*.hpp asset/registry/*.cpp)

FILE(GLOB CORE_APP core/application/*.h core/application/*.hpp core/application/*.cpp)
FILE(GLOB CORE_EVENTS core/events/*.h core/events/*.hpp core/events/*.cpp)
FILE(GLOB CORE_IDENTIFIERS core/identifiers/*.h core/identifiers/*.hpp core/identifiers/*.cpp)
FILE(GLOB CORE_INPUT core/input/*.h core/input/*.hpp core/input/*.cpp)
FILE(GLOB CORE_MEMORY core/memory/*.h core/memory/*.hpp core/memory/*.cpp)
FILE(GLOB CORE_LAYERS core/layers/*.h core/layers/*.hpp core/layers/*.cpp)
FILE(GLOB CORE_THREADING core/threading/*.h core/threading/*.hpp core/threading/*.cpp)
FILE(GLOB CORE_TIME core/time/*.h core/time/*.hpp core/time/*.cpp)
FILE(GLOB CORE_WINDOW core/window/*.h core/window/*.hpp core/window/*.cpp)
FILE(GLOB CORE_PLATFORM_CONFIG_FILES core/platform/config/*.h core/platform/config/*.hpp core/platform/config/*.cpp)
FILE(GLOB CORE_PLATFORM_FILESYSTEM core/platform/filesystem/*.h core/platform/filesystem/*.hpp core/platform/filesystem/*.cpp)
FILE(GLOB CORE_PLATFORM_SETTINGS core/platform/settings/*.h core/platform/settings/*.hpp core/platform/settings/*.cpp)

FILE(GLOB CORE_WINDOWS_PLATFORM core/platform/windows/*.h core/platform/windows/*.hpp core/platform/windows/*.cpp)
FILE(GLOB CORE_MAC_PLATFORM core/platform/mac/*.h core/platform/mac/*.hpp core/platform/mac/*.cpp)
FILE(GLOB CORE_LINUX_PLATFORM core/platform/linux/*.h core/platform/linux/*.hpp core/platform/linux/*.cpp)

FILE(GLOB LOGGER logging/*.cpp logging/*.h logging/*.hpp)
FILE(GLOB PROJECT project/*.cpp project/*.h project/*.hpp)
FILE(GLOB DEBUG_RENDERER_FILES renderer/debug/*.cpp renderer/debug/*.h renderer/debug/*.hpp)
FILE(GLOB REFLECTION_FILES utils/reflection/*.cpp utils/reflection/*.h utils/reflection/*.hpp)
FILE(GLOB SCENE scene/*.cpp scene/*.h scene/*.hpp)
FILE(GLOB SHADER_FILES shaders/*.glsl shaders/*.frag shaders/*.vert shaders/*.comp shaders/*.geom shaders/*.tesc shaders/*.tese)
FILE(GLOB SERIALIZER_FILES serialization/*.h serialization/*.cpp serialization/*.hpp)
FILE(GLOB SCENE_FILES scene/*.cpp scene/*.h scene/*.hpp)

FILE(GLOB UTILS_REFLECTION utils/reflection/*.cpp utils/reflection/*.h utils/reflection/*.hpp)
FILE(GLOB UTILS_CONVERSIONS utils/conversions/*.cpp utils/conversions/*.h utils/conversions/*.hpp)
FILE(GLOB UTILS_FILESTREAM utils/filestreaming/*.cpp utils/filestreaming/*.h utils/filestreaming/*.hpp)

#FILE(GLOB VULKAN_RENDER_PASS renderer/render_pass/*.cpp renderer/render_pass/*.h renderer/render_pass/*.hpp)
#FILE(GLOB VULKAN_SHADER_CODE renderer/shaders/*.cpp renderer/shaders/*.h renderer/shaders/*.hpp)

#FILE(GLOB UI_ACTIONS ui/actions/*.cpp ui/actions/*.h ui/actions/*.hpp)
#FILE(GLOB UI_PANELS ui/panels/*.cpp ui/panels/*.h ui/panels/*.hpp)
#FILE(GLOB FONT_CODE renderer/fonts/*.h renderer/fonts/*.hpp renderer/fonts/*.cpp)

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
	${CORE_APP}
	${CORE_EVENTS}
	${CORE_IDENTIFIERS}
	${CORE_INPUT}
	${CORE_MEMORY}
	${CORE_LAYERS}
	${CORE_THREADING}
	${CORE_TIME}
	${CORE_WINDOW}
	${LOGGER}
	${PROJECT}
	${CORE_PLATFORM_CONFIG_FILES}
	${CORE_PLATFORM_FILESYSTEM}
	${CORE_PLATFORM_SETTINGS}
	${CORE_WINDOWS_PLATFORM}
	${CORE_MAC_PLATFORM}
	${CORE_LINUX_PLATFORM}
	${SCENE}
	${PROJECT_CONFIG_FILES}
	${SERIALIZER_FILES}
	${SCENE_FILES}
	${REFLECTION_FILES}
	${SHADER_FILES}

	# ${DEBUG_RENDERER_FILES}

	# ${VULKAN_RENDER_PASS}
	# ${VULKAN_SHADER_CODE}
	# ${FONT_CODE}
	# ${UI_PANELS}
	# ${UI_ACTIONS}
	${UTILS_REFLECTION}
	${UTILS_CONVERSIONS}
	${UTILS_FILESTREAM}

	# --------------------------------
	asset/asset.cpp
	asset/asset.h
	asset/asset_extensions.h
	asset/asset_metadata.h
	asset/asset_pack_header.h
	asset/asset_types.h
	asset/ecs.cpp
	asset/ecs.h
	asset/mesh_file.h
	asset/object.cpp
	asset/object.h

	# --------------------------------
	core/base.h
	core/version.h
	core/platform/initializer.cpp
	core/platform/initializer.h
	core/platform/entrypoint.h
	core/platform/platform_context.h

	# --------------------------------
	renderer/command_buffer.cpp
	renderer/command_buffer.h
	renderer/command_manager.cpp
	renderer/command_manager.h
	renderer/command_pool.cpp
	renderer/command_pool.h
	renderer/dds.h
	renderer/device.cpp
	renderer/device.h
	renderer/enums.h
	renderer/framebuffer.cpp
	renderer/framebuffer.h
	renderer/memory_allocator.cpp
	renderer/memory_allocator.h
	renderer/render_config.h
	renderer/render_context.cpp
	renderer/render_context.h
	renderer/render_resource.cpp
	renderer/render_resource.h
	renderer/renderer.cpp
	renderer/renderer.h
	renderer/resource.h
	renderer/stb_image_impl.cpp
	renderer/swapchain.cpp
	renderer/swapchain.h
	renderer/vulkan_buffers.cpp
	renderer/vulkan_buffers.h
	renderer/vulkan_checks.cpp
	renderer/vulkan_checks.h
	renderer/vulkan_data.h
	renderer/vulkan_includes.h
	renderer/vulkan_utils.cpp
	renderer/vulkan_utils.h

	# --------------------------------

	# --------------------------------
	utils/container_utils.h
	utils/delegations.h
	utils/formatter.h
	utils/pointers.h
	utils/size_macro_utils.h
	utils/static_states.h
	utils/string_utils.cpp
	utils/string_utils.h
	utils/system_detection.h
	utils/repeat_call_tracker.h
	utils/repeat_call_tracker.cpp

	# --------------------------------
	${RESOURCE_FILE}
	cpp.hint
)

# --------------------------------
SOURCE_GROUP("Asset" FILES
	asset/asset.cpp
	asset/asset.h
	asset/asset_extensions.h
	asset/asset_metadata.h
	asset/asset_pack_header.h
	asset/asset_types.h
	asset/ecs.cpp
	asset/ecs.h
	asset/mesh_file.h
	asset/object.cpp
	asset/object.h
)
SOURCE_GROUP("Asset/Animation" FILES
	${ASSET_ANIMATION_FILES}
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
SOURCE_GROUP("Asset/Mesh" FILES
	${ASSET_MESH_FILES}
)
SOURCE_GROUP("Asset/Registry" FILES
	${ASSET_REGISTRY_FILES}
)
SOURCE_GROUP("Core" FILES
	core/base.h
	core/version.h
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
SOURCE_GROUP("Core/Platform" FILES
	core/platform/initializer.cpp
	core/platform/initializer.h
	core/platform/entrypoint.h
	core/platform/platform_context.h
)
SOURCE_GROUP("Core/Platform/Config" FILES
	${CORE_PLATFORM_CONFIG_FILES}
)
SOURCE_GROUP("Core/Platform/Files" FILES
	${CORE_PLATFORM_FILESYSTEM}
)
SOURCE_GROUP("Core/Platform/Settings" FILES
	${CORE_PLATFORM_SETTINGS}
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
SOURCE_GROUP("Serialization" FILES
	${SERIALIZER_FILES}
)
#SOURCE_GROUP("Renderer/Vulkan/Buffers" FILES
#	${VULKAN_BUFFERS}
#)
SOURCE_GROUP("Renderer/Debug" FILES
	${DEBUG_RENDERER_FILES}
)
SOURCE_GROUP("Utilities" FILES
	utils/container_utils.h
	utils/delegations.h
	utils/formatter.h
	utils/pointers.h
	utils/size_macro_utils.h
	utils/static_states.h
	utils/string_utils.cpp
	utils/string_utils.h
	utils/system_detection.h
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
SOURCE_GROUP("Steam" FILES
	platform/steam_parser.cpp
	platform/steam_parser.h
)
SOURCE_GROUP("Project" FILES
	${PROJECT}
)
SOURCE_GROUP("Renderer" FILES
	renderer/bindless_descriptor_manager.cpp
	renderer/bindless_descriptor_manager.h
	renderer/command_buffer.cpp
	renderer/command_buffer.h
	renderer/command_manager.cpp
	renderer/command_manager.h
	renderer/command_pool.cpp
	renderer/command_pool.h
	renderer/compute_pass.cpp
	renderer/compute_pass.h
	renderer/compute_pipeline.cpp
	renderer/compute_pipeline.h
	renderer/dds.h
	renderer/descriptor_set.cpp
	renderer/descriptor_set.h
	renderer/descriptor_set_manager.cpp
	renderer/descriptor_set_manager.h
	renderer/descriptors.cpp
	renderer/descriptors.h
	renderer/device.cpp
	renderer/device.h
	renderer/enums.h
	renderer/framebuffer.cpp
	renderer/framebuffer.h
	renderer/image.cpp
	renderer/image.h
	renderer/image_data.cpp
	renderer/image_data.h
	renderer/image_view.cpp
	renderer/image_view.h
	renderer/index_buffer.cpp
	renderer/index_buffer.h
	renderer/memory_allocator.cpp
	renderer/memory_allocator.h
	renderer/pipeline.cpp
	renderer/pipeline.h
	renderer/pipeline_cache.cpp
	renderer/pipeline_cache.h
	renderer/pipeline_state.cpp
	renderer/pipeline_state.h
	renderer/render_config.h
	renderer/render_context.cpp
	renderer/render_context.h
	renderer/render_dispatcher.cpp
	renderer/render_dispatcher.h
	renderer/render_pass.cpp
	renderer/render_pass.h
	renderer/render_resource.cpp
	renderer/render_resource.h
	renderer/renderer.cpp
	renderer/renderer.h
	renderer/resource.h
	renderer/stb_image_impl.cpp
	renderer/staging_buffer.cpp
	renderer/staging_buffer.h
	renderer/storage_buffer.cpp
	renderer/storage_buffer.h
	renderer/swapchain.cpp
	renderer/swapchain.h
	renderer/texture.cpp
	renderer/texture.h
	renderer/uniform_buffer.cpp
	renderer/uniform_buffer.h
	renderer/vertex_buffer.cpp
	renderer/vertex_buffer.h
	renderer/viewport.cpp
	renderer/viewport.h
	renderer/vulkan_buffers.cpp
	renderer/vulkan_buffers.h
	renderer/vulkan_checks.cpp
	renderer/vulkan_checks.h
	renderer/vulkan_data.h
	renderer/vulkan_includes.h
	renderer/vulkan_utils.cpp
	renderer/vulkan_utils.h

)
SOURCE_GROUP("Renderer/Vulkan" FILES
	${VULKAN_FILES}
)
SOURCE_GROUP("Renderer/Fonts" FILES
	${FONT_CODE}
)
SOURCE_GROUP("Renderer/Render Passes" FILES
	${VULKAN_RENDER_PASS}
)
SOURCE_GROUP("Scene" FILES
	${SCENE}
	${SCENE_FILES}
)
SOURCE_GROUP("Renderer/Shaders" FILES
	${VULKAN_SHADER_CODE}
)
SOURCE_GROUP("Utilities" FILES
	utils/container_utils.h
	utils/conversions.cpp
	utils/conversions.h
	utils/formatter.h
	utils/static_states.h
	utils/platform_states.h
	utils/string.cpp
	utils/string.h
	utils/string_utils.h
	utils/repeat_call_tracker.h
	utils/repeat_call_tracker.cpp
)
SOURCE_GROUP("Utilities/Reflection" FILES
	${REFLECTION_FILES}
)
SOURCE_GROUP("UI" FILES
	ui/colors.h
	ui/fonts.h
	ui/styles.cpp
	ui/ui.cpp
	ui/ui.h
	ui/ui_context.cpp
	ui/ui_context.h
	ui/ui_fonts.cpp
	ui/ui_fonts.h
	ui/ui_globals.cpp
	ui/ui_icons.h
	ui/ui_layer.cpp
	ui/ui_layer.h
	ui/ui_manager.cpp
	ui/ui_manager.h
	ui/ui_renderer.cpp
	ui/ui_renderer.h
	ui/ui_utils.cpp
	ui/ui_utils.h
	ui/ui_widgets.cpp
	ui/ui_widgets.h
)
SOURCE_GROUP("UI/Panels" FILES
	${UI_PANELS}
)
SOURCE_GROUP("UI/Actions" FILES
	${UI_ACTIONS}
)
SOURCE_GROUP("Resources" FILES
    ${RESOURCE_FILE}
	cpp.hint
)
SOURCE_GROUP("Resources/Shaders" FILES
	${SHADER_FILES}
)
SOURCE_GROUP("Resources/Icons" FILES
    ${ICON_FILES}
)
SOURCE_GROUP("Resources/Fonts" FILES
    ${FONT_FILES}
)

# --------------------------------
# SHADERS
# --------------------------------
IF(WIN32)
    cmake_path(SET GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin/glslangValidator.exe")
ELSE()
    cmake_path(SET GLSL_VALIDATOR "$ENV{VULKAN_SDK}/bin/glslangValidator")
ENDIF()
MESSAGE("GLSL VALIDATOR ${GLSL_VALIDATOR}")
ADD_COMPILE_DEFINITIONS(GLSL_VALIDATOR="${GLSL_VALIDATOR}")

# --------------------------------

TARGET_INCLUDE_DIRECTORIES(AppCore
    PRIVATE
        ${CMAKE_SOURCE_DIR}/source
		${CMAKE_SOURCE_DIR}/dependency
		${CMAKE_SOURCE_DIR}/dependency/xMath
		${CMAKE_SOURCE_DIR}/dependency/entt/src
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
		glfw
		dxgi
		d3d12
		Shell32.lib
		xMath::xMath
		TracyClient
)

# --------------------------------

TARGET_COMPILE_DEFINITIONS(AppCore
    PUBLIC
        APPCORE
        $<$<CONFIG:Debug>:SEDX_DEBUG>
        $<$<CONFIG:Release>:SEDX_RELEASE>
		SPDLOG_USE_STD_FORMAT
        VK_USE_PLATFORM_WIN32_KHR
        GLFW_INCLUDE_VULKAN
        GLFW_EXPOSE_NATIVE_WIN32
        CURL_STATICLIB
)

# --------------------------------

# --------------------------------
# Scenery Editor X | Application Core
# --------------------------------

MESSAGE(STATUS "=================================================")
MESSAGE(STATUS "Generating Application Core")

# --------------------------------
FILE(GLOB ASSET_FILES  asset/*.h asset/*.hpp asset/*.cpp)
FILE(GLOB CORE_APP core/application/*.h core/application/*.hpp core/application/*.cpp)
FILE(GLOB CORE_EVENTS core/events/*.h core/events/*.hpp core/events/*.cpp)
FILE(GLOB CORE_IDENTIFIERS core/identifiers/*.h core/identifiers/*.hpp core/identifiers/*.cpp)
FILE(GLOB CORE_INPUT core/input/*.h core/input/*.hpp core/input/*.cpp)
FILE(GLOB CORE_MEMORY core/memory/*.h core/memory/*.hpp core/memory/*.cpp)
FILE(GLOB CORE_MODULES core/modules/*.h core/modules/*.hpp core/modules/*.cpp)
FILE(GLOB CORE_THREADING core/threading/*.h core/threading/*.hpp core/threading/*.cpp)
FILE(GLOB CORE_TIME core/time/*.h core/time/*.hpp core/time/*.cpp)
FILE(GLOB CORE_WINDOW core/window/*.h core/window/*.hpp core/window/*.cpp)

FILE(GLOB PLATFORM_FILES platform/files/*.h platform/files/*.hpp platform/files/*.cpp)
FILE(GLOB PLATFORM_SETTINGS platform/settings/*.h platform/settings/*.hpp platform/settings/*.cpp)

FILE(GLOB LOGGER logging/*.cpp logging/*.h logging/*.hpp)
FILE(GLOB PROJECT project/*.cpp project/*.h project/*.hpp)
FILE(GLOB REFLECTION_FILES utils/reflection/*.cpp utils/reflection/*.h utils/reflection/*.hpp)
FILE(GLOB SCENE scene/*.cpp scene/*.h scene/*.hpp)
FILE(GLOB SHADER_FILES shaders/*.glsl shaders/*.frag shaders/*.vert shaders/*.comp shaders/*.geom shaders/*.tesc shaders/*.tese)
FILE(GLOB SERIALIZER_FILES serialization/*.h serialization/*.cpp serialization/*.hpp)
FILE(GLOB SCENE_FILES scene/*.cpp scene/*.h scene/*.hpp)

FILE(GLOB UTILS_MATH utils/math/*.cpp utils/math/*.h utils/math/*.hpp)
FILE(GLOB UTILS_REFLECTION utils/reflection/*.cpp utils/reflection/*.h utils/reflection/*.hpp)
FILE(GLOB UTILS_CONVERSIONS utils/conversions/*.cpp utils/conversions/*.h utils/conversions/*.hpp)
FILE(GLOB UTILS_FILESTREAM utils/filestreaming/*.cpp utils/filestreaming/*.h utils/filestreaming/*.hpp)

FILE(GLOB VULKAN_BUFFERS renderer/buffers/*.h renderer/buffers/*.cpp)
FILE(GLOB VULKAN_FILES renderer/vulkan/*.cpp renderer/vulkan/*.h renderer/vulkan/*.hpp)
FILE(GLOB VULKAN_SHADER_CODE renderer/shaders/*.cpp renderer/shaders/*.h renderer/shaders/*.hpp)
FILE(GLOB VULKAN_DEBUG renderer/debug/*.cpp renderer/debug/*.h renderer/debug/*.hpp)
FILE(GLOB VULKAN_SCENE renderer/scene/*.cpp renderer/scene/*.h renderer/scene/*.hpp)

FILE(GLOB FONT_CODE renderer/fonts/*.h renderer/fonts/*.hpp renderer/fonts/*.cpp)

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
	${ASSET_FILES}
	${CORE_APP}
	${CORE_EVENTS}
	${CORE_IDENTIFIERS}
	${CORE_INPUT}
	${CORE_MEMORY}
	${CORE_MODULES}
	${CORE_THREADING}
	${CORE_TIME}
	${CORE_WINDOW}
	${LOGGER}
	${PROJECT}
	${PLATFORM_FILES}
	${PLATFORM_SETTINGS}
	${SCENE}
	${PROJECT_CONFIG_FILES}
	${SERIALIZER_FILES}
	${SCENE_FILES}
	${REFLECTION_FILES}
	${SHADER_FILES}
	${VULKAN_FILES}
	${VULKAN_BUFFERS}
	${VULKAN_SHADER_CODE}
	${VULKAN_DEBUG}
	${VULKAN_SCENE}
	${FONT_CODE}
	${UTILS_MATH}
	${UTILS_REFLECTION}
	${UTILS_CONVERSIONS}
	${UTILS_FILESTREAM}
    # --------------------------------
	${CMAKE_SOURCE_DIR}/source/SceneryEditorX/EntryPoint.h
	core/base.hpp
	core/initializer.cpp
	core/initializer.h
	core/version.h
	# --------------------------------
	platform/cfg_loader.cpp
	platform/cfg_loader.h
	platform/editor_config.hpp
	platform/system_detection.h
	# --------------------------------
	renderer/2d_renderer.cpp
	renderer/2d_renderer.h
	renderer/command_queue.cpp
	renderer/command_queue.h
	renderer/compute_pass.h
	renderer/dds.h
	renderer/image_data.cpp
	renderer/image_data.h
	renderer/mesh.h
	renderer/primitives.cpp
	renderer/primitives.h
	renderer/render_config.h
	renderer/render_context.cpp
	renderer/render_context.h
	renderer/renderer.cpp
	renderer/renderer.h
	renderer/stb_image_impl.cpp
	renderer/texture.cpp
	renderer/texture.h
	renderer/xp_pbr.cpp
	renderer/xp_pbr.h
	renderer/xp_texture.cpp
	renderer/xp_texture.h
	# --------------------------------
	#ui/colors.h
	#ui/fonts.h
	#ui/ui_context.cpp
	#ui/ui_context.h
	#ui/styles.cpp
	#ui/ui.cpp
	#ui/ui.h
	#ui/ui_manager.h
	#ui/ui_globals.cpp
	#ui/panels/content_browser.cpp
	#ui/panels/layer_stack.cpp
	#ui/panels/loading_radial.cpp
	#ui/panels/menu_bar.cpp
	#ui/panels/settings_panel.cpp
	#ui/actions/modals.cpp
	#ui/actions/viewports.cpp
    # --------------------------------
	utils/container_utils.h
	utils/delegations.h
	utils/formatter.h
	utils/pointers.h
	utils/static_states.h
	utils/string_utils.cpp
	utils/string_utils.h
    # --------------------------------
    resource.h
)

# --------------------------------
SOURCE_GROUP("Asset" FILES
	${ASSET_FILES}
)
SOURCE_GROUP("Core" FILES
	${CMAKE_SOURCE_DIR}/source/SceneryEditorX/EntryPoint.h
	core/base.hpp
	core/initializer.cpp
	core/initializer.h
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
SOURCE_GROUP("Core/Modules" FILES
	${CORE_MODULES}
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
SOURCE_GROUP("Platform" FILES
	platform/cfg_loader.cpp
	platform/cfg_loader.h
	platform/editor_config.hpp
	platform/system_detection.h
)
SOURCE_GROUP("Platform/Files" FILES
	${PLATFORM_FILES}
)
SOURCE_GROUP("Platform/Settings" FILES
	${PLATFORM_SETTINGS}
)
SOURCE_GROUP("Serialization" FILES
	${SERIALIZER_FILES}
)
SOURCE_GROUP("Renderer/Vulkan/Buffers" FILES
	${VULKAN_BUFFERS}
)
SOURCE_GROUP("Renderer/Scene" FILES
	${VULKAN_SCENE}
)
SOURCE_GROUP("Utilities" FILES
	utils/container_utils.h
	utils/delegations.h
	utils/formatter.h
	utils/pointers.h
	utils/static_states.h
	utils/string_utils.cpp
	utils/string_utils.h
)
SOURCE_GROUP("Utilities/Math" FILES
	${UTILS_MATH}
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
	renderer/2d_renderer.cpp
	renderer/2d_renderer.h
	renderer/command_queue.cpp
	renderer/command_queue.h
	renderer/compute_pass.cpp
	renderer/compute_pass.h
	renderer/dds.h
	renderer/image_data.cpp
	renderer/image_data.h
	renderer/mesh.h
	renderer/primitives.cpp
	renderer/primitives.h
	renderer/renderer.cpp
	renderer/renderer.h
	renderer/render_context.cpp
	renderer/render_context.h
	renderer/render_config.h
	renderer/stb_image_impl.cpp
	renderer/texture.cpp
	renderer/texture.h
	renderer/xp_pbr.cpp
	renderer/xp_pbr.h
	renderer/xp_texture.cpp
	renderer/xp_texture.h
)
SOURCE_GROUP("Renderer/Vulkan" FILES
	${VULKAN_FILES}
)
SOURCE_GROUP("Renderer/Debug" FILES
	${VULKAN_DEBUG}
)
SOURCE_GROUP("Renderer/Fonts" FILES
	${FONT_CODE}
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
)
SOURCE_GROUP("Utilities/Reflection" FILES
	${REFLECTION_FILES}
)
SOURCE_GROUP("Renderer/UI" FILES
	ui/colors.h
	ui/fonts.h
	ui/ui_context.cpp
	ui/ui_context.h
	ui/styles.cpp
	ui/ui.cpp
	ui/ui.h
	ui/ui_manager.h
	ui/ui_globals.cpp
)
SOURCE_GROUP("Renderer/UI/Panels" FILES
	ui/panels/about.cpp
	ui/panels/content_browser.cpp
	ui/panels/layer_stack.cpp
	ui/panels/loading_radial.cpp
	ui/panels/menu_bar.cpp
	ui/panels/settings_panel.cpp
)
SOURCE_GROUP("Renderer/UI/Actions" FILES
	ui/actions/modals.cpp
	ui/actions/viewports.cpp
)
SOURCE_GROUP("Resources" FILES
    resource.h
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
        glm::glm-header-only
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

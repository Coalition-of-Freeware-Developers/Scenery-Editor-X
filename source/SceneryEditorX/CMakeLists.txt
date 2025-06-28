# --------------------------------
# Scenery Editor X | Application Core
# --------------------------------

MESSAGE(STATUS "=================================================")
MESSAGE(STATUS "Generating Application Core")

# --------------------------------

FILE(GLOB LOGGER logging/*.cpp logging//*.h logging//*.hpp)
FILE(GLOB PROJECT project/*.cpp project/*.h project/*.hpp)
FILE(GLOB SCENE scene/*.cpp scene/*.h scene/*.hpp)
FILE(GLOB UTILITIES_FILES utils/*.cpp utils/*.h utils/*.hpp)
FILE(GLOB SHADER_FILES shaders/*.glsl shaders/*.frag shaders/*.vert shaders/*.comp shaders/*.geom shaders/*.tesc shaders/*.tese)
FILE(GLOB SCENE_FILES scene/*.cpp scene/*.h scene/*.hpp)
FILE(GLOB VULKAN_FILES renderer/vulkan/*.cpp renderer/vulkan/*.h renderer/vulkan/*.hpp)
FILE(GLOB GFX_DEBUG renderer/debug/*.cpp renderer/debug/*.h renderer/debug/*.hpp)
FILE(GLOB ASSET_FILES asset/*.cpp asset/*.h asset/*.hpp)

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
	${LOGGER}
	${PROJECT}
	${SCENE}
	${ASSET_FILES}
	${PROJECT_CONFIG_FILES}
	${UTILITIES_FILES}
	${SCENE_FILES}
	${SHADER_FILES}
	${VULKAN_FILES}
	${GFX_DEBUG}
    # --------------------------------
	${CMAKE_SOURCE_DIR}/source/SceneryEditorX/EntryPoint.h
	core/application.cpp
	core/application.h
	core/application_data.h
	core/base.hpp
	core/encoding.cpp
	core/encoding.hpp
	core/initializer.cpp
	core/initializer.h
	core/modules/module.cpp
	core/modules/module.h
	core/modules/module_stage.cpp
	core/modules/module_stage.h
	core/memory.cpp
	core/memory.h
	core/pointers.cpp
	core/pointers.h
	core/serializer.hpp
	core/steam_parser.cpp
	core/steam_parser.h
	core/threading/thread.cpp
	core/threading/thread.h
	core/threading/thread_manager.cpp
	core/threading/thread_manager.h
	core/time.cpp
	core/time.h
	core/transforms.cpp
	core/version.h
	core/window/window.cpp
	core/window/window.h
	core/window/icon.h
	core/window/monitor_data.cpp
	core/window/monitor_data.h
	# --------------------------------
	platform/cfg_loader.cpp
	platform/cfg_loader.h
	platform/editor_config.hpp
	platform/file_dialog.cpp
	platform/file_dialog.h
	platform/file_manager.cpp
	platform/file_manager.hpp
	platform/platform_states.h
	platform/settings.cpp
	platform/settings.h
	platform/lock_file.cpp
	platform/system_detection.h
	# --------------------------------
	renderer/command_queue.cpp
	renderer/command_queue.h
	renderer/primitives.cpp
	renderer/primitives.h
	renderer/renderer.cpp
	renderer/renderer.h
	renderer/render_data.h
	renderer/render_context.cpp
	renderer/render_context.h
	renderer/render_config.h
	renderer/stb_image_impl.cpp
	renderer/xp_pbr.cpp
	renderer/xp_pbr.h
	renderer/xp_texture.cpp
	renderer/xp_texture.h
	# --------------------------------
	renderer/shaders/shader.cpp
	renderer/shaders/shader.h
	renderer/shaders/shader_resource.cpp
	renderer/shaders/shader_resource.h
	renderer/shaders/shader_compiler.cpp
	renderer/shaders/shader_compiler.h
	# --------------------------------
	renderer/buffers/framebuffer.cpp
	renderer/buffers/framebuffer.h
	renderer/buffers/index_buffer.h
	renderer/buffers/index_buffer.cpp
	renderer/buffers/storage_buffer.cpp
	renderer/buffers/storage_buffer.h
	renderer/buffers/uniform_buffer.h
	renderer/buffers/uniform_buffer.cpp
	renderer/buffers/vertex_buffer.cpp
	renderer/buffers/vertex_buffer.h
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
    resource.h
)

# --------------------------------
SOURCE_GROUP("Asset" FILES
	${ASSET_FILES}
)
SOURCE_GROUP("Core" FILES
	${CMAKE_SOURCE_DIR}/source/SceneryEditorX/EntryPoint.h
	${LOGGER}
	core/application.cpp
	core/application.h
	core/application_data.h
	core/base.hpp
	core/memory.cpp
	core/memory.h
	core/pointers.cpp
	core/pointers.h
	core/module.cpp
	core/module.h
	core/time.cpp
	core/time.h
	core/version.h
	core/window/monitor_data.cpp
	core/window/monitor_data.h
	core/window/icon.h
	core/window/window.cpp
	core/window/window.h
	core/initializer.cpp
	core/initializer.h
)
SOURCE_GROUP("Core/Threading" FILES
	core/threading/thread.cpp
	core/threading/thread.h
	core/threading/thread_manager.cpp
	core/threading/thread_manager.h
)
SOURCE_GROUP("Core/Modules" FILES
	core/modules/module.cpp
	core/modules/module.h
	core/modules/module_stage.cpp
	core/modules/module_stage.h
)
SOURCE_GROUP("Platform/Settings" FILES
	platform/settings.cpp
	platform/settings.h
)
SOURCE_GROUP("Encoding" FILES
	core/encoding.cpp
	core/encoding.hpp
)
SOURCE_GROUP("Serialization" FILES
	core/serializer.hpp
)
SOURCE_GROUP("Renderer/Vulkan/Buffers" FILES
	renderer/buffers/framebuffer.cpp
	renderer/buffers/framebuffer.h
	renderer/buffers/index_buffer.h
	renderer/buffers/index_buffer.cpp
	renderer/buffers/storage_buffer.cpp
	renderer/buffers/storage_buffer.h
	renderer/buffers/uniform_buffer.h
	renderer/buffers/uniform_buffer.cpp
	renderer/buffers/vertex_buffer.cpp
	renderer/buffers/vertex_buffer.h
)
SOURCE_GROUP("Utilities" FILES
	core/transforms.cpp
	platform/editor_config.hpp
	platform/platform_states.h
	platform/system_detection.h
	platform/file_manager.cpp
	platform/file_manager.hpp
	platform/file_dialog.cpp
	platform/file_dialog.h
	platform/cfg_loader.cpp
	platform/cfg_loader.h
	platform/lock_file.cpp
)
SOURCE_GROUP("Steam" FILES
	core/steam_parser.cpp
	core/steam_parser.h
)
SOURCE_GROUP("Project" FILES
	${PROJECT}
)
SOURCE_GROUP("Renderer" FILES
	renderer/command_queue.cpp
	renderer/command_queue.h
	renderer/primitives.cpp
	renderer/primitives.h
	renderer/renderer.cpp
	renderer/renderer.h
	renderer/render_data.h
	renderer/render_context.cpp
	renderer/render_context.h
	renderer/render_config.h
	renderer/stb_image_impl.cpp
	renderer/xp_pbr.cpp
	renderer/xp_pbr.h
	renderer/xp_texture.cpp
	renderer/xp_texture.h
)
SOURCE_GROUP("Renderer/Vulkan" FILES
	${VULKAN_FILES}
)
SOURCE_GROUP("Renderer/Debug" FILES
	${GFX_DEBUG}
)
SOURCE_GROUP("Scene" FILES
	${SCENE}
	${SCENE_FILES}
)
SOURCE_GROUP("Renderer/Shaders" FILES
	renderer/shaders/shader.cpp
	renderer/shaders/shader.h
	renderer/shaders/shader_resource.cpp
	renderer/shaders/shader_resource.h
	renderer/shaders/shader_compiler.cpp
	renderer/shaders/shader_compiler.h
)
SOURCE_GROUP("Utilities" FILES
	${UTILITIES_FILES}
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

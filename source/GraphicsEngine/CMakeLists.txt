# --------------------------------
# Scenery Editor X | Graphics Engine
# --------------------------------

MESSAGE(STATUS "=================================================")
MESSAGE(STATUS "Generating Scenery Editor X Graphics Engine")

# --------------------------------

FILE(GLOB RENDERER_FILES renderer/*.cpp renderer/*.h renderer/*.hpp)
FILE(GLOB SHADER_FILES shaders/*.glsl shaders/*.frag shaders/*.vert shaders/*.comp shaders/*.geom shaders/*.tesc shaders/*.tese)
FILE(GLOB SCENE_FILES scene/*.cpp scene/*.h scene/*.hpp)
FILE(GLOB VULKAN_FILES vulkan/*.cpp vulkan/*.h vulkan/*.hpp)

SET (PROJECT_CONFIG_FILES
	${CMAKE_SOURCE_DIR}/.clang-format
	${CMAKE_SOURCE_DIR}/.editorconfig
	${CMAKE_SOURCE_DIR}/SceneryEditorX.licenseheader
)

ADD_EXECUTABLE(glslang::validator IMPORTED)
FIND_PROGRAM(GLSLANG_VALIDATOR "glslangValidator" HINTS $ENV{VULKAN_SDK}/bin REQUIRED)
SET_PROPERTY(TARGET glslang::validator PROPERTY IMPORTED_LOCATION "${GLSLANG_VALIDATOR}")

# --------------------------------

ADD_LIBRARY(GraphicsEngine
    ${RENDERER_FILES}
	${SCENE_FILES}
	${SHADER_FILES}
	${VULKAN_FILES}
    # --------------------------------
	renderer/shaders/shader.cpp
	renderer/shaders/shader.h
	renderer/shaders/shader_compiler.cpp
	renderer/shaders/shader_compiler.h
	# --------------------------------
	buffers/buffer_data.h
	buffers/index_buffer.h
	buffers/index_buffer.cpp
	buffers/uniform_buffer.h
	buffers/uniform_buffer.cpp
	buffers/vertex_buffer.cpp
	buffers/vertex_buffer.h
	# --------------------------------
)
SOURCE_GROUP("Core" FILES
	${VULKAN_FILES}
)
SOURCE_GROUP("Core/Buffers" FILES
	buffers/buffer_data.h
	buffers/index_buffer.h
	buffers/index_buffer.cpp
	buffers/uniform_buffer.h
	buffers/uniform_buffer.cpp
	buffers/vertex_buffer.cpp
	buffers/vertex_buffer.h
)
SOURCE_GROUP("Renderer" FILES
    ${RENDERER_FILES}
)
SOURCE_GROUP("Renderer/Shaders" FILES
	renderer/shaders/shader.cpp
	renderer/shaders/shader.h
	renderer/shaders/shader_compiler.cpp
	renderer/shaders/shader_compiler.h
)
SOURCE_GROUP("Scene" FILES
	${SCENE_FILES}
)
SOURCE_GROUP("Shaders" FILES
	${SHADER_FILES}
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

TARGET_INCLUDE_DIRECTORIES(GraphicsEngine
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

TARGET_LINK_LIBRARIES(GraphicsEngine
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

TARGET_COMPILE_DEFINITIONS(GraphicsEngine
    PUBLIC
		GRAPHICS
        $<$<CONFIG:Debug>:SEDX_DEBUG>
        $<$<CONFIG:Release>:SEDX_RELEASE>
		SPDLOG_USE_STD_FORMAT
        VK_USE_PLATFORM_WIN32_KHR
        GLFW_INCLUDE_VULKAN
        GLFW_EXPOSE_NATIVE_WIN32
        CURL_STATICLIB
)

# --------------------------------

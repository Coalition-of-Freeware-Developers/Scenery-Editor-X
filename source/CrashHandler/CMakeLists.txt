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
    ${RESOURCE_FILE}
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
# Set the output name to xService while keeping CrashHandler as target name
SET_TARGET_PROPERTIES(CrashHandler PROPERTIES OUTPUT_NAME "xService")

# --------------------------------
TARGET_INCLUDE_DIRECTORIES(CrashHandler
	PRIVATE
	${CMAKE_SOURCE_DIR}/source
	${CMAKE_SOURCE_DIR}/dependency
	${REGISTRY_KEYS}
	$if(RENDERDOC_ENABLED)
		${RENDERDOC_INCLUDE_DIRS}
	$endif()
	${PORTABLE_FILE_DIALOGS_INCLUDE_DIRS}
	$ENV{VULKAN_SDK}/Include/
	$ENV{VULKAN_SDK}/Include/vma
	${Stb_INCLUDE_DIR}
	${CURL_INCLUDE_DIRS}
)

# --------------------------------

TARGET_LINK_LIBRARIES(CrashHandler
    PRIVATE
		nlohmann_json
        spdlog::spdlog_header_only
        CURL::libcurl
        Vulkan::Vulkan
		libconfig
		libconfig++
    	SDL3::SDL3
        dxgi
        d3d12
        Shell32.lib
		$<$<AND:$<CONFIG:Debug>,$<BOOL:${TRACY_ENABLE}>>:TracyClient>
)

# --------------------------------

TARGET_COMPILE_DEFINITIONS(CrashHandler
    PUBLIC
        EDITOR
        LAUNCHER
		$if(RENDERDOC_ENABLED)
			RENDERDOC_TOOLING_ENABLED
		$endif()
        $<$<CONFIG:Debug>:SEDX_DEBUG>
        $<$<CONFIG:Release>:SEDX_RELEASE>
		SPDLOG_USE_STD_FORMAT
        VK_USE_PLATFORM_WIN32_KHR
		VK_USE_64_BIT_NON_DISPATCHABLE_HANDLES
        CURL_STATICLIB
)

# --------------------------------
# Windows-only: auto-increment CrashHandler build number before compiling
# --------------------------------
IF(SEDX_ENABLE_VERSION_INCREMENT STREQUAL "ON")
	IF(WIN32)
    	FIND_PACKAGE(Python3 COMPONENTS Interpreter REQUIRED)

    	# Script path
    	SET(CRASH_VERSION_SCRIPT ${CMAKE_SOURCE_DIR}/scripts/increment_crash_handler_build.py)

    	IF (MSVC)
    	    # PRE_BUILD supported by Visual Studio
    	    ADD_CUSTOM_COMMAND(
    	        TARGET CrashHandler
    	        PRE_BUILD
    	        COMMAND ${Python3_EXECUTABLE} ${CRASH_VERSION_SCRIPT}
    	        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    	        COMMENT "Incrementing Crash Handler build number (resource.h)"
    	        VERBATIM
    	    )
    	ELSE()
    	    # Fallback for other Windows generators
    	    ADD_CUSTOM_TARGET(IncrementCrashHandlerVersion
    	        COMMAND ${Python3_EXECUTABLE} ${CRASH_VERSION_SCRIPT}
    	        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    	        COMMENT "Incrementing Crash Handler build number (resource.h)"
    	        VERBATIM
    	        BYPRODUCTS ${CMAKE_SOURCE_DIR}/source/resource.h
    	    )
    	    ADD_DEPENDENCIES(CrashHandler IncrementCrashHandlerVersion)
    	ENDIF()
	ENDIF()
ENDIF()
# --------------------------------

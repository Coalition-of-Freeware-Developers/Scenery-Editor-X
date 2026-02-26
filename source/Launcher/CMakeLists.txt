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
    ${ASSET_DIR}/splash_screen.bmp
	# --------------------------------
    ${RESOURCE_FILE}
	Launcher.rc
)

# --------------------------------
SOURCE_GROUP("Resource" FILES
	${RESOURCE_FILE}
	Launcher.rc
)
SOURCE_GROUP("Resource/Images" FILES
    ${ASSET_DIR}/splash_screen.bmp
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
		${CMAKE_SOURCE_DIR}/dependency/xMath
        ${CMAKE_SOURCE_DIR}/dependency/imgui
        ${CMAKE_SOURCE_DIR}/dependency/imgui/backends
        ${CMAKE_SOURCE_DIR}/dependency/imgui/misc
		${CMAKE_SOURCE_DIR}/dependency/ktx/include
		${CMAKE_SOURCE_DIR}/dependency/ktx/other_include
        ${REGISTRY_KEYS}
        ${PORTABLE_FILE_DIALOGS_INCLUDE_DIRS}
        $ENV{VULKAN_SDK}/Include/
        ${Stb_INCLUDE_DIR}
        ${CURL_INCLUDE_DIRS}
)

# --------------------------------

TARGET_LINK_LIBRARIES(Launcher
    PRIVATE
        nlohmann_json
        spdlog::spdlog_header_only
        GPUOpen::VulkanMemoryAllocator
        CURL::libcurl
        Vulkan::Vulkan
		ktx
		imgui
		AppCore
		libconfig
		libconfig++
		XPSceneryLib::XPSceneryLib
        SDL3::SDL3
        dxgi
        d3d12
        Shell32.lib
		xMath::xMath
)

# --------------------------------

TARGET_COMPILE_DEFINITIONS(Launcher
    PUBLIC
        LAUNCHER
        $<$<CONFIG:Debug>:SEDX_DEBUG>
        $<$<CONFIG:Release>:SEDX_RELEASE>
		SPDLOG_USE_STD_FORMAT
        VK_USE_PLATFORM_WIN32_KHR
        CURL_STATICLIB
)

# --------------------------------
IF(SEDX_ENABLE_VERSION_INCREMENT STREQUAL "ON")
	IF (WIN32)
	    FIND_PACKAGE(Python3 COMPONENTS Interpreter REQUIRED)

	    # Script path
	    SET(LAUNCHER_VERSION_SCRIPT ${CMAKE_SOURCE_DIR}/scripts/increment_launcher_build.py)

	    IF (MSVC)
	        # PRE_BUILD supported by Visual Studio
	        ADD_CUSTOM_COMMAND(
	            TARGET Launcher
	            PRE_BUILD
	            COMMAND ${Python3_EXECUTABLE} ${LAUNCHER_VERSION_SCRIPT}
	            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	            COMMENT "Incrementing Launcher build number (resource.h)"
	            VERBATIM
	        )
	    ELSE()
	        # Fallback for other Windows generators
	        ADD_CUSTOM_TARGET(IncrementedLauncherVersion
	            COMMAND ${Python3_EXECUTABLE} ${LAUNCHER_VERSION_SCRIPT}
	            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	            COMMENT "Incrementing Launcher build number (resource.h)"
	            VERBATIM
	            BYPRODUCTS ${CMAKE_SOURCE_DIR}/source/resource.h
	        )
	        ADD_DEPENDENCIES(Launcher IncrementedLauncherVersion)
	    ENDIF()
	ENDIF()
ENDIF()
# --------------------------------

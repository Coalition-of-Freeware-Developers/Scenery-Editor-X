# --------------------------------
# Scenery Editor X - Windows Toolchain
# --------------------------------

# Root of vcpkg inside this repo
SET(VCPKG_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../dependency/vcpkg" CACHE PATH "")

# Use vcpkg triplet unless provided by caller
IF(NOT DEFINED VCPKG_TARGET_TRIPLET)
    SET(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "")
ENDIF()

# Chainload vcpkg (respect a pre-set toolchain to avoid loops)
IF(NOT DEFINED CMAKE_TOOLCHAIN_FILE OR NOT CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg.cmake$")
    INCLUDE("${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
ENDIF()

# Multi-config default config types
SET(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "")

# Language standards
SET(CMAKE_C_STANDARD 17 CACHE STRING "")
SET(CMAKE_C_STANDARD_REQUIRED ON CACHE BOOL "")
SET(CMAKE_CXX_STANDARD 20 CACHE STRING "")
SET(CMAKE_CXX_STANDARD_REQUIRED ON CACHE BOOL "")

# MSVC runtime DLL by default
SET(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" CACHE STRING "")

# Help CMake locate Vulkan SDK if present
IF(DEFINED ENV{VULKAN_SDK})
    LIST(APPEND CMAKE_PREFIX_PATH "$ENV{VULKAN_SDK}")
    SET(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" CACHE STRING "")
ENDIF()

# ---- Apply Windows platform settings ----
# Ensure FetchContent is available for pulling external dependencies
IF(CMAKE_VERSION VERSION_GREATER_EQUAL 3.11)
    IF(POLICY CMP0169)
        CMAKE_POLICY(SET CMP0169 OLD)
    ENDIF()
    INCLUDE(FetchContent)
ELSE()
    MESSAGE(STATUS "CMake < 3.11: FetchContent module is unavailable. Consider upgrading CMake or vendoring dependencies.")
ENDIF()

## Ensure conforming preprocessor for both C and C++ when using MSVC
ADD_COMPILE_OPTIONS(
    $<$<COMPILE_LANG_AND_ID:C,MSVC>:/Zc:preprocessor>
    $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/Zc:preprocessor>
)

# Target Windows 10 SDK if available
IF(NOT DEFINED CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION)
    SET(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION "10.0" CACHE STRING "" FORCE)
ENDIF()

IF(MSVC)
    # Runtime and linking settings
    SET(CMAKE_CXX_SCAN_FOR_MODULES OFF)
    SET(CMAKE_INCREMENTAL_LINKING ON)

    # Common warning level and exceptions
    ADD_COMPILE_OPTIONS(
        $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/W4>
        $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/EHsc>
    )

    # Useful Windows-only macros
    ADD_COMPILE_DEFINITIONS(
        SEDX_PLATFORM_WINDOWS
        UNICODE
        _UNICODE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        VK_USE_PLATFORM_WIN32_KHR
    )
ENDIF()

# Vulkan validation layer settings: try to copy a default config if present
IF(DEFINED ENV{VULKAN_SDK})
    SET(VULKAN_SDK_PATH $ENV{VULKAN_SDK})
    SET(_vk_cfg_src "${VULKAN_SDK_PATH}/Config/vk_layer_settings.txt")
    IF(EXISTS "${_vk_cfg_src}")
        file(COPY "${_vk_cfg_src}" DESTINATION "${CMAKE_BINARY_DIR}")
        MESSAGE(STATUS "Copied Vulkan layer settings from ${_vk_cfg_src}")
    ELSE()
        MESSAGE(STATUS "Vulkan layer settings not found at ${_vk_cfg_src}; skipping copy")
    ENDIF()
ELSE()
    MESSAGE(STATUS "ENV{VULKAN_SDK} not set; skipping Vulkan layer settings copy")
ENDIF()

# Mark platform configuration applied to avoid duplicate work
SET(SEDX_PLATFORM_CONFIG_APPLIED TRUE CACHE BOOL "")

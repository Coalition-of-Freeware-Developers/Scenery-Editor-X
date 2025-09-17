# --------------------------------
# Scenery Editor X - Linux Toolchain
# --------------------------------

# Root of vcpkg inside this repo
SET(VCPKG_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../dependency/vcpkg" CACHE PATH "")

IF(NOT DEFINED VCPKG_TARGET_TRIPLET)
    SET(VCPKG_TARGET_TRIPLET "x64-linux" CACHE STRING "")
ENDIF()

IF(NOT DEFINED CMAKE_TOOLCHAIN_FILE OR NOT CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg.cmake$")
    INCLUDE("${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
ENDIF()

SET(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "")

SET(CMAKE_C_STANDARD 17 CACHE STRING "")
SET(CMAKE_C_STANDARD_REQUIRED ON CACHE BOOL "")
SET(CMAKE_CXX_STANDARD 20 CACHE STRING "")
SET(CMAKE_CXX_STANDARD_REQUIRED ON CACHE BOOL "")

IF(DEFINED ENV{VULKAN_SDK})
    LIST(APPEND CMAKE_PREFIX_PATH "$ENV{VULKAN_SDK}")
    SET(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" CACHE STRING "")
ENDIF()

# ---- Apply Linux platform settings ----
# Ensure FetchContent is available
IF(CMAKE_VERSION VERSION_GREATER_EQUAL 3.11)
    IF(POLICY CMP0169)
        CMAKE_POLICY(SET CMP0169 OLD)
    ENDIF()
    INCLUDE(FetchContent)
ELSE()
    MESSAGE(STATUS "CMake < 3.11: FetchContent is unavailable; consider vendoring dependencies.")
ENDIF()

ADD_COMPILE_OPTIONS(
    $<$<COMPILE_LANG_AND_ID:CXX,Clang,GNU>:-Wall>
    $<$<COMPILE_LANG_AND_ID:CXX,Clang,GNU>:-Wextra>
    $<$<COMPILE_LANG_AND_ID:CXX,Clang,GNU>:-Wpedantic>
)

ADD_COMPILE_DEFINITIONS(
    SEDX_PLATFORM_LINUX
    VK_USE_PLATFORM_XLIB_KHR
)

# Attempt to copy Vulkan layer settings from typical user location
SET(_vk_cfg_src "$ENV{HOME}/.local/share/vulkan/settings.d/vk_layer_settings.txt")
IF(EXISTS "${_vk_cfg_src}")
    FILE(COPY "${_vk_cfg_src}" DESTINATION "${CMAKE_BINARY_DIR}")
    MESSAGE(STATUS "Copied Vulkan layer settings from ${_vk_cfg_src}")
ELSE()
    MESSAGE(STATUS "Vulkan layer settings not found at ${_vk_cfg_src}; skipping copy")
ENDIF()

# Mark platform configuration applied to avoid duplicate work
SET(SEDX_PLATFORM_CONFIG_APPLIED TRUE CACHE BOOL "")

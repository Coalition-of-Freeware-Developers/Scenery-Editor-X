# --------------------------------
# Scenery Editor X - macOS Toolchain
# --------------------------------

SET(VCPKG_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../dependency/vcpkg" CACHE PATH "")

IF(NOT DEFINED VCPKG_TARGET_TRIPLET)
    # Default to arm64; override from presets if targeting Intel
    SET(VCPKG_TARGET_TRIPLET "arm64-osx" CACHE STRING "")
ENDIF()

IF(NOT DEFINED CMAKE_TOOLCHAIN_FILE OR NOT CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg.cmake$")
    INCLUDE("${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
ENDIF()

SET(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "")

SET(CMAKE_C_STANDARD 17 CACHE STRING "")
SET(CMAKE_C_STANDARD_REQUIRED ON CACHE BOOL "")
SET(CMAKE_CXX_STANDARD 20 CACHE STRING "")
SET(CMAKE_CXX_STANDARD_REQUIRED ON CACHE BOOL "")

# macOS deployment settings (override in presets if needed)
SET(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "")
SET(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "")

IF(DEFINED ENV{VULKAN_SDK})
    LIST(APPEND CMAKE_PREFIX_PATH "$ENV{VULKAN_SDK}")
    SET(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" CACHE STRING "")
ENDIF()

# ---- Apply macOS platform settings ----
# Ensure FetchContent is available
IF(CMAKE_VERSION VERSION_GREATER_EQUAL 3.11)
    IF(POLICY CMP0169)
        CMAKE_POLICY(SET CMP0169 OLD)
    ENDIF()
    INCLUDE(FetchContent)
ELSE()
    MESSAGE(STATUS "CMake < 3.11: FetchContent is unavailable; consider vendoring dependencies.")
ENDIF()

# Compiler options
ADD_COMPILE_OPTIONS(
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang,Clang>:-Wall>
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang,Clang>:-Wextra>
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang,Clang>:-Wpedantic>
)
ADD_COMPILE_DEFINITIONS(SEDX_PLATFORM_APPLE)

IF(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    ADD_COMPILE_DEFINITIONS(VK_USE_PLATFORM_MACOS_MVK)
ELSE()
    ADD_COMPILE_DEFINITIONS(VK_USE_PLATFORM_METAL_EXT)
ENDIF()

# Attempt to copy Vulkan layer settings from framework installation if present
SET(_vk_cfg_src "/Library/Frameworks/Vulkan.framework/Resources/vk_layer_settings.txt")
IF(EXISTS "${_vk_cfg_src}")
    FILE(COPY "${_vk_cfg_src}" DESTINATION "${CMAKE_BINARY_DIR}")
    MESSAGE(STATUS "Copied Vulkan layer settings from ${_vk_cfg_src}")
ELSE()
    MESSAGE(STATUS "Vulkan layer settings not found at ${_vk_cfg_src}; skipping copy")
ENDIF()

# Mark platform configuration applied to avoid duplicate work
SET(SEDX_PLATFORM_CONFIG_APPLIED TRUE CACHE BOOL "")

MESSAGE(STATUS "Configuring macOS platform options")

# Ensure FetchContent is available for pulling external dependencies like xMath
IF(CMAKE_VERSION VERSION_GREATER_EQUAL 3.11)
    IF(POLICY CMP0169)
        CMAKE_POLICY(SET CMP0169 OLD)
    ENDIF()
    INCLUDE(FetchContent)
ELSE()
    MESSAGE(STATUS "CMake < 3.11: FetchContent module is unavailable. Consider upgrading CMake or vendoring dependencies.")
ENDIF()

# --------------------------------
# Compiler options
# --------------------------------
ADD_COMPILE_OPTIONS(
	$<$<COMPILE_LANG_AND_ID:CXX,AppleClang,Clang>:-Wall>
	$<$<COMPILE_LANG_AND_ID:CXX,AppleClang,Clang>:-Wextra>
	$<$<COMPILE_LANG_AND_ID:CXX,AppleClang,Clang>:-Wpedantic>
)
ADD_COMPILE_DEFINITIONS( SEDX_PLATFORM_APPLE )

IF(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    ADD_COMPILE_DEFINITIONS(VK_USE_PLATFORM_MACOS_MVK)
ELSE()
	ADD_COMPILE_DEFINITIONS(VK_USE_PLATFORM_METAL_EXT)
ENDIF()

# --------------------------------

# Attempt to copy Vulkan layer settings from framework installation if present
SET(_vk_cfg_src "/Library/Frameworks/Vulkan.framework/Resources/vk_layer_settings.txt")
IF(EXISTS "${_vk_cfg_src}")
	FILE(COPY "${_vk_cfg_src}" DESTINATION "${CMAKE_BINARY_DIR}")
	MESSAGE(STATUS "Copied Vulkan layer settings from ${_vk_cfg_src}")
ELSE()
	MESSAGE(STATUS "Vulkan layer settings not found at ${_vk_cfg_src}; skipping copy")
ENDIF()

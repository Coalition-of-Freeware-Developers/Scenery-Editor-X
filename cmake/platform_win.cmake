MESSAGE(STATUS "Configuring Windows platform options")

# Ensure FetchContent is available for pulling external dependencies like xMath
IF(CMAKE_VERSION VERSION_GREATER_EQUAL 3.11)
    IF(POLICY CMP0169)
        CMAKE_POLICY(SET CMP0169 OLD)
    ENDIF()
    INCLUDE(FetchContent)
ELSE()
    MESSAGE(STATUS "CMake < 3.11: FetchContent module is unavailable. Consider upgrading CMake or vendoring dependencies.")
ENDIF()

# Target Windows 10 SDK if available
IF(NOT DEFINED CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION)
	SET(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION "10.0" CACHE STRING "" FORCE)
ENDIF()

# MSVC/Visual Studio specific tweaks
IF(MSVC)
	# Runtime and linking settings
	SET(CMAKE_CXX_SCAN_FOR_MODULES OFF)
	SET(CMAKE_INCREMENTAL_LINKING ON)
	# Prefer /MDd for Debug by default; adjust if you want static
	SET(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

	# Common warning level and exceptions, modern preprocessor
	ADD_COMPILE_OPTIONS(
		$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/W4>
		$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/EHsc>
		# Enable modern conforming preprocessor for both C and C++
		$<$<COMPILE_LANG_AND_ID:C,MSVC>:/Zc:preprocessor>
		$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/Zc:preprocessor>
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
		FILE(COPY "${_vk_cfg_src}" DESTINATION "${CMAKE_BINARY_DIR}")
		MESSAGE(STATUS "Copied Vulkan layer settings from ${_vk_cfg_src}")
	ELSE()
		MESSAGE(STATUS "Vulkan layer settings not found at ${_vk_cfg_src}; skipping copy")
	ENDIF()
ELSE()
	MESSAGE(STATUS "ENV{VULKAN_SDK} not set; skipping Vulkan layer settings copy")
ENDIF()

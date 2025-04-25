/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* xpeditor_pch.h
* -------------------------------------------------------
* Created: 5/2/2025
* -------------------------------------------------------
*/

#pragma once

#include <SceneryEditorX/platform/system_detection.h>

// -------------------------------------------------------

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

// TODO: Implement MAC and Linux detection
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(WINDOWS) || defined(WIN64)
#ifndef SEDX_PLATFORM_WINDOWS
#define SEDX_PLATFORM_WINDOWS
#endif // !SEDX_PLATFORM_WINDOWS
#endif

// -------------------------------------------------------

#ifdef SEDX_PLATFORM_WINDOWS
    #include <Windows.h>
    #include <fileapi.h>
    constexpr char dirSeparator = '\\';
    #if defined(_DEBUG) || defined(DEBUG)
        #ifndef SEDX_DEBUG
        #define SEDX_DEBUG
        #endif
    #define SEDX_DEBUGBREAK() __debugbreak()
    #define APP_USE_VULKAN_DEBUG_REPORT
    #endif
#endif

// -------------------------------------------------------

#if defined(__GNUC__)
	#if defined(__clang__)
		#define SEDX_COMPILER_CLANG
	#else
		#define SEDX_COMPILER_GCC
	#endif
#elif defined(_MSC_VER)
	#define SEDX_COMPILER_MSVC
#endif

// -------------------------------------------------------

#ifdef SEDX_PLATFORM_LINUX
    #include <unistd.h>
    #include <csignal>
	#include <unistd.h>
	#include <sys/types.h>
	#include <pwd.h>
    const char dirSeparator = '/';
    #if defined(_DEBUG) || defined(DEBUG)
        #ifndef SEDX_DEBUG
        #define SEDX_DEBUG
        #endif
    #define SEDX_DEBUGBREAK() raise(SIGTRAP)
    #endif
#endif

// -------------------------------------------------------

#ifdef SEDX_PLATFORM_MAC
	#include <unistd.h>
	#include <sys/types.h>
	#include <pwd.h>
#error "MAC is not yet supported!"
#endif

/*
##########################################################
                     C++ 20 INCLUDES
##########################################################
*/

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdarg>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

/*
##########################################################
					 GLFW INCLUDES & DEFINES
##########################################################
*/

//#define VK_NO_PROTOTYPES
#define GLFW_INCLUDE_VULKAN
//#define IMGUI_IMPL_VULKAN_USE_VOLK

/*
##########################################################
                        GLM LIBRARY
##########################################################
*/

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

/*
##########################################################
                        CONFIG FILE
##########################################################
*/

#include <stb_image.h> 
#include <portable-file-dialogs.h>
#include <nlohmann/json.hpp>

/*
##########################################################
                        SPDLOG LOGGER
##########################################################
*/

//#include <spdlog/logger.h>
//#include <spdlog/sinks/basic_file_sink.h>
//#include <spdlog/sinks/stdout_color_sinks.h>
//#include <spdlog/spdlog.h>

/*
##########################################################
                     Project Includes
##########################################################
*/

#include <SceneryEditorX/resource.h>
#include <SceneryEditorX/core/base.hpp>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/logging/profiler.hpp>
#include <SceneryEditorX/logging/asserts.h>
#include <SceneryEditorX/platform/platform_states.h>

/*
##########################################################
##########################################################
*/

#ifdef SEDX_DEBUG
	std::filesystem::path workingDir = std::filesystem::current_path();
#endif

namespace fs = std::filesystem;

// -------------------------------------------------------

/**
 * @brief - A macro to display an error message
 * @tparam T 
 * @param errorMessage 
 */
template <typename T>
void ErrMsg(const T &errorMessage)
{
    // Use fmt::format to convert errorMessage to a string
    std::string errorStr = fmt::format("{}", errorMessage);

#ifdef SEDX_PLATFORM_WINDOWS
    // Convert to wide string for Windows
    std::wstring errorWStr(errorStr.begin(), errorStr.end());
    MessageBoxW(nullptr, errorWStr.c_str(), L"Error", MB_OK | MB_ICONERROR);
#else
    // For other platforms, log to console and potentially show via GLFW
    spdlog::error("Error: {}", errorStr);
    // Note: If you have an active GLFW window, you could trigger a custom ImGui popup here
#endif
    throw std::runtime_error(errorStr);
};

// -------------------------------------------------------

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
#include <SceneryEditorX/core/version.h>

// -------------------------------------------------------

// TODO: Impliment MAC and Linux detection
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(WINDOWS) || defined(WIN64)
	#ifndef SEDX_PLATFORM_WINDOWS
	#define SEDX_PLATFORM_WINDOWS
	#endif // !SEDX_PLATFORM_WINDOWS
#endif

// -------------------------------------------------------

#ifdef SEDX_PLATFORM_WINDOWS
    #include <Windows.h>
    #include <fileapi.h>

	#ifdef _MSC_VER
	#define SEDX_DEBUGBREAK() __debugbreak()
	#endif

    #if defined(_DEBUG) || defined(DEBUG)
        #ifndef SEDX_DEBUG
        #define SEDX_DEBUG
        #endif
        #include <SceneryEditorX/logging/asserts.h>

		#define SEDX_ASSERTS
		#define SEDX_ENABLE_VERIFY
		#define APP_USE_VULKAN_DEBUG_REPORT
    #endif
#elif defined(SEDX_COMPILER_CLANG)
	#define SEDX_DEBUGBREAK __builtin_debugtrap()
#else
	#define SEDX_DEBUGBREAK
#endif



// -------------------------------------------------------

#ifdef SEDX_PLATFORM_LINUX
    #include <unistd.h>
    #include <csignal>
	#include <signal.h>
	#if defined(SIGTRAP)
		#define SEDX_DEBUGBREAK() raise(SIGTRAP)
	#else
		#define SEDX_DEBUGBREAK() raise(SIGABRT)
	#endif
#endif

// -------------------------------------------------------

#ifdef SEDX_PLATFORM_MAC
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

#include <spdlog/logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

/*
##########################################################
                     Project Includes
##########################################################
*/

#include <imgui/imgui.h>
#include <ImGuizmo.h>

// -------------------------------------------------------

//#include <SceneryEditorX/core/base.hpp>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/logging/profiler.hpp>
#include <SceneryEditorX/resource.h>

/*
##########################################################
##########################################################
*/

/*
* Type aliases for fixed-width integer types
*/
using u8  = uint8_t;  // Unsigned 8-bit integer
using u16 = uint16_t; // Unsigned 16-bit integer
using u32 = uint32_t; // Unsigned 32-bit integer
using u64 = uint64_t; // Unsigned 64-bit integer
using i8  = int8_t;   // Signed 8-bit integer
using i16 = int16_t;  // Signed 16-bit integer
using i32 = int32_t;  // Signed 32-bit integer
using i64 = int64_t;  // Signed 64-bit integer
using f32 = float;    // 32-bit floating point
using f64 = double;   // 64-bit floating point
using RID = u32;      // Resource Identifier, alias for unsigned 32-bit integer
using byte = uint8_t;

// -------------------------------------------------------

using Vec2 = glm::vec2; // 2D vector
using Vec3 = glm::vec3; // 3D vector
using Vec4 = glm::vec4; // 4D vector

using Mat2 = glm::mat2; // 2x2 matrix
using Mat3 = glm::mat3; // 3x3 matrix
using Mat4 = glm::mat4; // 4x4 matrix

// -------------------------------------------------------

namespace SceneryEditorX
{
	#define INTERNAL static
	#define LOCAL static
	#define GLOBAL static

	inline std::string ToString(const char* str)
    {
        return str ? std::string(str) : std::string("null");
    }
} // namespace SceneryEditorX

// -------------------------------------------------------

#ifdef SEDX_DEBUG
std::filesystem::path workingDir = std::filesystem::current_path();
#endif

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

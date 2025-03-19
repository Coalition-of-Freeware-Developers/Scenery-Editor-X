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

// TODO: Impliment MAC and Linux detection
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(WINDOWS) || defined(WIN64)
#ifndef SEDX_PLATFORM_WINDOWS
#define SEDX_PLATFORM_WINDOWS
#endif // !SEDX_PLATFORM_WINDOWS
#endif

#ifdef SEDX_PLATFORM_WINDOWS
#include <Windows.h>
#include <fileapi.h>
#endif

#ifdef SEDX_PLATFORM_LINUX
#include <unistd.h>
#endif

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

#include <imgui.h>
#include <ImGuizmo.h>
#include <SceneryEditorX/core/base.hpp>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/logging/profiler.hpp>
#include <SceneryEditorX/resource.h>

/*
##########################################################
##########################################################
*/

#define INTERNAL static
#define LOCAL_PERSIST static
#define GLOBAL static

// -------------------------------------------------------

#if defined(_DEBUG) || defined(DEBUG)
#ifndef SEDX_DEBUG
#define SEDX_DEBUG
#endif
#define APP_USE_VULKAN_DEBUG_REPORT
#endif // _DEBUG

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

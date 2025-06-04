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

/// -------------------------------------------------------

/**
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

/**
##########################################################
                    PLATFORM SPECIFIC
##########################################################
*/
#ifdef SEDX_PLATFORM_WINDOWS
    #include <Windows.h>
#endif

/**
##########################################################
					 GLFW INCLUDES & DEFINES
##########################################################
*/

//#define VK_NO_PROTOTYPES
#define GLFW_INCLUDE_VULKAN
//#define IMGUI_IMPL_VULKAN_USE_VOLK

/**
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

/**
##########################################################
                        CONFIG FILE
##########################################################
*/

#include <stb_image.h>
#include <portable-file-dialogs.h>
#include <nlohmann/json.hpp>

/**
##########################################################
                        SPDLOG LOGGER
##########################################################
*/

//#include <spdlog/logger.h>
//#include <spdlog/sinks/basic_file_sink.h>
//#include <spdlog/sinks/stdout_color_sinks.h>
//#include <spdlog/spdlog.h>

/**
##########################################################
                         FMT Library
##########################################################
*/
#include <fmt/core.h>
#include <fmt/format.h>

/**
##########################################################
                     Project Includes
##########################################################
*/

#include <SceneryEditorX/resource.h>
#include <SceneryEditorX/core/pointers.h>
#include <SceneryEditorX/core/base.hpp>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/logging/asserts.h>
#include <SceneryEditorX/platform/platform_states.h>
#include <GraphicsEngine/vulkan/vk_util.h>

/**
##########################################################
					Development Macros
##########################################################
*/

#ifdef SEDX_DEBUG
    extern std::filesystem::path workingDir;
#endif

#ifdef SEDX_DEBUG && SEDX_PROFILING_ENABLED
	#include <SceneryEditorX/logging/profiler.hpp>
	#define TRACY_ENABLE
#endif

/// -------------------------------------------------------

/**
 * @brief - A macro to display an error message
 * @tparam T
 * @param errorMessage
 */
template <typename T>
void ErrMsg(const T &errorMessage)
{
    /// Use fmt::format to convert errorMessage to a string
    std::string errorStr = fmt::format("{}", errorMessage);

#ifdef SEDX_PLATFORM_WINDOWS
    /// Convert to wide string for Windows
    const std::wstring errorWStr(errorStr.begin(), errorStr.end());
    MessageBoxW(nullptr, errorWStr.c_str(), L"Error", MB_OK | MB_ICONERROR);
#endif
#ifdef SEDX_PLATFORM_APPLE
    @autoreleasepool {
		NSString *errorStr = [NSString stringWithUTF8String:errorMessage.c_str()];
		NSString *nsTitle = [NSString stringWithUTF8String:"Error"];

    	NSAlert *alert = [[NSAlert alloc] init];
		[alert setMessageText:nsTitle];
		[alert setInformativeText:errorStr];
		[alert setAlertStyle:NSAlertStyleCritical]; /// Use Critical style for errors
		[alert runModal];
    }
    /// For other platforms, log to console and potentially show via GLFW
    spdlog::error("Error: {}", errorStr);
    /// Note: If you have an active GLFW window, you could trigger a custom ImGui popup here
#endif
#ifdef SEDX_PLATFORM_LINUX
    throw std::runtime_error(errorStr);
#endif
};

// -------------------------------------------------------

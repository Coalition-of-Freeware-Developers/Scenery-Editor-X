#pragma once

#include <core/SystemDetection.h>

// TODO: Impliment MAC and Linux detection
#ifdef SEDX_PLATFORM_WINDOWS
#include <Windows.h>
#include <fileapi.h>
#endif

#ifdef SEDX_PLATFORM_LINUX
#include <unistd.h>
#endif

#ifdef SEDX_PLATFORM_MAC
#error "MAC is not supported!"
#endif

/*
##########################################################
			         GENERAL INCLUDES
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

#include <core/Base.hpp>
#include <core/EdxAssert.h>

#include <logging/Logging.hpp>
#include <logging/Profiler.hpp>


/*
##########################################################
##########################################################
*/

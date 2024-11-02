#pragma once

#include "../src/core/SystemDetection.h"

// TODO: Impliment MAC and Linux detection
#ifdef SEDX_PLATFORM_WINDOWS
    #include <Windows.h>
    #include <fileapi.h>
#endif

/*
##########################################################
			         GENERAL INCLUDES
##########################################################
*/

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <cstdarg>
#include <filesystem>
#include <fstream>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <filesystem>
#include <thread>
#include <iostream>
#include <sstream>
#include <unordered_set>

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

//#include "../portable-file-dialogs.h"

/*
##########################################################
			            SPDLOG LOGGER
##########################################################
*/

#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

/*
##########################################################
			         Project Includes
##########################################################
*/

#include <imgui/imgui.h>
//#include <imgui/ImGuizmo.h>

#include <../src/core/Assert.h>
#include <../src/log/Logging.hpp>
#include <../src/log/Profiler.hpp>
#include <../src/core/Base.hpp>

/*
##########################################################
##########################################################
*/

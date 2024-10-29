#pragma once

#include "../src/core/SystemDetection.h"

// TODO: Impliment MAC and Linux detection
#ifdef SEDX_PLATFORM_WINDOWS
#include <Windows.h>
#endif


/*
##########################################################
			         GENERAL INCLUDES
##########################################################
*/

#include <algorithm>
#include <array>
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
			         Project Includes
##########################################################
*/

//#include <../src/core/Logger.h>
#include <../src/core/Assert.h>
//#include <../src/core/Main.h>

/*
##########################################################
			            GLM LIBRARY
##########################################################
*/

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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
##########################################################
*/


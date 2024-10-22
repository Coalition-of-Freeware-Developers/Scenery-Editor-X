#pragma once

#include "../src/core/SystemDetection.h"

#ifdef SEDX_PLATFORM_WINDOWS
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif
#endif

/*
##########################################################
			         GENERAL INCLUDES
##########################################################
*/

#include <fstream>
#include <iostream>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>
#include <string>
#include <sstream>
#include <array>
#include <vector>
#include <unordered_map>
#include <unordered_set>

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

/*
##########################################################
##########################################################
*/

// TODO: Impliment MAC and Linux detection
#ifdef SEDX_PLATFORM_WINDOWS
	#include <Windows.h>
#endif

#pragma once

#include "SceneryEditorX/src/systemdetection.h"

#ifdef SCENERYEDITORX_PLATFORM_WINDOWS
	#ifndef NOMINMAX

		#define NOMINMAX
	#endif
#endif

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

#ifdef SCENERYEDITORX_PLATFORM_WINDOWS
	#include <Windows.h>
#endif
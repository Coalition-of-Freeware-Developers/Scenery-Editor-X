/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* size_macro_utils.h
* -------------------------------------------------------
* Created: 25/9/2025
* -------------------------------------------------------
*/
#pragma once
#include <cstdio>

// -------------------------------------------------------

namespace SceneryEditorX
{
	// Internal helper: converts bytes into a human-readable string
	inline const char* human_size(double bytes, char* buf, std::size_t bufSize)
	{
	    static const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
	    int unitIndex = 0;
	    while (bytes >= 1024.0 && unitIndex < 5)
		{
	        bytes /= 1024.0;
	        ++unitIndex;
	    }
	    std::snprintf(buf, bufSize, "%.2f %s", bytes, units[unitIndex]);
	    return buf;
	}

	// Macro: expands into a thread-local buffer + formatted string
	// Works safely across Windows (MSVC), Linux (GCC/Clang), and macOS (Clang).
	#define FILE_SIZE(bytes) \
	    ([&]() { \
	        thread_local char __buf[64]; \
	        return human_size(static_cast<double>(bytes), __buf, sizeof(__buf)); \
	    }())

}

// -------------------------------------------------------

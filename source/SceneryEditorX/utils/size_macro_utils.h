/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray 
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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

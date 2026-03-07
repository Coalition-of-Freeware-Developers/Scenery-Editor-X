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
 * profiler.hpp
 * -------------------------------------------------------
 * Created: 4/4/2025
 * -------------------------------------------------------
 */
// ReSharper disable CppClangTidyClangDiagnosticUnusedMacros
// clang-format off
#pragma once

// -------------------------------------------------------

#ifdef SEDX_DEBUG && SEDX_ENABLE_PROFILING
    #define TRACY_ENABLE
    #define SEDX_PROFILING_ENABLED 1
#else SEDX_RELEASE
    #define SEDX_PROFILING_ENABLED 0
#endif

#if SEDX_ENABLE_PROFILING
    #include "tracy/Tracy.hpp"

    // -------------------------------------------------------

    // predefined RGB colors for "heavy" point-of-interest operations
	#define SEDX_PROFILE_COLOR_WAIT 0xff0000
	#define SEDX_PROFILE_COLOR_SUBMIT 0x0000ff
	#define SEDX_PROFILE_COLOR_PRESENT 0x00ff00
	#define SEDX_PROFILE_COLOR_CREATE 0xff6600
	#define SEDX_PROFILE_COLOR_DESTROY 0xffa500
	#define SEDX_PROFILE_COLOR_BARRIER 0xffffff
	#define SEDX_PROFILE_COLOR_CMD_DRAW 0x8b0000
	#define SEDX_PROFILE_COLOR_CMD_COPY 0x8b0a50
	#define SEDX_PROFILE_COLOR_CMD_RTX 0x8b0000
	#define SEDX_PROFILE_COLOR_CMD_DISPATCH 0x8b0000

    // -------------------------------------------------------

	#define SEDX_PROFILE_FUNC()					ZoneScoped
    #define SEDX_PROFILE_FUNC_COLOR(color)		ZoneScopedC(color)
    #define SEDX_PROFILE_FRAME(name)			FrameMarkNamed(name)
    #define SEDX_PROFILE_THREAD(name)			SetThreadName(name)
    #define SEDX_PROFILE_ZONE(name, color) {					\
		ZoneName(name, strlen(name));							\
		ZoneScopedC(color);										
    #define SEDX_PROFILE_ZONE_END() }

#else
	#define SEDX_PROFILER_FUNC()
	#define SEDX_PROFILER_FUNC_COLOR(color)
	#define SEDX_PROFILER_ZONE(name, color) {
	#define SEDX_PROFILER_ZONE_END() }
	#define SEDX_PROFILER_THREAD(name)
	#define SEDX_PROFILER_FRAME(name)
#endif

// -------------------------------------------------------

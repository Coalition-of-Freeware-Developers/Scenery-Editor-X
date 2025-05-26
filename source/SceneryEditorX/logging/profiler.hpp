/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* profiler.hpp
* -------------------------------------------------------
* Created: 4/4/2025
* -------------------------------------------------------
*/
#pragma once

#define SEDX_PROFILING_ENABLED !defined(SEDX_RELEASE)
#if SEDX_PROFILING_ENABLED
#include <tracy/Tracy.hpp>
#endif

/// -------------------------------------------------------

#if SEDX_PROFILING_ENABLED
	#define SEDX_PROFILE_FRAME() FrameMark
	#define SEDX_PROFILE_FUNC(...)			  ZoneScoped##__VA_OPT__(N(__VA_ARGS__))
	#define SEDX_PROFILE_SCOPE(...)			  SEDX_PROFILE_FUNC(__VA_ARGS__)
	#define SEDX_PROFILE_SCOPE_DYNAMIC(NAME)  ZoneScoped; ZoneName(NAME, strlen(NAME))
	#define SEDX_PROFILE_THREAD(...)          tracy::SetThreadName(__VA_ARGS__)
#else
	#define SEDX_PROFILE_MARK_FRAME
	#define SEDX_PROFILE_FUNC(...)
	#define SEDX_PROFILE_SCOPE(...)
	#define SEDX_PROFILE_SCOPE_DYNAMIC(NAME)
	#define SEDX_PROFILE_THREAD(...)
#endif
/// -------------------------------------------------------

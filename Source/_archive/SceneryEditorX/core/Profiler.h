#pragma once

#define SEDX_ENABLE_PROFILING !SEDX_DIST

#if SEDX_ENABLE_PROFILING
#define SEDX_PROFILE_MARK_FRAME FrameMark;
// NOTE: Use SEDX_PROFILE_FUNC ONLY at the top of a function
// Use SEDX_PROFILE_SCOPE / SEDX_PROFILE_SCOPE_DYNAMIC for an inner scope
#define SEDX_PROFILE_FUNC(...) ZoneScoped##__VA_OPT__(N(__VA_ARGS__))
#define SEDX_PROFILE_SCOPE(...) SEDX_PROFILE_FUNC(__VA_ARGS__)
#define SEDX_PROFILE_SCOPE_DYNAMIC(NAME)                                                                                 \
    ZoneScoped;                                                                                                        \
    ZoneName(NAME, strlen(NAME))
#define SEDX_PROFILE_THREAD(...) tracy::SetThreadName(__VA_ARGS__)
#else
#define SEDX_PROFILE_MARK_FRAME
#define SEDX_PROFILE_FUNC(...)
#define SEDX_PROFILE_SCOPE(...)
#define SEDX_PROFILE_SCOPE_DYNAMIC(NAME)
#define SEDX_PROFILE_THREAD(...)
#endif

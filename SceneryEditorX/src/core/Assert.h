#pragma once

#ifdef SEDX_PLATFORM_WINDOWS
    #define SEDX_DEBUG_BREAK __debugbreak()
#elif defined(SEDX_PLATFORM_LINUX)
    #define SEDX_DEBUG_BREAK __builtin_debugtrap()
#else
    #define SEDX_DEBUG_BREAK 
#endif

#ifdef SEDX_DEBUG
    #define SEDX_ASSERTS_ENABLED
#endif



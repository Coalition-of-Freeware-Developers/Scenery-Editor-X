#include <optick.h>

#define SEDX_PROFILE_FRAME() OPTICK_FRAME("MainThread")
#define SEDX_PROFILE_FUNC() OPTICK_EVENT()
#define SEDX_PROFILE_THREAD(name) OPTICK_EVENT((name))
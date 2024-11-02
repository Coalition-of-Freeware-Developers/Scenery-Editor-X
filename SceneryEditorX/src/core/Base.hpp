#pragma once

#include <cstdint>
#include <glm/glm.hpp>

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using f32 = float;
using f64 = double;
using RID = u32;

#define ALIGN_AS(size, alignment) ((size) % (alignment) > 0 ? (size) + (alignment) - (size) % (alignment) : (size))
#define COUNT_OF(arr) (sizeof((arr)) / sizeof((arr)[0]))

#ifdef _WIN32
    #ifndef _WIN64
        #error "x86 not supported!"
    #endif
    #define SEDX_PLATFORM_WINDOWS
#endif

#ifdef __linux__
    #define SEDX_PLATFORM_LINUX
#endif

#ifdef SEDX_DEBUG
    #ifdef SEDX_PLATFORM_WINDOWS
        #define SEDX_DEBUGBREAK() __debugbreak()
    #endif

    #ifdef SEDX_PLATFORM_LINUX
        #include <signal.h>
        #define SEDX_DEBUGBREAK() raise(SIGTRAP)
    #endif
#else
    #define SEDX_DEBUGBREAK()
#endif

#define ASSERT(condition, ...) {if (!(condition)) {LOG_ERROR("[ASSERTION FAILED] {0}", __VA_ARGS__); abort();}}

#ifdef SEDX_DEBUG
#define DEBUG_ASSERT(condition, ...) {if (!(condition)){LOG_ERROR("[ASSERTION FAILED] {0}", __VA_ARGS__); SEDX_DEBUGBREAK(); }}
#define DEBUG_VK(res, ...) {if ((res) != VK_SUCCESS) {LOG_ERROR("[VULKAN ERROR = {0}] {1}", VK_ERROR_STRING((res)), __VA_ARGS__); SEDX_DEBUGBREAK();}}
#else
#define DEBUG_ASSERT(condition, ...)
#define DEBUG_VK(res, ...)
#endif

//#define SEDX_EXPAND_MACRO(x) x
//#define SEDX_STRINGIFY_MACRO(x) #x

//#define BIT(x) (1 << x)

//#define SEDX_BIND_EVENT_FN(fn) [this](auto &&...args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

/*
namespace SceneryEditorX
{
    template <typename T>
    using Scope = std::unique_ptr<T>;
    template <typename T, typename... Args>
    constexpr Scope<T> CreateScope(Args &&...args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }
    
    template <typename T>
    using Ref = std::shared_ptr<T>;
    template <typename T, typename... Args>
    constexpr Ref<T> CreateRef(Args &&...args)
    {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }
}
*/

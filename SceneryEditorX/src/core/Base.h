#pragma once

#include <memory>

#ifdef SEDX_DEBUG
    #if defined(SEDX_PLATFORM_WINDOWS)
        #define SEDX_DEBUGBREAK() __debugbreak()
    #elif defined(SEDX_PLATFORM_LINUX)
        #include <signal.h>
        #define SEDX_DEBUGBREAK() raise(SIGTRAP)
    #else
        #error "Platform doesn't support debugbreak yet!"
    #endif
    #define SEDX_ENABLE_ASSERTS
#else
    #define SEDX_DEBUGBREAK()
#endif

#define SEDX_EXPAND_MACRO(x) x
#define SEDX_STRINGIFY_MACRO(x) #x

#define BIT(x) (1 << x)

#define SEDX_BIND_EVENT_FN(fn) [this](auto &&...args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

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


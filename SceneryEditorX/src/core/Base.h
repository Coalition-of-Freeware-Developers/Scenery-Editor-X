#pragma once

#include <memory>

#ifdef SCENERYEDITORX_DEBUG
    #if defined(SCENERYEDITORX_PLATFORM_WINDOWS)
        #define SCENERYEDITORX_DEBUGBREAK() __debugbreak()
    #elif defined(SCENERYEDITORX_PLATFORM_LINUX)
        #include <signal.h>
        #define SCENERYEDITORX_DEBUGBREAK() raise(SIGTRAP)
    #else
        #error "Platform doesn't support debugbreak yet!"
    #endif
    #define SCENERYEDITORX_ENABLE_ASSERTS
#else
    #define SCENERYEDITORX_DEBUGBREAK()
#endif

#define SCENERYEDITORX_EXPAND_MACRO(x) x
#define SCENERYEDITORX_STRINGIFY_MACRO(x) #x

#define BIT(x) (1 << x)

#define SCENERYEDITORX_BIND_EVENT_FN(fn) [this](auto &&...args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

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


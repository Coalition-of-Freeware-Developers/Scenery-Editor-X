#pragma once

#include <cstdint>
#include <glm/glm.hpp>

/*
* Type aliases for fixed-width integer types
*/
using u8  = uint8_t;  // Unsigned 8-bit integer
using u16 = uint16_t; // Unsigned 16-bit integer
using u32 = uint32_t; // Unsigned 32-bit integer
using u64 = uint64_t; // Unsigned 64-bit integer
using i8  = int8_t;   // Signed 8-bit integer
using i16 = int16_t;  // Signed 16-bit integer
using i32 = int32_t;  // Signed 32-bit integer
using i64 = int64_t;  // Signed 64-bit integer
using f32 = float;    // 32-bit floating point
using f64 = double;   // 64-bit floating point
using RID = u32;      // Resource Identifier, alias for unsigned 32-bit integer

// Macro to align a given size to the specified alignment
#define ALIGN_AS(size, alignment) ((size) % (alignment) > 0 ? (size) + (alignment) - (size) % (alignment) : (size))

// Macro to count the number of elements in an array
#define COUNT_OF(arr) (sizeof((arr)) / sizeof((arr)[0]))

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


/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* platform.h
* -------------------------------------------------------
* Created: 16/3/2025
* -------------------------------------------------------
*/

#pragma once

#include <functional>
#include <memory>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
    void InitializeEditor();
    void ShutdownEditor();
}

#if !defined(SEDX_PLATFORM_WINDOWS) && !defined(SEDX_PLATFORM_LINUX)
#endif

#define BIT(x) (1u << x)

namespace SceneryEditorX
{
    template <typename T>
    using Scope = std::unique_ptr<T>;

    template <typename T,typename... Args>
    constexpr Scope<T> CreateScope(Args &&...args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    using byte = uint8_t;

    template <typename T,typename... Args>
    constexpr Ref<T> CreateRef(Args &&...args)
    {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }
}

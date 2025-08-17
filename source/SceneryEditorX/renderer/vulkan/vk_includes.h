/**
 * -------------------------------------------------------
 * Scenery Editor X - Unified Vulkan Includes
 * -------------------------------------------------------
 * Central header that safely includes Vulkan headers in a way that
 * works across different SDK layouts and suppresses editor lint
 * complaints about missing headers when only vulkan_core.h is present.
 *
 * Usage: replace all direct `#include <vulkan/vulkan.h>` or
 * `#include <vulkan/vulkan_core.h>` with:
 *     #include <SceneryEditorX/utils/vulkan/vk_includes.h>
 *
 * This header prefers the full umbrella header when available but
 * falls back to vulkan_core.h (the subset shipped in some minimal
 * distributions or provided transitively through GLFW) while ensuring
 * prototypes are available via GLFW_INCLUDE_VULKAN or explicit defines.
 * -------------------------------------------------------
 */
#pragma once

// If the build system or another header (GLFW) already defined GLFW_INCLUDE_VULKAN
// we don't redefine it, otherwise we can optionally allow the user to opt-in
// by defining SEDX_FORCE_VK_PROTOTYPES before including this header.
#if !defined(VK_NO_PROTOTYPES) && defined(SEDX_FORCE_VK_PROTOTYPES)
    // Explicitly ensure prototypes; if a loader like volk is adopted later
    // we can gate this behind a different macro.
#endif

// Primary attempt: full umbrella header
#if __has_include(<vulkan/vulkan.h>)
    #include <vulkan/vulkan.h>
#elif __has_include(<vulkan/vulkan_core.h>)
    // Fallback: core header only (definitions & basic prototypes)
    #include <vulkan/vulkan_core.h>
#else
    #error "Neither <vulkan/vulkan.h> nor <vulkan/vulkan_core.h> could be found. Ensure the Vulkan SDK or loader headers are available."
#endif

// Convenience compile-time marker for code that may want to branch.
#ifndef SEDX_VK_INCLUDES_READY
    #define SEDX_VK_INCLUDES_READY 1
#endif

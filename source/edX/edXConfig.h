/**
* -------------------------------------------------------
* Scenery Editor X - edX File Format
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* edXConfig.h
* -------------------------------------------------------
* Created: 27/5/2025
* Updated: 11/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include "../../dependency/json/single_include/nlohmann/json.hpp"

/// ----------------------------------------------------------------------------

// Cross-platform DLL export/import macros
#if defined(_WIN32) || defined(_WIN64)
    #ifdef EDX_EXPORTS
        #define EDX_API __declspec(dllexport)
    #else
        #define EDX_API __declspec(dllimport)
    #endif
#elif defined(__GNUC__) && __GNUC__ >= 4
    #ifdef EDX_EXPORTS
        #define EDX_API __attribute__((visibility("default")))
    #else
        #define EDX_API
    #endif
#else
    #define EDX_API
#endif

// JSON namespace alias for convenience
using json = nlohmann::json;

// Version information
namespace edx {
    constexpr const char* VERSION = "2.0.0";
    constexpr int VERSION_MAJOR = 2;
    constexpr int VERSION_MINOR = 0;
    constexpr int VERSION_PATCH = 0;
}

/// ----------------------------------------------------------------------------


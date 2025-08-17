/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* math_config.h
* -------------------------------------------------------
* Created: 16/8/2025
* -------------------------------------------------------
*/
#pragma once

// -----------------------------------------------------------------------------
// xMath DLL export/import configuration
// -----------------------------------------------------------------------------
// Expected usage:
//   When building the xMath DLL target, CMake defines XMATH_API which causes
//   symbols marked with XMATH_API to be exported.  Downstream consumers that
//   include these headers (without defining XMATH_API) will import the
//   symbols.  On non-Windows platforms the macro collapses to empty so it is
//   harmless.
// -----------------------------------------------------------------------------

// Export macro pattern: when building the DLL define XMATH_BUILD in CMake.
// Consumers do NOT define XMATH_BUILD and thus import the symbols.
#if defined(_WIN32) || defined(_WINDOWS) || defined(SEDX_PLATFORM_WINDOWS)
    #if defined(XMATH_BUILD)
        #define XMATH_API __declspec(dllexport)
    #else
        #define XMATH_API __declspec(dllimport)
    #endif
#else
    #define XMATH_API
#endif

// -----------------------------------------------------------------------------

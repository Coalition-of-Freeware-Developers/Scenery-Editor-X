#pragma once
#ifndef EDX_CONFIG_H
#define EDX_CONFIG_H

// Define the export/import configuration for the library
// Checks for windows platform and if the lib is not being built as a static lib.
#if defined(_WINDOWS) || defined(_WIN32) || defined(SEDX_PLATFORM_WINDOWS) && !defined(SEDX_CORE_STATIC)
#ifdef EDX_API
#define EDX_API __declspec(dllexport)
#else
#define EDX_API __declspec(dllimport)
#endif
#endif

#endif /* EDX_CONFIG_H */

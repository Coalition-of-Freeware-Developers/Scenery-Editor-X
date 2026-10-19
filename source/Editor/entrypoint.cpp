/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* entrypoint.cpp
* -------------------------------------------------------
* Main entry point for the Editor executable.
* Uses the APP_MAIN macro from entrypoint.h which creates
* the platform-appropriate entry point (WinMain on Windows,
* main on Linux/macOS) and wires up the PlatformContext.
* -------------------------------------------------------
*/
#include <SceneryEditorX/core/platform/entrypoint.h>

/// -------------------------------------------------------

/**
 * @brief Platform entry point defined via the APP_MAIN macro.
 *
 * The APP_MAIN macro expands to the correct platform entry point
 * (WinMain on Windows, main on Linux/macOS), creates the
 * PlatformContext, and invokes SceneryEditorX::Main(context).
 *
 * The body below is the platform_initializer function which is
 * called before SceneryEditorX::Main for any pre-init work.
 */
APP_MAIN(context)
{
    // Pre-initialization (before SceneryEditorX::Main is called)
    // Additional platform-specific setup can go here if needed.
    return 0;
}

/// -------------------------------------------------------

/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* entrypoint.cpp
* -------------------------------------------------------
* Linux entry point for the application
* -------------------------------------------------------
* Created: 25/5/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/core/platform/entrypoint.h>
#include <SceneryEditorX/core/platform/linux/context.h>

// -----------------------------------------------

std::unique_ptr<SceneryEditorX::PlatformContext> CreatePlatformContext(int argc, char **argv)
{
    return std::make_unique<SceneryEditorX::LinuxContext>(argc, argv);
}

// -----------------------------------------------

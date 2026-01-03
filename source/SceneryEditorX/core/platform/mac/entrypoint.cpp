/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* entrypoint.cpp
* -------------------------------------------------------
* MacOS application context
* -------------------------------------------------------
* Created: 25/5/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/core/platform/entrypoint.h>
#include <SceneryEditorX/core/platform/mac/context.h>

// -----------------------------------------------

std::unique_ptr<SceneryEditorX::PlatformContext> CreatePlatformContext(int argc, char **argv)
{
    return std::make_unique<SceneryEditorX::MacOSContext>(argc, argv);
}

// -----------------------------------------------

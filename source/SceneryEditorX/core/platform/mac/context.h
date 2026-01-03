/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* context.h
* -------------------------------------------------------
* MacOS application context
* -------------------------------------------------------
* Created: 25/5/2025
* -------------------------------------------------------
*/
#pragma once
#include <string>
#include <SceneryEditorX/core/platform/platform_context.h>

// -----------------------------------------------

namespace SceneryEditorX
{
    class MacOSContext final : public PlatformContext
	{
	public:
        MacOSContext(int argc, char **argv);
        ~MacOSContext() override = default;
	};
}

// -----------------------------------------------

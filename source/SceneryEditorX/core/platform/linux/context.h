/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* context.h
* -------------------------------------------------------
* Linux application context
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
	class LinuxContext final : public PlatformContext
	{
	public:
        LinuxContext(int argc, char **argv);
        ~LinuxContext() override = default;
	};

}

// -----------------------------------------------

/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* initializer.cpp
* -------------------------------------------------------
* Created: 25/5/2025
* -------------------------------------------------------
*/
#include "initializer.h"
#include <SceneryEditorX/core/base.h>
#include <SceneryEditorX/core/memory/memory.h>
#include <SceneryEditorX/logging/logging.hpp>
#include <resource.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

    void InitCore()
    {
		Allocator::Init();
		Log::Init();
        Log::LogHeader();
    }

    void Shutdown()
    {
        Log::ShutDown();
    }

}

// -------------------------------------------------------

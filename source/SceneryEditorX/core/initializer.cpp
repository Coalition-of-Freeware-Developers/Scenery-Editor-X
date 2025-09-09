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
#include "base.hpp"
#include "SceneryEditorX/resource.h"
#include "SceneryEditorX/logging/logging.hpp"
#include "memory/memory.h"

/// -------------------------------------------------------

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

/// -------------------------------------------------------

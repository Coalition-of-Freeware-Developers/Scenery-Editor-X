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
#include <SceneryEditorX/resource.h>
#include <SceneryEditorX/core/initializer.h>
#include <SceneryEditorX/core/base.hpp>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/core/memory.h>

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

} // namespace SceneryEditorX

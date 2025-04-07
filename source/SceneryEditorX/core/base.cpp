/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* base.cpp
* -------------------------------------------------------
* Created: 4/4/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/core/base.hpp>
#include <SceneryEditorX/core/version.h>
#include <SceneryEditorX/logging/logging.hpp>

// -------------------------------------------------------

namespace SceneryEditorX
{
	void InitializeCore()
	{
        Log::Init();
        EDITOR_LOG_TRACE("Logger Initialized");
        SEDX_CORE_TRACE_TAG("Core", "Hazel Engine {}", SEDX_VERSION);
        SEDX_CORE_TRACE_TAG("Core", "Initializing...");
	}

	void ShutdownCore()
    {
        SEDX_CORE_TRACE_TAG("Core", "Shutting down logging system...");
        Log::ShutDown();
    }

} // namespace SceneryEditorX

// -------------------------------------------------------


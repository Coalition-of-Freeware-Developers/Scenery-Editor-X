/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* application.cpp
* -------------------------------------------------------
* Created: 4/4/2025
* -------------------------------------------------------
*/

#include <SceneryEditorX/core/application.h>
#include <SceneryEditorX/logging/Logging.hpp>

// -------------------------------------------------------

namespace SceneryEditorX
{

#define BIND_EVENT_FN(fn) std::bind(&Application::##fn, this, std::placeholders::_1)

	void InitializeCore()
	{
        Log::Init();
        EDITOR_LOG_TRACE("Logger Initialized");
	}

	void ShutdownCore()
    {
        EDITOR_LOG_TRACE("Shutting down logging system...");
        Log::ShutDown();
    }

}


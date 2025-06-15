/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* renderer.cpp
* -------------------------------------------------------
* Created: 7/6/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/core/application.h>
#include <SceneryEditorX/logging/profiler.hpp>
#include <SceneryEditorX/renderer/render_context.h>
#include <SceneryEditorX/renderer/renderer.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

	void Renderer::Init()
	{
	}

    void Renderer::Shutdown()
    {
    }

    Ref<RenderContext> Renderer::GetContext()
    {
        return Application::Get().GetWindow().GetRenderContext();
    }

	uint32_t Renderer::GetCurrentFrameIndex()
    {
        return Application::Get().GetCurrentFrameIndex();
    }

}

/// -------------------------------------------------------

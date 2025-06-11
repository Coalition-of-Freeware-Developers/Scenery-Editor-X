/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* renderer.h
* -------------------------------------------------------
* Created: 7/6/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/core/application.h>
#include <SceneryEditorX/platform/platform_states.h>
#include <SceneryEditorX/renderer/render_context.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    class Renderer
	{
	public:
        typedef void (*RenderCommandFn)(void *);

        GLOBAL Ref<RenderContext> GetContext()
        {
            return Application::Get().GetWindow().GetRenderContext();
        }

        GLOBAL void Init();
		GLOBAL void Shutdown();
	};

}

/// -------------------------------------------------------

/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* frame_data.h
* -------------------------------------------------------
* Created: 31/3/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/scene/camera.h>

// ---------------------------------------------------------

namespace SceneryEditorX
{
	struct FrameInfo
	{
        Camera &camera;
        float frameTime;
        int frameIndex;
	    int width;
	    int height;
	};

} // namespace SceneryEditorX

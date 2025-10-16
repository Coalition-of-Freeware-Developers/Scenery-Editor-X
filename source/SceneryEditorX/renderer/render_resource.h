/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* render_resource.h
* -------------------------------------------------------
* Created: 29/8/2025
* -------------------------------------------------------
*/
#pragma once

/// -------------------------------------------------------

namespace SceneryEditorX
{

    constexpr uint32_t RESOURCE_FRAME_LIFETIME	= 100;		// in frames, how long to keep a resource alive after last use
    constexpr uint32_t MAX_DRAW_CALLS			= 20000;	// max number of draw calls per frame
    constexpr uint32_t MAX_INSTANCE_COUNT		= 1024;     // max number of instances per draw call

    /// -------------------------------------------------------

    class Renderable;
    struct RendererDrawCall
    {
        Renderable *renderable;			// pointer to the renderable object
        uint32_t instanceGroupIndex;	// index of the instance group (used if instanced)
        uint32_t instanceIndex;			// starting index in the instance buffer (used if instanced)
        uint32_t instanceCount;			// number of instances to draw (used if instanced)
        uint32_t lodIndex;				// level of detail index for the mesh
        float distanceSquared;			// distance for sorting or other purposes
        bool isOccluder;				// is this draw call an occluder
        bool cameraVisible;				// is this draw call visible to the camera
    };

}

/// -------------------------------------------------------

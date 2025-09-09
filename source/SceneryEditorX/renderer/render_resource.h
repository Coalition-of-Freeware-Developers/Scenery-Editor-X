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

    constexpr uint32_t resource_frame_lifetime	= 100;		// in frames, how long to keep a resource alive after last use
    constexpr uint32_t max_draw_calls			= 20000;	// max number of draw calls per frame
    constexpr uint32_t max_instance_count		= 1024;     // max number of instances per draw call

    /// -------------------------------------------------------

    class Renderable;
    struct Renderer_DrawCall
    {
        Renderable *renderable;			// pointer to the renderable object
        uint32_t instance_group_index;	// index of the instance group (used if instanced)
        uint32_t instance_index;		// starting index in the instance buffer (used if instanced)
        uint32_t instance_count;		// number of instances to draw (used if instanced)
        uint32_t lod_index;				// level of detail index for the mesh
        float distance_squared;			// distance for sorting or other purposes
        bool is_occluder;				// is this draw call an occluder
        bool camera_visible;			// is this draw call visible to the camera
    };

}

/// -------------------------------------------------------

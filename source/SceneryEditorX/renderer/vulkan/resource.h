/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* resource.h
* -------------------------------------------------------
* Created: 7/6/2025
* -------------------------------------------------------
*/
#pragma once

/// -----------------------------------------------------------

namespace SceneryEditorX
{
    /**
     * @struct Resource
     * @brief Base class for all render-able resources in the graphics system.
     *
     * The Resource class serves as a foundation for all resources that can be 
     * tracked and managed by the rendering system. It provides basic identification
     * through a name and a unique resource ID.
     *
     * Resources include objects such as textures, buffers, shaders, and other
     * GPU-related assets that need to be managed throughout their lifecycle.
     */
	struct Resource : RefCounted
	{
	    /** @brief Descriptive name of the resource for debugging and tracking */
	    std::string name;
	
	    /** @brief Unique identifier for the resource (-1 indicates unassigned) */
	    int32_t resourceID = -1;
	
	    /** @brief Virtual destructor to ensure proper cleanup of derived resources */
        // ReSharper disable once CppEnforceOverridingDestructorStyle
        virtual ~Resource() = default;
	};

    /// -----------------------------------------------------------

	using ResourceDescriptorInfo = void *;

    class RendererResource
    {
    public:
        virtual ResourceDescriptorInfo GetDescriptorInfo() const = 0;
    };

}

/// -------------------------------------------------------

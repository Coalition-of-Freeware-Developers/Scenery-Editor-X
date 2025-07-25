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
#include <SceneryEditorX/utils/pointers.h>

/// -----------------------------------------------------------

namespace SceneryEditorX
{
	using ResourceDescriptorInfo = void *;

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
	struct Resource : public RefCounted
	{
	    /** @brief Descriptive name of the resource for debugging and tracking */
	    std::string name;

		/**
		 * @brief Retrieves descriptor information required for GPU resource binding.
		 *
		 * This pure virtual method must be implemented by all derived resource classes
		 * to provide the necessary descriptor information used by the Vulkan rendering
		 * pipeline for resource binding operations.
		 *
		 * The descriptor information typically contains details about how the resource
		 * should be accessed and bound to shaders, including layout information,
		 * binding points, and access patterns.
		 *
		 * @return ResourceDescriptorInfo Opaque pointer containing resource-specific
		 *         descriptor information. The actual type and content depend on the
		 *         specific resource implementation (e.g., VkDescriptorImageInfo for
		 *         textures, VkDescriptorBufferInfo for buffers).
		 *
		 * @note Derived classes must ensure the returned descriptor information
		 *       remains valid for the lifetime of the resource or until the next
		 *       call to this method.
		 * @note This method should be thread-safe as it may be called from multiple
		 *       rendering threads simultaneously.
		 */
	    virtual ResourceDescriptorInfo GetDescriptorInfo() const = 0;

	    /** @brief Unique identifier for the resource (-1 indicates unassigned) */
	    int32_t resourceID = -1;
	
	    /** @brief Virtual destructor to ensure proper cleanup of derived resources */
        // ReSharper disable once CppEnforceOverridingDestructorStyle
        virtual ~Resource() = default;

	};

    /// -----------------------------------------------------------

}

/// -------------------------------------------------------

/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* image_data.cpp
* -------------------------------------------------------
* Created: 11/5/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/renderer/vulkan/image_data.h>
#include <imgui/imgui.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    /**
     * @brief Retrieves the unique resource ID for this image.
     *
     * This method asserts that the underlying resource has a valid resource ID assigned.
     * The resource ID is used to uniquely identify the image within the graphics system.
     *
     * @return The unique resource ID as a uint32_t.
     * @throws Assertion failure if the resource ID is invalid (-1).
     */
    uint32_t Image::ID() const
    {
        SEDX_ASSERT(resource->resourceID != -1, "Invalid Image Resource ID!");
        return static_cast<uint32_t>(resource->resourceID);
    }

	/**
	 * @brief Retrieves the ImGui texture ID for the first layer of this image.
	 *
	 * This method returns the ImGui texture ID (ImTextureID) associated with the first layer
	 * of the image resource. The ImGui texture ID is used to display the image in ImGui widgets.
	 * If the image resource is invalid or does not have any ImGui texture IDs assigned,
	 * the method returns a null ImTextureID.
	 *
	 * @return ImTextureID for the first image layer, or nullptr if unavailable.
	 */
	ImTextureID Image::ImGuiRID() const
	{
	    if (!resource || resource->resourceID == -1 || resource->imguiRIDs.empty())
	    {
	        return reinterpret_cast<ImTextureID>(nullptr);
	    }
	    return resource->imguiRIDs[0];
	}

	/**
	 * @brief Retrieves the ImGui texture ID for a specific image layer.
	 *
	 * This method returns the ImGui texture ID (ImTextureID) associated with the specified layer
	 * of the image resource. The ImGui texture ID is used to display the image in ImGui widgets.
	 * If the image resource is invalid, the resource ID is not assigned, or the requested layer
	 * does not have an associated ImGui texture ID, the method returns a null ImTextureID.
	 *
	 * @param layer The index of the image layer for which to retrieve the ImGui texture ID.
	 * @return ImTextureID for the specified image layer, or nullptr if unavailable.
	 */
	ImTextureID Image::ImGuiRID(uint64_t layer) const
	{
		if (!resource || resource->resourceID == -1 || resource->imguiRIDs.size() <= layer)
		{
			return reinterpret_cast<ImTextureID>(nullptr);
		}
		return resource->imguiRIDs[layer];
	}

} // namespace SceneryEditorX

/// -------------------------------------------------------

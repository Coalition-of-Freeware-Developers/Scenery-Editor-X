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
#include <GraphicsEngine/vulkan/image_data.h>
#include <GraphicsEngine/vulkan/vk_core.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/*
	// Move resource cleanup logic to a separate method that can be called explicitly
    void CleanupResources()
	{
	    if (fromSwapchain || resourceID < 0)
	        return;
	
	    try
	    {
	        // Get logical device - safely handle potential null cases
	        VkDevice device = VK_NULL_HANDLE;
	        auto *graphicsEngine = GraphicsEngine::GetInstance();
	        if (graphicsEngine)
	        {
	            auto vkDevice = graphicsEngine->GetLogicDevice();
	            if (vkDevice)
	                device = vkDevice->GetDevice();
	        }
	
	        // Only proceed with cleanup if we have a valid device
	        if (device != VK_NULL_HANDLE)
	        {
	            // Clean up layer views
	            for (auto &layerView : layersView)
	            {
	                if (layerView != VK_NULL_HANDLE)
	                {
	                    vkDestroyImageView(device, layerView, nullptr);
	                    layerView = VK_NULL_HANDLE;
	                }
	            }
	            layersView.clear();
	
	            // Clean up main view
	            if (view != VK_NULL_HANDLE)
	            {
	                vkDestroyImageView(device, view, nullptr);
	                view = VK_NULL_HANDLE;
	            }
	
	            // Clean up image if we have a valid allocator
	            if (graphicsEngine)
	            {
	                VmaAllocator allocator = graphicsEngine->GetLogicDevice()->GetMemoryAllocator();
	                if (allocator != nullptr && allocation != VK_NULL_HANDLE && image != VK_NULL_HANDLE)
	                {
	                    vmaDestroyImage(allocator, image, allocation);
	                    image = VK_NULL_HANDLE;
	                    allocation = VK_NULL_HANDLE;
	                }
	            }
	
	            // Return resource ID to the pool and clean up ImGui resources
	            ImageID::availImageRID.push_back(resourceID);
	            for (ImTextureID imguiRID : imguiRIDs)
	            {
	                if (imguiRID)
	                    ImGui_ImplVulkan_RemoveTexture(reinterpret_cast<VkDescriptorSet>(imguiRID));
	            }
	
	            resourceID = -1;
	            imguiRIDs.clear();
	        }
	    }
	    catch (const std::exception &e)
	    {
	        // Log the error but don't crash
	        SEDX_CORE_ERROR("Exception in ImageResource cleanup: {}", e.what());
	    }
	}

	virtual ~ImageResource() override
    {
        // Skip cleanup for swapchain images as they're managed by the swapchain
        if (fromSwapchain)
            return;

        // Use a static cleanup method to prevent issues with device/allocator access
        CleanupResources();
    }
    */
	

} // namespace SceneryEditorX

// -------------------------------------------------------

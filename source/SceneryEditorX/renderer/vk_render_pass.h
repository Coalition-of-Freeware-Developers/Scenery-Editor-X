/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_render_pass.h
* -------------------------------------------------------
* Created: 16/4/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/renderer/vk_allocator.h>
#include <SceneryEditorX/renderer/vk_core.h>
#include <SceneryEditorX/renderer/vk_device.h>
#include <SceneryEditorX/renderer/vk_render_pass.h>
#include <SceneryEditorX/renderer/vk_swapchain.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

	class RenderPass
	{
	public:
	    RenderPass() = default;
	    virtual ~RenderPass();
	    void CreateRenderPass();

        [[nodiscard]] VkRenderPass GetRenderPass() const
	    {
	        return renderPass;
	    }
	
	private:
        GraphicsEngine *renderer;
        Ref<VulkanDevice> vkDevice;
        Ref<SwapChain> vkSwapChain;
        //Ref<MemoryAllocator> allocator;
	    VkRenderPass renderPass = nullptr;
        VkAllocationCallbacks *allocator = nullptr;
	};

} // namespace SceneryEditorX

// -------------------------------------------------------

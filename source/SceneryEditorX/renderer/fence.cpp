/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* fence.cpp
* -------------------------------------------------------
* Created: 28/8/2025
* -------------------------------------------------------
*/
#include "fence.h"
#include <SceneryEditorX/logging/asserts.h>
#include "render_context.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{
	Fence::Fence(void *&resource)
	{
	    VkFenceCreateInfo fence_info = {};
	    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	
		SEDX_ASSERT(vkCreateFence(RenderContext::Get()->GetLogicDevice()->GetDevice(), &fence_info, nullptr, reinterpret_cast<VkFence*>(&resource)));
	}

    // TODO: Evaluate if the 'Create' function should remain able to be called specifically or keep the RAII setup and create it directly in constructor?
    void Fence::Create(void* &resource)
	{
	    VkFenceCreateInfo fence_info = {};
	    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	
		SEDX_ASSERT(vkCreateFence(RenderContext::Get()->GetLogicDevice()->GetDevice(), &fence_info, nullptr, reinterpret_cast<VkFence*>(&resource)));
	}
	
	bool Fence::IsSignaled(void *&resource)
	{
	    return vkGetFenceStatus(RenderContext::Get()->GetLogicDevice()->GetDevice(), reinterpret_cast<VkFence>(resource)) == VK_SUCCESS;
	}
	
	void Fence::WaitTime(uint64_t timeout, void *&resource)
	{
	    SEDX_ASSERT(vkWaitForFences(RenderContext::Get()->GetLogicDevice()->GetDevice(), 1, reinterpret_cast<VkFence*>(&resource), true, timeout));
	}

    // TODO: Evaluate if the Create function should remain to be called specifically or keep the RAII setup and create it directly in constructor?
	void Fence::Reset(void *&resource)
	{
		SEDX_ASSERT(vkResetFences(RenderContext::Get()->GetLogicDevice()->GetDevice(), 1, reinterpret_cast<VkFence*>(&resource)));
	}
}

/// -------------------------------------------------------

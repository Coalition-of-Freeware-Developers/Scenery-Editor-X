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

// -------------------------------------------------------

namespace SceneryEditorX
{
	Fence::Fence(void *&resource) : m_Resource(resource)
	{
	    SEDX_ASSERT(resource != nullptr, "Fence resource cannot be null");
	}

    void Fence::Create(void* &resource)
	{
	    VkFenceCreateInfo fenceInfo{};
	    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Start in signaled state for first frame
	
		SEDX_ASSERT(vkCreateFence(RenderContext::Get()->GetLogicDevice()->GetDevice(), &fenceInfo,nullptr, reinterpret_cast<VkFence*>(&resource)) == VK_SUCCESS);
	}
	
	bool Fence::IsSignaled(void *&resource)
	{
	    return vkGetFenceStatus(RenderContext::Get()->GetLogicDevice()->GetDevice(), static_cast<VkFence>(resource)) == VK_SUCCESS;
	}
	
	void Fence::WaitTime(uint64_t timeout, void *&resource)
	{
	    SEDX_ASSERT(vkWaitForFences(RenderContext::Get()->GetLogicDevice()->GetDevice(), 1, reinterpret_cast<VkFence*>(&resource), VK_TRUE, timeout) == VK_SUCCESS);
	}

	void Fence::Reset(void *&resource)
	{
		SEDX_ASSERT(vkResetFences(RenderContext::Get()->GetLogicDevice()->GetDevice(),1, reinterpret_cast<VkFence*>(&resource)) == VK_SUCCESS);
	}
}

// -------------------------------------------------------

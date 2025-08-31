/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* semaphore.cpp
* -------------------------------------------------------
* Created: 28/8/2025
* -------------------------------------------------------
*/
#include "semaphore.h"
#include <SceneryEditorX/logging/asserts.h>
#include "render_context.h"
#include "vulkan/vk_util.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{
	Semaphore::Semaphore(const FrameSyncType type, void *&resource)
	{
	    SEDX_ASSERT(resource == nullptr);
	
	    VkSemaphoreTypeCreateInfo semaphoreCreateInfo = {};
	    semaphoreCreateInfo.sType                     = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
	    semaphoreCreateInfo.pNext                     = nullptr;
	    semaphoreCreateInfo.semaphoreType             = VK_SEMAPHORE_TYPE_TIMELINE;
	    semaphoreCreateInfo.initialValue              = 0;
	
	    VkSemaphoreCreateInfo semaphore_create_info = {};
	    semaphore_create_info.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	    semaphore_create_info.pNext                 = type == FrameSyncType::SyncSemaphoreTimeline ? &semaphoreCreateInfo : nullptr;
	    semaphore_create_info.flags                 = 0;
	
		VK_CHECK_RESULT(vkCreateSemaphore(RenderContext::Get()->GetLogicDevice()->GetDevice(), &semaphore_create_info, nullptr, reinterpret_cast<VkSemaphore*>(&resource)))
	}

    // TODO: Evaluate if the 'Create' function should remain able be called specifically or keep the RAII setup and create it directly in constructor?
    void Semaphore::Create(const FrameSyncType type, void *&resource)
	{
	    SEDX_ASSERT(resource == nullptr);
	
	    VkSemaphoreTypeCreateInfo semaphoreCreateInfo = {};
	    semaphoreCreateInfo.sType                     = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
	    semaphoreCreateInfo.pNext                     = nullptr;
	    semaphoreCreateInfo.semaphoreType             = VK_SEMAPHORE_TYPE_TIMELINE;
	    semaphoreCreateInfo.initialValue              = 0;
	
	    VkSemaphoreCreateInfo semaphore_create_info = {};
	    semaphore_create_info.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	    semaphore_create_info.pNext                 = type == FrameSyncType::SyncSemaphoreTimeline ? &semaphoreCreateInfo : nullptr;
	    semaphore_create_info.flags                 = 0;
	
		VK_CHECK_RESULT(vkCreateSemaphore(RenderContext::Get()->GetLogicDevice()->GetDevice(), &semaphore_create_info, nullptr, reinterpret_cast<VkSemaphore*>(&resource)))
	}
	
	void Semaphore::WaitTime(const uint64_t value, const uint64_t timeout, void *&resource)
	{
	    VkSemaphoreWaitInfo semaphoreWaitInfo = {};
	    semaphoreWaitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
	    semaphoreWaitInfo.pNext = nullptr;
	    semaphoreWaitInfo.flags = 0;
	    semaphoreWaitInfo.semaphoreCount = 1;
	    semaphoreWaitInfo.pSemaphores = reinterpret_cast<VkSemaphore *>(&resource);
	    semaphoreWaitInfo.pValues = &value;
	
		VK_CHECK_RESULT(vkWaitSemaphores(RenderContext::Get()->GetLogicDevice()->GetDevice(), &semaphoreWaitInfo, timeout))
	}
	
	void Semaphore::Signal(const uint64_t value, void *&resource)
	{
	    VkSemaphoreSignalInfo semaphoreInfo = {};
	    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
	    semaphoreInfo.pNext = nullptr;
	    semaphoreInfo.semaphore = static_cast<VkSemaphore>(resource);
	    semaphoreInfo.value = value;
	
		VK_CHECK_RESULT(vkSignalSemaphore(RenderContext::Get()->GetLogicDevice()->GetDevice(), &semaphoreInfo))
	}
	
	uint64_t Semaphore::GetCounterValue(void *&resource)
	{
	    uint64_t value = 0;
	    VK_CHECK_RESULT(vkGetSemaphoreCounterValue(RenderContext::Get()->GetLogicDevice()->GetDevice(), static_cast<VkSemaphore>(resource), &value))
	
		return value;
	}

}

/// -------------------------------------------------------

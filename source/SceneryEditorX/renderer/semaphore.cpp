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
#include "render_context.h"
#include "vulkan/vk_util.h"
#include <SceneryEditorX/logging/asserts.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	Semaphore::Semaphore(const FrameSyncType type, void*& resource) : m_Resource(resource), m_Type(type)
	{
	    SEDX_ASSERT(resource != nullptr, "Semaphore resource cannot be null");
        Create(m_Type, m_Resource);
	}

    void Semaphore::Create(const FrameSyncType type, void*& resource)
	{
	    SEDX_ASSERT(resource == nullptr, "Resource must be null before creation");
	
	    VkSemaphoreTypeCreateInfo semaphoreTypeInfo;
	    semaphoreTypeInfo.sType                     = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
	    semaphoreTypeInfo.pNext                     = nullptr;
	    semaphoreTypeInfo.semaphoreType             = VK_SEMAPHORE_TYPE_TIMELINE;
	    semaphoreTypeInfo.initialValue              = 0;
	
	    VkSemaphoreCreateInfo semaphoreCreateInfo;
	    semaphoreCreateInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	    semaphoreCreateInfo.pNext                 = type == FrameSyncType::SemaphoreTimeline ? &semaphoreTypeInfo : nullptr;
	    semaphoreCreateInfo.flags                 = 0;
	
		VK_CHECK_RESULT(vkCreateSemaphore(RenderContext::Get()->GetLogicDevice()->GetDevice(), &semaphoreCreateInfo,nullptr, reinterpret_cast<VkSemaphore*>(&resource)))
	}
	
	void Semaphore::WaitTime(const uint64_t value, const uint64_t timeout, void*& resource)
	{
	    VkSemaphoreWaitInfo semaphoreWaitInfo;
	    semaphoreWaitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
	    semaphoreWaitInfo.pNext = nullptr;
	    semaphoreWaitInfo.flags = 0;
	    semaphoreWaitInfo.semaphoreCount = 1;
	    semaphoreWaitInfo.pSemaphores = reinterpret_cast<VkSemaphore*>(&resource);
	    semaphoreWaitInfo.pValues = &value;
	
		VK_CHECK_RESULT(vkWaitSemaphores(RenderContext::Get()->GetLogicDevice()->GetDevice(), &semaphoreWaitInfo, timeout))
	}
	
	void Semaphore::Signal(const uint64_t value, void*& resource)
	{
	    VkSemaphoreSignalInfo semaphoreInfo;
	    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
	    semaphoreInfo.pNext = nullptr;
	    semaphoreInfo.semaphore = static_cast<VkSemaphore>(resource);
	    semaphoreInfo.value = value;
	
		VK_CHECK_RESULT(vkSignalSemaphore(RenderContext::Get()->GetLogicDevice()->GetDevice(), &semaphoreInfo))
	}
	
	uint64_t Semaphore::GetCounterValue(void*& resource)
	{
	    uint64_t value = 0;
	    VK_CHECK_RESULT(vkGetSemaphoreCounterValue(RenderContext::Get()->GetLogicDevice()->GetDevice(), static_cast<VkSemaphore>(resource), &value))
		return value;
	}

}

// -------------------------------------------------------

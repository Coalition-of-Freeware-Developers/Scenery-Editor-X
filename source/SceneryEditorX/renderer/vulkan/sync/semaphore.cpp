/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray 
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * semaphore.cpp
 * -------------------------------------------------------
 * Created: 26/02/2026
 * -------------------------------------------------------
 */
#include "semaphore.h"
#include <SceneryEditorX/renderer/vulkan/render_context.h>
#include <SceneryEditorX/renderer/vulkan/debug/graphics_debug.h>
#include <volk/volk.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	
	Semaphore::Semaphore()
	{
		m_Type = SyncType::Semaphore;
	}

	Semaphore::Semaphore(const SyncType type)
	{
		m_Type = type;
	}

	Semaphore::~Semaphore()
	{
	   Destroy();
	}

	void Semaphore::CreateSyncObject()
	{
	   SEDX_CORE_ASSERT(m_Semaphore == VK_NULL_HANDLE);
		Ref<Device> device = RenderContext::Get()->GetDevice();

		VkSemaphoreTypeCreateInfo typeInfo{};
		typeInfo.sType				= VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
		typeInfo.pNext				= nullptr;
		typeInfo.semaphoreType		= VK_SEMAPHORE_TYPE_TIMELINE;
		typeInfo.initialValue		= 0;

		VkSemaphoreCreateInfo semaphoreCreateInfo{};
		semaphoreCreateInfo.sType	= VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreCreateInfo.pNext	= m_Type == SyncType::SemaphoreTimeline ? &typeInfo : nullptr;
		semaphoreCreateInfo.flags	= 0;

		SEDX_VK_RESULT_ASSERT(vkCreateSemaphore(device->GetLogicalDevice(), &semaphoreCreateInfo, nullptr, &m_Semaphore),
			"Failed to create semaphore");

		Debugging::SetResourceName(m_Semaphore, ResourceType::Semaphore, m_ObjectName.c_str());
		SEDX_CORE_TRACE_TAG("Semaphore", "Semaphore {} scheduled for destruction", m_ObjectName);
	}

	void Semaphore::CreateSyncObject(const VkDevice device)
	{
		SEDX_CORE_ASSERT(device != VK_NULL_HANDLE, "Semaphore::CreateSyncObject(VkDevice): device must be valid");
		SEDX_CORE_ASSERT(m_Semaphore == VK_NULL_HANDLE, "Semaphore already created");

		VkSemaphoreTypeCreateInfo typeInfo{};
		typeInfo.sType         = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
		typeInfo.pNext         = nullptr;
		typeInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
		typeInfo.initialValue  = 0;

		VkSemaphoreCreateInfo semaphoreCreateInfo{};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreCreateInfo.pNext = m_Type == SyncType::SemaphoreTimeline ? &typeInfo : nullptr;
		semaphoreCreateInfo.flags = 0;

		SEDX_VK_RESULT_ASSERT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &m_Semaphore),
			"Failed to create semaphore");

		Debugging::SetResourceName(device, m_Semaphore, ResourceType::Semaphore, m_ObjectName.c_str());
	}

	void Semaphore::Wait(const uint64_t timeout)
	{
		Ref<Device> device = RenderContext::Get()->GetDevice();

		VkSemaphoreWaitInfo waitInfo;
		waitInfo.sType               = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
		waitInfo.pNext               = nullptr;
		waitInfo.flags               = 0;
		waitInfo.semaphoreCount      = 1;
		waitInfo.pSemaphores         = &m_Semaphore;
		waitInfo.pValues             = &m_Value;

		VkResult result = vkWaitSemaphores(device->GetLogicalDevice(), &waitInfo, timeout);
		if (result == VK_ERROR_DEVICE_LOST)
		{
			Device::SetDeviceLost();
		}
		SEDX_VK_RESULT_ASSERT(result, "Failed to wait for semaphore");
	}

	void Semaphore::Signal(const uint64_t value, const VkSemaphore &semaphore)
	{
		Ref<Device> device = RenderContext::Get()->GetDevice();

		VkSemaphoreSignalInfo signalInfo;
		signalInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
		signalInfo.pNext                 = nullptr;
		signalInfo.semaphore             = semaphore;
		signalInfo.value                 = value;
		
		SEDX_VK_RESULT_ASSERT(vkSignalSemaphore(device->GetLogicalDevice(), &signalInfo), "Failed to signal semaphore");
	}

	bool Semaphore::IsSignaled()
	{
		return GetValue(m_Semaphore) == m_Value;
	}

	void Semaphore::Destroy()
	{
	   if (m_Semaphore == VK_NULL_HANDLE)
		{
			return;
		}

		QueueManager::AddDeletionQueue(ResourceType::Semaphore, m_Semaphore);
		SEDX_CORE_TRACE_TAG("Semaphore", "Semaphore {} scheduled for destruction", m_ObjectName);
	   m_Semaphore = VK_NULL_HANDLE;
	}

	uint64_t Semaphore::GetValue(const VkSemaphore &semaphore)
	{
		Ref<Device> device = RenderContext::Get()->GetDevice();
		uint64_t value = 0;
		SEDX_VK_RESULT_ASSERT(vkGetSemaphoreCounterValue(device->GetLogicalDevice(), semaphore, &value));

		return value;
	}
	
} // namespace SceneryEditorX

// -------------------------------------------------------

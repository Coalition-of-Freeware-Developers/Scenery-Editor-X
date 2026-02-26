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
 * fence.cpp
 * -------------------------------------------------------
 * Created: 26/02/2026
 * -------------------------------------------------------
 */
#include "fence.h"
#include "graphics_debug.h"
#include "render_context.h"
#include <volk/volk.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	
	Fence::Fence()
	{
        m_Type = SyncType::Fence;
	}

	Fence::~Fence()
	{
        if (!m_Fence)
            return;

		m_Fence = VK_NULL_HANDLE;
	}

	void Fence::CreateSyncObject()
	{
        Ref<Device> device = RenderContext::Get()->GetDevice();
	    VkFenceCreateInfo fenceInfo = {};
	    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	    SEDX_VK_RESULT_ASSERT(vkCreateFence(device->GetLogicalDevice(), &fenceInfo, nullptr, &m_Fence), "Failed to create fence");
	}

	void Fence::Wait(const uint64_t timeout, const VkFence &fence)
	{
	    Ref<Device> device = RenderContext::Get()->GetDevice();
	    VkResult result = vkWaitForFences(device->GetLogicalDevice(), 1, &fence, true, timeout);
        if (result == VK_ERROR_DEVICE_LOST)
        {
            Device::SetDeviceLost();
        }
        SEDX_VK_RESULT_ASSERT(result, "Failed to wait for fence")
	}

	bool Fence::IsSignaled(const VkFence &fence)
    {
	    Ref<Device> device = RenderContext::Get()->GetDevice();
        return vkGetFenceStatus(device->GetLogicalDevice(), fence) == VK_SUCCESS;
    }

    void Fence::Destroy()
	{
	    QueueManager::AddDeletionQueue(ResourceType::Fence, m_Fence);
	    SEDX_CORE_TRACE_TAG("Fence", "Fence {} scheduled for destruction", m_ObjectName);
	}

    void Fence::Reset(const VkFence &fence)
	{
		Ref<Device> device = RenderContext::Get()->GetDevice();
		SEDX_VK_RESULT_ASSERT(vkResetFences(device->GetLogicalDevice(), 1, &fence));
    }
	
} // namespace SceneryEditorX

// -------------------------------------------------------

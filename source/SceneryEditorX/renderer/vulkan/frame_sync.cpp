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
 * frame_sync.cpp
 * -------------------------------------------------------
 * Created: 12/02/2026
 * -------------------------------------------------------
 */
#include "frame_sync.h"

// -------------------------------------------------------

namespace SceneryEditorX
{

	FrameSync::FrameSync(uint32_t framesInFlight, uint32_t swapchainImageCount)
	{
	    Create(framesInFlight, swapchainImageCount);
	}
	
	FrameSync::~FrameSync()
	{
	    if (!m_Destroyed)
	    {
	        Destroy(); // Best-effort cleanup; explicit Destroy() is preferred.
	    }
	}
	
	void FrameSync::Create(uint32_t framesInFlight, uint32_t swapchainImageCount)
	{
	    Ref<Device> device = RenderContext::Get()->GetDevice();
	
	    m_Fences.resize(framesInFlight, VK_NULL_HANDLE);
	    m_PresentSemaphores.resize(framesInFlight, VK_NULL_HANDLE);
	
	    VkSemaphoreCreateInfo semaphoreCI{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
	
	    VkFenceCreateInfo fenceCI{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT};
	
	    for (uint32_t i = 0; i < framesInFlight; ++i)
	    {
	        VkResult r = vkCreateFence(device->GetLogicalDevice(), &fenceCI, nullptr, &m_Fences[i]);
	        SEDX_CORE_ASSERT(r == VK_SUCCESS, "vkCreateFence failed: {}", r);
	
	        r = vkCreateSemaphore(device->GetLogicalDevice(), &semaphoreCI, nullptr, &m_PresentSemaphores[i]);
	        SEDX_CORE_ASSERT(r == VK_SUCCESS, "vkCreateSemaphore failed: {}", r);
	    }
	
	    m_RenderSemaphores.resize(swapchainImageCount, VK_NULL_HANDLE);
	    for (auto &s : m_RenderSemaphores)
	    {
	        SEDX_CORE_ASSERT(s == VK_NULL_HANDLE, "Expected uninitialized semaphore handle");
	        if (VkResult r = vkCreateSemaphore(device->GetLogicalDevice(), &semaphoreCI, nullptr, &s); r != VK_SUCCESS)
	        {
	            SEDX_CORE_ERROR_TAG("FrameSync", "vkCreateSemaphore failed with error code: {}", r);
	        }
	    }
	}

	void FrameSync::Destroy()
	{
	    Ref<Device> device = RenderContext::Get()->GetDevice();
	
	    for (auto &s : m_RenderSemaphores)
	    {
	        if (s != VK_NULL_HANDLE)
	        {
	            vkDestroySemaphore(device->GetLogicalDevice(), s, nullptr);
	            s = VK_NULL_HANDLE;
	        }
	    }
	    m_RenderSemaphores.clear();
	
	    for (auto &s : m_PresentSemaphores)
	    {
	        if (s != VK_NULL_HANDLE)
	        {
	            vkDestroySemaphore(device->GetLogicalDevice(), s, nullptr);
	            s = VK_NULL_HANDLE;
	        }
	    }
	
	    for (auto &f : m_Fences)
	    {
	        if (f != VK_NULL_HANDLE)
	        {
	            vkDestroyFence(device->GetLogicalDevice(), f, nullptr);
	            f = VK_NULL_HANDLE;
	        }
	    }
	
	    m_Destroyed = true;
	}
	
	
} // namespace SceneryEditorX

// -------------------------------------------------------

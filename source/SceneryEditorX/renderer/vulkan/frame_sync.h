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
 * frame_sync.h
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include "render_context.h"


#include <cstdint>
#include <iostream>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX
{
	// Non-templated FrameSync owning fences and semaphores. The caller should
	// call destroy(m_Device) before destroying the VkDevice to guarantee safe
	// teardown ordering.
	class FrameSync 
    {
	public:
	    FrameSync() = default;
	    FrameSync(uint32_t framesInFlight, uint32_t swapchainImageCount)
	    {
	        Create(framesInFlight, swapchainImageCount);
	    }
	
	    ~FrameSync()
	    {
	        if (!m_Destroyed && m_Device != VK_NULL_HANDLE)
			{
	            Destroy(); // Best-effort cleanup; explicit Destroy() is preferred.
	        }
	    }
	
	    void Create(uint32_t framesInFlight, uint32_t swapchainImageCount)
	    {
	        m_Device = RenderContext::Get()->GetDevice();

	        m_Fences.resize(framesInFlight, VK_NULL_HANDLE);
	        m_PresentSemaphores.resize(framesInFlight, VK_NULL_HANDLE);
	
	        VkSemaphoreCreateInfo semaphoreCI
	        {
	            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO 
	        };

	        VkFenceCreateInfo fenceCI
	        {
	            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, 
	            .flags = VK_FENCE_CREATE_SIGNALED_BIT 
	        };
	
	        for (uint32_t i = 0; i < framesInFlight; ++i)
			{
                VkResult r = vkCreateFence(Device::GetDevice(), &fenceCI, nullptr, &m_Fences[i]);
                SEDX_CORE_ASSERT(r == VK_SUCCESS, "vkCreateFence failed: " + ToString(r));

	            r = vkCreateSemaphore(Device::GetDevice(), &semaphoreCI, nullptr, &m_PresentSemaphores[i]);
                SEDX_CORE_ASSERT(r == VK_SUCCESS, "vkCreateSemaphore failed: " + ToString(r));
	        }
	
	        m_RenderSemaphores.resize(swapchainImageCount, VK_NULL_HANDLE);
	        for (auto& s : m_RenderSemaphores)
			{
                SEDX_CORE_ASSERT(s == VK_NULL_HANDLE, "Expected uninitialized semaphore handle");
                if (VkResult r = vkCreateSemaphore(Device::GetDevice(), &semaphoreCI, nullptr, &s); r != VK_SUCCESS)
	            {
                    SEDX_CORE_ERROR_TAG("FrameSync", "vkCreateSemaphore failed with error code: {}", r);
	            }
	        }
	    }
	
	    void Destroy()
	    {
	        for (auto& s : m_RenderSemaphores)
			{
	            if (s != VK_NULL_HANDLE)
				{
                    vkDestroySemaphore(Device::GetDevice(), s, nullptr);
	                s = VK_NULL_HANDLE;
	            }
	        }
	        m_RenderSemaphores.clear();
	
	        for (auto& s : m_PresentSemaphores)
			{
	            if (s != VK_NULL_HANDLE)
				{
                    vkDestroySemaphore(Device::GetDevice(), s, nullptr);
	                s = VK_NULL_HANDLE;
	            }
	        }
	
	        for (auto& f : m_Fences)
			{
	            if (f != VK_NULL_HANDLE)
				{
                    vkDestroyFence(Device::GetDevice(), f, nullptr);
	                f = VK_NULL_HANDLE;
	            }
	        }
	
	        m_Destroyed = true;
	        m_Device.Reset();
	    }
	
	    std::vector<VkFence>& Fences() { return m_Fences; }
	    std::vector<VkSemaphore>& PresentSemaphores() { return m_PresentSemaphores; }
	    std::vector<VkSemaphore>& RenderSemaphores() { return m_RenderSemaphores; }
	
	private:
	    Ref<Device> m_Device;
	    std::vector<VkFence> m_Fences{};
	    std::vector<VkSemaphore> m_PresentSemaphores{};
	    std::vector<VkSemaphore> m_RenderSemaphores{};
	    bool m_Destroyed = false;
	};
	
	// Small helper to create fences and semaphores and populate caller-owned
	// containers. Implemented as a header-only template for convenience.
	template <size_t N>
	inline void CreateSyncObjects(uint32_t swapchainImageCount, std::array<VkFence, N>& fences, std::array<VkSemaphore, N>& presentSemaphores, std::vector<VkSemaphore>& renderSemaphores)
	{
        VkDevice device = RenderContext::Get()->GetDevice()->GetDevice();
	    VkSemaphoreCreateInfo semaphoreCI
	    {
	        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO 
	    };

	    VkFenceCreateInfo fenceCI
	    { 
	        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, 
	        .flags = VK_FENCE_CREATE_SIGNALED_BIT 
	    };
	
	    for (size_t i = 0; i < N; ++i)
		{
	        vkCreateFence(device, &fenceCI, nullptr, &fences[i]);
	        vkCreateSemaphore(device, &semaphoreCI, nullptr, &presentSemaphores[i]);
	    }

	    renderSemaphores.resize(swapchainImageCount);
	    for (auto& s : renderSemaphores)
		{
	        vkCreateSemaphore(device, &semaphoreCI, nullptr, &s);
	    }
	}

}

// -------------------------------------------------------

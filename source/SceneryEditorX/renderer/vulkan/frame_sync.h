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
#include <cstdint>
#include <vector>
#include <SceneryEditorX/core/resource/iobject.h>
#include <volk/volk.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

    class RenderContext;

	// Non-templated FrameSync owning fences and semaphores. The caller should
	// call destroy(m_Device) before destroying the VkDevice to guarantee safe
	// teardown ordering.
    class FrameSync : public RefCounted, public IObject
    {
	public:
        /**
	     * @brief Construct a new FrameSync object with the specified number of frames in flight and swapchain images.
	     * @param framesInFlight Number of frames that can be processed concurrently.
	     * @param swapchainImageCount Number of images in the swapchain.
	     */
	    FrameSync(uint32_t framesInFlight, uint32_t swapchainImageCount);
        FrameSync() = default;
        virtual ~FrameSync() override;

        /**
         * @brief Create synchronization objects for the specified number of frames in flight and swapchain images.
         * @param framesInFlight Number of frames that can be processed concurrently.
         * @param swapchainImageCount Number of images in the swapchain.
         */
        void Create(uint32_t framesInFlight, uint32_t swapchainImageCount);

        /**
         * @brief Destroy the synchronization objects created by this FrameSync instance.
         * This should be called before destroying the VkDevice to ensure proper cleanup of Vulkan resources.
         */
        void Destroy();

        std::vector<VkFence>& Fences() { return m_Fences; }
	    std::vector<VkSemaphore>& PresentSemaphores() { return m_PresentSemaphores; }
	    std::vector<VkSemaphore>& RenderSemaphores() { return m_RenderSemaphores; }
	
	private:
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
